#ifndef HV_PLATE_ISOSPI_RECOVERY_H
#define HV_PLATE_ISOSPI_RECOVERY_H

#include "main.h"
#include "datastructs.h"
#include <stdbool.h>

/**
 * @brief Checks whether the hv plate isoSPI startup PEC mask timer has expired.
 *
 * @return true if the startup mask timer has expired (PECs should be checked), false otherwise.
 */
bool is_hv_plate_startup_pec_mask_timer_expired(void);

/**
 * @brief Initializes hv plate isoSPI break detection timers and state.
 *
 * Should be called during system startup after hv plate initialization.
 *
 * @param ic Pointer to the adbms2950 data structure.
 */
void hv_plate_isospi_break_detection_init(cell_asic_2950 *ic);

/**
 * @brief Manages the hv plate isoSPI communication state machine.
 *
 * Handles logic and transitions for states in @ref isospi_comm_state_t.
 *
 * @param hv_plate Pointer to the hv plate data structure.
 * @param state_mach Pointer to the state machine data structure.
 */
void hv_plate_isospi_handle_state(hv_plate_t *hv_plate,
				  state_machine_t *state_mach);

#endif // HV_PLATE_ISOSPI_RECOVERY_H