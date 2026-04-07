#pragma once
#include "serverdata.pb.h"
#include "u_nx_protobuf.h"
#include "tx_api.h"
#include <stdint.h>

#define ETH_MAX_TOPIC_SIZE 100

/* API */
/**
 * Initialize ethernet
 */
UINT ethernet1_init(void);
/**
 * Send a protobuf message over MQTT
 */
UINT ethernet1_mqtt_send(char* topic, uint8_t topic_size, char* unit, uint8_t unit_size, float* values, uint8_t values_len, uint64_t time_us);

typedef struct {
    uint8_t type;
    char topic[ETH_MAX_TOPIC_SIZE];
    uint8_t topic_size;
    serverdata_v2_ServerData msg;
} eth_mqtt_queue_message_t;

/**
 * Main ethernet manager, only one to access ethernet functions
 */
void vEthernetManager(ULONG thread_input);
