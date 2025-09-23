
#include "u_threads.h"
#include "u_debug.h"
#include "state_machine.h"
#include "datastructs.h"
#include <stdbool.h>


typedef void (*InitFunc_t)(bms_t *bms);   
typedef void (*HandlerFunc_t)(bms_t *bms);   

const InitFunc_t init_LUT[NUM_STATES] = { &init_boot, &init_ready,
					      &init_charging, &init_faulted };

const HandlerFunc_t handler_LUT[NUM_STATES] = { &handle_boot, &handle_ready,
						    &handle_charging,
						    &handle_faulted };


void init_boot(bms_t *bmsdata)
{
	// useless, really since handle_boot always requests a transition anyways
	return;
}

void handle_boot(bms_t *bmsdata)
{
	// the charger could be connected on state machine boot, so lets not re-enter ready!
	if (bmsdata->is_charger_connected) {
		//request_transition(bmsdata, CHARGING_STATE);
	} else {
		//request_transition(bmsdata, READY_STATE);
	}
	return;
}

void init_ready(bms_t *bmsdata)
{
	return;
}

void handle_ready(bms_t *bmsdata)
{
	// send our DCL and CCL to motors
	//send_mc_charge_message(bmsdata->cont_CCL);
	//send_mc_discharge_message(bmsdata->cont_DCL);
	compute_set_fault(false);
}

void init_charging(bms_t *bmsdata)
{
	//cancel_timer(&charger_settle_countup);
	return;
}

void handle_charging(bms_t *bmsdata)
{
    
}

void init_faulted(bms_t *bmsdata)
{
    
}

void handle_faulted(bms_t *bmsdata)
{

}

void request_transition(bms_t *bmsdata, state_t next_state)
{
	if (bmsdata->current_state == next_state)
		return;
	//if (!valid_transition_from_to[current_state][next_state])
	//	return;

	init_LUT[next_state](bmsdata);
	bmsdata->current_state = next_state;
}


