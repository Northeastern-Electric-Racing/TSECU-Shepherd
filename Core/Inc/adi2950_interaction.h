#ifndef _ADI2950_INTERACTION_H
#define _ADI2950_INTERACTION_H

#include <stdint.h>
#include "adi_bms_2950data.h"
#include "stm32f4xx_hal.h"

void read_current_registers(cell_asic_2950 ic, SPI_HandleTypeDef *hspi);

void read_vbat_regsisters(cell_asic_2950 ic, SPI_HandleTypeDef *hspi);

void read_ivbat_regsisters(cell_asic_2950 ic, SPI_HandleTypeDef *hspi);


#endif