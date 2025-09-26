#ifndef _CHARGING_H
#define _CHARGING_H

#include "datastructs.h"

/**
 * @brief entrypoint for handling balancing of cells.  DOES NOT ENABLE BALANCING, but does configure it.
 * 
 * @param bmsdata general BMS data struct
 */
void handle_balance_cells(bms_t *bmsdata);

#endif