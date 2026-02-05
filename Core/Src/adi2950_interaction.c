#include "adi2950_interaction.h"
#include "pal.h"
#include "u_tx_debug.h"

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

void set_accumulation_count(cell_asic_2950 *ic, ACCI count)
{
	ic->tx_cfga.acci = count;
	adBmsWakeupIc2950(1);
	adBmsWriteData2950(TOTAL_IC_2950, ic, WRCFGA2950, Config2950, A_2950);
	if (ic->cccrc.cfgr_pec != 0) {
		PRINTLN_ERROR("PEC: %d", ic->cccrc.cfgr_pec);
		PRINTLN_ERROR("PEC Error in writing Accumulation Count");
	}
}

uint16_t read_conversion_count_registers(cell_asic_2950 *ic)
{
	adBmsReadData2950(TOTAL_IC_2950, ic, RDFLAG, Flag, NONE2950);
	return ic->flag.i1cnt;
	if (ic->cccrc.flag_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading conversion count register");
	}
}

void read_accumulated_current_vbat_registers(cell_asic_2950 *ic)
{
	adBmsReadData2950(TOTAL_IC_2950, ic, RDIVB1ACC, AccIvbat,
			  NONE2950); /* Accumulated Battery Voltage Group*/
	if (ic->cccrc.avgivbat_pec != 0) {
		PRINTLN_ERROR(
			"PEC Error in reading accumulated current and battery register");
	}
}

void read_v7_v9_registers(cell_asic_2950 *ic)
{
	adBmsReadData2950(TOTAL_IC_2950, ic, RDV1D, GPV1, D_2950);
	if (ic->cccrc.vr_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading V7 and V9 registers");
	}
}

void read_v2_v3_registers(cell_asic_2950 *ic)
{
	adBmsReadData2950(TOTAL_IC_2950, ic, RDV1A, GPV1, A_2950);
	if (ic->cccrc.vr_pec != 0) {
		PRINTLN_ERROR("PEC Error in reading V7 and V9 registers");
	}
}

void read_flag_register(cell_asic_2950 *ic)
{
	adBmsReadData2950(TOTAL_IC_2950, ic, RDFLAG, Flag, FLAG_NOERR);
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
	adBmsReadData2950(TOTAL_IC_2950, ic, RDCFGA2950, Config2950, A_2950);

	if (ic->cccrc.cfgr_pec != 0) {
		PRINTLN_ERROR("PEC Error in writing GPO configuration");
	}
}
