#ifndef _CHARGING_H
#define _CHARGING_H

#include "datastructs.h"

/**
 * @brief set the duty cycle (atomic)
 *
 * @param duty_cycle_req_get the duty cycle 0-100, will be rounded UP appropriately
 */
void pwm_duty_cycle_set(uint8_t duty_cycle_req_get);

/**
 * @brief entrypoint for handling balancing of cells.  DOES NOT ENABLE BALANCING, but does configure it.
 *
 * @param analyzer general Analyzer struct for processed cell data
 * @param acc_data segment data
 */
void handle_balance_cells(analyzer_t *analyzer, acc_data_t *acc_data);

#endif
