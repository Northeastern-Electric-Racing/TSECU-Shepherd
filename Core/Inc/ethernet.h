#ifndef _ETHERNET_H
#define _ETHERNET_H

#include "nx_stm32_eth_driver.h"
#include "u_nx_ethernet.h"
#include "main.h"
#include "shep_queues.h"

/**
 * @brief 
 */
int ethernet1_init(void);
void ethernet_inbox(ethernet_message_t *message);
uint8_t queue_eth_msg(ethernet_message_t eth_msg);

#endif
