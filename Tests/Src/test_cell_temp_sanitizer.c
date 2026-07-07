#include "unity.h"

#include "cell_temp_sanitizer.h"

/* -------------------------------------------------
 * Setup / Teardown
 * ------------------------------------------------- */

static sanitizer_t test_sanitizer;
static analyzer_t test_analyzer;

void setUp(void)
{
    temp_sanitizer_init(&test_sanitizer);

    for (uint8_t chip = 0U; chip < NUM_CHIPS; chip++)
    {
        for (uint8_t cell = 0U; cell < NUM_CELLS_PER_CHIP; cell++)
        {
            test_analyzer.chip_data[chip].cell_temp[cell] = 25.0f;
        }
    }
}

void tearDown(void) {}

/* -------------------------------------------------
 * Cell Temperature Sanitizer
 * ------------------------------------------------- */

void test_temp_init_defaults(void)
{
    for (uint8_t chip = 0U; chip < NUM_CHIPS; chip++)
    {
        for (uint8_t cell = 0U; cell < NUM_CELLS_PER_CHIP; cell++)
        {
            TEST_ASSERT_EQUAL_FLOAT(0.0f, test_sanitizer.sanitized_therms[chip][cell].last_temp);
            TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[chip][cell].valid);
            TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[chip][cell].initialized);
            TEST_ASSERT_EQUAL_UINT8(0U, test_sanitizer.sanitized_therms[chip][cell].fault_count);
        }
    }
}

void test_temp_valid_min_max(void)
{
    /* -------- Valid temperature samples -------- */

    test_analyzer.chip_data[0].cell_temp[0] = 20.0f;
    test_analyzer.chip_data[0].cell_temp[1] = 30.0f;

    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_TRUE(test_sanitizer.sanitized_therms[0][0].valid);
    TEST_ASSERT_TRUE(test_sanitizer.sanitized_therms[0][0].initialized);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, test_sanitizer.sanitized_therms[0][0].last_temp);

    TEST_ASSERT_EQUAL_FLOAT(20.0f, test_sanitizer.min_sanitized_temp.val);
    TEST_ASSERT_EQUAL_UINT8(0U, test_sanitizer.min_sanitized_temp.chipIndex);
    TEST_ASSERT_EQUAL_UINT8(0U, test_sanitizer.min_sanitized_temp.cellNum);

    TEST_ASSERT_EQUAL_FLOAT(30.0f, test_sanitizer.max_sanitized_temp.val);
    TEST_ASSERT_EQUAL_UINT8(0U, test_sanitizer.max_sanitized_temp.chipIndex);
    TEST_ASSERT_EQUAL_UINT8(1U, test_sanitizer.max_sanitized_temp.cellNum);
}

void test_temp_range_limits(void)
{
    /* -------- Boundary temperatures accepted -------- */

    test_analyzer.chip_data[0].cell_temp[0] = (float)MIN_TEMP;
    test_analyzer.chip_data[0].cell_temp[1] = (float)MAX_CELL_TEMP;

    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_TRUE(test_sanitizer.sanitized_therms[0][0].valid);
    TEST_ASSERT_TRUE(test_sanitizer.sanitized_therms[0][1].valid);

    TEST_ASSERT_EQUAL_FLOAT((float)MIN_TEMP, test_sanitizer.min_sanitized_temp.val);
    TEST_ASSERT_EQUAL_UINT8(0U, test_sanitizer.min_sanitized_temp.chipIndex);
    TEST_ASSERT_EQUAL_UINT8(0U, test_sanitizer.min_sanitized_temp.cellNum);

    TEST_ASSERT_EQUAL_FLOAT((float)MAX_CELL_TEMP, test_sanitizer.max_sanitized_temp.val);
    TEST_ASSERT_EQUAL_UINT8(0U, test_sanitizer.max_sanitized_temp.chipIndex);
    TEST_ASSERT_EQUAL_UINT8(1U, test_sanitizer.max_sanitized_temp.cellNum);
}

void test_temp_startup_range_faults(void)
{
    /* -------- Temperatures outside valid range -------- */

    test_analyzer.chip_data[0].cell_temp[0] = (float)MIN_TEMP - 1.0f;
    test_analyzer.chip_data[0].cell_temp[1] = (float)MAX_CELL_TEMP + 1.0f;

    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][0].initialized);
    TEST_ASSERT_EQUAL_UINT8(1U, test_sanitizer.sanitized_therms[0][0].fault_count);

    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][1].initialized);
    TEST_ASSERT_EQUAL_UINT8(1U, test_sanitizer.sanitized_therms[0][1].fault_count);

    /* -------- Consecutive faults invalidate -------- */

    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][0].valid);
    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][0].initialized);
    TEST_ASSERT_EQUAL_UINT8(2U, test_sanitizer.sanitized_therms[0][0].fault_count);

    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][1].valid);
    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][1].initialized);
    TEST_ASSERT_EQUAL_UINT8(2U, test_sanitizer.sanitized_therms[0][1].fault_count);
}

