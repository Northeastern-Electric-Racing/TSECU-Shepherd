
#include "unity.h"

#include "cell_temp_sanitizer.h"
#include <float.h>
#include <stdbool.h>
#include <string.h>

static sanitizer_t sanitizer;
static analyzer_t analyzer;

chipdata_t *get_chip_data(analyzer_t *test_analyzer, uint8_t chip)
{
    return &test_analyzer->chip_data[chip];
}

static void set_all_cell_temps(float value)
{
    int chip;
    int cell;

    for (chip = 0; chip < NUM_CHIPS; chip++) {
        for (cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
            analyzer.chip_data[chip].cell_temp[cell] = value;
        }
    }
}

static void set_all_last_temp(float value)
{
    int chip;
    int cell;

    for (chip = 0; chip < NUM_CHIPS; chip++) {
        for (cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
            sanitizer.sanitized_therms[chip][cell].last_temp = value;
        }
    }
}

static void assert_all_valid(bool expected)
{
    int chip;
    int cell;

    for (chip = 0; chip < NUM_CHIPS; chip++) {
        for (cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
            if (expected) {
                TEST_ASSERT_TRUE(
                    sanitizer.sanitized_therms[chip][cell].valid);
            } else {
                TEST_ASSERT_FALSE(
                    sanitizer.sanitized_therms[chip][cell].valid);
            }
        }
    }
}

void setUp(void)
{
    memset(&sanitizer, 0, sizeof(sanitizer));
    memset(&analyzer, 0, sizeof(analyzer));
    temp_sanitizer_init(&sanitizer);
}

void tearDown(void)
{
}

void test_init_sets_max_sentinel_and_indices(void)
{
    TEST_ASSERT_EQUAL_FLOAT(FLT_MIN, sanitizer.max_sanitized_temp.val);
    TEST_ASSERT_EQUAL_UINT8(0, sanitizer.max_sanitized_temp.cellNum);
    TEST_ASSERT_EQUAL_UINT8(0, sanitizer.max_sanitized_temp.chipIndex);
}

void test_init_sets_min_sentinel_and_indices(void)
{
    TEST_ASSERT_EQUAL_FLOAT(FLT_MAX, sanitizer.min_sanitized_temp.val);
    TEST_ASSERT_EQUAL_UINT8(0, sanitizer.min_sanitized_temp.cellNum);
    TEST_ASSERT_EQUAL_UINT8(0, sanitizer.min_sanitized_temp.chipIndex);
}

void test_init_marks_all_therms_valid(void)
{
    assert_all_valid(true);
}

void test_init_zeroes_all_last_temp(void)
{
    int chip;
    int cell;

    for (chip = 0; chip < NUM_CHIPS; chip++) {
        for (cell = 0; cell < NUM_CELLS_PER_CHIP; cell++) {
            TEST_ASSERT_EQUAL_FLOAT(
                0.0f,
                sanitizer.sanitized_therms[chip][cell].last_temp);
        }
    }
}

void test_first_run_accepts_all_readings_regardless_of_jump_size(void)
{
    set_all_last_temp(100.0f);
    set_all_cell_temps(0.0f);

    temp_sanitizer_run(&sanitizer, &analyzer);

    assert_all_valid(true);
}

void test_global_max_computed_correctly(void)
{
    set_all_cell_temps(10.0f);
    set_all_last_temp(10.0f);

    analyzer.chip_data[2].cell_temp[3] = 42.0f;
    sanitizer.sanitized_therms[2][3].last_temp = 42.0f;

    temp_sanitizer_run(&sanitizer, &analyzer);

    TEST_ASSERT_EQUAL_FLOAT(42.0f, sanitizer.max_sanitized_temp.val);
    TEST_ASSERT_EQUAL_UINT8(2, sanitizer.max_sanitized_temp.chipIndex);
    TEST_ASSERT_EQUAL_UINT8(3, sanitizer.max_sanitized_temp.cellNum);
}

void test_global_min_computed_correctly(void)
{
    set_all_cell_temps(10.0f);
    set_all_last_temp(10.0f);

    /* Use a positive value: multiplicative thresholds misbehave with negatives */
    analyzer.chip_data[3].cell_temp[4] = 2.0f;
    sanitizer.sanitized_therms[3][4].last_temp = 2.0f;

    temp_sanitizer_run(&sanitizer, &analyzer);

    TEST_ASSERT_EQUAL_FLOAT(2.0f, sanitizer.min_sanitized_temp.val);
    TEST_ASSERT_EQUAL_UINT8(3, sanitizer.min_sanitized_temp.chipIndex);
    TEST_ASSERT_EQUAL_UINT8(4, sanitizer.min_sanitized_temp.cellNum);
}

