
#include "shep_tasks.h"
#include "can_handler.h"
#include "can_messages.h"
#include "cell_temp_sanitizer.h"
#include "compute.h"
#include "control.h"
#include "hv_plate.h"
#include "isospi_recovery.h"
#include "main.h"
#include "precharge_routine.h"
#include "segment.h"
#include "shep_queues.h"
#include "soc.h"
#include "state_machine.h"
#include "timer.h"
#include "u_tx_can.h"
#include "u_tx_debug.h"
#include "u_tx_flags.h"
#include "u_tx_general.h"
#include "u_tx_threads.h"

void vDefaultTask(ULONG thread_input)
{
	bool alt = true;

	/* Infinite loop */
	for (;;) {
#ifdef DEBUG_STATS
// print_bms_stats(&bmsdata);
#endif

		if (alt) {
			printf(".\n");
		} else {
			printf("..\n");
		}

		alt = !alt;

		HAL_IWDG_Refresh(&hiwdg);
		tx_thread_sleep(MS_TO_TICKS(500));
	}
}

void vStateMachine(ULONG thread_input)
{
	PRINTLN_INFO("Starting State Machine thread...");

	state_machine_args_t *state_machine_args =
		(state_machine_args_t *)thread_input;
	state_machine_t *state_machine = state_machine_args->state_machine;
	analyzer_t *analyzer = state_machine_args->analyzer;

	nertimer_t telem_timer;
	// sends unimportant telemetry messages every 500ms
	start_timer(&telem_timer, 500);

	for (;;) {
		sm_handle_state(state_machine_args);

		if (is_timer_expired(&telem_timer)) {
			// these are unimportant telemetry messages so they can be sent
			// infrequently
			send_bms_status_message( // TODO: can be moved to CAN dispatch
				analyzer->avg_temp,
				analyzer->internal_temp, // TODO: we never set internal temp
				get_current_state(state_machine),
				get_current_state(state_machine) ==
					BALANCING); //  TODO: remove is balancing
			send_fault_status_message(
				state_machine->fault_code_crit,
				state_machine->fault_code_noncrit);
			start_timer(&telem_timer, 500);
		}

		tx_thread_sleep(MS_TO_TICKS(20));
	}
}

void vCanReceive(ULONG thred_input)
{
	can_msg_t message;
	for (;;) {
		/* Process incoming messages */
		while (queue_receive(&can_incoming, &message,
				     TX_WAIT_FOREVER) == U_SUCCESS) {
			switch (message.id) {
			case CHARGERBOX_CANID:
				// TODO process charger can message
				break;
			case DTI_CURRENT_CANID:
				// TODO process charger can message
				break;
			default:
				break;
			}
		}
	}
}

extern can_t *can1; // TODO: pass can1 directly into thread
void vCanDispatch(ULONG thread_input)
{
	can_msg_t message;
	uint8_t status;

	for (;;) {
		/* Process incoming messages */
		while (queue_receive(&can_outgoing, &message,
				     TX_WAIT_FOREVER) == U_SUCCESS) {
			status = can_send_msg(can1, &message);
			if (status != U_SUCCESS) {
				PRINTLN_WARNING(
					"Failed to send message (on can1) after removing from "
					"outgoing queue (Message ID: %ld) - Status %d",
					message.id, status);
			}
		}
	}
}

