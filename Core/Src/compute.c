#include "compute.h"
#include <assert.h>
#include <stdlib.h>
#include "datastructs.h"
#include "main.h"
#include <sht30.h>

typedef struct {
	sht30_t sht30;
} compute_t;

compute_t *compute;
extern I2C_HandleTypeDef hi2c1;

// moving the bulk of the pointers over
static inline uint8_t sht30_i2c_write(uint8_t *data, uint8_t dev_address,
	
	uint8_t length)
{
	return HAL_I2C_Master_Transmit(&hi2c1, dev_address, data, length,
				       HAL_MAX_DELAY);
}
static inline uint8_t sht30_i2c_read(uint8_t *data, uint16_t command,
				     uint8_t dev_address, uint8_t length)
{
	return HAL_I2C_Mem_Read(&hi2c1, dev_address, command, sizeof(command),
				data, length, HAL_MAX_DELAY);
}
static inline uint8_t sht30_i2c_blocking_read(uint8_t *data, uint16_t command,
					      uint8_t dev_address,
					      uint8_t length)
{
	uint8_t command_buffer[2] = { (command & 0xff00u) >> 8u,
				      command & 0xffu };
	sht30_i2c_write(command_buffer, dev_address, sizeof(command_buffer));
	HAL_Delay(1);
	return HAL_I2C_Master_Receive(&hi2c1, dev_address, data, length,
				      HAL_MAX_DELAY);
}

void init_compute(compute_t *compute_ptr)
{
	assert(compute_ptr);
	assert(!sht30_init(&compute_ptr->sht30, (Write_ptr)sht30_i2c_write,
			   (Read_ptr)sht30_i2c_read,
			   (Read_ptr)sht30_i2c_blocking_read, SHT30_I2C_ADDR));
	compute = compute_ptr;
}

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
		HAL_GPIO_ReadPin(TS_MINUS_SENSE_GPIO_Port,
				 TS_MINUS_SENSE_Pin) &&
		HAL_GPIO_ReadPin(TS_PLUS_SENSE_GPIO_Port, TS_PLUS_SENSE_Pin) &&
		HAL_GPIO_ReadPin(ACC_SENSE_GPIO_Port, ACC_SENSE_Pin) &&
		HAL_GPIO_ReadPin(TSIP_SENSE_GPIO_Port, TSIP_SENSE_Pin);

	// If the pin is high, the shutdown circuit is closed. So, return false.
	// If the pin is low, the shutdown circuit is open. So, return true.
	return !shutdown;
}