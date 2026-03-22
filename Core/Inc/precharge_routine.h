#ifndef PRECHARGE_ROUTINE_H
#define PRECHARGE_ROUTINE_H

#include "adi2950_interaction.h"
#include "timer.h"
#include "hv_plate.h"
#include "can_messages_tx.h"

typedef struct {
	hv_plate_t *hv_plate;
	GPO_2950 gpo;
	float transition_ratio;
	nertimer_t open_debounce_timer;
	nertimer_t close_debounce_timer;
	uint32_t debounce_time;
	bool air_switch_closed;
} prechargeconfig_t;

/**
 * @brief Given the cell, SPI, AIR switch, initializes the pre-charge structure for the given BATT/TS voltage capacitor threshold ratio, 
 * and with the given debounce time to wait when the AIR opens before closing again.ADI1_delay_ms
 * @param precharge_config is the empty configuration to configure.
 * @param ic pointer to ADBMS2950 data struct
 * @param threshold_ratio if batt volts > ts volts * threshold_ratio, close the AIR switch
 * @param debounce_time is the time to wait after the AIR switch opens before closing again (to prevent immediate closes) in milliseconds.
 */
void precharge_init(prechargeconfig_t *precharge_config, hv_plate_t *hv_plate,
		    float transition_ratio, uint32_t debounce_time);
/**
 * @brief Handles the precharge routine given the current BATT and TS voltages.
 * @param precharge_config the prechsarge configuration struct.
 * NOTE: Ment to be run in a Thread on a loop due to debounces
 */
void handle_precharge(prechargeconfig_t *precharge_config);

void vPrecharge(ULONG args);

#endif