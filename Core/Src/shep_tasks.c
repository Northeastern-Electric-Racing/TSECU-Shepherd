
#include <assert.h>

#include "shep_tasks.h"
#include "can_handler.h"
#include "can_messages_tx.h"
#include "ccl.h"
#include "cell_temp_sanitizer.h"
#include "compute.h"
#include "control.h"
#include "bms_algos.h"
#include "dcl.h"
#include "ethernet.h"
#include "hv_plate.h"
#include "isospi_recovery.h"
#include "main.h"
#include "precharge_routine.h"
#include "segment.h"
#include "serialPrintResult.h"
#include "u_queues.h"
#include "soc.h"
#include "state_machine.h"
#include "timer.h"
#include "u_nx_ethernet.h"
#include "u_tx_can.h"
#include "u_tx_debug.h"
#include "u_tx_flags.h"
#include "u_tx_general.h"
#include "u_tx_threads.h"
#include "u_tx_mutex.h"

const void print_bms_stats(analyzer_t *analyzer, hv_plate_t *hv_plate,
			   acc_data_t *acc_data, bms_algos_t *bms_algos)
{
#ifdef DEBUG_HV_PLATE
	PRINTLN_INFO("HV Plate Data:");
	PRINTLN_INFO("TS Voltage: %.3f V", hv_plate->ts_volts);
	PRINTLN_INFO("BATT Voltage: %.3f V", hv_plate->batt_volts);
	PRINTLN_INFO("Shunt Temp: %.2f C", hv_plate->shunt_temp);
	PRINTLN_INFO("Pack Current: %.3f A", hv_plate->pack_current);
	PRINTLN_INFO("VREG: %.3f V", hv_plate->vreg);
	PRINTLN_INFO("VREF1P25: %.3f V", hv_plate->vref1p25);
	PRINTLN_INFO("EPAD: %.3f V", hv_plate->epad);
	PRINTLN_INFO("VDIG: %.3f V", hv_plate->vdig);
	PRINTLN_INFO("VDD: %.3f V", hv_plate->vdd);
	PRINTLN_INFO("VDIV: %.3f V", hv_plate->vdiv);
	PRINTLN_INFO("Primary Internal Temperature: %.3f C", hv_plate->tmp1);
	PRINTLN_INFO("Secondary Internal Temperature: %.3f C", hv_plate->tmp2);

	if (hv_plate->adbms_flags.raw > 0) {
		if (hv_plate->adbms_flags.flags.noclk) {
			PRINTLN_WARNING("HVP FLT: NOCLK - OSC1 STUCK");
		}
		if (hv_plate->adbms_flags.flags.oscflt) {
			PRINTLN_WARNING("HVP FLT: OSCFLT - OSC1 vs OSC2 diff");
		}
		if (hv_plate->adbms_flags.flags.reset) {
			PRINTLN_WARNING("HVP FLT: RESET - EVENT DETECTED");
		}
		if (hv_plate->adbms_flags.flags.spiflt) {
			PRINTLN_WARNING("HVP FLT: SPIFLT - SPI SDO mismatch");
		}
		if (hv_plate->adbms_flags.flags.thsd) {
			PRINTLN_WARNING("HVP FLT: THSD - Thermal Shutdown");
		}
		if (hv_plate->adbms_flags.flags.vdduv) {
			PRINTLN_WARNING("HVP FLT: VDDUV - VDD Undervoltage");
		}
		if (hv_plate->adbms_flags.flags.vde) {
			PRINTLN_WARNING(
				"HVP FLT: VDE - Mismatch on VREG or GND");
		}
		if (hv_plate->adbms_flags.flags.vdel) {
			PRINTLN_WARNING(
				"HVP FLT: VDEL - Mismatch on VREG or GND");
		}
		if (hv_plate->adbms_flags.flags.vdigov) {
			PRINTLN_WARNING(
				"HVP FLT: VDIGOV - Overvoltage on VDIG");
		}
		if (hv_plate->adbms_flags.flags.vdiguv) {
			PRINTLN_WARNING(
				"HVP FLT: VDIGUV - Undervoltage on VDIG");
		}
		if (hv_plate->adbms_flags.flags.vregov) {
			PRINTLN_WARNING(
				"HVP FLT: VREGOV - Overvoltage on VREG");
		}
		if (hv_plate->adbms_flags.flags.vreguv) {
			PRINTLN_WARNING(
				"HVP FLT: VREGUV - Undervoltage on VREG");
		}
	}
#endif

#ifdef DEBUG_VOLTAGES
	PRINTLN_INFO("Min, Max, Avg, Delta Voltages: %f, %f, %f, %f\n",
		     analyzer->min_voltage.val, analyzer->max_voltage.val,
		     analyzer->avg_voltage, analyzer->delt_voltage);

	PRINTLN_INFO("Raw Cell Voltages:");
	for (uint8_t c = 0; c < NUM_CHIPS; c++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			PRINTLN_INFO(
				"%.3f\t",
				analyzer->chip_data[c].cell_voltages[cell]);
		}
		printf("\n");
	}

	PRINTLN_INFO("S ADC Voltages:");
	for (uint8_t c = 0; c < NUM_CHIPS; c++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			PRINTLN_INFO(
				"%.3f\t",
				getVoltage(acc_data->chips[c].scell.sc_codes[cell]));
		}
		printf("\n");
	}

	PRINTLN_INFO("Raw Cell OCV:");
	for (uint8_t c = 0; c < NUM_CHIPS; c++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			PRINTLN_INFO(
				"%.3f\t",
				analyzer->chip_data[c].open_cell_voltage[cell]);
		}
		printf("\n");
	}
