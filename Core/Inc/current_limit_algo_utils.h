#ifndef CURRENT_LIMIT_ALGO_UTILS_H
#define CURRENT_LIMIT_ALGO_UTILS_H

#include <stdbool.h>
#include "datastructs.h"

/**
 * @brief Performs linear interpolation between two points.
 *
 * Computes the interpolated y-value corresponding to x, given two
 * reference points (x1, y1) and (x2, y2).
 *
 * @param x   Input value at which to interpolate.
 * @param x1  First reference x-coordinate.
 * @param x2  Second reference x-coordinate.
 * @param y1  y-value at x1.
 * @param y2  y-value at x2.
 *
 * @return Interpolated y-value.
 */
float linear_interpolate(float x, float x1, float x2, float y1, float y2);

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