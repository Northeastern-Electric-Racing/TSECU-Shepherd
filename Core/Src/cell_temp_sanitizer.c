#include "cell_temp_sanitizer.h"
#include "debounce.h"
#include <float.h>
#include <stdlib.h>

void temp_sanitizer_init(int num_chips, int num_cells,
			 therm_state_t sanitized_out[num_chips][num_cells])
{
	for (int chip = 0; chip < num_chips; chip++) {
		for (int cell = 0; cell < num_cells; cell++) {
			sanitized_out[chip][cell].last_temp = 0;
			sanitized_out[chip][cell].valid = true;
		}
	}
}

void temp_sanitizer_run(int num_chips, int num_cells,
			chipdata_t chip_data[num_chips][num_cells],
			therm_state_t sanitized_out[num_chips][num_cells])
{
	static bool first_reading = true;
	for (int chip = 0; chip < num_chips; chip++) {
		for (int cell = 0; cell < num_cells; cell++) {
			therm_state_t *therm_state = &sanitized_out[num_chips][num_cells];
			
			float cell_temp = chip_data[chip]->cell_temp[cell];
			if (!first_reading && cell_temp > therm_state->last_temp * (1 + (float)TOO_DIFF_THRESHOLD)) {
				therm_state->valid = false;
			} 
			therm_state->last_temp = cell_temp;
		}
	}
	first_reading = false;
}