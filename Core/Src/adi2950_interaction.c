
#include "adi2950_interaction.h"
#include "pal.h"
#include "u_tx_debug.h"

#define TOTAL_IC_2950 1

void start_adc_conversions(cell_asic_2950 *ic)
{
	cmd_description command;
	adBms2950_Adi1(TOTAL_IC_2950, ic, RD_ON2950, OPT8_C, &command);
	Delay_ms2950(ADI1_delay_ms);
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
