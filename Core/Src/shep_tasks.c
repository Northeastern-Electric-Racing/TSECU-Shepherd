
#include "u_tx_threads.h"
#include "u_tx_debug.h"
#include "u_tx_general.h"
#include "u_tx_can.h"
#include "shep_queues.h"
#include "can_messages.h"
#include "shep_mutexes.h"
#include "shep_tasks.h"
#include "timer.h"
#include "state_machine.h"
#include "can_handler.h"
#include "u_tx_flags.h"
#include "segment.h"
#include "main.h"
#include "hv_plate.h"
#include "compute.h"
#include "cell_temp_sanitizer.h"

bms_t bmsdata;

// TODO: default task

static thread_t _default_thread = {
	.name = "Default Task Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 2, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = 1000, /* Sleep (in ticks) */
	.function = vDefaultTask /* Thread Function */
};

void vDefaultTask(ULONG thread_input)
{
	bool alt = true;

	/* Infinite loop */
	for (;;) {
#ifdef DEBUG_STATS
//print_bms_stats(&bmsdata);
#endif

		if (alt) {
			printf(".\n");
		} else {
			printf("..\n");
		}

		alt = !alt;

		HAL_IWDG_Refresh(&hiwdg);
		tx_thread_sleep(MS_TO_TICKS(_default_thread.sleep));
	}
}

static thread_t _state_machine_thread = {
	.name = "State Machine Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 4, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = 20, /* Sleep (in ticks) */
	.function = vStateMachine /* Thread Function */
};

void vStateMachine(ULONG thread_input)
{
	PRINTLN_INFO("Starting State Machine thread...");

	nertimer_t telem_timer;
	// sends unimportant telemetry messages every 500ms
	start_timer(&telem_timer, 500);

	for (;;) {
		sm_handle_state(&bmsdata);

		if (is_timer_expired(&telem_timer)) {
			// these are unimportant telemetry messages so they can be sent infrequently
			send_bms_status_message(
				bmsdata.avg_temp, bmsdata.internal_temp,
				bmsdata.current_state,
				segment_is_balancing(bmsdata.chips));
			send_fault_status_message(bmsdata.fault_code_crit,
						  bmsdata.fault_code_noncrit);
			start_timer(&telem_timer, 500);
		}

		tx_thread_sleep(MS_TO_TICKS(_state_machine_thread.sleep));
	}
}

static thread_t _can_receive_thread = {
	.name = "Can Receive Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 2, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = 500,
	/* Sleep (in ticks) */ // TODO Change Can Receive to be triggered by thread flag
	.function = vCanReceive /* Thread Function */
};

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

		tx_thread_sleep(MS_TO_TICKS(_can_receive_thread.sleep));
	}
}

static thread_t _can_dispatch_thread = {
	.name = "CAN Dispatch Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 1, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = 20,
	/* Sleep (in ticks) */ // TODO: Change to trigger thread flag
	.function = vCanDispatch /* Thread Function */
};

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
				PRINTLN_INFO(
					"WARNING: Failed to send message (on can1) after removing from outgoing queue (Message ID: %ld) - Status %d",
					message.id, status);
				// u_TODO - maybe add the message back into the queue if it fails to send? not sure if this is a good idea tho
			}
		}

		tx_thread_sleep(MS_TO_TICKS(_can_dispatch_thread.sleep));
	}
}

static thread_t _analyzer_thread = {
	.name = "Analyzer Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 6, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = 100, /* Sleep (in ticks) */
	.function = vAnalyzer /* Thread Function */
};

void vAnalyzer(ULONG thread_input)
{
	for (;;) {
		get_flag(ANALYZER_FLAG, TX_WAIT_FOREVER);

		mutex_get(&bms_mutex);

		// calculate base values for later safety calcs
		calc_cell_temps(&bmsdata);
		calc_pack_temps(&bmsdata);
		calc_cell_voltages(&bmsdata);
		calc_open_cell_voltage(&bmsdata);
		calc_pack_voltage_stats(&bmsdata);
		calc_cell_resistances(&bmsdata);

		// send out telemetry data sourced from the above functions
		send_cell_voltage_message(bmsdata.max_ocv, bmsdata.min_ocv,
					  bmsdata.avg_ocv);
		send_segment_average_volt_message(&bmsdata);
		send_segment_total_volt_message(&bmsdata);
		send_cell_temp_message(bmsdata.max_temp, bmsdata.min_temp,
				       bmsdata.avg_temp);
		send_segment_temp_message(&bmsdata);

		mutex_put(&bms_mutex);
	}
}

