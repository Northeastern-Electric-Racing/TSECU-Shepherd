
#include "compute.h"

#include "compute.h"

#include <assert.h>

#include "datastructs.h"
#include "main.h"
#include <sht30.h>

// TODO: Fix pinout defines

void compute_set_fault(bool fault_state)
{
	HAL_GPIO_WritePin(FAULT_MCU_GPIO_Port, FAULT_MCU_Pin, fault_state);
}

bool read_shutdown()
{
	// If the pin is high, the shutdown circuit is closed. So, return false.
	// If the pin is low, the shutdown circuit is open. So, return true.
	// return !HAL_GPIO_ReadPin(SHUTDOWN_GPIO_Port, SHUTDOWN_Pin);
	return false;
}