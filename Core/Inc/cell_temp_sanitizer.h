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
			chipdata_t chip_data[num_chips][num_cells],
			therm_state_t sanitized_out[num_chips][num_cells]);

#endif // CEL_TEMP_SANITIZER_H