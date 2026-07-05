#include "adi2950_interaction.h"
#include "adi_bms_2950cmdlist.h"
#include "adi_bms_2950data.h"
#include "adi_bms_utility.h"
#include "adbmsCommonPal.h"
#include "hv_plate_isospi_recovery.h"
#include "isospi_recovery_common_config.h"
#include "u_tx_debug.h"
#include "can_messages_tx.h"
#include "c_utils.h"

static uint16_t hv_plate_pec_errors = { 0U };
static uint16_t prev_hv_plate_pec_errors = { 0U };

/**
 * @brief Increment PEC accumulator with saturation.
 *
 * @param ic Pointer to the adbms2950 data structure.
 * @param pec_mask_timer_expired True if accumulation is enabled.
 */
static void accumulate_hv_plate_pec_errors(cell_asic_2950 *ic,
					   const bool pec_mask_timer_expired)
{
	// Accumulate PEC errors only after startup mask timer ends
	if (pec_mask_timer_expired) {
		// Saturate at MAX_PEC_ERROR_ACCUM
		if (ic->pec_error_sum < ISOSPI_RECOVERY_MAX_PEC_ERROR_ACCUM) {
			ic->pec_error_sum += 1U;
		}
	}
}

/**
 * @brief Update the PEC errors and accumulation counter for the given register read.
 *
 * @param ic Pointer to the adbms2950 data structure.
 * @param type Register type that was read.
 */
static void update_hv_plate_pec_errors(cell_asic_2950 *ic, TYPE2950 type)
{
	const bool pec_mask_timer_expired =
		is_hv_plate_startup_pec_mask_timer_expired();

	// clang-format off
	switch (type)
	{
		case GPV1:
			if (ic->cccrc.vr_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 0U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] VR PEC %u", ic->cccrc.vr_pec);
			}
			break;
		case GPV2:
			if (ic->cccrc.rvr_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 1U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] RVR PEC %u", ic->cccrc.rvr_pec);
			}
			break;
		case Config2950:
			if (ic->cccrc.cfgr_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 2U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] CFGR PEC %u", ic->cccrc.cfgr_pec);
			}
			break;
		case Cr:
			if (ic->cccrc.cr_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 3U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] CR PEC %u", ic->cccrc.cr_pec);
			}
			break;
		case Vbat:
			if (ic->cccrc.vbat_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 4U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] VBAT PEC %u", ic->cccrc.vbat_pec);
			}
			break;
		case Ivbat:
			if (ic->cccrc.ivbat_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 5U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] IVBAT PEC %u", ic->cccrc.ivbat_pec);
			}
			break;
		case Oc:
			if (ic->cccrc.oc_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 6U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] OC PEC %u", ic->cccrc.oc_pec);
			}
			break;
		case AccCr:
			if (ic->cccrc.avgcr_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 7U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] AVGCR PEC %u", ic->cccrc.avgcr_pec);
			}
			break;
		case AccVbat:
			if (ic->cccrc.avgvbat_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 8U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] AVGVBAT PEC %u", ic->cccrc.avgvbat_pec);
			}
			break;
		case AccIvbat:
			if (ic->cccrc.avgivbat_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 9U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] AVGIVBAT PEC %u", ic->cccrc.avgivbat_pec);
			}
			break;
		case Aux2950:
			if (ic->cccrc.aux_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 10U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] AUX PEC %u", ic->cccrc.aux_pec);
			}
			break;
		case Flag:
			if (ic->cccrc.flag_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 11U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] FLAG PEC %u", ic->cccrc.flag_pec);
			}
			break;
		case Status2950:
			if (ic->cccrc.stat_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 12U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] STAT PEC %u", ic->cccrc.stat_pec);
			}
			break;
		case Comm2950:
			if (ic->cccrc.comm_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 13U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				printf("[HV_PLATE] COMM PEC %u", ic->cccrc.comm_pec);
			}
			break;
		case SID:
			if (ic->cccrc.sid2950_pec) {
				NER_SET_BIT(hv_plate_pec_errors, 14U);
				accumulate_hv_plate_pec_errors(ic, pec_mask_timer_expired);
				PRINTLN_WARNING("[HV_PLATE] SID2950 PED %u", ic->cccrc.sid2950_pec);
			}
			break;
		default:
			break;
	}
	// clang-format on
}

void send_hv_plate_pec_errors_message(void)
{
	uint16_t current_pec_errors = hv_plate_pec_errors;

	if (current_pec_errors != prev_hv_plate_pec_errors) {
		send_hv_plate_pec_errors(current_pec_errors);

		prev_hv_plate_pec_errors = current_pec_errors;
	}

	// Clear PEC errors for next cycle
	hv_plate_pec_errors = 0U;
}

void set_hv_plate_chips_isospi_line(cell_asic_2950 *ic, isospi_line_2950_ line)
{
	ic->isospi_line = line;
}

