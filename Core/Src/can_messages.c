#include "can_messages.h"
#include "bitstream.h"
#include "c_utils.h"
#include "can_handler.h"
#include "fdcan.h"
#include "shep_queues.h"
#include <math.h>

/// @brief A helper which sends appropriate error to stdout and CAN if a
/// bistream overflows
/// @param bitstream_res The bitstream to check for overflow
/// @param can_id The CAN ID this bistream data is intended for
/// @return 0 if success
static const bool handle_bitstream_overflow(bitstream_t *bitstream_res,
					    uint32_t can_id)
{
	if (!bitstream_res->overflow) {
		return 0;
	}

	// printf("CAN MESSAGE %ld overflowed!\n", can_id);

	static uint16_t overflow_cnt = 0;
	overflow_cnt++;
	struct __attribute__((__packed__)) {
		uint32_t can_id;
		uint16_t overflow_cnt;
	} overflow_data;

	endian_swap(&can_id, sizeof(can_id));
	overflow_data.can_id = can_id;
	overflow_data.overflow_cnt = overflow_cnt;

	can_msg_t overflow_msg = { .id = OVERFLOW_CANID,
				   .len = 6,
				   .data = { 0 } };

	memcpy(&overflow_msg.data, &overflow_data, sizeof(overflow_data));

	queue_can_msg(overflow_msg);

	return 0;
}

int send_charging_message(float voltage_to_set, float current_to_set,
			  bool is_charging_enabled)
{
	struct __attribute__((__packed__)) {
		uint16_t charger_voltage; // Note the charger voltage sent over should be
		// 10*desired voltage
		uint16_t charger_current; // Note the charge current sent over should be
		// 10*desired current
		uint8_t charger_control;
		uint8_t reserved_1;
		uint16_t reserved_23;
	} charger_msg_data;

	charger_msg_data.charger_voltage = (uint16_t)(voltage_to_set * 10);
	charger_msg_data.charger_current = (uint16_t)(current_to_set * 10);

	if (is_charging_enabled) {
		charger_msg_data.charger_control = 0x00; // 0：Start charging.
	} else {
		charger_msg_data.charger_control =
			0xFF; // 1：battery protection, stop charging
	}

	charger_msg_data.reserved_1 = 0x00;
	charger_msg_data.reserved_23 = 0x0000;

	can_msg_t charger_msg = { .id = CHARGER_CANID,
				  .id_is_extended = true,
				  .len = 8,
				  .data = { 0 } };
	memcpy(charger_msg.data, &charger_msg_data, sizeof(charger_msg_data));

	uint8_t temp = charger_msg.data[0];
	charger_msg.data[0] = charger_msg.data[1];
	charger_msg.data[1] = temp;
	temp = charger_msg.data[2];
	charger_msg.data[2] = charger_msg.data[3];
	charger_msg.data[3] = temp;

	if (is_charging_enabled) {
		HAL_StatusTypeDef res = queue_can_msg(charger_msg);
		if (res != HAL_OK) {
			printf("queue_can_msg() ERROR CODE %X", res);
		}
	}

	return 0;
}

void send_mc_discharge_message(float discharge_limit)
{
	struct __attribute__((__packed__)) {
		uint16_t max_discharge;
	} discharge_data;

	/* scale to A * 10 */
	discharge_data.max_discharge = 10 * discharge_limit;

	/* convert to big endian */
	endian_swap(&discharge_data.max_discharge,
		    sizeof(discharge_data.max_discharge));

	can_msg_t msg = { .id = DISCHARGE_CANID,
			  .len = DISCHARGE_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &discharge_data, sizeof(discharge_data));

	queue_can_msg(msg);
}

void send_mc_charge_message(float charge_limit)
{
	struct __attribute__((__packed__)) {
		int16_t max_charge;
	} charge_data;

	/* scale to A * 10 */
	charge_data.max_charge = -10 * charge_limit;

	/* convert to big endian */
	endian_swap(&charge_data.max_charge, sizeof(charge_data.max_charge));

	can_msg_t msg = { .id = CHARGE_CANID,
			  .len = CHARGE_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &charge_data, sizeof(charge_data));

	queue_can_msg(msg);
}

