#include "can_handler.h"
#include "datastructs.h"
#include "shep_queues.h"
#include "state_machine.h"
#include "u_tx_general.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define CAN_MSG_QUEUE_SIZE 50 /* messages */

can_t *can1;

static uint16_t can1_id_list_standard[4] = {
	// CANID_X,
	DTI_CURRENT_CANID,
};

static uint32_t can1_id_list_extended[2] = {
	// CANID_X,
	CHARGERBOX_CANID
};

uint8_t init_can(FDCAN_HandleTypeDef *hcan)
{
	return can_filter_init(hcan, can1, can1_id_list_standard,
			       can1_id_list_extended);
}

void can_receive_callback(FDCAN_HandleTypeDef *hcan, uint32_t RxFifo0ITs)
{
	/* If a message has just been recieved... */
	if (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) {
		can_msg_t message;
		FDCAN_RxHeaderTypeDef rx_header;

		if (HAL_FDCAN_GetRxMessage(hcan, FDCAN_RX_FIFO0, &rx_header,
					   message.data) == HAL_OK) {
			message.id = rx_header.Identifier;
			message.id_is_extended =
				(rx_header.IdType == FDCAN_EXTENDED_ID);
			message.len = (uint8_t)rx_header.DataLength;

			/* Check size */
			if (rx_header.DataLength > 8) {
				printf("[main.c/HAL_FDCAN_RxFifo0Callback()] ERROR: Recieved message "
				       "is larger than 8 bytes.\n");
				return;
			}

			/* Send message to incoming CAN queue */
			queue_can_msg(message);
		}
	}
}

uint8_t queue_can_msg(can_msg_t can_msg)
{
	return queue_send(&can_outgoing, &can_msg, TX_WAIT_FOREVER);
}

/**
 * @brief Parses the DTI can message for pack current
 *
 * @param msg
 * @return float
 */
float parse_dti_current(can_msg_t msg)
{
	int16_t curr = msg.data[2] << 8 | msg.data[3];
	return ((float)curr) / 10;
}
float parse_charger_current(can_msg_t msg)
{
	int16_t curr = msg.data[2] << 8 | msg.data[3];
	return ((float)curr) / 10;
}
