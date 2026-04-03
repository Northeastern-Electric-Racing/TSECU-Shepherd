
#ifndef _SOC_H
#define _SOC_H

#include "datastructs.h"

/**
 * @brief Initialize SoC module.
 */
void soc_init(void);

/**
 * @brief Request SoC reinitialization from OCV reference.
 */
void soc_request_reinit_from_ocv(void);

/**
 * @brief Execute SoC estimator state machine.
 *
 * @param analyzer Pack analyzer data structure
 * @param hv_plate HV plate measurement structure
 */
void soc_handle_state(analyzer_t *const analyzer,
		      const hv_plate_t *const hv_plate);

/**
 * @brief Get pack SoC drift relative to OCV estimate.
 *
 * @return SoC drift (−1.0 to +1.0)
 */
float get_soc_drift(void);

#endif // _SOC_H