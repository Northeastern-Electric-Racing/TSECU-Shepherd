#include "state_machine.h"
#include "can_messages.h"
#include "compute.h"
#include "segment.h"
#include "charging.h"
#include "c_utils.h"

const bool valid_transition_from_to[NUM_STATES][NUM_STATES] = {
	/*   BOOT, READY, CHARGING, BALANCING, FAULTED */
	{ true, true, true, false, true }, /* BOOT */
	{ false, true, true, false, true }, /* READY */
	{ false, true, true, true, true }, /* CHARGING */
	{ false, true, true, true, true }, /* BALANCING */
	{ true, false, false, false, true } /* FAULTED */
};

/* private function prototypes */

// init functions
void init_boot(state_machine_args_t *state_machine_args);
void init_ready(state_machine_args_t *state_machine_args);
void init_charging(state_machine_args_t *state_machine_args);
void init_balancing(state_machine_args_t *state_machine_args);
void init_faulted(state_machine_args_t *state_machine_args);

// handle functions
void handle_boot(state_machine_args_t *state_machine_args);
void handle_ready(state_machine_args_t *state_machine_args);
void handle_charging(state_machine_args_t *state_machine_args);
void handle_balancing(state_machine_args_t *state_machine_args);
void handle_faulted(state_machine_args_t *state_machine_args);

void request_transition(state_machine_args_t *state_machine_args, state_t next_state);

typedef void (*HandlerFunction_t)(state_machine_args_t *state_machine_args);
typedef void (*InitFunction_t)(state_machine_args_t *state_machine_args);

const InitFunction_t init_LUT[NUM_STATES] = { &init_boot, &init_ready,
					      &init_charging, &init_faulted };

const HandlerFunction_t handler_LUT[NUM_STATES] = { &handle_boot, &handle_ready,
						    &handle_charging,
						    &handle_faulted };

void init_boot(state_machine_args_t *state_machine_args)
{
	// useless, really since handle_boot always requests a transition anyways
	return;
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
	cancel_timer(&state_machine_args->state_machine->charger_settle_countup_timer);
	return;
}

void init_balancing(state_machine_args_t *state_machine_args)
{
	// disable discharge and charge from the MC
	send_mc_discharge_message(0);
	send_mc_charge_message(0);
	return;
}

void handle_balancing(state_machine_args_t *state_machine_args) {
	if (sm_balancing_check(state_machine_args)) {
		handle_balance_cells(state_machine_args->analyzer);
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
		if (is_timer_expired(&state_machine_args->state_machine->charger_message_timer) ||
		    !is_timer_active(&state_machine_args->state_machine->charger_message_timer)) {
			send_charging_message((MAX_CHARGE_VOLT *
					       (NUM_CELLS_PER_CHIP * 2) *
					       NUM_SEGMENTS),
					      CHARGING_CURRENT, true);
			start_timer(&state_machine_args->state_machine->charger_message_timer, 1000);
		}
	} else {
		send_charging_message(0, 0, false);
	}

	/* Check if we should balance */
	if (sm_balancing_check(state_machine_args))
		request_transition(state_machine_args, BALANCING);

	// disable discharge and charge from the MC
	send_mc_discharge_message(0);
	send_mc_charge_message(0);
}

void charger_message_recieved(state_machine_args_t *state_machine_args)
{
	// this is irreversible, a LV power cycle occurs before re-connection to car
	request_transition(state_machine_args, CHARGING);
}

void init_faulted(state_machine_args_t *bmsdata)
{
	send_mc_charge_message(0);
	send_mc_discharge_message(0);	
	send_charging_message(0, 0, false);
}

void handle_faulted(state_machine_args_t *state_machine_args)
{
	// leave faulted if all is well
	if (state_machine_args->state_machine->fault_code_crit == FAULTS_CLEAR) {
		compute_set_fault(false);
		request_transition(state_machine_args, BOOT);
		return;
	}
}