void vAnalyzer(ULONG thread_input)
{
	analyzer_args_t *analyzer_args = (analyzer_args_t *)thread_input;

	analyzer_t *analyzer = analyzer_args->analyzer;
	acc_data_t *acc_data = analyzer_args->acc_data;
	state_machine_t *state_machine = analyzer_args->state_machine;
	hv_plate_t *hv_plate = analyzer_args->hv_plate;

	create_mutex(&analyzer->analyzer_mutex);

	for (;;) {
		get_flag(ANALYZER_FLAG, TX_WAIT_FOREVER);

		// NOTE: All functions that modify chip data are externall mutexed
		mutex_get(&analyzer->analyzer_mutex);

		// calculate base values for later safety calcs
		calc_cell_temps(analyzer, acc_data);
		calc_pack_temps(analyzer, acc_data);
		calc_cell_voltages(analyzer, acc_data, state_machine);
		calc_open_cell_voltage(analyzer, acc_data, hv_plate);
		calc_pack_voltage_stats(analyzer, acc_data);
		calc_cell_resistances(analyzer, acc_data, hv_plate);
		update_chip_status(analyzer, acc_data);

		mutex_put(&analyzer->analyzer_mutex);

		set_flag(DEBUG_FLAG);

		// send out telemetry data sourced from the above functions
		send_cell_voltage_message(analyzer->max_ocv, analyzer->min_ocv,
					  analyzer->avg_ocv);
		send_segment_average_volt_message(
			analyzer); // TODO: Update CAN message send function defintions
		send_segment_total_volt_message(analyzer);
		send_cell_temp_message(analyzer->max_temp, analyzer->min_temp,
				       analyzer->avg_temp);
		send_segment_temp_message(analyzer);
	}
}

void vGetSegmentData(ULONG thread_input)
{
	acc_data_args_t *acc_data_args = (acc_data_args_t *)thread_input;

	acc_data_t *acc_data = acc_data_args->acc_data;
	state_machine_t *state_machine = acc_data_args->state_machine;

	segment_init(acc_data->chips, &hspi2);

	isospi_break_detection_init(acc_data->chips);

	// must delay after init for some reason, or else ADC doesnt start up (-3.45
	// or something)
	tx_thread_sleep(MS_TO_TICKS(500));

	for (;;) {
		segment_mute(acc_data->chips, &hspi2);

		if (get_current_state(state_machine) == CHARGING) {
			tx_thread_sleep(75);
			// must delay to let settle after balancing has halted, or else cells read
			// high
		}

		if (get_current_state(state_machine) == CHARGING) {
			// in charging, debug data is required to get things like die temp
			segment_retrieve_charging_data(acc_data->chips, &hspi2);

			isospi_handle_state(acc_data->chips, state_machine,
					    &hspi2);

		} else {
			// snap before getting data
			segment_snap(acc_data->chips, &hspi2);
			segment_retrieve_active_data(acc_data->chips, &hspi2);
			// unsnap after getting data
			segment_unsnap(acc_data->chips, &hspi2);

			isospi_handle_state(acc_data->chips, state_machine,
					    &hspi2);

			if (DEBUG_MODE_ENABLED) {
				segment_retrieve_debug_data(acc_data->chips,
							    &hspi2);
			}
		}

		if (get_current_state(state_machine) == CHARGING) {
			segment_unmute(acc_data->chips, &hspi2);
		}

		if (get_current_state(state_machine) == BALANCING) {
			segment_configure_balancing(
				acc_data->chips, acc_data->discharge_config,
				&hspi2); // TODO: Move to state machine
		}

		set_flag(ANALYZER_FLAG);
		tx_thread_sleep(MS_TO_TICKS(100));
	}
}

void vHvPlateData(ULONG thread_input)
{
	hv_plate_args_t *hv_plate_args = (hv_plate_args_t *)thread_input;

	hv_plate_t *hv_plate = hv_plate_args->hv_plate;
	analyzer_t *analyzer = hv_plate_args->analyzer;

	init_hv_plate_chip(*hv_plate->ic);
	tx_thread_sleep(TICKS_TO_MS(500));

	for (;;) {
		// get the current reading from the pack
		hv_plate->pack_current = get_pack_current(hv_plate->ic, &hspi2);

		// updates the SoC value in the analyzer struct based on the pack current
		// received
		update_soc(analyzer, hv_plate);

		// read voltages
		hv_plate->ts_volts = get_ts_voltage(hv_plate->ic, &hspi2);
		hv_plate->batt_volts = get_batt_voltage(hv_plate->ic, &hspi2);

		// read shunt temperature
		hv_plate->shunt_temp = get_shunt_temp(hv_plate->ic, &hspi2);

		tx_thread_sleep(MS_TO_TICKS(100));
	}
}

