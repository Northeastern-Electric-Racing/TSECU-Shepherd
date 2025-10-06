#ifndef _HV_PLATE_H
#define _HV_PLATE_H

#include <stdint.h>
#include "hv_plate.h"
#include "datastructs.h"
#include "stm32xx_hal.h"

// TODO: docs

#define SHUNT_RESISTANCE 0.05 / 1000 // 0.05 mOhms

void init_hv_plate_chip(cell_asic_2950 ic);
float get_pack_current(acc_data_t *bmsdata, SPI_HandleTypeDef *hspi);
float get_batt_voltage(acc_data_t *bmsdata, SPI_HandleTypeDef *hspi);
float get_ts_voltage(acc_data_t *bmsdata, SPI_HandleTypeDef *hspi);  
void trigger_precharge_relay(acc_data_t *bmsdata, SPI_HandleTypeDef *hspi);

#endif