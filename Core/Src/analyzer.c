
#include "analyzer.h"
#include <math.h>
#include <float.h>
#include "serialPrintResult.h"
#include "timer.h"

// the OCV timer
nertimer_t ocvTimer;

/**
 * @brief Map cells to therms (ra codes).  Note beta has only 6 therms. 
 */
const int THERM_MAP[NUM_CELLS_PER_CHIP] = { 0, 0, 1, 1, 2, 2, 3,
					    3, 4, 4, 5, 5, 6 };

// clang-format on

/**
 * @brief Calculate the cell temperature of a 10,000 ohm NTP resistor (model 103)
 * 
 * @param res The resistance of the resistor
 * @return float The temperature
 */
static float calc_temp(float res)
{
	float coef = res / 10000.0;
	// achieved via passing ThermCalcs.xlsx into https://www.standardsapplied.com/nonlinear-curve-fitting-calculator.html
	return -1149.531863 * (pow(coef, 1.0 / 8)) +
	       658.9396848 * (pow(coef, 1.0 / 4)) +
	       -87.8102815 * (pow(coef, 1.0 / 2)) + 2.034216235 * coef +
	       601.008351;
}

/**
 * @brief Calculate a cell temperature based on the thermistor reading.
 * 
 * @param voltage The thremistor reading.
 * @return float The temperature in degrees Celsius.
 */
static float calc_cell_temp(float voltage)
{
	float res = (5600 * (3 - voltage)) / voltage;
	return calc_temp(res);
}

/**
 * @brief Calculate a cell temperature of onboard therm
 * 
 * @param voltage the voltage read by ADC
 * @return float The temperature in degrees C
 */
static float calc_cell_temp_onboard(float voltage)
{
	float res = (5600 * (5 - voltage)) / voltage;
	return calc_temp(res);
}

void calc_cell_temps(analyzer_t *analyzer)
{
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		for (int cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			int16_t x = analyzer->acc_data->chips[chip]
					    .raux.ra_codes[THERM_MAP[cell]];
			analyzer->chip_data[chip].cell_temp[cell] =
				calc_cell_temp(getVoltage(x));
		}

		// Calculate onboard therm temps and chip temp

		// TODO Check where inboard therms are input
		// Take average of both onboard therms
		analyzer->chip_data[chip].on_board_temp =
			(calc_cell_temp_onboard(getVoltage(
				 analyzer->acc_data->chips[chip].raux.ra_codes[6])) +
			 calc_cell_temp_onboard(getVoltage(
				 analyzer->acc_data->chips[chip].raux.ra_codes[7]))) /
			2;

		/* set the die temp */
		// conversion rate from datasheet, Table 105.  also in driver src
		analyzer->chip_data[chip].die_temp =
			(getVoltage(analyzer->acc_data->chips[chip].stata.itmp) / 0.0075) -
			273;
	}
}

void calc_pack_temps(analyzer_t *analyzer)
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
					analyzer->chip_data[chip].cell_temp[cell];
				analyzer->max_temp.cellNum = cell;
				analyzer->max_temp.chipIndex = chip;
			}

			/* finds out the minimum cell temp and location */
			if (analyzer->chip_data[chip].cell_temp[cell] <
			    analyzer->min_temp.val) {
				analyzer->min_temp.val =
					analyzer->chip_data[chip].cell_temp[cell];
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
				total_seg_temp / ((float)(NUM_CELLS * 2));
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

void calc_cell_voltages(analyzer_t *analyzer)
{
	for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			if (analyzer->acc_data->bms_state == CHARGING) {
				// in charging state, we read single shot c codes ONLY
				analyzer->chip_data[chip].cell_voltages[cell] =
					getVoltage(analyzer->acc_data->chips[chip]
							   .cell.c_codes[cell]);
			} else {
				analyzer->chip_data[chip].cell_voltages[cell] =
					getVoltage(
						analyzer->acc_data->chips[chip]
							.fcell.fc_codes[cell]);
			}
		}
	}
}

void calc_pack_voltage_stats(analyzer_t *analyzer)
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

			total_volt += analyzer->chip_data[c].cell_voltages[cell];
			total_ocv +=
				analyzer->chip_data[c].open_cell_voltage[cell];

			total_seg_volt +=
				analyzer->chip_data[c].open_cell_voltage[cell];
		}
		if (c % 2 == 1) {
			analyzer->segment_average_volts[c / 2] =
				total_seg_volt / ((float)(NUM_CELLS * 2));
			total_seg_volt = 0;
		}
	}

	/* calculate some voltage stats */
	// TODO: Make this based on total cells when actual segment is here
	analyzer->avg_voltage = total_volt / NUM_CELLS;

	analyzer->pack_voltage = total_volt;

	analyzer->delt_voltage =
		analyzer->max_voltage.val - analyzer->min_voltage.val;

	analyzer->avg_ocv = total_ocv / NUM_CELLS;
	analyzer->pack_ocv = total_ocv;
	analyzer->delt_ocv = analyzer->max_ocv.val - analyzer->min_ocv.val;
}

void calc_cell_resistances(analyzer_t *analyzer)
{
	for (uint8_t c = 0; c < NUM_CHIPS; c++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			if (fabs(analyzer->hv_plate->pack_current) >= 0.001) {
				analyzer->chip_data[c].cell_resistance[cell] =
					(analyzer->chip_data[c]
						 .open_cell_voltage[cell] -
					 analyzer->chip_data[c]
						 .cell_voltages[cell]) /
					fabs(analyzer->hv_plate->pack_current);
			} else {
				analyzer->chip_data[c].cell_resistance[cell] =
					0.015; // default resistance from data sheet
			}
		}
	}
}

void calc_open_cell_voltage(analyzer_t *analyzer)
{
	static bool is_first_reading = true;
	/* if there is no previous data point, set inital open cell voltage to current reading */
	if (is_first_reading) {
		// sanity check the last cell that the reading is good, oftentimes the first readings are bad
		float last_cell =
			analyzer->chip_data[NUM_CHIPS - 1]
				.cell_voltages[NUM_CELLS_PER_CHIP - 1];
		if (last_cell > 1 && last_cell < 5) {
			is_first_reading = false;
			start_timer(&ocvTimer, 750);
		}

		for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
			for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP;
			     cell++) {
				analyzer->chip_data[chip]
					.open_cell_voltage[cell] =
					analyzer->chip_data[chip]
						.cell_voltages[cell];
			}
		}
	}

	// TODO validate
	// If we are within the current threshold for open voltage measurments (1.5 mA)
	if (analyzer->hv_plate->pack_current < OCV_CURR_THRESH &&
	    analyzer->hv_plate->pack_current > -1 * OCV_CURR_THRESH) {
		// Timer expired or not active
		if (is_timer_expired(&ocvTimer) ||
		    !is_timer_active(&ocvTimer)) {
			for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
				for (uint8_t cell = 0;
				     cell < NUM_CELLS_PER_CHIP; cell++) {
					// Set current OCV value, ensure value is true OCV
					if (analyzer->chip_data[chip]
							    .cell_voltages[cell] <
						    4.5 && // TODO globally define max and min volts
					    analyzer->chip_data[chip]
							    .cell_voltages[cell] >
						    2) {
						analyzer->chip_data[chip]
							.open_cell_voltage[cell] =
							analyzer->chip_data[chip]
								.cell_voltages
									[cell];
					} else {
						analyzer->chip_data[chip]
							.open_cell_voltage[cell] =
							analyzer->segment_average_volts
								[chip /
								 2]; // TODO should delete
					}
				}
			}
		} else {
			start_timer(&ocvTimer, 750);
		}
	}
}
