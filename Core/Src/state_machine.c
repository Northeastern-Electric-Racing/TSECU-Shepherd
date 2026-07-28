#include "state_machine.h"
#include "c_utils.h"
#include "can_messages_tx.h"
#include "charging.h"
#include "compute.h"
#include "segment.h"
#include "charging.h"
#include "c_utils.h"
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <stdatomic.h>
#include "app_threadx.h"
#include "shep_mutexes.h"
#include "u_tx_threads.h"

static fault_eval_t fault_eval_table[NUM_FAULTS];

static _Atomic uint32_t severity_mask = 0;
static _Atomic uint32_t fault_flags = 0;

#define LONG_CHARGE_DURATION_MS  (15U * 60U * 1000U)
#define SHORT_CHARGE_DURATION_MS (20U * 1000U)
#define CHARGE_SETTLE_DURATION_MS (60U * 1000U)

const bool valid_transition_from_to[NUM_STATES][NUM_STATES] = {
	/*   BOOT, READY, CHARGING, FAULTED */
	{ true, true, true, true }, /* BOOT */
	{ false, true, true, true }, /* READY */
	{ false, true, true, true }, /* CHARGING */
	{ true, false, false, true } /* FAULTED */
};

/* private function prototypes */
void update_eval_table(state_machine_args_t *state_machine_args);

static void set_charging_stage(state_machine_t *state_machine,
			       charge_stage_t stage);

void request_transition(state_machine_args_t *state_machine_args,
			state_t next_state);

typedef void (*HandlerFunction_t)(state_machine_args_t *state_machine_args);
typedef void (*InitFunction_t)(state_machine_args_t *state_machine_args);

const InitFunction_t init_LUT[NUM_STATES] = { &init_boot, &init_ready,
					      &init_charging, &init_faulted };

const HandlerFunction_t handler_LUT[NUM_STATES] = { &handle_boot, &handle_ready,
						    &handle_charging,
						    &handle_faulted };

static void set_charging_stage(state_machine_t *state_machine,
			       charge_stage_t stage)
{
	state_machine->charging_stage = stage;

	switch (stage) {
		case LONG_CHARGE_UP:
			start_timer(&state_machine->charging_stage_timer,
				    LONG_CHARGE_DURATION_MS);
			break;
		case SHORT_CHARGE_UP:
			start_timer(&state_machine->charging_stage_timer,
				    SHORT_CHARGE_DURATION_MS);
			break;
		case LONG_SETTLE:
		case SHORT_SETTLE:
			start_timer(&state_machine->charging_stage_timer,
				    CHARGE_SETTLE_DURATION_MS);
			break;
		case DONE:
		case FAULT:
			break;
	}
}

void init_boot(state_machine_args_t *state_machine_args)
{
	state_machine_args->state_machine->bms_state = BOOT;
	state_machine_args->state_machine->balancing_active = false;
	state_machine_args->state_machine->is_charger_connected = false;
	cancel_timer(&state_machine_args->state_machine->charger_message_timer);

	update_eval_table(
		state_machine_args); // initialize eval table with crit and non crit faults

	for (int fault_id = 0; fault_id < NUM_FAULTS; fault_id++) {
		/* Initialize severity_mask. */
		if (fault_eval_table[fault_id].is_critical) {
			atomic_fetch_or(&severity_mask,
					((uint32_t)1 << fault_id));
		}
	}
}

void handle_boot(state_machine_args_t *state_machine_args)
{
	request_transition(state_machine_args, READY);
}

void init_ready(state_machine_args_t *state_machine_args)
{
	state_machine_args->state_machine->charger_output_disabled = true;
	compute_set_fault(false);
	return;
}

void handle_ready(state_machine_args_t *state_machine_args)
{
	if (state_machine_args->state_machine->is_charger_connected) {
		request_transition(state_machine_args, CHARGING);
	}

	return;
}

void init_charging(state_machine_args_t *state_machine_args)
{
	set_charging_stage(state_machine_args->state_machine, LONG_CHARGE_UP);
	state_machine_args->state_machine->charger_output_disabled = false;

	send_max_dc_current_command(0);
	send_max_dc_brake_current_command(0);
}

