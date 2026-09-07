#ifndef _CHARGING_H
#define _CHARGING_H

#include "datastructs.h"
#include "adBms6830Data.h"


/**
 * @brief set the duty cycle (atomic)
 *
 * @param duty_cycle_req_get the duty cycle 0-100, will be rounded UP appropriately
 */
void pwm_duty_cycle_set(uint8_t duty_cycle_req_get);

/**
 * @brief get the selected ADBMS duty cycle
 *
 * @returns the selected duty cycle percentage
 */
float pwm_duty_cycle_setting_get(void);

/**
 * @brief get the duty cycle (atomic)
 *
 * @returns the duty cycle enum for the driver (PWM_DUTY)
 */
PWM_DUTY pwm_duty_cycle_get();

/**
 * @brief entrypoint for handling balancing of cells.  DOES NOT ENABLE BALANCING, but does configure it.
 *
 * @param analyzer general Analyzer struct for processed cell data
 * @param acc_data segment data
 * @return true if at least one cell needs balancing, otherwise false
 */
bool handle_balance_cells(analyzer_t *analyzer, acc_data_t *acc_data);

#endif
