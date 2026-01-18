#ifndef DCL_H
#define DCL_H

#include "datastructs.h"

/**
 * @brief Initializes discharge current limit (DCL) algorithms timers and state
 *
 * Should be called once during system startup in the bms algorithms task, 
 * may need additional calls during isoSPI recovery. 
 */
void dcl_init(void);

/**
 * @brief Calculates the continuous discharge current limit.
 *
 * Computes the discharge current limit from cell temperature and cell voltage.
 * When operating in the safe region, a time-limited discharge pulse and
 * subsequent cooldown may be applied.
 *
 * @param analyzer   Analyzer data containing cell and pack measurements.
 * @param hv_plate   High-voltage plate data used for DCL pulse evaluation.
 * @param bms_algos  Output structure where the computed DCL is stored.
 */
void calc_dcl(const analyzer_t *const analyzer,
	      const hv_plate_t *const hv_plate, bms_algos_t *const bms_algos);

#endif // DCL_H
