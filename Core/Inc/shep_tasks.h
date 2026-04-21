
#ifndef SHEP_TASKS_H
#define SHEP_TASKS_H

#include "tx_api.h"
#include "datastructs.h"

#define DEBUG_HV_PLATE
// #define DEBUG_RAW_VOLTAGES
// #define DEBUG_OCV_VOLTAGES
// #define DEBUG_TEMPS
// #define DEBUG_AlGOS

/* Initializes all ThreadX threads.
*  Calls to create_thread() should go in here
*/
uint8_t shep_threads_init(TX_BYTE_POOL *byte_pool);

#endif
