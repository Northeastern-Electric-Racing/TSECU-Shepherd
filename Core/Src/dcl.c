#include "dcl.h"
#include "timer.h"
#include "current_limit_algo_utils.h"
#include "current_limit_algo_config.h"

/**
 * @brief DCL pulse control state and timers.
 *
 * Holds the internal state for discharge current pulse and cooldown handling.
 */
static current_limit_pulse_ctrl_t dcl_ctrl = { .cooldown_mode = COOLDOWN_ALWAYS,
					       .state =
						       CURRENT_LIMIT_STATE_REST,
					       .pulse_allowed = false };

/**
 * @brief Compute discharge current limit based on cell temperature.
 *
 * Applies a piecewise-linear derating of discharge current using configured
 * cell temperature thresholds.
 *
 * @param temperature_c  Cell temperature (deg C)
 *
 * @return Discharge current limit (A)
 */
static float dcl_from_temp(float temperature_c)
{
	float dcl = 0.0f;

	// Outside valid temperature range -> minimum current
	if ((temperature_c <= DCL_TEMP_MIN_C) ||
	    (temperature_c >= DCL_TEMP_MAX_C)) {
		dcl = DCL_MIN_CURRENT_A;
	} else if (temperature_c <
		   DCL_TEMP_RAMP_UP_END_C) { // Ramp up above low-temperature derate threshold
		dcl = linear_interpolate(temperature_c, DCL_TEMP_MIN_C,
					 DCL_TEMP_RAMP_UP_END_C,
					 DCL_MIN_CURRENT_A, DCL_MAX_CURRENT_A);
	} else if (temperature_c >
		   DCL_TEMP_RAMP_DOWN_START_C) { // Ramp down above high-temperature derate threshold
		dcl = linear_interpolate(temperature_c,
					 DCL_TEMP_RAMP_DOWN_START_C,
					 DCL_TEMP_MAX_C, DCL_MAX_CURRENT_A,
					 DCL_MIN_CURRENT_A);
	} else { // Within nominal temperature range -> maximum current
		dcl = DCL_MAX_CURRENT_A;
	}

	return dcl;
}

/**
 * @brief Compute discharge current limit based on cell open-circuit voltage.
 *
 * Applies a linear derating of discharge current using the minimum cell OCV.
 *
 * @param ocv  Minimum cell open-circuit voltage (V)
 *
 * @return Discharge current limit (A)
 */
static float dcl_from_cell_volt(float ocv)
{
	float dcl = 0.0f;

	// Below minimum OCV -> minimum current
	if (ocv <= DCL_OCV_MIN_V) {
		dcl = DCL_MIN_CURRENT_A;
	} else if (ocv <
		   DCL_OCV_DERATE_THRESH) { // Ramp down below OCV derate threshold
		dcl = linear_interpolate(ocv, DCL_OCV_MIN_V,
					 DCL_OCV_DERATE_THRESH,
					 DCL_MIN_CURRENT_A, DCL_MAX_CURRENT_A);
	} else { // Above OCV derate threshold -> maximum current
		dcl = DCL_MAX_CURRENT_A;
	}

	return dcl;
}

/**
 * @brief Compute instantaneous discharge current limit.
 *
 * Determines the most restrictive discharge current limit based on cell
 * temperature and cell open-circuit voltage.
 *
 * @param min_temp  Minimum cell temperature (deg C)
 * @param max_temp  Maximum cell temperature (deg C)
 * @param min_ocv   Minimum cell open-circuit voltage (V)
 *
 * @return Instantaneous discharge current limit (A)
 */
static float calc_inst_dcl(float min_temp, float max_temp, float min_ocv)
{
	float dcl_min_temp = dcl_from_temp(min_temp);
	float dcl_max_temp = dcl_from_temp(max_temp);
	float dcl_temp = fminf(dcl_min_temp, dcl_max_temp);

	float dcl_ocv = dcl_from_cell_volt(min_ocv);

	float dcl = fminf(dcl_temp, dcl_ocv);

	if (dcl < 0.0f) {
		dcl = 0.0f;
	} else if (dcl > DCL_MAX_CURRENT_A) {
		dcl = DCL_MAX_CURRENT_A;
	}

	return dcl;
}

void dcl_init(pulse_cooldown_mode_t cd_mode)
{
	if (cd_mode != COOLDOWN_ALWAYS && cd_mode != COOLDOWN_ON_FULL_PULSE) {
		printf("[DCL] Invalid cooldown mode!! Using default mode [COOLDOWN_ALWAYS]");
		dcl_ctrl.cooldown_mode = COOLDOWN_ALWAYS;
	} else {
		dcl_ctrl.cooldown_mode = cd_mode;
	}

	dcl_ctrl.state = CURRENT_LIMIT_STATE_REST;
	dcl_ctrl.pulse_allowed = false;

	cancel_timer(&dcl_ctrl.pulse_timer);
	cancel_timer(&dcl_ctrl.t_above);
	cancel_timer(&dcl_ctrl.t_below);
	cancel_timer(&dcl_ctrl.cooldown_timer);
}

