
#include "analyzer.h"
#include <math.h>
#include <float.h>
#include "bms_config.h"
#include "datastructs.h"
#include "serialPrintResult.h"
#include "timer.h"
#include "state_machine.h"
#include "u_tx_debug.h"
#include "u_tx_flags.h"
#include "can_messages_tx.h"
#include "shep_mutexes.h"
#include "soc.h"

#define OCV_TIMER_DURATION 1500 // in ms

/**
 * @brief Open-wire threshold while the S-ADC switch is active.
 */
#define CELL_OPEN_WIRE_MAX_DROP_PERCENT 15.0f

/**
 * @brief Map cells to therms (ra codes).  Note beta has only 6 therms.
 */
const int THERM_MAP[NUM_CELLS_PER_CHIP] = { 0, 0, 1, 1, 5, 5, 6,
					    6, 7, 7, 8, 8, 9 };

// clang-format on

/**
 * @brief Calculate the cell temperature of a 10,000 ohm NTP resistor (model 103)
 *
 * @param res The resistance of the resistor
 * @return float The temperature
 */
static float calc_temp(float res)
{
	// achieved via math --  See BMS 25 Mapping and Calcs
	return ((298.15f * 3462.28f) / (298.15f * logf(res / 10100) + 3462.28f)) -
	       273.15f;
}

/**
 * @brief Calculate a cell temperature based on the thermistor reading.
 *
 * @param voltage The thremistor reading.
 * @return float The temperature in degrees Celsius.
 */
static float calc_cell_temp(float voltage)
{
	float res = (10000 * (3 - voltage)) / voltage;
	return calc_temp(res);
}

chipdata_t *get_chip_data(analyzer_t *analyzer, uint8_t chip)
{
	assert_param(chip < NUM_CHIPS);
	return &analyzer->chip_data[chip]; // TODO; MUTEX
}

void calc_cell_temps(analyzer_t *analyzer, acc_data_t *acc_data)
{
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		for (int cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			int16_t x = acc_data->chips[chip]
					    .raux.ra_codes[THERM_MAP[cell]];
			analyzer->chip_data[chip].cell_temp[cell] =
				calc_cell_temp(getVoltage(x));
		}

		// Calculate onboard therm temps and chip temps
		analyzer->chip_data[chip].on_board_temp[0] = calc_cell_temp(
			getVoltage(acc_data->chips[chip].raux.ra_codes[2]));
		analyzer->chip_data[chip].on_board_temp[1] = calc_cell_temp(
			getVoltage(acc_data->chips[chip].raux.ra_codes[3]));
		analyzer->chip_data[chip].on_board_temp[2] = calc_cell_temp(
			getVoltage(acc_data->chips[chip].raux.ra_codes[4]));

		/* set the die temp */
		// conversion rate from datasheet, Table 105.  also in driver src
		analyzer->chip_data[chip].die_temp =
			(getVoltage(acc_data->chips[chip].stata.itmp) /
			 0.0075f) -
			273;
	}
}

void calc_pack_temps(analyzer_t *analyzer, acc_data_t *acc_data)
{
	analyzer->max_temp.val = FLT_MIN;
	analyzer->max_temp.cellNum = 0;
	analyzer->max_temp.chipIndex = 0;

	analyzer->min_temp.val = FLT_MAX;
	analyzer->min_temp.cellNum = 0;
	analyzer->min_temp.chipIndex = 0;

	analyzer->max_chiptemp.val = 0;

	float total_temp = 0;
	float total_seg_temp = 0;

	for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			if (analyzer->chip_data[chip].cell_temp[cell] >
			    analyzer->max_temp.val) {
				analyzer->max_temp.val =
					analyzer->chip_data[chip]
						.cell_temp[cell];
				analyzer->max_temp.cellNum = cell;
				analyzer->max_temp.chipIndex = chip;
			}

			/* finds out the minimum cell temp and location */
			if (analyzer->chip_data[chip].cell_temp[cell] <
			    analyzer->min_temp.val) {
				analyzer->min_temp.val =
					analyzer->chip_data[chip]
						.cell_temp[cell];
				analyzer->min_temp.cellNum = cell;
				analyzer->min_temp.chipIndex = chip;
			}

			total_temp += analyzer->chip_data[chip].cell_temp[cell];
			total_seg_temp +=
				analyzer->chip_data[chip].cell_temp[cell];
		}

		/* only for NERO */
		if (chip % 2 == 1) {
			analyzer->segment_average_temps[chip / 2] =
				total_seg_temp / ((float)(NUM_CELLS_PER_CHIP * NUM_CHIPS_PER_SEGMENT));
			total_seg_temp = 0;
		}

		if (analyzer->max_chiptemp.val <
		    analyzer->chip_data[chip].die_temp) {
			analyzer->max_chiptemp = (crit_chipval_t){
				.chipNum = chip,
				.val = analyzer->chip_data[chip].die_temp
			};
		}
	}

	/* Takes the average of all the cell temperatures. */
	analyzer->avg_temp = total_temp / NUM_CELLS;
}

