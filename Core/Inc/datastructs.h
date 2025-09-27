
#ifndef _DATA_STRUCTS_H
#define _DATA_STRUCTS_H

#include <stdint.h>
#include <stdbool.h>
#include "bms_config.h"
#include "u_tx_mutex.h"
#include "adBms6830Data.h"
#include "timer.h"

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

    state_t current_state;
} bms_t;

enum {
	FAULTS_CLEAR = 0x0,

	/* Shepherd BMS faults */
	CELLS_NOT_BALANCING		            = 0x1,
	CELL_VOLTAGE_TOO_HIGH	            = 0x2,
	CELL_VOLTAGE_TOO_LOW	            = 0x4,
	PACK_TOO_HOT			            = 0x8,
	OPEN_WIRING_FAULT		            = 0x10, /* cell tap wire is either weakly connected or not connected */
	INTERNAL_SOFTWARE_FAULT             = 0x20, /* general software fault */
	INTERNAL_THERMAL_ERROR              = 0x40, /* internal hardware fault reulting from too hot of onboard temps */
	INTERNAL_CELL_COMM_FAULT            = 0x80, /* this is due to an invalid CRC from retrieving values */
	CURRENT_SENSOR_FAULT	            = 0x100,
	CHARGE_READING_MISMATCH             = 0x200, /* charge voltage when not supposed to be charging*/
	LOW_CELL_VOLTAGE				    = 0x400, /* voltage of a cell falls below 90 mV */
	WEAK_PACK_FAULT					    = 0x800,
	EXTERNAL_CAN_FAULT				    = 0x1000,
	DISCHARGE_LIMIT_ENFORCEMENT_FAULT   = 0x2000,
	CHARGER_SAFETY_RELAY			    = 0x4000,
	BATTERY_THERMISTOR				    = 0x8000,
	CHARGER_CAN_FAULT				    = 0x10000,
	CHARGE_LIMIT_ENFORCEMENT_FAULT	    = 0x20000,
	DIE_TEMP_MAXIMUM_FAULT       	    = 0x40000,

	MAX_FAULTS = 0x80000000 /* Maximum allowable fault code */
};

/**
 * @brief Represents fault evaluation operators
 */
typedef enum {
	GT, /* fault if {data} greater than {threshold}             */
	LT, /* fault if {data} less than {threshold}                */
	GE, /* fault if {data} greater than or equal to {threshold} */
	LE, /* fault if {data} less than or equal to {threshold}    */
	EQ, /* fault if {data} equal to {threshold}                 */
	NEQ, /* fault if {data} not equal to {threshold}             */
	NOP /* no operation, use for single threshold faults        */

} fault_evalop_t;

/**
 * @brief Represents data to be packaged into a fault evaluation
 */
typedef struct {
	char id[100];
	nertimer_t timer;

	float data_1;
	fault_evalop_t optype_1;
	float lim_1;

	int timeout;
	int code;

	fault_evalop_t optype_2;
	float data_2;
	float lim_2;

	bool is_critical;
	// bool is_faulted; /* note: unused field */
} fault_eval_t;

#endif