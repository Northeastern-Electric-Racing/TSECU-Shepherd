
#include "segment.h"
#include "adBms6830Data.h"
#include "adi6830_interation.h"
#include "bms_config.h"
#include "can_messages_tx.h"
#include "c_utils.h"
#include "charging.h"
#include "datastructs.h"
#include "segment_isospi_recovery.h"
#include "serialPrintResult.h"
#include "u_tx_flags.h"
#include "state_machine.h"
#include "app_threadx.h"
#include "main.h"
#include "shep_mutexes.h"

enum onboard_therm_index {
	THERM_ALPHA_BALANCING_BANK = 0,
	THERM_CHIPS,
	THERM_BETA_BALANCING_BANK
};

static bool update_chip_balance_thermal_limits(
	analyzer_t *analyzer, balancing_thermal_state_t *thermal_state)
{
	bool state_changed = false;
	float die_temp[NUM_CHIPS] = { 0.0f };
	float chip_temp[NUM_CHIPS] = { 0.0f };
	float resistor_temp[NUM_CHIPS] = { 0.0f };

	mutex_get(&analyzer_mutex);
	for (uint8_t segment = 0U; segment < NUM_SEGMENTS; segment++) {
		uint8_t alpha_chip = segment * NUM_CHIPS_PER_SEGMENT;
		uint8_t beta_chip = alpha_chip + 1U;
		chipdata_t *alpha_data = &analyzer->chip_data[alpha_chip];
		chipdata_t *beta_data = &analyzer->chip_data[beta_chip];
		float onboard_chip_temp =
			alpha_data->on_board_temp[THERM_CHIPS];

		die_temp[alpha_chip] = alpha_data->die_temp;
		die_temp[beta_chip] = beta_data->die_temp;

		// Both RAUX4 thermistors monitor the chips.
		if (beta_data->on_board_temp[THERM_CHIPS] >
		    onboard_chip_temp) {
			onboard_chip_temp =
				beta_data->on_board_temp[THERM_CHIPS];
		}
		chip_temp[alpha_chip] = onboard_chip_temp;
		chip_temp[beta_chip] = onboard_chip_temp;

		// Alpha RAUX3 monitors the Alpha balancing bank.
		// Alpha RAUX5 monitors the Beta balancing bank.
		resistor_temp[alpha_chip] =
			alpha_data->on_board_temp[THERM_ALPHA_BALANCING_BANK];
		resistor_temp[beta_chip] =
			alpha_data->on_board_temp[THERM_BETA_BALANCING_BANK];
	}
	mutex_put(&analyzer_mutex);

	for (uint8_t chip = 0U; chip < NUM_CHIPS; chip++) {
		bool was_blocked = thermal_state->balance_blocked[chip];

		thermal_state->chip_die_too_hot[chip] =
			update_active_high_hysteresis(
				die_temp[chip], BALANCE_CHIP_DIE_DISABLE_TEMP,
				BALANCE_CHIP_DIE_ENABLE_TEMP,
				thermal_state->chip_die_too_hot[chip]);
		thermal_state->chip_onboard_therm_too_hot[chip] =
			update_active_high_hysteresis(
				chip_temp[chip],
				BALANCE_CHIP_ONBOARD_DISABLE_TEMP,
				BALANCE_CHIP_ONBOARD_ENABLE_TEMP,
				thermal_state->chip_onboard_therm_too_hot[chip]);
		thermal_state->balancing_resistor_too_hot[chip] =
			update_active_high_hysteresis(
				resistor_temp[chip],
				BALANCE_RESISTOR_DISABLE_TEMP,
				BALANCE_RESISTOR_ENABLE_TEMP,
				thermal_state
					->balancing_resistor_too_hot[chip]);

		thermal_state->balance_blocked[chip] =
			thermal_state->chip_die_too_hot[chip] ||
			thermal_state->chip_onboard_therm_too_hot[chip] ||
			thermal_state->balancing_resistor_too_hot[chip];
		state_changed |=
			was_blocked != thermal_state->balance_blocked[chip];
	}

	return state_changed;
}

