#ifndef CURRENT_LIMIT_ALGO_CONFIG_H
#define CURRENT_LIMIT_ALGO_CONFIG_H

// clang-format off

/******************************* COMMON *******************************/

#define TRIGGER_DEBOUNCE_MS           (100UL)
#define QUIET_DEBOUNCE_MS             (100UL)
#define PULSE_ENABLE_MARGIN_A         (0.01f)

/******************************* DCL **********************************/

#define DCL_TEMP_MIN_C                   (0.0f)
#define DCL_TEMP_RAMP_UP_END_C           (10.0f)
#define DCL_TEMP_RAMP_DOWN_START_C       (50.0f)
#define DCL_TEMP_MAX_C                   (60.0f)
#define DCL_OCV_MIN_V                    (3.0f)
#define DCL_OCV_DERATE_THRESH            (3.7f)
#define DCL_MAX_CURRENT_A                (180.0f)
#define DCL_MIN_CURRENT_A                (30.0f)
#define DCL_PULSE_PERCENT                (1.1f)
#define DCL_COOLDOWN_PERCENT             (0.9f)
#define DCL_PULSE_DURATION_MS            (2000UL)
#define DCL_COOLDOWN_DURATION_MS         (1000UL)

#define DCL_MAX_PULSE_CURRENT_A       (DCL_MAX_CURRENT_A * DCL_PULSE_PERCENT)
#define DCL_COOLDOWN_CURRENT_A        (DCL_MAX_CURRENT_A * DCL_COOLDOWN_PERCENT)

/******************************* CCL **********************************/

#define CCL_TEMP_MIN_C                   (0.0f)
#define CCL_TEMP_RAMP_UP_END_C           (10.0f)
#define CCL_TEMP_RAMP_DOWN_START_C       (50.0f)
#define CCL_TEMP_MAX_C                   (60.0f)
#define CCL_OCV_MAX_V                    (4.2f)
#define CCL_OCV_DERATE_THRESH            (4.0f)
#define CCL_MAX_CURRENT_A                (75.0f)
#define CCL_MIN_CURRENT_A                (0.0f)
#define CCL_PULSE_PERCENT                (1.1f)
#define CCL_COOLDOWN_PERCENT             (0.9f)
#define CCL_PULSE_DURATION_MS            (2000UL)
#define CCL_COOLDOWN_DURATION_MS         (1000UL)

#define CCL_MAX_PULSE_CURRENT_A       (CCL_MAX_CURRENT_A * CCL_PULSE_PERCENT)
#define CCL_COOLDOWN_CURRENT_A        (CCL_MAX_CURRENT_A * CCL_COOLDOWN_PERCENT)

// clang-format on

#endif // CURRENT_LIMIT_ALGO_CONFIG_H