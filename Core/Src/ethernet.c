#include "ethernet.h"

void _ethernet_recieve(ethernet_message_t message)
{
	/* Send the message to the incoming ethernet queue. */
	int status = queue_send(&eth_incoming, &message, TX_NO_WAIT);
	if (status != U_SUCCESS) {
		PRINTLN_ERROR(
			"Failed to send message to the incoming ethernet queue (Status: %d).",
			status);
		return;
	}
}

uint8_t ethernet1_init(void)
{
	/* PHY_RESET Pin has to be set HIGH for the PHY to function. */
	HAL_GPIO_WritePin(PHY_RESET_GPIO_Port, PHY_RESET_Pin, GPIO_PIN_SET);

	/* Init the ethernet. */
	return ethernet_init(COMPUTE, nx_stm32_eth_driver, _ethernet_recieve);
}

void ethernet_inbox(ethernet_message_t *message)
{
	switch (message->message_id) {
		case 0x01:
			// do thing
			break;
		case 0x02:
			// do thing
			break;
		case 0x03:
			// etc
			break;
		default:
			PRINTLN_ERROR(
				"Unknown Ethernet Message Recieved (Message ID: %d).",
				message->message_id);
			break;
	}
}

uint8_t queue_eth_msg(ethernet_message_t eth_msg)
{
	return queue_send(&eth_outgoing, &eth_msg, TX_WAIT_FOREVER);
}

// ETHERNET INCOMING THREAD
void vEthernetIncoming(ULONG thread_input)
{
	while (1) {
		ethernet_message_t message;
		/* Process incoming messages */
		while (queue_receive(&eth_incoming, &message,
				     TX_WAIT_FOREVER) == U_SUCCESS) {
			ethernet_inbox(&message);
		}
	}
}
 
// ETHERNET OUTGOING THREAD
void vEthernetOutgoing(ULONG thread_input)
{
	while (1) {
		ethernet_message_t message;
		uint8_t status;
		/* Send outgoing messages */
		while (queue_receive(&eth_outgoing, &message,
				     TX_WAIT_FOREVER) == U_SUCCESS) {
			status = ethernet_send_message(&message);
			if (status != U_SUCCESS) {
				PRINTLN_WARNING(
					"Failed to send Ethernet message after removing from "
					"outgoing queue (Message ID: %d).",
					message.message_id);
				// u_TODO - maybe add the message back into the queue if it fails to
				// send? not sure if this is a good idea tho
			} else {
				PRINTLN_INFO("Sent ethernet message!");
			}
		}
	}
}