/**
 * @brief Initialize a chip with our default values.
 *
 * @param chip Pointer to chip to initialize.
 */
void init_chip(cell_asic *chip)
{
	chip->tx_cfga.gpo = 0;

	// init config registers
	memset(chip->configa.rx_data, 0, sizeof(chip->configa.rx_data));
	memset(chip->configa.tx_data, 0, sizeof(chip->configa.rx_data));
	memset(chip->configb.rx_data, 0, sizeof(chip->configb.rx_data));
	memset(chip->configb.tx_data, 0, sizeof(chip->configb.tx_data));

	set_REFON(chip, PWR_UP);

	set_volt_adc_comp_thresh(chip, CVT_22_5mV);
	clear_diagnostic_flags(chip);

	// Short soak on ADAX
	set_soak_on(chip, SOAKON_SET);
	set_aux_soak_range(chip, SHORT_6830);

	// No open wire detect soak
	set_open_wire_soak_time(chip, OWA0);

	// Set therm GPIOs
	set_gpio_pull(chip, GPO1, GPO_SET);
	set_gpio_pull(chip, GPO2, GPO_SET);
	set_gpio_pull(chip, GPO3, GPO_SET);
	set_gpio_pull(chip, GPO4, GPO_SET);
	set_gpio_pull(chip, GPO5, GPO_SET);
	set_gpio_pull(chip, GPO6, GPO_SET);
	set_gpio_pull(chip, GPO7,
		      GPO_SET); // this is a on board therm for beta only
	set_gpio_pull(chip, GPO8, GPO_SET); // this is a on board therm

	// set outputs, 9=iso led 10=bal LED. false=lit up
	set_gpio_pull(chip, GPO9, GPO_SET);
	set_gpio_pull(chip, GPO10, GPO_SET);

	set_iir_corner_freq(chip, IIR_FPA16);

	// Init config B

	// If the corresponding fault bits are sent high, it does not affect the IC
	chip->tx_cfgb.vov = SetOverVoltageThreshold(4.2);
	chip->tx_cfgb.vuv = SetUnderVoltageThreshold(2.5);

	// Discharge timer monitor off
	set_discharge_timer_monitor(chip, DTMEN_OFF);

	// set this to allow sleep mode
	set_discharge_timeout(chip, 0);

	// Set discharge timer range to 0 to 63 minutes with 1 minute increments
	set_discharge_timer_range(chip, RANG_0_TO_63_MIN);

	// Disable discharge for all cells
	clear_cell_discharge(chip);
}

void segment_init(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi)
{
	printf("Initializing Segments...");
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		init_chip(&chips[chip]);
	}

	// One-time init for isoSPI line and comm_break.
	static bool is_first_init = true;

	/*
	 * These fields are later controlled by isoSPI recovery to
	 * manage communication on each line after an isoSPI break,
	 * so re-inits from segment_restart() must not overwrite them.
	 */
	if (is_first_init) {
		for (int chip = 0; chip < NUM_CHIPS; chip++) {
			// Set chip to primary isoSPI line A
			set_segment_chips_isospi_line(&chips[chip],
						      ADBMS6830_ISOSPI_LINE_A);

			// Not an endpoint in the daisy chain
			set_comm_break(&chips[chip], COMM_BK_OFF);
		}

		is_first_init = false;
	}

	write_config_regs(chips, hspi);

	// disable balancing on init
	mute_chips(chips, hspi);

	start_c_adc_conv(chips, hspi);
}

void segment_mute(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi)
{
	mute_chips(chips, hspi);
}
void segment_unmute(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi)
{
	unmute_chips(chips, hspi);
}
void segment_snap(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi)
{
	snap_chips(chips, hspi);
}
void segment_unsnap(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi)
{
	unsnap_chips(chips, hspi);
}

