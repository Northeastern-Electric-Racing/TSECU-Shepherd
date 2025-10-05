
#include "adi2950_interaction.h"
#include "pal.h"

#define TOTAL_IC_2950 1

/* Starts a Single Shot Measurement */
static void start_conversion(cell_asic_2950 ic)
{
	cmd_description command;
	adBmsWakeupIc2950(TOTAL_IC_2950);
	// Send ADI1 command with REDUNDANT_MEASUREMENT2950 off and SingleShot and start timer for first conversion
	adBms2950_Adi1(TOTAL_IC_2950, &ic, RD_OFF2950, OPT0_SS, &command);
	// adBms2950_Adi2(TOTAL_IC_2950, &ic, OPT0_SS, &command); TODO: add redundant readings
	Delay_ms2950(ADI1_delay_ms);
}

void read_current_registers(cell_asic_2950 ic, SPI_HandleTypeDef *hspi)
{
	start_conversion(ic);
	adBmsWakeupIc2950(TOTAL_IC_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDI, Cr,
			  NONE2950); /* Current Register Group */
}

void read_vbat_regsisters(cell_asic_2950 ic, SPI_HandleTypeDef *hspi)
{
	start_conversion(ic);
	adBmsWakeupIc2950(TOTAL_IC_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDVB, Vbat,
			  NONE2950); /* Battery Voltage Group*/
}

void read_ivbat_regsisters(cell_asic_2950 ic, SPI_HandleTypeDef *hspi)
{
	start_conversion(ic);
	adBmsWakeupIc2950(TOTAL_IC_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDIVB1, Ivbat,
			  NONE2950); /* Battery Voltage Group*/
}

void read_vr_registers(cell_asic_2950 ic, SPI_HandleTypeDef *hspi)
{
	start_conversion(ic);

	adBmsReadData2950(TOTAL_IC_2950, &ic, RDV1A, GPV1, A_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDV1B, GPV1, B_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDV1C, GPV1, C_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDV1C, GPV1, D_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDV1C, GPV1, E_2950);

	adBmsReadData2950(TOTAL_IC_2950, &ic, RDV2A, GPV2, A_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDV2B, GPV2, B_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDV2C, GPV2, C_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDV2B, GPV2, D_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDV2C, GPV2, E_2950);
}

void set_gpo(cell_asic_2950 ic, SPI_HandleTypeDef *hspi, GPO_2950 gpo)
{
	switch (gpo) {
	case GPO1_2950:
		ic.tx_cfga.gpo1od = PUSH_PULL;
		ic.tx_cfga.gpo1c = PULLED_UP_TRISTATED;
		break;
	case GPO2_2950:
		ic.tx_cfga.gpo2od = PUSH_PULL;
		ic.tx_cfga.gpo2c = PULLED_UP_TRISTATED;
		break;
	case GPO3_2950:
		ic.tx_cfga.gpo3od = PUSH_PULL;
		ic.tx_cfga.gpo3c = PULLED_UP_TRISTATED;
		break;
	case GPO4_2950:
		ic.tx_cfga.gpo4od = PUSH_PULL;
		ic.tx_cfga.gpo4c = PULLED_UP_TRISTATED;
		break;
	case GPO5_2950:
		ic.tx_cfga.gpo5od = PUSH_PULL;
		ic.tx_cfga.gpo5c = PULLED_UP_TRISTATED;
		break;
	case GPO6_2950:
		ic.tx_cfga.gpo6od = PUSH_PULL;
		ic.tx_cfga.gpo6c = PULLED_UP_TRISTATED;
		break;
	default:
		break;
	}

	adBmsWakeupIc2950(TOTAL_IC_2950);
	adBmsWriteData2950(TOTAL_IC_2950, &ic, WRCFGA2950, Config2950, A_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDCFGA2950, Config2950, A_2950);
}

void reset_gpo(cell_asic_2950 ic, SPI_HandleTypeDef *hspi, GPO_2950 gpo)
{
	switch (gpo) {
	case GPO1_2950:
        ic.tx_cfga.gpo1od = PUSH_PULL;

		ic.tx_cfga.gpo1c = PULLED_DOWN;
		break;
	case GPO2_2950:
        ic.tx_cfga.gpo2od = PUSH_PULL;
		ic.tx_cfga.gpo2c = PULLED_DOWN;
		break;
	case GPO3_2950:
        ic.tx_cfga.gpo3od = PUSH_PULL;
		ic.tx_cfga.gpo3c = PULLED_DOWN;
		break;
	case GPO4_2950:
        ic.tx_cfga.gpo4od = PUSH_PULL;
		ic.tx_cfga.gpo4c = PULLED_DOWN;
		break;
	case GPO5_2950:
        ic.tx_cfga.gpo5od = PUSH_PULL;
		ic.tx_cfga.gpo5c = PULLED_DOWN;
		break;
	case GPO6_2950:
        ic.tx_cfga.gpo6od = PUSH_PULL;
		ic.tx_cfga.gpo6c = PULLED_DOWN;
		break;
	default:
		break;
	}

	adBmsWakeupIc2950(TOTAL_IC_2950);
	adBmsWriteData2950(TOTAL_IC_2950, &ic, WRCFGA2950, Config2950, A_2950);
	adBmsReadData2950(TOTAL_IC_2950, &ic, RDCFGA2950, Config2950, A_2950);
}