void vSanitizer(ULONG thread_input)
{
	sanitizer_args_t *sanitizer_args = (sanitizer_args_t *)thread_input;

	sanitizer_t *sanitizer = sanitizer_args->sanitizer;
	analyzer_t *analyzer = sanitizer_args->analyzer;

	temp_sanitizer_init(sanitizer);

	for (;;) {
		temp_sanitizer_run(sanitizer, analyzer);
		tx_thread_sleep(MS_TO_TICKS(500));
	}
}

void vPrecharge(ULONG args)
{
	hv_plate_t *hv_plate = (hv_plate_t *)args;

	prechargeconfig_t precharge_config;
	precharge_init(&precharge_config, hv_plate, 0.9f,
		       200 /* ms debounce time */);

	for (;;) {
		handle_precharge(&precharge_config);
		tx_thread_sleep(MS_TO_TICKS(50)); // TODO; fix thread timing
	}
}

void vBMSAlgorithms(ULONG thread_input)
{
	for (;;) {
		// TODO: implement algo thread
		tx_thread_sleep(MS_TO_TICKS(500));
	}
}

void vControl(ULONG thread_input)
{
	analyzer_t *analyzer = (analyzer_t *)thread_input;

	// Initialize peripherals for control
	bool failed = !control_init_peripherals();
	if (failed) {
		printf("Failed to initialize one or more peripherals.\n");
	}

	for (;;) {
		mutex_get(&analyzer->analyzer_mutex);
		float max_temp = analyzer->max_temp.val;
		handle_max_temp(max_temp);
		mutex_put(&analyzer->analyzer_mutex);

		tx_thread_sleep(MS_TO_TICKS(50));
	}
}

void vDebug(ULONG thread_input)
{
	analyzer_t *analyzer = (analyzer_t *)thread_input;

	for (;;) {
		get_flag(DEBUG_FLAG, TX_WAIT_FOREVER);

		for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
			chipdata_t *chip_data = get_chip_data(analyzer, chip);
			for (uint8_t cell = 0; cell < NUM_CELLS_PER_CHIP;
			     cell += 2) {
				// Sends two cells per messages
				// Accounts for odd number of cells
				send_cell_data_message(
					chip_data->alpha,
					chip_data->cell_temp[cell],
					chip_data->cell_voltages[cell],

					cell + 1 == NUM_CELLS_PER_CHIP ?
						0 :
						chip_data->cell_voltages[cell +
									 1],
					chip, cell, cell + 1,
					chip_data->is_balancing[cell],

					cell + 1 == NUM_CELLS_PER_CHIP ?
						0 :
						chip_data->is_balancing[cell +
									1],
					chip_data->cs_fault[cell],
					cell + 1 == NUM_CELLS_PER_CHIP ?
						0 :
						chip_data->cs_fault[cell + 1]);

				tx_thread_sleep(10); // TODO: enhance timing
			}

			send_status_a_message(chip_data->on_board_temp, chip,
					      chip_data->die_temp,
					      chip_data->vpv, chip_data->vmv,
					      &chip_data->flt_reg);

			tx_thread_sleep(30); // TODO: enhance timing

			send_status_b_message(chip_data->v_res, chip,
					      chip_data->vref2,
					      chip_data->v_analog,
					      chip_data->v_digital,
					      &chip_data->flt_reg);

			tx_thread_sleep(30); // TODO: enhance timing
		}
	}
}

