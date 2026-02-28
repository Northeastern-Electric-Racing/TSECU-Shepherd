#include "hv_plate.h"
#include "adi2950_interaction.h"
#include "dcl.h"
#include "ccl.h"
#include "soc.h"
#include "can_messages.h"
#include "bms_algos.h"
#include "app_threadx.h"
#include "shep_mutexes.h"

#define SHUNT_RESISTANCE 0.05 / 1000 // 0.05 mOhms

#define microV(x) ((x * 1e-6))

static float get_current_conversion(uint32_t data)
{
	float current = ((int32_t)(data << (32 - 24)) >> (32 - 24));
	return current / (float)SHUNT_RESISTANCE;
}

static float get_voltage_conversion(int data)
{
	float voltage = microV(100) * (int16_t)data;
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

	hv_plate->adbms_flags.raw = 0; // reset flags
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
			get_voltage_conversion(hv_plate->ic->i_vbacc.vb1acc) /
			hv_plate->conversion_count;

		hv_plate->pack_current =
			get_current_conversion(hv_plate->ic->i_vbacc.i1acc) /
			hv_plate->conversion_count;

		hv_plate->last_total_converion_count = num_conversitions;
	}
	unsnap_2950(hv_plate->ic);
}

void get_ts_voltage(hv_plate_t *hv_plate)
{
	read_v2_v3_registers(hv_plate->ic);
	// note: ts+ is output to both v2 and v3
	float avg_volts =
		(get_voltage_conversion(hv_plate->ic->vr.v_codes[1]) + // v2
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

void get_flags(hv_plate_t *hv_plate)
{
	read_flag_register(hv_plate->ic);
	cell_asic_2950 *ic = hv_plate->ic;
	hv_plate->adbms_flags.flags.vreguv = ic->flag.vreguv;
	hv_plate->adbms_flags.flags.vregov = ic->flag.vregov;
	hv_plate->adbms_flags.flags.vdduv = ic->flag.vdduv;
	hv_plate->adbms_flags.flags.vdiguv = ic->flag.vdiguv;
	hv_plate->adbms_flags.flags.vdigov = ic->flag.vdigov;
	hv_plate->adbms_flags.flags.vde = ic->flag.vde;
	hv_plate->adbms_flags.flags.vdel = ic->flag.vdel;
	hv_plate->adbms_flags.flags.oscflt = ic->flag.oscflt;
	hv_plate->adbms_flags.flags.noclk = ic->flag.noclk;
	hv_plate->adbms_flags.flags.spiflt = ic->flag.spiflt;
	hv_plate->adbms_flags.flags.thsd = ic->flag.thsd;
	hv_plate->adbms_flags.flags.reset = ic->flag.reset;
}

void get_aux_adc_data(hv_plate_t *hv_plate)
{
	poll_and_read_aux_registers(hv_plate->ic);

	cell_asic_2950 *ic = hv_plate->ic;
	// Read voltage results into struct
	hv_plate->vreg = ic->auxa.vreg * microV(240);
	hv_plate->vref1p25 = get_voltage_conversion(ic->auxa.vref1p25);
	hv_plate->epad = get_voltage_conversion(ic->auxb.epad);
	// Different conversions for vreg and vdd
	hv_plate->vdd = ic->auxb.vdd * microV(1000);
	hv_plate->vdiv = get_voltage_conversion(ic->auxc.vdiv);
	hv_plate->vdig = ic->auxb.vdig * microV(240);

	hv_plate->tmp1 = (ic->auxa.tmp1 / 61.8f) - 250;
	hv_plate->tmp2 = (ic->auxc.tmp2 / 20.5f) - 267;

	hv_plate->osccnt = ic->auxc.osccnt;
}

// HV PLATE DATA THREAD
void vHvPlateData(ULONG thread_input)
{
	PRINTLN_INFO("Starting HV Plate thread...");

	const int hv_plate_task_delay = 100; // in ms
	const uint16_t diagnostic_read_frequency = 2000; // 5s
	nertimer_t diagnostic_read_timer;

	hv_plate_args_t *hv_plate_args = (hv_plate_args_t *)thread_input;

	hv_plate_t *hv_plate = hv_plate_args->hv_plate;
	analyzer_t *analyzer = hv_plate_args->analyzer;
	bms_algos_t *bms_algos = hv_plate_args->bms_algos;
	state_machine_t *state_machine = hv_plate_args->state_machine;

	dcl_init(COOLDOWN_ON_FULL_PULSE);
	ccl_init(COOLDOWN_ON_FULL_PULSE);

	// initialize HV Plate struct and start conversions
	init_hv_plate(hv_plate, ACCI_8);

	tx_thread_sleep(MS_TO_TICKS(500));

	start_timer(&diagnostic_read_timer, diagnostic_read_frequency);
	for (;;) {
		// get the current reading from the pack
		get_pack_current_and_batt_voltage(hv_plate,
						  hv_plate_task_delay);

		// updates the SoC value in the analyzer struct based on the pack current
		// received
		update_soc(analyzer, hv_plate);

		/* Check whether pulse operation needs to be disabled due to charging state or faults */

		if (disable_pulse(state_machine)) {
			mutex_get(&bms_algos_mutex);
			bms_algos->cont_DCL = bms_algos->inst_DCL;
			bms_algos->cont_CCL = bms_algos->inst_CCL;
			mutex_put(&bms_algos_mutex);
		} else {
			// Calculate continous DCL and CCL
			dcl_calc_cont_limit(hv_plate->pack_current, bms_algos);
			ccl_calc_cont_limit(hv_plate->pack_current, bms_algos);
		}

		// read ts voltage
		get_ts_voltage(hv_plate);

		// read shunt temperature
		get_shunt_temp(hv_plate);

		if (is_timer_expired(&diagnostic_read_timer)) {
			// read flags
			get_flags(hv_plate);
			get_aux_adc_data(hv_plate);
			start_timer(&diagnostic_read_timer,
				    diagnostic_read_frequency);
			// Restart continuous conversion
			//start_adc_conversions(hv_plate->ic);
			// Send can message
			send_hv_plate_diagnostic_data(hv_plate);
		}

		tx_thread_sleep(MS_TO_TICKS(hv_plate_task_delay));
	}
}
