#include "dcl.h"
#include "timer.h"

#define DCL_TEMP_MIN_C		   (-20.0f)
#define DCL_TEMP_RAMP_UP_END_C	   (10.0f)
#define DCL_TEMP_RAMP_DOWN_START_C (50.0f)
#define DCL_TEMP_MAX_C		   (60.0f)

#define DCL_OCV_MIN_V	      (2.5f)
#define DCL_OCV_DERATE_THRESH (3.7f)

#define DCL_MAX_CURRENT_A (180.0f)
#define DCL_MIN_CURRENT_A (30.0f)

#define PULSE_PERCENT_OF_CONT	 (110.0f)
#define COOLDOWN_PERCENT_OF_CONT (90.0f)
#define PULSE_DURATION_MS	 (3000UL)
#define COOLDOWN_DURATION_MS	 (10000UL)

#define TRIGGER_HYST_A	    (3.0f)
#define TRIGGER_DEBOUNCE_MS (100UL)
#define QUIET_DEBOUNCE_MS   (100UL)

typedef enum {
	DCL_STATE_REST = 0,
	DCL_STATE_PULSE,
	DCL_STATE_COOLDOWN
} dcl_state_t;

typedef struct {
	dcl_state_t state;
	nertimer_t pulse_timer;
	nertimer_t t_above;
	nertimer_t t_below;
	nertimer_t rest_timer;
	bool pulse_allowed;
} dcl_ctrl_t;

static dcl_ctrl_t dcl_ctrl;

static inline float linear_interpolate(float x, float x1, float x2, float y1,
				       float y2)
{
	return y1 + ((x - x1) * (y2 - y1) / (x2 - x1));
}

static float dcl_from_temp(float temperature_c)
{
	float dcl = 0.0f;

	if (temperature_c <= DCL_TEMP_MIN_C ||
	    temperature_c >= DCL_TEMP_MAX_C) {
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

static float calc_inst_dcl(const analyzer_t *const analyzer,
			   const hv_plate_t *const hv_plate)
{
	float dcl_min_temp = dcl_from_temp(analyzer->min_temp.val);
	float dcl_max_temp = dcl_from_temp(analyzer->max_temp.val);
	float dcl_temp = (dcl_min_temp < dcl_max_temp) ? dcl_min_temp :
							 dcl_max_temp;

	float dcl_ocv = dcl_from_cell_volt(analyzer->min_ocv.val);
	float dcl = (dcl_temp < dcl_ocv) ? dcl_temp : dcl_ocv;

	if (dcl < 0.0f) {
		dcl = 0.0f;
	}

	if (dcl > DCL_MAX_CURRENT_A) {
		dcl = DCL_MAX_CURRENT_A;
	}

	return dcl;
}

void dcl_init(void)
{
	dcl_ctrl.state = DCL_STATE_REST;
	cancel_timer(&dcl_ctrl.pulse_timer);
	cancel_timer(&dcl_ctrl.t_above);
	cancel_timer(&dcl_ctrl.t_below);
	cancel_timer(&dcl_ctrl.rest_timer);
	dcl_ctrl.pulse_allowed = false;
}

void calc_cont_dcl(const analyzer_t *const analyzer,
		   const hv_plate_t *const hv_plate,
		   bms_algos_t *const bms_algos)
{
	float cont_dcl = calc_inst_dcl(analyzer, hv_plate);
	bool in_safe_zone = (cont_dcl == DCL_MAX_CURRENT_A);

	float applied_dcl = cont_dcl;

	if (!in_safe_zone && dcl_ctrl.pulse_allowed) {
		dcl_ctrl.state = DCL_STATE_REST;
		cancel_timer(&dcl_ctrl.t_above);
		cancel_timer(&dcl_ctrl.t_below);
		cancel_timer(&dcl_ctrl.pulse_timer);
		cancel_timer(&dcl_ctrl.rest_timer);
	}

	if (in_safe_zone) {
		float dcl_pulse_limit =
			DCL_MAX_CURRENT_A * (PULSE_PERCENT_OF_CONT / 100.0f);
		float dcl_rest_limit =
			DCL_MAX_CURRENT_A * (COOLDOWN_PERCENT_OF_CONT / 100.0f);
		float pack_current = hv_plate->pack_current;

		switch (dcl_ctrl.state) {
		case DCL_STATE_REST:
			applied_dcl = dcl_pulse_limit;
			if (pack_current > DCL_MAX_CURRENT_A + TRIGGER_HYST_A) {
				if (!is_timer_active(&dcl_ctrl.t_above)) {
					start_timer(&dcl_ctrl.t_above,
						    TRIGGER_DEBOUNCE_MS);
				} else if (is_timer_expired(
						   &dcl_ctrl.t_above)) {
					dcl_ctrl.state = DCL_STATE_PULSE;
					cancel_timer(&dcl_ctrl.t_above);
					start_timer(&dcl_ctrl.pulse_timer,
						    PULSE_DURATION_MS);
				}
			} else {
				cancel_timer(&dcl_ctrl.t_above);
			}
			break;

		case DCL_STATE_PULSE:
			applied_dcl = dcl_pulse_limit;
			if (pack_current < DCL_MAX_CURRENT_A - TRIGGER_HYST_A) {
				if (!is_timer_active(&dcl_ctrl.t_below)) {
					start_timer(&dcl_ctrl.t_below,
						    QUIET_DEBOUNCE_MS);
				} else if (is_timer_expired(
						   &dcl_ctrl.t_below)) {
					dcl_ctrl.state = DCL_STATE_COOLDOWN;
					cancel_timer(&dcl_ctrl.t_below);
					cancel_timer(&dcl_ctrl.pulse_timer);
					start_timer(&dcl_ctrl.rest_timer,
						    COOLDOWN_DURATION_MS);
					applied_dcl = dcl_rest_limit;
				}
			} else {
				cancel_timer(&dcl_ctrl.t_below);
			}

			if (is_timer_expired(&dcl_ctrl.pulse_timer)) {
				dcl_ctrl.state = DCL_STATE_COOLDOWN;
				cancel_timer(&dcl_ctrl.pulse_timer);
				start_timer(&dcl_ctrl.rest_timer,
					    COOLDOWN_DURATION_MS);
				applied_dcl = dcl_rest_limit;
			}
			break;

		case DCL_STATE_COOLDOWN:
			applied_dcl = dcl_rest_limit;
			if (is_timer_expired(&dcl_ctrl.rest_timer)) {
				dcl_ctrl.state = DCL_STATE_REST;
				cancel_timer(&dcl_ctrl.rest_timer);
				applied_dcl = dcl_pulse_limit;
			}
			break;

		default:
			dcl_ctrl.state = DCL_STATE_REST;
			break;
		}
	}

	dcl_ctrl.pulse_allowed = in_safe_zone;

	bms_algos->cont_DCL = applied_dcl;
}