#include "soc.h"
#include "u_tx_general.h"
#include "tx_api.h"

#define FULL_CAPACITY_AH \
	5.0f // datasheet specified 5000 mAh capacity for the Molicel P50Bs

typedef struct {
	float min_ocv; // in Volts
	float capacity; // in Ah
} soc_lookup_t;

/**
 * @brief Estimate SoC from cell OCV using a polynomial fit.
 *
 * Original polynomial (standard form):
 *
 * SoC(x) =
 *     -0.0179218x^5
 *     +0.0830236x^4
 *     +0.905277x^3
 *     -7.43367x^2
 *     +18.7306x
 *     -16.0039
 *
 * Implemented below in Horner form to reduce multiplications
 * and improve runtime performance.
 *
 * @param ocv Cell open-circuit voltage (V)
 * @return Estimated SoC (0.0 to 1.0)
 */
static float get_soc_from_ocv(float ocv)
{
	float soc;

	// clang-format off
	soc = ((((-0.0179218f * ocv + 0.0830236f) * ocv
	         + 0.905277f) * ocv
	         - 7.43367f) * ocv
	         + 18.7306f) * ocv
	         - 16.0039f;
	// clang-format on

	/* Saturate to valid SOC range */
	if (soc < 0.0f) {
		soc = 0.0f;
	} else if (soc > 1.0f) {
		soc = 1.0f;
	}

	return soc;
}

static float get_initial_soc(analyzer_t *analyzer)
{
	float min_ocv = analyzer->min_ocv.val;

	/* Check for invalid OCV reading */
	if ((min_ocv < MIN_VOLT) || (min_ocv > MAX_VOLT)) {
		return -1.0f;
	}

	return get_soc_from_ocv(min_ocv);
}

void update_soc(analyzer_t *analyzer, hv_plate_t *hv_plate)
{
	static bool is_first_run = true;
	static float prev_time = 0;

	// OCV-SoC curve for initial SoC
	if (is_first_run) {
		float initial_soc = get_initial_soc(analyzer);
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
