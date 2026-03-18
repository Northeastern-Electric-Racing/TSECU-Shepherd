#include "soc.h"
#include "u_tx_general.h"
#include "tx_api.h"
#include "c_utils.h"

#define FULL_CAPACITY_AH \
	5.0f // datasheet specified 5000 mAh capacity for the Molicel P50Bs

#define MS_TO_HOURS(ms) ((ms) / 3600000.0f) // Convert milliseconds to hours

/**
 * @brief Flag to initialize SoC from OCV.
 *
 * Set at startup or when a new OCV-based SoC is needed.
 */
static bool soc_reinit_request = true;

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
	// clang-format off
	float soc = ((((-0.0179218f * ocv + 0.0830236f) * ocv
					+ 0.905277f) * ocv
					- 7.43367f) * ocv
					+ 18.7306f) * ocv
					- 16.0039f;
	// clang-format on

	return soc;
}

void init_soc(void)
{
	soc_reinit_request = true;
}

void update_soc(analyzer_t *analyzer, hv_plate_t *hv_plate)
{
	static float prev_time = 0.0f;

	float min_ocv = analyzer->min_ocv.val;
	bool ocv_valid;

	/* Check if OCV is within valid range */
	if ((min_ocv >= MIN_VOLT) && (min_ocv <= MAX_VOLT)) {
		ocv_valid = true;
	} else {
		ocv_valid = false;
	}

	/**
	 * Initialize or reinitialize SoC from OCV
	 * Used when current data is unreliable/unavailable
	 * (e.g., IsoSPI break) to get a fresh SoC reference.
	 */
	if (soc_reinit_request == true) {
		if (ocv_valid == true) {
			float soc = get_soc_from_ocv(min_ocv);

			/* Limit upper bound */
			if (soc > 1.0f) {
				soc = 1.0f;
			}

			analyzer->soc = soc;

			/* Reset time reference for integration */
			prev_time = TICKS_TO_MS(tx_time_get());

			/* Clear request after successful initialization */
			soc_reinit_request = false;
		}

	} else {
		float curr_time = TICKS_TO_MS(tx_time_get());
		float delta_time = MS_TO_HOURS(curr_time - prev_time);
		float current = hv_plate->pack_current;

		// Coulomb Counting
		// SoC(t) = SoC(t-1) + I(t)/Qn * (t1 - t0)
		float soc = analyzer->soc -
			    (current * delta_time) / FULL_CAPACITY_AH;

		/* Enforce bounds */
		if (soc > 1.0f) {
			soc = 1.0f;
		} else if (soc < 0.0f) {
			soc = 0.0f;
		}

		analyzer->soc = soc;
		prev_time = curr_time;
	}
}