void send_acc_status_message(float pack_voltage, float pack_current, float soc)
{
	struct __attribute__((__packed__)) {
		uint16_t packVolt;
		int16_t pack_current;
		uint16_t pack_ah;
		uint8_t pack_soc;
		uint8_t pack_health;
	} acc_status_msg_data;

	acc_status_msg_data.packVolt = pack_voltage * 10;
	acc_status_msg_data.pack_current =
		(int16_t)(pack_current *
			  10); // converted to signed int and scaled by 10
	acc_status_msg_data.pack_ah = 0;
	acc_status_msg_data.pack_soc = soc;
	acc_status_msg_data.pack_health = 0;

	/* convert to big endian */
	endian_swap(&acc_status_msg_data.packVolt,
		    sizeof(acc_status_msg_data.packVolt));
	endian_swap(&acc_status_msg_data.pack_current,
		    sizeof(acc_status_msg_data.pack_current));
	endian_swap(&acc_status_msg_data.pack_ah,
		    sizeof(acc_status_msg_data.pack_ah));

	can_msg_t msg = { .id = ACC_STATUS_CANID,
			  .len = ACC_STATUS_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &acc_status_msg_data, sizeof(acc_status_msg_data));

	queue_can_msg(msg);
}

void send_fault_status_message(uint32_t fault_code_crit,
			       uint32_t fault_code_noncrit)
{
	struct __attribute__((__packed__)) {
		uint32_t fault_crit;
		uint32_t fault_noncrit;
	} fault_status_msg_data;

	fault_status_msg_data.fault_crit = fault_code_crit;
	fault_status_msg_data.fault_noncrit = fault_code_noncrit;

	/* convert to big endian */
	endian_swap(&fault_status_msg_data.fault_crit,
		    sizeof(fault_status_msg_data.fault_crit));
	endian_swap(&fault_status_msg_data.fault_noncrit,
		    sizeof(fault_status_msg_data.fault_noncrit));

	can_msg_t fault_msg = { .id = FAULT_STATUS_CANID,
				.len = FAULT_STATUS_SIZE,
				.data = { 0 } };

	memcpy(fault_msg.data, &fault_status_msg_data,
	       sizeof(fault_status_msg_data));

	queue_can_msg(fault_msg);
}

void send_bms_status_message(float avg_temp, float temp_internal, int bms_state)
{
	struct __attribute__((__packed__)) {
		uint8_t state;
		int8_t temp_avg;
		uint8_t temp_internal;
	} bms_status_msg_data;

	bms_status_msg_data.temp_avg = (int8_t)(avg_temp);
	bms_status_msg_data.state = (uint8_t)(bms_state);
	bms_status_msg_data.temp_internal = (uint8_t)(temp_internal);

	can_msg_t msg = { .id = BMS_STATUS_CANID,
			  .len = BMS_STATUS_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &bms_status_msg_data, sizeof(bms_status_msg_data));

	queue_can_msg(msg);
}

// UNUSED
void send_shutdown_ctrl_message(uint8_t mpe_state)
{
	struct __attribute__((__packed__)) {
		uint8_t mpeState;
	} shutdown_control_msg_data;

	shutdown_control_msg_data.mpeState = mpe_state;

	can_msg_t msg = { .id = SHUTDOWN_CTRL_CANID,
			  .len = SHUTDOWN_CTRL_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &shutdown_control_msg_data,
	       sizeof(shutdown_control_msg_data));

	queue_can_msg(msg);
}

