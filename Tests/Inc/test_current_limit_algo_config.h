#ifndef TEST_CURRENT_LIMIT_ALGO_CONFIG_H
#define TEST_CURRENT_LIMIT_ALGO_CONFIG_H

/******************************* DCL **********************************/

#define DCL_MAX_CURRENT_A                (135.0f)
#define DCL_MIN_CURRENT_A                (30.0f)
#define DCL_PULSE_PERCENT                (1.1f)
#define DCL_COOLDOWN_PERCENT             (0.9f)

#define DCL_MAX_PULSE_CURRENT_A       (DCL_MAX_CURRENT_A * DCL_PULSE_PERCENT)
#define DCL_COOLDOWN_CURRENT_A        (DCL_MAX_CURRENT_A * DCL_COOLDOWN_PERCENT)

/******************************* CCL **********************************/

#define CCL_MAX_CURRENT_A                (30.0f)
#define CCL_MIN_CURRENT_A                (0.0f)
#define CCL_PULSE_PERCENT                (1.1f)
#define CCL_COOLDOWN_PERCENT             (0.9f)

#define CCL_MAX_PULSE_CURRENT_A       (CCL_MAX_CURRENT_A * CCL_PULSE_PERCENT)
#define CCL_COOLDOWN_CURRENT_A        (CCL_MAX_CURRENT_A * CCL_COOLDOWN_PERCENT)

#endif // TEST_CURRENT_LIMIT_ALGO_CONFIG_H