#include "segment_isospi_recovery.h"
#include "isospi_recovery_common_config.h"
#include "adi6830_interation.h"
#include "segment.h"
#include "bms_config.h"
#include "can_messages_tx.h"
#include "timer.h"
#include "state_machine.h"

/**
 * @brief Break detect threshold.
 *
 * PEC errors > this value in the accumulation window indicate a break.
 */
#define SEGMENT_ISOSPI_PEC_ERROR_THRESHOLD (25U)

/**
 * @brief Threshold for accumulation timer.
 *
 * Set just above the PEC error sum noise level per cycle,
 * so random noise doesn’t start the accumulation window.
 */
#define SEGMENT_ISOSPI_PEC_ACCUM_START_THRESH (5U)

/**
 * @brief Accumulation window (ms).
 *
 * For accumulation, the PEC sum updates at the ADBMS system-wide sample rate
 * defined in bmsConfig.h.
 * Observed PECs/run for chips with break: ~9 (discharge_state), ~20 (charge_state)
 * Current: 2 Hz -> 500 ms * 8 runs = 4000 ms
 */
#define SEGMENT_ISOSPI_ACCUM_PERIOD_MS (4000U)

/**
 * @brief Segment isoSPI break detection status structure.
 *
 * Holds data used for isoSPI break detection and recovery
 */
static segment_isospi_status_t segment_isospi_status;

/**
 * @brief Reset segment PEC error accumulators for all chips.
 *
 * Sets pec_error_sum to 0 for each chip in the provided array.
 *
 * @param chips Pointer to the array of adbms6830 cell_asic structures.
 */
static void reset_all_segment_pec_error_sums(cell_asic chips[NUM_CHIPS])
{
	for (uint8_t i = 0; i < NUM_CHIPS; i++) {
		chips[i].pec_error_sum = 0U;
	}
}

/**
 * @brief Verifies whether segment isoSPI communication recovery succeeded.
 *
 * Performs multiple read cycles and checks if PEC errors have dropped
 * below the acceptable threshold for the chips after the detected break.
 *
 * @param chips Pointer to the array of adbms6830 cell_asic structures.
 * @param start_chip_idx Index of the chip where the break occurred.
 * @return 0 if all chips recovered successfully, 1 otherwise.
 */
static uint8_t segment_isospi_verify_recovery(cell_asic chips[NUM_CHIPS],
					      uint8_t start_chip_idx)
{
	uint8_t result = 0U;

	for (uint8_t i = start_chip_idx; i < NUM_CHIPS; i++) {
		if (chips[i].pec_error_sum >
		    ISOSPI_RECOVERY_VALIDATION_THRESHOLD) {
			printf("[SEGMENT isoSPI] Verification failed at chip %u (PEC: %u)\n\r",
			       i + 1, chips[i].pec_error_sum);
			result = 1U;
		}
	}

	if (result == 0U) {
		printf("[SEGMENT isoSPI] Verification passed\n\r");
	}

	reset_all_segment_pec_error_sums(chips);

	return result;
}

/**
 * @brief Checks for segment isoSPI communication break using PEC error tracking.
 *
 * If a break is detected based on PEC thresholds, the internal state is updated and
 * a non-critical fault is flagged. Resets counters after each check.
 *
 * @param chips Pointer to the array of adbms6830 cell_asic structures.
 * @param state_mach Pointer to the state machine data structure.
 */
