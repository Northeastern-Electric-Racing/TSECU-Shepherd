#ifndef CURRENT_LIMIT_ALGO_CONFIG_H
#define CURRENT_LIMIT_ALGO_CONFIG_H

#include "bms_config.h"

// clang-format off

#define TRIGGER_DEBOUNCE_MS           (50UL)
#define QUIET_DEBOUNCE_MS             (50UL)
#define PULSE_ENABLE_MARGIN_A         (0.01f)

/******************************* DCL ****************************************/

#define DCL_TEMP_MIN_C		          MIN_DISCHG_TEMP
#define DCL_TEMP_RAMP_UP_END_C	      (10.0f)
#define DCL_TEMP_RAMP_DOWN_START_C    (50.0f)
#define DCL_TEMP_MAX_C		          MAX_CELL_TEMP

#define DCL_OCV_MIN_V	              MIN_VOLT
#define DCL_OCV_DERATE_THRESH         (3.7f)

#define DCL_MAX_CURRENT_A             MAX_PACK_DISCHG_CURR
#define DCL_MIN_CURRENT_A             (30.0f)

#define DCL_PULSE_PERCENT       	  (1.1f)
#define DCL_COOLDOWN_PERCENT          (0.90f)

#define DCL_MAX_PULSE_CURRENT_A       (DCL_MAX_CURRENT_A * DCL_PULSE_PERCENT)
#define DCL_COOLDOWN_CURRENT_A        (DCL_MAX_CURRENT_A * DCL_COOLDOWN_PERCENT)

#define DCL_PULSE_DURATION_MS	      (3000UL)
#define DCL_COOLDOWN_DURATION_MS	  (10000UL)

#define DCL_TRIGGER_HYST_A	          (2.0f)

/******************************* CCL ****************************************/

#define CCL_TEMP_MIN_C		          MIN_CHG_TEMP
#define CCL_TEMP_RAMP_UP_END_C	      (10.0f)
#define CCL_TEMP_RAMP_DOWN_START_C    (50.0f)
#define CCL_TEMP_MAX_C		          MAX_CELL_TEMP

#define CCL_OCV_MAX_V	              MAX_CHARGE_VOLT
#define CCL_OCV_DERATE_THRESH         (4.0f)

#define CCL_MAX_CURRENT_A             MAX_CHG_CURR
#define CCL_MIN_CURRENT_A             (0.0f)

#define CCL_PULSE_PERCENT       	  (1.1f)
#define CCL_COOLDOWN_PERCENT          (0.90f)

#define CCL_MAX_PULSE_CURRENT_A       (CCL_MAX_CURRENT_A * CCL_PULSE_PERCENT)
#define CCL_COOLDOWN_CURRENT_A        (CCL_MAX_CURRENT_A * CCL_COOLDOWN_PERCENT)

#define CCL_PULSE_DURATION_MS	      (3000UL)
#define CCL_COOLDOWN_DURATION_MS	  (10000UL)

#define CCL_TRIGGER_HYST_A	          (1.0f)

// clang-format on

#endif // CURRENT_LIMIT_ALGO_CONFIG_H