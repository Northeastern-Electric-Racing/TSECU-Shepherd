/**
 * @file tracex.h
 * @brief TraceX control interface.
 */

#include "tx_api.h"
#include <stdint.h>

#define TRACEX_BUFFER_SIZE (60u * 1024u)

/** Get pointer to the TraceX buffer. */
UCHAR *tracex_get_buffer(void);

/** Start TraceX recording. */
void tracex_start(void);

/** Stop TraceX recording. */
void tracex_stop(void);