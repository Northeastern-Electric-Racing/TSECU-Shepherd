#ifndef CCL_H
#define CCL_H

#include "datastructs.h"

/**
 * @brief Initialize the charge current limit (CCL) pulse controller.
 *
 * Initializes internal state and timers for the CCL pulse and cooldown
 * mechanism. This function should be called once at system startup and
 * may be re-invoked during recovery events to reset internal state.
 * 
 * @param cd_mode  Cooldown behavior mode for pulse operation
 */
void ccl_init(pulse_cooldown_mode_t cd_mode);

/**
 * @brief Compute instantaneous charge current limit.
 *
 * Determines the most restrictive charge current limit based on cell
 * temperature and cell open-circuit voltage.
 *
 * @param curr_lim_inputs  Current limit algorithm inputs
 * @param bms_algos        Output structure updated with the instantaneous CCL
 *
 * @return Instantaneous charge current limit (A)
 */
void ccl_calc_inst_limit(
	const current_limit_algo_inputs_t *const curr_lim_inputs,
	bms_algos_t *const bms_algos);

/**
 * @brief Compute the continuous charge current limit.
 *
 * Applies pulse and cooldown behavior to the charge current limit.
 * When pulse operation is not permitted, the limit defaults to the
 * previously computed instantaneous charge current limit.
 *
 * @param pack_current  Pack charge current (A)
 * @param bms_algos     Output structure updated with the applied CCL
 */
void ccl_calc_cont_limit(float pack_current, bms_algos_t *const bms_algos);

#endif // CCL_H