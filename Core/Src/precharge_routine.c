#include "precharge_routine.h"

prechargeconfig_t *precharge_init(cell_asic_2950 ic, SPI_HandleTypeDef *hspi,
				  GPO_2950 gpo, float transition_ratio,
				  float lower_ratio, uint32_t debounce_time,
				  prechargeconfig_t *precharge_config)
{
	precharge_config->ic = ic;
	precharge_config->hspi = hspi;
	precharge_config->gpo = gpo;
	precharge_config->transition_ratio = transition_ratio;
	precharge_config->lower_ratio = lower_ratio;
	precharge_config->debounce_timer = (nertimer_t){ 0, 0, false, false };
	precharge_config->debounce_time = debounce_time;
}

void precharge_run(prechargeconfig_t *precharge_config)
{
	if (precharge_config == NULL) {
		return;
	}
	bool air_switch_closed = false;
	for (;;) {
		float batt_v = read_batt_voltage_volts(precharge_config->ic,
						       precharge_config->hspi);
		float ts_v = read_ts_voltage_volts(precharge_config->ic,
						   precharge_config->hspi);
		// Chcek if timer is active, cannot change switch state in that case.
		if (is_timer_active(&precharge_config->debounce_timer)) {
			if (is_timer_expired(
				    &precharge_config->debounce_timer)) {
				cancel_timer(&precharge_config->debounce_timer);
			} else {
				// SOME SORT OF DELAY?
				continue;
			}
		} else if (!air_switch_closed) {
			// Check if charged up until threshold.
			// Should there be another check for depleted battery voltage?
			if (ts_v >=
			    (batt_v * precharge_config->transition_ratio)) {
				set_gpo(precharge_config->ic,
					precharge_config->hspi,
					precharge_config->gpo);
				air_switch_closed = true;
				start_timer(&precharge_config->debounce_timer,
					    precharge_config->debounce_time);
			}
		} else {
			// Reopen capacitor if voltage too low.
			if (ts_v <= (batt_v * precharge_config->lower_ratio)) {
				reset_gpo(precharge_config->ic,
					  precharge_config->hspi,
					  precharge_config->gpo);
				air_switch_closed = false;
				// I would assume there could be a voltage spike here as well.
				start_timer(&precharge_config->debounce_timer,
					    precharge_config->debounce_time);
			}
		}
		// DELAY?
	}
}