void handle_charging(state_machine_args_t *state_machine_args)
{
	/* Check if we should charge */
	if (sm_charging_check(state_machine_args)) {
		state_machine_args->state_machine->charger_output_disabled = false;

		/* Send CAN message, but not too often */
		if (is_timer_expired(&state_machine_args->state_machine
					     ->charger_message_timer) ||
		    !is_timer_active(&state_machine_args->state_machine
					     ->charger_message_timer)) {
			send_bms_charge_message_send((MAX_CHARGE_VOLT *
						      (NUM_CELLS_PER_CHIP * 2) *
						      NUM_SEGMENTS),
						     CHARGING_CURRENT, 0x0);
			start_timer(&state_machine_args->state_machine
					     ->charger_message_timer,
				    1000);
		}
	} else {
		state_machine_args->state_machine->charger_output_disabled = true;
		send_bms_charge_message_send(0, 0, 0xFF);
	}

	// disable discharge and charge from the MC
	send_max_dc_current_command(0);
	send_max_dc_brake_current_command(0);

	/* Check if we should balance */
	if (sm_balancing_check(state_machine_args)) {
		handle_balance_cells(state_machine_args->analyzer,
				     state_machine_args->acc_data);
		state_machine_args->state_machine->balancing_active = true;
	} else {
		state_machine_args->state_machine->balancing_active = false;
	}
}

void charger_message_recieved(state_machine_args_t *state_machine_args)
{
	// this is irreversible, a LV power cycle occurs before re-connection to car
	state_machine_args->state_machine->is_charger_connected = true;
	request_transition(state_machine_args, CHARGING);
}

void init_faulted(state_machine_args_t *state_machine_args)
{
	state_machine_args->state_machine->charger_output_disabled = true;
	send_max_dc_current_command(0);
	send_max_dc_brake_current_command(0);
	send_bms_charge_message_send(0, 0, 0xFF);
	state_machine_args->state_machine->balancing_active = false;
	compute_set_fault(true);
}

void handle_faulted(state_machine_args_t *state_machine_args)
{
	compute_set_fault(true);
	// leave faulted if all is well
	if (!are_critical_faults_active()) {
		request_transition(state_machine_args, BOOT);
		return;
	}
}

void sm_handle_state(state_machine_args_t *state_machine_args)
{
	// always check for faults no matter the current state
	sm_fault_return(state_machine_args);

	if (are_critical_faults_active()) {
		request_transition(state_machine_args, FAULTED);
	}

	handler_LUT[state_machine_args->state_machine->bms_state](
		state_machine_args);
}

void request_transition(state_machine_args_t *state_machine_args,
			state_t next_state)
{
	state_machine_t *state_machine = state_machine_args->state_machine;
	bool transition_requested = false;

	mutex_get(&state_mutex);

	if ((state_machine->bms_state != next_state) &&
	    valid_transition_from_to[state_machine->bms_state][next_state]) {
		state_machine->bms_state = next_state;
		transition_requested = true;
	}

	mutex_put(&state_mutex);

	if (transition_requested) {
		init_LUT[next_state](state_machine_args);
	}
}

void sm_fault_return(state_machine_args_t *state_machine_args)
{
	uint32_t fault_mask;
	fault_state_t fault_state = FAULT_NONE;

	/* FAULT CHECK */
	update_eval_table(state_machine_args);

	for (uint32_t fault_id = 0U; fault_id < (uint32_t)NUM_FAULTS;
	     fault_id++) {
		fault_mask = ((uint32_t)1U << fault_id);

		fault_state = sm_fault_eval(&fault_eval_table[fault_id],
					    (fault_code_t)fault_id);

		if (fault_state == FAULT_NONE) {
			(void)atomic_fetch_and(&fault_flags, ~fault_mask);
		} else if (fault_state == FAULT_TRIGGERED) {
			if (fault_eval_table[fault_id].is_critical) {
				send_bms_critically_faulted(true);
			}

			(void)atomic_fetch_or(&fault_flags, fault_mask);
		} else {
			/* FAULT_ONGOING: do nothing */
		}
	}
}

