#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include "u_tx_can.h"

/**
 * @brief Callback to be called when a message is received on CAN line 1.
 *
 * @param hcan Pointer to struct representing CAN hardware.
 */
#define NUM_INBOUND_CAN1_IDS 1
#define NUM_INBOUND_CAN2_IDS 1

#define CHARGE_CANID		   0x176
#define CHARGE_SIZE		   8
#define DISCHARGE_CANID		   0x156
#define DISCHARGE_SIZE		   8
#define ACC_STATUS_CANID	   0x80
#define ACC_STATUS_SIZE		   8
#define BMS_STATUS_CANID	   0x81
#define BMS_STATUS_SIZE		   3
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
#define ISOSPI_STS_CANID	   0x86
#define ISOSPI_STS_SIZE		   4
#define SEGMENT_AVERAGE_VOLT_CANID 0x90
#define SEGMENT_AVERAGE_VOLT_SIZE  8
#define SEGMENT_TOTAL_VOLT_CANID   0x91
#define SEGMENT_TOTAL_VOLT_SIZE	   8
#define SEGMENT_DELTA_VOLT_CANID   0x92
#define SEGMENT_DELTA_VOLT_SIZE	   8
#define ONBOARD_THERM_CANID	   0x93
#define ONBOARD_THERM_SIZE	   7
#define FAULT_CANID		   0x703 // TODO: cleanup
#define FAULT_SIZE		   5
#define NOISE_CANID		   0x88
#define NOISE_SIZE		   6
#define DEBUG_CANID		   0x702
#define HV_PLATE_DIAGNOSTIC_CANID  0x100
#define HV_PLATE_DIAGNOSTIC_SIZE   8
#define CHARGER_CANID		   0x1806E5F4
#define CHARGERBOX_CANID	   0x18FF50E5
#define DTI_CURRENT_CANID	   0x436
#define BATTBOX_TEMP_CANID     0xD2

#define OVERFLOW_CANID	 0x6F1
#define OVERFLOW_SIZE	 6
#define SEGMENT_PEC_ERROR_CANID	 0x6F2
#define SEGMENT_PEC_ERROR_SIZE	 3
#define HV_PLATE_PEC_ERROR_CANID 0x6F3
#define HV_PLATE_PEC_ERROR_SIZE  2
#define ALPHA_CELL_CANID 0x6FA
#define BETA_CELL_CANID	 0x6FB
#define CELL_MSG_SIZE	 7
#define STAT_A_CANID	 0x6FC
#define STAT_A_SIZE	 7
#define STAT_B_CANID	 0x6FF
#define STAT_B_SIZE	 8

#define CONTROL_CANID	      0x700
#define CONTROL_SIZE	      1
#define CALYPSO_CONTROL_CANID 0x500
#define CALYPSO_CONTROL_SIZE  1

#define DEBUG_SIZE	  8
#define FAULT_TIMER_CANID 0x6F9
#define FAULT_TIMER_SIZE  4

void can_receive_callback(FDCAN_HandleTypeDef *hcan, uint32_t RxFifo0ITs);

/**
 * @brief Place a CAN message in a queue.
 *
 * @param msg CAN message to be sent.
 * @return int8_t Error code.
 */
uint8_t queue_can_msg(can_msg_t can_msg);

/**
 * @brief Initialize CAN lines.
 * @param pointer to FDCAN handler
 *
 * @return error code
 */
uint8_t init_can(FDCAN_HandleTypeDef *hcan);

#endif // CAN_HANDLER_H
