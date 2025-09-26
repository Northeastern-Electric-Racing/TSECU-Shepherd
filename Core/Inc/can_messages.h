#ifndef CAN_MESSAGES_H
#define CAN_MESSAGES_H

#include "datastructs.h"

#define CHARGE_CANID		   0x176
#define CHARGE_SIZE		   8
#define DISCHARGE_CANID		   0x156
#define DISCHARGE_SIZE		   8
#define ACC_STATUS_CANID	   0x80
#define ACC_STATUS_SIZE		   8
#define BMS_STATUS_CANID	   0x81
#define BMS_STATUS_SIZE		   4
#define FAULT_STATUS_CANID	   0x89
#define FAULT_STATUS_SIZE	   8
#define SHUTDOWN_CTRL_CANID	   0x82
#define SHUTDOWN_CTRL_SIZE	   1
#define CELL_DATA_CANID		   0x83
#define CELL_DATA_SIZE		   8
#define CELL_VOLTAGE_CANID	   0x87
#define CELL_VOLTAGE_SIZE	   8
#define CURRENT_SIZE		   6
#define CELL_TEMP_CANID		   0x84
#define CELL_TEMP_SIZE		   8
#define SEGMENT_TEMP_CANID	   0x85
#define SEGMENT_TEMP_SIZE	   5
#define SEGMENT_AVERAGE_VOLT_CANID 0x90
#define SEGMENT_AVERAGE_VOLT_SIZE  8
#define SEGMENT_TOTAL_VOLT_CANID   0x91
#define SEGMENT_TOTAL_VOLT_SIZE	   8
#define FAULT_CANID		   0x703 // TODO: cleanup
#define FAULT_SIZE		   5
#define NOISE_CANID		   0x88
#define NOISE_SIZE		   6
#define DEBUG_CANID		   0x702
#define CHARGER_CANID		   0x1806E5F4
#define CHARGERBOX_CANID	   0x18FF50E5
#define DTI_CURRENT_CANID	   0x436

#define OVERFLOW_CANID	   0x6F1
#define OVERFLOW_SIZE	   6
#define PEC_ERROR_CANID	   0x6F2
#define PEC_ERROR_SIZE	   3
#define ALPHA_CELL_CANID   0x6FA
#define BETA_CELL_CANID	   0x6FB
#define CELL_MSG_SIZE	   7
#define BETA_STAT_A_CANID  0x6FD
#define BETA_STAT_A_SIZE   8
#define BETA_STAT_B_CANID  0x6FE
#define BETA_STAT_B_SIZE   8
#define BETA_STAT_C_CANID  0x6F0
#define BETA_STAT_C_SIZE   3
#define ALPHA_STAT_A_CANID 0x6FC
#define ALPHA_STAT_A_SIZE  8
#define ALPHA_STAT_B_CANID 0x6FF
#define ALPHA_STAT_B_SIZE  8

#define DEBUG_SIZE	  8
#define FAULT_TIMER_CANID 0x6F9
#define FAULT_TIMER_SIZE  4

/**
 * @brief sends charger message
 *
 * @param voltage_to_set the voltage to charge at
 * @param current_to_set the current to charge at
 * @param is_charging_enabled whether charging is allowed
 *
 */
int send_charging_message(float voltage_to_set, float current_to_set,
			  bool is_charging_enabled);

/**
 * @brief Sends max discharge current to Motor Controller.
 *
 * @param bmsdata data structure containing the discharge limit
 */
void send_mc_discharge_message(float discharge_limit);

/**
 * @brief sends max charge/discharge current to Motor Controller
 *
 * @param charge_limit
 */
void send_mc_charge_message(float charge_limit);

/**
 * @brief sends acc status message
 *
 * @param pack_voltage
 * @param pack_current
 * @param soc
 *
 * @return Returns a fault if we are not able to send
 */
void send_acc_status_message(float pack_voltage, float pack_current, float soc);

/**
 * @brief sends fault status message
 *
 * @param fault_code_crit
 * @param fault_code_noncrit
 *
 */
void send_fault_status_message(uint32_t fault_code_crit,
			       uint32_t fault_code_noncrit);

/**
 * @brief sends BMS status message
 *
 * @param avg_temp
 * @param internal_temp
 * @param bms_state
 * @param balance
 *
 * @return Returns a fault if we are not able to send
 */
void send_bms_status_message(float avg_temp, float internal_temp, int bms_state,
			     bool balance);

/**
 * @brief sends shutdown control message
 * @note unused
 *
 * @param mpe_state
 *
 * @return Returns a fault if we are not able to send
 */
void send_shutdown_ctrl_message(uint8_t mpe_state);

/**
 * @brief sends cell data message
 *
 * @param max_voltage
 * @param min_voltage
 * @param avg_voltage
 *
 * @return Returns a fault if we are not able to send
 */
void send_cell_voltage_message(crit_cellval_t max_voltage,
			       crit_cellval_t min_voltage, float avg_voltage);

