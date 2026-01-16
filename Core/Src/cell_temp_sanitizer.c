#include "cell_temp_sanitizer.h"
#include "analyzer.h"
#include "debounce.h"
#include <float.h>
#include <stdlib.h>

void temp_sanitizer_init(sanitizer_t *sanitizer)
{
	sanitizer->max_sanitized_temp.val = FLT_MIN;
	sanitizer->max_sanitized_temp.cellNum = 0;
	sanitizer->max_sanitized_temp.chipIndex = 0;

	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		for (int cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			sanitizer->sanitized_therms[chip][cell].last_temp = 0;
			sanitizer->sanitized_therms[chip][cell].valid = true;
		}
	}
}

void temp_sanitizer_run(sanitizer_t *sanitizer, analyzer_t *analyzer)
{
	static bool first_reading = true;
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		for (int cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			therm_state_t *therm_state =
				&sanitizer->sanitized_therms[chip][cell];

			float cell_temp =
				get_chip_data(analyzer, chip)->cell_temp[cell];
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

			// update the max sanitized temp on a valid max
			if (therm_state->valid) {
				if (cell_temp >
				    sanitizer->max_sanitized_temp.val) {
					sanitizer->max_sanitized_temp.val =
						cell_temp;
					sanitizer->max_sanitized_temp.chipIndex =
						chip;
					sanitizer->max_sanitized_temp.cellNum =
						cell;
				}
				// if max is no longer valid, max is reset
			} else if (sanitizer->max_sanitized_temp.cellNum ==
					   cell &&
				   sanitizer->max_sanitized_temp.chipIndex ==
					   chip) {
				sanitizer->max_sanitized_temp.val = FLT_MIN;
				sanitizer->max_sanitized_temp.cellNum = 0;
				sanitizer->max_sanitized_temp.chipIndex = 0;
			}
		}
	}
	first_reading = false;
}