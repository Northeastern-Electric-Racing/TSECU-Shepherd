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
 * @param cooldown_mode  Cooldown behavior mode for pulse operation
 */
void dcl_init(pulse_cooldown_mode_t cooldown_mode);

/**
 * @brief Compute instantaneous discharge current limit.
 *
 * Determines the most restrictive discharge current limit based on cell
 * temperature and cell open-circuit voltage.
 *
 * @param curr_lim_inputs  Current limit algorithm inputs
 * @param bms_algos        Output structure updated with the instantaneous DCL
 *
 * @return Instantaneous discharge current limit (A)
 */
void dcl_calc_inst_limit(current_limit_algo_inputs_t curr_lim_inputs,
			 bms_algos_t *const bms_algos);

/**
 * @brief Compute the continuous discharge current limit.
 *
 * Applies pulse and cooldown behavior to the discharge current limit.
 * When pulse operation is not permitted, the limit defaults to the
 * previously computed instantaneous discharge current limit.
 *
 * @param pack_current  Pack discharge current (A)
 * @param bms_algos     Output structure updated with the applied DCL
 */
void dcl_calc_cont_limit(float pack_current, bms_algos_t *const bms_algos);

#endif // DCL_H