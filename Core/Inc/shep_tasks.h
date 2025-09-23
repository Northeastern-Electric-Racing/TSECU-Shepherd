
#ifndef SHEP_TASKS_H
#define SHEP_TASKS_H

#include <stdint.h>
#include "tx_api.h"

/* Initializes all ThreadX threads. 
*  Calls to _create_thread() should go in here
*/
uint8_t shep_threads_init(TX_BYTE_POOL *byte_pool);

#endif