void send_cell_voltage_message(crit_cellval_t max_voltage,
			       crit_cellval_t min_voltage, float avg_voltage)
{
	bitstream_t cell_voltage_msg;
	uint8_t bitstream_data[8];
	bitstream_init(&cell_voltage_msg, bitstream_data,
		       8); // Create 7-byte bitstream
	bitstream_add(&cell_voltage_msg, max_voltage.val * 10000, 16);
	bitstream_add(&cell_voltage_msg, max_voltage.chipIndex, 4);
	bitstream_add(&cell_voltage_msg, max_voltage.cellNum, 4);
	bitstream_add(&cell_voltage_msg, min_voltage.val * 10000, 16);
	bitstream_add(&cell_voltage_msg, min_voltage.chipIndex, 4);
	bitstream_add(&cell_voltage_msg, min_voltage.cellNum, 4);
	bitstream_add(&cell_voltage_msg, avg_voltage * 10000, 16);

	can_msg_t msg = { .id = CELL_DATA_CANID,
			  .len = CELL_DATA_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &bitstream_data, sizeof(bitstream_data));

	queue_can_msg(msg);
}
void send_segment_average_volt_message(analyzer_t *analyzer)
{
	bitstream_t segment_average_volt_msg;
	uint8_t bitstream_data[8];
	bitstream_init(&segment_average_volt_msg, bitstream_data, 8);

	bitstream_add(&segment_average_volt_msg,
		      analyzer->segment_average_volts[0] * 1000, 12);
	bitstream_add(&segment_average_volt_msg,
		      analyzer->segment_average_volts[1] * 1000, 12);
	bitstream_add(&segment_average_volt_msg,
		      analyzer->segment_average_volts[2] * 1000, 12);
	bitstream_add(&segment_average_volt_msg,
		      analyzer->segment_average_volts[3] * 1000, 12);
	bitstream_add(&segment_average_volt_msg,
		      analyzer->segment_average_volts[4] * 1000, 12);

	can_msg_t msg;
	msg.id = SEGMENT_AVERAGE_VOLT_CANID;
	msg.len = SEGMENT_AVERAGE_VOLT_SIZE;

	memcpy(msg.data, &bitstream_data, 8);

	handle_bitstream_overflow(&segment_average_volt_msg, msg.id);
	queue_can_msg(msg);
}

void send_segment_total_volt_message(analyzer_t *analyzer)
{
	bitstream_t segment_total_volt_msg;
	uint8_t bitstream_data[8];
	bitstream_init(&segment_total_volt_msg, bitstream_data, 8);

	bitstream_add(&segment_total_volt_msg,
		      analyzer->segment_total_volts[0] * 39, 12); // Segment 1
	bitstream_add(&segment_total_volt_msg,
		      analyzer->segment_total_volts[1] * 39, 12); // Segment 2
	bitstream_add(&segment_total_volt_msg,
		      analyzer->segment_total_volts[2] * 39, 12); // Segment 3
	bitstream_add(&segment_total_volt_msg,
		      analyzer->segment_total_volts[3] * 39, 12); // Segment 4
	bitstream_add(&segment_total_volt_msg,
		      analyzer->segment_total_volts[4] * 39, 12); // Segment 5
	bitstream_add(&segment_total_volt_msg, 0, 4); // Extra (4 bits)

	can_msg_t msg;
	msg.id = SEGMENT_TOTAL_VOLT_CANID;
	msg.len = SEGMENT_TOTAL_VOLT_SIZE;

	memcpy(msg.data, &bitstream_data, 8);

	handle_bitstream_overflow(&segment_total_volt_msg, msg.id);
	queue_can_msg(msg);
}

void send_segment_delta_volt_message(analyzer_t *analyzer)
{
	bitstream_t segment_delta_volt_msg;
	uint8_t bitstream_data[8];
	bitstream_init(&segment_delta_volt_msg, bitstream_data, 8);

	bitstream_add(&segment_delta_volt_msg,
		      analyzer->segment_delt_volts[0] * 1000, 12); // Segment 1
	bitstream_add(&segment_delta_volt_msg,
		      analyzer->segment_delt_volts[1] * 1000, 12); // Segment 2
	bitstream_add(&segment_delta_volt_msg,
		      analyzer->segment_delt_volts[2] * 1000, 12); // Segment 3
	bitstream_add(&segment_delta_volt_msg,
		      analyzer->segment_delt_volts[3] * 1000, 12); // Segment 4
	bitstream_add(&segment_delta_volt_msg,
		      analyzer->segment_delt_volts[4] * 1000, 12); // Segment 5
	bitstream_add(&segment_delta_volt_msg, 0, 4); // Extra (4 bits)

	can_msg_t msg;
	msg.id = SEGMENT_DELTA_VOLT_CANID;
	msg.len = SEGMENT_DELTA_VOLT_SIZE;

	memcpy(msg.data, &bitstream_data, 8);

	handle_bitstream_overflow(&segment_delta_volt_msg, msg.id);
	queue_can_msg(msg);
}

