#ifndef _ADI2950_INTERACTION_H
#define _ADI2950_INTERACTION_H

#include <stdint.h>
#include "adi_bms_2950data.h"
#include "stm32xx_hal.h"

/**
 * @brief Reads the current measurement registers from the adbms2950.
 * @param ic Pointer to the adbms2950 data structure.
 * @param hspi Pointer to the SPI interface handle.
 */
void read_current_registers(cell_asic_2950 ic, SPI_HandleTypeDef *hspi);

/**
 * @brief Reads the battery voltage registers from the adbms2950.
 * @param ic Pointer to the adbms2950 data structure.
 * @param hspi Pointer to the SPI interface handle.
 */
void read_vbat_regsisters(cell_asic_2950 ic, SPI_HandleTypeDef *hspi);

/**
 * @brief Sets the state of a gpo pin.
 * @param ic Pointer to the adbms2950 data structure.
 * @param hspi Pointer to the SPI interface handle.
 * @param gpo GPO to set
 */
void set_gpo(cell_asic_2950 ic, SPI_HandleTypeDef *hspi, GPO_2950 gpo);

/**
 * @brief Resets a gpo pin.
 * @param ic Pointer to the adbms2950 data structure.
 * @param hspi Pointer to the SPI interface handle.
 * @param gpo GPO to reset
 */
void reset_gpo(cell_asic_2950 ic, SPI_HandleTypeDef *hspi, GPO_2950 gpo);

/**
 * @brief Reads the voltage registers from the ADBMS2950.
 * @param ic Pointer to the adbms2950 data structure.
 * @param hspi Pointer to the SPI interface handle.
 */
void read_vr_registers(cell_asic_2950 ic, SPI_HandleTypeDef *hspi);


#endif