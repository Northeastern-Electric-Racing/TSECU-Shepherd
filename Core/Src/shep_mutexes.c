#include "cell_data_logging.h"s
#include "u_debug.h"

mutex_t logger_mutex;

/* Initializes all ThreadX mutexes. */
uint8_t mutexes_init() {
    /* Create Mutexes. */
    CATCH_ERROR(_create_mutex(&logger_mutex), U_SUCCESS); // Create Logger Mutex.

    DEBUG_PRINTLN("Ran mutexes_init().");
    return U_SUCCESS;
}
