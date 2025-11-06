#include "traceout.h"
#include "tracex.h"
#include "main.h"

extern UART_HandleTypeDef huart6;

/* GPIO pin used to trigger trace output */
#define TRACEOUT_TRIGGER_PIN GPIO_PIN_5

void TraceOut_AppInit(void)
{
	traceout_init(&huart6);
}

#if (ENABLE_TRACEX)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == TRACEOUT_TRIGGER_PIN) {
		traceout_start_from_isr();
	}
}
#endif

#if (ENABLE_TRACEX)
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	traceout_on_uart_tx_complete_from_isr(huart);
}
#endif