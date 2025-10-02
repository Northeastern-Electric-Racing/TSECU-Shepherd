
#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

#include "analyzer.h"

#define NUM_FAULTS 8

typedef enum {
	FAULT_STAT_FAULTED = 1,
	FAULT_STAT_CLEARED = 2,
} fault_stat_t;

/**
 * @brief Called when we receive a message from the charger
 * 
 * @param bmsdata
 * 
 */
void charger_message_recieved(bms_t *bmsdata);

/**
 * @brief Returns if we want to balance cells during a particular frame
 *
 * @param bmsdata
 * @return true
 * @return false
 */
bool sm_balancing_check(bms_t *bmsdata);

/**
 * @brief Returns if we want to charge cells during a particular frame
 *
 * @param bmsdata
 * @return true
 * @return false
 */
bool sm_charging_check(bms_t *bmsdata);

/**
 * @brief Returns any new faults or current faults that have come up
 * @note Should be bitwise OR'ed with the current fault status
 *
 * @param accData
 */
void sm_fault_return(bms_t *accData);

/**
 * @brief Used in parellel to faultReturn(), calculates each fault to append the
 * fault status
 *
 * @param fault_item
 * @return fault_status
 */
fault_stat_t sm_fault_eval(fault_eval_t *fault_item);

/**
 * @brief handles the state machine, calls the appropriate handler function and
 * runs every loop functions
 *
 * @param bmsdata
 */
void sm_handle_state(bms_t *bmsdata);

/**
 * @brief Algorithm behind determining which cells we want to balance
 * @note Directly interfaces with the segments
 *
 * @param bms_data
 */
void sm_balance_cells(bms_t *bms_data);

#endif