void send_cell_temp_message(crit_cellval_t max_temp, crit_cellval_t min_temp,
			    float avg_temp)
{
	bitstream_t cell_voltage_msg;
	uint8_t bitstream_data[8];
	bitstream_init(&cell_voltage_msg, bitstream_data,
		       8); // Create 7-byte bitstream
	bitstream_add(&cell_voltage_msg, max_temp.val * 100, 16);
	bitstream_add(&cell_voltage_msg, max_temp.chipIndex, 4);
	bitstream_add(&cell_voltage_msg, max_temp.cellNum, 4);
	bitstream_add(&cell_voltage_msg, min_temp.val * 100, 16);
	bitstream_add(&cell_voltage_msg, min_temp.chipIndex, 4);
	bitstream_add(&cell_voltage_msg, min_temp.cellNum, 4);
	bitstream_add(&cell_voltage_msg, avg_temp * 100, 16);

	can_msg_t msg = { .id = CELL_TEMP_CANID,
			  .len = CELL_TEMP_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &bitstream_data, sizeof(bitstream_data));

	queue_can_msg(msg);
}

void send_segment_temp_message(analyzer_t *analyzer)
{
	struct __attribute__((__packed__)) {
		int8_t segment1_average_temp;
		int8_t segment2_average_temp;
		int8_t segment3_average_temp;
		int8_t segment4_average_temp;
		int8_t segment5_average_temp;
	} segment_temp_msg_data;

	segment_temp_msg_data.segment1_average_temp =
		analyzer->segment_average_temps[0];
	segment_temp_msg_data.segment2_average_temp =
		analyzer->segment_average_temps[1];
	segment_temp_msg_data.segment3_average_temp =
		analyzer->segment_average_temps[2];
	segment_temp_msg_data.segment4_average_temp =
		analyzer->segment_average_temps[3];
	segment_temp_msg_data.segment5_average_temp =
		analyzer->segment_average_temps[4];

	can_msg_t msg = { .id = SEGMENT_TEMP_CANID,
			  .len = SEGMENT_TEMP_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &segment_temp_msg_data, sizeof(segment_temp_msg_data));

	queue_can_msg(msg);
}

void send_fault_timer_message(uint8_t start_stop, uint32_t fault_code,
			      float data_1)
{
	struct __attribute__((__packed__)) {
		uint8_t start_stop;
		uint8_t fault_code;
		float data_1;
	} fault_timer_msg_data;

	fault_timer_msg_data.start_stop = start_stop;
	fault_timer_msg_data.fault_code = log2(fault_code);
	fault_timer_msg_data.data_1 = data_1;

	endian_swap(&fault_timer_msg_data.data_1,
		    sizeof(fault_timer_msg_data.data_1));

	can_msg_t msg = { .id = FAULT_TIMER_CANID,
			  .len = FAULT_TIMER_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &fault_timer_msg_data, sizeof(fault_timer_msg_data));

	queue_can_msg(msg);
}

void send_debug_message(uint8_t debug0, uint8_t debug1, uint16_t debug2,
			uint32_t debug3)
{
	struct __attribute__((__packed__)) {
		uint8_t debug0;
		uint8_t debug1;
		uint16_t debug2;
		uint32_t debug3;
	} debug_msg_data;

	debug_msg_data.debug0 = debug0;
	debug_msg_data.debug1 = debug1;
	debug_msg_data.debug2 = debug2;
	debug_msg_data.debug3 = debug3;

	endian_swap(&debug_msg_data.debug2, sizeof(debug_msg_data.debug2));
	endian_swap(&debug_msg_data.debug3, sizeof(debug_msg_data.debug3));

	can_msg_t msg = { .id = DEBUG_CANID, .len = DEBUG_SIZE, .data = { 0 } };

	memcpy(msg.data, &debug_msg_data, 8);

	queue_can_msg(msg);
}

