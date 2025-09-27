#include "cell_data_logging.h"s
#include "u_debug.h"

/* Logger Mutex */
mutex_t logger_mutex = {
    .name = "Logger Mutex",
    .priority_inherit = TX_INHERIT
};

/* BMS Mutex */
mutex_t bms_mutex = {
    .name = "BMS Mutex",           
    .priority_inherit = TX_INHERIT 
};

/* Initializes all ThreadX mutexes. */
uint8_t mutexes_init() {
    /* Create Mutexes. */
    CATCH_ERROR(_create_mutex(&logger_mutex), U_SUCCESS); // Create Logger Mutex.
    CATCH_ERROR(_create_mutex(&bms_mutex), U_SUCCESS); // Create BMS Mutex.

    DEBUG_PRINTLN("Ran mutexes_init().");
    return U_SUCCESS;
}
