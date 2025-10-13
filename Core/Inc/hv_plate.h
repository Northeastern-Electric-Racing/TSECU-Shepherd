#ifndef _HV_PLATE_H
#define _HV_PLATE_H

#include <stdint.h>
#include "datastructs.h"
#include "stm32xx_hal.h"
#include "adi_bms_2950data.h"

// TODO: docs

#define SHUNT_RESISTANCE 0.05 / 1000 // 0.05 mOhms

void init_hv_plate_chip(cell_asic_2950 ic);
float get_pack_current(bms_t *bmsdata, SPI_HandleTypeDef *hspi);
float get_batt_voltage(bms_t *bmsdata, SPI_HandleTypeDef *hspi);
float get_ts_voltage(bms_t *bmsdata, SPI_HandleTypeDef *hspi);  
void trigger_precharge_relay(bms_t *bmsdata, SPI_HandleTypeDef *hspi);

#endif