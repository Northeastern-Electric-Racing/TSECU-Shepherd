
#include "u_threads.h"
#include "u_debug.h"
#include "u_general.h"
#include "u_can.h"
#include "shep_queues.h"
#include "can_messages.h"
<<<<<<< HEAD
#include "shep_mutexes.h"
#include "shep_tasks.h"

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
    
    for (;;) {
        
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
        tx_event_flags_set(&analyzer_event, ANALYZER_FLAG, TX_OR);

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
    
}

uint8_t shep_threads_init(TX_BYTE_POOL *byte_pool) {
    CATCH_ERROR(_create_thread(byte_pool, &_state_machine_thread), U_SUCCESS);            // Create Default thread.
}