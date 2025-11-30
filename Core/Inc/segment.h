#ifndef _SEGMENT_H
#define _SEGMENT_H

#include "bms_config.h"
#include "stm32h5xx.h"
#include "adBms6830Data.h"
#include <stdbool.h>

/**
 * @brief Initialize chips with default values.
 * 
 */
void segment_init(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi);
/**
 * @brief Stop discharge quickly 
 */
void segment_mute(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi);
/**
 * @brief Start discharge again (inherits config)
 */
void segment_unmute(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi);

/**
 * @brief Freeze result registers 
 */
void segment_snap(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi);
/**
 * @brief Unfreeze result registers 
 */
void segment_unsnap(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi);

/**
 * @brief Pulls all cell data from the segments and returns all cell data.  For drive mode.
 */
void segment_retrieve_active_data(cell_asic chips[NUM_CHIPS],
				  SPI_HandleTypeDef *hspi);
/**
 * @brief Pulls all cell data from the segments and returns all cell data.  For charge mode.
 */
void segment_retrieve_charging_data(cell_asic chips[NUM_CHIPS],
				    SPI_HandleTypeDef *hspi);

/**
 * @brief Fetch extra data for segment
 */
void segment_retrieve_debug_data(cell_asic chips[NUM_CHIPS],
				 SPI_HandleTypeDef *hspi);

/**
 * @brief Disables balancing for all cells.  Will also clear balancing setting.
 */
void segment_disable_balancing(cell_asic chips[NUM_CHIPS],
			       SPI_HandleTypeDef *hspi);

/**
 * @brief Enable balancing (still need to configure it).  Identical to segment_unmute
 */
void segment_enable_balancing(cell_asic chips[NUM_CHIPS],
			      SPI_HandleTypeDef *hspi);

void segment_manual_balancing(cell_asic chips[NUM_CHIPS],
			      SPI_HandleTypeDef *hspi);

/**
 * @brief Configure which cells should discharge, and send configuration to ICs.  Does not enable the actual balancing
 * 
 * @param discharge_config Array containing the discharge configuration. true = discharge, false = do not discharge.
 */
void segment_configure_balancing(
	cell_asic chips[NUM_CHIPS],
	bool discharge_config[NUM_CHIPS][NUM_CELLS_PER_CHIP],
	SPI_HandleTypeDef *hspi);

/**
 * @brief Returns if any cells are balancing. Must read back config register B and PWM registers.
 * 
 * Checks both the DCC full discharge bits and the PWM bits.  Does not check if the cell is in thermal shutdown or muted.
 * 
 * @param chips Array of ADBMS6830 chips.
 * @return true if balancing, false otherwise
 */
bool segment_is_balancing(cell_asic chips[NUM_CHIPS]);

/**
 * @brief Reset, then wake, then re-configure all chips
 * 
 * @param bmsdata 
 */
void segment_restart(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi);

/**
 * @brief Read the serial ID of the chip.
 * 
 * @param chips Array of chips to read.
 */
void read_serial_id(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi);

#endif