void sm_handle_state(state_machine_args_t *state_machine_args)
{
	
	// always check for faults no matter the current state
	sm_fault_return(state_machine_args);

	if (state_machine_args->state_machine->fault_code_crit != FAULTS_CLEAR) {
		request_transition(state_machine_args, FAULTED);
	}

	PRINTLN_INFO("FAULT STATUS: %d\n", get_current_state(state_machine_args->state_machine));
	handler_LUT[get_current_state(state_machine_args->state_machine) ](state_machine_args);
}

state_t get_current_state(state_machine_t *state_machine) {
	state_t state;
	mutex_get(&state_machine->state_mutex);
	state = state_machine->bms_state;
	mutex_put(&state_machine->state_mutex);
	return state;
}


void request_transition(state_machine_t *state_machine, state_t next_state)
{
	if (get_current_state(state_machine_args->state_machine) == next_state)
		return;
	if (!valid_transition_from_to[get_current_state(state_machine_args->state_machine)][next_state])
		return;

	mutex_get(&state_machine_args->state_machine->state_mutex);	

	state_machine_args->state_machine->bms_state = next_state;
	init_LUT[next_state](state_machine_args);

	mutex_put(&state_machine_args->state_machine->state_mutex);
}

void sm_fault_return(state_machine_args_t *state_machine_args)
{
	/* FAULT CHECK (Check for fuckies) */

	nertimer_t ovr_curr_timer = { 0 };
	nertimer_t ovr_chgcurr_timer = { 0 };
	nertimer_t undr_volt_timer = { 0 };
	nertimer_t ovr_chgvolt_timer = { 0 };
	nertimer_t ovr_volt_timer = { 0 };
	nertimer_t low_cell_timer = { 0 };
	nertimer_t high_temp_timer = { 0 };
	nertimer_t die_overtemp_timer = { 0 };

	// initialize fault timers
	cancel_timer(&ovr_curr_timer);
	cancel_timer(&ovr_chgcurr_timer);
	cancel_timer(&undr_volt_timer);
	cancel_timer(&ovr_chgvolt_timer);
	cancel_timer(&ovr_volt_timer);
	cancel_timer(&low_cell_timer);
	cancel_timer(&high_temp_timer);
	cancel_timer(&die_overtemp_timer);

	fault_eval_t fault_table[NUM_FAULTS];
	analyzer_t *fault_data = state_machine_args->analyzer; 

	// TODO: Change to mutexed getter callback for retrieving fault data

	// clang-format off
											// ___________FAULT ID____________   __________TIMER___________   _____________DATA________________    __OPERATOR__   ____________________________________THRESHOLD____________________________  _______TIMER LENGTH_________  _____________FAULT CODE_________________    	___OPERATOR 2__ ________________________DATA 2______________   __THRESHOLD 2_____ ______CRITICAL________
	fault_table[0]  = (fault_eval_t) {.id = "Discharge Current Limit", .timer =       ovr_curr_timer, .data_1 =     state_machine_args->hv_plate->pack_current,  .optype_1 = GT, .lim_1 = fault_data->cont_DCL ,                                                .timeout =      OVER_CURR_TIME, .code = DISCHARGE_LIMIT_ENFORCEMENT_FAULT,  .optype_2 = NOP/* ------------------------------UNUSED-------------------------*/, .is_critical = true  };
	fault_table[1]  = (fault_eval_t) {.id = "Charge Current Limit",    .timer =    ovr_chgcurr_timer, .data_1 =     state_machine_args->hv_plate->pack_current,  .optype_1 = GT, .lim_1 =                                        fault_data->cont_CCL,          .timeout =  OVER_CHG_CURR_TIME, .code =    CHARGE_LIMIT_ENFORCEMENT_FAULT,  .optype_2 = LT,  .data_2 =         state_machine_args->hv_plate->pack_current,  .lim_2 =          0, .is_critical = true  };
	fault_table[2]  = (fault_eval_t) {.id = "Low Cell Voltage",        .timer =      undr_volt_timer, .data_1 =  fault_data->min_ocv.val,      .optype_1 = LT, .lim_1 =                                                     MIN_VOLT,         .timeout =     UNDER_VOLT_TIME, .code =              CELL_VOLTAGE_TOO_LOW,  .optype_2 = NOP/* ------------------------------UNUSED-------------------------*/, .is_critical = true  };
	fault_table[3]  = (fault_eval_t) {.id = "High Charge Voltage",     .timer =    ovr_chgvolt_timer, .data_1 =  fault_data->max_ocv.val,      .optype_1 = GT, .lim_1 =                                              MAX_CHARGE_VOLT,         .timeout =  OVER_VOLT_TIME,     .code =             CELL_VOLTAGE_TOO_HIGH,  .optype_2 = EQ, .data_2 = state_machine_args->state_machine->bms_state == CHARGING,  .lim_2 =      true,   .is_critical = true  };
	fault_table[4]  = (fault_eval_t) {.id = "High Cell Voltage",       .timer =       ovr_volt_timer, .data_1 =  fault_data->max_ocv.val,      .optype_1 = GT, .lim_1 =                                                     MAX_VOLT,         .timeout =      OVER_VOLT_TIME, .code =             CELL_VOLTAGE_TOO_HIGH,  .optype_2 = NOP/* ------------------------------UNUSED-------------------------*/, .is_critical = true  };
	fault_table[5]  = (fault_eval_t) {.id = "High Temp",               .timer =      high_temp_timer, .data_1 =     fault_data->max_temp.val,  .optype_1 = GT, .lim_1 =                                                        MAX_CELL_TEMP, .timeout =      HIGH_TEMP_TIME, .code =                      PACK_TOO_HOT,  .optype_2 = NOP/* ------------------------------UNUSED-------------------------*/, .is_critical = true  };
	fault_table[6]  = (fault_eval_t) {.id = "Extremely Low Voltage",   .timer =       low_cell_timer, .data_1 =  fault_data->min_ocv.val,      .optype_1 = LT, .lim_1 =                                                                  0.9, .timeout =       LOW_CELL_TIME, .code =                  LOW_CELL_VOLTAGE,  .optype_2 = NOP/* ------------------------------UNUSED-------------------------*/, .is_critical = true  };
	fault_table[7]  = (fault_eval_t) {.id = "Die Overtemp",            .timer =   die_overtemp_timer, .data_1 = fault_data->max_chiptemp.val,  .optype_1 = GT, .lim_1 = 													   MAX_CHIP_TEMP, .timeout =   MAX_CHIPTEMP_TIME, .code =            DIE_TEMP_MAXIMUM_FAULT,  .optype_2 = NOP/* ------------------------------UNUSED-------------------------*/, .is_critical = true  };
	// clang-format on


	fault_stat_t status;
	for (int i = 0; i < NUM_FAULTS; i++) {
		uint32_t item_code = fault_table[i].code;
		status = sm_fault_eval(&fault_table[i]);
		if (status == FAULT_STAT_FAULTED) {
			if (fault_table[i].is_critical) {
				state_machine_args->state_machine->fault_code_crit |= item_code;
			} else {
				state_machine_args->state_machine->fault_code_noncrit |= item_code;
			}
		} else if (status == FAULT_STAT_CLEAR) {
			// Clear bit for non-critical faults
			if (fault_table[i].is_critical) {
				state_machine_args->state_machine->fault_code_crit &= ~item_code;
			} else {
				state_machine_args->state_machine->fault_code_noncrit &= ~item_code;
			}
		}
	}
}

