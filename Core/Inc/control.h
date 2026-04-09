#ifndef _CONTROL_H
#define _CONTROL_H

#include "app_threadx.h"
#include "fdcan.h"
#include "stm32h5xx_hal.h"
#include <stdint.h>

#define CONTROL_MIN_DUTY_FAN 20

typedef struct {
  TIM_HandleTypeDef *tim_handle;
  int channel_identifier;
} pwm_device_t;

typedef enum { DEVICE_FAN0, DEVICE_FAN1, NUM_DEVICES } control_devices;

extern uint8_t control_device_signals[];

/**
 * @brief Initialize i/o for control
 *
 * @return Returns false if failed to init one or more peripherals
 */
bool control_init_peripherals(void);

/**
 * @brief Do control based on max temp from analyzer
 */
void control_fan(float pack_high_temp);

/**
 * @brief Send calypso message for controlling fans
 */
void control_message_fans(can_msg_t msg);

/**
 * @breif send calypso message for controling the lv box cooling fans
 * @param msg expected to be a uint_8 representing lv box temp in degrees c
 */
void control_message_fans_lv(can_msg_t msg);

void vControl(ULONG thread_input);

void control_message_balancing_pwm(can_msg_t msg);
#endif
