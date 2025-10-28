/**
 * @file tracex.h
 * @brief TraceX control interface.
 */

#include "tx_api.h"
#include <stdint.h>

#define ENABLE_TRACEX 0

#define TRACEX_BUFFER_SIZE (64U * 1024U)

/** Get pointer to the TraceX buffer. */
UCHAR *tracex_get_buffer(void);

/** Start TraceX recording. */
void tracex_start(void);

/** Stop TraceX recording. */
void tracex_stop(void);