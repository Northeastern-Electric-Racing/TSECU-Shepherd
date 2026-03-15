#include "u_queues.h"
#include "fdcan.h"
#include "u_tx_debug.h"
#include "u_nx_ethernet.h"

/* Incoming CAN Queue */
queue_t can_incoming = {
	.name = "Incoming CAN Queue", /* Name of the queue. */
	.message_size =
		sizeof(can_msg_t), /* Size of each queue message, in bytes. */
	.capacity = 100 /* Number of messages the queue can hold. */
};

/* Outgoing CAN Queue */
queue_t can_outgoing = {
	.name = "Outgoing CAN Queue", /* Name of the queue. */
	.message_size =
		sizeof(can_msg_t), /* Size of each queue message, in bytes. */
	.capacity = 200 /* Number of messages the queue can hold. */
};

/* Incoming CAN Queue */
queue_t eth_incoming = {
	.name = "Incoming Ethernet Queue", /* Name of the queue. */
	.message_size = sizeof(
		ethernet_message_t), /* Size of each queue message, in bytes. */
	.capacity = 10 /* Number of messages the queue can hold. */
};

/* Outgoing CAN Queue */
queue_t eth_outgoing = {
	.name = "Outgoing Ethernet Queue", /* Name of the queue. */
	.message_size = sizeof(
		ethernet_message_t), /* Size of each queue message, in bytes. */
	.capacity = 10 /* Number of messages the queue can hold. */
};

/* Initializes all ThreadX queues.
 *  Calls to _create_queue() should go in here
 */
uint8_t queues_init(TX_BYTE_POOL *byte_pool)
{
	/* Create Queues */
	CATCH_ERROR(create_queue(byte_pool, &can_incoming),
		    U_SUCCESS); // Create Incoming CAN Queue
	CATCH_ERROR(create_queue(byte_pool, &can_outgoing),
		    U_SUCCESS); // Create Outgoing CAN Queue
	CATCH_ERROR(create_queue(byte_pool, &eth_incoming),
		    U_SUCCESS); // Create Incoming Ethernet Queue
	CATCH_ERROR(create_queue(byte_pool, &eth_outgoing),
		    U_SUCCESS); // Create Outgoing Ethernet Queue

	PRINTLN_INFO("Ran queues_init().");
	return U_SUCCESS;
}
