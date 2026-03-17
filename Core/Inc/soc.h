
#ifndef _SOC_H
#define _SOC_H

#include "datastructs.h"

/**
 * @brief Initialize SoC module.
 */
void init_soc(void);

/**
 * @brief Update state of charge using coulomb counting.
 *
 * @param analyzer pointer to analyzer data struct
 * @param hv_plate pointer to hv plate data struct
 */
void update_soc(analyzer_t *analyzer, hv_plate_t *hv_plate);

#endif // _SOC_H