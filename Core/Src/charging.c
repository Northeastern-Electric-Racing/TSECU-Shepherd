
#include "charging.h"
#include "adBms6830Data.h"
#include "bms_config.h"
#include "c_utils.h"
#include "analyzer.h"

/// @brief A struct to hold the original float value and the index originally,
/// as that holds meaning
typedef struct {
	float val;
	size_t idex;
} val_idexed_t;

static _Atomic uint8_t duty_cycle_req = 0;

void pwm_duty_cycle_set(uint8_t duty_cycle_req_get) {
    duty_cycle_req = duty_cycle_req_get;
}

float pwm_duty_cycle_setting_get(void) {
	float duty_cycle = 0.0f;

	switch (pwm_duty_cycle_get()) {
		case PWM_0_0_PCT:
			duty_cycle = 0.0f;
			break;
		case PWM_6_6_PCT:
			duty_cycle = 6.6f;
			break;
		case PWM_13_2_PCT:
			duty_cycle = 13.2f;
			break;
		case PWM_19_8_PCT:
			duty_cycle = 19.8f;
			break;
		case PWM_26_4_PCT:
			duty_cycle = 26.4f;
			break;
		case PWM_33_0_PCT:
			duty_cycle = 33.0f;
			break;
		case PWM_39_6_PCT:
			duty_cycle = 39.6f;
			break;
		case PWM_46_2_PCT:
			duty_cycle = 46.2f;
			break;
		case PWM_52_8_PCT:
			duty_cycle = 52.8f;
			break;
		case PWM_59_4_PCT:
			duty_cycle = 59.4f;
			break;
		case PWM_66_0_PCT:
			duty_cycle = 66.0f;
			break;
		case PWM_72_6_PCT:
			duty_cycle = 72.6f;
			break;
		case PWM_79_2_PCT:
			duty_cycle = 79.2f;
			break;
		case PWM_85_8_PCT:
			duty_cycle = 85.8f;
			break;
		case PWM_92_4_PCT:
			duty_cycle = 92.4f;
			break;
		case PWM_100_0_PCT:
			duty_cycle = 100.0f;
			break;
		default:
			duty_cycle = 0.0f;
			break;
	}

	return duty_cycle;
}

PWM_DUTY pwm_duty_cycle_get() {
    switch (duty_cycle_req) {
        case 1 ... 6:
            return PWM_6_6_PCT;
        case 7 ... 13:
            return PWM_13_2_PCT;
        case 14 ... 19:
            return PWM_19_8_PCT;
        case 20 ... 26:
            return PWM_26_4_PCT;
        case 27 ... 33:
            return PWM_33_0_PCT;
        case 34 ... 39:
            return PWM_39_6_PCT;
        case 40 ... 46:
            return PWM_46_2_PCT;
        case 47 ... 52:
            return PWM_52_8_PCT;
        case 53 ... 59:
            return PWM_59_4_PCT;
        case 60 ... 66:
            return PWM_66_0_PCT;
        case 67 ... 72:
            return PWM_72_6_PCT;
        case 73 ... 79:
            return PWM_79_2_PCT;
        case 80 ... 85:
            return PWM_85_8_PCT;
        case 86 ... 92:
            return PWM_92_4_PCT;
        case 93 ... 100:
            return PWM_100_0_PCT;
        default:
            return PWM_0_0_PCT;
    }
}

/**
 * @brief selection sorts ocv into structs that remember values
 * @param arr
 * @param n count
 */
static void
chipsSelectionSort(analyzer_t *analyzer,
		   val_idexed_t replaced_val[NUM_CHIPS][NUM_CELLS_PER_CHIP])
{
	for (size_t chip = 0; chip < NUM_CHIPS; chip++) {
		// first fill the outer row
		for (int i = 0; i < NUM_CELLS_PER_CHIP; i++) {
			replaced_val[chip][i] = (val_idexed_t){
				.idex = i,
				.val = get_chip_data(analyzer, chip)
					       ->open_cell_voltage[i]
			};
		}

		// now actually sort it
		for (size_t i = 0; i < NUM_CELLS_PER_CHIP - 1; i++) {
			// Assume the current position holds
			// the maximum element
			size_t max_idx = i;

			// Iterate through the unsorted portion
			// to find the actual maximum
			for (size_t j = i + 1; j < NUM_CELLS_PER_CHIP; j++) {
				if (replaced_val[chip][j].val >
				    replaced_val[chip][max_idx].val) {
					// Update max_idx if a smaller element is found
					max_idx = j;
				}
			}

			// Move maximum element to its
			// correct position
			val_idexed_t temp = replaced_val[chip][i];
			replaced_val[chip][i] = replaced_val[chip][max_idx];
			replaced_val[chip][max_idx] = temp;
		}
	}
}

/* Send cell balancing config to the segments */
bool handle_balance_cells(analyzer_t *analyzer, acc_data_t *acc_data)
{
	// the maximum number of cells to balance per chip, usually tuned for thermal
	// reasons
	static const int MAX_BAL_CHIP = 7;
	bool balancing_needed = false;

	// the low cell, eventually they all must get there
	float low = analyzer->min_ocv.val;
	val_idexed_t new_ocv_map[NUM_CHIPS][NUM_CELLS_PER_CHIP] = { 0 };

	// first, sort and cleanup everything
	chipsSelectionSort(analyzer, new_ocv_map);

	PWM_DUTY duty_cycle = pwm_duty_cycle_get();

	/* Balance all cells above the threshold, using the sorted ocv map values but
   * preserve the indexes*/
	for (size_t chip = 0; chip < NUM_CHIPS; chip++) {
		// ONLY iterate to MAX_BAL or the number of cells, whatever is lower.
		// this is OK because they are sorted greatest to least in delta
		for (size_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
		    if (cell >= MAX_BAL_CHIP) {
				acc_data->discharge_config[chip][new_ocv_map[chip][cell].idex] = PWM_0_0_PCT;
				continue;
			}
			/* Check if cell voltage is above (low + threshold) */
			if (new_ocv_map[chip][cell].val >
			    (low + BALANCE_DELTA_V)) {
				/* Balance cell */
				acc_data->discharge_config // TODO: Mutex
					[chip][new_ocv_map[chip][cell].idex] = duty_cycle;
				if (duty_cycle > PWM_0_0_PCT) {
					balancing_needed = true;
				}
			} else {
				/* Do not balance cell */
				acc_data->discharge_config
					[chip][new_ocv_map[chip][cell].idex] = PWM_0_0_PCT;
			}
		}
	}

	return balancing_needed;
}
