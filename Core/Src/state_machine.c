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
#include <stdatomic.h>
#include "app_threadx.h"
#include "shep_mutexes.h"
#include "u_tx_threads.h"

static fault_eval_t fault_eval_table[NUM_FAULTS];

static _Atomic uint32_t severity_mask = 0;
static _Atomic uint32_t fault_flags = 0;

const bool valid_transition_from_to[NUM_STATES][NUM_STATES] = {
	/*   BOOT, READY, CHARGING, BALANCING, FAULTED */
	{ true, true, true, false, true }, /* BOOT */
	{ false, true, true, false, true }, /* READY */
	{ false, true, true, true, true }, /* CHARGING */
	{ false, true, true, true, true }, /* BALANCING */
	{ true, false, false, false, true } /* FAULTED */
};

/* private function prototypes */
void update_eval_table(state_machine_args_t *state_machine_args);

void request_transition(state_machine_args_t *state_machine_args,
			state_t next_state);

typedef void (*HandlerFunction_t)(state_machine_args_t *state_machine_args);
typedef void (*InitFunction_t)(state_machine_args_t *state_machine_args);

const InitFunction_t init_LUT[NUM_STATES] = { &init_boot, &init_ready,
					      &init_charging, &init_balancing,
					      &init_faulted };

const HandlerFunction_t handler_LUT[NUM_STATES] = { &handle_boot, &handle_ready,
						    &handle_charging,
						    &handle_balancing,
						    &handle_faulted };

void init_boot(state_machine_args_t *state_machine_args)
{
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
	return;
}

void init_ready(state_machine_args_t *state_machine_args)
{
	compute_set_fault(false); // TODO: Make BMS Utils
	return;
}

void handle_ready(state_machine_args_t *state_machine_args)
{
	return;
}

void init_charging(state_machine_args_t *state_machine_args)
{
	state_machine_args->state_machine->charging_stage = LONG_CHARGE_UP;
	start_timer(&state_machine_args->state_machine->charging_stage_timer,
		    15 * 60 * 1000); // 15 minutes
}

void init_balancing(state_machine_args_t *state_machine_args)
{
	// disable discharge and charge from the MC
	send_max_dc_current_command(0);
	send_max_dc_brake_current_command(0);
	return;
}

void handle_balancing(state_machine_args_t *state_machine_args)
{
	if (sm_balancing_check(state_machine_args)) {
		handle_balance_cells(state_machine_args->analyzer,
				     state_machine_args->acc_data);
	} else {
		request_transition(state_machine_args, CHARGING);
	}
	return;
}

void handle_charging(state_machine_args_t *state_machine_args)
{
	/* Check if we should charge */
	if (sm_charging_check(state_machine_args)) {
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
		send_bms_charge_message_send(0, 0, 0xFF);
	}

	// disable discharge and charge from the MC
	send_max_dc_current_command(0);
	send_max_dc_brake_current_command(0);

	/* Check if we should balance */
	if (sm_balancing_check(state_machine_args))
		request_transition(state_machine_args, BALANCING);
}

void charger_message_recieved(state_machine_args_t *state_machine_args)
{
	// this is irreversible, a LV power cycle occurs before re-connection to car
	request_transition(state_machine_args, CHARGING);
}

void init_faulted(state_machine_args_t *bmsdata)
{
	send_max_dc_current_command(0);
	send_max_dc_brake_current_command(0);
	send_bms_charge_message_send(0, 0, 0xFF);
}

void handle_faulted(state_machine_args_t *state_machine_args)
{
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

	mutex_get(&state_mutex);

	if (state_machine->bms_state == next_state)
		return;

	if (!valid_transition_from_to[state_machine->bms_state][next_state])
		return;

	state_machine_args->state_machine->bms_state = next_state;

	mutex_put(&state_mutex);

	init_LUT[next_state](state_machine_args);
}

void sm_fault_return(state_machine_args_t *state_machine_args)
{
	/* FAULT CHECK (Check for fuckies) */
	update_eval_table(state_machine_args);

	bool faulted = false;
	for (int i = 0; i < NUM_FAULTS; i++) {
		faulted = sm_fault_eval(&fault_eval_table[i], i);
		if (faulted) {
			fault_flags |= (1 << i);
		} else {
			fault_flags &= ~(1 << i);
		}
	}
}

