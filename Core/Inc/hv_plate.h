#ifndef _HV_PLATE_H
#define _HV_PLATE_H

#include <stdint.h>

uint16_t get_pack_current();

uint16_t get_batt_voltage();

uint16_t get_ts_voltage();  

void trigger_precharge_relay();

#endif