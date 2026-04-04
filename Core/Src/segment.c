
#include "segment.h"
#include "adBms6830Data.h"
#include "adi6830_interation.h"
#include "bms_config.h"
#include "c_utils.h"
#include "isospi_recovery.h"
#include "serialPrintResult.h"
#include "u_tx_flags.h"
#include "state_machine.h"
#include "app_threadx.h"

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
			set_iso_spi_line(&chips[chip], ISOSPI_LINE_A);

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

void segment_adc_comparison(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi)
{
	// TODO: S-ADC measurements are all over the place.

	// Take single shot measurement
	// adBms6830_Adcv(RD_ON, SINGLE, DCP_OFF, RSTF_OFF, OW_OFF_ALL_CH);
	// adBmsPollAdc_indicator(PLCADC);
	// read_adbms_data(bmsdata->chips, RDCVALL, Rdcvall, ALL_GRP);

	// Result of C-ADC and S-ADC comparison is stored in status register group C
	read_status_registers(chips, hspi);

	for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			if (NER_GET_BIT(chips[chip].statc.cs_flt, cell)) {
				printf("ADC VOLTAGE DISCREPANCY ERROR\nChip %d, Cell %d\nC-ADC: %f, "
				       "S-ADC: %f\n",
				       chip + 1, cell + 1,
				       getVoltage(
					       chips[chip].fcell.fc_codes[cell]),
				       getVoltage(
					       chips[chip]
						       .scell.sc_codes[cell]));
			}
		}
	}
}

void segment_monitor_flts(cell_asic chips[NUM_CHIPS], SPI_HandleTypeDef *hspi)
{
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		// printf("CHIP %d :", chip);
		//printf("MUTE: %d, %d\n", chip, chips[chip].rx_cfga.mute_st);
		if (chips[chip].statc.cs_flt > 0) {
			printf("C VS S MISMATCH on cells ");
			for (int i = 0; i < NUM_CELLS_PER_CHIP; i++) {
				if (NER_GET_BIT(chips[chip].statc.cs_flt, i)) {
					printf("%d, ", i);
				}
			}
			printf(" of c%d\n", chip);
		}
		if (chips[chip].statc.va_ov) {
			printf("A OV FLT c%d\n", chip);
		}
		if (chips[chip].statc.va_uv) {
			printf("A UV FLT c%d\n", chip);
		}
		if (chips[chip].statc.vd_ov) {
			printf("D OV FLT c%d\n", chip);
		}
		if (chips[chip].statc.vd_uv) {
			printf("D UV FLT c%d\n", chip);
		}
		if (chips[chip].statc.vde) {
			printf("VDE FLT c%d\n", chip);
		}
		if (chips[chip].statc.vdel) {
			printf("VDEL FLT c%d\n", chip);
		}
		if (chips[chip].statc.spiflt) {
			printf("SPI SLV FLT c%d\n", chip);
		}
		if (chips[chip].statc.sleep) {
			printf("SLEEP OCCURED c%d\n", chip);
		}
		if (chips[chip].statc.thsd) {
			printf("THERMAL FLT c%d\n", chip);
		}
		if (chips[chip].statc.tmodchk) {
			printf("TMODE FLT c%d\n", chip);
		}

		if (chips[chip].statc.otp1_med) {
			printf("CMED? FLT c%d\n", chip);
		}
		if (chips[chip].statc.otp2_med) {
			printf("SMED? FLT c%d\n", chip);
		}
	}
	// clear them.  they will still be in memory for usage until this function or
	// read_status_registers is called
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

	// read from ADC convs
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

	read_s_voltage_registers(chips, hspi);
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
		// right now this checks all cells, even depop-ed ones
		for (uint8_t i = 0; i < 12; i++) {
			if (chips[chip].PwmA.pwma[i] > 0) {
				return true;
			}
		}
		for (uint8_t i = 0; i < 4; i++) {
			if (chips[chip].PwmB.pwmb[i] > 0) {
				return true;
			}
		}
	}
	return false;
}

void segment_disable_balancing(cell_asic chips[NUM_CHIPS],
			       SPI_HandleTypeDef *hspi)
{
	// Initializes all array elements to zero
	bool discharge_config[NUM_CHIPS][NUM_CELLS_PER_CHIP] = { 0 };
	segment_configure_balancing(chips, discharge_config, hspi);

	// force balancing muted
	mute_chips(chips, hspi);
}

void segment_enable_balancing(cell_asic chips[NUM_CHIPS],
			      SPI_HandleTypeDef *hspi)
{
	unmute_chips(chips, hspi);
}

void segment_manual_balancing(cell_asic chips[NUM_CHIPS],
			      SPI_HandleTypeDef *hspi)
{
	// clang-format off
	bool discharge_confg[NUM_CHIPS][NUM_CELLS_PER_CHIP] = {
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};
	// clang-format on

	segment_configure_balancing(chips, discharge_confg, hspi);
}

void segment_configure_balancing(
	cell_asic chips[NUM_CHIPS],
	bool discharge_config[NUM_CHIPS][NUM_CELLS_PER_CHIP],
	SPI_HandleTypeDef *hspi)
{
	for (int chip = 0; chip < NUM_CHIPS; chip++) {
		for (int cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			set_cell_discharge(&chips[chip], cell,
					   discharge_config[chip][cell]);
		}
	}
	write_config_regs(chips, hspi);
}

// GET SEGEMENT DATA THREAD
void vGetSegmentData(ULONG thread_input)
{
	PRINTLN_INFO("Starting GetSegmentData thread...");

	const uint16_t balancing_delay = 75;

	acc_data_args_t *acc_data_args = (acc_data_args_t *)thread_input;

	acc_data_t *acc_data = acc_data_args->acc_data;
	state_machine_t *state_machine = acc_data_args->state_machine;

	segment_init(acc_data->chips, &hspi2);

	isospi_break_detection_init(acc_data->chips);

	// must delay after init for ADC to start up
	tx_thread_sleep(MS_TO_TICKS(200));

	state_t prev_state = BOOT;
	state_t current_state = BOOT;

	for (;;) {
		segment_mute(acc_data->chips, &hspi2);

		prev_state = current_state;
		current_state = state_machine->bms_state;

		if (prev_state == BALANCING && current_state == CHARGING) {
			tx_thread_sleep(MS_TO_TICKS(
				balancing_delay)); // delay after balancing to let cells settle
		}

		if (current_state == CHARGING || current_state == BALANCING) {
			// in charging, debug data is required to get things like die temp
			segment_retrieve_charging_data(acc_data->chips, &hspi2);
			send_segment_pec_errors_message();
			isospi_handle_state(acc_data->chips, state_machine,
					    &hspi2);
		} else {
			// snap before getting data
			segment_snap(acc_data->chips, &hspi2);
			segment_retrieve_active_data(acc_data->chips, &hspi2);
			// unsnap after getting data
			segment_unsnap(acc_data->chips, &hspi2);
			send_segment_pec_errors_message();
			isospi_handle_state(acc_data->chips, state_machine,
					    &hspi2);

			if (DEBUG_MODE_ENABLED) {
				segment_retrieve_debug_data(acc_data->chips,
							    &hspi2);
			}
		}

		if (current_state == CHARGING || current_state == BALANCING) {
			segment_unmute(acc_data->chips, &hspi2);
		}

		if (current_state == BALANCING) {
			segment_configure_balancing(
				acc_data->chips, acc_data->discharge_config,
				&hspi2);
		}

		set_flag(ANALYZER_FLAG);
		tx_thread_sleep(MS_TO_TICKS(300));
	}
}
