#ifndef _ADI2950_INTERACTION_H
#define _ADI2950_INTERACTION_H

#include <stdint.h>
#include "adi_bms_2950data.h"

/**
 * @brief Sends PEC errors for hv plate over CAN and clears them for the next cycle.
 */
void send_hv_plate_pec_errors(void);

/**
 * Snaps registers of ADBMS2950
 * 
 * @param ic Pointer to the adbms2950 data structure.
 */
void snap_2950(cell_asic_2950 *ic);

/**
 * Unsnaps registers of ADBMS2950
 * 
 * @param ic Pointer to the adbms2950 data structure.
 */
void unsnap_2950(cell_asic_2950 *ic);

/**
 * Begins continuous ADC conversions with redundancy
 * 
 * @param ic Pointer to the adbms2950 data structure.
 */
void start_adc_conversions(cell_asic_2950 *ic);

/**
 * @brief Sets the config for current and voltage measurements.
 * 
 * @param ic Pointer to the adbms2950 data structure.
 * @param count Accumulation count to set.
 */
void write_config(cell_asic_2950 *ic, ACCI count);

/**
 * @brief Reads the accumulated current and battery voltage registers from the adbms2950.
 * @param ic Pointer to the adbms2950 data structure.
 */
void read_accumulated_current_vbat_registers(cell_asic_2950 *ic);

/**
 * @brief Reads V7 and V9 redundant pair voltage registers.
 * @param ic Pointer to the adbms2950 data structure.
 */
void read_v7_v9_registers(cell_asic_2950 *ic);

/**
 * @brief Reads V2 voltage register.
 * @param ic Pointer to the adbms2950 data structure.
 */
void read_v2_register(cell_asic_2950 *ic);

/**
 * @brief Reads flag register.
 * @param ic Pointer to the adbms2950 data structure.
 */
void read_flag_register(cell_asic_2950 *ic);

/**
 * @brief Reads all aux register groups. (NOTE: Must restart continuous conversion after)
 * @param ic Pointer to the adbms2950 data structure.
 */
void read_aux_registers(cell_asic_2950 *ic);

/**
 * @brief Reads the conversion count registerm for total number of ADC conversions.
 * @param ic Pointer to the adbms2950 data structure.
 * @return uint16_t The current conversion count.
 */
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
