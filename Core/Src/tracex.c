#include "tracex.h"
#include "stm32f405xx.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* 32-byte alignment for cache line safety. */
__attribute__((aligned(32))) static UCHAR tx_trace_buffer[TRACEX_BUFFER_SIZE];

static bool tracex_started = false;
static const size_t TRACEX_CHUNK_BYTES = 1024u;

/**
 * @brief  Enables the DWT cycle counter for precise CPU cycle measurements.
 * @note   Requires the trace unit (DEMCR.TRCENA) to be enabled.
 */
static inline void DWT_EnableCycleCounter(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* Enable trace and debug block. */
    DWT->CYCCNT = 0;                                 /* Reset cycle counter. */
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;            /* Start cycle counting. */
}

UCHAR *tracex_get_buffer(void)
{
	return tx_trace_buffer;
}

void tracex_start(void)
{
	if (tracex_started) {
		return;
	}

	DWT_EnableCycleCounter();

	tx_trace_enable(tx_trace_buffer, sizeof(tx_trace_buffer), 32U);
	tracex_started = true;
}

void tracex_stop(void)
{
	if (!tracex_started) {
		return;
	}

	tx_trace_disable();
	tracex_started = false;
}

int tracex_transfer_data(tracex_writer_t write_fn)
{
	if (write_fn == NULL) {
		return 1;
	}

	const uint8_t *p = tx_trace_buffer;
	size_t remaining = sizeof(tx_trace_buffer);

	while (remaining > 0u) {
		size_t n = (remaining > TRACEX_CHUNK_BYTES) ?
				   TRACEX_CHUNK_BYTES :
				   remaining;

		if (write_fn(p, (uint32_t)n) != 0) {
			return 1;
		}

		p += n;
		remaining -= n;
	}

	return 0;
}