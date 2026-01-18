#include "dcl.h"
#include "timer.h"
#include "current_limit_algo_utils.h"
#include "current_limit_algo_config.h"

static current_limit_pulse_ctrl_t dcl_ctrl = { .state =
						       CURRENT_LIMIT_STATE_REST,
					       .pulse_allowed = false };

static float dcl_from_temp(float temperature_c)
{
	float dcl = 0.0f;

	if ((temperature_c <= DCL_TEMP_MIN_C) ||
	    (temperature_c >= DCL_TEMP_MAX_C)) {
		dcl = DCL_MIN_CURRENT_A;
	} else if (temperature_c < DCL_TEMP_RAMP_UP_END_C) {
		dcl = linear_interpolate(temperature_c, DCL_TEMP_MIN_C,
					 DCL_TEMP_RAMP_UP_END_C,
					 DCL_MIN_CURRENT_A, DCL_MAX_CURRENT_A);
	} else if (temperature_c > DCL_TEMP_RAMP_DOWN_START_C) {
		dcl = linear_interpolate(temperature_c,
					 DCL_TEMP_RAMP_DOWN_START_C,
					 DCL_TEMP_MAX_C, DCL_MAX_CURRENT_A,
					 DCL_MIN_CURRENT_A);
	} else {
		dcl = DCL_MAX_CURRENT_A;
	}

	return dcl;
}

static float dcl_from_cell_volt(float ocv)
{
	float dcl = 0.0f;

	if (ocv <= DCL_OCV_MIN_V) {
		dcl = DCL_MIN_CURRENT_A;
	} else if (ocv < DCL_OCV_DERATE_THRESH) {
		dcl = linear_interpolate(ocv, DCL_OCV_MIN_V,
					 DCL_OCV_DERATE_THRESH,
					 DCL_MIN_CURRENT_A, DCL_MAX_CURRENT_A);
	} else {
		dcl = DCL_MAX_CURRENT_A;
	}

	return dcl;
}

static float calc_inst_dcl(const analyzer_t *const analyzer)
{
	float dcl_min_temp = dcl_from_temp(analyzer->min_temp.val);
	float dcl_max_temp = dcl_from_temp(analyzer->max_temp.val);
	float dcl_temp = fminf(dcl_min_temp, dcl_max_temp);

	float dcl_ocv = dcl_from_cell_volt(analyzer->min_ocv.val);

	float dcl = fminf(dcl_temp, dcl_ocv);

	if (dcl < 0.0f) {
		dcl = 0.0f;
	} else if (dcl > DCL_MAX_CURRENT_A) {
		dcl = DCL_MAX_CURRENT_A;
	}

	return dcl;
}

void dcl_init(void)
{
	dcl_ctrl.state = CURRENT_LIMIT_STATE_REST;
	dcl_ctrl.pulse_allowed = false;

	cancel_timer(&dcl_ctrl.pulse_timer);
	cancel_timer(&dcl_ctrl.t_above);
	cancel_timer(&dcl_ctrl.t_below);
	cancel_timer(&dcl_ctrl.rest_timer);
}

void calc_dcl(const analyzer_t *const analyzer,
	      const hv_plate_t *const hv_plate, bms_algos_t *const bms_algos)
{
	float inst_dcl = calc_inst_dcl(analyzer);
	float applied_dcl = inst_dcl;

	bool is_pulse_allowed =
		(inst_dcl >= (DCL_MAX_CURRENT_A - DCL_PULSE_ENABLE_MARGIN_A));

	if (is_pulse_allowed == true) {
		float pack_current = hv_plate->pack_current;

		// clang-format off
		switch (dcl_ctrl.state) {
		case CURRENT_LIMIT_STATE_REST:
			applied_dcl = DCL_MAX_PULSE_CURRENT_A;
			if (pack_current > (DCL_MAX_CURRENT_A + TRIGGER_HYST_A)) {
				
				if (is_timer_active(&dcl_ctrl.t_above) == false) {

					start_timer(&dcl_ctrl.t_above, TRIGGER_DEBOUNCE_MS);

				} else if (is_timer_expired(&dcl_ctrl.t_above) == true) {

					dcl_ctrl.state = CURRENT_LIMIT_STATE_PULSE;
					start_timer(&dcl_ctrl.pulse_timer, DCL_PULSE_DURATION_MS);

				}

			} else {

				cancel_timer(&dcl_ctrl.t_above);

			}
			break;

		case CURRENT_LIMIT_STATE_PULSE:
			applied_dcl = DCL_MAX_PULSE_CURRENT_A;
			if (pack_current < (DCL_MAX_CURRENT_A - TRIGGER_HYST_A)) {

				if (is_timer_active(&dcl_ctrl.t_below) == false) {

					start_timer(&dcl_ctrl.t_below, QUIET_DEBOUNCE_MS);

				} else if (is_timer_expired(&dcl_ctrl.t_below) == true) {

					dcl_ctrl.state = CURRENT_LIMIT_STATE_COOLDOWN;
					cancel_timer(&dcl_ctrl.t_below);
					cancel_timer(&dcl_ctrl.pulse_timer);
					start_timer(&dcl_ctrl.rest_timer, DCL_COOLDOWN_DURATION_MS);
					applied_dcl = DCL_COOLDOWN_CURRENT_A;
					
				}

			} else {

				cancel_timer(&dcl_ctrl.t_below);

			}

			if (is_timer_expired(&dcl_ctrl.pulse_timer) == true) {

				dcl_ctrl.state = CURRENT_LIMIT_STATE_COOLDOWN;
				cancel_timer(&dcl_ctrl.pulse_timer);
				start_timer(&dcl_ctrl.rest_timer, DCL_COOLDOWN_DURATION_MS);
				applied_dcl = DCL_COOLDOWN_CURRENT_A;

			}
			break;

		case CURRENT_LIMIT_STATE_COOLDOWN:
			applied_dcl = DCL_COOLDOWN_CURRENT_A;
			if (is_timer_expired(&dcl_ctrl.rest_timer) == true) {

				dcl_ctrl.state = CURRENT_LIMIT_STATE_REST;
			}
			break;

		default:
			dcl_init();
			break;
		}

		// clang-format on
	} else {

		if (dcl_ctrl.pulse_allowed == true) {
			dcl_ctrl.state = CURRENT_LIMIT_STATE_REST;
			cancel_timer(&dcl_ctrl.t_above);
			cancel_timer(&dcl_ctrl.t_below);
			cancel_timer(&dcl_ctrl.pulse_timer);
			cancel_timer(&dcl_ctrl.rest_timer);
		}
		
	}

	dcl_ctrl.pulse_allowed = is_pulse_allowed;

	bms_algos->cont_DCL = applied_dcl;
}