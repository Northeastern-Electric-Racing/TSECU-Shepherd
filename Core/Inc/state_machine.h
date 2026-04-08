
#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

#include "analyzer.h"
#include "datastructs.h"

/**
 * @brief Called when we receive a message from the charger
 * 
 * @param state_machine_args pointer to state_machine_args data
 * 
 */
void charger_message_recieved(state_machine_args_t *state_machine_args);

/**
 * @brief Returns if we want to balance cells during a particular frame
 *
 * @param state_machine_args pointer to state_machine_args data
 * @return true is can balance, false otherwise
 */
bool sm_balancing_check(state_machine_args_t *state_machine_args);

/**
 * @brief Returns if we want to charge cells during a particular frame
 *
 * @param state_machine_args pointer to state_machine_args data
 * @return true is can charge, false otherwise
 */
bool sm_charging_check(state_machine_args_t *state_machine_args);

/**
 * @brief Returns any new faults or current faults that have come up
 * @note Should be bitwise OR'ed with the current fault status
 *
 * @param state_machine_args pointer to state_machine_args data
 */
void sm_fault_return(state_machine_args_t *state_machine_args);

/**
 * @brief Used in parellel to faultReturn(), calculates each fault to append the
 * fault status
 *
 * @param fault_item fault data
 * @param fault_code fault code
 * @return true if fault is present, false otherwise
 */
bool sm_fault_eval(fault_eval_t *fault_item, fault_code_t fault_code);

/**
 * @brief handles the state machine, calls the appropriate handler function and
 * runs every loop functions
 *
 * @param state_machine_args pointer to state_machine_args data
 */
void sm_handle_state(state_machine_args_t *state_machine_args);

/**
 * @brief Sets the segment communication fault.
 * 
 * @param state_mach Pointer to the state machine data structure.
 */
void set_segment_comms_fault(state_machine_t *state_mach);

/**
 * @brief Clears the segment communication fault.
 * 
 * @param state_mach Pointer to the state machine data structure.
 */
void clear_segment_comms_fault(state_machine_t *state_mach);

/**
 * @brief Determines if there is a critical fault that is active.
 * 
 * @return true if critical faults are active, false otherwise
 */
bool are_critical_faults_active();

/**
 * @brief Gets the status of a given fault code.
 * 
 * @param fault The fault code to check
 * @return true if the fault is active, false otherwise
 */
bool get_fault(fault_code_t fault);

// init functions
void init_boot(state_machine_args_t *state_machine_args);
void init_ready(state_machine_args_t *state_machine_args);
void init_charging(state_machine_args_t *state_machine_args);
void init_faulted(state_machine_args_t *state_machine_args);

// handle functions
void handle_boot(state_machine_args_t *state_machine_args);
void handle_ready(state_machine_args_t *state_machine_args);
void handle_charging(state_machine_args_t *state_machine_args);
void handle_faulted(state_machine_args_t *state_machine_args);

/**
 * @brief State machine thread function for BMS state management.
 * 
 * @param thread_input Pointer to state_machine_args_t structure
 */
void vStateMachine(ULONG thread_input);

#endif