void calc_dcl(current_limit_algo_inputs_t curr_lim_inputs,
	      bms_algos_t *const bms_algos)
{
	// Calculate instantaneous DCL from temperature and OCV
	float inst_dcl = calc_inst_dcl(curr_lim_inputs.min_temp,
				       curr_lim_inputs.max_temp,
				       curr_lim_inputs.min_ocv);

	// Default applied DCL is the instantaneous limit
	float applied_dcl = inst_dcl;

	// Check if pulse operation is allowed
	bool is_pulse_allowed =
		(inst_dcl >= (DCL_MAX_CURRENT_A - DCL_PULSE_ENABLE_MARGIN_A));

	if (is_pulse_allowed == true) {
		float pack_current = curr_lim_inputs.pack_current;

		// clang-format off
		switch (dcl_ctrl.state) {
			case CURRENT_LIMIT_STATE_REST:

				// Apply pulse current while monitoring entry condition
				applied_dcl = DCL_MAX_PULSE_CURRENT_A;
				
				if (pack_current > (DCL_MAX_CURRENT_A + TRIGGER_HYST_A)) {
					
					// Start debounce for pulse entry
					if (is_timer_active(&dcl_ctrl.t_above) == false) {

						start_timer(&dcl_ctrl.t_above, TRIGGER_DEBOUNCE_MS);

					} else if (is_timer_expired(&dcl_ctrl.t_above) == true) {	// Enter pulse after debounce expires

						dcl_ctrl.state = CURRENT_LIMIT_STATE_PULSE;
						start_timer(&dcl_ctrl.pulse_timer, DCL_PULSE_DURATION_MS);
					}

				} else {

					// Cancel debounce if condition clears
					cancel_timer(&dcl_ctrl.t_above);
				}
				break;

			case CURRENT_LIMIT_STATE_PULSE:

				// Apply pulse current during active pulse
				applied_dcl = DCL_MAX_PULSE_CURRENT_A;

				if (pack_current < (DCL_MAX_CURRENT_A - TRIGGER_HYST_A)) {

					// Start debounce for early pulse exit
					if (is_timer_active(&dcl_ctrl.t_below) == false) {

						start_timer(&dcl_ctrl.t_below, QUIET_DEBOUNCE_MS);

					} else if (is_timer_expired(&dcl_ctrl.t_below) == true) {	// Handle early pulse exit after debounce

						switch (dcl_ctrl.cooldown_mode)
						{
							case COOLDOWN_ON_FULL_PULSE:
								// Return to rest without cooldown
								dcl_ctrl.state = CURRENT_LIMIT_STATE_REST;
								applied_dcl = DCL_MAX_PULSE_CURRENT_A;
								break;
							case COOLDOWN_ALWAYS:
								// Enter cooldown on early exit
								dcl_ctrl.state = CURRENT_LIMIT_STATE_COOLDOWN;
								applied_dcl = DCL_COOLDOWN_CURRENT_A;
								start_timer(&dcl_ctrl.cooldown_timer, DCL_COOLDOWN_DURATION_MS);
								break;
							default:
								// Fallback to rest state
								dcl_ctrl.state = CURRENT_LIMIT_STATE_REST;
								break;
						}

						cancel_timer(&dcl_ctrl.t_below);
						cancel_timer(&dcl_ctrl.pulse_timer);
					}

				} else {

					// Cancel debounce if condition clears
					cancel_timer(&dcl_ctrl.t_below);
				}

				// Exit pulse and enter cooldown after maximum allowed pulse duration
				if (is_timer_expired(&dcl_ctrl.pulse_timer) == true) {

					dcl_ctrl.state = CURRENT_LIMIT_STATE_COOLDOWN;
					cancel_timer(&dcl_ctrl.pulse_timer);
					start_timer(&dcl_ctrl.cooldown_timer, DCL_COOLDOWN_DURATION_MS);
					applied_dcl = DCL_COOLDOWN_CURRENT_A;

				}
				break;

			case CURRENT_LIMIT_STATE_COOLDOWN:

				// Apply cooldown current limit
				applied_dcl = DCL_COOLDOWN_CURRENT_A;

				// Exit cooldown after timer expires
				if (is_timer_expired(&dcl_ctrl.cooldown_timer) == true) {

					dcl_ctrl.state = CURRENT_LIMIT_STATE_REST;
				}
				break;

			default:
				// Fallback to rest state
				dcl_ctrl.state = CURRENT_LIMIT_STATE_REST;
				break;
		}

		// clang-format on
	} else {
		// Reset pulse state if pulse eligibility is lost
		if (dcl_ctrl.pulse_allowed == true) {
			dcl_ctrl.state = CURRENT_LIMIT_STATE_REST;
			cancel_timer(&dcl_ctrl.t_above);
			cancel_timer(&dcl_ctrl.t_below);
			cancel_timer(&dcl_ctrl.pulse_timer);
			cancel_timer(&dcl_ctrl.cooldown_timer);
		}
	}

	// Track pulse eligibility edge
	dcl_ctrl.pulse_allowed = is_pulse_allowed;

	// Publish applied discharge current limit
	bms_algos->cont_DCL = applied_dcl;
}