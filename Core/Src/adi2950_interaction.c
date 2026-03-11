#include "adi2950_interaction.h"
#include "pal.h"
#include "u_tx_debug.h"
#include "can_messages.h"

#define TOTAL_IC_2950 1

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
	ic->tx_cfga.vs1 = (VSB)VSMV_VREF1P25;
	ic->tx_cfga.vs2 = (VSB)VSMV_VREF1P25;
	ic->tx_cfga.vs7 = (VSB)VSMV_SGND;
	adBmsWakeupIc2950(1);
	adBmsWriteData2950(TOTAL_IC_2950, ic, WRCFGA2950, Config2950, A_2950);
}

uint16_t read_conversion_count_registers(cell_asic_2950 *ic)
{
	adBmsWakeupIc2950(1);
	adBmsReadData2950(TOTAL_IC_2950, ic, RDFLAG, Flag, NONE2950);
	return ic->flag.i1cnt;
	if (ic->cccrc.flag_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading conversion count register");
	}
}

void read_accumulated_current_vbat_registers(cell_asic_2950 *ic)
{
	adBmsWakeupIc2950(1);
	adBmsReadData2950(TOTAL_IC_2950, ic, RDIVB1ACC, AccIvbat,
			  NONE2950); /* Accumulated Battery Voltage Group*/
	if (ic->cccrc.avgivbat_pec != 0) {
		PRINTLN_ERROR(
			"PEC Error in reading accumulated current and battery register");
	}
}

void trigger_vr_converion(cell_asic_2950 *ic) {
	adBmsWakeupIc2950(1);
	adBms2950_Adv(1, ic, OW_OFF, RR_VCH0_VCH8);
	Delay_ms2950(Polling_Delay_ms2950);
}

void read_v7_register(cell_asic_2950 *ic)
{
	adBmsWakeupIc2950(1);
	adBmsReadData2950(TOTAL_IC_2950, ic, RDV1C, GPV1, C_2950);
	if (ic->cccrc.vr_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading V7 and V9 registers");
	}
}

void read_v2_register(cell_asic_2950 *ic)
{
	adBmsWakeupIc2950(1);
	adBmsReadData2950(TOTAL_IC_2950, ic, RDV1A, GPV1, A_2950);
	if (ic->cccrc.vr_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading V2 register");
	}
}

void read_flag_register(cell_asic_2950 *ic)
{
	adBmsWakeupIc2950(1);
	adBmsReadData2950(TOTAL_IC_2950, ic, RDFLAG, Flag, FLAG_NOERR);
	if (ic->cccrc.flag_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading flag register");
	}
}

void poll_and_read_aux_registers(cell_asic_2950 *ic)
{
	spiSendCmd2950(TOTAL_IC_2950, ic, sADX);
	// Poll on conversion to block thread
	ic[0].pladc_count = adBmsPollAdc2950(TOTAL_IC_2950, ic, PLX);

	// Read all relevant register groups
	adBmsReadData2950(TOTAL_IC_2950, ic, RDXA, Aux2950, A_2950);
	adBmsReadData2950(TOTAL_IC_2950, ic, RDXB, Aux2950, B_2950);
	adBmsReadData2950(TOTAL_IC_2950, ic, RDXC, Aux2950, C_2950);

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

	adBmsWakeupIc2950(TOTAL_IC_2950);
	adBmsWriteData2950(TOTAL_IC_2950, ic, WRCFGA2950, Config2950, A_2950);
	adBmsReadData2950(TOTAL_IC_2950, ic, RDCFGA2950, Config2950, A_2950);
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

	adBmsWakeupIc2950(TOTAL_IC_2950);
	adBmsWriteData2950(TOTAL_IC_2950, ic, WRCFGA2950, Config2950, A_2950);
	adBmsReadData2950(TOTAL_IC_2950, ic, RDCFGA2950, Config2950, A_2950);

	if (ic->cccrc.cfgr_pec != 0) {
		PRINTLN_ERROR("PEC Error in writing GPO configuration");
	}
}

