#include "bms_algos.h"
#include "state_machine.h"
#include "ccl.h"
#include "dcl.h"

bool disable_pulse(state_machine_t *const state_machine)
{
	bool dis_pulse = false;

	/** @todo If HV_PLATE_COMMS_FAULT is classified as non-critical, add it to this check.
 	 *  If classified as critical, it is already handled.
 	 */
	if ((state_machine->bms_state == CHARGING) ||
	    (are_critical_faults_active()) ||
	    (get_fault(SEGMENT_COMMS_FAULT))) {
		dis_pulse = true;
	}

	return dis_pulse;
}

void vBMSAlgorithms(ULONG thread_input)
{
	PRINTLN_INFO("Starting BMS Algorithms thread...");

	bms_algos_args_t *bms_algos_args = (bms_algos_args_t *)thread_input;

	bms_algos_t *bms_algos = bms_algos_args->bms_algos;
	sanitizer_t *sanitizer = bms_algos_args->sanitizer;
	analyzer_t *analyzer = bms_algos_args->analyzer;

	for (;;) {
		current_limit_algo_inputs_t algo_inputs = {
			.max_ocv = analyzer->max_ocv.val,
			.min_ocv = analyzer->min_ocv.val,
			.max_temp = sanitizer->max_sanitized_temp.val,
			.min_temp = sanitizer->min_sanitized_temp.val
		};

		dcl_calc_inst_limit(algo_inputs, bms_algos);
		ccl_calc_inst_limit(algo_inputs, bms_algos);

		tx_thread_sleep(500);
	}
}