fault_state_t sm_fault_eval(fault_eval_t *item, fault_code_t fault_code)
{
	bool condition1 = false;
	bool condition2 = false;
	bool fault_present = false;
	bool timer_active = false;
	fault_state_t fault_state = FAULT_NONE;

	switch (item->optype_1) {
		case GT:
			condition1 = item->data_1 > item->lim_1;
			break;
		case LT:
			condition1 = item->data_1 < item->lim_1;
			break;
		case GE:
			condition1 = item->data_1 >= item->lim_1;
			break;
		case LE:
			condition1 = item->data_1 <= item->lim_1;
			break;
		case EQ:
			condition1 = item->data_1 == item->lim_1;
			break;
		case NEQ:
			condition1 = item->data_1 != item->lim_1;
			break;
		case NOP:
			condition1 = false;
			break;
		default:
			condition1 = false;
			break;
	}

	switch (item->optype_2) {
		case GT:
			condition2 = item->data_2 > item->lim_2;
			break;
		case LT:
			condition2 = item->data_2 < item->lim_2;
			break;
		case GE:
			condition2 = item->data_2 >= item->lim_2;
			break;
		case LE:
			condition2 = item->data_2 <= item->lim_2;
			break;
		case EQ:
			condition2 = item->data_2 == item->lim_2;
			break;
		case NEQ:
			condition2 = item->data_2 != item->lim_2;
			break;
		case NOP:
			condition2 = false;
			break;
		default:
			condition2 = false;
			break;
	}

	fault_present = (condition1 && condition2) ||
			(condition1 && (item->optype_2 == NOP));

	timer_active = is_timer_active(&item->timer);

	if (fault_present == false) {
		if (timer_active) {
			PRINTLN_INFO("\tFault cleared: %s\n", item->id);

			cancel_timer(&item->timer);

			/* STOPPING TIMER MESSSAGE */
			send_bms_fault_timers(FAULT_TIMER_STOPPED, fault_code,
					      item->data_1);
		} else {
			/* Timer is already inactive. */
		}

		fault_state = FAULT_NONE;
	} else {
		if (timer_active == false) {
			PRINTLN_INFO("\tStarting Fault Timer: %s\n", item->id);

			start_timer(&item->timer, item->timeout);

			/* STARTING FAULTED TIMER MESSAGE */
			send_bms_fault_timers(FAULT_TIMER_STARTED, fault_code,
					      item->data_1);

			fault_state = FAULT_ONGOING;
		} else {
			if (is_timer_expired(&item->timer)) {
				PRINTLN_INFO("\tFaulted: %s\n", item->id);

				/* FAULT TIMER EXPIRED MESSAGE */
				send_bms_fault_timers(FAULT_TIMER_EXPIRED,
						      fault_code, item->data_1);

				cancel_timer(&item->timer);

				fault_state = FAULT_TRIGGERED;
			} else {
				fault_state = FAULT_ONGOING;
			}
		}
	}

	return fault_state;
}

/* This charging algorithm has 3 stages
 * 1. Charge for up to 15 minutes, pause for 1 minute, and repeat.
 * 2. Once loaded max cell voltage reaches 4.19 V, charge for up to 20 seconds,
 *    pause for 1 minute, and repeat while settled max OCV is below 4.19 V.
 * 3. Stop charging when settled max OCV reaches 4.19 V.
 */
bool sm_charging_check(state_machine_args_t *state_machine_args)
{
	state_machine_t *state_machine = state_machine_args->state_machine;
	analyzer_t *analyzer = state_machine_args->analyzer;
	nertimer_t *state_timer = &state_machine->charging_stage_timer;
	charge_stage_t next_stage = state_machine->charging_stage;
	bool charging_allowed = false;
	float max_ocv = analyzer->max_ocv.val;
	float max_voltage = analyzer->max_voltage.val;

	if (max_ocv >= MAX_CHARGE_VOLT_FLT ||
	    max_voltage >= MAX_CHARGE_VOLT_FLT) {
		set_charging_stage(state_machine, FAULT);
		PRINTLN_INFO("Max OCV: %f", (double)max_ocv);
		PRINTLN_INFO("Max Volts: %f", (double)max_voltage);
	} else {
		switch (state_machine->charging_stage) {
			case LONG_CHARGE_UP:
				if (max_voltage >= MAX_CHARGE_VOLT) {
					next_stage = SHORT_SETTLE;
				} else if (is_timer_expired(state_timer)) {
					next_stage = LONG_SETTLE;
				}
				break;
			case LONG_SETTLE:
				if (is_timer_expired(state_timer)) {
					next_stage =
						(max_ocv >= MAX_CHARGE_VOLT) ?
							DONE : LONG_CHARGE_UP;
				}
				break;
			case SHORT_CHARGE_UP:
				if (max_voltage >= MAX_CHARGE_VOLT ||
				    is_timer_expired(state_timer)) {
					next_stage = SHORT_SETTLE;
				}
				break;
			case SHORT_SETTLE:
				if (is_timer_expired(state_timer)) {
					next_stage =
						(max_ocv >= MAX_CHARGE_VOLT) ?
							DONE : SHORT_CHARGE_UP;
				}
				break;
			case DONE:
			case FAULT:
				break;
		}

		if (next_stage != state_machine->charging_stage) {
			set_charging_stage(state_machine, next_stage);
		}

		charging_allowed =
			state_machine->charging_stage == LONG_CHARGE_UP ||
			state_machine->charging_stage == SHORT_CHARGE_UP;
	}

	return charging_allowed;
}

