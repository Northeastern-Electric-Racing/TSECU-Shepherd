
#include "adi2950_interaction.h"
#include "pal.h"

#define TOTAL_IC_2950 1

/* Starts a Single Shot Measurement */
static void start_conversion(cell_asic_2950 ic) {
    cmd_description command;
    adBmsWakeupIc2950(TOTAL_IC_2950);
	// Send ADI1 command with REDUNDANT_MEASUREMENT2950 off and SingleShot and start timer for first conversion
	adBms2950_Adi1(TOTAL_IC_2950, &ic, RD_OFF2950, OPT0_SS, &command);
    // adBms2950_Adi2(TOTAL_IC_2950, &ic, OPT0_SS, &command); TODO: add redundant readings
	Delay_ms2950(ADI1_delay_ms);
}

void read_current_registers(cell_asic_2950 ic, SPI_HandleTypeDef *hspi) {
    start_conversion(ic);
    adBmsWakeupIc2950(TOTAL_IC_2950);
    adBmsReadData2950(TOTAL_IC_2950, &ic, RDI, Cr,
			  NONE2950); /* Current Register Group */
}

void read_vbat_regsisters(cell_asic_2950 ic, SPI_HandleTypeDef *hspi) {
    start_conversion(ic);
    adBmsWakeupIc2950(TOTAL_IC_2950);
    adBmsReadData2950(TOTAL_IC_2950, &ic, RDVB, Vbat,
			  NONE2950); /* Battery Voltage Group*/
}

void read_ivbat_regsisters(cell_asic_2950 ic, SPI_HandleTypeDef *hspi) {
    start_conversion(ic);
    adBmsWakeupIc2950(TOTAL_IC_2950);
    adBmsReadData2950(TOTAL_IC_2950, &ic, RDIVB1, Ivbat,
			  NONE2950); /* Battery Voltage Group*/
}

