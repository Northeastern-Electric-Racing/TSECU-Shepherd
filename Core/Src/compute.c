
#include "compute.h"

#include <assert.h>

#include "datastructs.h"
#include "main.h"
#include <sht30.h>

void compute_set_fault(bool fault_state)
{
	if (fault_state) {
		HAL_GPIO_WritePin(FAULT_MCU_GPIO_Port, FAULT_MCU_Pin,
				  GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(FAULT_MCU_GPIO_Port, FAULT_MCU_Pin,
				  GPIO_PIN_RESET);
	}
}

bool read_shutdown()
{
	// Read shutdown sense using TS_MINUS_SENSE pin
	bool shutdown =
		HAL_GPIO_ReadPin(TS_MINUS_SENSE_GPIO_Port, TS_MINUS_SENSE_Pin) && 
    HAL_GPIO_ReadPin(TS_PLUS_SENSE_GPIO_Port, TS_PLUS_SENSE_Pin) &&
    HAL_GPIO_ReadPin(ACC_SENSE_GPIO_Port, ACC_SENSE_Pin) &&
    HAL_GPIO_ReadPin(TSIP_SENSE_GPIO_Port, TSIP_SENSE_Pin);

	// If the pin is high, the shutdown circuit is closed. So, return false.
	// If the pin is low, the shutdown circuit is open. So, return true.
	return !shutdown;
}