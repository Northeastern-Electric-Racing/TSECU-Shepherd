#ifndef _CONTROL_H
#define _CONTROL_H

#include <stdint.h>

#define _PERCENT_16(x) ((uint16_t)(x * ((1 << 16) - 1) / 100.0f))
typedef enum {
	PWM_DUTY_0,
	PWM_DUTY_10 = _PERCENT_16(10),
	PWM_DUTY_20 = _PERCENT_16(20),
	PWM_DUTY_30 = _PERCENT_16(30),
	PWM_DUTY_40 = _PERCENT_16(40),
	PWM_DUTY_50 = _PERCENT_16(50),
	PWM_DUTY_60 = _PERCENT_16(60),
	PWM_DUTY_70 = _PERCENT_16(70),
	PWM_DUTY_80 = _PERCENT_16(80),
	PWM_DUTY_90 = _PERCENT_16(90),
	PWM_DUTY_100 = _PERCENT_16(100),
} pwm_duty_t;
#undef _PERCENT_16

#define CONTROL_MAX_TEMP_NO_FAN 20.0f
#define CONTROL_MAX_TEMP_20_FAN 30.0f
#define CONTROL_MAX_TEMP_40_FAN 40.0f
#define CONTROL_MAX_TEMP_60_FAN 50.0f
#define CONTROL_MAX_TEMP_80_FAN 60.0f

/**
 * @brief Initialize i/o for control
 *
 */
void control_init(void);

/**
 * @brief Write pwm signal to fan
 *
 */
void write_fan_duty_cycle(pwm_duty_t pwm_duty);

/**
 * @brief Control fan based on segement average temps
 *
 */
void handle_segement_average_temps(float *segment_average_temps);

#endif