void calc_cell_voltages(analyzer_t *analyzer, acc_data_t *acc_data,
			state_machine_t *state_machine)
{
	for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			if (state_machine->bms_state == CHARGING) {
				analyzer->chip_data[chip].cell_voltages[cell] =
					getVoltage(acc_data->chips[chip]
							   .cell.c_codes[cell]);
			} else {
				analyzer->chip_data[chip].cell_voltages[cell] =
					getVoltage(
						acc_data->chips[chip]
							.fcell.fc_codes[cell]);
			}
		}
		// 25A patch only: alpha lowest and beta highest need to be offset correctly
		uint8_t cell = 0;
		static const float unit_res = 0.00139f; // from 1/2oz copper, 0.71mm trace width, 30C
		static const float trace_len_alpha = 234.56f + 27.23f; // mm
		static const float trace_len_beta = 116.36f + 27.431f; // mm
		static const float fuse_res = 0.1637f; // ohms, 470 series 1206 1.25A, cold

		static const float trace_res_onboard_alpha = 0.015f; // ohms, correction offset
		static const float trace_res_onboard_beta = 0.20f; // ohms, correction offset
		// the distance times the unit resistance, plus the resistance of the fuse
		float res = 0.0f;
		if (chip % 2 == 1) {
		    // BETA
		    cell = 12;
			res = (unit_res * trace_len_beta) + fuse_res + trace_res_onboard_beta;
		} else {
		    // ALPHA
		    res = (unit_res * trace_len_alpha) + fuse_res + trace_res_onboard_alpha;
		}
		// measured on 4/5/2026, the current through the cells when in active mode continous C/S read compare
		float curr_bal = 0.031f;
		if (state_machine->bms_state == CHARGING) {
		    // measured on 4/5/2026, the current through the cells when in charging mode single shot C ADCs
			// redone to be higher 4/8 sans measurement
		    curr_bal = 0.029f;
		}
		// I*R is the way
		analyzer->chip_data[chip].cell_voltages[cell] += curr_bal * res;
	}


}

void calc_pack_voltage_stats(analyzer_t *analyzer, acc_data_t *acc_data)
{
	analyzer->max_voltage.val = FLT_MIN;
	analyzer->max_voltage.cellNum = 0;
	analyzer->max_voltage.chipIndex = 0;

	analyzer->max_ocv.val = FLT_MIN;
	analyzer->max_ocv.cellNum = 0;
	analyzer->max_ocv.chipIndex = 0;

	analyzer->min_voltage.val = FLT_MAX;
	analyzer->min_voltage.cellNum = 0;
	analyzer->min_voltage.chipIndex = 0;

	analyzer->min_ocv.val = FLT_MAX;
	analyzer->min_ocv.cellNum = 0;
	analyzer->min_ocv.chipIndex = 0;

	float total_volt = 0;
	float total_ocv = 0;
	float total_seg_volt = 0;

	for (uint8_t c = 0; c < NUM_CHIPS; c++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			/* fings out the maximum cell voltage and location */
			if (analyzer->chip_data[c].cell_voltages[cell] >
			    analyzer->max_voltage.val) {
				analyzer->max_voltage.val =
					analyzer->chip_data[c]
						.cell_voltages[cell];
				analyzer->max_voltage.chipIndex = c;
				analyzer->max_voltage.cellNum = cell;
			}

			if (analyzer->chip_data[c].open_cell_voltage[cell] >
			    analyzer->max_ocv.val) {
				analyzer->max_ocv.val =
					analyzer->chip_data[c]
						.open_cell_voltage[cell];
				analyzer->max_ocv.chipIndex = c;
				analyzer->max_ocv.cellNum = cell;
			}

			/* finds out the minimum cell voltage and location */
			if (analyzer->chip_data[c].cell_voltages[cell] <
			    analyzer->min_voltage.val) {
				analyzer->min_voltage.val =
					analyzer->chip_data[c]
						.cell_voltages[cell];
				analyzer->min_voltage.chipIndex = c;
				analyzer->min_voltage.cellNum = cell;
			}

			if (analyzer->chip_data[c].open_cell_voltage[cell] <
			    analyzer->min_ocv.val) {
				analyzer->min_ocv.val =
					analyzer->chip_data[c]
						.open_cell_voltage[cell];
				analyzer->min_ocv.chipIndex = c;
				analyzer->min_ocv.cellNum = cell;
			}

			total_volt +=
				analyzer->chip_data[c].cell_voltages[cell];
			total_ocv +=
				analyzer->chip_data[c].open_cell_voltage[cell];

			total_seg_volt +=
				analyzer->chip_data[c].open_cell_voltage[cell];
		}
		if (c % 2 == 1) {
			// calc averge volatage across a segment
			analyzer->segment_average_volts[c / 2] =
				total_seg_volt / ((float)(NUM_CELLS_PER_CHIP * NUM_CHIPS_PER_SEGMENT));
			analyzer->segment_total_volts[c / 2] = total_seg_volt;
			analyzer->segment_delt_volts[c / 2] =
				analyzer->max_voltage.val -
				analyzer->min_voltage.val;
			total_seg_volt = 0;
		}
	}

	/* calculate some voltage stats */
	analyzer->avg_voltage = total_volt / NUM_CELLS;

	analyzer->pack_voltage = total_volt;

	analyzer->delta_voltage =
		analyzer->max_voltage.val - analyzer->min_voltage.val;

	analyzer->avg_ocv = total_ocv / NUM_CELLS;
	analyzer->pack_ocv = total_ocv;
	analyzer->delt_ocv = analyzer->max_ocv.val - analyzer->min_ocv.val;
}