static void segment_isospi_detect_break(cell_asic chips[NUM_CHIPS],
					state_machine_t *state_mach)
{
	// Start accumulation timer on a spike in PEC errors
	if (!is_timer_active(&segment_isospi_status.pec_accum_timer)) {
		uint8_t is_active = 0U;
		for (uint8_t i = 0U; (i < NUM_CHIPS) && (is_active == 0U);
		     i++) {
			if (chips[i].pec_error_sum >
			    SEGMENT_ISOSPI_PEC_ACCUM_START_THRESH) {
				is_active = 1U;
			}
		}

		if (is_active == 1U) {
			start_timer(&segment_isospi_status.pec_accum_timer,
				    SEGMENT_ISOSPI_ACCUM_PERIOD_MS);
		} else {
			// Reset PEC sums; PEC rise rate not high enough for a break
			reset_all_segment_pec_error_sums(chips);
		}

	} else {
		// Only proceed if timer has expired
		if (is_timer_expired(&segment_isospi_status.pec_accum_timer)) {
			uint8_t first_faulty_chip_idx = 0U, fault_detected = 0U;

			// Find the first chip that has too many PEC errors
			for (uint8_t chip = 0U; chip < NUM_CHIPS; chip++) {
				if (chips[chip].pec_error_sum >
				    SEGMENT_ISOSPI_PEC_ERROR_THRESHOLD) {
					first_faulty_chip_idx = chip;
					fault_detected = 1U;
					break;
				}
			}

			// Check that all chips after the break also exceed threshold
			if (fault_detected == 1U) {
				uint8_t all_above_thresh = 1U;

				// clang-format off
				for (uint8_t chip = first_faulty_chip_idx; chip < NUM_CHIPS; chip++) {
					if (chips[chip].pec_error_sum <= SEGMENT_ISOSPI_PEC_ERROR_THRESHOLD) {
						all_above_thresh = 0U;
						break;
					}
				}

				if (all_above_thresh == 1U) {

					segment_isospi_status.state = ISOSPI_BREAK_DETECTED;
					segment_isospi_status.break_chip = (uint8_t)(first_faulty_chip_idx + 1U);

					// Sets non-critical isospi break fault
					set_segment_comms_fault(state_mach);

					printf("[SEGMENT isoSPI] Break Detected at Chip %u\n\r", first_faulty_chip_idx + 1U);
				}
				// clang-format on
			}

			// Reset PEC accumulation
			reset_all_segment_pec_error_sums(chips);
		}
	}
}

/**
 * @brief Recover from a segment isoSPI break by switching chips after the break to the secondary line.
 *
 * @param chips Pointer to the array of adbms6830 cell_asic structures.
 * @param hspi SPI handle used for isoSPI communication.
 */
static void segment_isospi_recover_break(cell_asic chips[NUM_CHIPS],
					 SPI_HandleTypeDef *hspi)
{
	uint8_t break_chip_idx =
		(uint8_t)(segment_isospi_status.break_chip - 1U);

	if (break_chip_idx < (NUM_CHIPS - 1)) {
		printf("[SEGMENT isoSPI] Switching chips %u to %u to Line B\n\r",
		       break_chip_idx + 1U, NUM_CHIPS);
	} else {
		printf("[SEGMENT isoSPI] Switching chip %u to Line B\n\r",
		       break_chip_idx + 1U);
	}

	// Switch all chips after the break to use the other isoSPI line
	for (uint8_t i = break_chip_idx; i < NUM_CHIPS; i++) {
		set_segment_chips_isospi_line(&chips[i],
					      ADBMS6830_ISOSPI_LINE_B);
	}

	// Only set COMM_BK if we're not rerouting the entire chain
	if (break_chip_idx > 0U) {
		// Set COMM_BK on both sides of the break
		set_comm_break(&chips[break_chip_idx], COMM_BK_ON);
		set_comm_break(&chips[break_chip_idx - 1], COMM_BK_ON);
	}

	// Restart the segment to apply the new isoSPI line setup and resynchronize
	segment_restart(chips, hspi);

	reset_all_segment_pec_error_sums(chips);
}

bool is_segment_startup_pec_mask_timer_expired(void)
{
	return is_timer_expired(&segment_isospi_status.startup_pec_mask_timer);
}

