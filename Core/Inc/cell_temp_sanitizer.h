#include "datastructs.h"

#ifndef CELL_TEMP_SANITIZER_H
#define CELL_TEMP_SANITIZER_H

/**
 * @brief Initializes the temperature sanitizer state.
 *
 * @param sanitizer Pointer to the sanitizer data structure.
 */
void temp_sanitizer_init(sanitizer_t *sanitizer);

/**
 * @brief Processes and sanitizes cell temperature readings.
 *
 * Valid samples update the tracked minimum and maximum temperatures while
 * invalid samples are filtered and tracked using the fault counter.
 *
 * @param sanitizer Pointer to the sanitizer data structure.
 * @param analyzer Pointer to the analyzer containing raw temperature data.
 */
void temp_sanitizer_run(sanitizer_t *const sanitizer, const analyzer_t *const analyzer);

void vSanitizer(ULONG thread_input);

#endif // CEL_TEMP_SANITIZER_H