void calc_cell_resistances(analyzer_t *analyzer, acc_data_t *acc_data,
			   hv_plate_t *hv_plate)
{
	for (uint8_t c = 0; c < NUM_CHIPS; c++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			// Cell resistance drops when there is current running through the pack
			if (fabsf(hv_plate->pack_current) >= 0.001f) {
				analyzer->chip_data[c].cell_resistance[cell] =
					(analyzer->chip_data[c]
						 .open_cell_voltage[cell] -
					 analyzer->chip_data[c]
						 .cell_voltages[cell]) /
					fabsf(hv_plate->pack_current);
			} else {
				analyzer->chip_data[c].cell_resistance[cell] =
					0.015; // default resistance from data sheet
			}
		}
	}
}

void calc_open_cell_voltage(analyzer_t *analyzer,
			    hv_plate_t *hv_plate,
			    state_machine_t *state_machine)
{
	static bool is_first_reading = true;
	bool update_ocv = false;

	// OCV requires low current with both charging and balancing disabled.
	const bool ocv_update_allowed =
		(fabsf(hv_plate->pack_current) < OCV_CURR_THRESH) &&
		state_machine->charger_output_disabled &&
		!state_machine->balancing_active;

	if (is_first_reading) {
		float last_cell =
			analyzer->chip_data[NUM_CHIPS - 1]
				.cell_voltages[NUM_CELLS_PER_CHIP - 1];

		if (last_cell > 1.0f && last_cell < 5.0f) {
			is_first_reading = false;
			update_ocv = true;
		}
	}

	if (ocv_update_allowed) {
		if (is_timer_expired(&analyzer->ocvTimer)) {
			update_ocv = true;
		} else if (!is_timer_active(&analyzer->ocvTimer)) {
			start_timer(&analyzer->ocvTimer,
				    OCV_TIMER_DURATION);
		}
	} else {
		cancel_timer(&analyzer->ocvTimer);
	}

	if (update_ocv) {
		for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
			for (uint8_t cell = 0;
			     cell < NUM_CELLS_PER_CHIP; cell++) {
				analyzer->chip_data[chip]
					.open_cell_voltage[cell] =
					analyzer->chip_data[chip]
						.cell_voltages[cell];
			}
		}
	}
}

void detect_cell_open_wire(analyzer_t *analyzer, acc_data_t *acc_data,
			   state_machine_t *state_machine)
{
	bool open_wire_fault_active = false;

	for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
		chipdata_t *chip_data = get_chip_data(analyzer, chip);

		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			const float even_voltage = getVoltage(
				acc_data->chips[chip].owcell.cell_ow_even[cell]);
			const float odd_voltage = getVoltage(
				acc_data->chips[chip].owcell.cell_ow_odd[cell]);
			const bool even_cell = ((cell + 1U) % 2U) == 0U;
			const float excited_voltage =
				even_cell ? even_voltage : odd_voltage;
			const float baseline_voltage =
				even_cell ? odd_voltage : even_voltage;
			const float drop_voltage =
				baseline_voltage - excited_voltage;
			float drop_percent = 0.0f;

			if (baseline_voltage > 0.0f) {
				drop_percent =
					(drop_voltage / baseline_voltage) * 100.0f;
			}

			const bool was_open = chip_data->ow_fault[cell];
			const bool is_open =
				drop_percent > CELL_OPEN_WIRE_MAX_DROP_PERCENT;

			chip_data->ow_fault[cell] = is_open;
			open_wire_fault_active =
				open_wire_fault_active || is_open;

			if (is_open && !was_open) {
				PRINTLN_WARNING(
					"[OW] Open wire IC%u C%02u: even=%.3f V, odd=%.3f V, drop=%.3f V (%.1f%%)",
					chip + 1U, cell + 1U, even_voltage,
					odd_voltage, drop_voltage, drop_percent);
			}
		}
	}

	if (open_wire_fault_active) {
		set_cell_open_wire_fault(state_machine);
	} else {
		clear_cell_open_wire_fault(state_machine);
	}
}

