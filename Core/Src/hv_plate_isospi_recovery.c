#include "hv_plate_isospi_recovery.h"
#include "isospi_recovery_common_config.h"
#include "adi2950_interaction.h"
#include "hv_plate.h"
#include "can_messages_tx.h"
#include "timer.h"
#include "state_machine.h"

/**
 * @brief Break detect threshold.
 *
 * PEC errors > this value in the accumulation window indicate a break.
 */
#define HV_PLATE_ISOSPI_PEC_ERROR_THRESHOLD (25U)

/**
 * @brief Threshold for accumulation timer.
 *
 * Set just above the PEC error sum noise level per cycle,
 * so random noise doesn’t start the accumulation window.
 */
#define HV_PLATE_ISOSPI_PEC_ACCUM_START_THRESH (5U)

/**
 * @brief Accumulation window (ms).
 *
 * For accumulation, the PEC sum updates at the ADBMS system-wide sample rate
 * defined in bmsConfig.h.
 * Observed PECs/run for chips with break: ~9 (discharge_state), ~20 (charge_state)
 * Current: 2 Hz -> 500 ms * 8 runs = 4000 ms
 */
#define HV_PLATE_ISOSPI_ACCUM_PERIOD_MS (4000U)

/**
 * @brief HV plate isoSPI break detection status structure.
 *
 * Holds data used for isoSPI break detection and recovery
 */
static hv_plate_isospi_status_t hv_plate_isospi_status;

/**
 * @brief Reset hv plate PEC error accumulators for all chips.
 *
 * Sets pec_error_sum to 0 for each chip in the provided array.
 *
 * @param ic Pointer to the adbms2950 data structure.
 */
static void reset_hv_plate_pec_error_sum(cell_asic_2950 *ic)
{
	ic->pec_error_sum = 0U;
}

/**
 * @brief Verifies whether hv plate isoSPI communication recovery succeeded.
 *
 * Performs multiple read cycles and checks if PEC errors have dropped
 * below the acceptable threshold for the chips after the detected break.
 *
 * @param ic Pointer to the adbms2950 data structure.
 * @return 0 if all chips recovered successfully, 1 otherwise.
 */
static uint8_t hv_plate_isospi_verify_recovery(cell_asic_2950 *ic)
{
	uint8_t result = 0U;

	if (ic->pec_error_sum > ISOSPI_RECOVERY_VALIDATION_THRESHOLD) {
		printf("[HV PLATE isoSPI] Line B Verification failed (PEC: %u)\n\r",
		       ic->pec_error_sum);
		result = 1U;
	}

	if (result == 0U) {
		printf("[HV PLATE isoSPI] Line B Verification passed\n\r");
	}

	reset_hv_plate_pec_error_sum(ic);

	return result;
}

/**
 * @brief Checks for hv plate isoSPI communication break using PEC error tracking.
 *
 * If a break is detected based on PEC thresholds, the internal state is updated and
 * a non-critical fault is flagged. Resets counters after each check.
 *
 * @param ic Pointer to the adbms2950 data structure.
 * @param state_mach Pointer to the state machine data structure.
 */
static void hv_plate_isospi_detect_break(cell_asic_2950 *ic,
					 state_machine_t *state_mach)
{
	// Start accumulation timer on a spike in PEC errors
	if (!is_timer_active(&hv_plate_isospi_status.pec_accum_timer)) {
		if (ic->pec_error_sum >
		    HV_PLATE_ISOSPI_PEC_ACCUM_START_THRESH) {
			start_timer(&hv_plate_isospi_status.pec_accum_timer,
				    HV_PLATE_ISOSPI_ACCUM_PERIOD_MS);

		} else {
			// Reset PEC sums; PEC rise rate not high enough for a break
			reset_hv_plate_pec_error_sum(ic);
		}

	} else {
		// Only proceed if timer has expired
		if (is_timer_expired(&hv_plate_isospi_status.pec_accum_timer)) {
			// Detect break if PEC errors exceed threshold
			if (ic->pec_error_sum >
			    HV_PLATE_ISOSPI_PEC_ERROR_THRESHOLD) {
				hv_plate_isospi_status.state =
					ISOSPI_BREAK_DETECTED;

				// Sets non-critical isospi break fault
				set_hv_plate_comms_fault(state_mach);

				printf("[HV PLATE isoSPI] Line A Break Detected\n\r");
			}

			// Reset PEC accumulation
			reset_hv_plate_pec_error_sum(ic);
		}
	}
}

/**
 * @brief Recover from a hv plate isoSPI break by switching chips after the break to the secondary line.
 *
 * @param hv_plate Pointer to the hv plate data structure.
 */
