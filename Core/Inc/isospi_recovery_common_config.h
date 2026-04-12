#ifndef ISOSPI_RECOVERY_COMMON_CONFIG_H
#define ISOSPI_RECOVERY_COMMON_CONFIG_H

/**
 * @brief Max accumulated PECs
 */
#define ISOSPI_RECOVERY_MAX_PEC_ERROR_ACCUM (100U)

/**
 * @brief Validation threshold during recovery.
 *
 * Maximum PEC errors allowed while verifying recovery success.
 * Lower this value for stricter validation.
 */
#define ISOSPI_RECOVERY_VALIDATION_THRESHOLD (3U)

/**
 * @brief Startup mask time (ms).
 *
 * Time to ignore PECs after init to avoid false detections.
 */
#define ISOSPI_RECOVERY_STARTUP_MASK_TIME (1500U)

/**
 * @brief Maximum number of verification read attempts after recovery.
 *
 * Recovery passes if any attempt succeeds; fails if all attempts fail.
 */
#define ISOSPI_RECOVERY_VERIFICATION_READS (5U)

#endif // ISOSPI_RECOVERY_COMMON_CONFIG_H