bool sm_fault_eval(fault_eval_t *item, fault_code_t fault_code)
{
	bool condition1;
	bool condition2;

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
		default:
			condition1 = false;
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
		default:
			condition2 = false;
	}

	bool fault_present = (condition1 && condition2) ||
			     (condition1 && item->optype_2 == NOP);

	if (!is_timer_active(&item->timer) && !fault_present) {
		return false;
	}

	if (is_timer_active(&item->timer)) {
		if (!fault_present) {
			PRINTLN_INFO("\tFault cleared: %s\n", item->id);
			cancel_timer(&item->timer);
			// STOPPING TIMER MESSSAGE
			send_bms_fault_timers(FAULT_TIMER_STOPPED, fault_code,
					      item->data_1);
			return false;
		}

		if (is_timer_expired(&item->timer) && fault_present) {
			PRINTLN_INFO("\tFaulted: %s\n", item->id);

			// FAULT TIMER EXPIRED MESSAGE
			send_bms_fault_timers(FAULT_TIMER_EXPIRED, fault_code,
					      item->data_1);
			return true;
		}

		return false;

	} else if (!is_timer_active(&item->timer) && fault_present) {
		PRINTLN_INFO("\tStarting Fault Timer: %s\n", item->id);
		start_timer(&item->timer, item->timeout);
		// STARTING FAULTED TIMER MESSAGE
		send_bms_fault_timers(FAULT_TIMER_STARTED, fault_code,
				      item->data_1);

		return false;
	}

	PRINTLN_ERROR("Should not have reached here.");
	return true;
}

/* This charging algorithm has 3 stages
* 1. Charge up until the high cell non OCV max voltage is > 4.19, pause for 1 minute every 15 minutes, repeat
* 2. Charge up until the high cell     OCV max voltage is > 4.19, pause for 1 minute every 20 seconds, repeat
* 3. Stop charging :)
*/
bool sm_charging_check(state_machine_args_t *state_machine_args)
{
	state_machine_t *state_machine = state_machine_args->state_machine;
	analyzer_t *analyzer = state_machine_args->analyzer;

	nertimer_t *state_timer =
		&state_machine_args->state_machine->charging_stage_timer;

	charge_stage_t next_stage = state_machine->charging_stage;

	// TODO: MUTEX GET
	if (analyzer->max_ocv.val > MAX_CHARGE_VOLT_FLT ||
	    analyzer->max_voltage.val > MAX_CHARGE_VOLT_FLT) {
		state_machine->charging_stage = FAULT;
		return false;
	}

	switch (state_machine->charging_stage) {
		case LONG_CHARGE_UP:
			if (analyzer->max_voltage.val > MAX_CHARGE_VOLT ||
			    is_timer_expired(state_timer)) {
				next_stage = LONG_SETTLE;
			}
			break;
		case LONG_SETTLE:
			if (is_timer_expired(state_timer)) {
				 next_stage = SHORT_CHARGE_UP;
			}
			break;
		case SHORT_CHARGE_UP:
			if (analyzer->max_ocv.val > MAX_CHARGE_VOLT ||
			    is_timer_expired(state_timer)) {
				next_stage = SHORT_SETTLE;
			}
			break;
		case SHORT_SETTLE:
			if (is_timer_expired(state_timer)) {
				if (analyzer->max_ocv.val < MAX_CHARGE_VOLT) {
					next_stage =
						SHORT_CHARGE_UP; // continue charging
				} else {
					next_stage = DONE;
				}
			}
			break;
		case DONE:
			return false; // done charging
		case FAULT:
			return false; // stuck faulting until restart
	}
	// TODO: MUTEX RELEASE

	// Transitioning stages, start the corresponding timer lengths
	if (next_stage != state_machine->charging_stage) {
		switch (next_stage) {
			case LONG_CHARGE_UP:
				start_timer(state_timer,
					    15 * 60 * 1000); // 15 minutes
				break;
			case SHORT_CHARGE_UP:
				start_timer(state_timer,
					    20 * 1000); // 20 seconds
				break;

			case LONG_SETTLE:
			case SHORT_SETTLE:
				start_timer(state_timer, 60 * 1000); // 1 minute
				break;

			// cases return earlier or arent possible
			case DONE:
			case FAULT:
				break;
		}

		state_machine->charging_stage = next_stage;
	}

	/* if not charging stage, dont charge
	 * (LONG_SETTLE, SHORT_SETTLE, DONE, FAULT) */
	return state_machine->charging_stage == LONG_CHARGE_UP ||
	       state_machine->charging_stage == SHORT_CHARGE_UP;
}

