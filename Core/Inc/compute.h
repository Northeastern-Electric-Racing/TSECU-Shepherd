#ifndef _COMPUTE_H
#define _COMPUTE_H

#include <stdbool.h>
#include <stdint.h>

#include "datastructs.h"
#include "stm32h5xx.h"
#include "u_tx_threads.h"

#define CURRENT_SENSOR_PIN_L A1
#define CURRENT_SENSOR_PIN_H A0
#define MEAS_5VREF_PIN A7
#define FAULT_PIN 2
#define CHARGE_SAFETY_RELAY 4
#define CHARGE_DETECT 5
#define CHARGER_BAUD 250000U
#define MC_BAUD 1000000U
#define MAX_ADC_RESOLUTION 4095 // 12 bit ADC
#define P3T1755_DEV_ADDR 0x48

/* @brief User-facing initalization with sane defaults for p3t1755[...] sensor
 * @returns U_SUCCESS if initialization was successful, U_ERROR otherwise.
 */
int p3t_init(void);

/* @brief gets board temperature of BMS compute module via p3t1755 senso
 * @param temp_c Pointer to float where temperature in Celsius will be stored.
 * @returns status
 */
int p3t1755_getBoardTemp(float *temp_c);

int imu_init(void);

/**
 * Gets the IMU's accerlation reading.
 * @param data
 * @return Status
 */
int imu_getAcceleration(vector3_t *data);

/**
 * Gets the IMU's angular rate reading.
 * @param data
 * @return Status
 */
int imu_getAngularRate(vector3_t *data);

/**
 * @brief updates fault relay
 *
 * @param fault_state
 */
void compute_set_fault(bool fault_state);

/**
 * @brief Checks if the shutdown circuit is open.
 */
bool read_shutdown();

/**
 * @brief Initializes peripherals for compute thread.
 * @param peripherals Pointer to peripherals struct
 */
void init_compute(peripherals_t *peripherals);

void vPeripherals(ULONG thread_input);

#endif // COMPUTE_H
