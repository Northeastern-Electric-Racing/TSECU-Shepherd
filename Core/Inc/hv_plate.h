#ifndef _HV_PLATE_H
#define _HV_PLATE_H

#include <stdint.h>
#include "datastructs.h"
#include "stm32xx_hal.h"
#include "adi_bms_2950data.h"
#include "app_threadx.h"

#define SHUNT_RESISTANCE 0.05 / 1000 // 0.05 mOhms
#define HV_CTRL_GPO	 GPIO4_2950

/**
 * @brief Initializes an hv plate struct
 *
 * @param hv_plate pointer to hv plate data struct
 * @param conversion_count accumulation count for current and voltage measurements
 */
void init_hv_plate(hv_plate_t *hv_plate, ACCI conversion_count);

/**
 * @brief Gets the pack current reading from the adbms2950
 *
 * @param hv_plate pointer to hv plate data struct
 * @param reques_rate the rate at which this function is called in milliseconds
 */
void get_pack_current_and_batt_voltage(hv_plate_t *hv_plate,
				       uint16_t request_rate);

/**
 * @brief Gets the TS voltage from the adbms2950
 *
 * @param hv_p'ate pointer to hv plate data struct
 */
void get_ts_voltage(hv_plate_t *hv_plate);

/**
 * @brief Gets the current temperature of the shunt resistor
 *
 * @param hv_p'ate pointer to hv splate data struct
 */
void get_shunt_temp(hv_plate_t *hv_plate);

/**
 * @brief Gets flags from adbms2950
 *
 * @param hv_p'ate pointer to hv splate data struct
 */
void get_flags(hv_plate_t *hv_plate);

/**
 * @brief Gets all diagnostic data from aux adc
 *
 * @param hv_p'ate pointer to hv splate data struct
 */
void get_aux_adc_data(hv_plate_t *hv_plate);

/**
 * @brief Reset, then wake, then re-configure adbms2950 chip
 *
 * @param hv_plate Pointer to the hv plate data structure.
 */
void hv_plate_restart(hv_plate_t *hv_plate);

void vHvPlateData(ULONG thread_input);

#endif