// Changes made by Sam on 3/30/25, not verified
// verified by Jack on 3/17/2025 for beta and alpha chip 0
void send_cell_data_message(bool alpha, float temperature, float voltage_a,
			    float voltage_b, uint8_t chip_ID, uint8_t cell_a,
			    uint8_t cell_b, bool discharging_a,
			    bool discharging_b, bool cvs_a, bool cvs_b)
{
	// clang-format off
	can_msg_t msg = { .len = CELL_MSG_SIZE, .data = { 0 } };
	if (alpha) {
		msg.id = ALPHA_CELL_CANID;
	} else {
		msg.id = BETA_CELL_CANID;
	}

	// patch bc 0 to 4
	chip_ID /= 2;

	/* Multiply data by scaling factor before converuting to int */
	temperature *= 10;
	voltage_a *= 1000;
	voltage_b *= 1000;

	bitstream_t cell_data_message;
	uint8_t bitstream_data[7];
	bitstream_init(&cell_data_message, bitstream_data, 7); // Create 7-byte bitstream

	bitstream_add(&cell_data_message, temperature, 10); 			// Cell temperature (10 bits)
	bitstream_add(&cell_data_message, voltage_a, 13);   			// Voltage A (13 bits)
	bitstream_add(&cell_data_message, voltage_b, 13);   			// Voltage B (13 bits)
	bitstream_add(&cell_data_message, chip_ID, 4);   // Chip ID (4 bits)
	bitstream_add(&cell_data_message, cell_a, 4);    // Cell A (4 bits)
	bitstream_add(&cell_data_message, cell_b, 4);    // Cell B (4 bits)
	bitstream_add(&cell_data_message, discharging_a, 1); 			// Discharging A (1 bit)
	bitstream_add(&cell_data_message, discharging_b, 1); 			// Discharging B (1 bit)
	bitstream_add(&cell_data_message, cvs_a, 1); 			// C v S of A (1 bit)
	bitstream_add(&cell_data_message, cvs_b, 1); 			// C v S of B (1 bit)
	bitstream_add(&cell_data_message, 0, 4);             			// Extra (4 bits)

	memcpy(msg.data, &bitstream_data, CELL_MSG_SIZE);

	handle_bitstream_overflow(&cell_data_message, msg.id);

	queue_can_msg(msg);
}

// verified 3/17/2025 for chip 0 by Jack, EXCLUDING VMV (see TODO)
void send_status_a_message(uint8_t chip,
				 float die_temperature, float vpv, float vmv,
				 stc_ *flt_reg)
{
	can_msg_t msg = { .id = STAT_A_CANID, .len = STAT_A_SIZE, .data = { 0 } };

	die_temperature *= 100;
	vpv *= 100;
	vmv *= 1000;
	chip /= 2;


	bitstream_t alpha_status_a_message;
	uint8_t bitstream_data[8];
	bitstream_init(&alpha_status_a_message, bitstream_data, 7);	// Create 7-byte bitstream

	bitstream_add(&alpha_status_a_message, chip, 4);	// Chip ID (4 bits)
	bitstream_add(&alpha_status_a_message, die_temperature, 13);	// Die Temp (13 bits)
	bitstream_add(&alpha_status_a_message, vpv, 13);				// Vpv (13 bits)
	bitstream_add_signed(&alpha_status_a_message, vmv, 13);					// Vmv (8 bits)			// Vpv (5 bits)
	bitstream_add(&alpha_status_a_message, flt_reg->va_ov, 1);		// VA_OV (1 bit)
	bitstream_add(&alpha_status_a_message, flt_reg->va_uv, 1);		// VA_UV (1 bit)
	bitstream_add(&alpha_status_a_message, flt_reg->vd_ov, 1);		// VD_OV (1 bit)
	bitstream_add(&alpha_status_a_message, flt_reg->vd_uv, 1);		// VD_UV (1 bit)
	bitstream_add(&alpha_status_a_message, flt_reg->vde, 1);		// VDE (1 bit)
	bitstream_add(&alpha_status_a_message, flt_reg->vdel, 1);		// VDEL (1 bit)
	bitstream_add(&alpha_status_a_message, flt_reg->spiflt, 1);		// SPIFLT (1 bit)
	bitstream_add(&alpha_status_a_message, flt_reg->sleep, 1);		// SLEEP (1 bit)
	bitstream_add(&alpha_status_a_message, flt_reg->thsd, 1);		// THSD (1 bit)
	bitstream_add(&alpha_status_a_message, flt_reg->tmodchk, 1);	// TMODCHK (1 bit)
	bitstream_add(&alpha_status_a_message, flt_reg->oscchk, 1);	 	// OSCCHK (1 bit)

	memcpy(msg.data,
		&bitstream_data, STAT_A_SIZE);

	handle_bitstream_overflow(&alpha_status_a_message, msg.id);

	queue_can_msg(msg);
}