void update_chip_status(analyzer_t *analyzer, acc_data_t *acc_data)
{
	for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
		chipdata_t *chip_data = get_chip_data(analyzer, chip);

		// Cell Diagnostics
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			// balancing status
			if (cell < PWMA) {
			    chip_data->is_balancing[cell] = acc_data->chips[chip].PwmA.pwma[cell] > 0;
			} else {
			    chip_data->is_balancing[cell] = acc_data->chips[chip].PwmB.pwmb[cell - PWMA] > 0;
			}
			// S_C fault status
			chip_data->cs_fault[cell] =
				(acc_data->chips[chip].statc.cs_flt >> cell) &
				1;
		}

		// Chip Diagnotics
		chip_data->vpv =
			20.0f *
			getVoltage( // VPV is ra_code 11 w/ different scale
				acc_data->chips[chip].aux.a_codes[11]),
		chip_data->vmv =
			(20.0f * getVoltage( // VMV is ra_code 10
					acc_data->chips[chip].aux.a_codes[10])),
		chip_data->flt_reg = acc_data->chips[chip].statc;
		chip_data->v_res = getVoltage(acc_data->chips[chip].statb.vr4k);
		chip_data->vref2 =
			getVoltage(acc_data->chips[chip].stata.vref2);
		chip_data->v_analog =
			getVoltage(acc_data->chips[chip].statb.va),
		chip_data->v_digital =
			getVoltage(acc_data->chips[chip].statb.vd);
	}
}

// ANALYZER THREAD
void vAnalyzer(ULONG thread_input)
{
	PRINTLN_INFO("Starting Analyzer Thread...");

	analyzer_args_t *analyzer_args = (analyzer_args_t *)thread_input;

	analyzer_t *analyzer = analyzer_args->analyzer;
	acc_data_t *acc_data = analyzer_args->acc_data;
	state_machine_t *state_machine = analyzer_args->state_machine;
	hv_plate_t *hv_plate = analyzer_args->hv_plate;

	memset(analyzer->chip_data, 0, sizeof(analyzer->chip_data));

	for (;;) {
		get_flag(ANALYZER_FLAG, TX_WAIT_FOREVER);

		// NOTE: All functions that modify chip data are externally mutexed
		mutex_get(&analyzer_mutex);

		// calculate base values for later safety calcs
		calc_cell_temps(analyzer, acc_data);
		calc_pack_temps(analyzer, acc_data);
		calc_cell_voltages(analyzer, acc_data, state_machine);
		calc_open_cell_voltage(analyzer, hv_plate, state_machine);
		calc_pack_voltage_stats(analyzer, acc_data);
		calc_cell_resistances(analyzer, acc_data, hv_plate);
		detect_cell_open_wire(analyzer, acc_data, state_machine);
		update_chip_status(analyzer, acc_data);

		mutex_put(&analyzer_mutex);

		set_flag(SANITIZER_FLAG);
		set_flag(DEBUG_FLAG);

		// send out telemetry data sourced from the above functions
		send_cell_voltage(analyzer->max_ocv.val,
				  analyzer->max_ocv.chipIndex,
				  analyzer->max_ocv.cellNum,
				  analyzer->min_ocv.val,
				  analyzer->min_ocv.chipIndex,
				  analyzer->min_ocv.cellNum, analyzer->avg_ocv);
		send_segment_average_voltages(
			analyzer->segment_average_volts[0], analyzer->segment_average_volts[1], analyzer->segment_average_volts[2], analyzer->segment_average_volts[3], analyzer->segment_average_volts[4]);
		send_segment_total_voltages(
		analyzer->segment_total_volts[0], analyzer->segment_total_volts[1], analyzer->segment_total_volts[2], analyzer->segment_total_volts[3], analyzer->segment_total_volts[4]);
		send_cell_temperatures(analyzer->max_temp.val, analyzer->max_temp.chipIndex, analyzer->max_temp.cellNum,
		                  analyzer->min_temp.val, analyzer->min_temp.chipIndex, analyzer->min_temp.cellNum,
					  analyzer->avg_temp
		    );
		send_segment_temperatures(
		analyzer->segment_average_temps[0], analyzer->segment_average_temps[1], analyzer->segment_average_temps[2], analyzer->segment_average_temps[3], analyzer->segment_average_temps[4]);
		send_pack_soc_status(analyzer->soc, get_soc_drift());
	}
}
