#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "datastructs.h"

/**
 * @brief Calculate thermistor values and cell temps using thermistors.
 * 
 */
void calc_cell_temps(analyzer_t *analyzer);

/**
 * @brief Calculates pack temp, and avg, min, and max cell temperatures.
 * 
 */
void calc_pack_temps(analyzer_t *analyzer);

/**
 * @brief Calclaute the voltage of every cell in the pack.
 * 
 * @param bmsdata Pointer to BMS data struct.
 */
void calc_cell_voltages(analyzer_t *analyzer);

/**
 * @brief Calculate statistics about pack voltage, such as min and max cell volt, pack and avg voltage, pack and avg OCV, and deltas.
 * 
 */
void calc_pack_voltage_stats(analyzer_t *analyzer);

/**
 * @brief Calculate open cell voltages based on cell voltages and previous open cell voltages.
 * 
 */
void calc_open_cell_voltage(analyzer_t *analyzer);

/**
 * @brief Calculate cell resistances using Rin = ( Voc - V )/I
 * 
 */
void calc_cell_resistances(analyzer_t *analyzer);

#endif