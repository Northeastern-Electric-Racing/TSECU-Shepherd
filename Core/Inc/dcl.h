#ifndef DCL_H
#define DCL_H

#include "datastructs.h"

/**
 * @brief Initialize the discharge current limit (DCL) pulse controller.
 *
 * Initializes internal state and timers for the DCL pulse and cooldown
 * mechanism. This function should be called once at system startup and
 * may be re-invoked during recovery events to reset internal state.
 * 
 * @param cd_mode  Cooldown behavior mode for pulse operation
 */
void dcl_init(pulse_cooldown_mode_t cd_mode);

/**
 * @brief Compute the discharge current limit.
 *
 * Calculates the discharge current limit based on the provided operating
 * conditions. When permitted, a time-limited discharge pulse and optional
 * cooldown behavior may be applied.
 *
 * @param curr_lim_inputs  Current limit algorithm inputs
 * @param bms_algos        Output structure updated with the computed DCL
 */
void calc_dcl(current_limit_algo_inputs_t curr_lim_inputs,
	      bms_algos_t *const bms_algos);

#endif // DCL_H
