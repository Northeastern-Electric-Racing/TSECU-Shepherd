#ifndef __SHEP_MUTEX_H
#define __SHEP_MUTEX_H

#include "tx_api.h"
#include "u_tx_debug.h"
#include "u_tx_mutex.h"
#include <stdint.h>
#include <stdbool.h>

/* Mutex List */
extern mutex_t analyzer_mutex;
extern mutex_t state_mutex;
extern mutex_t bms_algos_mutex;
extern mutex_t peripherals_mutex; 
extern mutex_t shutdown_mutex; 
extern mutex_t hv_plate_comms_mutex;
// add more as necessary...

/* API */
uint8_t mutexes_init(); // Initializes all mutexes set up in u_mutexes.c

#endif /* u_mutex.h */