#endif

#ifdef DEBUG_TEMPS
	PRINTLN_INFO("Therm Temps:");
	for (uint8_t c = 0; c < NUM_CHIPS; c++) {
		for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
			PRINTLN_INFO("Chip %d, Cell: %d, %.1f C\t",
				     c, cell, analyzer->chip_data[c].cell_temp[cell]);
		}
		printf("\n");
	}
	PRINTLN_INFO("CHIP TEMPS: \n");
	for (uint8_t c = 0; c < NUM_CHIPS; c++) {
		PRINTLN_INFO("Chip: %d, %.1f\t C", c, analyzer->chip_data[c].die_temp);
	}
	printf("\n");
#endif

#ifdef DEBUG_ALGOS
	PRINTLN_INFO("Cont CCL: %.2f A, Const DCL: %.2f A\n",
		     bms_algos->cont_CCL, bms_algos->cont_DCL);

	PRINTLN_INFO("Inst CCL: %.2f A, Inst DCL: %.2f A\n",
		     bms_algos->inst_CCL, bms_algos->inst_DCL);
#endif
}

void vDefaultTask(ULONG thread_input)
{
	PRINTLN_INFO("Starting Default thread...");

	default_task_args_t *default_task_args =
		(default_task_args_t *)thread_input;
	analyzer_t *analyzer = default_task_args->analyzer;
	acc_data_t *acc_data = default_task_args->acc_data;
	hv_plate_t *hv_plate = default_task_args->hv_plate;
	bms_algos_t *bms_algos = default_task_args->bms_algos;

	bool alt = true;

	/* Infinite loop */
	for (;;) {
#ifdef DEBUG_STATS
		print_bms_stats(analyzer, hv_plate, acc_data, bms_algos);
#endif

		if (alt) {
			printf(".\n");
		} else {
			printf("..\n");
		}

		alt = !alt;

		HAL_IWDG_Refresh(&hiwdg);
		tx_thread_sleep(MS_TO_TICKS(100));
	}
}