// check if balancing is allowed
bool sm_balancing_check(state_machine_args_t *state_machine_args)
{
	analyzer_t *analyzer = state_machine_args->analyzer;
	state_machine_t *state_machine = state_machine_args->state_machine;
	bool balancing_allowed = false;
	bool shutdown_active = true;
	float max_voltage = 0.0f;
	float delta_voltage = 0.0f;
	charge_stage_t charging_stage = FAULT;

	mutex_get(&analyzer_mutex);
	max_voltage = analyzer->max_voltage.val;
	delta_voltage = analyzer->delta_voltage;
	mutex_put(&analyzer_mutex);

	mutex_get(&state_mutex);
	charging_stage = state_machine->charging_stage;
	mutex_put(&state_mutex);

	if ((max_voltage <= BAL_MIN_V) || (delta_voltage <= MAX_DELTA_V) ||
	    (charging_stage == LONG_SETTLE) ||
	    (charging_stage == SHORT_SETTLE)) {
		balancing_allowed = false;
	} else {
		// Do not balance if the shutdown circuit is open.
		mutex_get(&shutdown_mutex);
		shutdown_active =
			state_machine_args->peripherals->shutdown_active;
		mutex_put(&shutdown_mutex);

		balancing_allowed = shutdown_active;
	}

	return balancing_allowed;
}

void set_segment_comms_fault(state_machine_t *state_mach)
{
	mutex_get(&state_mutex);
	state_mach->segment_comms_fault_flag = true;
	mutex_put(&state_mutex);
}

void clear_segment_comms_fault(state_machine_t *state_mach)
{
	mutex_get(&state_mutex);
	state_mach->segment_comms_fault_flag = false;
	mutex_put(&state_mutex);
}

void set_hv_plate_comms_fault(state_machine_t *state_mach)
{
	mutex_get(&state_mutex);
	state_mach->hv_plate_comms_fault_flag = true;
	mutex_put(&state_mutex);
}

void clear_hv_plate_comms_fault(state_machine_t *state_mach)
{
	mutex_get(&state_mutex);
	state_mach->hv_plate_comms_fault_flag = false;
	mutex_put(&state_mutex);
}

void set_cell_open_wire_fault(state_machine_t *state_mach)
{
	mutex_get(&state_mutex);
	state_mach->cell_open_wire_fault_flag = true;
	mutex_put(&state_mutex);
}

void clear_cell_open_wire_fault(state_machine_t *state_mach)
{
	mutex_get(&state_mutex);
	state_mach->cell_open_wire_fault_flag = false;
	mutex_put(&state_mutex);
}

bool get_fault(fault_code_t fault)
{
	return (fault_flags & (1 << fault)) != 0;
}

bool are_critical_faults_active(void)
{
	return (fault_flags & severity_mask) != 0;
}

