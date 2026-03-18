#ifndef _ETHERNET_H
#define _ETHERNET_H

#include "nx_stm32_eth_driver.h"
#include "u_nx_ethernet.h"
#include "main.h"
#include "u_queues.h"

/**
 * @brief Initializes ethernet.
 *
 * @return Status.
 */
uint8_t ethernet1_init(void);

/**
 * @brief Processes received ethernet messages.
 *
 */
void ethernet_inbox(ethernet_message_t *message);

/**
 * @brief Queue message over ethernet
 *
 * @return Status.
 */
uint8_t queue_eth_msg(ethernet_message_t eth_msg);

void vEthernetIncoming(ULONG thread_input);

void vEthernetOutgoing(ULONG thread_input);

#endif
