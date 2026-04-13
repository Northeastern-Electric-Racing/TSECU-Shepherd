#ifndef SEGMENT_ISOSPI_RECOVERY_H
#define SEGMENT_ISOSPI_RECOVERY_H

#include "main.h"
#include "datastructs.h"
#include <stdbool.h>

/**
 * @brief Checks whether the segment isoSPI startup PEC mask timer has expired.
 *
 * @return true if the startup mask timer has expired (PECs should be checked), false otherwise.
 */
bool is_segment_startup_pec_mask_timer_expired(void);

/**
 * @brief Initializes segment isoSPI break detection timers and state.
 *
 * Should be called during system startup after segment initialization.
 *
 * @param chips Pointer to the array of adbms6830 cell_asic structures.
 */
void segment_isospi_break_detection_init(cell_asic chips[NUM_CHIPS]);

/**
 * @brief Manages the segment isoSPI communication state machine.
 *
 * Handles logic and transitions for states in @ref isospi_comm_state_t.
 *
 * @param chips Pointer to the array of adbms6830 cell_asic structures.
 * @param state_mach Pointer to the state machine data structure.
 * @param hspi    SPI handle used for isoSPI communication.
 */
void segment_isospi_handle_state(cell_asic chips[NUM_CHIPS],
				 state_machine_t *state_mach,
				 SPI_HandleTypeDef *hspi);

#endif // SEGMENT_ISOSPI_RECOVERY_H