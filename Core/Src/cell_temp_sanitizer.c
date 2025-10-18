#include "cell_temp_sanitizer.h"
#include "debounce.h"
#include <float.h>
#include <stdlib.h>

void mark_invalid(void *arg)
{
	if (arg != NULL) {
		((therm_state_t *)arg)->state = UNHEALTHY;
	}
}

void temp_sanitizer_init(int num_chips, int num_cells,
			 therm_state_t sanitized_out[num_chips][num_cells])
{
	for (int chip = 0; chip < num_chips; chip++) {
		for (int cell = 0; cell < num_cells; cell++) {
			sanitized_out[chip][cell].last_valid_temp = 0;
			sanitized_out[chip][cell].state = HEALTHY;
			sanitized_out[chip][cell].debounce_timer.active = false;
			sanitized_out[chip][cell].debounce_timer.completed =
				false;
		}
	}
}

void temp_sanitizer_run(int num_chips, int num_cells,
			float temps_in[num_chips][num_cells],
			therm_state_t sanitized_out[num_chips][num_cells])
{
	for (int chip = 0; chip < num_chips; chip++) {
		for (int cell = 0; cell < num_cells; cell++) {
			therm_state_t *therm_state = &sanitized_out;
			float cell_temp = temps_in[chip][cell];
			bool is_cell_temp_bad =
				isnan(cell_temp) ||
				(cell_temp > TEMP_UPPER_BOUND) ||
				(cell_temp < TEMP_LOWER_BOUND);
			// Start debounce timer if bad, end debounce timer if good,
			// mark invalid if debounce timer over and still bad.
			debounce(is_cell_temp_bad, &therm_state->debounce_timer,
				 DEBOUNCE_PERIOD_MS, mark_invalid, therm_state);
			// If the cell temperature is good, update cell temp and cell validity.
			if (!is_cell_temp_bad) {
				therm_state->last_valid_temp = cell_temp;
				therm_state->state = HEALTHY;
			}
		}
	}
}