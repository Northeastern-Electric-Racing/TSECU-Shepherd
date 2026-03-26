#include "can_handler.h"
#include "control.h"
#include "datastructs.h"
#include "state_machine.h"
#include "u_queues.h"
#include "u_tx_debug.h"
#include "u_tx_general.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define CAN_MSG_QUEUE_SIZE 50 /* messages */

extern FDCAN_HandleTypeDef hfdcan2;
can_t can1;

uint8_t init_can1(FDCAN_HandleTypeDef *hcan) {
  /* Init CAN interface */
  HAL_StatusTypeDef status = can_init(&can1, hcan);
  if (status != HAL_OK) {
    PRINTLN_ERROR(
        "Failed to execute can_init() when initializing can1 (Status: %d/%s).",
        status, hal_status_toString(status));
    return U_ERROR;
  }

  /* Add filters for standard IDs */
  uint16_t standard1[] = {BATTBOX_TEMP_CANID, CALYPSO_CONTROL_CANID};
  status = can_add_filter_standard(&can1, standard1);
  if (status != HAL_OK) {
    PRINTLN_ERROR("Failed to add standard filter to can1 (Status: %d/%s, ID1: "
                  "%d, ID2: %d).",
                  status, hal_status_toString(status), standard1[0],
                  standard1[1]);
    return U_ERROR;
  }

  /* Add fitlers for extended IDs */
  uint32_t extended1[] = {CHARGERBOX_CANID, 0x00};
  status = can_add_filter_extended(&can1, extended1);
  if (status != HAL_OK) {
    PRINTLN_ERROR("Failed to add extended filter to can1 (Status: %d/%s, ID1: "
                  "%ld, ID2: %ld).",
                  status, hal_status_toString(status), extended1[0],
                  extended1[1]);
    return U_ERROR;
  }

  PRINTLN_INFO("Ran can1_init().");

  return U_SUCCESS;
}

static uint8_t receive_can_msg(can_msg_t can_msg) {
  return queue_send(&can_incoming, &can_msg, TX_NO_WAIT);
}

void can_receive_callback(FDCAN_HandleTypeDef *hcan, uint32_t RxFifo0ITs) {
  /* If a message has just been recieved... */
  if (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) {
    can_msg_t message;
    FDCAN_RxHeaderTypeDef rx_header;

    if (HAL_FDCAN_GetRxMessage(hcan, FDCAN_RX_FIFO0, &rx_header,
                               message.data) == HAL_OK) {
      message.id = rx_header.Identifier;
      message.id_is_extended = (rx_header.IdType == FDCAN_EXTENDED_ID);
      message.len = (uint8_t)rx_header.DataLength;

      /* Check size */
      if (rx_header.DataLength > 8) {
        printf("[main.c/HAL_FDCAN_RxFifo0Callback()] ERROR: Recieved message "
               "is larger than 8 bytes.\n");
        return;
      }

      /* Send message to incoming CAN queue */
      receive_can_msg(message);
    }
  }
}

uint8_t queue_can_msg(can_msg_t can_msg) {
  return queue_send(&can_outgoing, &can_msg, TX_NO_WAIT);
}

/**
 * @brief Parses the DTI can message for pack current
 *
 * @param msg
 * @return float
 */
float parse_dti_current(can_msg_t msg) {
  int16_t curr = msg.data[2] << 8 | msg.data[3];
  return ((float)curr) / 10;
}
float parse_charger_current(can_msg_t msg) {
  int16_t curr = msg.data[2] << 8 | msg.data[3];
  return ((float)curr) / 10;
}

// CAN RECIEVE THREAD
void vCanReceive(ULONG thread_input) {
  state_machine_args_t *state_machine_args = (state_machine_args_t *)thread_input;
  can_msg_t message;
  for (;;) {
    /* Process incoming messages */
    while (queue_receive(&can_incoming, &message, TX_WAIT_FOREVER) ==
           U_SUCCESS) {
      switch (message.id) {
      case CHARGERBOX_CANID:
        charger_message_recieved(state_machine_args);
        break;
      case CALYPSO_CONTROL_CANID:
        control_message_fans(message);
        break;
      case BATTBOX_TEMP_CANID:
        control_message_fans_lv(message);
        break;
      default:
        break;
      }
    }
  }
}

// CAN DISPATCH THREAD
void vCanDispatch(ULONG thread_input) {
  // INITIALIZING CAN
  PRINTLN_INFO("INITIALIZED CAN");

  can_msg_t message;
  uint8_t status;

  for (;;) {
    /* Process incoming messages */
    while (queue_receive(&can_outgoing, &message, TX_WAIT_FOREVER) ==
           U_SUCCESS) {
      status = can_send_msg(&can1, &message);
      if (status != U_SUCCESS) {
        PRINTLN_WARNING("Failed to send message (on can1) after removing from "
                        "outgoing queue (Message ID: %ld) - Status %d",
                        message.id, status);
      } else {
        PRINTLN_INFO("Sent CAN message with ID: %ld", message.id);
      }
    }
  }
}
