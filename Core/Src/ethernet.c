#include "ethernet.h"
#include "u_queues.h"
#include "nx_stm32_eth_driver.h"
#include "u_nx_ethernet.h"
#include "u_nx_debug.h"
#include "main.h"
#include "u_tx_debug.h"
#include "u_tx_general.h"
#include "u_tx_queues.h"

#define ETH_TYPE_SEND 0
#define ETH_TYPE_RECV 1

/* Callback for when a ethernet message is recieved. */
void _ethernet_recieve(eth_mqtt_queue_message_t message) {
  /* Send the message to the incoming ethernet queue. */
  int status = queue_send(&eth_manager, &message, TX_NO_WAIT);
  if(status != U_SUCCESS) {
    PRINTLN_ERROR("Failed to send message to the incoming ethernet queue (Status: %d).", status);
    return;
  }
}

/* Initializes ethernet. */
UINT ethernet1_init(void) {
    /* PHY_RESET Pin has to be set HIGH for the PHY to function. */
    HAL_GPIO_WritePin(PHY_RESET_GPIO_Port, PHY_RESET_Pin, GPIO_PIN_SET);

    /* Init the ethernet. */
    return ethernet_init(VCU, nx_stm32_eth_driver, _ethernet_recieve);
}


void vEthernetManager(ULONG thread_input) {

    ethernet_mqtt_message_t message = { 0 };

    while(1) {

        /* Send outgoing messages, recieve incoming messages */
        while(queue_receive(&eth_manager, &message, TX_WAIT_FOREVER) == U_SUCCESS) {
            int status = nx_protobuf_mqtt_message_send(&message);
            if(status != U_SUCCESS) {
                PRINTLN_ERROR("Failed to send outgoing mqtt ethernet message (Status: %d).", status);
            }
        }

        /* No sleep. Thread timing is controlled completely by the queue timeout. */
    }
}
