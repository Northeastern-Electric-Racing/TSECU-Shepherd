#include "ccl.h"
#include "timer.h"
#include "c_utils.h"
#include "bms_algos.h"
#include "current_limit_algo_config.h"
#include "shep_mutexes.h"
#include <math.h>

/**
 * @brief CCL pulse control state and timers.
 *
 * Holds the internal state for charge current pulse and cooldown handling.
 */
static current_limit_pulse_ctrl_t ccl_ctrl = { .cooldown_mode = COOLDOWN_ALWAYS,
					       .state =
						       CURRENT_LIMIT_STATE_REST,
					       .pulse_allowed = false };

/**
 * @brief Compute charge current limit based on cell temperature.
 *
 * Applies a piecewise-linear derating of charge current using configured
 * cell temperature thresholds.
 *
 * @param temperature_c  Cell temperature (deg C)
 *
 * @return Charge current limit (A)
 */
static float ccl_from_temp(float temperature_c)
{
	float ccl = 0.0f;

	// Outside valid temperature range -> minimum charge current
	if ((temperature_c <= CCL_TEMP_MIN_C) ||
	    (temperature_c >= CCL_TEMP_MAX_C)) {
		ccl = CCL_MIN_CURRENT_A;
	} else if (temperature_c <
		   CCL_TEMP_RAMP_UP_END_C) { // Ramp up charge current above low-temperature derate threshold
		ccl = linear_interpolate(temperature_c, CCL_TEMP_MIN_C,
					 CCL_TEMP_RAMP_UP_END_C,
					 CCL_MIN_CURRENT_A, CCL_MAX_CURRENT_A);
	} else if (temperature_c >
		   CCL_TEMP_RAMP_DOWN_START_C) { // Ramp down charge current above high-temperature derate threshold
		ccl = linear_interpolate(temperature_c,
					 CCL_TEMP_RAMP_DOWN_START_C,
					 CCL_TEMP_MAX_C, CCL_MAX_CURRENT_A,
					 CCL_MIN_CURRENT_A);
	} else { // Within nominal temperature range -> maximum charge current
		ccl = CCL_MAX_CURRENT_A;
	}

	return ccl;
}

/**
 * @brief Compute charge current limit based on cell open-circuit voltage.
 *
 * Applies a linear derating of charge current using the maximum cell OCV.
 *
 * @param ocv  Minimum cell open-circuit voltage (V)
 *
 * @return Charge current limit (A)
 */
static float ccl_from_cell_volt(float ocv)
{
	float ccl = 0.0f;

	// Above maximum OCV -> minimum charge current
	if (ocv >= CCL_OCV_MAX_V) {
		ccl = CCL_MIN_CURRENT_A;
	} else if (ocv >
		   CCL_OCV_DERATE_THRESH) { // Ramp down charge current above OCV derate threshold
		ccl = linear_interpolate(ocv, CCL_OCV_DERATE_THRESH,
					 CCL_OCV_MAX_V, CCL_MAX_CURRENT_A,
					 CCL_MIN_CURRENT_A);
	} else { // Below and at OCV derate threshold -> maximum charge current
		ccl = CCL_MAX_CURRENT_A;
	}

	return ccl;
}

void ccl_init(pulse_cooldown_mode_t cooldown_mode)
{
	if (cooldown_mode != COOLDOWN_ALWAYS &&
	    cooldown_mode != COOLDOWN_ON_FULL_PULSE) {
		PRINTLN_WARNING(
			"[CCL] Invalid cooldown mode (%d). Defaulting to COOLDOWN_ALWAYS",
			cooldown_mode);
		ccl_ctrl.cooldown_mode = COOLDOWN_ALWAYS;
	} else {
		ccl_ctrl.cooldown_mode = cooldown_mode;
	}

	ccl_ctrl.state = CURRENT_LIMIT_STATE_REST;
	ccl_ctrl.pulse_allowed = false;

	cancel_timer(&ccl_ctrl.pulse_timer);
	cancel_timer(&ccl_ctrl.t_above);
	cancel_timer(&ccl_ctrl.t_below);
	cancel_timer(&ccl_ctrl.cooldown_timer);
}

void ccl_calc_inst_limit(current_limit_algo_inputs_t curr_lim_inputs,
			 bms_algos_t *const bms_algos)
{
	float ccl_min_temp = ccl_from_temp(curr_lim_inputs.min_temp);
	float ccl_max_temp = ccl_from_temp(curr_lim_inputs.max_temp);
	float ccl_temp = fminf(ccl_min_temp, ccl_max_temp);

	float ccl_ocv = ccl_from_cell_volt(curr_lim_inputs.max_ocv);

	float ccl = fminf(ccl_temp, ccl_ocv);

	if (ccl < 0.0f) {
		ccl = 0.0f;
	} else if (ccl > CCL_MAX_CURRENT_A) {
		ccl = CCL_MAX_CURRENT_A;
	}

	mutex_get(&bms_algos_mutex);
	bms_algos->inst_CCL = ccl;
	mutex_put(&bms_algos_mutex);
}