void segment_monitor_flts(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi)
{
	// clear them.  they will still be in memory for usage
	// read statc to retrieve any new faults
	// this must be called, otherwise bootup transients will pollute the faults
	write_clear_flags(chips, hspi);
}

// ensure stuff used is in the correctfunction
void segment_retrieve_active_data(cell_asic chips[NUM_CHIPS],
				  SPI_HandleTypeDef *hspi)

{
	// read all therms using AUX 2
	adc_and_read_aux2_registers(chips, hspi);

	// read from ADC convs
	read_filtered_voltage_registers(chips, hspi);
}

// ensure stuff used is in the correctfunction
void segment_retrieve_charging_data(cell_asic chips[NUM_CHIPS],
				    SPI_HandleTypeDef *hspi)

{
	// read all therms using AUX 2
	adc_and_read_aux2_registers(chips, hspi);

	// poll stuff like vref, etc.
	adc_and_read_aux_registers(chips, hspi);

	// run a clean single-shot conversion
	get_c_adc_voltages(chips, hspi);

	read_status_registers(chips, hspi);

	// Read configuration registers to monitor burning status and the like
	read_config_register_a(chips, hspi);
	read_config_register_b(chips, hspi);

	// segment_adc_comparison(bmsdata);
	//  check our fault flags
	segment_monitor_flts(chips, hspi);
}

void segment_retrieve_debug_data(cell_asic chips[NUM_CHIPS],
				 SPI_HandleTypeDef *hspi)
{
	// poll stuff like vref, etc.
	adc_and_read_aux_registers(chips, hspi);

	// read the above into status registers
	read_status_registers(chips, hspi);

	// Read configuration registers to monitor burning status and the like
	read_config_register_a(chips, hspi);
	read_config_register_b(chips, hspi);

	// segment_adc_comparison(bmsdata);
	// check our fault flags
	segment_monitor_flts(chips, hspi);

}

void segment_restart(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi)
{
	soft_reset_chips(chips, hspi);
	segment_init(chips, hspi);
}

bool segment_is_balancing(cell_asic chips[NUM_CHIPS])
{
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		if (chips[chip].rx_cfgb.dcc > 0) {
			return true;
		}
		for (uint8_t i = 0; i < PWMA; i++) {
			if (chips[chip].PwmA.pwma[i] > 0) {
				return true;
			}
		}
		for (uint8_t i = 0; i < NUM_CELLS_PER_CHIP - PWMA; i++) {
			if (chips[chip].PwmB.pwmb[i] > 0) {
				return true;
			}
		}
	}
	return false;
}

void segment_disable_balancing(cell_asic chips[NUM_CHIPS],
			       balancing_thermal_state_t *thermal_state,
			       SPI_HandleTypeDef *hspi)
{
	// Stop balancing before clearing the PWM configuration.
	mute_chips(chips, hspi);

	// Initializes all array elements to zero
	PWM_DUTY discharge_config[NUM_CHIPS][NUM_CELLS_PER_CHIP] = { 0 };
	segment_configure_balancing(chips, discharge_config, thermal_state,
				    hspi);
	segment_read_pwm_registers(chips, hspi);
}

void segment_enable_balancing(cell_asic chips[NUM_CHIPS],
			      SPI_HandleTypeDef *hspi)
{
	unmute_chips(chips, hspi);
}

void segment_manual_balancing(cell_asic chips[NUM_CHIPS],
			      balancing_thermal_state_t *thermal_state,
			      SPI_HandleTypeDef *hspi)
{
	// clang-format off
	bool discharge_config_en[NUM_CHIPS][NUM_CELLS_PER_CHIP] = {
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};
	PWM_DUTY cycle = pwm_duty_cycle_get();
	PWM_DUTY discharge_confg[NUM_CHIPS][NUM_CELLS_PER_CHIP] = { 0 };
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		for (int cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			if (discharge_config_en[chip][cell]) {
                discharge_confg[chip][cell] = cycle;
			}
		}
	}
	// clang-format on

	segment_configure_balancing(chips, discharge_confg, thermal_state, hspi);
}

