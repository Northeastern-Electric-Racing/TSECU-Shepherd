#include "control.h"

#include "main.h"

#define PWM_INIT_MAX_ATTEMPTS 10
#define INIT_TIMEOUT_MS	      10

#define _PERCENT_16(x) ((uint16_t)(x * ((1 << 16) - 1) / 100.0f))

float calypso_signals[NUM_DEVICES];
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
		.channel_identifier = TIM_CHANNEL_3,
	};

	bool error = false;
	error |= _init_pwm_device(&device_fan0);
	return error;
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
	_write_pwm_device(&device_fan0, duty);
}

void control_message_fans(can_msg_t msg)
{
	calypso_signals[DEVICE_FAN0] = *((float *)msg.data);
}
#undef _PERCENT_16
