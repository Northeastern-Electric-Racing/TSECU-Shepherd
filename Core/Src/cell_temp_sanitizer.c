#include "cell_temp_sanitizer.h"
#include "debounce.h"
#include <float.h>
#include <stdlib.h>

void temp_sanitizer_init(
	therm_state_t sanitized_out[NUM_CHIPS][NUM_CELLS_PER_CHIP])
{
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		for (int cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			sanitized_out[chip][cell].last_temp = 0;
			sanitized_out[chip][cell].valid = true;
		}
	}
}

void temp_sanitizer_run(
	chipdata_t chip_data[NUM_CHIPS],
	therm_state_t sanitized_out[NUM_CHIPS][NUM_CELLS_PER_CHIP])
{
	static bool first_reading = true;
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		for (int cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			therm_state_t *therm_state = &sanitized_out[chip][cell];

			float cell_temp = chip_data[chip].cell_temp[cell];
			if (!first_reading &&
			    cell_temp >
				    therm_state->last_temp *
					    (1 + (float)TOO_DIFF_THRESHOLD)) {
				therm_state->valid = false;
			} else if (!first_reading &&
				   cell_temp <
					   therm_state->last_temp *
						   (1 -
						    (float)TOO_DIFF_THRESHOLD)) {
				therm_state->valid = false;
			}
			therm_state->last_temp = cell_temp;
		}
	}
	first_reading = false;
}