void update_eval_table(state_machine_args_t *state_machine_args)
{
	static bool initialized = false;

	bms_algos_t *bms_algos = state_machine_args->bms_algos;
	hv_plate_t *hv_plate = state_machine_args->hv_plate;
	state_machine_t *state_machine = state_machine_args->state_machine;
	analyzer_t *analyzer = state_machine_args->analyzer;
	sanitizer_t *sanitizer = state_machine_args->sanitizer;

	static nertimer_t ovr_curr_timer = { 0 };
	static nertimer_t ovr_chgcurr_timer = { 0 };
	static nertimer_t undr_volt_timer = { 0 };
	static nertimer_t ovr_chgvolt_timer = { 0 };
	static nertimer_t ovr_volt_timer = { 0 };
	static nertimer_t high_temp_timer = { 0 };
	static nertimer_t die_overtemp_timer = { 0 };
	static nertimer_t segment_comms_timer = { 0 };
	static nertimer_t hv_plate_comms_timer = { 0 };
	static nertimer_t open_wire_timer = { 0 };

	mutex_get(&state_mutex);
	// clang-format off
	const bool segment_comms_fault  = state_machine->segment_comms_fault_flag;
	const bool hv_plate_comms_fault = state_machine->hv_plate_comms_fault_flag;
	const bool ow_fault             = state_machine->cell_open_wire_fault_flag;
	// clang-format on
	mutex_put(&state_mutex);

	if (initialized) {
		// clang-format off
		fault_eval_table[DISCHARGE_LIMIT_ENFORCEMENT_FAULT].data_1 = hv_plate->pack_current;
		fault_eval_table[DISCHARGE_LIMIT_ENFORCEMENT_FAULT].lim_1  = bms_algos->cont_DCL;
		fault_eval_table[CHARGE_LIMIT_ENFORCEMENT_FAULT]   .data_1 = hv_plate->pack_current;
		fault_eval_table[CHARGE_LIMIT_ENFORCEMENT_FAULT]   .lim_1  = (-1.0f * bms_algos->cont_CCL);
		fault_eval_table[CELL_VOLTAGE_TOO_LOW]             .data_1 = analyzer->min_ocv.val;
		fault_eval_table[CELL_VOLTAGE_TOO_HIGH]            .data_1 = analyzer->max_ocv.val;
		fault_eval_table[CELL_CHARGE_VOLTAGE_TOO_HIGH]     .data_1 = analyzer->max_ocv.val;
		fault_eval_table[CELL_CHARGE_VOLTAGE_TOO_HIGH]     .data_2 = (state_machine->bms_state == CHARGING);
		fault_eval_table[PACK_TOO_HOT]                     .data_1 = sanitizer->max_sanitized_temp.val;
		fault_eval_table[DIE_TEMP_MAXIMUM_FAULT]           .data_1 = analyzer->max_chiptemp.val;
		fault_eval_table[SEGMENT_COMMS_FAULT]              .data_1 = segment_comms_fault;
		fault_eval_table[HV_PLATE_COMMS_FAULT]             .data_1 = hv_plate_comms_fault;
		fault_eval_table[CELL_OPEN_WIRE_FAULT]             .data_1 = ow_fault;
		// clang-format on
	} else {
		// clang-format off
		// FAULT_CODE_________________________________________________________FAULT_ID__________________________TIMER__________________________DATA_1_________________________________________OPERATOR_1______LIMIT_1_____________________________________TIMER_LENGTH____________________OPERATOR_2_______DATA_2____________________________________________LIMIT_2________CRITICAL_______________
		fault_eval_table[DISCHARGE_LIMIT_ENFORCEMENT_FAULT] = (fault_eval_t){ .id = "Discharge Current Limit",  .timer = ovr_curr_timer,       .data_1 = hv_plate->pack_current,              .optype_1 = GT, .lim_1 = bms_algos->cont_DCL,               .timeout = OVER_CURR_TIME,      .optype_2 = NOP,                                                                  .is_critical = true  };
		fault_eval_table[CHARGE_LIMIT_ENFORCEMENT_FAULT]    = (fault_eval_t){ .id = "Charge Current Limit",     .timer = ovr_chgcurr_timer,    .data_1 = hv_plate->pack_current,              .optype_1 = LT, .lim_1 = (-1.0f * bms_algos->cont_CCL),     .timeout = OVER_CHG_CURR_TIME,  .optype_2 = NOP,                                                                  .is_critical = true  };
		fault_eval_table[CELL_VOLTAGE_TOO_LOW]              = (fault_eval_t){ .id = "Low Cell Voltage",         .timer = undr_volt_timer,      .data_1 = analyzer->min_ocv.val,               .optype_1 = LT, .lim_1 = MIN_VOLT,                          .timeout = UNDER_VOLT_TIME,     .optype_2 = NOP,                                                                  .is_critical = true  };
		fault_eval_table[CELL_VOLTAGE_TOO_HIGH]             = (fault_eval_t){ .id = "High Cell Voltage",        .timer = ovr_volt_timer,       .data_1 = analyzer->max_ocv.val,               .optype_1 = GT, .lim_1 = MAX_VOLT,                          .timeout = OVER_VOLT_TIME,      .optype_2 = NOP,                                                                  .is_critical = true  };
		fault_eval_table[CELL_CHARGE_VOLTAGE_TOO_HIGH]      = (fault_eval_t){ .id = "High Charge Voltage",      .timer = ovr_chgvolt_timer,    .data_1 = analyzer->max_ocv.val,               .optype_1 = GT, .lim_1 = MAX_CHARGE_VOLT,                   .timeout = OVER_VOLT_TIME,      .optype_2 = EQ,  .data_2 = (state_machine->bms_state == CHARGING), .lim_2 = true, .is_critical = true  };
		fault_eval_table[PACK_TOO_HOT]                      = (fault_eval_t){ .id = "High Cell Temp",           .timer = high_temp_timer,      .data_1 = sanitizer->max_sanitized_temp.val,   .optype_1 = GT, .lim_1 = MAX_CELL_TEMP,                     .timeout = HIGH_TEMP_TIME,      .optype_2 = NOP,                                                                  .is_critical = true  };
		fault_eval_table[DIE_TEMP_MAXIMUM_FAULT]            = (fault_eval_t){ .id = "Die Overtemp",             .timer = die_overtemp_timer,   .data_1 = analyzer->max_chiptemp.val,          .optype_1 = GT, .lim_1 = MAX_CHIP_TEMP,                     .timeout = MAX_CHIPTEMP_TIME,   .optype_2 = NOP,                                                                  .is_critical = true  };
		fault_eval_table[SEGMENT_COMMS_FAULT]               = (fault_eval_t){ .id = "Segment Comms Fault",      .timer = segment_comms_timer,  .data_1 = segment_comms_fault,                 .optype_1 = EQ, .lim_1 = true,                              .timeout = COMMS_FAULT_TIME,    .optype_2 = NOP,                                                                  .is_critical = false };
		fault_eval_table[HV_PLATE_COMMS_FAULT]              = (fault_eval_t){ .id = "HV Plate Comms Fault",     .timer = hv_plate_comms_timer, .data_1 = hv_plate_comms_fault,                .optype_1 = EQ, .lim_1 = true,                              .timeout = COMMS_FAULT_TIME,    .optype_2 = NOP,                                                                  .is_critical = true  };
		fault_eval_table[CELL_OPEN_WIRE_FAULT]              = (fault_eval_t){ .id = "Cell Open Wire Fault",     .timer = open_wire_timer,      .data_1 = ow_fault,                            .optype_1 = EQ, .lim_1 = true,                              .timeout = OW_FAULT_TIME,       .optype_2 = NOP,                                                                  .is_critical = true  };
		// clang-format on

		initialized = true;
	}
}

