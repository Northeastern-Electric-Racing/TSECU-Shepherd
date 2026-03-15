#include "precharge_routine.h"
#include <assert.h>
#include "debounce.h"

static void set_precharge_relay(cell_asic_2950 *ic, bool state)
{
	if (state) {
		set_gpo(ic, HV_CTRL_GPO);
	} else {
		reset_gpo(ic, HV_CTRL_GPO);
	}
}

static void close_relay(void *args)
{
	prechargeconfig_t *precharge_config = (prechargeconfig_t *)args;
	set_precharge_relay(precharge_config->hv_plate->ic, true);
	precharge_config->air_switch_closed = true;
	// TODO: Send the CAN Message here 
}

static void open_relay(void *args)
{
	prechargeconfig_t *precharge_config = (prechargeconfig_t *)args;
	set_precharge_relay(precharge_config->hv_plate->ic, false);
	precharge_config->air_switch_closed = false;
	// TODO: Send the CAN Message here
}

void precharge_init(prechargeconfig_t *precharge_config, hv_plate_t *hv_plate,
		    float transition_ratio, uint32_t debounce_time)
{
	assert(precharge_config != NULL);
	assert(hv_plate != NULL);
	assert(transition_ratio > 0 && transition_ratio < 1);

	precharge_config->transition_ratio = transition_ratio;
	precharge_config->open_debounce_timer =
		(nertimer_t){ 0, 0, false, false };
	precharge_config->close_debounce_timer =
		(nertimer_t){ 0, 0, false, false };
	precharge_config->debounce_time = debounce_time;
	precharge_config->air_switch_closed = false;
}

void handle_precharge(prechargeconfig_t *precharge_config)
{
	hv_plate_t *hv_plate = precharge_config->hv_plate;
	bool should_precharge = // TODO: mutex hv plate data
		hv_plate->ts_volts >=
		hv_plate->batt_volts * precharge_config->transition_ratio;

	debounce(should_precharge, &precharge_config->open_debounce_timer,
		 precharge_config->debounce_time, close_relay,
		 precharge_config);

	debounce(!should_precharge, &precharge_config->close_debounce_timer,
		 precharge_config->debounce_time, open_relay, precharge_config);
}

// PRECHARGE THREAD
void vPrecharge(ULONG args)
{
	PRINTLN_INFO("Starting Precharge thread...");

	hv_plate_t *hv_plate = (hv_plate_t *)args;
	nertimer_t *update_loop_timer = (nertimer_t *){0, 0, false, false};
	static const TELEMETRY_LOOP_TIMEOUT = 2000;

	prechargeconfig_t precharge_config;
	precharge_init(&precharge_config, hv_plate, 0.9f,
		       200 /* ms debounce time */);
	start_timer(&update_loop_timer, TELEMETRY_LOOP_TIMEOUT);

	for (;;) {
		handle_precharge(&precharge_config);
		if (!is_timer_active(&update_loop_timer) && is_timer_expired(&update_loop_timer)){
			// TODO: Send the CAN Message here 

			start_timer(&update_loop_timer, TELEMETRY_LOOP_TIMEOUT);
		}
		tx_thread_sleep(MS_TO_TICKS(50)); // TODO; fix thread timing
	}
}