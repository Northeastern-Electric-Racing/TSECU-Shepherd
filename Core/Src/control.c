#include "control.h"

#include "datastructs.h"
#include "main.h"

void control_init(void)
{
	// Init PWM for fan
	HAL_TIM_PWM_Init(&htim3);
}

void write_fan_duty_cycle(pwm_duty_t pwm)
{
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm);
}

void handle_segement_average_temps(float *segment_average_temps)
{
	float avg_temp = 0;
	for (int i = 0; i < NUM_SEGMENTS; i++) {
		avg_temp += segment_average_temps[i];
	}
	avg_temp /= (float)NUM_SEGMENTS;

	pwm_duty_t pwm;
	if (avg_temp <= CONTROL_MAX_TEMP_NO_FAN) {
		pwm = PWM_DUTY_0;
	} else if (avg_temp <= CONTROL_MAX_TEMP_20_FAN) {
		pwm = PWM_DUTY_20;
	} else if (avg_temp <= CONTROL_MAX_TEMP_40_FAN) {
		pwm = PWM_DUTY_40;
	} else if (avg_temp <= CONTROL_MAX_TEMP_60_FAN) {
		pwm = PWM_DUTY_60;
	} else if (avg_temp <= CONTROL_MAX_TEMP_80_FAN) {
		pwm = PWM_DUTY_80;
	} else {
		pwm = PWM_DUTY_100;
	}
	write_fan_duty_cycle(pwm);
}
