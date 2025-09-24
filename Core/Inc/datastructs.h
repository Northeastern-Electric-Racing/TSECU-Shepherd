
#ifndef _DATA_STRUCTS_H
#define _DATA_STRUCTS_H

#include <stdint.h>
#include <stdbool.h>
#include "bms_config.h"s
#include "u_mutex.h"
#include "adBms6830Data.h"

/**
 * @brief Stores critical values for the pack (across all chips), and where that critical value can be found
 */
typedef struct {
	float val;
	uint8_t chipNum;
} crit_chipval_t;

/**
 * @brief Individual chip data
 * @note stores thermistor values, voltage readings, and the discharge status
 */
typedef struct {
	int error_reading;

	/* These are calculated during the analysis of data */

	/* Cell temperature in celsius */
	float cell_temp[NUM_CELLS];
	float cell_resistance[NUM_CELLS];
	float open_cell_voltage[NUM_CELLS];

	float cell_voltages[NUM_CELLS];

	/* True if chip is alpha, False if Chip is Beta */
	bool alpha;

	/* For temperatures of on-board therms. */
	float on_board_temp;

	/// temperature of the die
	float die_temp;
} chipdata_t;

/**
 * @brief Stores critical values for the pack, and where that critical value can be found
 */
typedef struct {
	float val;
	uint8_t chipIndex;
	uint8_t cellNum;
} crit_cellval_t;

typedef enum {
    BOOT,
    READY,
    CHARGING,
    FAULTED,
    NUM_STATES,
} state_t;

/**
 * @brief stores all data related to the bms
 */
typedef struct {
	/* chip_data and chips are parallel arrays. */

	/* Array of data from all chips in the system */
	chipdata_t chip_data[NUM_CHIPS];

	/* Array of structs containing raw data from and configurations for the ADBMS6830 chips */
	cell_asic chips[NUM_CHIPS];

	float pack_current;
	float pack_voltage;
	float pack_ocv;
	float pack_res;

	float cont_DCL;
	float cont_CCL;
	float soc;

	float segment_average_temps[NUM_SEGMENTS];
	/* OCV average voltages */
	float segment_average_volts[NUM_SEGMENTS];
	/* Total voltages for each segment */
	float segment_total_volts[NUM_SEGMENTS];

	// the board temperature
	float internal_temp;

	/**
	 * @brief Note that this is a 32 bit integer, so there are 32 max possible fault codes
	 */
	// uint32_t fault_code;
	uint32_t fault_code_crit;
	uint32_t fault_code_noncrit;

	/* Max, min, and avg thermistor readings */
	crit_cellval_t max_temp;
	crit_cellval_t min_temp;
	float avg_temp;

	// the highest current chip temperature, for faulting
	crit_chipval_t max_chiptemp;

	/* Max and min cell resistances */
	crit_cellval_t max_res;
	crit_cellval_t min_res;

	/* Max, min, and avg voltage of the cells */
	crit_cellval_t max_voltage;
	crit_cellval_t min_voltage;
	float avg_voltage;
	float delt_voltage;

	crit_cellval_t max_ocv;
	crit_cellval_t min_ocv;
	float avg_ocv;
	float delt_ocv;

	// the current discharge configuration the state machine wants
	bool discharge_config[NUM_CHIPS][NUM_CELLS_PER_CHIP];
	// whether balancing should be on, or muted
	bool should_balance;

	/// whether the charger is connected, synonymous with being in the state of CHARGING, and therefore irreversible
	bool is_charger_connected;
	/// whether the state machine has determined its time to charge
	bool is_charging_enabled;

	mutex_t mutex; // TODO: INIT MUTEX

    state_t current_state;
} bms_t;

#endif