void send_segment_average_volt_message(bms_t *bmsdata);
void send_segment_total_volt_message(bms_t *bmsdata);

/**
 * @brief sends cell temperature message
 *
 * @param max_temp
 * @param min_temp
 * @param avg_temp
 * 
 * @return Returns a fault if we are not able to send
 */
void send_cell_temp_message(crit_cellval_t max_temp, crit_cellval_t min_temp,
			    float avg_temp);

/**	
 * @brief sends the average segment temperatures	
 *	
 *	
 *	
 * @return Returns a fault if we are not able to send	
 */
void send_segment_temp_message(bms_t *bmsdata);

void send_fault_message(uint8_t status, int16_t curr, int16_t in_dcl);

void send_fault_timer_message(uint8_t start_stop, uint32_t fault_code,
			      float data_1);

/**
 * @brief Send CAN message for debugging the car on the fly.
 * 
 * @param debug0 
 * @param debug1 
 * @param debug2 
 * @param debug3 
 */
void send_debug_message(uint8_t debug0, uint8_t debug1, uint16_t debug2,
			uint32_t debug3);

/**
 * @brief Send a message containing cell data.
 * 
 * @param alpha If this message contains alpha cell data. False sends a beta cell message.
 * @param temperature Temperature in Celsius. Has a maximum value of 80 degrees celsius.
 * @param voltage_a The voltage of cell A.
 * @param voltage_b The voltage of cell B.
 * @param chip_ID The chip ID.
 * @param cell_a The number of cell A.
 * @param cell_b The number of cell B.
 * @param discharging_a The state of cell A while balancing.
 * @param discharging_b The state of cell B while balancing.
 * @param cvs_a The C v S fault of cell A.
 * @param cvs_b The C v S fault of cell B.
 */
void send_cell_data_message(bool alpha, float temperature, float voltage_a,
			    float voltage_b, uint8_t chip_ID, uint8_t cell_a,
			    uint8_t cell_b, bool discharging_a,
			    bool discharging_b, bool cvs_a, bool cvs_b);

/**
 * @brief Send cell message containing Beta cell 10, the Beta onboard therm, the temperature of the ADBMS6830 die, and the voltage from V+ to V-.
 * 
 * @param cell_temperature Temperature of Beta cell 10.
 * @param voltage Voltage of Beta cell 10.
 * @param discharging Whether or not the cell is discharging.
 * @param chip The ID of the chip.
 * @param segment_temperature The output of the onboard therm.
 * @param die_temperature The temperature of the ADBMS6830 die.
 * @param vpv The voltage from V+ to V-.
 */
void send_beta_status_a_message(float cell_temperature, float voltage,
				bool discharging, uint8_t chip,
				float segment_temperature,
				float die_temperature, float vpv);

/**
 * @brief Send message containing ADBMS6830 diagnostic data.
 * 
 * @param vref2 Second reference voltage for ADBMS6830.
 * @param v_analog Analog power supply voltage.
 * @param v_digital Digital power supply voltage.
 * @param chip ID of the chip.
 * @param v_res VREF2 across a resistor for open wire detection.
 * @param vmv Voltage between S1N and V-.
 * @param cvs The C v S fault of Beta chip 10
 */
void send_beta_status_b_message(float vref2, float v_analog, float v_digital,
				uint8_t chip, float v_res, float vmv, bool cvs);

/**
 * @brief Send a message for the faults of beta chips.
 * TODO: remove thus
 * 
 * @param chip ID of chip
 * @param flt_reg  the fault data register
 */
void send_beta_status_c_message(uint8_t chip, stc_ *flt_reg);

/**
 * @brief Send message containing ADBMS6830 diagnostic data and onboard therm data.
 * 
 * @param segment_temp Temperature reading from on-board therm.
 * @param chip ID of the chip.
 * @param die_temperature Temperature of the ADBOS6830 die.
 * @param vpv The voltage from V+ to V-.
 * @param vmv Voltage between S1N and V-.
 * @param flt_reg The fault register of the chip (statc)
 */
void send_alpha_status_a_message(float segment_temp, uint8_t chip,
				 float die_temperature, float vpv, float vmv,
				 stc_ *flt_reg);

/**
 * @brief Send message containing ADBMS6830 diagnostic data.
 * 
 * @param v_res VREF2 across a resistor for open wire detection.
 * @param chip ID of the chip.
 * @param vref2 Second reference voltage for ADBMS6830.
 * @param v_analog Analog power supply voltage.
 * @param v_digital Digital power supply voltage.
 * @param flt_reg The fault register of the chip (statc)
 */
void send_alpha_status_b_message(float v_res, uint8_t chip, float vref2,
				 float v_analog, float v_digital,
				 stc_ *flt_reg);

/**
 * @brief Sends a CAN message containing the PEC error count for a specific chip.
 *
 * @param chip_num The index of the chip that reported PEC errors.
 * @param pec_count The total number of PEC errors detected for the specified chip.
 */
void send_pec_error_message(uint8_t chip_num, uint16_t pec_count);

#endif
