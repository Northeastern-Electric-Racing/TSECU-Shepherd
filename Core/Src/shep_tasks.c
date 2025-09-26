
#include "u_threads.h"
#include "u_debug.h"
#include "u_general.h"
#include "u_can.h"
#include "shep_queues.h"
#include "can_messages.h"
<<<<<<< HEAD
#include "shep_mutexes.h"
#include "shep_tasks.h"
#include "timer.h"

// TODO: Fill in threads

TX_EVENT_FLAGS_GROUP analyzer_event;

uint8_t shep_flags_init() {
    tx_event_flags_create(&analyzer_event, "Analyzer Event");
}

extern bms_t bms;

=======

// TODO: Fill in threads

>>>>>>> 61f77d2 (can setup)
static thread_t _state_machine_thread = {
        .name       = "State Machine Thread", /* Name */
        .size       = 2048,             /* Stack Size (in bytes) */
        .priority   = 4,               /* Priority */
        .threshold  = 0,               /* Preemption Threshold */
        .time_slice = TX_NO_TIME_SLICE, /* Time Slice */
        .auto_start = TX_AUTO_START,    /* Auto Start */
        .sleep      = 2,                /* Sleep (in ticks) */
        .function   = vStateMachine    /* Thread Function */
    };

void vStateMachine(ULONG thread_input) {

    DEBUG_PRINTLN("Starting State Machine thread...");
    
    nertimer_t telem_timer;
	// sends unimportant telemetry messages every 500ms
	start_timer(&telem_timer, 500);

	for (;;) {
		sm_handle_state(bms);

		if (is_timer_expired(&telem_timer)) {
			// these are unimportant telemetry messages so they can be sent infrequently
			send_bms_status_message(
				bms.avg_temp, bms.internal_temp,
				bms.current_state,
				segment_is_balancing(bms.chips));
			send_fault_status_message(bms.fault_code_crit,
						  bms.fault_code_noncrit);
			start_timer(&telem_timer, 500);
		}

		osDelay(100);
	}
}

static thread_t _can_receive_thread = {
        .name       = "Can Receive Thread", /* Name */
        .size       = 2048,             /* Stack Size (in bytes) */
        .priority   = 4,               /* Priority */
        .threshold  = 0,               /* Preemption Threshold */
        .time_slice = TX_NO_TIME_SLICE, /* Time Slice */
        .auto_start = TX_AUTO_START,    /* Auto Start */
        .sleep      = 2,                /* Sleep (in ticks) */
        .function   = vCanReceive    /* Thread Function */
    };