// STATE MACHINE THREAD
void vStateMachine(ULONG thread_input)
{
	PRINTLN_INFO("Starting State Machine thread...");

	state_machine_args_t *state_machine_args =
		(state_machine_args_t *)thread_input;

	state_machine_t *state_machine = state_machine_args->state_machine;
	analyzer_t *analyzer = state_machine_args->analyzer;

	init_boot(state_machine_args);

	nertimer_t telem_timer;
	// sends unimportant telemetry messages every 500ms
	start_timer(&telem_timer, 500);

	for (;;) {
		sm_handle_state(state_machine_args);

		// send unimportant messages less frequently
		if (is_timer_expired(&telem_timer)) {
			send_bms_status(state_machine->bms_state,
					analyzer->avg_temp);

			send_bms_critically_faulted(
				are_critical_faults_active());

			send_fault_status(
				get_fault(DISCHARGE_LIMIT_ENFORCEMENT_FAULT),
				get_fault(CHARGE_LIMIT_ENFORCEMENT_FAULT),
				get_fault(CELL_VOLTAGE_TOO_LOW),
				get_fault(CELL_VOLTAGE_TOO_HIGH),
				get_fault(CELL_CHARGE_VOLTAGE_TOO_HIGH),
				get_fault(PACK_TOO_HOT),
				get_fault(DIE_TEMP_MAXIMUM_FAULT),
				get_fault(SEGMENT_COMMS_FAULT),
				get_fault(HV_PLATE_COMMS_FAULT),
				get_fault(CELL_OPEN_WIRE_FAULT));

			start_timer(&telem_timer, 500);
		}

		thread_sleep_ms(20);
	}
}
