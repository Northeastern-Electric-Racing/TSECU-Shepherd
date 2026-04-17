#include "precharge_routine.h"
#include <assert.h>
#include "debounce.h"

#define MINIMUM_PACK_VOLTAGE 325.0f

#define NUM_SAMPLES_FOR_AVG 20
#define PRRECHARGE_TRIGGER_THRESHOLD 0.95f
s
#define TS_VOLT_BUFFER 5.0f
#define BATT_VOLT_BUFFER 10.0f

#define PRECHARGE_TOGGLE_TIME 200
#define PRECHARGE_FLOATING_FAULT_TIME 10000

typedef struct {
	uint8_t size;
	float sample[NUM_SAMPLES_FOR_AVG];
	uint8_t index;
} sample_buffer_t;

static sample_buffer_t ts_volts_sample_buffer = { 0 };
static sample_buffer_t batt_volts_sample_buffer = { 0 };

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
	set_precharge_relay(&precharge_config->hv_plate->ic, true);
	if (precharge_config->precharge_state == PRECHARGE_CLOSED) {
		return;
	}
	precharge_config->precharge_state = PRECHARGE_CLOSED;
	send_precharge_status((uint8_t)PRECHARGE_CLOSED);
}

static void open_relay(void *args)
{
	prechargeconfig_t *precharge_config = (prechargeconfig_t *)args;
	set_precharge_relay(&precharge_config->hv_plate->ic, false);
	if (precharge_config->precharge_state == PRECHARGE_OPEN) {
		return;
	}
	precharge_config->precharge_state = PRECHARGE_OPEN;
	send_precharge_status((uint8_t)PRECHARGE_OPEN);
}

static void send_floating_precharge_fault(void *args)
{
	prechargeconfig_t *precharge_config = (prechargeconfig_t *)args;
	set_precharge_relay(&precharge_config->hv_plate->ic, false); // open relay if floating
	if (precharge_config->precharge_state == PRECHARGE_FLOATING) {
		return;
	}
	precharge_config->precharge_state = PRECHARGE_FLOATING;
	send_precharge_status((uint8_t)PRECHARGE_FLOATING);	
}

static void init_sample_buffer(sample_buffer_t *buffer)
{
	buffer->size = 0;
	buffer->index = 0;
	memset(buffer->sample, 0, sizeof(buffer->sample));
}

static void update_sample_buffer(sample_buffer_t *buffer, float new_sample)
{
	if (buffer->size < NUM_SAMPLES_FOR_AVG) {
		buffer->sample[buffer->index] = new_sample;
		buffer->index = (buffer->index + 1) % NUM_SAMPLES_FOR_AVG;
		buffer->size++;
	} else {
		buffer->sample[buffer->index] = new_sample;
		buffer->index = (buffer->index + 1) % NUM_SAMPLES_FOR_AVG;
	}
}

static float get_average_sample(sample_buffer_t *buffer)
{
	float average = 0.0f;

	if (buffer->size == 0) {
		return 0.0f;
	}

	float sum = 0.0f;
	for (uint8_t i = 0; i < buffer->size; i++) {
		sum += buffer->sample[i];
	}
	average = sum / buffer->size;

	return average;
}

static precharge_state_t get_precharge_state(float ts_volts_avg, float batt_volts_avg,
					    float transition_ratio)
{
	if (batt_volts_avg < (MINIMUM_PACK_VOLTAGE - BATT_VOLT_BUFFER)) {
		return PRECHARGE_OPEN; 
	}

	if (ts_volts_avg >= batt_volts_avg * transition_ratio) {
		return PRECHARGE_CLOSED;
	} else if (ts_volts_avg < batt_volts_avg * transition_ratio &&
		   ts_volts_avg > TS_VOLT_BUFFER) {
		return PRECHARGE_FLOATING;
	} else {
		return PRECHARGE_OPEN;
	}
}

void precharge_init(prechargeconfig_t *precharge_config, hv_plate_t *hv_plate,
		    float transition_ratio)
{
	assert(precharge_config != NULL);
	assert(hv_plate != NULL);
	assert(transition_ratio > 0 && transition_ratio < 1);

	precharge_config->precharge_state = PRECHARGE_OPEN;

	precharge_config->hv_plate = hv_plate;
	precharge_config->transition_ratio = transition_ratio;
	precharge_config->open_debounce_timer =
		(nertimer_t){ 0, 0, false, false };
	precharge_config->close_debounce_timer =
		(nertimer_t){ 0, 0, false, false };
	precharge_config->floating_debounce_timer =
		(nertimer_t){ 0, 0, false, false };
}

void handle_precharge(prechargeconfig_t *precharge_config)
{
	hv_plate_t *hv_plate = precharge_config->hv_plate;

	update_sample_buffer(&ts_volts_sample_buffer, hv_plate->ts_volts);
	update_sample_buffer(&batt_volts_sample_buffer, hv_plate->batt_volts);

	float ts_volts_avg = get_average_sample(&ts_volts_sample_buffer);
	float batt_volts_avg = get_average_sample(&batt_volts_sample_buffer);

	precharge_state_t precharge_state = get_precharge_state(ts_volts_avg, batt_volts_avg,
					    precharge_config->transition_ratio);

	debounce(precharge_state == PRECHARGE_CLOSED, &precharge_config->open_debounce_timer,
		 PRECHARGE_TOGGLE_TIME, close_relay,
		 precharge_config);

	debounce(precharge_state == PRECHARGE_OPEN, &precharge_config->close_debounce_timer,
		 PRECHARGE_TOGGLE_TIME, open_relay, precharge_config);

	debounce(precharge_state == PRECHARGE_FLOATING, &precharge_config->floating_debounce_timer,
		 PRECHARGE_FLOATING_FAULT_TIME, send_floating_precharge_fault, precharge_config);
}

// PRECHARGE THREAD
void vPrecharge(ULONG args)
{
	PRINTLN_INFO("Starting Precharge thread...");

	hv_plate_t *hv_plate = (hv_plate_t *)args;
	nertimer_t update_loop_timer = { 0 };
	static const uint16_t TELEMETRY_LOOP_TIMEOUT = 2000;

	prechargeconfig_t precharge_config;
	precharge_init(&precharge_config, hv_plate, PRRECHARGE_TRIGGER_THRESHOLD);

	init_sample_buffer(&ts_volts_sample_buffer);
	init_sample_buffer(&batt_volts_sample_buffer);

	start_timer(&update_loop_timer, TELEMETRY_LOOP_TIMEOUT);

	for (;;) {
		handle_precharge(&precharge_config);

		if (is_timer_expired(&update_loop_timer) && !is_timer_active(&update_loop_timer)) {
			send_precharge_status(
				(uint8_t)precharge_config.precharge_state);
			start_timer(&update_loop_timer, TELEMETRY_LOOP_TIMEOUT);
		}

		tx_thread_sleep(50);
	}
}