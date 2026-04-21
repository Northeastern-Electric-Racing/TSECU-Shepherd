
#ifndef _DATA_STRUCTS_H
#define _DATA_STRUCTS_H

#include <stdint.h>
#include <stdbool.h>
#include "bms_config.h"
#include "u_tx_mutex.h"
#include "adBms6830Data.h"
#include "adi_bms_2950data.h"
#include "timer.h"
#include "sht30.h"

// clang-format off
#define ANALYZER_FLAG		            		 (1U)
#define SANITIZER_FLAG		            		 (1U << 1)
#define DEBUG_FLAG		                		 (1U << 2)
#define ADBMS6830_SPI_LINE_A_DMA_RX_CPLT_FLAG    (1U << 3)
#define ADBMS6830_SPI_LINE_B_DMA_RX_CPLT_FLAG    (1U << 4)
#define ADBMS2950_SPI_LINE_A_DMA_RX_CPLT_FLAG    (1U << 5)
#define ADBMS2950_SPI_LINE_B_DMA_RX_CPLT_FLAG    (1U << 6)
#define ADBMS6830_SPI_LINE_A_ADC_CONV_CPLT_FLAG  (1U << 7)
#define ADBMS6830_SPI_LINE_B_ADC_CONV_CPLT_FLAG  (1U << 8)
// clang-format on

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

	/* Maximum temperature of on-board therms.*/
	float on_board_temp[NUM_ONBOARD_THERMS_PER_CHIP];

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
	FAULTED,
	NUM_STATES,
} state_t;

typedef enum {
	FAULT_TIMER_STOPPED,
	FAULT_TIMER_STARTED,
	FAULT_TIMER_EXPIRED,
} fault_timer_status_t;

/**
 * @brief Data needed for the therm temp sanitizer
 */
typedef struct {
	therm_state_t sanitized_therms[NUM_CHIPS][NUM_CELLS_PER_CHIP];
	crit_cellval_t max_sanitized_temp;
	crit_cellval_t min_sanitized_temp;
} sanitizer_t;

/**
 * @brief Flags from adbms_2950 chip
 */
typedef union {
	struct {
		uint8_t vreguv : 1;
		uint8_t vregov : 1;
		uint8_t vdduv : 1;
		uint8_t vdiguv : 1;
		uint8_t vdigov : 1;
		uint8_t vde : 1;
		uint8_t vdel : 1;
		uint8_t oscflt : 1;
		uint8_t noclk : 1;
		uint8_t spiflt : 1;
		uint8_t thsd : 1;
		uint8_t reset : 1;
		unsigned : 4;
	} flags;
	uint16_t raw;
} adbms_2950_flags_t;

/**
 * @brief data read from the ADBMS2950 on our HV Plate
 */
typedef struct {
	cell_asic_2950 ic; // ADBMS2950 struct
	float ts_volts; // TS Voltage (V)
	float batt_volts; // BATT Voltage (V)
	float shunt_temp; // Temperature of shunt resistor (C)
	float pack_current; // Current read through the shunt (A)
	uint16_t conversion_count; // Number of conversions taken for each voltage and current measurement
	uint16_t last_total_converion_count; // previously read total conversion count
	adbms_2950_flags_t adbms_flags; // Relevant flags from flag register
	// AUX ADC values
	float vreg; // VREG power supply pin (V)
	float tmp1; // Primary internal temperature sensor (C)
	float vref1p25; // VREF1P25 reference pin (V)
	float epad; // Exposed pad (V)
	float vdig; // Internal digital 3V supply (V)
	float vdd; // VDD power supply pin (V)
	float tmp2; // Secondary internal temperature sensor (C)
	float vdiv; // Divided VREF1 Voltage (V)
	uint16_t osccnt; // Oscillator count
} hv_plate_t;

/**
 * @brief data read from the ADBMS6830 chips on our segments
 */
