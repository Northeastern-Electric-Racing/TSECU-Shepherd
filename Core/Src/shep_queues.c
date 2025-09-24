#include "shep_queues.h"
#include "u_debug.h"
#include "fdcan.h"

/* Incoming CAN Queue */
queue_t can_incoming = {
    .name = "Incoming CAN Queue",           /* Name of the queue. */
    .message_size = sizeof(can_msg_t),      /* Size of each queue message, in bytes. */
    .capacity = 10                          /* Number of messages the queue can hold. */
};

/* Outgoing CAN Queue */
queue_t can_outgoing = {
    .name = "Outgoing CAN Queue",          /* Name of the queue. */
    .message_size = sizeof(can_msg_t),     /* Size of each queue message, in bytes. */
    .capacity = 10                         /* Number of messages the queue can hold. */
};

/* Initializes all ThreadX queues. 
*  Calls to _create_queue() should go in here
*/
uint8_t queues_init(TX_BYTE_POOL *byte_pool) {

    /* Create Queues */
    CATCH_ERROR(create_queue(byte_pool, &can_incoming), U_SUCCESS); // Create Incoming CAN Queue
    CATCH_ERROR(create_queue(byte_pool, &can_outgoing), U_SUCCESS); // Create Outgoing CAN Queue

    DEBUG_PRINTLN("Ran queues_init().");
    return U_SUCCESS;
}