void vDebug(ULONG thread_input)
{
	PRINTLN_INFO("Starting Debug thread...");

	analyzer_t *analyzer = (analyzer_t *)thread_input;

	for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
		analyzer->chip_data[chip].alpha = (chip % 2 == 0);
	}

	for (;;) {
		get_flag(DEBUG_FLAG, TX_WAIT_FOREVER);

		for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
			chipdata_t *chip_data = get_chip_data(analyzer, chip);
			for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP;
			     cell += 2) {
				// Sends two cells per messages
				// Accounts for odd number of cells

				if (chip_data->alpha) {
					send_alpha_cell_data_debug(
						chip_data->cell_temp[cell],
						chip_data->cell_voltages[cell],

						cell + 1 == NUM_CELLS_PER_CHIP ?
							0 :
							chip_data->cell_voltages
								[cell + 1],
						chip, cell, cell + 1,
						chip_data->is_balancing[cell],

						cell + 1 == NUM_CELLS_PER_CHIP ?
							0 :
							chip_data->is_balancing
								[cell + 1],
						chip_data->cs_fault[cell],
						cell + 1 == NUM_CELLS_PER_CHIP ?
							0 :
							chip_data->cs_fault[cell +
									    1]);
				} else {
					send_beta_cell_data_debug(
						chip_data->cell_temp[cell],
						chip_data->cell_voltages[cell],

						cell + 1 == NUM_CELLS_PER_CHIP ?
							0 :
							chip_data->cell_voltages
								[cell + 1],
						chip - 1, cell, cell + 1,
						chip_data->is_balancing[cell],

						cell + 1 == NUM_CELLS_PER_CHIP ?
							0 :
							chip_data->is_balancing
								[cell + 1],
						chip_data->cs_fault[cell],
						cell + 1 == NUM_CELLS_PER_CHIP ?
							0 :
							chip_data->cs_fault[cell +
									    1]);
				}

				tx_thread_sleep(10); // TODO: enhance timing
			}


			if (chip % 2 == 0) {
			    send_alpha_chip_a_debug(chip / 2, chip_data->die_temp,
					  chip_data->vpv, chip_data->vmv,
					  chip_data->flt_reg.va_ov > 0,
					  chip_data->flt_reg.va_uv > 0,
					  chip_data->flt_reg.vd_ov > 0,
					  chip_data->flt_reg.vd_uv > 0,
					  chip_data->flt_reg.vde > 0,
					  chip_data->flt_reg.vdel > 0,
					  chip_data->flt_reg.spiflt > 0,
					  chip_data->flt_reg.sleep > 0,
					  chip_data->flt_reg.thsd > 0,
					  chip_data->flt_reg.tmodchk > 0,
					  chip_data->flt_reg.oscchk > 0);

				tx_thread_sleep(30); // TODO: enhance timing

				send_alpha_chip_b_debug(chip_data->v_res, chip / 2,
					chip_data->vref2, chip_data->v_analog,
				    chip_data->v_digital,
					chip_data->flt_reg.otp1_med > 0,
				    chip_data->flt_reg.otp2_med > 0);
			} else {
                send_beta_chip_a_debug(chip / 2, chip_data->die_temp,
                    chip_data->vpv, chip_data->vmv,
                    chip_data->flt_reg.va_ov > 0,
                    chip_data->flt_reg.va_uv > 0,
                    chip_data->flt_reg.vd_ov > 0,
                    chip_data->flt_reg.vd_uv > 0,
                    chip_data->flt_reg.vde > 0,
                    chip_data->flt_reg.vdel > 0,
                    chip_data->flt_reg.spiflt > 0,
                    chip_data->flt_reg.sleep > 0,
                    chip_data->flt_reg.thsd > 0,
                    chip_data->flt_reg.tmodchk > 0,
                    chip_data->flt_reg.oscchk > 0);

                tx_thread_sleep(30); // TODO: enhance timing

                send_beta_chip_b_debug(chip_data->v_res, chip / 2,
                    chip_data->vref2, chip_data->v_analog,
                    chip_data->v_digital,
                    chip_data->flt_reg.otp1_med > 0,
                    chip_data->flt_reg.otp2_med > 0);
			}



			send_onboard_therm_temperatures(
				chip, chip_data->on_board_temp[0],
				chip_data->on_board_temp[1],
				chip_data->on_board_temp[2]);

			tx_thread_sleep(30); // TODO: enhance timings
		}
	}
}

