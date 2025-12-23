#ifndef _CHARGING_H
#define _CHARGING_H

#include "datastructs.h"

/**
 * @brief entrypoint for handling balancing of cells.  DOES NOT ENABLE BALANCING, but does configure it.
 * 
 * @param analyzer general Analyzer struct for processed cell data
 * @param acc_data segment data 
 */
void handle_balance_cells(analyzer_t *analyzer, acc_data_t *acc_data);

#endif