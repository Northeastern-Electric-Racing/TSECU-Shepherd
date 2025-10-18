#include "tx_api.h"
#include <stdint.h>

/**
 * @file tracex.h
 * @brief TraceX control interface.
 */

#ifndef TRACEX_BUFFER_SIZE
#define TRACEX_BUFFER_SIZE (60u * 1024u)
#endif

/** Start TraceX recording. */
void tracex_start(void);

/** Stop TraceX recording. */
void tracex_stop(void);

/**
 * @brief Writer callback for trace dumping.
 * @param data Pointer to data block.
 * @param len  Data length in bytes.
 * @return 0 on success, non-zero on error.
 */
typedef int (*tracex_writer_t)(const uint8_t *data, uint32_t len);

/**
 * @brief Transfer trace data using a writer function.
 * @return 0 on success, 1 on failure.
 */
int tracex_transfer_data(tracex_writer_t write_fn);

/** Get pointer to the TraceX buffer. */
UCHAR *tracex_get_buffer(void);