uint8_t shep_threads_init(TX_BYTE_POOL *byte_pool)
{
	/* Init Interfaces Start */
	static acc_data_t acc_data = { 0 };
	static analyzer_t analyzer = { 0 };
	static state_machine_t state_machine = { 0 };
	static hv_plate_t hv_plate = { 0 };
	static sanitizer_t sanitizer = { 0 };
	static bms_algos_t bms_algos = { 0 };
	static peripherals_t peripherals = { 0 };

	static default_task_args_t default_task_args = { 0 };
	default_task_args.analyzer = &analyzer;
	default_task_args.acc_data = &acc_data;
	default_task_args.hv_plate = &hv_plate;
	default_task_args.bms_algos = &bms_algos;

	static analyzer_args_t analyzer_args = { 0 };
	analyzer_args.acc_data = &acc_data;
	analyzer_args.hv_plate = &hv_plate;
	analyzer_args.analyzer = &analyzer;
	analyzer_args.state_machine = &state_machine;

	static acc_data_args_t acc_data_args = { 0 };
	acc_data_args.acc_data = &acc_data;
	acc_data_args.state_machine = &state_machine;

	static state_machine_args_t state_machine_args = { 0 };
	state_machine_args.acc_data = &acc_data;
	state_machine_args.analyzer = &analyzer;
	state_machine_args.hv_plate = &hv_plate;
	state_machine_args.state_machine = &state_machine;
	state_machine_args.bms_algos = &bms_algos;
	state_machine_args.sanitizer = &sanitizer;
	state_machine_args.peripherals = &peripherals;

	static hv_plate_args_t hv_plate_args = { 0 };
	hv_plate_args.hv_plate = &hv_plate;
	hv_plate_args.analyzer = &analyzer;
	hv_plate_args.state_machine = &state_machine;
	hv_plate_args.bms_algos = &bms_algos;

	static sanitizer_args_t sanitizer_args = { 0 };
	sanitizer_args.analyzer = &analyzer;
	sanitizer_args.sanitizer = &sanitizer;

	static bms_algos_args_t bms_algos_args = { 0 };
	bms_algos_args.analyzer = &analyzer;
	bms_algos_args.sanitizer = &sanitizer;
	bms_algos_args.bms_algos = &bms_algos;

	static peripherals_args_t peripherals_args = { 0 };
	peripherals_args.peripherals = &peripherals;

	PRINTLN_INFO("FINISHED INITIALIZING INTERFACES");

	/* Init Interfaces End */

	/* Task Definitions Start */

	static thread_t _default_thread = {
		.name = "Default Task Thread", /* Name */
		.size = 1024, /* Stack Size (in bytes) */
		.priority = 2, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&default_task_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vDefaultTask /* Thread Function */
	};

	static thread_t _state_machine_thread = {
		.name = "State Machine Thread", /* Name */
		.size = 5120, /* Stack Size (in bytes) */
		.priority = 4, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&state_machine_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vStateMachine /* Thread Function */
	};

	static thread_t _can_receive_thread = {
		.name = "Can Receive Thread", /* Name */
		.size = 1024, /* Stack Size (in bytes) */
		.priority = 2, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&state_machine_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vCanReceive /* Thread Function */
	};

	static thread_t _can_dispatch_thread = {
		.name = "CAN Dispatch Thread", /* Name */
		.size = 1024, /* Stack Size (in bytes) */
		.priority = 1, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vCanDispatch /* Thread Function */
	};

	static thread_t _ethernet_incoming_thread = {
		.name = "Ethernet Incoming Thread", /* Name */
		.size = 1024, /* Stack Size (in bytes) */
		.priority = 1, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vEthernetIncoming /* Thread Function */
	};

	static thread_t _ethernet_outgoing_thread = {
		.name = "Ethernet Outgoing Thread", /* Name */
		.size = 1024, /* Stack Size (in bytes) */
		.priority = 1, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vEthernetOutgoing /* Thread Function */
	};

	static thread_t _analyzer_thread = {
		.name = "Analyzer Thread", /* Name */
		.size = 1024, /* Stack Size (in bytes) */
		.priority = 1, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&analyzer_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vAnalyzer /* Thread Function */
	};

	static thread_t _segment_data_thread = {
		.name = "Segment Data Thread", /* Name */
		.size = 4096, /* Stack Size (in bytes) */
		.priority = 1, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&acc_data_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vGetSegmentData, /* Thread Function */
	};

	static thread_t _hv_plate_data_thread = {
		.name = "HV Plate Data Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 2, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&hv_plate_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vHvPlateData, /* Thread Function */
	};

	static thread_t _sanitizer_thread = {
		.name = "Sanitizer Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 3, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&sanitizer_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vSanitizer, /* Thread Function */
	};

	static thread_t _bms_algorithms_thread = {
		.name = "BMS Algorithms Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 4, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&bms_algos_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vBMSAlgorithms, /* Thread Function */
	};

	static thread_t _control_thread = {
		.name = "Control Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 4, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&analyzer, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vControl, /* Thread Function */
	};

	static thread_t _peripherals_thread = {
		.name = "Peripherals Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 2, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&peripherals_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vPeripherals, /* Thread Function */
	};

	static thread_t _precharge_thread = {
		.name = "Precharge Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 4, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&hv_plate, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vPrecharge, /* Thread Function */
	};

	static thread_t _debug_thread = {
		.name = "BMS Debug Mode Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 4, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)&analyzer, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vDebug, /* Thread Function */
	};

	PRINTLN_INFO("RUNNING THREADS");

	/* Task Definitions End */
	CATCH_ERROR(create_thread(byte_pool, &_default_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_state_machine_thread),
		    U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_analyzer_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_can_receive_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_can_dispatch_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_ethernet_incoming_thread),
		    U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_ethernet_outgoing_thread),
		    U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_segment_data_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_hv_plate_data_thread),
		    U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_sanitizer_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_bms_algorithms_thread),
		    U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_control_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_peripherals_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_precharge_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_debug_thread), U_SUCCESS);

	PRINTLN_INFO("Ran threads_init()");
	return U_SUCCESS;
}