fault_stat_t sm_fault_eval(fault_eval_t *item)
{
	bool condition1;
	bool condition2;

	// clang-format off
    switch (item->optype_1)
    {
        case GT: condition1 = item->data_1 > item->lim_1; break;
        case LT: condition1 = item->data_1 < item->lim_1; break;
        case GE: condition1 = item->data_1 >= item->lim_1; break;
        case LE: condition1 = item->data_1 <= item->lim_1; break;
        case EQ: condition1 = item->data_1 == item->lim_1; break;
		case NEQ: condition1 = item->data_1 != item->lim_1; break;
        case NOP: condition1 = false;
		default: condition1 = false;
    }

    switch (item->optype_2)
    {
        case GT: condition2 = item->data_2 > item->lim_2; break;
        case LT: condition2 = item->data_2 < item->lim_2; break;
        case GE: condition2 = item->data_2 >= item->lim_2; break;
        case LE: condition2 = item->data_2 <= item->lim_2; break;
        case EQ: condition2 = item->data_2 == item->lim_2; break;
		case NEQ: condition2 = item->data_2 != item->lim_2; break;
        case NOP: condition2 = false;
		default: condition2 = false;
    }
	// clang-format on

	bool fault_present = ((condition1 && condition2) ||
			      (condition1 && (item->optype_2 == NOP)));

	if ((!(is_timer_active(&item->timer))) && !fault_present) {
		return FAULT_STAT_CLEAR;
	}

	if (is_timer_active(&item->timer)) {
		if (!fault_present) {
			PRINTLN_INFO("\tFault cleared: %s\n", item->id);
			cancel_timer(&item->timer);
			send_fault_timer_message(0, item->code, item->data_1);
			return FAULT_STAT_CLEAR;
		}

		if (is_timer_expired(&item->timer) && fault_present) {
			PRINTLN_INFO("\tFaulted: %s\n", item->id);
			send_fault_timer_message(2, item->code, item->data_1);
			return FAULT_STAT_FAULTED;
		}

		return FAULT_STAT_CLEAR;
	}

	else if (!is_timer_active(&item->timer) && fault_present) {
		PRINTLN_INFO("\tStarting Fault Timer: %s\n", item->id);
		start_timer(&item->timer, item->timeout);
		send_fault_timer_message(1, item->code, item->data_1);

		return 0;
	}
	
	PRINTLN_ERROR("Should not have reached here.");
	return 0;
}