/**
 * @brief Wake the core of ADBMS2950 IC on the specified isoSPI line.
 *
 * @param line   isoSPI line to wake (LINE_A or LINE_B).
 * @param num_ic Number of ICs present on the specified isoSPI line.
 */
void adbms_wake_core_2950(isospi_line_2950_ line, uint8_t num_ic)
{
	switch (line) {
		case ADBMS2950_ISOSPI_LINE_A:
		case ADBMS2950_ISOSPI_LINE_B:
			adBmsLineCsLow2950(line);
			adBmsLineCsHigh2950(line);
			delay_us_2950(500);
			break;
		default:
			printf(" Invalid isoSPI line selected \n");
			break;
	}
}

void soft_reset_chip_2950(cell_asic_2950 *ic)
{
	uint8_t ic_count_a = 0U, ic_count_b = 0U;

	getIsoSPILineChipCount2950(TOTAL_IC_2950, ic, &ic_count_a, &ic_count_b);
	spiSendCmd2950(TOTAL_IC_2950, ic, SRST2950);
	if (ic_count_a > 0U) {
		adbms_wake_core_2950(ADBMS2950_ISOSPI_LINE_A, TOTAL_IC_2950);
	}

	if (ic_count_b > 0U) {
		adbms_wake_core_2950(ADBMS2950_ISOSPI_LINE_B, TOTAL_IC_2950);
	}
}

/**
 * @brief Read data from hv plate 2950.
 *
 * @param ic Pointer to the adbms2950 data structure.
 * @param command Command to issue to the chip.
 * @param type Register type to write to.
 * @param group Group of registers to write to.
 */
void read_adbms2950_data(cell_asic_2950 *ic, uint8_t command[2], TYPE2950 type,
			 GRP2950 group)
{
	adBmsReadData2950(TOTAL_IC_2950, ic, command, type, group);

	update_hv_plate_pec_errors(ic, type);
}

void snap_2950(cell_asic_2950 *ic)
{
	spiSendCmd2950(TOTAL_IC_2950, ic, SNAP2950);
}

void unsnap_2950(cell_asic_2950 *ic)
{
	spiSendCmd2950(TOTAL_IC_2950, ic, UNSNAP2950);
}

void start_adc_conversions(cell_asic_2950 *ic)
{
	cmd_description command;
	adBms2950_Adi1(TOTAL_IC_2950, ic, RD_ON2950, OPT8_C, &command);
	delay_ms(ADI1_delay_ms);
}

void write_config(cell_asic_2950 *ic, ACCI count)
{
	ic->tx_cfga.acci = count;
	ic->tx_cfga.vs1 = (VSB)VSMV_VREF1P25;
	ic->tx_cfga.vs2 = (VSB)VSMV_VREF1P25;
	ic->tx_cfga.vs7 = (VSB)VSMV_SGND;
	ic->tx_cfga.gpo4c = PULLED_UP_TRISTATED;
	ic->tx_cfga.gpo4od = OPEN_DRAIN;
	adBmsWriteData2950(TOTAL_IC_2950, ic, WRCFGA2950, Config2950, A_2950);
}

void write_clear_flags_2950(cell_asic_2950 *ic)
{
	for (int cic = 0; cic < TOTAL_IC_2950; cic++) {
		ic[cic].clflag.vdruv = CL_FLAG_SET2950;
		ic[cic].clflag.ocmm = CL_FLAG_SET2950;
		ic[cic].clflag.oc3l = CL_FLAG_SET2950;
		ic[cic].clflag.ocagd_clrm = CL_FLAG_SET2950;
		ic[cic].clflag.ocal = CL_FLAG_SET2950;
		ic[cic].clflag.oc1l = CL_FLAG_SET2950;

		ic[cic].clflag.vdduv = CL_FLAG_SET2950;
		ic[cic].clflag.noclk = CL_FLAG_SET2950;
		ic[cic].clflag.refflt = CL_FLAG_SET2950;
		ic[cic].clflag.ocbgd = CL_FLAG_SET2950;
		ic[cic].clflag.ocbl = CL_FLAG_SET2950;
		ic[cic].clflag.oc2l = CL_FLAG_SET2950;

		ic[cic].clflag.vregov = CL_FLAG_SET2950;
		ic[cic].clflag.vreguv = CL_FLAG_SET2950;
		ic[cic].clflag.vdigov = CL_FLAG_SET2950;
		ic[cic].clflag.vdiguv = CL_FLAG_SET2950;
		ic[cic].clflag.sed1 = CL_FLAG_SET2950;
		ic[cic].clflag.med1 = CL_FLAG_SET2950;
		ic[cic].clflag.sed2 = CL_FLAG_SET2950;
		ic[cic].clflag.med2 = CL_FLAG_SET2950;

		ic[cic].clflag.vdel = CL_FLAG_SET2950;
		ic[cic].clflag.vde = CL_FLAG_SET2950;
		ic[cic].clflag.spiflt = CL_FLAG_SET2950;
		ic[cic].clflag.reset = CL_FLAG_SET2950;
		ic[cic].clflag.thsd = CL_FLAG_SET2950;
		ic[cic].clflag.tmode = CL_FLAG_SET2950;
		ic[cic].clflag.oscflt = CL_FLAG_SET2950;
	}
	adBmsWriteData2950(TOTAL_IC_2950, ic, CLRFLAG2950, Clrflag2950,
			   NONE2950);
}

