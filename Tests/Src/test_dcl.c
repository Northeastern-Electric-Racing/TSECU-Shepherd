#include "unity.h"

#include "dcl.h"
#include "test_current_limit_algo_config.h"

#include "mock_timer.h"
#include "mock_u_tx_mutex.h"

/* -------------------------------------------------
 * Setup / Teardown
 * ------------------------------------------------- */

void setUp(void)
{
    mutex_get_IgnoreAndReturn(0);
    mutex_put_IgnoreAndReturn(0);

    cancel_timer_Ignore();
    start_timer_Ignore();

    dcl_init(COOLDOWN_ALWAYS);
}

void tearDown(void) {}

/* -------------------------------------------------
 * Instantaneous DCL – Temperature & OCV regions
 * ------------------------------------------------- */

void test_inst_dcl_temperature_and_ocv_regions(void)
{
    current_limit_algo_inputs_t test_inputs;
    bms_algos_t test_algos;

    /* -------- Temperature below minimum (hard clamp) -------- */
    test_algos.inst_DCL = 0.0f;
    test_inputs.min_temp = -5.0f;
    test_inputs.max_temp = 20.0f;
    test_inputs.min_ocv  = 4.0f;

    dcl_calc_inst_limit(test_inputs, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MIN_CURRENT_A, test_algos.inst_DCL);

    /* -------- Temperature above maximum (hard clamp) -------- */
    test_inputs.min_temp = 25.0f;
    test_inputs.max_temp = 70.0f;
    test_inputs.min_ocv  = 4.0f;

    dcl_calc_inst_limit(test_inputs, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MIN_CURRENT_A, test_algos.inst_DCL);

    /* -------- Temperature ramp-up region -------- */
    test_inputs.min_temp = 5.0f;     /* between TEMP_MIN and RAMP_UP_END */
    test_inputs.max_temp = 25.0f;
    test_inputs.min_ocv  = 4.0f;

    dcl_calc_inst_limit(test_inputs, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(105.0f, test_algos.inst_DCL);

    /* -------- Temperature ramp-down region -------- */
    test_inputs.min_temp = 25.0f;
    test_inputs.max_temp = 52.0f;    /* between RAMP_DOWN_START and TEMP_MAX */
    test_inputs.min_ocv  = 4.0f;

    dcl_calc_inst_limit(test_inputs, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(150.0f, test_algos.inst_DCL);

    /* -------- OCV below minimum dominates -------- */
    test_inputs.min_temp = 25.0f;
    test_inputs.max_temp = 30.0f;
    test_inputs.min_ocv  = 2.5f;

    dcl_calc_inst_limit(test_inputs, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MIN_CURRENT_A, test_algos.inst_DCL);

    /* -------- OCV derating region -------- */
    test_inputs.min_temp = 25.0f;
    test_inputs.max_temp = 30.0f;
    test_inputs.min_ocv  = 3.1f;     /* between OCV_MIN and DERATE_THRESH */

    dcl_calc_inst_limit(test_inputs, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(105.0f, test_algos.inst_DCL);

    /* -------- Fully nominal region -------- */
    test_inputs.min_temp = 25.0f;
    test_inputs.max_temp = 30.0f;
    test_inputs.min_ocv  = 3.2f;

    dcl_calc_inst_limit(test_inputs, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_CURRENT_A, test_algos.inst_DCL);
}

/* -------------------------------------------------
 * Continuous DCL – Pulse disabled path
 * ------------------------------------------------- */

void test_cont_dcl_follows_inst_limit_when_pulse_not_allowed(void)
{
    bms_algos_t test_algos;
    float test_pack_current = 0.0f;
    current_limit_algo_inputs_t test_inputs;

    /* -------- Below pulse enable margin -------- */

    test_pack_current = 100.0f;
    test_inputs.min_ocv = 3.1f;
    test_inputs.max_temp = 35.0f;
    test_inputs.min_temp = 31.0f;

    dcl_calc_inst_limit(test_inputs, &test_algos);
    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(test_algos.inst_DCL, test_algos.cont_DCL);

    /* -------- Pulse eligibility lost resets behavior -------- */
    test_pack_current = 185.0f;
    test_inputs.min_ocv = 4.0f;

    is_timer_active_ExpectAnyArgsAndReturn(false);

    dcl_calc_inst_limit(test_inputs, &test_algos);
    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    test_inputs.max_temp = 55.0f;

    dcl_calc_inst_limit(test_inputs, &test_algos);
    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(test_algos.inst_DCL, test_algos.cont_DCL);
}

/* -------------------------------------------------
 * Continuous DCL – REST -> PULSE -> COOLDOWN
 * ------------------------------------------------- */

void test_cont_dcl_pulse_state_transitions(void)
{
    bms_algos_t test_algos = {0};
    float test_pack_current = 0.0f;

    test_algos.inst_DCL = DCL_MAX_CURRENT_A;

    /* -------- Early pulse exit -> COOLDOWN_ALWAYS -------- */

    dcl_init(COOLDOWN_ALWAYS);

    /* REST -> under max continous current */
    test_pack_current = DCL_MAX_CURRENT_A - 20.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* REST -> over max continuous dcl current, debounce not active */
    is_timer_active_ExpectAnyArgsAndReturn(false);
    test_pack_current = DCL_MAX_CURRENT_A + 2.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* REST -> debounce active but not expired */
    is_timer_active_ExpectAnyArgsAndReturn(true);
    is_timer_expired_ExpectAnyArgsAndReturn(false);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* REST -> debounce expired -> PULSE */
    is_timer_active_ExpectAnyArgsAndReturn(true);
    is_timer_expired_ExpectAnyArgsAndReturn(true);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* PULSE -> below max, t_below inactive */
    is_timer_active_ExpectAnyArgsAndReturn(false);
    is_timer_expired_ExpectAnyArgsAndReturn(false);
    test_pack_current = DCL_MAX_CURRENT_A - 1.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* PULSE -> below max, t_below active but not expired */
    is_timer_active_ExpectAnyArgsAndReturn(true);
    is_timer_expired_ExpectAnyArgsAndReturn(false);
    is_timer_expired_ExpectAnyArgsAndReturn(false);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* PULSE -> early exit, debounce expired */
    is_timer_active_ExpectAnyArgsAndReturn(true);
    is_timer_expired_ExpectAnyArgsAndReturn(true);
    is_timer_expired_ExpectAnyArgsAndReturn(false);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_COOLDOWN_CURRENT_A, test_algos.cont_DCL);

    /* COOLDOWN -> timer not expired */
    is_timer_expired_ExpectAnyArgsAndReturn(false);
    test_pack_current = DCL_MAX_CURRENT_A - 30.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_COOLDOWN_CURRENT_A, test_algos.cont_DCL);

    /* COOLDOWN -> timer expired */
    is_timer_expired_ExpectAnyArgsAndReturn(true);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_COOLDOWN_CURRENT_A, test_algos.cont_DCL);

    /* REST -> after cooldown */
    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* -------- Full pulse -> COOLDOWN_ALWAYS -------- */

    /* REST -> under max continuous current */
    test_pack_current = DCL_MAX_CURRENT_A - 20.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* REST -> over hysteresis, debounce not active */
    is_timer_active_ExpectAnyArgsAndReturn(false);
    test_pack_current = DCL_MAX_CURRENT_A + 10.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* REST -> debounce expired -> PULSE */
    is_timer_active_ExpectAnyArgsAndReturn(true);
    is_timer_expired_ExpectAnyArgsAndReturn(true);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* PULSE -> pulse timer active but not expired */
    is_timer_expired_ExpectAnyArgsAndReturn(false);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* PULSE -> pulse timer expired */
    is_timer_expired_ExpectAnyArgsAndReturn(true);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_COOLDOWN_CURRENT_A, test_algos.cont_DCL);

    /* COOLDOWN -> timer not expired */
    is_timer_expired_ExpectAnyArgsAndReturn(false);
    test_pack_current = DCL_MAX_CURRENT_A - 20.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_COOLDOWN_CURRENT_A, test_algos.cont_DCL);

    /* COOLDOWN -> timer expired */
    is_timer_expired_ExpectAnyArgsAndReturn(true);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_COOLDOWN_CURRENT_A, test_algos.cont_DCL);

    /* REST -> after cooldown */
    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* -------- Early pulse exit -> COOLDOWN_ON_FULL_PULSE -------- */

    dcl_init(COOLDOWN_ON_FULL_PULSE);

    test_pack_current = DCL_MAX_CURRENT_A - 20.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* REST -> over hysteresis, debounce not active */
    is_timer_active_ExpectAnyArgsAndReturn(false);
    test_pack_current = DCL_MAX_CURRENT_A + 10.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* REST -> debounce expired -> PULSE */
    is_timer_active_ExpectAnyArgsAndReturn(true);
    is_timer_expired_ExpectAnyArgsAndReturn(true);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* PULSE -> below max, t_below inactive */
    is_timer_active_ExpectAnyArgsAndReturn(false);
    is_timer_expired_ExpectAnyArgsAndReturn(false);
    test_pack_current = DCL_MAX_CURRENT_A - 5.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* PULSE -> early exit, debounce expired */
    is_timer_active_ExpectAnyArgsAndReturn(true);
    is_timer_expired_ExpectAnyArgsAndReturn(true);
    is_timer_expired_ExpectAnyArgsAndReturn(false);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* REST -> after pulse early exit */
    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* -------- Full pulse -> COOLDOWN_ON_FULL_PULSE -------- */

    /* REST -> under max continuous current */
    test_pack_current = DCL_MAX_CURRENT_A - 20.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* REST -> over hysteresis, debounce not active */
    is_timer_active_ExpectAnyArgsAndReturn(false);
    test_pack_current = DCL_MAX_CURRENT_A + 10.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* REST -> debounce expired -> PULSE */
    is_timer_active_ExpectAnyArgsAndReturn(true);
    is_timer_expired_ExpectAnyArgsAndReturn(true);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* PULSE -> pulse timer active but not expired */
    is_timer_expired_ExpectAnyArgsAndReturn(false);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);

    /* PULSE -> pulse timer expired */
    is_timer_expired_ExpectAnyArgsAndReturn(true);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_COOLDOWN_CURRENT_A, test_algos.cont_DCL);

    /* COOLDOWN -> timer not expired */
    is_timer_expired_ExpectAnyArgsAndReturn(false);
    test_pack_current = DCL_MAX_CURRENT_A - 20.0f;

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_COOLDOWN_CURRENT_A, test_algos.cont_DCL);

    /* COOLDOWN -> timer expired */
    is_timer_expired_ExpectAnyArgsAndReturn(true);

    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_COOLDOWN_CURRENT_A, test_algos.cont_DCL);

    /* REST -> after cooldown */
    dcl_calc_cont_limit(test_pack_current, &test_algos);
    TEST_ASSERT_EQUAL_FLOAT(DCL_MAX_PULSE_CURRENT_A, test_algos.cont_DCL);
}

/* -------------------------------------------------
 * Test Runner
 * ------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_inst_dcl_temperature_and_ocv_regions);
    RUN_TEST(test_cont_dcl_follows_inst_limit_when_pulse_not_allowed);
    RUN_TEST(test_cont_dcl_pulse_state_transitions);

    return UNITY_END();
}
