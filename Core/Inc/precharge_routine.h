#include "adi2950_interaction.h"
#include "timer.h"
#include "hv_plate.h"

typedef struct {
	hv_plate_t *hv_plate;
	GPO_2950 gpo;
	float transition_ratio;
	float lower_ratio;
	nertimer_t open_debounce_timer;
	nertimer_t close_debounce_timer;
	uint32_t debounce_time;
	bool air_switch_closed;
} prechargeconfig_t;

/**
 * @brief Given the cell, SPI, AIR switch, initializes the pre-charge structure for the given BATT/TS voltage capacitor threshold ratio, 
 * and with the given debounce time to wait when the AIR opens before closing again.ADI1_delay_ms
 * @param ic idk what this is T-T.
 * @param hspi idk what this is either T-T.
 * @param gpo the AIR voltage-precharge switch.
 * @param threshold_ratio is the ratio to charge the capacitor until of total voltage.
 * @param debounce_time is the time to wait after the AIR switch opens before closing again (to prevent immediate closes) in milliseconds.
 * @param precharge_config is the empty configuration to configure.
 * @return a precharge configuration for running the precharge thread.
 */
prechargeconfig_t *precharge_init(hv_plate_t *hv_plate, GPO_2950 gpo,
				  float transition_ratio, float lower_ratio,
				  uint32_t debounce_time,
				  prechargeconfig_t *precharge_config);
/**
 * @brief Handles the precharge routine given the current BATT and TS voltages.
 * @param precharge_config the prechsarge configuration struct.
 * @param batt_voltage the current BATT voltage.
 * @param ts_voltage the current TS voltage.
 * NOTE: Ment to be run in a Thread on a loop due to debounces
 */
void handle_precharge(prechargeconfig_t *precharge_config, float batt_voltage,
		      float ts_voltage);