typedef struct {
	/* Array of structs containing raw data from and configurations for the ADBMS6830 chips */
	cell_asic chips[NUM_CHIPS];

	// the current discharge configuration the state machine wants
	PWM_DUTY discharge_config[NUM_CHIPS][NUM_CELLS_PER_CHIP];
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
 * @brief Segment isoSPI break detection and recovery status structure.
 */
typedef struct {
	isospi_comm_state_t state;
	uint8_t break_chip;
	uint8_t verification_attempts;
	uint8_t recovery_successful;
	uint8_t fault_latched;
	nertimer_t startup_pec_mask_timer;
	nertimer_t pec_accum_timer;
} segment_isospi_status_t;

/**
 * @brief HV plate isoSPI break detection and recovery status structure.
 */
typedef struct {
	isospi_comm_state_t state;
	uint8_t verification_attempts;
	uint8_t recovery_successful;
	uint8_t fault_latched;
	nertimer_t startup_pec_mask_timer;
	nertimer_t pec_accum_timer;
} hv_plate_isospi_status_t;

/**
 * @brief SoC estimator state machine states.
 */
typedef enum {
	SOC_STATE_INIT_FROM_OCV,
	SOC_STATE_COULOMB_COUNTING
} soc_state_t;

/**
 * @brief SoC estimator runtime data.
 */
typedef struct {
	uint32_t prev_time;
	bool soc_reinit_request;
	float soc_drift;
	soc_state_t soc_state;
} soc_data_t;

/**
 * @brief data needed for processing raw data
 */
typedef struct {
	/* Array of data from all chips in the system */
	chipdata_t chip_data[NUM_CHIPS];

	/* Max, min, and avg thermistor readings */
	crit_cellval_t max_temp;
	crit_cellval_t min_temp;
	float avg_temp;

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
	/* Delta voltages for each segment */
	float segment_delt_volts[NUM_SEGMENTS];

	/* Voltage of pack */
	float pack_voltage;

	/* SoC of the Pack*/
	float soc;
} analyzer_t;

/**
 * @brief data retrieved from BMS algorithms
 */
typedef struct {
	// All current limit values are positive
	float cont_DCL;
	float cont_CCL;
	float inst_DCL;
	float inst_CCL;
} bms_algos_t;

/**
 * @brief Cooldown behavior selection for pulse-based current limiting.
 *
 * Defines when a cooldown period is enforced after a pulse.
 */
typedef enum {
	COOLDOWN_ON_FULL_PULSE = 0, // Cooldown only after full pulse
	COOLDOWN_ALWAYS // Cooldown after any pulse exit
} pulse_cooldown_mode_t;

/**
 * @brief State machine states for the pulse-based current limit algorithm.
 *
 * Represents the high-level operating phase of the limiter.
 */
typedef enum {
	CURRENT_LIMIT_STATE_REST = 0, // Limiter idle
	CURRENT_LIMIT_STATE_PULSE, // Pulse active
	CURRENT_LIMIT_STATE_COOLDOWN // Cooldown active
} current_limit_algo_state_t;

/**
 * @brief Input values for the current limit algorithms.
 *
 * This structure contains only the operating-point inputs required by the
 * algorithm. It is algorithm-owned and does not represent system state.
 */
typedef struct {
	float min_temp;
	float max_temp;
	float min_ocv;
	float max_ocv;
} current_limit_algo_inputs_t;

/**
 * @brief Internal control and state for pulse-based current limiting.
 *
 * Holds the algorithm state machine, timers, and configuration needed to
 * manage pulse and cooldown behavior. This structure is owned and maintained
 * by the current limit algorithm.
 */
typedef struct {
	// Current limiter state
	current_limit_algo_state_t state;

	// Cooldown behavior mode
	pulse_cooldown_mode_t cooldown_mode;

	// Pulse duration timer
	nertimer_t pulse_timer;

	// Above and below threshold debounce timer
	nertimer_t t_above;
	nertimer_t t_below;

	// Cooldown duration timer
	nertimer_t cooldown_timer;

	// Pulse allowed flag
	bool pulse_allowed;
} current_limit_pulse_ctrl_t;

typedef enum {
	LONG_CHARGE_UP,
	LONG_SETTLE,
	SHORT_CHARGE_UP,
	SHORT_SETTLE,
	DONE,
	FAULT
} charge_stage_t;

/**
 * @brief data for determine the current BMS State
 */
typedef struct {
	state_t bms_state;

	/**
	 * @brief Note that this is a 32 bit integer, so there are 32 max possible fault codes
	 */

	// charge settling timers
	nertimer_t charging_stage_timer;
	charge_stage_t charging_stage;

	// charging message timer for telemetry
	nertimer_t charger_message_timer;

	bool segment_comms_fault_flag;
	bool hv_plate_comms_fault_flag;

	bool balancing_active;
	bool is_charger_connected;

} state_machine_t;

/**
 * Represents a 3D vector for IMU data
 */
typedef struct {
	float x;
	float y;
	float z;
} vector3_t;

typedef struct {
	vector3_t accel_data;
	vector3_t ang_rate_data;
} imu_data_t;

typedef struct {
	imu_data_t imu_data;
	float onboard_temp;
	bool shutdown_active;
} peripherals_t;

/* Task Args */

typedef struct {
	hv_plate_t *hv_plate;
	analyzer_t *analyzer;
	bms_algos_t *bms_algos;
	acc_data_t *acc_data;
} default_task_args_t;

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
	sanitizer_t *sanitizer;
	peripherals_t *peripherals;
} state_machine_args_t;

typedef struct {
	state_machine_args_t *state_machine_args;
	hv_plate_t *hv_plate;
} can_receive_args_t;

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
	bms_algos_t *bms_algos;
	state_machine_t *state_machine;
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

/**
 * @brief args for peripheral thread
 */
typedef struct {
	peripherals_t *peripherals;
} peripherals_args_t;

/* Task args end */

/**
 * @brief Fault codes
 */
typedef enum {

	/* SHEP CONDITIONAL FAUTS */
	DISCHARGE_LIMIT_ENFORCEMENT_FAULT,
	CHARGE_LIMIT_ENFORCEMENT_FAULT,
	CELL_VOLTAGE_TOO_LOW,
	CELL_VOLTAGE_TOO_HIGH,
	CELL_CHARGE_VOLTAGE_TOO_HIGH,
	PACK_TOO_HOT,
	DIE_TEMP_MAXIMUM_FAULT,

	HV_PLATE_COMMS_FAULT,
	SEGMENT_COMMS_FAULT,

	NUM_FAULTS, /* NUM_REACTIONARY_FAULTS = NUM_FAULTS - NUM_CONDITIONAL_FAULTS - 1 */

	/* TOTAL FAULTS = NUM_FAULTS - 1 */

} fault_code_t;

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

	fault_evalop_t optype_2;
	float data_2;
	float lim_2;

	bool is_critical;
	// bool is_faulted; /* note: unused field */
} fault_eval_t;

#endif
