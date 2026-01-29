#ifndef _HV_PLATE_H
#define _HV_PLATE_H

#include <stdint.h>
#include "datastructs.h"
#include "stm32xx_hal.h"
#include "adi_bms_2950data.h"

<<<<<<< HEAD
#define SHUNT_RESISTANCE 0.05 / 1000 // 0.05 mOhms
=======
#define HV_CTRL_GPO GPIO4_2950
>>>>>>> develop

/**
 * @brief initializes the adbms2950
 * @param ic adbms2950 data struct
 */
void init_hv_plate_chip(cell_asic_2950 ic);

/**
 * @brief Gets the pack current reading from the adbms2950
 * 
 * @param ic pointer to adbms data struct
 * @param hspi pointer to spi handler 
 */
float get_pack_current(cell_asic_2950 *ic, SPI_HandleTypeDef *hspi);

/**
 * @brief Gets the batt volatage reading from the adbms2950
 * 
 * @param ic pointer to adbms data struct
 * @param hspi pointer to spi handler 
 */
float get_batt_voltage(cell_asic_2950 *ic, SPI_HandleTypeDef *hspi);

/**
 * @brief Gets the TS voltage from the adbms2950
 * 
 * @param ic pointer to adbms data struct
 * @param hspi pointer to spi handler 
 */
float get_ts_voltage(cell_asic_2950 *ic, SPI_HandleTypeDef *hspi);

/**
 * @brief Gets the current temperature of the shunt resistor
 * 
 * @param ic pointer to adbms data struct
 * @param hspi pointer to spi handler  
 */
float get_shunt_temp(cell_asic_2950 *ic, SPI_HandleTypeDef *hspi);


#endif