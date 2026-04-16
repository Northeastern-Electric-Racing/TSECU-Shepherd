#include "precharge_routine.h"
#include <assert.h>
#include "debounce.h"

#define BATT_VOLTS_PRECHARGE_THRESHOLD 60.0f
#define NUM_SAMPLES_FOR_AVG 20

typedef struct {
	uint8_t capacity;
	uint8_t size;
	float sample[capacity];
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
	precharge_config->air_switch_closed = true;
	send_precharge_status(precharge_config->air_switch_closed);
}

static void open_relay(void *args)
{
	prechargeconfig_t *precharge_config = (prechargeconfig_t *)args;
	set_precharge_relay(&precharge_config->hv_plate->ic, false);
	precharge_config->air_switch_closed = false;
	send_precharge_status(precharge_config->air_switch_closed);
}

static void init_sample_buffer(sample_buffer_t *buffer, uint8_t capacity)
{
	buffer->capacity = capacity;
	buffer->size = 0;
	buffer->index = 0;
	memset(buffer->sample, 0, sizeof(buffer->sample));
}

static void update_sample_buffer(sample_buffer_t *buffer, float new_sample)
{
	if (buffer->size < buffer->capacity) {
		buffer->sample[buffer->index] = new_sample;
		buffer->index = (buffer->index + 1) % buffer->capacity;
		buffer->size++;
	} else {
		buffer->sample[buffer->index] = new_sample;
		buffer->index = (buffer->index + 1) % buffer->capacity;
	}
}

static float get_average_sample(sample_buffer_t *buffer)
{
	float average = 0.0f;

	if (buffer->size == 0) {
		average = 0.0f;
		return;
	}

	float sum = 0.0f;
	for (uint8_t i = 0; i < buffer->size; i++) {
		sum += buffer->sample[i];
	}
	average = sum / buffer->size;

	return average;
}

void precharge_init(prechargeconfig_t *precharge_config, hv_plate_t *hv_plate,
		    float transition_ratio, uint32_t debounce_time)
{
	assert(precharge_config != NULL);
	assert(hv_plate != NULL);
	assert(transition_ratio > 0 && transition_ratio < 1);

	precharge_config->hv_plate = hv_plate;
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
	update_sample_buffer(&ts_volts_sample_buffer,
			     precharge_config->hv_plate->ts_volts);
	update_sample_buffer(&batt_volts_sample_buffer,
			     precharge_config->hv_plate->batt_volts);

	float ts_volts_avg = get_average_sample(&ts_volts_sample_buffer);
	float batt_volts_avg = get_average_sample(&batt_volts_sample_buffer);

	hv_plate_t *hv_plate = precharge_config->hv_plate;
	bool should_precharge = 
		ts_volts_avg >=
		batt_volts_avg * precharge_config->transition_ratio;

	if (hv_plate->batt_volts < BATT_VOLTS_PRECHARGE_THRESHOLD) {
		should_precharge = false; 
	}

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
	nertimer_t update_loop_timer = { 0 };
	static const uint16_t TELEMETRY_LOOP_TIMEOUT = 2000;

	prechargeconfig_t precharge_config;
	precharge_init(&precharge_config, hv_plate, 0.95f,
		       200 /* ms debounce time */);

	init_sample_buffer(&ts_volts_sample_buffer, NUM_SAMPLES_FOR_AVG);
	init_sample_buffer(&batt_volts_sample_buffer, NUM_SAMPLES_FOR_AVG);

	start_timer(&update_loop_timer, TELEMETRY_LOOP_TIMEOUT);

	for (;;) {
		handle_precharge(&precharge_config);

		if (is_timer_expired(&update_loop_timer) && !is_timer_active(&update_loop_timer)) {
			send_precharge_status(
				precharge_config.air_switch_closed);
			start_timer(&update_loop_timer, TELEMETRY_LOOP_TIMEOUT);
		}

		tx_thread_sleep(50);
	}
}