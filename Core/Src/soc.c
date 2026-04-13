#include "soc.h"
#include "u_tx_general.h"
#include "tx_api.h"
#include "c_utils.h"
#include "datastructs.h"

/**
 * @brief Nominal pack capacity used for coulomb counting integration.
 *
 * Datasheet specified 5000 mAh capacity for the Molicel P50B cell.
 */
#define FULL_CAPACITY_AH (5.0f)

/**
 * @brief Current hysteresis threshold for coulomb counting
 */
#define CURRENT_HYST_A (0.01f)

/**
 * @brief Convert elapsed time from milliseconds to hours.
 *
 * @param ms Time in milliseconds
 *
 * @return Time in hours
 */
#define MS_TO_HOURS(ms) ((ms) / 3600000.0f)

/**
 * @brief SoC estimator runtime data.
 */
soc_data_t soc_data;

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
	const float soc = ((((-0.0179218f * ocv + 0.0830236f) * ocv
					      + 0.905277f) * ocv
					      - 7.43367f) * ocv
						  + 18.7306f) * ocv
						  - 16.0039f;
	// clang-format on

	return soc;
}

/**
 * @brief Compute pack SoC drift from OCV-based estimate.
 *
 * @param analyzer Analyzer containing pack SoC and OCV data.
 */
static void calculate_soc_drift(const analyzer_t *const analyzer)
{
	const float min_ocv = analyzer->min_ocv.val;
	const float current_soc_from_ocv = get_soc_from_ocv(min_ocv);
	const float current_soc_coulomb_counting = analyzer->soc;

	soc_data.soc_drift =
		current_soc_coulomb_counting - current_soc_from_ocv;
}

void soc_init(void)
{
	soc_data.soc_reinit_request = false;
	soc_data.soc_drift = 0.0f;
	soc_data.prev_time = 0U;
	soc_data.soc_state = SOC_STATE_INIT_FROM_OCV;
}

void soc_request_reinit_from_ocv(void)
{
	soc_data.soc_reinit_request = true;
}

void soc_handle_state(analyzer_t *const analyzer,
		      const hv_plate_t *const hv_plate)
{
	const float min_ocv = analyzer->min_ocv.val;
	const float pack_current = hv_plate->pack_current;

	/**
	 * Initialize or reinitialize SoC from OCV
	 * Used when current data is unreliable/unavailable
	 * (e.g., IsoSPI break) to get a fresh SoC reference.
	 */
	if (soc_data.soc_reinit_request == true) {
		soc_data.soc_state = SOC_STATE_INIT_FROM_OCV;
	}

	switch (soc_data.soc_state) {
		case SOC_STATE_INIT_FROM_OCV: {
			const bool is_ocv_valid = (min_ocv >= MIN_VOLT) &&
						  (min_ocv <= MAX_VOLT);

			if (is_ocv_valid) {
				float soc_from_ocv = get_soc_from_ocv(min_ocv);

				if (soc_from_ocv > 1.0f) {
					soc_from_ocv = 1.0f;
				}

				analyzer->soc = soc_from_ocv;

				soc_data.prev_time = tx_time_get();

				soc_data.soc_reinit_request = false;

				soc_data.soc_state = SOC_STATE_COULOMB_COUNTING;
			}

			break;
		}

		case SOC_STATE_COULOMB_COUNTING: {
			const bool current_valid_for_cc =
				(pack_current > CURRENT_HYST_A) ||
				(pack_current < -CURRENT_HYST_A);

			if (current_valid_for_cc) {
				const float curr_time = tx_time_get();

				const float delta_time = MS_TO_HOURS(
					curr_time - soc_data.prev_time);

				// Coulomb Counting
				// SoC(t) = SoC(t-1) + I(t)/Qn * (t1 - t0)
				float soc = analyzer->soc -
					    (pack_current * delta_time) /
						    FULL_CAPACITY_AH;

				if (soc > 1.0f) {
					soc = 1.0f;
				} else if (soc < 0.0f) {
					soc = 0.0f;
				}

				analyzer->soc = soc;

				soc_data.prev_time = curr_time;

				calculate_soc_drift(analyzer);
			} else {
				// Keep time reference aligned while inside hysteresis band
				soc_data.prev_time = tx_time_get();

				// TODO: Implement SOC_STATE_DRIFT_CORRECTION state and transition.
			}

			break;
		}

		default: {
			soc_data.soc_state = SOC_STATE_INIT_FROM_OCV;
			break;
		}
	}
}

float get_soc_drift(void)
{
	return soc_data.soc_drift;
}