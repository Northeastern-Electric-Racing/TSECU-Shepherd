#include "main.h"
#include "tracex.h"

/**
 * @file traceout.h
 * @brief UART-based TraceX output interface.
 */

#ifndef TRACEOUT_CHUNK_BYTES
#define TRACEOUT_CHUNK_BYTES (2048u)
#endif

/**
 * @brief Initialize the UART used for trace output.
 * @param huart Pointer to the UART handle.
 */
void traceout_init(UART_HandleTypeDef *huart);

/**
 * @brief Start trace output from an ISR context.
 */
void traceout_start_from_isr(void);

/**
 * @brief Continue trace output after UART TX completes.
 * @param huart Pointer to the UART handle used for trace output.
 */
void traceout_on_uart_tx_complete_from_isr(UART_HandleTypeDef *huart);