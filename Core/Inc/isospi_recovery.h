#ifndef ISOSPI_RECOVERY_H
#define ISOSPI_RECOVERY_H

#include "datastructs.h"
#include <stdbool.h>

/**
 * @brief Checks whether the isoSPI startup PEC mask timer has expired.
 *
 * @return true if the startup mask timer has expired (PECs should be checked), false otherwise.
 */
bool is_startup_pec_mask_timer_expired(void);

/**
 * @brief Initializes ISO SPI break detection timers and state.
 *
 * Should be called during system startup after segment initialization.
 *
 * @param chips Pointer to the array of cell_asic structures.
 */
void isospi_break_detection_init(cell_asic chips[NUM_CHIPS]);

/**
 * @brief Manages the isoSPI communication state machine.
 *
 * Handles logic and transitions for states in @ref isospi_comm_state_t.
 *
 * @param chips Pointer to the array of cell_asic structures.
 * @param state_mach Pointer to the state machine data structure.
 * @param hspi    SPI handle used for isoSPI communication.
 */
void isospi_handle_state(cell_asic chips[NUM_CHIPS],
			 state_machine_t *state_mach, SPI_HandleTypeDef *hspi);

#endif // ISOSPI_RECOVERY_H