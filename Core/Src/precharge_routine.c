#include "precharge_routine.h"
#include <assert.h>
#include "debounce.h"

static void close_relay(void *args)
{
	prechargeconfig_t *precharge_config = (prechargeconfig_t *)args;
	set_gpo(*precharge_config->hv_plate->ic, precharge_config->gpo);
	precharge_config->air_switch_closed = true;
}

static void open_relay(void *args)
{
	prechargeconfig_t *precharge_config = (prechargeconfig_t *)args;
	reset_gpo(*precharge_config->hv_plate->ic, precharge_config->gpo);
	precharge_config->air_switch_closed = false;
}

prechargeconfig_t *precharge_init(hv_plate_t *hv_plate, GPO_2950 gpo,
				  float transition_ratio, float lower_ratio,
				  uint32_t debounce_time,
				  prechargeconfig_t *precharge_config)
{
	assert(precharge_config != NULL);
	assert(hv_plate != NULL);
	assert(transition_ratio > 0 && transition_ratio < 1);

	precharge_config->gpo = gpo;
	precharge_config->transition_ratio = transition_ratio;
	precharge_config->lower_ratio = lower_ratio;
	precharge_config->open_debounce_timer =
		(nertimer_t){ 0, 0, false, false };
	precharge_config->close_debounce_timer =
		(nertimer_t){ 0, 0, false, false };
	precharge_config->debounce_time = debounce_time;
	precharge_config->air_switch_closed = false;
}

void handle_precharge(prechargeconfig_t *precharge_config,
		       float batt_voltage, float ts_voltage)
{
	bool should_precharge =
		ts_voltage * precharge_config->lower_ratio >= batt_voltage;

	debounce(should_precharge,
		 &precharge_config->open_debounce_timer,
		 precharge_config->debounce_time, close_relay,
		 precharge_config);

	debounce(!should_precharge,
		 &precharge_config->close_debounce_timer,
		 precharge_config->debounce_time, open_relay,
		 precharge_config);
}
