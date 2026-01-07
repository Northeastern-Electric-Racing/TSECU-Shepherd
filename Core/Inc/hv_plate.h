#ifndef _HV_PLATE_H
#define _HV_PLATE_H

#include <stdint.h>
#include "datastructs.h"
#include "stm32xx_hal.h"
#include "adi_bms_2950data.h"

#define SHUNT_RESISTANCE 0.05 / 1000 // 0.05 mOhms

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
float get_batt_voltage(cell_asic_2950 *bmsdata, SPI_HandleTypeDef *hspi);

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

/**
 * @brief Sets the HV_CTRL GPO to the desired state to toggle precharge
 * 
 * @param ic pointer to adbms data struct
 * @param hspi pointer to spi handler 
 * @param state if true, pulls the GPO up, if false pulls it down
 */
void set_precharge_relay(cell_asic_2950 *ic, SPI_HandleTypeDef *hspi,
			 bool state);

#endif