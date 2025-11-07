#include <stdbool.h>
#include "timer.h"
#include "datastructs.h"

#ifndef CELL_TEMP_SANITIZER_H
#define CELL_TEMP_SANITIZER_H

#define TOO_DIFF_THRESHOLD 0.20

/**
 * @brief A therm_state_t is a struct of a (float, bool).
 * - Last recorded temperature of a cell.
 * - Whether the cell temperature can be used (i.e. whether the measurement is bad).
 */
typedef struct {
	float last_temp;
	bool valid;
} therm_state_t;

/**
 * @brief Creates a grid of therm_t.
 * @param sanitized_out is the output array of therm_t, which gets initialized by this function.
 */
void temp_sanitizer_init(
	therm_state_t sanitized_out[NUM_CHIPS][NUM_CELLS_PER_CHIP]);

/**
 * @brief Given a grid of cell temperatures, updates the given grid of sanitized cell temperatures.
 * Should be run periodically.
 * @param chip_data adbms6830 chip data (stores therm temps)
 * @param sanitized_out is the output array of therm_t, which gets initialized by this function.
 */
void temp_sanitizer_run(
	chipdata_t chip_data[NUM_CHIPS],
	therm_state_t sanitized_out[NUM_CHIPS][NUM_CELLS_PER_CHIP]);

#endif // CEL_TEMP_SANITIZER_H