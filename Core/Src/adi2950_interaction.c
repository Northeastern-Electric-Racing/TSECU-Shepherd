#include "adi2950_interaction.h"
#include "pal.h"
#include "u_tx_debug.h"
#include "can_messages.h"
#include "c_utils.h"

#define TOTAL_IC_2950 1

static uint16_t hv_plate_pec_errors = { 0U };
static uint16_t prev_hv_plate_pec_errors = { 0U };

/**
 * @brief Update the PEC errors and accumulation counter for the given register read.
 *
 * @param ic Pointer to the adbms2950 data structure.
 * @param type Register type that was read.
 */
static void update_hv_plate_pec_errors(cell_asic_2950 *ic, TYPE2950 type)
{
	// clang-format off
	switch (type)
	{
		case GPV1:
			NER_SET_BIT(hv_plate_pec_errors, 0U);
			printf("[HV_PLATE] VR PEC %u, ", ic->cccrc.vr_pec);
			break;
		case GPV2:
			NER_SET_BIT(hv_plate_pec_errors, 1U);
			printf("[HV_PLATE] RVR PEC %u, ", ic->cccrc.rvr_pec);
			break;
		case Config2950:
			NER_SET_BIT(hv_plate_pec_errors, 2U);
			printf("[HV_PLATE] CFGR PEC %u, ", ic->cccrc.cfgr_pec);
			break;
		case Cr:
			NER_SET_BIT(hv_plate_pec_errors, 3U);
			printf("[HV_PLATE] CR PEC %u, ", ic->cccrc.cr_pec);
			break;
		case Vbat:
			NER_SET_BIT(hv_plate_pec_errors, 4U);
			printf("[HV_PLATE] VBAT PEC %u, ", ic->cccrc.vbat_pec);
			break;
		case Ivbat:
			NER_SET_BIT(hv_plate_pec_errors, 5U);
			printf("[HV_PLATE] IVBAT PEC %u, ", ic->cccrc.ivbat_pec);
			break;
		case Oc:
			NER_SET_BIT(hv_plate_pec_errors, 6U);
			printf("[HV_PLATE] OC PEC %u, ", ic->cccrc.oc_pec);
			break;
		case AccCr:
			NER_SET_BIT(hv_plate_pec_errors, 7U);
			printf("[HV_PLATE] AVGCR PEC %u, ", ic->cccrc.avgcr_pec);
			break;
		case AccVbat:
			NER_SET_BIT(hv_plate_pec_errors, 8U);
			printf("[HV_PLATE] AVGVBAT PEC %u, ", ic->cccrc.avgvbat_pec);
			break;
		case AccIvbat:
			NER_SET_BIT(hv_plate_pec_errors, 9U);
			printf("[HV_PLATE] AVGIVBAT PEC %u, ", ic->cccrc.avgivbat_pec);
			break;
		case Aux2950:
			NER_SET_BIT(hv_plate_pec_errors, 10U);
			printf("[HV_PLATE] AUX PEC %u, ", ic->cccrc.aux_pec);
			break;
		case Flag:
			NER_SET_BIT(hv_plate_pec_errors, 11U);
			printf("[HV_PLATE] FLAG PEC %u, ", ic->cccrc.flag_pec);
			break;
		case Status2950:
			NER_SET_BIT(hv_plate_pec_errors, 12U);
			printf("[HV_PLATE] STAT PEC %u, ", ic->cccrc.stat_pec);
			break;
		case Comm2950:
			NER_SET_BIT(hv_plate_pec_errors, 13U);
			printf("[HV_PLATE] COMM PEC %u, ", ic->cccrc.comm_pec);
			break;
		case SID:
			NER_SET_BIT(hv_plate_pec_errors, 14U);
			printf("[HV_PLATE] SID2950 PED %u, ", ic->cccrc.sid2950_pec);
			break;
		default:
			break;
	}
	// clang-format on
}

void send_hv_plate_pec_errors(void)
{
	uint16_t current_pec_errors = hv_plate_pec_errors;

	if (current_pec_errors != prev_hv_plate_pec_errors) {
		send_hv_plate_pec_error_message(current_pec_errors);

		prev_hv_plate_pec_errors = current_pec_errors;
	}

	// Clear PEC errors for next cycle
	current_pec_errors &= 0x0000U;
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
	Delay_ms2950(ADI1_delay_ms);
}

