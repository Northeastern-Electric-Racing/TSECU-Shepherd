
#include "u_tx_threads.h"
#include "u_tx_debug.h"
#include "u_tx_general.h"
#include "u_tx_can.h"
#include "shep_queues.h"
#include "can_messages.h"
#include "shep_mutexes.h"
#include "shep_tasks.h"
#include "state_machine.h"
#include "timer.h"
#include "can_handler.h"
#include "main.h"

// TODO: Fill in threads

TX_EVENT_FLAGS_GROUP analyzer_event;

uint8_t shep_flags_init()
{
	tx_event_flags_create(&analyzer_event, "Analyzer Event");
}

static thread_t _state_machine_thread = {
	.name = "State Machine Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 4, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = 2, /* Sleep (in ticks) */
	.function = vStateMachine /* Thread Function */
};

void vStateMachine(ULONG thread_input)
{
	nertimer_t telem_timer;
	// sends unimportant telemetry messages every 500ms
	start_timer(&telem_timer, 500);

	for (;;) {
		sm_handle_state(bmsdata);

		if (is_timer_expired(&telem_timer)) {
			// these are unimportant telemetry messages so they can be sent infrequently
			send_bms_status_message(
				bmsdata->avg_temp, bmsdata->internal_temp,
				current_state,
				segment_is_balancing(bmsdata->chips));
			send_fault_status_message(bmsdata->fault_code_crit,
						  bmsdata->fault_code_noncrit);
			start_timer(&telem_timer, 500);
		}

		tx_thread_sleeplay(_state_machine_thread.sleep);
	}
}

static thread_t _can_receive_thread = {
	.name = "Can Receive Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 4, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = 2, /* Sleep (in ticks) */
	.function = vCanReceive /* Thread Function */
};

void vCanReceive(ULONG thred_input)
{
	can_msg_t message;

	for (;;) {
		/* Process incoming messages */
		while (queue_receive(&can_incoming, &message) == U_SUCCESS) {
			switch (msg.id) {
			case CHARGERBOX_CANID:
				charger_message_recieved(bmsdata);
				bmsdata->pack_current =
					parse_charger_current(message);
				break;
			case DTI_CURRENT_CANID:
				bmsdata->pack_current =
					parse_dti_current(message);
				break;
			default:
				break;
			}
		}

		tx_thread_sleep(_can_receive_thread.sleep);
	}
}

static thread_t _can_dispatch_thread = {
	.name = "CAN Dispatch Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 4, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = 2, /* Sleep (in ticks) */
	.function = vCanDispatch /* Thread Function */
};

extern can_t can1; // TODO: pass can1 directly into thread
void vCanDispatch(ULONG thread_input)
{
	can_msg_t message;
	uint8_t status;

	for (;;) {
		/* Process incoming messages */
		while (queue_receive(&can_outgoing, &message) == U_SUCCESS) {
			status = can_send_msg(&can1, &message);
			if (status != U_SUCCESS) {
				DEBUG_PRINTLN(
					"WARNING: Failed to send message (on can1) after removing from outgoing queue (Message ID: %ld).",
					message.id);
				// u_TODO - maybe add the message back into the queue if it fails to send? not sure if this is a good idea tho
			}
		}

		tx_thread_sleep(_can_dispatch_thread.sleep);
	}
}

static thread_t _analyzer_thread = {
	.name = "Analyzer Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 4, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = 2, /* Sleep (in ticks) */
	.function = vAnalyzer /* Thread Function */
};

void vAnalyzer(ULONG thread_input)
{
	acc_data_t *bmsdata = (acc_data_t *)pv_params;

	for (int i = 0; i < NUM_CHIPS; i++) {
		bmsdata->chip_data[i].alpha = i % 2 == 0;
	}

	for (;;) {
		//osThreadFlagsWait(ANALYZER_FLAG, osFlagsWaitAny, osWaitForever);

		mutex_get(bms_mutex);

		// calculate base values for later safety calcs
		calc_cell_temps(bmsdata);
		calc_pack_temps(bmsdata);
		calc_cell_voltages(bmsdata);
		calc_open_cell_voltage(bmsdata);
		calc_pack_voltage_stats(bmsdata);
		calc_cell_resistances(bmsdata);

		// these are dependent on above calculations
		calc_cont_dcl(bmsdata);
		calc_cont_ccl(bmsdata);
		calc_state_of_charge(bmsdata);

		// send out telemetry data sourced from the above functions
		send_acc_status_message(bmsdata->pack_ocv,
					bmsdata->pack_current, bmsdata->soc);
		send_cell_voltage_message(bmsdata->max_ocv, bmsdata->min_ocv,
					  bmsdata->avg_ocv);
		send_segment_average_volt_message(bmsdata);
		send_segment_total_volt_message(bmsdata);
		send_cell_temp_message(bmsdata->max_temp, bmsdata->min_temp,
				       bmsdata->avg_temp);
		send_segment_temp_message(bmsdata);

		mutex_put(&bms_mutex);
	}
}

