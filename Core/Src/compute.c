
#include "compute.h"

#include "compute.h"

#include <assert.h>

#include "datastructs.h"
#include "main.h"
#include <sht30.h>  

// TODO: Fix pinout defines

extern I2C_HandleTypeDef hi2c1;

sht30_t temp_sensor;

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

// blocking read uses master transmit (in blocking mode)
static inline uint8_t sht30_i2c_blocking_read(uint8_t *data, uint16_t command,
					      uint8_t dev_address,
					      uint8_t length)
{
	uint8_t command_buffer[2] = { (command & 0xff00u) >> 8u,
				      command & 0xffu };
	// write command to sht30 before reading
	sht30_i2c_write(command_buffer, dev_address, sizeof(command_buffer));
	HAL_Delay(1); // 1 ms11 delay to ensure sht30 returns to idle state
	return HAL_I2C_Master_Receive(&hi2c1, dev_address, data, length,
				      HAL_MAX_DELAY);
}

void compute_init()
{
	assert(!sht30_init(&temp_sensor, (Write_ptr)sht30_i2c_write,
			   (Read_ptr)sht30_i2c_read,
			   (Read_ptr)sht30_i2c_blocking_read,
			   (SHT30_I2C_ADDR)));
}

int8_t compute_measure_temp(float *temp, float *humidity)
{
	uint8_t status = sht30_get_temp_humid(&temp_sensor);
	if (status)
		return status;

	*temp = temp_sensor.temp;
	*humidity = temp_sensor.humidity;

	return 0;
}

void compute_set_fault(bool fault_state)
{
	//HAL_GPIO_WritePin(FAULT_MCU_GPIO_Port, FAULT_MCU_Pin, fault_state);
}

void toggle_debug_led_1()
{
	//HAL_GPIO_TogglePin(DEBUG_LED_1_GPIO_Port, DEBUG_LED_2_Pin);
}

void set_debug_led_2(int mode)
{
	//HAL_GPIO_WritePin(DEBUG_LED_2_GPIO_Port, DEBUG_LED_2_Pin, mode);
}


void pet_watchdog()
{
	//HAL_GPIO_WritePin(WATCHDOG_GPIO_Port, WATCHDOG_Pin, true);
	//HAL_GPIO_WritePin(WATCHDOG_GPIO_Port, WATCHDOG_Pin, false);
}

bool read_shutdown()
{
	// If the pin is high, the shutdown circuit is closed. So, return false.
	// If the pin is low, the shutdown circuit is open. So, return true.
	// return !HAL_GPIO_ReadPin(SHUTDOWN_GPIO_Port, SHUTDOWN_Pin);
}