// verified 3/17/2025 for chip 0 by Jack. mostly faults too
void send_status_b_message(float v_res, uint8_t chip, float vref2,
				 float v_analog, float v_digital, stc_ *flt_reg)
{
	can_msg_t msg = { .id = STAT_B_CANID, .len = STAT_B_SIZE, .data = { 0 } };

	// printf("Vres %f\n", v_res);
	// printf("Vref2 %f\n", vref2);
	// printf("Vanalog %f\n", v_analog);
	// printf("Vdigital %f\n", v_digital);

	v_res *= 1000;
	vref2 *= 1000;
	v_analog *= 1000;
	v_digital *= 1000;

	chip /= 2;

	bitstream_t alpha_status_b_message;
	uint8_t bitstream_data[8];
	bitstream_init(&alpha_status_b_message, bitstream_data, 8);	// Create 8-byte bitstream

	bitstream_add(&alpha_status_b_message, v_res, 13);				// Vres (13 bits)
	bitstream_add(&alpha_status_b_message, chip, 4);	// Chip ID (4 bits)
	bitstream_add(&alpha_status_b_message, vref2, 13);				// Vref2 (13 bits)
	bitstream_add(&alpha_status_b_message, v_analog, 13);			// Vanalog (13 bits)
	bitstream_add(&alpha_status_b_message, v_digital, 13);			// Vdigital (13 bits)
	bitstream_add(&alpha_status_b_message, flt_reg->otp1_med, 1);	// OTP1_MED (1 bit)
	bitstream_add(&alpha_status_b_message, flt_reg->otp2_med, 1);	// OTP2_MED (1 bit)
	bitstream_add(&alpha_status_b_message, 0, 6);					// Extra (6 bits)

	memcpy(msg.data, &bitstream_data, STAT_B_SIZE);

	handle_bitstream_overflow(&alpha_status_b_message, msg.id);

	queue_can_msg(msg);
	// clang-format on
}

void send_segment_pec_error_message(const uint8_t chip_num, const uint16_t pec_error)
{
	struct __attribute__((__packed__)) {
		uint8_t chip_number;
		uint16_t pec_error;
	} pec_data;

	pec_data.chip_number = chip_num;
	pec_data.pec_error = pec_error;

	endian_swap(&pec_data.pec_error, sizeof(pec_data.pec_error));

	can_msg_t msg = { .id = SEGMENT_PEC_ERROR_CANID,
			  .len = SEGMENT_PEC_ERROR_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &pec_data, sizeof(pec_data));

	queue_can_msg(msg);
}

void send_hv_plate_pec_error_message(const uint16_t pec_error)
{
	uint16_t pec_error_msg = pec_error;

	endian_swap(&pec_error_msg, sizeof(pec_error_msg));

	can_msg_t msg = { .id = HV_PLATE_PEC_ERROR_CANID,
			  .len = HV_PLATE_PEC_ERROR_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &pec_error_msg, sizeof(pec_error_msg));

	queue_can_msg(msg);
}

/**
 * @brief Sends ISO SPI status over CAN.
 *
 * @param status Pointer to isospi_status_t structure.
 */
