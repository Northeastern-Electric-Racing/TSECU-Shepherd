
#include "test_state_machine.h"
#include "datastructs.h"
#include "mock_charging.h"
#include "mock_timer.h"
#include <stdbool.h>
#include <stdlib.h>

bms_t *bmsdata;

I2C_HandleTypeDef hi2c1;
ADC_HandleTypeDef hadc1;

void setUp(void) {
    bmsdata = malloc(sizeof(bms_t));
}

void tearDown(void) {
    free(bmsdata);
}

// A simple random test
void test_sm_balance_cells(void) {
    handle_balance_cells_Ignore();
    sm_balance_cells(bmsdata);
    TEST_ASSERT_EQUAL_INT(true, bmsdata->should_balance);
}

// testing if we should charge
void test_should_charge(void) {
    bmsdata->is_charger_connected = true;
    is_timer_expired_IgnoreAndReturn(true);
    is_timer_active_IgnoreAndReturn(false);

    is_timer_active_IgnoreAndReturn(false);
    start_timer_Ignore();
    TEST_ASSERT_EQUAL_INT(false, sm_charging_check(bmsdata));
}

void test_charger_not_connected(void) {
    bmsdata->is_charger_connected = false;
    TEST_ASSERT_EQUAL_INT(false, sm_charging_check(bmsdata));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sm_balance_cells);
    RUN_TEST(test_should_charge);
    RUN_TEST(test_charger_not_connected);
    return UNITY_END();
}