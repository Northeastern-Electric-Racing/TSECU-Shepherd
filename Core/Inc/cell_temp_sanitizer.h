#include <stdbool.h>
#include "timer.h"

#ifndef CELL_TEMP_SANITIZER_H
#define CELL_TEMP_SANITIZER_H

#define TEMP_LOWER_BOUND   20.0f
#define TEMP_UPPER_BOUND   80.0f
#define DEBOUNCE_PERIOD_MS 5000

/**
 * @brief This enumeration represents the health state of a battery cell for its temperature
 * readings.
 */
typedef enum { HEALTHY, UNHEALTHY, INVALID } health_state;

/**
 * @brief A therm_state_t is a struct of a (float, bool).
 * - Last recorded temperature of a cell.
 * - Whether the cell temperature can be used (i.e. whether the measurement is bad).
 */
typedef struct {
	float last_valid_temp;
	health_state state;
	nertimer_t debounce_timer;
} therm_state_t;

/**
 * @brief Marks the given therm_state_t as invalid.
 */
void mark_invalid(void *arg);

/**
 * @brief Given a grid of temperature values for cells, creates a grid of therm_t.
 * @param temps_in are the cell temperatures.
 * @param num_chips is the number of chips in the cell temperature array.
 * @param num_cells is the number of cells in the cell temperature array.
 * @param sanitized_out is the output array of therm_t, which gets initialized by this function.
 */
void temp_sanitizer_init(int num_chips, int num_cells,
			 therm_state_t sanitized_out[num_chips][num_cells]);

/**
 * @brief Given a grid of cell temperatures, updates the given grid of sanitized cell temperatures.
 * Should be run periodically.
 * @param temps_in are the cell temperatures.
 * @param num_chips is the number of chips in the cell temperature array.
 * @param num_cells is the number of cells in the cell temperature array.
 * @param sanitized_out is the output array of therm_t, which gets initialized by this function.
 */
void temp_sanitizer_run(int num_chips, int num_cells,
			float temps_in[num_chips][num_cells],
			therm_state_t sanitized_out[num_chips][num_cells]);

#endif // CEL_TEMP_SANITIZER_H