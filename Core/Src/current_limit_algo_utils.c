#include "current_limit_algo_utils.h"
#include "state_machine.h"
#include <assert.h>
#include <math.h>

/* Default epsilon for generic float comparisons */
#define FLOAT_EPSILON (0.0001f)

float linear_interpolate(float x, float x1, float x2, float y1, float y2)
{
	assert(fabs(x2 - x1) > FLOAT_EPSILON);

	return y1 + ((x - x1) * (y2 - y1) / (x2 - x1));
}

bool disable_pulse(state_machine_t *const state_machine)
{
	bool dis_pulse = false;

	/** @todo If HV_PLATE_COMMS_FAULT is classified as non-critical, add it to this check.
 	 *  If classified as critical, it is already handled.
 	 */
	mutex_get(&state_machine->state_mutex);
	if ((get_current_state(state_machine) == CHARGING) ||
	    (state_machine->fault_code_crit != FAULTS_CLEAR) ||
	    ((state_machine->fault_code_noncrit & SEGMENT_COMMS_FAULT) != 0U)) {
		dis_pulse = true;
	}
	mutex_put(&state_machine->state_mutex);

	return dis_pulse;
}