static thread_t _segment_data_thread = {
	.name = "Segment Data Thread", /* Name */
	.size = 2048, /* Stack Size (in bytes) */
	.priority = 4, /* Priority */
	.threshold = 0, /* Preemption Threshold */
	.time_slice = TX_NO_TIME_SLICE, /* Time Slice */
	.auto_start = TX_AUTO_START, /* Auto Start */
	.sleep = 1000 / SAMPLE_RATE, /* Sleep (in ticks) */
	.function = vGetSegmentData, /* Thread Function */
};

void vGetSegmentData(ULONG thread_input)
{
	acc_data_t *bmsdata = args->bmsdata;
	assert(bmsdata);
	SPI_HandleTypeDef *hspi = args->hspi;
	assert(hspi);

	free(args);

	HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
	segment_init(bmsdata->chips, hspi);
	HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);

	// must delay after init for some reason, or else ADC doesnt start up (-3.45 or something)
	tx_thread_sleep(500);

	for (;;) {
		HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);

		segment_mute(bmsdata->chips, hspi);

		if (current_state == CHARGING_STATE) {
			tx_thread_sleep(75);
			// must delay to let settle after balancing has halted, or else cells read high
		}

		if (current_state == CHARGING_STATE) {
			// in charging, debug data is required to get things like die temp
			segment_retrieve_charging_data(bmsdata->chips, hspi);
		} else {
			// snap before getting data
			segment_snap(bmsdata->chips, hspi);
			segment_retrieve_active_data(bmsdata->chips, hspi);
			// unsnap after getting data
			segment_unsnap(bmsdata->chips, hspi);
			if (DEBUG_MODE_ENABLED) {
				segment_retrieve_debug_data(bmsdata->chips,
							    hspi);
			}
		}

		// if in normal drive mode, reboot the segment every 45 seconds in case the chips go out of sync
		// if (current_state == READY_STATE) {
		// 	if (++i % (45 * SAMPLE_RATE) == 0) {
		// 		printf(" ***********  REBOOTING SEGMENT\n\n");
		// 		segment_restart(bmsdata);
		// 	}
		// }

		// if (current_state_2 == CHARGING_STATE) {
		// 	//segment_disable_balancing(bmsdata);
		// 	if (current_state != FAULTED_STATE) {
		// 		segment_manual_balancing(bmsdata);
		// 		segment_enable_balancing(bmsdata);
		// 		//handle_balance_cells(bmsdata);
		// 	}
		// }

		if (current_state == CHARGING_STATE) {
			segment_unmute(bmsdata->chips, hspi);
		}

		if (bmsdata->should_balance) {
			segment_configure_balancing(bmsdata->chips,
						    bmsdata->discharge_config,
						    hspi);
		}

		HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);

		// osThreadFlagsSet(analyzer_thread, ANALYZER_FLAG); // TODO FIX FLAGS
		tx_thread_sleep(_segment_data_thread.sleep);
	}
}

uint8_t shep_threads_init(TX_BYTE_POOL *byte_pool)
{
	CATCH_ERROR(create_thread(byte_pool, &_state_machine_thread),
		    U_SUCCESS); // Create Default thread.
	CATCH_ERROR(create_thread(byte_pool, &_analyzer_thread),
		    U_SUCCESS); // Create Analyzer thread.
	CATCH_ERROR(create_thread(byte_pool, &_can_dispatch_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_can_receive_thread), U_SUCCESS);
	CATCH_ERROR(create_thread(byte_pool, &_segment_data_thread), U_SUCCESS);
}