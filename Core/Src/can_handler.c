#include "can_handler.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "datastructs.h"
#include "state_machine.h"
#include "shep_queues.h"

#define CAN_MSG_QUEUE_SIZE 50 /* messages */

#define CAN_DISPATCH_FLAG 1U

#define NEW_CAN_MSG_FLAG 1U

/**
 * @brief Datastructure for keeping track of the last time a CAN message was transmitted.
 * 
 */
typedef struct {
	uint32_t id;
	uint32_t prev_tick;
	uint32_t msg_rate; /* in milliseconds */
} rl_can_msg_t;

struct node_t {
	rl_can_msg_t val;
	struct node_t *next;
};

struct node_t *rl_bms_msgs = NULL;

can_t can1;

static uint16_t can1_id_list_standard[4] = {
	//CANID_X,
	DTI_CURRENT_CANID,
};

static uint32_t can1_id_list_extended[2] = {
	//CANID_X,
	CHARGERBOX_CANID
};

/**
 * @brief Add a CAN message to the list of rate limited CAN messages.
 * 
 * @param id ID of the CAN message to rate limit.
 * @param msg_rate The amount of time that must pass before this CAN message can be ttansmitted again.
 */
void init_rl_can_msg(uint32_t id, uint32_t msg_rate)
{
	if (rl_bms_msgs == NULL) {
		rl_bms_msgs = malloc(sizeof(struct node_t));
		rl_bms_msgs->val.id = id;
		rl_bms_msgs->val.msg_rate = msg_rate;
		rl_bms_msgs->val.prev_tick = HAL_GetTick();
		rl_bms_msgs->next = NULL;
		return;
	}

	struct node_t *curr = rl_bms_msgs;

	while (curr->next != NULL) {
		curr = curr->next;
	}

	struct node_t *next = malloc(sizeof(struct node_t));
	next->val.id = id;
	next->val.msg_rate = msg_rate;
	next->val.prev_tick = HAL_GetTick();
	next->next = NULL;

	curr->next = next;
}

void init_can(CAN_HandleTypeDef *hcan1)
{
	can1 = malloc(sizeof(can_t));
	assert(can1);

	can1->hcan = hcan1;
	assert(can_init(can1));
	assert(!can_add_filter_extended(can1, can1_id_list_extended));
	assert(!can_add_filter_standard(can1, can1_id_list_standard));
}

void can_receive_callback(CAN_HandleTypeDef *hcan)
{
	CAN_RxHeaderTypeDef rx_header;
	can_msg_t new_msg;
	/* Read in CAN message */
	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header,
				 new_msg.data) != HAL_OK) {
		// TODO add non crtical fault capability - could create one for failed can receieve
		return;
	}

	new_msg.len = rx_header.DLC;

	if (rx_header.IDE == CAN_ID_EXT) {
		// If the message has an extended CAN ID, save the message accordingly.
		new_msg.id = rx_header.ExtId;
		new_msg.id_is_extended = true;
	} else {
		// If the message has a standard CAN ID, save the message accordingly.
		new_msg.id = rx_header.StdId;
		new_msg.id_is_extended = false;
	}

	queue_can_msg(new_msg);
}

int8_t queue_can_msg(can_msg_t msg)
{
	struct node_t *curr = rl_bms_msgs;

	while (curr != NULL) {
		if (curr->val.id == msg.id) {
			if (HAL_GetTick() <=
			    curr->val.prev_tick +
				    pdMS_TO_TICKS(curr->val.msg_rate)) {
				// block message
				// printf("Blocked 0x%lX\t", msg.id);
				return 0;
			} else {
				// printf("Sent 0x%lX\n", msg.id);
				curr->val.prev_tick = HAL_GetTick();
				break;
			}
		}
		curr = curr->next;
	}

	return queue_send(&can_incoming, &msg);
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
