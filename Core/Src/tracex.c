#include "tracex.h"
#include "stm32f405xx.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* 32-byte alignment for cache line safety. */
__attribute__((aligned(32))) static UCHAR tx_trace_buffer[TRACEX_BUFFER_SIZE];

static bool tracex_started = false;

/**
 * @brief  Enables the DWT cycle counter for precise CPU cycle measurements.
 * @note   Requires the trace unit (DEMCR.TRCENA) to be enabled.
 */
static inline void DWT_EnableCycleCounter(void)
{
	CoreDebug->DEMCR |=
		CoreDebug_DEMCR_TRCENA_Msk; /* Enable trace and debug block. */
	DWT->CYCCNT = 0; /* Reset cycle counter. */
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; /* Start cycle counting. */
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