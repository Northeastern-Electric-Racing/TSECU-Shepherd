
#include "u_threads.h"
#include "u_debug.h"
#include "u_general.h"
#include "u_can.h"
#include "shep_queues.h"
#include "can_messages.h"

// TODO: Fill in threads

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

extern can_t can1; // TODO: pass can1 directly into thread
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