void count_hv_plate_pec_errors(cell_asic_2950 *ic)
{
	uint8_t pec_error_count =
		(uint8_t)(ic->cccrc.cfgr_pec + ic->cccrc.cr_pec +
			  ic->cccrc.vbat_pec + ic->cccrc.ivbat_pec +
			  ic->cccrc.oc_pec + ic->cccrc.avgcr_pec +
			  ic->cccrc.avgvbat_pec + ic->cccrc.avgivbat_pec +
			  ic->cccrc.aux_pec + ic->cccrc.flag_pec +
			  ic->cccrc.vr_pec + ic->cccrc.rvr_pec +
			  ic->cccrc.comm_pec + ic->cccrc.stat_pec +
			  ic->cccrc.sid2950_pec);

	if (pec_error_count > 0U) {
		PRINTLN_ERROR("HV Plate PEC Error: Count: %u\n",
			      pec_error_count);

		// if only a few PEC errors happened, print which registers they came from
		if (pec_error_count < 10U) {
			if (ic->cccrc.cfgr_pec > 0U) {
				printf("[HV_PLATE] CFGR PEC %u, ",
				       ic->cccrc.cfgr_pec);
			}
			if (ic->cccrc.cr_pec > 0U) {
				printf("[HV_PLATE] CR PEC %u, ",
				       ic->cccrc.cr_pec);
			}
			if (ic->cccrc.vbat_pec > 0U) {
				printf("[HV_PLATE] VBAT PEC %u, ",
				       ic->cccrc.vbat_pec);
			}
			if (ic->cccrc.ivbat_pec > 0U) {
				printf("[HV_PLATE] IVBAT PEC %u, ",
				       ic->cccrc.ivbat_pec);
			}
			if (ic->cccrc.oc_pec > 0U) {
				printf("[HV_PLATE] OC PEC %u, ",
				       ic->cccrc.oc_pec);
			}
			if (ic->cccrc.avgcr_pec > 0U) {
				printf("[HV_PLATE] AVGCR PEC %u, ",
				       ic->cccrc.avgcr_pec);
			}
			if (ic->cccrc.avgvbat_pec > 0U) {
				printf("[HV_PLATE] AVGVBAT PEC %u, ",
				       ic->cccrc.avgvbat_pec);
			}
			if (ic->cccrc.avgivbat_pec > 0U) {
				printf("[HV_PLATE] AVGIVBAT PEC %u, ",
				       ic->cccrc.avgivbat_pec);
			}
			if (ic->cccrc.aux_pec > 0U) {
				printf("[HV_PLATE] AUX PEC %u, ",
				       ic->cccrc.aux_pec);
			}
			if (ic->cccrc.flag_pec > 0U) {
				printf("[HV_PLATE] FLAG PEC %u, ",
				       ic->cccrc.flag_pec);
			}
			if (ic->cccrc.vr_pec > 0U) {
				printf("[HV_PLATE] VR PEC %u, ",
				       ic->cccrc.vr_pec);
			}
			if (ic->cccrc.rvr_pec > 0U) {
				printf("[HV_PLATE] RVR PEC %u, ",
				       ic->cccrc.rvr_pec);
			}
			if (ic->cccrc.comm_pec > 0U) {
				printf("[HV_PLATE] COMM PEC %u, ",
				       ic->cccrc.comm_pec);
			}
			if (ic->cccrc.stat_pec > 0U) {
				printf("[HV_PLATE] STAT PEC %u, ",
				       ic->cccrc.stat_pec);
			}
			if (ic->cccrc.sid2950_pec > 0U) {
				printf("[HV_PLATE] SID2950 PED %u, ",
				       ic->cccrc.sid2950_pec);
			}
		}

		send_hv_plate_pec_error_message(pec_error_count);
	}

	ic->cccrc.cfgr_pec = 0U;
	ic->cccrc.cr_pec = 0U;
	ic->cccrc.vbat_pec = 0U;
	ic->cccrc.ivbat_pec = 0U;
	ic->cccrc.oc_pec = 0U;
	ic->cccrc.avgcr_pec = 0U;
	ic->cccrc.avgvbat_pec = 0U;
	ic->cccrc.avgivbat_pec = 0U;
	ic->cccrc.aux_pec = 0U;
	ic->cccrc.flag_pec = 0U;
	ic->cccrc.vr_pec = 0U;
	ic->cccrc.rvr_pec = 0U;
	ic->cccrc.comm_pec = 0U;
	ic->cccrc.stat_pec = 0U;
	ic->cccrc.sid2950_pec = 0U;
}