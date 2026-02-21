
#include "control.h"
#include "main.h"
#include "datastructs.h"
#include "can_messages.h"

#define INIT_TIMEOUT_MS 10

#define _PERCENT_16(x) ((uint16_t)(x * (((1 << 16) - 1) / 100)))

uint8_t calypso_signals[NUM_DEVICES];
uint8_t control_device_signals[NUM_DEVICES];
pwm_device_t device_fan0;

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

	bool status = _init_pwm_device(&device_fan0);
	return !status;
}

void control_fan(float pack_high_temp)
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

	uint16_t duty_from_calypso = _PERCENT_16(calypso_signals[DEVICE_FAN0]);
	if (duty_from_calypso > duty) {
		duty = duty_from_calypso;
	}

	control_device_signals[DEVICE_FAN0] = (uint8_t)(duty >> 8);
	_write_pwm_device(&device_fan0, duty);
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

	for (;;) {
		mutex_get(&analyzer->analyzer_mutex);
		float pack_high_temp = analyzer->max_temp.val;
		control_fan(pack_high_temp);
		mutex_put(&analyzer->analyzer_mutex);

		send_control_signals(control_device_signals);

		tx_thread_sleep(MS_TO_TICKS(100));
	}
}
