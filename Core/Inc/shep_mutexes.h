#ifndef _H_SHEP_MUTEXES
#define _H_SHEP_MUTEXES

#include <stdint.h>
#include "u_mutex.h"

extern mutex_t logger_mutex;
extern mutex_t bms_mutex;

uint8_t mutexes_init(); // Initializes all mutexes

#endif