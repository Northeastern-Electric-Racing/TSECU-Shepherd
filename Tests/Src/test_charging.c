#include "unity.h"

#include <string.h>

#include "charging.h"
#include "mock_u_tx_mutex.h"

static analyzer_t analyzer;
static acc_data_t acc_data;

chipdata_t *get_chip_data(analyzer_t *analyzer_ptr, uint8_t chip)
{
	return &analyzer_ptr->chip_data[chip];
}

static void set_all_ocv(float voltage)
{
	for (uint8_t chip = 0U; chip < NUM_CHIPS; chip++) {
		for (uint8_t cell = 0U; cell < NUM_CELLS_PER_CHIP; cell++) {
			analyzer.chip_data[chip].open_cell_voltage[cell] =
				voltage;
		}
	}
}

void setUp(void)
{
	memset(&analyzer, 0, sizeof(analyzer));
	memset(&acc_data, 0, sizeof(acc_data));

	analyzer.min_ocv.val = 3.5f;
	set_all_ocv(analyzer.min_ocv.val);
	pwm_duty_cycle_set(50U);

	mutex_get_IgnoreAndReturn(0U);
	mutex_put_IgnoreAndReturn(0U);
}

void tearDown(void)
{
}

void test_pwm_duty_cycle(void)
{
	static const struct {
		uint8_t min;
		uint8_t max;
		PWM_DUTY duty_cycle;
		float percentage;
	} ranges[] = {
		{ 0U, 0U, PWM_0_0_PCT, 0.0f },
		{ 1U, 6U, PWM_6_6_PCT, 6.6f },
		{ 7U, 13U, PWM_13_2_PCT, 13.2f },
		{ 14U, 19U, PWM_19_8_PCT, 19.8f },
		{ 20U, 26U, PWM_26_4_PCT, 26.4f },
		{ 27U, 33U, PWM_33_0_PCT, 33.0f },
		{ 34U, 39U, PWM_39_6_PCT, 39.6f },
		{ 40U, 46U, PWM_46_2_PCT, 46.2f },
		{ 47U, 52U, PWM_52_8_PCT, 52.8f },
		{ 53U, 59U, PWM_59_4_PCT, 59.4f },
		{ 60U, 66U, PWM_66_0_PCT, 66.0f },
		{ 67U, 72U, PWM_72_6_PCT, 72.6f },
		{ 73U, 79U, PWM_79_2_PCT, 79.2f },
		{ 80U, 85U, PWM_85_8_PCT, 85.8f },
		{ 86U, 92U, PWM_92_4_PCT, 92.4f },
		{ 93U, 100U, PWM_100_0_PCT, 100.0f },
	};

	for (size_t range = 0U; range < sizeof(ranges) / sizeof(ranges[0]);
	     range++) {
		for (uint16_t request = ranges[range].min;
		     request <= ranges[range].max; request++) {
			pwm_duty_cycle_set((uint8_t)request);
			TEST_ASSERT_EQUAL(ranges[range].duty_cycle,
					  pwm_duty_cycle_get());
			TEST_ASSERT_EQUAL_FLOAT(ranges[range].percentage,
					pwm_duty_cycle_setting_get());
		}
	}

	pwm_duty_cycle_set(101U);
	TEST_ASSERT_EQUAL(PWM_0_0_PCT, pwm_duty_cycle_get());
	TEST_ASSERT_EQUAL_FLOAT(0.0f, pwm_duty_cycle_setting_get());

	pwm_duty_cycle_set(UINT8_MAX);
	TEST_ASSERT_EQUAL(PWM_0_0_PCT, pwm_duty_cycle_get());
	TEST_ASSERT_EQUAL_FLOAT(0.0f, pwm_duty_cycle_setting_get());
}

void test_balance_threshold(void)
{
	const float threshold = analyzer.min_ocv.val + 0.02f;

	analyzer.chip_data[0].open_cell_voltage[3] = threshold + 0.001f;
	analyzer.chip_data[0].open_cell_voltage[4] = threshold;

	handle_balance_cells(&analyzer, &acc_data);

	TEST_ASSERT_EQUAL(PWM_52_8_PCT, acc_data.discharge_config[0][3]);
	TEST_ASSERT_EQUAL(PWM_0_0_PCT, acc_data.discharge_config[0][4]);
}

void test_balance_limit(void)
{
	for (uint8_t cell = 0U; cell < 8U; cell++) {
		analyzer.chip_data[0].open_cell_voltage[cell] =
			3.53f + (0.01f * cell);
	}

	handle_balance_cells(&analyzer, &acc_data);

	TEST_ASSERT_EQUAL(PWM_0_0_PCT, acc_data.discharge_config[0][0]);
	for (uint8_t cell = 1U; cell < 8U; cell++) {
		TEST_ASSERT_EQUAL(PWM_52_8_PCT,
				  acc_data.discharge_config[0][cell]);
	}
}

void test_balance_indexes(void)
{
	analyzer.chip_data[0].open_cell_voltage[2] = 3.60f;
	analyzer.chip_data[0].open_cell_voltage[7] = 3.70f;
	analyzer.chip_data[0].open_cell_voltage[12] = 3.80f;

	handle_balance_cells(&analyzer, &acc_data);

	TEST_ASSERT_EQUAL(PWM_52_8_PCT, acc_data.discharge_config[0][2]);
	TEST_ASSERT_EQUAL(PWM_52_8_PCT, acc_data.discharge_config[0][7]);
	TEST_ASSERT_EQUAL(PWM_52_8_PCT, acc_data.discharge_config[0][12]);
	TEST_ASSERT_EQUAL(PWM_0_0_PCT, acc_data.discharge_config[0][0]);
}

void test_balance_clear(void)
{
	for (uint8_t chip = 0U; chip < NUM_CHIPS; chip++) {
		for (uint8_t cell = 0U; cell < NUM_CELLS_PER_CHIP; cell++) {
			acc_data.discharge_config[chip][cell] = PWM_100_0_PCT;
		}
	}

	handle_balance_cells(&analyzer, &acc_data);

	for (uint8_t chip = 0U; chip < NUM_CHIPS; chip++) {
		for (uint8_t cell = 0U; cell < NUM_CELLS_PER_CHIP; cell++) {
			TEST_ASSERT_EQUAL(PWM_0_0_PCT,
					  acc_data.discharge_config[chip][cell]);
		}
	}
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_pwm_duty_cycle);
	RUN_TEST(test_balance_threshold);
	RUN_TEST(test_balance_limit);
	RUN_TEST(test_balance_indexes);
	RUN_TEST(test_balance_clear);

	return UNITY_END();
}
