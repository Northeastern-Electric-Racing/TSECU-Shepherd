#include <stdio.h>
#include "shep_mutexes.h"
#include "u_tx_debug.h"

mutex_t analyzer_mutex = {
	.name = "Analyzer Mutex", /* Name of the mutex. */
	.priority_inherit = TX_INHERIT /* Priority inheritance setting. */
};

mutex_t state_mutex = {
	.name = "BMS State Mutex", /* Name of the mutex. */
	.priority_inherit = TX_INHERIT /* Priority inheritance setting. */
};

mutex_t bms_algos_mutex = {
	.name = "BMS Algos Mutex", /* Name of the mutex. */
	.priority_inherit = TX_INHERIT /* Priority inheritance setting. */
};

mutex_t peripherals_mutex = {
	.name = "Peripherals Mutex", /* Name of the mutex. */
	.priority_inherit = TX_INHERIT /* Priority inheritance setting. */
};

mutex_t shutdown_mutex = {
	.name = "Shutdown Mutex", /* Name of the mutex. */
	.priority_inherit = TX_INHERIT /* Priority inheritance setting. */
};

/* Initializes all ThreadX mutexes. 
*  Calls to _create_mutex() should go in here
*/
uint8_t mutexes_init()
{
	/* Create Mutexes. */
	CATCH_ERROR(create_mutex(&analyzer_mutex), U_SUCCESS);
	CATCH_ERROR(create_mutex(&state_mutex), U_SUCCESS);
	CATCH_ERROR(create_mutex(&bms_algos_mutex), U_SUCCESS);
	CATCH_ERROR(create_mutex(&peripherals_mutex), U_SUCCESS);
	CATCH_ERROR(create_mutex(&shutdown_mutex), U_SUCCESS);

	// add more as necessary.

	PRINTLN_INFO("Ran mutexes_init().");
	return U_SUCCESS;
}