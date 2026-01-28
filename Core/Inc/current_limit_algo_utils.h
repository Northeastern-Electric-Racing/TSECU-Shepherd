#ifndef CURRENT_LIMIT_ALGO_UTILS_H
#define CURRENT_LIMIT_ALGO_UTILS_H

#include <stdbool.h>
#include "datastructs.h"

/**
 * @brief Determine whether pulse operation shall be disabled.
 *
 * This function evaluates system state and fault conditions to decide
 * whether pulse operation must be inhibited. Pulse operation is disabled
 * when the system is in the charging state, a critical fault is active,
 * or a segment communication fault is present.
 *
 * @param state_machine  Pointer to the system state machine struct.
 *
 * @return true if pulse operation shall be disabled, false otherwise.
 */
bool disable_pulse(state_machine_t *const state_machine);

#endif // CURRENT_LIMIT_ALGO_UTILS_H