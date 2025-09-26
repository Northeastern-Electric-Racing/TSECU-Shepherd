#ifndef _SHEP_QUEUES_H
#define _SHEP_QUEUES_H

#include "tx_api.h"
#include <stdint.h>
#include "u_queues.h"

extern queue_t can_incoming; // Incoming CAN Queue
extern queue_t can_outgoing; // Outgoing CAN Queue

uint8_t queues_init(TX_BYTE_POOL *byte_pool); // Initializes all queues. Called from app_threadx.c

#endif