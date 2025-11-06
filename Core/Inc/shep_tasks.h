
#ifndef SHEP_TASKS_H
#define SHEP_TASKS_H

#include <stdint.h>
#include "tx_api.h"

/* Initializes all ThreadX threads. 
*  Calls to _create_thread() should go in here
*/
uint8_t shep_threads_init(TX_BYTE_POOL *byte_pool);

void vDefaultTask(ULONG thread_input);
void vStateMachine(ULONG thread_input);
void vCanReceive(ULONG thred_input);
void vCanDispatch(ULONG thread_input);
void vAnalyzer(ULONG thread_input);
void vGetSegmentData(ULONG thread_input);
void vHvPlateData(ULONG thread_input);
void vSanitizer(ULONG thread_input);

#define ANALYZER_FLAG 0x1

#endif