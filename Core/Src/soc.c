
#include "soc.h"
#include "stm32xx_hal.h"
#include "tx_api.h"

#define FULL_CAPACITY_AH \
	5.0f // datasheet specified 5000 mAh capacity for the Molicel P50Bs

typedef struct {
	float min_ocv; // in Volts
	float capacity; // in Ah
} soc_lookup_t;

// clang-format off
const soc_lookup_t SOC_LOOKUP_TABLE[] = {
    {MAX_VOLT, FULL_CAPACITY_AH}, // Max possible capacity at max voltage
    {4.10f, 4.5f},
    {4.00f, 4.0f},
    {3.90f, 3.5f},
    {3.80f, 3.0f},
    {3.70f, 2.5f},
    {3.60f, 2.0f},
    {3.50f, 1.5f},
    {3.40f, 1.0f},
    {3.30f, 0.5f},
    {MIN_VOLT, 0.0f} // Minimum capacity at min voltage
};
// clang-format on

static float _get_initial_soc(analyzer_t *analyzer, hv_plate_t *hv_plate)
{
	float min_ocv = analyzer->min_ocv.val;

	// check for if min OCV has not been initialized yet
	if (min_ocv < MIN_VOLT) {
		return -1;
	} else if (min_ocv > MAX_VOLT) {
		return -1;
	}

	// lookup the capacity from the table
	for (size_t i = 0;
	     i < sizeof(SOC_LOOKUP_TABLE) / sizeof(SOC_LOOKUP_TABLE[0]) - 1;
	     i++) {
		if (min_ocv <= SOC_LOOKUP_TABLE[i].min_ocv &&
		    min_ocv > SOC_LOOKUP_TABLE[i + 1].min_ocv) {
			float cap_high = SOC_LOOKUP_TABLE[i].capacity;
			float cap_low = SOC_LOOKUP_TABLE[i + 1].capacity;
			float volt_high = SOC_LOOKUP_TABLE[i].min_ocv;
			float volt_low = SOC_LOOKUP_TABLE[i + 1].min_ocv;
			// linear interpolation bewteen the two points
			float capacity =
				cap_low + (cap_high - cap_low) *
						  (min_ocv - volt_low) /
						  (volt_high - volt_low);
			return capacity /
			       FULL_CAPACITY_AH; // calc OCV with capacity
		}
	}

	return -1; // invalid OCV reading
}

void update_soc(analyzer_t *analyzer, hv_plate_t *hv_plate)
{
	static bool is_first_run = true;
	static float prev_time = 0;

	// Lookup Table for initial SoC
	if (is_first_run) {
		float initial_soc = _get_initial_soc(analyzer, hv_plate);
		if (initial_soc < 0) {
			return; // invalid OCV reading, cannot initialize SoC
		}
		analyzer->soc = initial_soc;
		prev_time = TICKS_TO_MS(tx_time_get()); // in milliseconds
		is_first_run = false;
		return;
	}

	// Coulomb Counting
	// SoC(t) = SoC(t-1) + I(t)/Qn * (t1 - t0)

	float last_soc = analyzer->soc;
	float curr_time = TICKS_TO_MS(tx_time_get()); // in milliseconds
	float delta_time =
		(curr_time - prev_time) / 3600000.0; // convert to hours

	float current = hv_plate->pack_current; // in Amperes

	// subtracted since discharging current is positive
	float soc = last_soc - (current * delta_time) / FULL_CAPACITY_AH;
	if (soc > 1.0f) {
		soc = 1.0f;
	} else if (soc < 0.0f) {
		soc = 0.0f;
	}

	analyzer->soc = soc;
	prev_time = curr_time;
}
