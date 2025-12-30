
#include "soc.h"
#include "stm32xx_hal.h"

#define FULL_CAPACITY_AH 5.0f // datasheet specified 5000 mAh capacity for the Molicel P50Bs

static float _get_initial_soc(analyzer_t *analyzer, hv_plate_t *hv_plate)
{
    return 1.0f; // assume full at startup
    // TODO: Create look-up table for initial SoC based on battery profiling
}

void update_soc(analyzer_t *analyzer, hv_plate_t *hv_plate)
{   
    static bool is_first_run = true;
    static float prev_time = 0;

    // Lookup Table for initial SoC
    if (is_first_run) {
        analyzer->soc = _get_initial_soc(analyzer, hv_plate);
        prev_time = (float)HAL_GetTick(); // in milliseconds
        is_first_run = false;
        return;
    }

    // Coulomb Counting
    // SoC(t) = SoC(t-1) + I(t)/Qn * (t1 - t0)

    float last_soc = analyzer->soc;
    float curr_time = (float)HAL_GetTick(); // in milliseconds
    float delta_time = (curr_time - prev_time) / 3600000.0; // convert to hours 

    float current = hv_plate->pack_current; // in Amperes

    float soc = last_soc + (current * delta_time) / FULL_CAPACITY_AH;
    if (soc > 1.0f) {
        soc = 1.0f;
    } else if (soc < 0.0f) {
        soc = 0.0f;
    }

    analyzer->soc = soc;
    prev_time = curr_time;
}
