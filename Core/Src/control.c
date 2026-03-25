
#include "control.h"
#include "can_messages_tx.h"
#include "datastructs.h"
#include "main.h"
#include "shep_mutexes.h"
#include "u_tx_debug.h"

#define INIT_TIMEOUT_MS 10

#define _PERCENT_16(x) ((uint16_t)(x * (((1 << 16) - 1) / 100)))

uint8_t calypso_signals[NUM_DEVICES] = { 0 };
pwm_device_t device_fan0;
pwm_device_t device_fan1;

static HAL_StatusTypeDef _init_pwm_device(pwm_device_t *device)
{
	HAL_StatusTypeDef status;

	status = HAL_TIM_PWM_Init(device->tim_handle);
	if (status != HAL_OK) {
		return status;
	}
	status = HAL_TIM_PWM_Start(device->tim_handle,
				   device->channel_identifier);
	return status;
}

static void _write_pwm_device(pwm_device_t *device, uint16_t duty)
{
	__HAL_TIM_SET_COMPARE(device->tim_handle, device->channel_identifier,
			      duty);
}

bool control_init_peripherals(void)
{
	device_fan0 = (pwm_device_t){
		.tim_handle = &htim3,
		.channel_identifier = TIM_CHANNEL_2,
	};

	device_fan1 = (pwm_device_t){
		.tim_handle = &htim3,
		.channel_identifier = TIM_CHANNEL_1,
	};

	bool status = _init_pwm_device(&device_fan0);
	if (!status) {
		return false;
	}
	status = _init_pwm_device(&device_fan1);
	if (!status) {
		return false;
	}
	return true;
}

void update_batt_box_fan(float pack_high_temp)
{
	uint16_t duty;
	if (pack_high_temp <= 30.0f) {
		duty = _PERCENT_16(CONTROL_MIN_DUTY_FAN);
	} else if (pack_high_temp <= 50.0f) {
		duty = _PERCENT_16(50);
	} else if (pack_high_temp <= 60.0f) {
		duty = _PERCENT_16(75);
	} else {
		duty = _PERCENT_16(100);
	}

	device_fan0.requested_duty = duty;
}

void write_fan_duty_cycle_percentage()
{
	uint16_t duty_from_calypso = _PERCENT_16(calypso_signals[DEVICE_FAN0]);
	if (duty_from_calypso > device_fan0.requested_duty) {
		device_fan0.current_duty = duty_from_calypso;
	} else {
		device_fan0.current_duty = device_fan0.requested_duty;
	}

	_write_pwm_device(&device_fan0, device_fan0.current_duty);

	uint16_t duty1_from_calypso = _PERCENT_16(calypso_signals[DEVICE_FAN1]);
	if (duty1_from_calypso > device_fan1.requested_duty) {
		device_fan1.current_duty = duty1_from_calypso;
	} else {
		device_fan1.current_duty = device_fan1.requested_duty;
	}

	_write_pwm_device(&device_fan1, device_fan1.current_duty);
}

void control_message_fans(can_msg_t msg)
{
	// First byte is the requested duty cycle (0-100)
	uint8_t duty = *(msg.data);
	if (duty > 100) {
		duty = 100;
	}
	calypso_signals[DEVICE_FAN0] = duty;
}

void control_message_fans_lv(can_msg_t msg)
{
	uint8_t temp_c = *(msg.data);
	uint8_t duty_cycle = temp_c >= 35 ? 100 : 75;
	device_fan1.requested_duty = _PERCENT_16(duty_cycle);
}

#undef _PERCENT_16

// CONTROL THREAD
void vControl(ULONG thread_input)
{
	PRINTLN_INFO("Starting Control thread...");

	analyzer_t *analyzer = (analyzer_t *)thread_input;

	PRINTLN_INFO("Starting Control thread...");

	// Initialize peripherals for control
	bool failed = !control_init_peripherals();
	if (failed) {
		PRINTLN_ERROR(
			"Failed to initialize one or more peripherals.\n");
	}

	nertimer_t update_loop_timer = { 0 };
	static const uint16_t TELEMETRY_LOOP_TIMEOUT = 2000;

	start_timer(&update_loop_timer, TELEMETRY_LOOP_TIMEOUT);

	for (;;) {
		mutex_get(&analyzer_mutex);
		float pack_high_temp = analyzer->max_temp.val;
		update_batt_box_fan(pack_high_temp);
		mutex_put(&analyzer_mutex);

		write_fan_duty_cycle_percentage();

		if (is_timer_expired(&update_loop_timer) &&
		    !is_timer_active(&update_loop_timer)) {
			send_fan_duty_cycle_percentage(
				(uint8_t)(device_fan0.current_duty >> 8));
			send_fan_duty_cycle_percentage(
				(uint8_t)(device_fan0.current_duty >> 8));

			start_timer(&update_loop_timer, TELEMETRY_LOOP_TIMEOUT);
		}

		tx_thread_sleep(MS_TO_TICKS(100));
	}
}
