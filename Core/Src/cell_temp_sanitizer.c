#include "cell_temp_sanitizer.h"
#include "analyzer.h"
#include "u_tx_flags.h"
#include <stdbool.h>
#include <stdint.h>
#include <float.h>
#include <math.h>

/**
 * @brief Number of consecutive invalid samples before a thermistor is marked invalid.
 */
#define THERM_FAULT_COUNT_THRESHOLD (2U)

/**
 * @brief Maximum allowed temperature change from the last trusted thermistor sample.
 */
#define TEMP_MAX_DELTA_C (5.0f)

/**
 * @brief Resets the sanitized minimum and maximum temperature tracking.
 *
 * @param sanitizer Pointer to the sanitizer data structure.
 */
static void reset_sanitized_min_max(sanitizer_t *const sanitizer)
{
	sanitizer->max_sanitized_temp.val = -FLT_MAX;
	sanitizer->max_sanitized_temp.chipIndex = 0U;
	sanitizer->max_sanitized_temp.cellNum = 0U;

	sanitizer->min_sanitized_temp.val = FLT_MAX;
	sanitizer->min_sanitized_temp.chipIndex = 0U;
	sanitizer->min_sanitized_temp.cellNum = 0U;
}

/**
 * @brief Records a thermistor fault and marks it invalid if the threshold is reached.
 *
 * @param therm_state Pointer to the thermistor state.
 */
static void record_therm_fault(therm_state_t *const therm_state)
{
	therm_state->fault_count++;

	// Repeated faults mark thermistor invalid
	if (therm_state->fault_count >= THERM_FAULT_COUNT_THRESHOLD)
	{
		therm_state->valid = false;
	}
}

/**
 * @brief Updates the sanitized maximum temperature if the provided value is higher.
 *
 * @param sanitizer Pointer to the sanitizer instance.
 * @param chip Chip index.
 * @param cell Cell index.
 * @param cell_temp Temperature reading.
 */
static void update_sanitized_max_temp(sanitizer_t *const sanitizer, const uint8_t chip, const uint8_t cell, const float cell_temp)
{
	if (cell_temp > sanitizer->max_sanitized_temp.val)
	{
		sanitizer->max_sanitized_temp.val = cell_temp;
		sanitizer->max_sanitized_temp.chipIndex = chip;
		sanitizer->max_sanitized_temp.cellNum = cell;
	}
}

/**
 * @brief Updates the sanitized minimum temperature if the provided value is lower.
 *
 * @param sanitizer Pointer to the sanitizer instance.
 * @param chip Chip index.
 * @param cell Cell index.
 * @param cell_temp Temperature reading.
 */
static void update_sanitized_min_temp(sanitizer_t *const sanitizer, const uint8_t chip, const uint8_t cell, const float cell_temp)
{
	if (cell_temp < sanitizer->min_sanitized_temp.val)
	{
		sanitizer->min_sanitized_temp.val = cell_temp;
		sanitizer->min_sanitized_temp.chipIndex = chip;
		sanitizer->min_sanitized_temp.cellNum = cell;
	}
}

void temp_sanitizer_init(sanitizer_t *const sanitizer)
{
	reset_sanitized_min_max(sanitizer);

	for (uint8_t chip = 0U; chip < NUM_CHIPS; chip++) {
		for (uint8_t cell = 0U; cell < NUM_CELLS_PER_CHIP; cell++) {
			sanitizer->sanitized_therms[chip][cell].last_temp = 0.0f;
			sanitizer->sanitized_therms[chip][cell].valid = false;
			sanitizer->sanitized_therms[chip][cell].initialized = false;
			sanitizer->sanitized_therms[chip][cell].fault_count = 0U;
		}
	}
}

void temp_sanitizer_run(sanitizer_t *const sanitizer, const analyzer_t *const analyzer)
{
	reset_sanitized_min_max(sanitizer);

	for (uint8_t chip = 0U; chip < NUM_CHIPS; chip++)
	{
		for (uint8_t cell = 0U; cell < NUM_CELLS_PER_CHIP; cell++)
		{
			therm_state_t *const therm_state = &sanitizer->sanitized_therms[chip][cell];
			const float cell_temp = analyzer->chip_data[chip].cell_temp[cell];
			bool sample_valid = false;
			const bool temp_in_range = ((cell_temp >= (float)MIN_TEMP) && (cell_temp <= (float)MAX_CELL_TEMP));

			// Only initialize thermistors that have not faulted out during startup
			if ((therm_state->initialized == false) && (therm_state->fault_count < THERM_FAULT_COUNT_THRESHOLD))
			{
				// First trusted sample initializes thermistor
				if (temp_in_range)
				{
					therm_state->last_temp = cell_temp;
					therm_state->initialized = true;
					therm_state->valid = true;
					therm_state->fault_count = 0U;
					sample_valid = true;
				}
				else
				{
					// Startup sample outside physical range
					record_therm_fault(therm_state);
					sample_valid = false;
				}
			}
			else if (therm_state->valid == false)
			{
				// Invalid thermistors do not recover automatically
				sample_valid = false;
			}
			else
			{
				const float temp_delta = fabsf(cell_temp - therm_state->last_temp);

				if (temp_in_range == false)
				{
					// Reject physically invalid sample
					record_therm_fault(therm_state);
					sample_valid = false;
				}
				else if (temp_delta > TEMP_MAX_DELTA_C)
				{
					// Reject sudden jump from last trusted value
					record_therm_fault(therm_state);
					sample_valid = false;
				}
				else
				{
					// Sample is trusted
					therm_state->last_temp = cell_temp;
					therm_state->fault_count = 0U;
					sample_valid = true;
				}
			}

			if (sample_valid == true)
			{
				update_sanitized_max_temp(sanitizer, chip, cell, cell_temp);
				update_sanitized_min_temp(sanitizer, chip, cell, cell_temp);
			}
		}
	}
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
		set_flag(SANITIZER_FLAG);
		temp_sanitizer_run(sanitizer, analyzer);
	}
}