static thread_t _segment_data_thread = {
	.name = "Segment Data Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 1, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = 2, /* Sleep (in ticks) */
	.function = vGetSegmentData, /* Thread Function */
};

void vGetSegmentData(ULONG thread_input)
{

	acc_data_t *acc_data = (acc_data_t *)acc_data;

	segment_init(acc_data->chips, &hspi2);

	// must delay after init for some reason, or else ADC doesnt start up (-3.45 or something)
	tx_thread_sleep(MS_TO_TICKS(500));

	for (;;) {
		segment_mute(acc_data->chips, &hspi2);

		if (acc_data->bms_state == CHARGING) {
			tx_thread_sleep(75);
			// must delay to let settle after balancing has halted, or else cells read high
		}

		if (acc_data->bms_state == CHARGING) {
			// in charging, debug data is required to get things like die temp
			segment_retrieve_charging_data(acc_data->chips, &hspi2);
		} else {
			// snap before getting data
			segment_snap(acc_data->chips, &hspi2);
			segment_retrieve_active_data(acc_data->chips, &hspi2);
			// unsnap after getting data
			segment_unsnap(acc_data->chips, &hspi2);
			if (DEBUG_MODE_ENABLED) {
				segment_retrieve_debug_data(acc_data->chips,
							    &hspi2);
			}
		}

		if (acc_data->bms_state == CHARGING) {
			segment_unmute(acc_data->chips, &hspi2);
		}

		if (acc_data->bms_state == BALANCING) {
			segment_configure_balancing(acc_data->chips,
						    acc_data->discharge_config,
						    &hspi2);
		}

		set_flag(ANALYZER_FLAG);
		tx_thread_sleep(MS_TO_TICKS(100));
	}
}

static thread_t _hv_plate_data_thread = {
	.name = "HV Plate Data Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 2, /* Priority */	
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = MS_TO_TICKS(100), /* Sleep (in ticks) */
	.function = vHvPlateData, /* Thread Function */
};

void vHvPlateData(ULONG thread_input)
{
	hv_plate_t *hv_plate = (hv_plate_t *)thread_input;

	init_hv_plate_chip(*hv_plate->ic);
	tx_thread_sleep(TICKS_TO_MS(500));
	float current = 0;
	for (;;) {
		// get the current reading from the pack
		hv_plate->pack_current = get_pack_current(hv_plate->ic, &hspi2);

		// read voltages
		hv_plate->ts_volts = get_ts_voltage(hv_plate->ic, &hspi2);
		hv_plate->batt_volts = get_batt_voltage(hv_plate->ic, &hspi2);

		// read shunt temperature
		hv_plate->shunt_temp = get_shunt_temp(hv_plate->ic, &hspi2);

		tx_thread_sleep(MS_TO_TICKS(_hv_plate_data_thread.sleep));
	}
}

static thread_t _sanitizer_thread = {
	.name = "Sanitizer Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 3, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = MS_TO_TICKS(500), /* Sleep (in ticks) */
	.function = vHvPlateData, /* Thread Function */
};

void vSanitizer(ULONG thread_input)
{
	therm_state_t therm_states[NUM_CHIPS][NUM_CELLS_PER_CHIP];
	temp_sanitizer_init(therm_states);

	for (;;) {
		temp_sanitizer_run(bmsdata.chip_data, therm_states);
		tx_thread_sleep(MS_TO_TICKS(_hv_plate_data_thread.sleep));
	}
}

uint8_t shep_threads_init(TX_BYTE_POOL *byte_pool)
{
	CATCH_ERROR(create_thread(byte_pool, &_default_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_state_machine_thread),
		    U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_analyzer_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_can_dispatch_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_can_receive_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_segment_data_thread), U_SUCCESS);
	//CATCH_ERROR(create_thread(byte_pool, &_hv_plate_data_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_sanitizer_thread), U_SUCCESS);

	PRINTLN_INFO("Ran threads_init()");
	return U_SUCCESS;
}