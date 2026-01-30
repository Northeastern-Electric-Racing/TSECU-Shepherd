#ifndef _ADI2950_INTERACTION_H
#define _ADI2950_INTERACTION_H

#include <stdint.h>
#include "adi_bms_2950data.h"
#include "stm32xx_hal.h"

void set_accumulation_count(cell_asic_2950 *ic, ACCI count);
/**
 * @brief Reads the battery voltage registers from the adbms2950.
 * @param ic Pointer to the adbms2950 data structure.
 * @param hspi Pointer to the SPI interface handle.
 */
void read_accumulated_current_vbat_registers(cell_asic_2950 *ic);

void init_hv_plate(hv_plate_t *hv_plate, cell_asic_2950 *ic,
		       ACCI conversion_count);

void read_v7_v9_registers(cell_asic_2950 *ic);

void read_v2_v3_registers(cell_asic_2950 *ic);

uint16_t read_conversion_count_registers(cell_asic_2950 *ic);

/**
 * @brief Sets the state of a gpo pin.
 * @param ic Pointer to the adbms2950 data structure.
 * @param gpo GPO to set
 */
void set_gpo(cell_asic_2950 *ic, GPO_2950 gpo);

/**
 * @brief Resets a gpo pin.
 * @param ic Pointer to the adbms2950 data structure.
 * @param gpo GPO to reset
 */
void reset_gpo(cell_asic_2950 *ic, GPO_2950 gpo);

#endif