#include <stdbool.h>
#include "timer.h"
#include "datastructs.h"

#ifndef CELL_TEMP_SANITIZER_H
#define CELL_TEMP_SANITIZER_H

#define TOO_DIFF_THRESHOLD 0.20

/**
 * @brief Creates a grid of therm_t.
 * @param sanitizer Struct containing sanitized therm data
 */
void temp_sanitizer_init(sanitizer_t *sanitizer);

/**
 * @brief Given a grid of cell temperatures, updates the given grid of sanitized cell temperatures.
 * Should be run periodically.
 * @param sanitizer Struct containing sanitized therm data
 * @param analyzer Struct containing analyzer data
 */
void temp_sanitizer_run(sanitizer_t *sanitizer, analyzer_t *analyzer);

#endif // CEL_TEMP_SANITIZER_H