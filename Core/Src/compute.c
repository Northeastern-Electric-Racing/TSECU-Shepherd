#include "compute.h"

#include "app_threadx.h"
#include "datastructs.h"
#include "lsm6dsv_reg.h"
#include "main.h"
#include "p3t1755.h"
#include "shep_mutexes.h"
#include "sht30.h"
#include "stm32h5xx_hal_def.h"
#include "stm32h5xx_hal_i2c.h"
#include "can_messages_tx.h"
#include "timer.h"
#include "u_tx_debug.h"
#include "debounce.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define IMU_CS_GPIO_Port SPI6_CS_GPIO_Port
#define IMU_CS_Pin	 SPI6_CS_Pin

extern I2C_HandleTypeDef hi2c1;

// NOTE: that this is a blocking call
static int32_t _p3t1755_read(uint16_t dev_addr, uint16_t reg, uint8_t *data,
			     uint8_t length)
{
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, dev_addr, reg,
						    sizeof(reg), data, length,
						    HAL_MAX_DELAY);

	if (status != HAL_OK) {
		PRINTLN_ERROR(
			"Failed to call HAL_I2C_Master_Receive() to read from P3T1755 (Status: "
			"%d/%s).",
			status, hal_status_toString(status));
		return status;
	}
	return HAL_OK;
}

