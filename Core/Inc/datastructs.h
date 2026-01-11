
#ifndef _DATA_STRUCTS_H
#define _DATA_STRUCTS_H

#include <stdint.h>
#include <stdbool.h>
#include "bms_config.h"
#include "u_tx_mutex.h"
#include "adBms6830Data.h"
#include "adi_bms_2950data.h"
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

	bool alpha;
	/* whether the chip is an alpha or beta */ // TODO initialize properly

	/* These are calculated during the analysis of data */

	/* Cell temperature in celsius */
	float cell_temp[NUM_CELLS_PER_CHIP];
	float cell_resistance[NUM_CELLS_PER_CHIP];
	float open_cell_voltage[NUM_CELLS_PER_CHIP];
	float cell_voltages[NUM_CELLS_PER_CHIP];

	/* For temperatures of on-board therms.*/
	float on_board_temp;

	/// temperature of the die
	float die_temp;

	/* Chip and Cell Diagnostic Data */
	bool is_balancing[NUM_CELLS_PER_CHIP];
	bool cs_fault[NUM_CELLS_PER_CHIP];

	float vpv;
	float vmv;
	float v_res;
	float vref2;
	float v_analog;
	float v_digital;

	stc_ flt_reg;

} chipdata_t;

/**
 * @brief Stores critical values for the pack, and where that critical value can be found
 */
typedef struct {
	float val;
	uint8_t chipIndex;
	uint8_t cellNum;
} crit_cellval_t;

/**
 * @brief A therm_state_t is a struct of a (float, bool).
 * - Last recorded temperature of a cell.
 * - Whether the cell temperature can be used (i.e. whether the measurement is bad).
 */
typedef struct {
	float last_temp;
	bool valid;
} therm_state_t;

/**
 * @brief States the BMS can be in
 */
typedef enum {
	BOOT,
	READY,
	CHARGING,
	BALANCING,
	FAULTED,
	NUM_STATES,
} state_t;

/**
 * @brief Data needed for the therm temp sanitizer
 */
typedef struct {
	therm_state_t sanitized_therms[NUM_CHIPS][NUM_CELLS_PER_CHIP];
} sanitizer_t;

/**
 * @brief data read from the ADBMS2950 on our HV Plate
 */
typedef struct {
	cell_asic_2950 *ic; // ADBMS2950 struct
	float ts_volts; // TS Voltage (V)
	float batt_volts; // BATT Voltage (V)
	float shunt_temp; // Temperature of shunt resistor (C)
	float pack_current; // Current read through the shunt
} hv_plate_t;

/**
 * @brief data read from the ADBMS6830 chips on our segments
 */
typedef struct {
	/* Array of structs containing raw data from and configurations for the ADBMS6830 chips */
	cell_asic chips[NUM_CHIPS];

	// the current discharge configuration the state machine wants
	bool discharge_config[NUM_CHIPS][NUM_CELLS_PER_CHIP];
} acc_data_t;

/**
 * @brief ISO SPI communication state machine states.
 */
typedef enum {
	ISOSPI_STATE_NORMAL = 0x01U,
	ISOSPI_BREAK_DETECTED = 0x02U,
	ISOSPI_STATE_VERIFYING = 0x03U,
	ISOSPI_RECOVERY_SUCCESS = 0x04U,
	ISOSPI_RECOVERY_FAILED = 0x05U
} isospi_comm_state_t;

/**
 * @brief ISO SPI break detection and recovery status structure.
 */
typedef struct {
	isospi_comm_state_t state;
	uint8_t break_chip;
	uint8_t verification_attempts;
	uint8_t recovery_successful;
	uint8_t fault_latched;
} isospi_status_t;

/**
 * @brief data needed for processing raw data
 */