void ccl_calc_cont_limit(float pack_current, bms_algos_t *const bms_algos)
{
	mutex_get(&bms_algos_mutex);
	float inst_ccl = bms_algos->inst_CCL;
	mutex_put(&bms_algos_mutex);

	// Default applied CCL is the instantaneous limit
	float applied_ccl = inst_ccl;

	// Check if pulse operation is allowed
	bool is_pulse_allowed =
		(inst_ccl >= (CCL_MAX_CURRENT_A - PULSE_ENABLE_MARGIN_A));

	if (is_pulse_allowed == true) {
		// clang-format off
		switch (ccl_ctrl.state) {
			case CURRENT_LIMIT_STATE_REST:

				// Apply pulse current while monitoring entry condition
				applied_ccl = CCL_MAX_PULSE_CURRENT_A;
				
				// Evaluate CCL pulse logic only for negative (charging) current
				if (pack_current < 0.0f)
				{
					float pack_current_abs_val = fabsf(pack_current);

					// Use absolute value of current for CCL threshold comparisons 
					if (pack_current_abs_val > (CCL_MAX_CURRENT_A + CURRENT_TRIGGER_HYST_A)) {
					
						// Start debounce for pulse entry
						if (is_timer_active(&ccl_ctrl.t_above) == false) {

							start_timer(&ccl_ctrl.t_above, TRIGGER_DEBOUNCE_MS);

						} else if (is_timer_expired(&ccl_ctrl.t_above) == true) {	// Enter pulse after debounce expires

							ccl_ctrl.state = CURRENT_LIMIT_STATE_PULSE;
							start_timer(&ccl_ctrl.pulse_timer, CCL_PULSE_DURATION_MS);
						}

					} else {

						// Cancel debounce if condition clears
						cancel_timer(&ccl_ctrl.t_above);
					}	
				}
				break;

			case CURRENT_LIMIT_STATE_PULSE:

				// Apply pulse current during active pulse
				applied_ccl = CCL_MAX_PULSE_CURRENT_A;
				
				// Evaluate CCL pulse logic only for negative (charging) current
				if (pack_current < 0.0f)
				{
					float pack_current_abs_value = fabsf(pack_current);

					// Use absolute value of current for CCL threshold comparisons 
					if (pack_current_abs_value < CCL_MAX_CURRENT_A) {

						// Start debounce for early pulse exit
						if (is_timer_active(&ccl_ctrl.t_below) == false) {

							start_timer(&ccl_ctrl.t_below, QUIET_DEBOUNCE_MS);

						} else if (is_timer_expired(&ccl_ctrl.t_below) == true) {	// Handle early pulse exit after debounce

							switch (ccl_ctrl.cooldown_mode)
							{
								case COOLDOWN_ON_FULL_PULSE:
									// Return to rest without cooldown
									ccl_ctrl.state = CURRENT_LIMIT_STATE_REST;
									applied_ccl = CCL_MAX_PULSE_CURRENT_A;
									break;
								case COOLDOWN_ALWAYS:
									// Enter cooldown on early exit
									ccl_ctrl.state = CURRENT_LIMIT_STATE_COOLDOWN;
									applied_ccl = CCL_COOLDOWN_CURRENT_A;
									start_timer(&ccl_ctrl.cooldown_timer, CCL_COOLDOWN_DURATION_MS);
									break;
								default:
									// Fallback to rest state
									ccl_ctrl.state = CURRENT_LIMIT_STATE_REST;
									break;
							}

							cancel_timer(&ccl_ctrl.t_below);
							cancel_timer(&ccl_ctrl.pulse_timer);
						}

					} else {

						// Cancel debounce if condition clears
						cancel_timer(&ccl_ctrl.t_below);
					}
				}

				// Exit pulse and enter cooldown after maximum allowed pulse duration
				if (is_timer_expired(&ccl_ctrl.pulse_timer) == true) {

					ccl_ctrl.state = CURRENT_LIMIT_STATE_COOLDOWN;
					cancel_timer(&ccl_ctrl.pulse_timer);
					start_timer(&ccl_ctrl.cooldown_timer, CCL_COOLDOWN_DURATION_MS);
					applied_ccl = CCL_COOLDOWN_CURRENT_A;

				}
				break;

			case CURRENT_LIMIT_STATE_COOLDOWN:

				// Apply cooldown current limit
				applied_ccl = CCL_COOLDOWN_CURRENT_A;

				// Exit cooldown after timer expires
				if (is_timer_expired(&ccl_ctrl.cooldown_timer) == true) {

					ccl_ctrl.state = CURRENT_LIMIT_STATE_REST;
				}
				break;

			default:
				// Fallback to rest state
				ccl_ctrl.state = CURRENT_LIMIT_STATE_REST;
				break;
		}

		// clang-format on
	} else {
		// Reset pulse state if pulse eligibility is lost
		if (ccl_ctrl.pulse_allowed == true) {
			ccl_ctrl.state = CURRENT_LIMIT_STATE_REST;
			cancel_timer(&ccl_ctrl.t_above);
			cancel_timer(&ccl_ctrl.t_below);
			cancel_timer(&ccl_ctrl.pulse_timer);
			cancel_timer(&ccl_ctrl.cooldown_timer);
		}
	}

	// Track pulse eligibility edge
	ccl_ctrl.pulse_allowed = is_pulse_allowed;

	mutex_get(&bms_algos_mutex);
	// Publish applied charge current limit
	bms_algos->cont_CCL = applied_ccl;
	mutex_put(&bms_algos_mutex);
}