// NOTE:  this is a blocking call
static int32_t _p3t1755_write(uint16_t dev_addr, uint16_t reg, uint8_t *data,
			      uint8_t length)
{
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1, dev_addr, reg,
						    sizeof(reg), data, length,
						    HAL_MAX_DELAY);
	if (status != HAL_OK) {
		PRINTLN_ERROR(
			"Failed to call HAL_I2C_Mem_Write() to write to "
			"P3T1755 (Status: %d/%s).",
			status, hal_status_toString(status));
		return status;
	}
	return HAL_OK;
}

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
			  0b10000000); // Bits 0 through 6 store 'reg' (the register
	// address), while Bit 7 lets you chose if it's a
	// read or write operation (1=read, 0=write).
	status = HAL_SPI_Transmit(handle, &spi_reg, sizeof(spi_reg),
				  HAL_MAX_DELAY);
	if (status != HAL_OK) {
		PRINTLN_ERROR(
			"Failed to call HAL_SPI_Transmit() to write the first SPI "
			"command (Status: %d/%s).",
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
			  0b01111111); // Bits 0 through 6 store 'reg' (the register
	// address), while Bit 7 lets you chose if it's a
	// read or write operation (1=read, 0=write).
	status = HAL_SPI_Transmit(handle, &spi_reg, sizeof(spi_reg),
				  HAL_MAX_DELAY);
	if (status != HAL_OK) {
		PRINTLN_ERROR(
			"Failed to call HAL_SPI_Transmit() to write the first SPI "
			"command (Status: %d/%s).",
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

static p3t1755_t p3t = { P3T1755_DEV_ADDR, _p3t1755_write, _p3t1755_read };
int p3t_init(void)
{
	p3t1755_init(&p3t, p3t.write, p3t.read, P3T1755_DEV_ADDR);
	int status = p3t1755_configure(&p3t, 0, 0, 0,
				       p3t1755_2_CONSECUTIVE_FAULTS,
				       p3t1755_27_5MS_CONVERSION_TIME);
	if (status != 0) {
		PRINTLN_ERROR(
			"Failed to configure P3T1755 via p3t1755_configure() (Status: %d).",
			status);
		return U_ERROR;
	}

	return U_SUCCESS;
}

int p3t1755_getBoardTemp(float *temp_c)
{
	int status = p3t1755_read_temperature(&p3t, temp_c);
	PRINTLN_INFO("Read board temp: %f", *temp_c);
	return status;
}

static const stmdev_ctx_t imu = { .handle = &hspi6,
				  .read_reg = _lsm6dsv_read,
				  .write_reg = _lsm6dsv_write };

static bool imu_available = false;
static bool p3t_available = false;

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
			"lsm6dsv_device_id_get() returned an unexpected ID (id=%d, "
			"expected=%d). This means that the IMU is not configured correctly.",
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
	// HAL_Delay(30); // This is probably overkill, but the datasheet lists the
	// gyroscope's "Turn-on time" as 30ms, and I can't find anything else that
	// specifies how long resets take.
	tx_thread_sleep(30);
	printf("after HAL_DELAY()\n");

	/* Enable Block Data Update. */
	status = lsm6dsv_block_data_update_set(
		&imu,
		PROPERTY_ENABLE); // Makes it so "output registers are not updated until
	// LSB and MSB have been read". Datasheet says this is
	// enabled by default but figured it was better to be
	// explicit.
	if (status != 0) {
		PRINTLN_ERROR("Failed to enable Block Data Update via "
			      "lsm6dsv_block_data_update_set() (Status: %d).",
			      status);
		return U_ERROR;
	}

	/* Set Accelerometer Full Scale. */
	status = lsm6dsv_xl_full_scale_set(&imu, LSM6DSV_2g);
	if (status != 0) {
		PRINTLN_ERROR("Failed to set IMU Accelerometer Full Scale via "
			      "lsm6dsv_xl_full_scale_set() (Status: %d).",
			      status);
		return U_ERROR;
	}

	/* Set gyroscope full scale. */
	status = lsm6dsv_gy_full_scale_set(&imu, LSM6DSV_2000dps);
	if (status != 0) {
		PRINTLN_ERROR("Failed to set IMU Gyroscope Full Scale via "
			      "lsm6dsv_gy_full_scale_set() (Status: %d).",
			      status);
		return U_ERROR;
	}

	/* Set accelerometer output data rate. */
	status = lsm6dsv_xl_data_rate_set(&imu, LSM6DSV_ODR_AT_120Hz);
	if (status != 0) {
		PRINTLN_ERROR("Failed to set IMU Accelerometer Datarate via "
			      "lsm6dsv_xl_data_rate_set() (Status: %d).",
			      status);
		return U_ERROR;
	}

	/* Set gyroscope output data rate. */
	status = lsm6dsv_gy_data_rate_set(&imu, LSM6DSV_ODR_AT_120Hz);
	if (status != 0) {
		PRINTLN_ERROR("Failed to set IMU Gyroscope Datarate via "
			      "lsm6dsv_gy_data_rate_set() (Status: %d).",
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
		raw_data[0]); // Somewhat important: These functions MUST match the
	// full-scale settings configured in peripherals_init(). The
	// conversions will be incorrect if you use the wrong
	// functions.
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
		raw_data[0]); // Somewhat important: These functions MUST match the
	// full-scale settings configured in peripherals_init(). The
	// conversions will be incorrect if you use the wrong
	// functions.
	data->y = lsm6dsv_from_fs2000_to_mdps(raw_data[1]);
	data->z = lsm6dsv_from_fs2000_to_mdps(raw_data[2]);

	return U_SUCCESS;
}

int init_compute(peripherals_t *peripherals)
{
	int status;

	assert(peripherals);

	status = imu_init();
	if (status == U_SUCCESS) {
		imu_available = true;
		PRINTLN_INFO("IMU initialized successfully.");
	} else {
		imu_available = false;
		PRINTLN_ERROR(
			"IMU failed to initialize. Continuing without IMU.");
	}

	status = p3t_init();
	if (status == U_SUCCESS) {
		p3t_available = true;
		PRINTLN_INFO("P3T initialized successfully.");
	} else {
		p3t_available = false;
		PRINTLN_ERROR(
			"P3T init failed. Continuing without temperature sensor.");
	}

	return U_SUCCESS;
}

void compute_set_fault(bool fault_state)
{
	if (!fault_state) {
		HAL_GPIO_WritePin(FAULT_MCU_GPIO_Port, FAULT_MCU_Pin,
				  GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(FAULT_MCU_GPIO_Port, FAULT_MCU_Pin,
				  GPIO_PIN_RESET);
	}
}

static void set_shutdown_active(void *arg)
{
	peripherals_t *peripherals = (peripherals_t *)arg;
	// only react to change in state
	if (peripherals->shutdown_active) {
		return;
	}

	mutex_get(&shutdown_mutex);
	peripherals->shutdown_active = true;
	mutex_put(&shutdown_mutex);
	send_shutdown_as_read_by_bms(true, read_shutdown_ts_minus_sense(), read_shutdown_ts_plus_sense(),
				   read_shutdown_acc_sense(), read_shutdown_tsip_sense());
}

static void set_shutdown_inactive(void *arg)
{
	peripherals_t *peripherals = (peripherals_t *)arg;

	// only react to change in state
	if (!peripherals->shutdown_active) {
		return;
	}

	mutex_get(&shutdown_mutex);
	peripherals->shutdown_active = false;
	mutex_put(&shutdown_mutex);
	send_shutdown_as_read_by_bms(false, read_shutdown_ts_minus_sense(), read_shutdown_ts_plus_sense(),
				   read_shutdown_acc_sense(), read_shutdown_tsip_sense());
}

bool read_shutdown_ts_minus_sense(void)
{
	return HAL_GPIO_ReadPin(TS_MINUS_SENSE_GPIO_Port, TS_MINUS_SENSE_Pin);
}
bool read_shutdown_ts_plus_sense(void)
{
	return HAL_GPIO_ReadPin(TS_PLUS_SENSE_GPIO_Port, TS_PLUS_SENSE_Pin);
}
bool read_shutdown_acc_sense(void)
{
	return HAL_GPIO_ReadPin(ACC_SENSE_GPIO_Port, ACC_SENSE_Pin);
}
bool read_shutdown_tsip_sense(void)
{
	return HAL_GPIO_ReadPin(TSIP_SENSE_GPIO_Port, TSIP_SENSE_Pin);
}

void read_shutdown(peripherals_t *peripherals)
{
	static nertimer_t shutdown_active_timer = { 0 };
	static nertimer_t shutdown_inactive_timer = { 0 };
	static const uint16_t debounce_time = 200; // ms

	// Read shutdown sense using TS_MINUS_SENSE pin
	bool shutdown_inactive =
		read_shutdown_ts_minus_sense() &&
		read_shutdown_ts_plus_sense() &&
		read_shutdown_acc_sense() &&
		read_shutdown_tsip_sense();

	debounce(!shutdown_inactive, &shutdown_active_timer, debounce_time,
		 set_shutdown_active, peripherals);
	debounce(shutdown_inactive, &shutdown_inactive_timer, debounce_time,
		 set_shutdown_inactive, peripherals);
}

// PERIPHERALS THREAD
void vPeripherals(ULONG thread_input)
{
	const uint32_t TELEM_TIMEOUT = 500; // ms

	PRINTLN_INFO("Starting Peripherals thread...");

	peripherals_args_t *peripherals_args =
		(peripherals_args_t *)thread_input;
	peripherals_t *peripherals = peripherals_args->peripherals;
	peripherals->shutdown_active = false;

	nertimer_t telem_timer = { 0 };

	init_compute(peripherals);

	start_timer(&telem_timer, TELEM_TIMEOUT);

	for (;;) {
		mutex_get(&peripherals_mutex);

		if (imu_available) { // guarded method to ensure things still work if the IMU chip doesn't
			if (imu_getAcceleration(
				    &peripherals->imu_data.accel_data) !=
			    U_SUCCESS) {
				PRINTLN_ERROR(
					"IMU accel read failed. Disabling IMU.");
				imu_available = false;
			}

			if (imu_available &&
			    imu_getAngularRate(
				    &peripherals->imu_data.ang_rate_data) !=
				    U_SUCCESS) {
				PRINTLN_ERROR(
					"IMU gyro read failed. Disabling IMU.");
				imu_available = false;
			}
		}

		if (!imu_available) {
			peripherals->imu_data.accel_data.x =
				0; // IMU will return 0's in the case it is not functioning
			peripherals->imu_data.accel_data.y = 0;
			peripherals->imu_data.accel_data.z = 0;

			peripherals->imu_data.ang_rate_data.x = 0;
			peripherals->imu_data.ang_rate_data.y = 0;
			peripherals->imu_data.ang_rate_data.z = 0;
		}

		if (p3t_available) {
			if (p3t1755_getBoardTemp(&peripherals->onboard_temp) !=
			    U_SUCCESS) {
				PRINTLN_ERROR(
					"P3T read failed. Disabling sensor.");
				p3t_available = false;
			}
		}

		if (!p3t_available) {
			peripherals->onboard_temp = 0;
		}

		mutex_put(&peripherals_mutex);

		read_shutdown(peripherals);

		if (is_timer_expired(&telem_timer) &&
		    !is_timer_active(&telem_timer)) {
			// send shutdown state periodically

			send_shutdown_as_read_by_bms(
				peripherals->shutdown_active, read_shutdown_ts_minus_sense(), read_shutdown_ts_plus_sense(),
				read_shutdown_acc_sense(), read_shutdown_tsip_sense());

			mutex_get(&peripherals_mutex);
			send_bms_onboard_temperature(peripherals->onboard_temp);
			send_bms_imu_accelerometer(
				peripherals->imu_data.accel_data.x,
				peripherals->imu_data.accel_data.y,
				peripherals->imu_data.accel_data.z);
			send_bms_imu_gyro(
				peripherals->imu_data.ang_rate_data.x,
				peripherals->imu_data.ang_rate_data.y,
				peripherals->imu_data.ang_rate_data.z);
			mutex_put(&peripherals_mutex);
		}

		tx_thread_sleep(MS_TO_TICKS(50));
	}
}
