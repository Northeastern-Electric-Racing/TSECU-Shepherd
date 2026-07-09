#ifndef BMS_CONFIG_H
#define BMS_CONFIG_H

#define DEBUG_MODE_ENABLED true
#define DEBUG_STATS

// Hardware definition
#define NUM_SEGMENTS	   5
#define NUM_CHIPS_PER_SEGMENT 2
#define NUM_CHIPS	   (NUM_SEGMENTS * NUM_CHIPS_PER_SEGMENT)
#define NUM_CELLS_PER_CHIP 13
#define NUM_CELLS	   (NUM_CELLS_PER_CHIP * NUM_CHIPS)
// only actual flexPCB therms counted
#define NUM_THERMS		    (7 * NUM_CHIPS)
#define NUM_ONBOARD_THERMS_PER_CHIP 3

// Firmware limits
#define MAX_TEMP    60 /* Celsius */
#define MIN_TEMP    -40 /* Celsius */
#define MAX_DELTA_V 0.005f
#define BAL_MIN_V   3.00f

/* Molicel P50B Cell Specifications */
#define TYP_CAPICITY_AH	    5.0f /* Amp-hours */
#define TYP_CAPACITY_WH	    18.0f /* Watt-hours */
#define MIN_CAPICITY_AH	    4.85f /* Amp-hours */
#define MIN_CAPACITY_WH	    17.5f /* Watt-hours */
#define MIN_VOLT	    2.5f
#define NOM_VOLT	    3.6f
#define MAX_VOLT	    4.2f
#define MAX_CHARGE_VOLT	    4.19f
#define MAX_CHARGE_VOLT_FLT 4.25f // LOADED FAULT
#define MAX_CHG_CURR	    25.0f /* Amps */
#define MAX_DISCHG_CURR	    60.0f /* Amps */
#define MIN_CHG_TEMP	    -20 /* Celsius */
#define MIN_DISCHG_TEMP	    -40 /* Celsius */
#define MAX_CELL_TEMP	    60 /* Celsius (rules) */
#define TYP_IMPDNCE	    0.0128f /* Ohms, DC, 50% SoC */

// Pack Limits
#define MAX_PACK_CHG_CURR \
	30 /* Pack-level charge limit: (MAX_CHG_CURR - 3.5A margin) × 3 cells in parallel */
#define MAX_PACK_DISCHG_CURR \
	(MAX_DISCHG_CURR *   \
	 3) /* Pack-level discharge limit: MAX_DISCHG_CURR × 3 cells in parallel */
#define MIN_DCL 30.0f

// ADBMS6830 limits
#define MAX_CHIP_TEMP 60

// Algorithm settings
#define VOLT_SAG_MARGIN \
	0.45f // Volts above the minimum cell voltage we would like to aim for
#define OCV_CURR_THRESH 0.5f /* in A */

// Charging settings
#define CHARGING_CURRENT    5.0f
#define CHARGE_SETL_TIMEOUT 30000 // 1 minute, may need adjustment
#define CHARGE_SETL_TIMEUP  120000 // 5 minutes, may need adjustment

//Fault times
#define OVER_CURR_TIME \
	55000 //todo adjust these based on testing and/or counter values
#define OVER_CHG_CURR_TIME 55000
#define UNDER_VOLT_TIME	   55000
#define OVER_VOLT_CHG_TIME 15000
#define OVER_VOLT_TIME	   55000
#define LOW_CELL_TIME	   55000
#define HIGH_TEMP_TIME	   55000
#define MAX_CHIPTEMP_TIME  55000

// system wide base ADBMS sample rate
#define SAMPLE_RATE 2 /* Hz */

#endif
