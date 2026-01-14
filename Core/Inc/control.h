#ifndef _CONTROL_H
#define _CONTROL_H

#include "fdcan.h"
#include "stm32h5xx_hal.h"
#include <stdint.h>

#define CONTROL_MIN_DUTY_FAN 20

typedef struct {
  TIM_HandleTypeDef *tim_handle;
  int channel_identifier;
} pwm_device_t;

typedef enum { DEVICE_FAN0, NUM_DEVICES } control_devices;

/**
 * @brief Initialize i/o for control
 *
 * @return Returns false if failed to init one or more peripherals
 */
bool control_init_peripherals(void);

/**
 * @brief Do control based on max temp from analyzer
 */
void handle_max_temp(float max_temp);

/**
 * @brief Send calypso message for controlling fans
 */
void control_message_fans(can_msg_t msg);

#endif
