#include "compute.h"
#include <assert.h>
#include <stdlib.h>
#include "datastructs.h"
#include "main.h"
#include "sht30.h"
#include "lsm6dsv_reg.h"

#define IMU_CS_GPIO_Port SPI6_CS_GPIO_Port
#define IMU_CS_Pin	 SPI6_CS_Pin

/* Wrapper for lsm6dsv SPI reading. */
static int32_t _lsm6dsv_read(void *spi_handle, uint8_t reg, uint8_t *buffer,
			     uint16_t length)
{
	SPI_HandleTypeDef *handle = (SPI_HandleTypeDef *)spi_handle;
	HAL_StatusTypeDef status;

	/* Select the IMU by setting its CS pin LOW. */
	HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_RESET);

	/* Tell the IMU you want to read from 'reg'. */
	uint8_t spi_reg =
		(uint8_t)(reg |
			  0b10000000); // Bits 0 through 6 store 'reg' (the register address), while Bit 7 lets you chose if it's a read or write operation (1=read, 0=write).
	status = HAL_SPI_Transmit(handle, &spi_reg, sizeof(spi_reg),
				  HAL_MAX_DELAY);
	if (status != HAL_OK) {
		PRINTLN_ERROR(
			"Failed to call HAL_SPI_Transmit() to write the first SPI command (Status: %d/%s).",
			status, hal_status_toString(status));
		HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin,
				  GPIO_PIN_SET); // Deselect IMU since error.
		return -1;
	}

	/* Read from 'reg'. */
	status = HAL_SPI_Receive(handle, buffer, length, HAL_MAX_DELAY);
	if (status != HAL_OK) {
		PRINTLN_ERROR(
			"Failed to call HAL_SPI_Receive() to read from 'reg' (Status: %d/%s).",
			status, hal_status_toString(status));
		HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin,
				  GPIO_PIN_SET); // Deselect IMU since error.
		return -1;
	}

	/* Deselect the IMU by setting its CS pin HIGH. */
	HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_SET);

	return 0;
}

/* Wrapper for lsm6dsv SPI writing. */
static int32_t _lsm6dsv_write(void *spi_handle, uint8_t reg,
			      const uint8_t *data, uint16_t length)
{
	SPI_HandleTypeDef *handle = (SPI_HandleTypeDef *)spi_handle;
	HAL_StatusTypeDef status;

	/* Select the IMU by setting its CS pin LOW. */
	HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_RESET);

	/* Tell the IMU you want to write to 'reg'. */
	uint8_t spi_reg =
		(uint8_t)(reg &
			  0b01111111); // Bits 0 through 6 store 'reg' (the register address), while Bit 7 lets you chose if it's a read or write operation (1=read, 0=write).
	status = HAL_SPI_Transmit(handle, &spi_reg, sizeof(spi_reg),
				  HAL_MAX_DELAY);
	if (status != HAL_OK) {
		PRINTLN_ERROR(
			"Failed to call HAL_SPI_Transmit() to write the first SPI command (Status: %d/%s).",
			status, hal_status_toString(status));
		HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin,
				  GPIO_PIN_SET); // Deselect IMU since error.
		return -1;
	}

	/* Write to 'reg'. */
	status = HAL_SPI_Transmit(handle, data, length, HAL_MAX_DELAY);
	if (status != HAL_OK) {
		PRINTLN_ERROR(
			"Failed to call HAL_SPI_Transmit() to write to 'reg' (Status: %d/%s).",
			status, hal_status_toString(status));
		HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin,
				  GPIO_PIN_SET); // Deselect IMU since error.
		return -1;
	}

	/* Deselect the IMU by setting its CS pin HIGH. */
	HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_SET);

	return 0;
}

static const stmdev_ctx_t imu = { .handle = &hspi2,
				  .read_reg = _lsm6dsv_read,
				  .write_reg = _lsm6dsv_write };