uint8_t shep_threads_init(TX_BYTE_POOL *byte_pool)
{
	/* Init Interfaces Start */
	acc_data_t *acc_data = (acc_data_t *)malloc(sizeof(acc_data));
	analyzer_t *analyzer = (analyzer_t *)malloc(sizeof(analyzer_t));
	state_machine_t *state_machine =
		(state_machine_t *)malloc(sizeof(state_machine_t));
	hv_plate_t *hv_plate = (hv_plate_t *)malloc(sizeof(hv_plate_t));
	sanitizer_t *sanitizer = (sanitizer_t *)malloc(sizeof(sanitizer_t));
	bms_algos_t *bms_algos = (bms_algos_t *)malloc(sizeof(bms_algos_t));

	analyzer_args_t *analyzer_args =
		(analyzer_args_t *)malloc(sizeof(analyzer_args_t));
	analyzer_args->acc_data = acc_data;
	analyzer_args->hv_plate = hv_plate;
	analyzer_args->analyzer = analyzer;
	analyzer_args->state_machine = state_machine;

	acc_data_args_t *acc_data_args =
		(acc_data_args_t *)malloc(sizeof(acc_data_args_t));
	acc_data_args->acc_data = acc_data;
	acc_data_args->state_machine = state_machine;

	state_machine_args_t *state_machine_args =
		(state_machine_args_t *)malloc(sizeof(state_machine_args_t));
	state_machine_args->acc_data = acc_data;
	state_machine_args->analyzer = analyzer;
	state_machine_args->hv_plate = hv_plate;
	state_machine_args->state_machine = state_machine;
	state_machine_args->bms_algos = bms_algos;

	hv_plate_args_t *hv_plate_args =
		(hv_plate_args_t *)malloc(sizeof(hv_plate_args_t));
	hv_plate_args->hv_plate = hv_plate;
	hv_plate_args->analyzer = analyzer;

	sanitizer_args_t *sanitizer_args =
		(sanitizer_args_t *)malloc(sizeof(sanitizer_args_t));
	sanitizer_args->analyzer = analyzer;
	sanitizer_args->sanitizer = sanitizer;

	bms_algos_args_t *bms_algos_args =
		(bms_algos_args_t *)malloc(sizeof(bms_algos_args_t));
	bms_algos_args->analyzer = analyzer;
	bms_algos_args->sanitizer = sanitizer;
	bms_algos_args->bms_algos = bms_algos;

	/* Init Interfaces End */

	/* Task Definitions Start */

	thread_t _default_thread = {
		.name = "Default Task Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 2, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vDefaultTask /* Thread Function */
	};

	thread_t _state_machine_thread = {
		.name = "State Machine Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 4, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)state_machine_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vStateMachine /* Thread Function */
	};

	thread_t _can_dispatch_thread = {
		.name = "CAN Dispatch Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 1, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vCanDispatch /* Thread Function */
	};

	thread_t _can_receive_thread = {
		.name = "Can Receive Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 2, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vCanReceive /* Thread Function */
	};

	thread_t _analyzer_thread = {
		.name = "Analyzer Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 6, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)analyzer_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vAnalyzer /* Thread Function */
	};

	thread_t _segment_data_thread = {
		.name = "Segment Data Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 1, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)acc_data_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vGetSegmentData, /* Thread Function */
	};

	thread_t _hv_plate_data_thread = {
		.name = "HV Plate Data Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 2, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)hv_plate_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vHvPlateData, /* Thread Function */
	};

	thread_t _sanitizer_thread = {
		.name = " Sanitizer Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 3, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)sanitizer_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vSanitizer, /* Thread Function */
	};

	thread_t _bms_algorithms_thread = {
		.name = "BMS Algorithms Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 4, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)bms_algos_args, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vBMSAlgorithms, /* Thread Function */
	};

	thread_t _control_thread = {
		.name = "Control Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 4, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)analyzer, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vControl, /* Thread Function */
	};

	thread_t _debug_thread = {
		.name = "BMS Debug Mode Thread", /* Name */
		.size = 2048, /* Stack Size (in bytes) */
		.priority = 4, /* Priority */
		.threshold = 0, /* Preemption Threshold */
		.thread_input = (ULONG)analyzer, /* Thread Args */
		.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
		.auto_start = TX_AUTO_START, /* Auto Start */
		.function = vDebug, /* Thread Function */
	};

	/* Task Definitions End */
	CATCH_ERROR(create_thread(byte_pool, &_default_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_state_machine_thread),
		    U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_analyzer_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_can_dispatch_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_can_receive_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_segment_data_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_hv_plate_data_thread),
		    U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_sanitizer_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_bms_algorithms_thread),
		    U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_control_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_debug_thread), U_SUCCESS);

	PRINTLN_INFO("Ran threads_init()");
	return U_SUCCESS;
}
