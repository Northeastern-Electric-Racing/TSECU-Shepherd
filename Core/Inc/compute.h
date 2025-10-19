#ifndef _COMPUTE_H
#define _COMPUTE_H

#include <stdint.h>
#include <stdbool.h>

#include "stm32h5xx.h"

#define CURRENT_SENSOR_PIN_L A1
#define CURRENT_SENSOR_PIN_H A0
#define MEAS_5VREF_PIN	     A7
#define FAULT_PIN	     2
#define CHARGE_SAFETY_RELAY  4
#define CHARGE_DETECT	     5
#define CHARGER_BAUD	     250000U
#define MC_BAUD		     1000000U
#define MAX_ADC_RESOLUTION   4095 // 12 bit ADC

/**
 * @brief updates fault relay
 *
 * @param fault_state
 */
void compute_set_fault(bool fault_state);

/**
 * @brief Checks if the shutdown circuit is open.
 */
bool read_shutdown();

#endif // COMPUTE_H
