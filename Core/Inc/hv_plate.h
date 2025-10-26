#ifndef _HV_PLATE_H
#define _HV_PLATE_H

#include <stdint.h>
#include "datastructs.h"
#include "stm32xx_hal.h"
#include "adi_bms_2950data.h"

#define SHUNT_RESISTANCE 0.05 / 1000 // 0.05 mOhms

/**
 * @brief initializes the adbms2950
 * 
 * @param ic adbms2950 data struct
 */
void init_hv_plate_chip(cell_asic_2950 ic);

/**
 * @brief Gets the pack current reading from the adbms2950
 * 
 * @param bmsdata pointer to bms data struct
 * @param hspi pointer to spi handler 
 */
float get_pack_current(bms_t *bmsdata, SPI_HandleTypeDef *hspi);

/**
 * @brief Gets the batt volatage reading from the adbms2950
 * 
 * @param bmsdata pointer to bms data struct
 * @param hspi pointer to spi handler 
 */
float get_batt_voltage(bms_t *bmsdata, SPI_HandleTypeDef *hspi);

/**
 * @brief Gets the TS voltage from the adbms2950
 * 
 * @param bmsdata pointer to bms data struct
 * @param hspi pointer to spi handler 
 */
float get_ts_voltage(bms_t *bmsdata, SPI_HandleTypeDef *hspi);  

/**
 * @brief Gets the current temperature of the shunt resistor
 * 
 * @param bmsdata pointer to bms data struct
 * @param hspi pointer to spi handler  
 */
float get_shunt_temp(bms_t *bmsdata, SPI_HandleTypeDef *hspi);

/**
 * @brief Sets the HV_CTRL GPO to the desired state to toggle precharge
 * 
 * @param bmsdata pointer to bms data struct
 * @param hspi pointer to spi handler 
 * @param state if true, pulls the GPO up, if false pulls it down
 */
void set_precharge_relay(bms_t *bmsdata, SPI_HandleTypeDef *hspi, bool state);


#endif