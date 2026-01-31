#include "hv_plate.h"
#include "adi2950_interaction.h"

#define SHUNT_RESISTANCE 0.05 / 1000 // 0.05 mOhms

static float get_current_conversion(uint32_t data)
{
	float current = 1e-6 * ((int32_t)(data << (32 - 24)) >> (32 - 24));
	return current / (float)SHUNT_RESISTANCE;
}

static float get_voltage_conversion(int data)
{
	float voltage = 100e-6 * (int16_t)data;
	return voltage;
}

void init_hv_plate(hv_plate_t *hv_plate, ACCI conversion_count)
{
	switch (conversion_count) {
	case ACCI_8:
		hv_plate->conversion_count = 8;
		break;
	case ACCI_16:
		hv_plate->conversion_count = 16;
		break;
	case ACCI_32:
		hv_plate->conversion_count = 32;
		break;
	default:
		PRINTLN_WARNING(
			"Unsupported accumulation count, defaulting to 8");
		hv_plate->conversion_count = 8;
		break;
	}
	hv_plate->last_total_converion_count = 0;

	set_accumulation_count(hv_plate->ic, conversion_count);
	start_adc_conversions(hv_plate->ic);
}

void get_pack_current_and_batt_voltage(hv_plate_t *hv_plate,
				       uint16_t request_rate)
{
	snap_2950(hv_plate->ic);
	const uint16_t expected_conversions =
		request_rate / hv_plate->conversion_count;

	read_accumulated_current_vbat_registers(hv_plate->ic);
	uint16_t num_conversitions =
		read_conversion_count_registers(hv_plate->ic);

	// indicates that the I1CNT register wrapped around
	if (num_conversitions < hv_plate->last_total_converion_count) {
		hv_plate->last_total_converion_count = 0;
	}

	// check if the adequate number of conversions have been
	// made before determining if acculmulated current is valid current reading is valid
	if ((num_conversitions - hv_plate->last_total_converion_count) /
		    hv_plate->conversion_count >=
	    expected_conversions) {
		hv_plate->batt_volts =
			get_voltage_conversion(hv_plate->ic->vbacc.vb1acc) /
			hv_plate->conversion_count;

		hv_plate->pack_current =
			get_current_conversion(hv_plate->ic->iacc.i1acc) /
			hv_plate->conversion_count;

		hv_plate->last_total_converion_count = num_conversitions;
	}
	unsnap_2950(hv_plate->ic);
}

void get_ts_voltage(hv_plate_t *hv_plate)
{
	read_v2_v3_registers(hv_plate->ic);
	// NOTE: TS+ is output to both V2 and V3
	float avg_volts =
		(get_voltage_conversion(hv_plate->ic->vr.v_codes[1]) + // V2
		 get_voltage_conversion(hv_plate->ic->vr.v_codes[2])) / // V3
		2;
	hv_plate->ts_volts = avg_volts;
}

void get_shunt_temp(hv_plate_t *hv_plate)
{
	read_v7_v9_registers(hv_plate->ic);
	// NOTE: TS+ is output to both V2 and V3
	float avg_volts =
		(get_voltage_conversion(hv_plate->ic->vr.v_codes[9]) + // V7A
		 get_voltage_conversion(hv_plate->ic->vr.v_codes[11])) / // V9B
		2;

	hv_plate->shunt_temp = avg_volts; // TODO: convert to temp
}