void segment_configure_balancing(
	cell_asic chips[NUM_CHIPS],
	PWM_DUTY discharge_config[NUM_CHIPS][NUM_CELLS_PER_CHIP],
	balancing_thermal_state_t *thermal_state,
	SPI_HandleTypeDef *hspi)
{
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		for (int cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			PWM_DUTY duty_cycle = discharge_config[chip][cell];

			if (thermal_state->balance_blocked[chip]) {
				duty_cycle = PWM_0_0_PCT;
			}

			set_cell_pwm(&chips[chip], cell, duty_cycle);
		}
	}
	write_pwm_regs(chips, hspi);
}

void segment_read_pwm_registers(cell_asic chips[NUM_CHIPS],
				SPI_HandleTypeDef *hspi)
{
	read_pwm_registers(chips, hspi);
}

void segment_set_dcto(cell_asic chips[NUM_CHIPS], uint8_t dcto,
			      SPI_HandleTypeDef *hspi)
{
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		set_discharge_timeout(&chips[chip], dcto);
	}
	write_config_regs(chips, hspi);
}

/**
 * @brief Run the cell open-wire diagnostic.
 *
 * @param chips Array of chips.
 * @param analyzer Analyzer data to update.
 * @param charging True when charging.
 * @param hspi SPI handle.
 */
static void segment_run_cell_open_wire_test(cell_asic chips[NUM_CHIPS],
					    analyzer_t *analyzer,
					    bool charging,
					    SPI_HandleTypeDef *hspi)
{
	/*
	 * Open-wire sequence:
	 * 1. Read and store the normal S values.
	 * 2. Run and store the even-cell open-wire conversion.
	 * 3. Run and store the odd-cell open-wire conversion.
	 * 4. Restart continuous S outside charging.
	 */

	if (charging) {
		get_s_adc_voltages(chips, hspi);
	} else {
		segment_snap(chips, hspi);
		read_s_voltage_registers(chips, hspi);
		segment_unsnap(chips, hspi);
	}

	for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			analyzer->chip_data[chip].s_cell_voltages[cell] =
				getVoltage(chips[chip].scell.sc_codes[cell]);
		}
	}

	get_s_adc_open_wire_voltages(chips, hspi, OW_ON_EVEN_CH);

	for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			chips[chip].owcell.cell_ow_even[cell] =
				chips[chip].scell.sc_codes[cell];
		}
	}

	get_s_adc_open_wire_voltages(chips, hspi, OW_ON_ODD_CH);

	for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			chips[chip].owcell.cell_ow_odd[cell] =
				chips[chip].scell.sc_codes[cell];
		}
	}

	if (!charging) {
		start_s_adc_conv(chips, hspi);
	}
}

static void segment_send_s_adc_cell_data(const analyzer_t *analyzer)
{
	for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
		const chipdata_t *chip_data = &analyzer->chip_data[chip];

		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell += 2) {
			const bool has_cell_b = cell + 1U < NUM_CELLS_PER_CHIP;
			const float s_voltage_b = has_cell_b ?
				chip_data->s_cell_voltages[cell + 1U] :
				0.0f;

			if ((chip % 2U) == 0U) {
				send_alpha_cell_s_adc_data(
					chip_data->s_cell_voltages[cell], s_voltage_b,
					chip / 2U, cell, cell + 1U);
			} else {
				send_beta_cell_s_adc_data(
					chip_data->s_cell_voltages[cell], s_voltage_b,
					chip / 2U, cell, cell + 1U);
			}
		}
	}
}

