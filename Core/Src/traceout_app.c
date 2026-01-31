/**
 * @file traceout_app.c
 * @brief Application-level TraceX output integration.
 */

#include "traceout_app.h"

#if (ENABLE_TRACEX)

#include "tracex.h"
#include "traceout.h"
#include "main.h"
#include "stm32h5xx.h"

/*************** Configuration ***************/

#define TRACE_BUFFER_SIZE    (256U * 1024U)
#define TRACEOUT_TRIGGER_PIN GPIO_PIN_8

extern UART_HandleTypeDef huart4;

/*************** TraceX Buffer ***************/

/* Aligned for cache-line safety */
__attribute__((aligned(32))) static UCHAR s_trace_buffer[TRACE_BUFFER_SIZE];

/*************** Platform Hooks ***************/

/**
 * @brief Enable CPU cycle counter for TraceX timestamps.
 */
void tracex_enable_cycle_counter(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0U;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/*************** HAL Callbacks ***************/

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == TRACEOUT_TRIGGER_PIN) {
		traceout_start_from_isr();
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart4) {
		traceout_on_tx_complete_from_isr();
	}
}

/*************** UART Transport ***************/

static void uart_tx_start(const uint8_t *data, uint16_t len)
{
	(void)HAL_UART_Transmit_DMA(&huart4, (uint8_t *)data, len);
}

/*************** Public API ***************/

void TraceOut_AppInit(void)
{
	tracex_init(s_trace_buffer, sizeof(s_trace_buffer));
	traceout_init(uart_tx_start);
}

#endif /* ENABLE_TRACEX */