void write_config(cell_asic_2950 *ic, ACCI count)
{
	ic->tx_cfga.acci = count;
	ic->tx_cfga.vs7 = (VSB)VSMV_SGND;
	adBmsWakeupIc2950(1);
	adBmsWriteData2950(TOTAL_IC_2950, ic, WRCFGA2950, Config2950, A_2950);
}

uint16_t read_conversion_count_registers(cell_asic_2950 *ic)
{
	adBmsWakeupIc2950(1);
	read_adbms2950_data(ic, RDFLAG, Flag, NONE2950);
	return ic->flag.i1cnt;
	if (ic->cccrc.flag_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading conversion count register");
	}
}

void read_accumulated_current_vbat_registers(cell_asic_2950 *ic)
{
	adBmsWakeupIc2950(1);
	read_adbms2950_data(ic, RDIVB1ACC, AccIvbat,
			    NONE2950); /* Accumulated Battery Voltage Group*/
	if (ic->cccrc.avgivbat_pec != 0) {
		PRINTLN_ERROR(
			"PEC Error in reading accumulated current and battery register");
	}
}

void read_v7_v9_registers(cell_asic_2950 *ic)
{
	adBmsWakeupIc2950(1);
	adBms2950_Adv(1, ic, OW_OFF, SM_V7_V9);
	Delay_ms2950(Polling_Delay_ms2950);

	adBmsWakeupIc2950(1);
	read_adbms2950_data(ic, RDV1D, GPV1, D_2950);
	if (ic->cccrc.vr_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading V7 and V9 registers");
	}
}

void read_v2_register(cell_asic_2950 *ic)
{
	adBmsWakeupIc2950(1);
	adBms2950_Adv(1, ic, OW_OFF, SM_V2);
	Delay_ms2950(Polling_Delay_ms2950);

	adBmsWakeupIc2950(1);
	read_adbms2950_data(ic, RDV1A, GPV1, A_2950);
	if (ic->cccrc.vr_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading V2 register");
	}
}

void read_flag_register(cell_asic_2950 *ic)
{
	adBmsWakeupIc2950(1);
	read_adbms2950_data(ic, RDFLAG, Flag, FLAG_NOERR);
	if (ic->cccrc.flag_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading flag register");
	}
}

void read_aux_registers(cell_asic_2950 *ic)
{
	spiSendCmd2950(TOTAL_IC_2950, ic, sADX);
	// Poll on conversion to block thread
	ic[0].pladc_count = adBmsPollAdc2950(TOTAL_IC_2950, ic, PLX);

	// Read all relevant register groups
	read_adbms2950_data(ic, RDXA, Aux2950, A_2950);
	read_adbms2950_data(ic, RDXB, Aux2950, B_2950);
	read_adbms2950_data(ic, RDXC, Aux2950, C_2950);

	if (ic->cccrc.aux_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading auxiliary registers");
	}
	spiSendCmd2950(TOTAL_IC_2950, ic, CLRVX);
}

void set_gpo(cell_asic_2950 *ic, GPO_2950 gpo)
{
	switch (gpo) {
		case GPO1_2950:
			ic->tx_cfga.gpo1c = PULLED_DOWN;
			break;
		case GPO2_2950:
			ic->tx_cfga.gpo2c = PULLED_DOWN;
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

	adBmsWakeupIc2950(TOTAL_IC_2950);
	adBmsWriteData2950(TOTAL_IC_2950, ic, WRCFGA2950, Config2950, A_2950);
	read_adbms2950_data(ic, RDCFGA2950, Config2950, A_2950);
	if (ic->cccrc.cfgr_pec != 0) {
		PRINTLN_ERROR("PEC Error in writing GPO configuration");
	}
}

void reset_gpo(cell_asic_2950 *ic, GPO_2950 gpo)
{
	switch (gpo) {
		case GPO1_2950:
			ic->tx_cfga.gpo1c = PULLED_UP_TRISTATED;
			break;
		case GPO2_2950:
			ic->tx_cfga.gpo2c = PULLED_UP_TRISTATED;
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

	adBmsWakeupIc2950(TOTAL_IC_2950);
	adBmsWriteData2950(TOTAL_IC_2950, ic, WRCFGA2950, Config2950, A_2950);
	read_adbms2950_data(ic, RDCFGA2950, Config2950, A_2950);

	if (ic->cccrc.cfgr_pec != 0) {
		PRINTLN_ERROR("PEC Error in writing GPO configuration");
	}
}