void test_subsequent_run_invalidates_large_positive_jump(void)
{
    /* Set last_temp directly so first_reading state does not corrupt the run */
    set_all_last_temp(100.0f);
    set_all_cell_temps(100.0f);
    analyzer.chip_data[0].cell_temp[1] = 121.0f;  /* > 100 * 1.20 = 120 */
    temp_sanitizer_run(&sanitizer, &analyzer);

    TEST_ASSERT_FALSE(sanitizer.sanitized_therms[0][1].valid);
    TEST_ASSERT_TRUE(sanitizer.sanitized_therms[0][0].valid);
}

void test_subsequent_run_invalidates_large_negative_jump(void)
{
    set_all_last_temp(100.0f);
    set_all_cell_temps(100.0f);
    analyzer.chip_data[0].cell_temp[1] = 79.0f;  /* < 100 * 0.80 = 80 */
    temp_sanitizer_run(&sanitizer, &analyzer);

    TEST_ASSERT_FALSE(sanitizer.sanitized_therms[0][1].valid);
    TEST_ASSERT_TRUE(sanitizer.sanitized_therms[0][0].valid);
}

void test_upper_threshold_boundary_remains_valid(void)
{
    /* last_temp=100, cell_temp=exactly 100*(1+0.20)=120; strict > means not invalid */
    set_all_last_temp(100.0f);
    set_all_cell_temps(100.0f * (1.0f + (float)TOO_DIFF_THRESHOLD));
    temp_sanitizer_run(&sanitizer, &analyzer);

    TEST_ASSERT_TRUE(sanitizer.sanitized_therms[0][1].valid);
}

void test_lower_threshold_boundary_remains_valid(void)
{
    /* last_temp=100, cell_temp=exactly 100*(1-0.20)=80; strict < means not invalid */
    set_all_last_temp(100.0f);
    set_all_cell_temps(100.0f * (1.0f - (float)TOO_DIFF_THRESHOLD));
    temp_sanitizer_run(&sanitizer, &analyzer);

    TEST_ASSERT_TRUE(sanitizer.sanitized_therms[0][1].valid);
}

void test_in_range_change_remains_valid(void)
{
    set_all_last_temp(100.0f);
    set_all_cell_temps(100.0f);
    analyzer.chip_data[0].cell_temp[1] = 110.0f;  /* within [80, 120] */
    temp_sanitizer_run(&sanitizer, &analyzer);

    TEST_ASSERT_TRUE(sanitizer.sanitized_therms[0][1].valid);
}

void test_invalid_cell_is_excluded_from_max_updates(void)
{
    set_all_last_temp(100.0f);
    set_all_cell_temps(100.0f);
    analyzer.chip_data[0].cell_temp[1] = 130.0f;  /* > 120 → invalid */
    analyzer.chip_data[1].cell_temp[2] = 110.0f;  /* within range → valid, new max */
    temp_sanitizer_run(&sanitizer, &analyzer);

    TEST_ASSERT_FALSE(sanitizer.sanitized_therms[0][1].valid);
    TEST_ASSERT_EQUAL_FLOAT(110.0f, sanitizer.max_sanitized_temp.val);
    TEST_ASSERT_EQUAL_UINT8(1, sanitizer.max_sanitized_temp.chipIndex);
    TEST_ASSERT_EQUAL_UINT8(2, sanitizer.max_sanitized_temp.cellNum);
}

void test_invalid_cell_is_excluded_from_min_updates(void)
{
    set_all_last_temp(100.0f);
    set_all_cell_temps(100.0f);
    analyzer.chip_data[0].cell_temp[1] = 70.0f;   /* < 80 → invalid */
    analyzer.chip_data[1].cell_temp[2] = 90.0f;   /* within range → valid, new min */
    temp_sanitizer_run(&sanitizer, &analyzer);

    TEST_ASSERT_FALSE(sanitizer.sanitized_therms[0][1].valid);
    TEST_ASSERT_EQUAL_FLOAT(90.0f, sanitizer.min_sanitized_temp.val);
    TEST_ASSERT_EQUAL_UINT8(1, sanitizer.min_sanitized_temp.chipIndex);
    TEST_ASSERT_EQUAL_UINT8(2, sanitizer.min_sanitized_temp.cellNum);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_sets_max_sentinel_and_indices);
    RUN_TEST(test_init_sets_min_sentinel_and_indices);
    RUN_TEST(test_init_marks_all_therms_valid);
    RUN_TEST(test_init_zeroes_all_last_temp);
    RUN_TEST(test_first_run_accepts_all_readings_regardless_of_jump_size);
    RUN_TEST(test_global_max_computed_correctly);
    RUN_TEST(test_global_min_computed_correctly);
    RUN_TEST(test_subsequent_run_invalidates_large_positive_jump);
    RUN_TEST(test_subsequent_run_invalidates_large_negative_jump);
    RUN_TEST(test_upper_threshold_boundary_remains_valid);
    RUN_TEST(test_lower_threshold_boundary_remains_valid);
    RUN_TEST(test_in_range_change_remains_valid);
    RUN_TEST(test_invalid_cell_is_excluded_from_max_updates);
    RUN_TEST(test_invalid_cell_is_excluded_from_min_updates);

    return UNITY_END();
}