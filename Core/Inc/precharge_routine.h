#include "adi2950_interaction.h"
#include "timer.h"

typedef struct {
	cell_asic_2950 ic;
	SPI_HandleTypeDef *hspi;
	GPO_2950 gpo;
	float transition_ratio;
	float lower_ratio;
	nertimer_t debounce_timer;
	uint32_t debounce_time;
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
prechargeconfig_t *precharge_init(cell_asic_2950 ic, SPI_HandleTypeDef *hspi,
				  GPO_2950 gpo, float transition_ratio,
				  float lower_ratio, uint32_t debounce_time,
				  prechargeconfig_t *precharge_config);

/**
 * @brief Runs the pre-charge until the threshold-ratio is reached, after which the AIR switch is set to open.
 * @param precharge_config is the configuration to run.
 */
void precharge_run(prechargeconfig_t *precharge_config);