static void hv_plate_isospi_recover_break(hv_plate_t *hv_plate)
{
	printf("[HV PLATE isoSPI] Switching comms to Line B\n\r");

	// Switch communication to the other isoSPI line after break
	set_hv_plate_chips_isospi_line(&hv_plate->ic, ADBMS2950_ISOSPI_LINE_B);

	// Restart hv plate to apply the new isoSPI line setup and resynchronize
	hv_plate_restart(hv_plate);

	reset_hv_plate_pec_error_sum(&hv_plate->ic);
}

bool is_hv_plate_startup_pec_mask_timer_expired(void)
{
	return is_timer_expired(&hv_plate_isospi_status.startup_pec_mask_timer);
}

void hv_plate_isospi_break_detection_init(cell_asic_2950 *ic)
{
	// Wait a short time before enabling PEC detection to avoid startup noise
	start_timer(&hv_plate_isospi_status.startup_pec_mask_timer,
		    ISOSPI_RECOVERY_STARTUP_MASK_TIME);
	cancel_timer(&hv_plate_isospi_status.pec_accum_timer);

	hv_plate_isospi_status.state = ISOSPI_STATE_NORMAL;
	hv_plate_isospi_status.verification_attempts = 0U;
	hv_plate_isospi_status.recovery_successful = 0U;
	hv_plate_isospi_status.fault_latched = 0U;

	reset_hv_plate_pec_error_sum(ic);

	send_hv_plate_isospi_communication_status(
		hv_plate_isospi_status.state,
		hv_plate_isospi_status.verification_attempts,
		hv_plate_isospi_status.recovery_successful);
}

void hv_plate_isospi_handle_state(hv_plate_t *hv_plate,
				  state_machine_t *state_mach)
{
	switch (hv_plate_isospi_status.state) {
		case ISOSPI_STATE_NORMAL:
			if (hv_plate_isospi_status.recovery_successful == 0U) {
				hv_plate_isospi_detect_break(&hv_plate->ic,
							     state_mach);
			} else {
				// After the first recovery is successful, any further breaks cannot be corrected.
				reset_hv_plate_pec_error_sum(&hv_plate->ic);
			}
			break;

		case ISOSPI_BREAK_DETECTED:
			send_hv_plate_isospi_communication_status(
				hv_plate_isospi_status.state,
				hv_plate_isospi_status.verification_attempts,
				hv_plate_isospi_status.recovery_successful);
			printf("[HV PLATE isoSPI] Recovery Started\n\r");
			hv_plate_isospi_recover_break(hv_plate);
			hv_plate_isospi_status.state = ISOSPI_STATE_VERIFYING;
			break;

		case ISOSPI_STATE_VERIFYING:
			send_hv_plate_isospi_communication_status(
				hv_plate_isospi_status.state,
				hv_plate_isospi_status.verification_attempts,
				hv_plate_isospi_status.recovery_successful);
			// clang-format off
			if (hv_plate_isospi_status.verification_attempts >= ISOSPI_RECOVERY_VERIFICATION_READS) {
				printf("[HV_PLATE isoSPI] Verification failed after max attempts\n\r");
				hv_plate_isospi_status.state = ISOSPI_RECOVERY_FAILED;
			} else {

				// Confirm PEC errors have dropped below acceptable level after recovery
				if (hv_plate_isospi_verify_recovery(&hv_plate->ic) == 0U) {
					printf("[HV PLATE isoSPI] Recovery succeeded\n\r");
					hv_plate_isospi_status.state = ISOSPI_RECOVERY_SUCCESS;
					hv_plate_isospi_status.recovery_successful = 1U;
				}
				hv_plate_isospi_status.verification_attempts++;
			}
			// clang-format on
			break;

		case ISOSPI_RECOVERY_SUCCESS:
			send_hv_plate_isospi_communication_status(
				hv_plate_isospi_status.state,
				hv_plate_isospi_status.verification_attempts,
				hv_plate_isospi_status.recovery_successful);
			// Clear all faults return to normal operation state
			printf("[HV PLATE isoSPI] Recovery Complete, Fault Cleared\n\r");
			clear_hv_plate_comms_fault(state_mach);
			hv_plate_isospi_status.state = ISOSPI_STATE_NORMAL;
			break;

		case ISOSPI_RECOVERY_FAILED:
			// Run recovery failed fault logic only once to avoid repeating logs and CAN messages
			if (!hv_plate_isospi_status.fault_latched) {
				send_hv_plate_isospi_communication_status(
					hv_plate_isospi_status.state,
					hv_plate_isospi_status
						.verification_attempts,
					hv_plate_isospi_status
						.recovery_successful);
				printf("[HV PLATE isoSPI] Recovery Failed. Non-critical Fault Latched\n\r");

				hv_plate_isospi_status.recovery_successful = 0U;
				hv_plate_isospi_status.fault_latched = 1U;
			}

			reset_hv_plate_pec_error_sum(&hv_plate->ic);
			break;

		default:
			printf("[HV PLATE isoSPI] Invalid state: %d\n\r",
			       hv_plate_isospi_status.state);
			break;
	}
}
