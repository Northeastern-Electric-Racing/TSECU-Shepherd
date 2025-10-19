#include "adi2950_interaction.h"
#include "hv_plate.h"

#define HV_CTRL_GPO GPIO4_2950

static float get_current_conversion(uint32_t data)
{
	float current;
	current = 1e-6 * ((int32_t)(data << (32 - 24)) >> (32 - 24));
	return current / (float)SHUNT_RESISTANCE;
}

static float get_voltage_conversion(int data)
{
	float voltage;
	voltage = 100e-6 * (int16_t)data;
	return voltage;
}

void init_hv_plate_chip(cell_asic_2950 ic)
{
	// TODO: iron out config (mostly taken from adi code)
	ic.tx_cfga.gpo1c = PULLED_UP_TRISTATED;
	ic.tx_cfga.gpo2c = PULLED_UP_TRISTATED;
	ic.tx_cfga.gpo3c = PULLED_UP_TRISTATED;
	ic.tx_cfga.gpo4c = PULLED_UP_TRISTATED;
	ic.tx_cfga.gpo5c = PULLED_UP_TRISTATED;
	ic.tx_cfga.gpo6c = PULLED_UP_TRISTATED;

	ic.tx_cfga.gpo1od = PUSH_PULL;
	ic.tx_cfga.gpo2od = PUSH_PULL;
	ic.tx_cfga.gpo3od = PUSH_PULL;
	ic.tx_cfga.gpo4od = PUSH_PULL;
	ic.tx_cfga.gpo5od = PUSH_PULL;
	ic.tx_cfga.gpo6od = PUSH_PULL;

	ic.tx_cfga.vs1 = VSM_SGND;
	ic.tx_cfga.vs2 = VSM_SGND;
	ic.tx_cfga.vs3 = VSMV_SGND;
	ic.tx_cfga.vs4 = VSMV_SGND;
	ic.tx_cfga.vs5 = VSMV_SGND;
	ic.tx_cfga.vs6 = VSMV_SGND;
	ic.tx_cfga.vs7 = VSMV_SGND;
	ic.tx_cfga.vs8 = VSMV_SGND;
	ic.tx_cfga.vs9 = VSMV_SGND;
	ic.tx_cfga.vs10 = VSMV_SGND;

	ic.tx_cfga.injosc = INJOSC0_NORMAL;
	ic.tx_cfga.injmon = INJMON0_NORMAL;
	ic.tx_cfga.injts = NO_THSD;
	ic.tx_cfga.injecc = NO_ECC;
	ic.tx_cfga.injtm = NO_TMODE;

	ic.tx_cfga.soak = SOAK_DISABLE;
	ic.tx_cfga.ocen = OC_DISABLE;
	ic.tx_cfga.gpio1fe = FAULT_STATUS_DISABLE;
	ic.tx_cfga.spi3w = FOUR_WIRE;

	ic.tx_cfga.acci = ACCI_8;
	ic.tx_cfga.commbk = COMMBK_OFF;
	ic.tx_cfga.vb1mux = SINGLE_ENDED_SGND;
	ic.tx_cfga.vb2mux = SINGLE_ENDED_SGND;

	//CFGB
	ic.tx_cfgb.gpio1c = PULL_DOWN_OFF;
	ic.tx_cfgb.gpio2c = PULL_DOWN_OFF;
	ic.tx_cfgb.gpio3c = PULL_DOWN_OFF;
	ic.tx_cfgb.gpio4c = PULL_DOWN_OFF;

	ic.tx_cfgb.oc1th = 0x0;
	ic.tx_cfgb.oc2th = 0x0;
	ic.tx_cfgb.oc3th = 0x0;

	ic.tx_cfgb.oc1ten = NORMAL_INPUT;
	ic.tx_cfgb.oc2ten = NORMAL_INPUT;
	ic.tx_cfgb.oc3ten = NORMAL_INPUT;

	ic.tx_cfgb.ocdgt = OCDGT0_1oo1;
	ic.tx_cfgb.ocdp = OCDP0_NORMAL;
	ic.tx_cfgb.reften = NORMAL_INPUT;
	ic.tx_cfgb.octsel = OCTSEL0_OCxADC_P140_REFADC_M20;

	ic.tx_cfgb.ocod = PUSH_PULL;
	ic.tx_cfgb.oc1gc = GAIN_1;
	ic.tx_cfgb.oc2gc = GAIN_1;
	ic.tx_cfgb.oc3gc = GAIN_1;
	ic.tx_cfgb.ocmode = OCMODE0_DISABLED2950;
	ic.tx_cfgb.ocax = OCABX_ACTIVE_HIGH;
	ic.tx_cfgb.ocbx = OCABX_ACTIVE_HIGH;

	ic.tx_cfgb.diagsel = DIAGSEL0_IAB_VBAT;
	ic.tx_cfgb.gpio2eoc = EOC_DISABLED2950;
}

float get_pack_current(bms_t *bmsdata, SPI_HandleTypeDef *hspi)
{
	read_current_registers(bmsdata->plate_chip, hspi);
	return get_current_conversion(bmsdata->plate_chip.i.i1);
}

// TODO: finish API
float get_batt_voltage(bms_t *bmsdata, SPI_HandleTypeDef *hspi)
{
	read_vbat_regsisters(bmsdata->plate_chip, hspi);
	float avg_volts =
		(get_voltage_conversion(bmsdata->plate_chip.vbat.vbat1) +
		 get_voltage_conversion(bmsdata->plate_chip.vbat.vbat2)) /
		2;
	return avg_volts;
}

float get_ts_voltage(bms_t *bmsdata, SPI_HandleTypeDef *hspi)
{
	read_vr_registers(bmsdata->plate_chip, hspi);
	// TODO: validate reading V2 and V3
	// NOTE: TS+ is output to both V2 and V3
	float avg_volts =
		(get_voltage_conversion(bmsdata->plate_chip.vr.v_codes[1]) +
		 get_voltage_conversion(bmsdata->plate_chip.vr.v_codes[2])) /
		2;
	return avg_volts;
}

void set_precharge_relay(bms_t *bmsdata, SPI_HandleTypeDef *hspi, bool state)
{
	if (state) {	
		set_gpo(bmsdata->plate_chip, hspi, HV_CTRL_GPO);
	} else {
		reset_gpo(bmsdata->plate_chip, hspi, HV_CTRL_GPO);
	}
}