void vCanReceive(ULONG thred_input)
{
	can_msg_t message;

	for (;;) {
    /* Process incoming messages */
        while(queue_receive(&can_incoming, &message) == U_SUCCESS) {
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
        
        tx_thread_sleep(_can_receive_thread.sleep);
	}
}


static thread_t _can_dispatch_thread = {
        .name       = "CAN Dispatch Thread", /* Name */
        .size       = 2048,             /* Stack Size (in bytes) */
        .priority   = 4,               /* Priority */
        .threshold  = 0,               /* Preemption Threshold */
        .time_slice = TX_NO_TIME_SLICE, /* Time Slice */
        .auto_start = TX_AUTO_START,    /* Auto Start */
        .sleep      = 2,                /* Sleep (in ticks) */
        .function   = vCanDispatch    /* Thread Function */
    };

<<<<<<< HEAD
extern can_t can1; // TODO: pass can1 directly into thread
=======
extern can_t can1; // TODO pass can1 directly into thread
>>>>>>> 61f77d2 (can setup)
void vCanDispatch(ULONG thread_input) {

    can_msg_t message;
    uint8_t status;

    for (;;) {
    /* Process incoming messages */
        while(queue_receive(&can_outgoing, &message) == U_SUCCESS) {
            status = can_send_msg(&can1, &message);
            if(status != U_SUCCESS) {
                DEBUG_PRINTLN("WARNING: Failed to send message (on can1) after removing from outgoing queue (Message ID: %ld).", message.id);
                // u_TODO - maybe add the message back into the queue if it fails to send? not sure if this is a good idea tho
            }
        }	
        
        tx_thread_sleep(_can_dispatch_thread.sleep);
	} 
}

static thread_t _analyzer_thread = {
        .name       = "Analyzer Thread", /* Name */
        .size       = 2048,             /* Stack Size (in bytes) */
        .priority   = 4,               /* Priority */
        .threshold  = 0,               /* Preemption Threshold */
        .time_slice = TX_NO_TIME_SLICE, /* Time Slice */
        .auto_start = TX_AUTO_START,    /* Auto Start */
        .sleep      = 2,                /* Sleep (in ticks) */
        .function   = vAnalyzer    /* Thread Function */
    };


void vAnalyzer(ULONG thread_input)
{
<<<<<<< HEAD
    for (int i = 0; i < NUM_CHIPS; i++) {
		bms.chip_data[i].alpha = i % 2 == 0;
	}

	for (;;) {

        ULONG recevied_flags;
        tx_event_flags_get(&analyzer_event, ANALYZER_FLAG, TX_OR_CLEAR, &recevied_flags, TX_WAIT_FOREVER);

        mutex_get(&bms_mutex);

		// calculate base values for later safety calcs
		calc_cell_temps(bms);
		calc_pack_temps(bms);
		calc_cell_voltages(bms);
		calc_open_cell_voltage(bms);
		calc_pack_voltage_stats(bms);
		calc_cell_resistances(bms);

		// these are dependent on above calculations
		calc_cont_dcl(bms);
		calc_cont_ccl(bms);
		calc_state_of_charge(bms);

		// send out telemetry data sourced from the above functions
		send_acc_status_message(bms.pack_ocv,
					bms.pack_current, bms.soc);
		send_cell_voltage_message(bms.max_ocv, bms.min_ocv,
					  bms.avg_ocv);
		send_segment_average_volt_message(&bms);
		send_segment_total_volt_message(&bms);
		send_cell_temp_message(bms.max_temp, bms.min_temp,
				       bms.avg_temp);
		send_segment_temp_message(&bms);

		mutex_put(&bms_mutex);
	}
=======

>>>>>>> 61f77d2 (can setup)
}

static thread_t _segment_data_thread = {
        .name       = "Segment Data Thread", /* Name */
        .size       = 2048,             /* Stack Size (in bytes) */
        .priority   = 4,               /* Priority */
        .threshold  = 0,               /* Preemption Threshold */
        .time_slice = TX_NO_TIME_SLICE, /* Time Slice */
        .auto_start = TX_AUTO_START,    /* Auto Start */
        .sleep      = 2,                /* Sleep (in ticks) */
        .function   = vGetSegmentData,    /* Thread Function */
    };

void vGetSegmentData(ULONG thread_input)
{
	// frequency (Hz) at which each unique reading is updated (CHANGE THIS).
	// in reality this is far from exactly because osDelays yield, but it should be a good enough relative value
	static const float REFRESH_RATE = 1;
	// the number of ms each chip should be sent in (DONT CHANGE)
	const uint16_t CHIP_TIME = ((1 / REFRESH_RATE) * 1000) / NUM_CHIPS;

	for (;;) {
		for (uint8_t chip = 0; chip < NUM_CHIPS; chip++) {
			uint8_t num_cells =
				get_num_cells(&bms.chip_data[chip]);
			// dont send the 11th cell of beta as it goes in a beta stat msg
			if (!bms.chip_data[chip].alpha) {
				num_cells -= 1;
			}
			for (int cell = 0; cell < num_cells; cell += 2) {
				send_cell_data_message(
					bms.chip_data[chip].alpha,

					bms.chip_data[chip].cell_temp[cell],

					bms.chip_data[chip]
						.cell_voltages[cell],

					bms.chip_data[chip]
						.cell_voltages[cell + 1],

					chip,

					cell,

					cell + 1,

					(bms.chips[chip].tx_cfgb.dcc >>
					 cell) & 1,

					(bms.chips[chip].tx_cfgb.dcc >>
					 (cell + 1)) &
						1,
					(bms.chips[chip].statc.cs_flt >>
					 cell) & 1,
					(bms.chips[chip].statc.cs_flt >>
					 (cell + 1)) &
						1);
				// wait for a fraction of the chip time alotted between each cell
				// the fraction is determined manually by the fact that there are 2 or 3 status messages and 24 cell messages
				osDelay((CHIP_TIME * 0.85) /
					(NUM_CELLS_PER_CHIP * 2 -
					 1)); // 4ms for 24A segment
			}

			// Send chip status messages
			if (!bms.chip_data[chip].alpha) {
				send_beta_status_a_message(
					bms.chip_data[chip].cell_temp[10],
					bms.chip_data[chip]
						.cell_voltages[10],
					NER_GET_BIT(
						bms.chips[chip].tx_cfgb.dcc,
						10),
					chip,

					bms.chip_data[chip].on_board_temp,

					(getVoltage(bms.chips[chip]
							    .stata.itmp) /
					 0.0075) -
						273,
					20.0 * getVoltage( // VPV is ra_code 11 w/ different scale
						       bms.chips[chip]
							       .aux
							       .a_codes[11]));

				send_beta_status_b_message(
					getVoltage(bms.chips[chip]
							   .stata.vref2),
					getVoltage(
						bms.chips[chip].statb.va),
					getVoltage(
						bms.chips[chip].statb.vd),
					chip,
					getVoltage(
						bms.chips[chip].statb.vr4k),
					20.0 * getVoltage( // VMV is ra_code 10
						       bms.chips[chip]
							       .aux.a_codes[10]),
					(bms.chips[chip].statc.cs_flt >>
					 10) & 1);
				send_beta_status_c_message(
					chip, &bms.chips[chip].statc);
			} else {
				send_alpha_status_a_message(
					bms.chip_data->on_board_temp, chip,
					((getVoltage(bms.chips[chip]
							     .stata.itmp) /
					  0.0075) -
					 273),
					(20.0 *
					 getVoltage( // VPV is ra_code 11 w/ different scale
						 bms.chips[chip]
							 .aux.a_codes[11])),
					(20.0 *
					 getVoltage( // VMV is ra_code 10
						 bms.chips[chip]
							 .aux.a_codes[10])),
					&bms.chips[chip].statc);
				send_alpha_status_b_message(
					getVoltage(
						bms.chips[chip].statb.vr4k),
					chip,
					getVoltage(bms.chips[chip]
							   .stata.vref2),
					getVoltage(
						bms.chips[chip].statb.va),
					getVoltage(
						bms.chips[chip].statb.vd),
					&bms.chips[chip].statc);
			}
			// wait for the remaining time
			osDelay(0.15 * CHIP_TIME);
		}
    }
}

uint8_t shep_threads_init(TX_BYTE_POOL *byte_pool) {
    CATCH_ERROR(create_thread(byte_pool, &_state_machine_thread), U_SUCCESS); // Create Default thread.
    CATCH_ERROR(create_thread(byte_pool, &_analyzer_thread), U_SUCCESS); // Create Analyzer thread.
    CATCH_ERROR(create_thread(byte_pool, &_can_dispatch_thread), U_SUCCESS);
    CATCH_ERROR(create_thread(byte_pool, &_can_receive_thread), U_SUCCESS);
    CATCH_ERROR(create_thread(byte_pool, &_segment_data_thread), U_SUCCESS);

}