void send_isospi_status_message(const isospi_status_t *status)
{
	struct __attribute__((__packed__)) {
		uint8_t state;
		uint8_t break_location;
		uint8_t ver_attempts;
		uint8_t recovery_successful;
	} msg_data;

	msg_data.state = (uint8_t)status->state;
	msg_data.break_location = status->break_chip;
	msg_data.ver_attempts = status->verification_attempts;
	msg_data.recovery_successful = status->recovery_successful;

	can_msg_t msg = { .id = ISOSPI_STS_CANID,
			  .len = ISOSPI_STS_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &msg_data, sizeof(msg_data));
	queue_can_msg(msg);
}

void send_onboard_therm_message(uint8_t chip_id, chipdata_t *chip_data)
{
	struct __attribute__((__packed__)) {
		uint8_t chip_id;
		uint16_t therm1_temp;
		uint16_t therm2_temp;
		uint16_t therm3_temp;
	} msg_data;

	msg_data.chip_id = chip_id;
	msg_data.therm1_temp =
		(uint16_t)(chip_data->on_board_temp[0] * 100) / 100;
	msg_data.therm2_temp =
		(uint16_t)(chip_data->on_board_temp[1] * 100) / 100;
	msg_data.therm3_temp =
		(uint16_t)(chip_data->on_board_temp[2] * 100) / 100;

	can_msg_t msg = { .id = ONBOARD_THERM_CANID,
			  .len = ONBOARD_THERM_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &msg_data, sizeof(msg_data));
	queue_can_msg(msg);
}

/**
 * @brief Send PWM duty cycle signals over CAN.
 *
 * @param pointer to signals
 */
void send_control_signals(const uint8_t *signals)
{
	struct __attribute__((__packed__)) {
		uint8_t fan_duty;
	} msg_data;

	msg_data.fan_duty = signals[0];

	can_msg_t msg = { .id = CONTROL_CANID,
			  .len = CONTROL_SIZE,
			  .data = { 0 } };

	memcpy(msg.data, &msg_data, sizeof(msg_data));
	queue_can_msg(msg);
}

/**
 * @brief Send HV plate diagnostic data over CAN.
 *
 * @param pointer to signals
 */
void send_hv_plate_diagnostic_data(const hv_plate_t *hv_plate)
{
	can_msg_t msg = { .id = HV_PLATE_DIAGNOSTIC_CANID,
			  .len = HV_PLATE_DIAGNOSTIC_SIZE,
			  .data = { 0 } };

	// Lower resolution of data
	float vreg = hv_plate->vreg * 100;
	float tmp1 = hv_plate->tmp1 * 100;
	float vref1p25 = hv_plate->vref1p25 * 100;
	float epad = hv_plate->epad * 100;
	float vdig = hv_plate->vdig * 100;
	float vdd = hv_plate->vdd * 100;
	float tmp2 = hv_plate->tmp2 * 100;
	float vdiv = hv_plate->vdiv * 100;

	bitstream_t bitstream;
	uint8_t bitstream_data[8];

	// Send first msg
	bitstream_init(&bitstream, bitstream_data, 8);
	bitstream_add(&bitstream, hv_plate->adbms_flags.raw, 12);
	bitstream_add(&bitstream, vreg, 12);
	bitstream_add(&bitstream, tmp1, 12);
	bitstream_add(&bitstream, vref1p25, 12);
	bitstream_add(&bitstream, hv_plate->osccnt, 16);
	memcpy(msg.data, &bitstream_data, HV_PLATE_DIAGNOSTIC_SIZE);
	queue_can_msg(msg);

	// Send second message
	msg.id++;
	bitstream_init(&bitstream, bitstream_data, 8);
	bitstream_add(&bitstream, epad, 12);
	bitstream_add(&bitstream, vdig, 12);
	bitstream_add(&bitstream, vdd, 12);
	bitstream_add(&bitstream, tmp2, 12);
	bitstream_add(&bitstream, vdiv, 12);
	memcpy(msg.data, &bitstream_data, HV_PLATE_DIAGNOSTIC_SIZE);
	queue_can_msg(msg);
}
