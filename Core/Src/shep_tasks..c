
#include "u_threads.h"
#include "u_debug.h"
#include "u_general.h"

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

uint8_t shep_threads_init(TX_BYTE_POOL *byte_pool) {
    CATCH_ERROR(_create_thread(byte_pool, &_state_machine_thread), U_SUCCESS);            // Create Default thread.
}