// GET SEGEMENT DATA THREAD
void vGetSegmentData(ULONG thread_input)
{
	PRINTLN_INFO("Starting GetSegmentData thread...");

	acc_data_args_t *acc_data_args = (acc_data_args_t *)thread_input;

	acc_data_t *acc_data = acc_data_args->acc_data;
	state_machine_t *state_machine = acc_data_args->state_machine;
	analyzer_t *analyzer = acc_data_args->analyzer;

	HAL_NVIC_DisableIRQ(FDCAN2_IT0_IRQn);
	segment_init(acc_data->chips, &hspi2);
	segment_isospi_break_detection_init(acc_data->chips);
	HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);

	// must delay after init for ADC to start up
	tx_thread_sleep(500);

	// Run after the monitor ADC has initialized, before normal acquisition
	HAL_NVIC_DisableIRQ(FDCAN2_IT0_IRQn);
	segment_run_cell_open_wire_test(acc_data->chips, analyzer, false,
					&hspi2);
	segment_send_s_adc_cell_data(analyzer);
	HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);

	state_t prev_state = BOOT;
	state_t current_state = BOOT;
	bool prev_balancing_active = false;

	nertimer_t pwm_timer;
	nertimer_t open_wire_timer;
	// assumes a DCTO of 1 minute for PWM balancing in extended balancing mode
	const uint32_t pwm_update_frequency = 55000;
	// Required fault-tolerant time interval
	const uint32_t open_wire_test_frequency = 30000;

	start_timer(&pwm_timer, 0); // start timer immeditately on first run
	start_timer(&open_wire_timer, open_wire_test_frequency);

	for (;;) {
		prev_state = current_state;
		current_state = state_machine->bms_state;
		const bool charging = (current_state == CHARGING);
		const bool balancing_active = state_machine->balancing_active;
		const bool thermal_state_changed =
			update_chip_balance_thermal_limits(
				analyzer, &acc_data->balancing_thermal);

		HAL_NVIC_DisableIRQ(FDCAN2_IT0_IRQn);

		if ((prev_state != current_state && !charging) ||
		    (prev_balancing_active && !balancing_active)) {
			segment_disable_balancing(
				acc_data->chips, &acc_data->balancing_thermal,
				&hspi2);
		}

		if (charging) {
			// in charging, debug data is required to get things like die temp
			segment_retrieve_charging_data(acc_data->chips, &hspi2);
			send_segment_pec_errors_message();
			segment_isospi_handle_state(acc_data->chips,
						    state_machine, &hspi2);
		} else {
			// snap before getting data
			segment_snap(acc_data->chips, &hspi2);
			segment_retrieve_active_data(acc_data->chips, &hspi2);
			// unsnap after getting data
			segment_unsnap(acc_data->chips, &hspi2);
			send_segment_pec_errors_message();
			segment_isospi_handle_state(acc_data->chips,
						    state_machine, &hspi2);

			if (DEBUG_MODE_ENABLED) {
				segment_retrieve_debug_data(acc_data->chips,
							    &hspi2);
			}
		}

		if (is_timer_expired(&open_wire_timer)) {
			segment_run_cell_open_wire_test(acc_data->chips, analyzer,
						    charging, &hspi2);
			segment_send_s_adc_cell_data(analyzer);
			start_timer(&open_wire_timer, open_wire_test_frequency);
		}

		if (charging && balancing_active &&
		    ((!prev_balancing_active) ||
		     thermal_state_changed ||
		     (is_timer_expired(&pwm_timer) &&
		      !is_timer_active(&pwm_timer)))) {
			segment_mute(acc_data->chips, &hspi2);
			segment_set_dcto(acc_data->chips, TIME_1MIN_OR_0_26HR,
					&hspi2);
			segment_configure_balancing(acc_data->chips,
						    acc_data->discharge_config,
						    &acc_data->balancing_thermal,
						    &hspi2);
			segment_read_pwm_registers(acc_data->chips, &hspi2);
			segment_unmute(acc_data->chips, &hspi2);
			start_timer(&pwm_timer, pwm_update_frequency);
		}

		HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);

		prev_balancing_active = balancing_active;
		set_flag(ANALYZER_FLAG);
		tx_thread_sleep(300);
	}
}
