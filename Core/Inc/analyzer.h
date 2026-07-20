#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "app_threadx.h"
#include "datastructs.h"

/**
 * @brief Get the chip data for the given chip
 */
chipdata_t *get_chip_data(analyzer_t *analyzer, uint8_t chip);

/**
 * @brief Calculate thermistor values and cell temps using thermistors.
 */
void calc_cell_temps(analyzer_t *analyzer, acc_data_t *acc_data);

/**
 * @brief Calculates pack temp, and avg, min, and max cell temperatures.
 *
 */
void calc_pack_temps(analyzer_t *analyzer, acc_data_t *acc_data);

/**
 * @brief Calclaute the voltage of every cell in the pack.
 *
 */
void calc_cell_voltages(analyzer_t *analyzer, acc_data_t *acc_data,
                        state_machine_t *state_machine);

/**
 * @brief Calculate statistics about pack voltage, such as min and max cell
 * volt, pack and avg voltage, pack and avg OCV, and deltas.
 *
 */
void calc_pack_voltage_stats(analyzer_t *analyzer, acc_data_t *acc_data);

/**
 * @brief Calculate open cell voltages based on cell voltages and previous open
 * cell voltages.
 *
 */
void calc_open_cell_voltage(analyzer_t *analyzer, hv_plate_t *hv_plate);

/**
 * @brief Calculate cell resistances using Rin = ( Voc - V )/I
 */
void calc_cell_resistances(analyzer_t *analyzer, acc_data_t *acc_data,
                           hv_plate_t *hv_plate);

/**
 * @brief Calculate voltage drop percentages and detect open wires.
 */
void detect_cell_open_wire(analyzer_t *analyzer, acc_data_t *acc_data);

/**
 * @brief Updates the cell status of balancing and S_C_faults based on raw cell
 * data
 */
void update_chip_status(analyzer_t *analyzer, acc_data_t *acc_data);

void vAnalyzer(ULONG thread_input);

#endif