// check if balancing is allowed
bool sm_balancing_check(state_machine_args_t *state_machine_args)
{
	//state_machine_t *state_machine = state_machine_args->state_machine;
	analyzer_t *analyzer = state_machine_args->analyzer;

	// TODO: replace with mutexed getter
	if (analyzer->max_voltage.val <= BAL_MIN_V)
		return false;
	if (analyzer->delt_voltage <= MAX_DELTA_V)
		return false;

	// Do not balance during settling.
	// if (state_machine->charging_stage != LONG_SETTLE &&
	//     state_machine->charging_stage != SHORT_SETTLE) {
	// 	return false;
	// }

	// Do not balance if the shutdown circuit is open.

	bool shutdown_active;
	mutex_get(&shutdown_mutex);
	shutdown_active = state_machine_args->peripherals->shutdown_active;
	mutex_put(&shutdown_mutex);

	return !shutdown_active;
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

	if (initialized) {
		fault_eval_table[DISCHARGE_LIMIT_ENFORCEMENT_FAULT].data_1 =
			hv_plate->pack_current;
		fault_eval_table[DISCHARGE_LIMIT_ENFORCEMENT_FAULT].lim_1 =
			bms_algos->cont_DCL;
		fault_eval_table[CHARGE_LIMIT_ENFORCEMENT_FAULT].data_1 =
			hv_plate->pack_current;
		fault_eval_table[CHARGE_LIMIT_ENFORCEMENT_FAULT].lim_1 =
			bms_algos->cont_CCL;
		fault_eval_table[CELL_VOLTAGE_TOO_LOW].data_1 =
			analyzer->min_ocv.val;
		fault_eval_table[CELL_VOLTAGE_TOO_HIGH].data_1 =
			analyzer->max_ocv.val;
		fault_eval_table[CELL_CHARGE_VOLTAGE_TOO_HIGH].data_1 =
			analyzer->max_ocv.val;
		fault_eval_table[CELL_CHARGE_VOLTAGE_TOO_HIGH].data_2 =
			(state_machine->bms_state == CHARGING);
		fault_eval_table[PACK_TOO_HOT].data_1 =
			sanitizer->max_sanitized_temp.val;
		fault_eval_table[DIE_TEMP_MAXIMUM_FAULT].data_1 =
			analyzer->max_chiptemp.val;
		fault_eval_table[SEGMENT_COMMS_FAULT].data_1 =
			state_machine->segment_comms_fault_flag;
		fault_eval_table[HV_PLATE_COMMS_FAULT].data_1 =
			state_machine->hv_plate_comms_fault_flag;
	} else {
		fault_eval_table[DISCHARGE_LIMIT_ENFORCEMENT_FAULT] =
			(fault_eval_t){ .id = "Discharge Current Limit",
					.timer = ovr_curr_timer,
					.data_1 = hv_plate->pack_current,
					.optype_1 = GT,
					.lim_1 = bms_algos->cont_DCL,
					.timeout = OVER_CURR_TIME,
					.optype_2 = NOP, // UNUSED
					.is_critical = true };

		fault_eval_table[CHARGE_LIMIT_ENFORCEMENT_FAULT] =
			(fault_eval_t){ .id = "Charge Current Limit",
					.timer = ovr_chgcurr_timer,
					.data_1 = hv_plate->pack_current,
					.optype_1 = GT,
					.lim_1 = bms_algos->cont_CCL,
					.timeout = OVER_CHG_CURR_TIME,
					.optype_2 = NOP, // UNUSED
					.is_critical = true };

		fault_eval_table[CELL_VOLTAGE_TOO_LOW] =
			(fault_eval_t){ .id = "Low Cell Voltage",
					.timer = undr_volt_timer,
					.data_1 = analyzer->min_ocv.val,
					.optype_1 = LT,
					.lim_1 = MIN_VOLT,
					.timeout = UNDER_VOLT_TIME,
					.optype_2 = NOP, // UNUSED
					.is_critical = true };

		fault_eval_table[CELL_VOLTAGE_TOO_HIGH] =
			(fault_eval_t){ .id = "High Cell Voltage",
					.timer = ovr_volt_timer,
					.data_1 = analyzer->max_ocv.val,
					.optype_1 = GT,
					.lim_1 = MAX_VOLT,
					.timeout = OVER_VOLT_TIME,
					.optype_2 = NOP, // UNUSED
					.is_critical = true };

		fault_eval_table[CELL_CHARGE_VOLTAGE_TOO_HIGH] = (fault_eval_t){
			.id = "High Charge Voltage",
			.timer = ovr_chgvolt_timer,
			.data_1 = analyzer->max_ocv.val,
			.optype_1 = GT,
			.lim_1 = MAX_CHARGE_VOLT,
			.timeout = OVER_VOLT_TIME,
			.optype_2 = EQ,
			.data_2 = (state_machine->bms_state == CHARGING),
			.lim_2 = true,
			.is_critical = true
		};

		fault_eval_table[PACK_TOO_HOT] = (fault_eval_t){
			.id = "High Cell Temp",
			.timer = high_temp_timer,
			.data_1 = sanitizer->max_sanitized_temp.val,
			.optype_1 = GT,
			.lim_1 = MAX_CELL_TEMP,
			.timeout = HIGH_TEMP_TIME,
			.optype_2 = NOP, // UNUSED
			.is_critical = true
		};

		fault_eval_table[DIE_TEMP_MAXIMUM_FAULT] =
			(fault_eval_t){ .id = "Die Overtemp",
					.timer = die_overtemp_timer,
					.data_1 = analyzer->max_chiptemp.val,
					.optype_1 = GT,
					.lim_1 = MAX_CHIP_TEMP,
					.timeout = MAX_CHIPTEMP_TIME,
					.optype_2 = NOP, // UNUSED
					.is_critical = true };

		fault_eval_table[SEGMENT_COMMS_FAULT] = (fault_eval_t){
			.id = "Segment Comms Fault",
			.timer = segment_comms_timer,
			.data_1 = state_machine->segment_comms_fault_flag,
			.optype_1 = GE,
			.lim_1 = true,
			.timeout = 0,
			.optype_2 = NOP, // UNUSED
			.is_critical = false
		};

		fault_eval_table[HV_PLATE_COMMS_FAULT] = (fault_eval_t){
			.id = "HV Plate Comms Fault",
			.timer = hv_plate_comms_timer,
			.data_1 = state_machine->hv_plate_comms_fault_flag,
			.optype_1 = GE,
			.lim_1 = true,
			.timeout = 0,
			.optype_2 = NOP, // UNUSED
			.is_critical = true
		};
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

	state_machine->bms_state = BOOT;

	nertimer_t telem_timer;
	// sends unimportant telemetry messages every 500ms
	start_timer(&telem_timer, 500);

	for (;;) {
		sm_handle_state(state_machine_args);

		// send unimportant messages less frequently
		if (is_timer_expired(&telem_timer)) {
			send_bms_status(state_machine->bms_state,
					analyzer->avg_temp);

			send_fault_status(
				get_fault(DISCHARGE_LIMIT_ENFORCEMENT_FAULT),
				get_fault(CHARGE_LIMIT_ENFORCEMENT_FAULT),
				get_fault(CELL_VOLTAGE_TOO_LOW),
				get_fault(CELL_VOLTAGE_TOO_HIGH),
				get_fault(CELL_CHARGE_VOLTAGE_TOO_HIGH),
				get_fault(PACK_TOO_HOT),
				get_fault(DIE_TEMP_MAXIMUM_FAULT),
				get_fault(SEGMENT_COMMS_FAULT),
				get_fault(HV_PLATE_COMMS_FAULT));

			start_timer(&telem_timer, 500);
		}

		thread_sleep_ms(20);
	}
}