void segment_isospi_break_detection_init(cell_asic chips[NUM_CHIPS])
{
	// Wait a short time before enabling PEC detection to avoid startup noise
	start_timer(&segment_isospi_status.startup_pec_mask_timer,
		    ISOSPI_RECOVERY_STARTUP_MASK_TIME);
	cancel_timer(&segment_isospi_status.pec_accum_timer);

	segment_isospi_status.state = ISOSPI_STATE_NORMAL;
	segment_isospi_status.break_chip = 0U;
	segment_isospi_status.verification_attempts = 0U;
	segment_isospi_status.recovery_successful = 0U;
	segment_isospi_status.fault_latched = 0U;

	reset_all_segment_pec_error_sums(chips);

	send_segment_isospi_communication_status(
		segment_isospi_status.state, segment_isospi_status.break_chip,
		segment_isospi_status.verification_attempts,
		segment_isospi_status.recovery_successful);
}

void segment_isospi_handle_state(cell_asic chips[NUM_CHIPS],
				 state_machine_t *state_mach,
				 SPI_HandleTypeDef *hspi)
{
	switch (segment_isospi_status.state) {
		case ISOSPI_STATE_NORMAL:
			if (segment_isospi_status.recovery_successful == 0U) {
				segment_isospi_detect_break(chips, state_mach);
			} else {
				// After the first recovery is successful, any further breaks cannot be corrected.
				reset_all_segment_pec_error_sums(chips);
			}
			break;

		case ISOSPI_BREAK_DETECTED:
			send_segment_isospi_communication_status(
				segment_isospi_status.state,
				segment_isospi_status.break_chip,
				segment_isospi_status.verification_attempts,
				segment_isospi_status.recovery_successful);
			printf("[SEGMENT isoSPI] Recovery Started\n\r");
			segment_isospi_recover_break(chips, hspi);
			segment_isospi_status.state = ISOSPI_STATE_VERIFYING;
			break;

		case ISOSPI_STATE_VERIFYING:
			send_segment_isospi_communication_status(
				segment_isospi_status.state,
				segment_isospi_status.break_chip,
				segment_isospi_status.verification_attempts,
				segment_isospi_status.recovery_successful);
			// clang-format off
			if (segment_isospi_status.verification_attempts >= ISOSPI_RECOVERY_VERIFICATION_READS) {
				printf("[SEGMENT isoSPI] Verification failed after max attempts\n\r");
				segment_isospi_status.state = ISOSPI_RECOVERY_FAILED;
			} else {

				// Confirm PEC errors have dropped below acceptable level after recovery
				uint8_t break_chip_idx = (uint8_t)(segment_isospi_status.break_chip - 1U);

				if (segment_isospi_verify_recovery(chips, break_chip_idx) == 0U) {
					printf("[SEGMENT isoSPI] Recovery succeeded\n\r");
					segment_isospi_status.state = ISOSPI_RECOVERY_SUCCESS;
					segment_isospi_status.recovery_successful = 1U;
				}
				segment_isospi_status.verification_attempts++;
			}
			// clang-format on
			break;

		case ISOSPI_RECOVERY_SUCCESS:
			send_segment_isospi_communication_status(
				segment_isospi_status.state,
				segment_isospi_status.break_chip,
				segment_isospi_status.verification_attempts,
				segment_isospi_status.recovery_successful);
			// Clear all faults return to normal operation state
			printf("[SEGMENT isoSPI] Recovery Complete, Fault Cleared\n\r");
			clear_segment_comms_fault(state_mach);
			segment_isospi_status.state = ISOSPI_STATE_NORMAL;
			break;

		case ISOSPI_RECOVERY_FAILED:
			// Run recovery failed fault logic only once to avoid repeating logs and CAN messages
			if (!segment_isospi_status.fault_latched) {
				send_segment_isospi_communication_status(
					segment_isospi_status.state,
					segment_isospi_status.break_chip,
					segment_isospi_status
						.verification_attempts,
					segment_isospi_status
						.recovery_successful);
				printf("[SEGMENT isoSPI] Recovery Failed. Non-critical Fault Latched\n\r");

				segment_isospi_status.recovery_successful = 0U;
				segment_isospi_status.fault_latched = 1U;
			}

			reset_all_segment_pec_error_sums(chips);
			break;

		default:
			printf("[SEGMENT isoSPI] Invalid state: %d\n\r",
			       segment_isospi_status.state);
			break;
	}
}
