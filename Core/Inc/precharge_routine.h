#ifndef PRECHARGE_ROUTINE_H
#define PRECHARGE_ROUTINE_H

#include "adi2950_interaction.h"
#include "timer.h"
#include "hv_plate.h"
#include "can_messages_tx.h"

typedef enum {
	PRECHARGE_OPEN = 0,
	PRECHARGE_FLOATING = 1,
	PRECHARGE_CLOSED = 2,
} precharge_state_t;

typedef struct {
	hv_plate_t *hv_plate;
	peripherals_t *peripherals;
	float transition_ratio;
	nertimer_t open_debounce_timer;
	nertimer_t close_debounce_timer;
	nertimer_t open_to_floating_debounce_timer;
	precharge_state_t precharge_state;
} prechargeconfig_t;

/**
 * @brief Given the cell, SPI, AIR switch, initializes the pre-charge structure for the given BATT/TS voltage capacitor threshold ratio, 
 * and with the given debounce time to wait when the AIR opens before closing again.ADI1_delay_ms
 * @param precharge_config is the empty configuration to configure.
 * @param ic pointer to ADBMS2950 data struct
 * @param threshold_ratio if batt volts > ts volts * threshold_ratio, close the AIR switch
 */
void precharge_init(prechargeconfig_t *precharge_config, hv_plate_t *hv_plate,
		    peripherals_t *peripherals, float transition_ratio);
/**
 * @brief Handles the precharge routine given the current BATT and TS voltages.
 * @param precharge_config the prechsarge configuration struct.
 * NOTE: Ment to be run in a Thread on a loop due to debounces
 */
void handle_precharge(prechargeconfig_t *precharge_config);

void vPrecharge(ULONG args);

#endif