void test_temp_delta_fault(void)
{
    test_analyzer.chip_data[0].cell_temp[0] = 25.0f;
    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    /* -------- Delta at threshold accepted -------- */

    test_analyzer.chip_data[0].cell_temp[0] = 30.0f;
    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_TRUE(test_sanitizer.sanitized_therms[0][0].valid);
    TEST_ASSERT_EQUAL_FLOAT(30.0f, test_sanitizer.sanitized_therms[0][0].last_temp);
    TEST_ASSERT_EQUAL_UINT8(0U, test_sanitizer.sanitized_therms[0][0].fault_count);

    /* -------- Delta above threshold rejected -------- */

    test_analyzer.chip_data[0].cell_temp[0] = 35.1f;
    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_TRUE(test_sanitizer.sanitized_therms[0][0].valid);
    TEST_ASSERT_EQUAL_FLOAT(30.0f, test_sanitizer.sanitized_therms[0][0].last_temp);
    TEST_ASSERT_EQUAL_UINT8(1U, test_sanitizer.sanitized_therms[0][0].fault_count);

    /* -------- Second fault invalidates -------- */

    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][0].valid);
    TEST_ASSERT_EQUAL_FLOAT(30.0f, test_sanitizer.sanitized_therms[0][0].last_temp);
    TEST_ASSERT_EQUAL_UINT8(2U, test_sanitizer.sanitized_therms[0][0].fault_count);
}

void test_temp_runtime_range_fault(void)
{
    test_analyzer.chip_data[0].cell_temp[0] = 25.0f;
    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    /* -------- Runtime out-of-range rejected -------- */

    test_analyzer.chip_data[0].cell_temp[0] = (float)MAX_CELL_TEMP + 1.0f;
    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_TRUE(test_sanitizer.sanitized_therms[0][0].valid);
    TEST_ASSERT_TRUE(test_sanitizer.sanitized_therms[0][0].initialized);
    TEST_ASSERT_EQUAL_FLOAT(25.0f, test_sanitizer.sanitized_therms[0][0].last_temp);
    TEST_ASSERT_EQUAL_UINT8(1U, test_sanitizer.sanitized_therms[0][0].fault_count);

    /* -------- Consecutive runtime faults invalidate -------- */

    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][0].valid);
    TEST_ASSERT_TRUE(test_sanitizer.sanitized_therms[0][0].initialized);
    TEST_ASSERT_EQUAL_FLOAT(25.0f, test_sanitizer.sanitized_therms[0][0].last_temp);
    TEST_ASSERT_EQUAL_UINT8(2U, test_sanitizer.sanitized_therms[0][0].fault_count);
}

void test_temp_fault_reset(void)
{
    test_analyzer.chip_data[0].cell_temp[0] = 25.0f;
    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    /* -------- Create first fault -------- */

    test_analyzer.chip_data[0].cell_temp[0] = 31.0f;
    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_EQUAL_UINT8(1U, test_sanitizer.sanitized_therms[0][0].fault_count);

    /* -------- Valid sample clears fault count -------- */

    test_analyzer.chip_data[0].cell_temp[0] = 26.0f;
    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_TRUE(test_sanitizer.sanitized_therms[0][0].valid);
    TEST_ASSERT_EQUAL_UINT8(0U, test_sanitizer.sanitized_therms[0][0].fault_count);
    TEST_ASSERT_EQUAL_FLOAT(26.0f, test_sanitizer.sanitized_therms[0][0].last_temp);
}

void test_temp_invalid_ignored(void)
{
    /* -------- Thermistor becomes invalid -------- */

    test_analyzer.chip_data[0].cell_temp[0] = (float)MAX_CELL_TEMP + 1.0f;

    temp_sanitizer_run(&test_sanitizer, &test_analyzer);
    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][0].valid);
    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][0].initialized);
    TEST_ASSERT_EQUAL_UINT8(2U, test_sanitizer.sanitized_therms[0][0].fault_count);

    /* -------- Valid sample after invalidation -------- */

    test_analyzer.chip_data[0].cell_temp[0] = 10.0f;
    test_analyzer.chip_data[0].cell_temp[1] = 20.0f;

    temp_sanitizer_run(&test_sanitizer, &test_analyzer);

    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][0].valid);
    TEST_ASSERT_FALSE(test_sanitizer.sanitized_therms[0][0].initialized);
    TEST_ASSERT_EQUAL_UINT8(2U, test_sanitizer.sanitized_therms[0][0].fault_count);

    /* -------- Invalid thermistor is ignored -------- */

    TEST_ASSERT_EQUAL_FLOAT(20.0f, test_sanitizer.min_sanitized_temp.val);
    TEST_ASSERT_EQUAL_UINT8(0U, test_sanitizer.min_sanitized_temp.chipIndex);
    TEST_ASSERT_EQUAL_UINT8(1U, test_sanitizer.min_sanitized_temp.cellNum);
}

/* -------------------------------------------------
 * Test Runner
 * ------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_temp_init_defaults);
    RUN_TEST(test_temp_valid_min_max);
    RUN_TEST(test_temp_range_limits);
    RUN_TEST(test_temp_startup_range_faults);
    RUN_TEST(test_temp_delta_fault);
    RUN_TEST(test_temp_runtime_range_fault);
    RUN_TEST(test_temp_fault_reset);
    RUN_TEST(test_temp_invalid_ignored);

    return UNITY_END();
}