int imu_init(void)
{
	HAL_StatusTypeDef status;

	/* Make sure IMU is set up correctly. */
	uint8_t id;
	printf("before lsm6dsv_device_id_get()\n");
	status = lsm6dsv_device_id_get(&imu, &id);
	if (status != 0) {
		PRINTLN_ERROR(
			"Failed to call lsm6dsv_device_id_get() (Status: %d).",
			status);
		return U_ERROR;
	}
	if (id != LSM6DSV_ID) {
		PRINTLN_ERROR(
			"lsm6dsv_device_id_get() returned an unexpected ID (id=%d, expected=%d). This means that the IMU is not configured correctly.",
			id, LSM6DSV_ID);
		return U_ERROR;
	}
	printf("after lsm6dsv_device_id_get()\n");

	/* Reset IMU. */
	printf("before lsm6dsv_reset_set()\n");
	status = lsm6dsv_reset_set(&imu, LSM6DSV_GLOBAL_RST);
	if (status != 0) {
		PRINTLN_ERROR(
			"Failed to reset the IMU via lsm6dsv_reset_set() (Status: %d).",
			status);
		return U_ERROR;
	}
	printf("after lsm6dsv_reset_set()\n");

	printf("before HAL_DELAY()\n");
	//HAL_Delay(30); // This is probably overkill, but the datasheet lists the gyroscope's "Turn-on time" as 30ms, and I can't find anything else that specifies how long resets take.
	tx_thread_sleep(30);
	printf("after HAL_DELAY()\n");

	/* Enable Block Data Update. */
	status = lsm6dsv_block_data_update_set(
		&imu,
		PROPERTY_ENABLE); // Makes it so "output registers are not updated until LSB and MSB have been read". Datasheet says this is enabled by default but figured it was better to be explicit.
	if (status != 0) {
		PRINTLN_ERROR(
			"Failed to enable Block Data Update via lsm6dsv_block_data_update_set() (Status: %d).",
			status);
		return U_ERROR;
	}

	/* Set Accelerometer Full Scale. */
	status = lsm6dsv_xl_full_scale_set(&imu, LSM6DSV_2g);
	if (status != 0) {
		PRINTLN_ERROR(
			"Failed to set IMU Accelerometer Full Scale via lsm6dsv_xl_full_scale_set() (Status: %d).",
			status);
		return U_ERROR;
	}

	/* Set gyroscope full scale. */
	status = lsm6dsv_gy_full_scale_set(&imu, LSM6DSV_2000dps);
	if (status != 0) {
		PRINTLN_ERROR(
			"Failed to set IMU Gyroscope Full Scale via lsm6dsv_gy_full_scale_set() (Status: %d).",
			status);
		return U_ERROR;
	}

	/* Set accelerometer output data rate. */
	status = lsm6dsv_xl_data_rate_set(&imu, LSM6DSV_ODR_AT_120Hz);
	if (status != 0) {
		PRINTLN_ERROR(
			"Failed to set IMU Accelerometer Datarate via lsm6dsv_xl_data_rate_set() (Status: %d).",
			status);
		return U_ERROR;
	}

	/* Set gyroscope output data rate. */
	status = lsm6dsv_gy_data_rate_set(&imu, LSM6DSV_ODR_AT_120Hz);
	if (status != 0) {
		PRINTLN_ERROR(
			"Failed to set IMU Gyroscope Datarate via lsm6dsv_gy_data_rate_set() (Status: %d).",
			status);
		return U_ERROR;
	}

	PRINTLN_INFO("Ran imu_init().");
	return U_SUCCESS;
}

/* Gets the IMU's accerlation reading. */
int imu_getAcceleration(vector3_t *data)
{
	int16_t raw_data[3];

	/* Read raw accelerometer data. */
	int status = lsm6dsv_acceleration_raw_get(&imu, raw_data);
	if (status != 0) {
		PRINTLN_ERROR("Failed to read accelerometer data.");
		return U_ERROR;
	}

	/* Convert to mg (milligravity). */
	data->x = lsm6dsv_from_fs2_to_mg(
		raw_data[0]); // Somewhat important: These functions MUST match the full-scale settings configured in peripherals_init(). The conversions will be incorrect if you use the wrong functions.
	data->y = lsm6dsv_from_fs2_to_mg(raw_data[1]);
	data->z = lsm6dsv_from_fs2_to_mg(raw_data[2]);

	return U_SUCCESS;
}

/* Gets the IMU's angular rate reading. */
int imu_getAngularRate(vector3_t *data)
{
	int16_t raw_data[3];

	/* Read raw gyroscope data. */
	int status = lsm6dsv_angular_rate_raw_get(&imu, raw_data);
	if (status != 0) {
		PRINTLN_ERROR("Failed to read gyroscope data.");
		return U_ERROR;
	}

	/* Convert to mdps (millidegrees per second). */
	data->x = lsm6dsv_from_fs2000_to_mdps(
		raw_data[0]); // Somewhat important: These functions MUST match the full-scale settings configured in peripherals_init(). The conversions will be incorrect if you use the wrong functions.
	data->y = lsm6dsv_from_fs2000_to_mdps(raw_data[1]);
	data->z = lsm6dsv_from_fs2000_to_mdps(raw_data[2]);

	return U_SUCCESS;
}

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

void init_compute(peripherals_t *peripherals)
{
	assert(peripherals);
	assert(!imu_init());
	assert(!sht30_init(&peripherals->sht30, (Write_ptr)sht30_i2c_write,
			   (Read_ptr)sht30_i2c_read,
			   (Read_ptr)sht30_i2c_blocking_read, SHT30_I2C_ADDR));
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

int tempsensor_getTemperatureAndHumdidty(peripherals_t *peripherals,
					float *temperature,
					float *humidity)
{
    CATCH_ERROR(mutex_get(&peripherals->peripherals_mutex), U_SUCCESS);
    int status = sht30_get_temp_humid(&peripherals->sht30);
    CATCH_ERROR(mutex_put(&peripherals->peripherals_mutex), U_SUCCESS);
    if (status != 0) {
        PRINTLN_ERROR("Failed to read SHT30 temperature/humidity (Status: %d).", status);
        return U_ERROR;
    }

    *temperature = peripherals->sht30.temp;
    *humidity = peripherals->sht30.humidity;
    return U_SUCCESS;
}