uint16_t read_conversion_count_registers(cell_asic_2950 *ic)
{
	read_adbms2950_data(ic, RDFLAG, Flag, NONE2950);
	return ic->flag.i1cnt;
	if (ic->cccrc.flag_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading conversion count register");
	}
}

void read_accumulated_current_vbat_registers(cell_asic_2950 *ic)
{
	read_adbms2950_data(ic, RDIVB1ACC, AccIvbat,
			    NONE2950); /* Accumulated Battery Voltage Group*/
}

void read_current_vbat_registers(cell_asic_2950 *ic)
{
	read_adbms2950_data(ic, RDIVB1, Ivbat,
			    NONE2950); /* Battery Voltage and Current Group*/
}

void read_v7_register(cell_asic_2950 *ic)
{
	adBms2950_Adv(1, ic, OW_OFF, SM_V7_V9);
	delay_ms(Polling_Delay_ms2950);

	read_adbms2950_data(ic, RDV1C, GPV1, C_2950);
}

void read_v2_register(cell_asic_2950 *ic)
{
	adBms2950_Adv(1, ic, OW_OFF, SM_V2);
	delay_ms(Polling_Delay_ms2950);

	read_adbms2950_data(ic, RDV1A, GPV1, A_2950);
}

void read_flag_register(cell_asic_2950 *ic)
{
	read_adbms2950_data(ic, RDFLAG, Flag, FLAG_NOERR);
}

void poll_and_read_aux_registers(cell_asic_2950 *ic)
{
	spiSendCmd2950(TOTAL_IC_2950, ic, sADX);
	// Poll on conversion to block thread
	ic[0].pladc_count = adBmsPollAdc2950(TOTAL_IC_2950, ic, PLX);

	// Read all relevant register groups
	read_adbms2950_data(ic, RDXA, Aux2950, A_2950);

	read_adbms2950_data(ic, RDXB, Aux2950, B_2950);

	read_adbms2950_data(ic, RDXC, Aux2950, C_2950);

	spiSendCmd2950(TOTAL_IC_2950, ic, CLRVX);
}

void set_gpo(cell_asic_2950 *ic, GPO_2950 gpo)
{
	switch (gpo) {
		case GPO1_2950:
			ic->tx_cfga.gpo1c = PULLED_DOWN;
			break;
		case GPO2_2950:
			// NOTE: temporary change for enabling HV readings on devkit
			// GPO2 is PUSH_PULL
			ic->tx_cfga.gpo2od = PUSH_PULL;
			ic->tx_cfga.gpo2c = PULLED_UP_TRISTATED;
			break;
		case GPO3_2950:
			ic->tx_cfga.gpo3c = PULLED_DOWN;
			break;
		case GPO4_2950:
			ic->tx_cfga.gpo4c = PULLED_DOWN;
			break;
		case GPO5_2950:
			ic->tx_cfga.gpo5c = PULLED_DOWN;
			break;
		case GPO6_2950:
			ic->tx_cfga.gpo6c = PULLED_DOWN;
			break;
		default:
			break;
	}

	adBmsWriteData2950(TOTAL_IC_2950, ic, WRCFGA2950, Config2950, A_2950);
	read_adbms2950_data(ic, RDCFGA2950, Config2950, A_2950);
}

void reset_gpo(cell_asic_2950 *ic, GPO_2950 gpo)
{
	switch (gpo) {
		case GPO1_2950:
			ic->tx_cfga.gpo1c = PULLED_UP_TRISTATED;
			break;
		case GPO2_2950:
			// NOTE: temporary change for enabling HV readings on devkit
			// GPO2 is PUSH_PULL
			ic->tx_cfga.gpo2od = PUSH_PULL;
			ic->tx_cfga.gpo2c = PULLED_DOWN;
			break;
		case GPO3_2950:
			ic->tx_cfga.gpo3c = PULLED_UP_TRISTATED;
			break;
		case GPO4_2950:
			ic->tx_cfga.gpo4c = PULLED_UP_TRISTATED;
			break;
		case GPO5_2950:
			ic->tx_cfga.gpo5c = PULLED_UP_TRISTATED;
			break;
		case GPO6_2950:
			ic->tx_cfga.gpo6c = PULLED_UP_TRISTATED;
			break;
		default:
			break;
	}

	adBmsWriteData2950(TOTAL_IC_2950, ic, WRCFGA2950, Config2950, A_2950);
	read_adbms2950_data(ic, RDCFGA2950, Config2950, A_2950);
}
