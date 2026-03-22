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

	sanitizer->min_sanitized_temp.val = FLT_MAX;   
	sanitizer->min_sanitized_temp.cellNum = 0;
	sanitizer->min_sanitized_temp.chipIndex = 0;

	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		for (int cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			sanitizer->sanitized_therms[chip][cell].last_temp = 0;
			sanitizer->sanitized_therms[chip][cell].valid = true;
		}
	}
}

static void sanitized_max_temp(sanitizer_t *sanitizer, analyzer_t *analyzer, int chip, int cell, float cell_temp, therm_state_t *therm_state)
{
	
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
	
static void sanitized_min_temp(sanitizer_t *sanitizer, analyzer_t *analyzer, int chip, int cell, float cell_temp, therm_state_t *therm_state)
{

	// update the min sanitized temp on a valid min
	if (therm_state->valid) {
		if (cell_temp <   // to get the lowest value
			sanitizer->min_sanitized_temp.val) {
			sanitizer->min_sanitized_temp.val =
				cell_temp;
			sanitizer->min_sanitized_temp.chipIndex =
				chip;
			sanitizer->min_sanitized_temp.cellNum =
				cell;
		}
		// if min is no longer valid, min is reset
	} else if (sanitizer->min_sanitized_temp.cellNum ==
			cell &&
		sanitizer->min_sanitized_temp.chipIndex ==
			chip) {
		sanitizer->min_sanitized_temp.val = FLT_MAX;
		sanitizer->min_sanitized_temp.cellNum = 0;
		sanitizer->min_sanitized_temp.chipIndex = 0;
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

			sanitized_max_temp(sanitizer, analyzer, chip, cell, cell_temp, therm_state);
			sanitized_min_temp(sanitizer, analyzer, chip, cell, cell_temp, therm_state);
		}
	}
	first_reading = false;
}


// SANITIZER THREAD
void vSanitizer(ULONG thread_input)
{
	PRINTLN_INFO("Starting Sanitizer thread...");
	sanitizer_args_t *sanitizer_args = (sanitizer_args_t *)thread_input;

	sanitizer_t *sanitizer = sanitizer_args->sanitizer;
	analyzer_t *analyzer = sanitizer_args->analyzer;

	temp_sanitizer_init(sanitizer);

	for (;;) {
		temp_sanitizer_run(sanitizer, analyzer);
		thread_sleep_ms(500);
	}
}


	
	