typedef struct {
	mutex_t analyzer_mutex;

	/* Array of data from all chips in the system */
	chipdata_t chip_data[NUM_CHIPS];

	/* Max, min, and avg thermistor readings */
	crit_cellval_t max_temp;
	crit_cellval_t min_temp;
	float avg_temp;

	// the board temperature
	float internal_temp;

	/* Max, min, and avg voltage of the cells */
	crit_cellval_t max_voltage;
	crit_cellval_t min_voltage;
	float avg_voltage;
	float delt_voltage;

	// OCV timer
	nertimer_t ocvTimer;

	/* Max, min, average OCV readings */
	crit_cellval_t max_ocv;
	crit_cellval_t min_ocv;
	float avg_ocv;
	float delt_ocv;
	float pack_ocv;
	float pack_res;

	// the highest current chip temperature, for faulting
	crit_chipval_t max_chiptemp;

	/* semgent temperature averages */
	float segment_average_temps[NUM_SEGMENTS];
	/* OCV average voltages */
	float segment_average_volts[NUM_SEGMENTS];
	/* Total voltages for each segment */
	float segment_total_volts[NUM_SEGMENTS];
	/* Pack voltage */
	float pack_voltage;

	/* SoC of the Pack*/
	float soc;
} analyzer_t;

/**
 * @brief data retrieved from BMS algorithms
 */
typedef struct {
	float cont_DCL;
	float cont_CCL;
} bms_algos_t;

/**
 * @brief data for determine the current BMS State
 */
typedef struct {
	state_t bms_state;

	/**
	 * @brief Note that this is a 32 bit integer, so there are 32 max possible fault codes
	 */
	// uint32_t fault_code;
	uint32_t fault_code_crit;
	uint32_t fault_code_noncrit;

	// charge settling timers
	nertimer_t charger_settle_countup_timer;
	nertimer_t charge_settle_countdown_timer;

	// charging message timer for telemetry
	nertimer_t charger_message_timer;

	mutex_t state_mutex;

} state_machine_t;

/* Task Args */

/**
 * @brief args for vStateMachine
 */
typedef struct {
	state_machine_t *state_machine;
	analyzer_t *analyzer;
	hv_plate_t *
		hv_plate; // TODO add hv plate interal data to analyzer to remove hv_plate
	acc_data_t *acc_data;
	bms_algos_t *bms_algos;
} state_machine_args_t;

/**
 * @brief args for vAnalyzer
 */
typedef struct {
	analyzer_t *analyzer;
	state_machine_t *state_machine;
	acc_data_t *acc_data;
	hv_plate_t *hv_plate;
} analyzer_args_t;

/**
 * @brief args for vGetSegmentData
 */
typedef struct {
	acc_data_t *acc_data;
	state_machine_t *state_machine;
} acc_data_args_t;

/**
 * @brief args for vHvPlate
 */
typedef struct {
	hv_plate_t *hv_plate;
	analyzer_t *analyzer;
} hv_plate_args_t;

/**
 * @brief args for vSanitizer
 */
typedef struct {
	sanitizer_t *sanitizer;
	analyzer_t *analyzer;
} sanitizer_args_t;

/**
 * @brief args for vBmsAlgorithms
 */
typedef struct {
	sanitizer_t *sanitizer;
	analyzer_t *analyzer;
	bms_algos_t *bms_algos;
} bms_algos_args_t;

/* Task args end */

/**
 * @brief Fault codes
 */
enum {
	FAULTS_CLEAR = 0x0,

	/* Shepherd BMS faults */
	CELLS_NOT_BALANCING = 0x1,
	CELL_VOLTAGE_TOO_HIGH = 0x2,
	CELL_VOLTAGE_TOO_LOW = 0x4,
	PACK_TOO_HOT = 0x8,
	WEAK_PACK_FAULT = 0x10,
	EXTERNAL_CAN_FAULT = 0x20,
	DISCHARGE_LIMIT_ENFORCEMENT_FAULT = 0x40,
	CHARGE_LIMIT_ENFORCEMENT_FAULT = 0x80,
	DIE_TEMP_MAXIMUM_FAULT = 0x100,
	HV_PLATE_COMMS_FAULT = 0x200,
	SEGMENT_COMMS_FAULT = 0x400,

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