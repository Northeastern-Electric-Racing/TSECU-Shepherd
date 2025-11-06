#include "traceout.h"

static UART_HandleTypeDef *s_huart = NULL;
static volatile uint8_t s_output_active = 0U;
static const uint8_t *s_dp = NULL;
static uint32_t s_remaining = 0U;

static void start_next_chunk(void)
{
	if (s_remaining == 0U) {
		s_output_active = 0U;
		tracex_start();
		return;
	}

	uint32_t n = (s_remaining > TRACEOUT_CHUNK_BYTES) ?
			     TRACEOUT_CHUNK_BYTES :
			     s_remaining;

	if (HAL_UART_Transmit_DMA(s_huart, (uint8_t *)s_dp, (uint16_t)n) ==
	    HAL_OK) {
		s_dp += n;
		s_remaining -= n;
	} else {
		s_output_active = 0U;
	}
}

void traceout_init(UART_HandleTypeDef *huart)
{
	s_huart = huart;
}

void traceout_start_from_isr(void)
{
	if ((s_huart == NULL) || (s_output_active != 0U)) {
		return;
	}

	tracex_stop();

	s_dp = tracex_get_buffer();
	s_remaining = TRACEX_BUFFER_SIZE;
	s_output_active = 1U;

	start_next_chunk();
}

void traceout_on_uart_tx_complete_from_isr(UART_HandleTypeDef *huart)
{
	if ((huart == s_huart) && (s_output_active != 0U)) {
		start_next_chunk();
	}
}