/* charger settle countup =  1 minute pause to let readings settle and get good
 * OCV */
/* charger settle countdown = 5 minute interval between 1 minute settle pauses */
bool sm_charging_check(state_machine_args_t *state_machine_args)
{
	// dont charge during the countup
	if (!is_timer_expired(&state_machine_args) &&
	    is_timer_active(&state_machine_args->state_machine->charge_settle_countdown_timer)) {
		//printf("Charger settle countup active\r\n");
		return false;
	}

	// if we are counting down (the normal charging time)
	if (is_timer_active(&state_machine_args->state_machine->charge_settle_countdown_timer)) {
		// if we need to stop charging, start the pause timer and stop charging immediately
		if (is_timer_expired(&state_machine_args->state_machine->charge_settle_countdown_timer)) {
			start_timer(&state_machine_args->state_machine->charge_settle_countdown_timer,
				    CHARGE_SETL_TIMEOUT);
			return false;
		} else
			return true;
	} else {
		// start the countdown timer if it is inactive, meaning we went from pause --> unpause
		start_timer(&state_machine_args->state_machine->charge_settle_countdown_timer, CHARGE_SETL_TIMEUP);
		return true;
	}
}

// check if balancing is allowed
bool sm_balancing_check(state_machine_args_t *state_machine_args)
{

	// TODO: replace with mutexed getter
	if (state_machine_args->analyzer->max_voltage.val <= BAL_MIN_V)
		return false;
	if (state_machine_args->analyzer->delt_voltage <= MAX_DELTA_V)
		return false;

	// Do not balance during the countup.
	if (is_timer_active(&state_machine_args->state_machine->charger_settle_countup_timer &&
	    !is_timer_expired(&state_machine_args->state_machine->charger_settle_countup_timer))) {
		return false;
	}

	// Do not balance if the shutdown circuit is open.
	return !read_shutdown();
}
