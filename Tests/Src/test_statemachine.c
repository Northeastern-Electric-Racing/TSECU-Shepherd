#include "unity.h"

#include "state_machine.h"
#include "datastructs.h"
#include "mock_u_tx_mutex.h"
#include "mock_timer.h"
#include "mock_can_messages_tx.h"
#include "mock_compute.h"

state_machine_t state_machine;
analyzer_t analyzer;
hv_plate_t hv_plate;
acc_data_t acc_data;
bms_algos_t bms_algos;
sanitizer_t sanitizer;
peripherals_t peripherals;

state_machine_args_t args = { .state_machine = &state_machine,
			      .analyzer = &analyzer,
			      .hv_plate = &hv_plate,
			      .acc_data = &acc_data,
			      .bms_algos = &bms_algos,
			      .sanitizer = &sanitizer,
			      .peripherals = &peripherals };

void setUp(void)
{
    analyzer.min_ocv.val = 2.6;
	analyzer.max_ocv.val = 4.0f;
	analyzer.max_voltage.val = 4.0f;
	analyzer.delta_voltage = MAX_DELTA_V + 0.01f;
	peripherals.shutdown_active = true;
    mutex_get_IgnoreAndReturn(0);
    mutex_put_IgnoreAndReturn(0);
    cancel_timer_Expect(&state_machine.charger_message_timer);
	init_boot(&args);
}

void tearDown(void)
{
}

void test_initial_state(void)
{
	mutex_get_IgnoreAndReturn(0);
	mutex_put_IgnoreAndReturn(0);

	// Check that the initial state is BOOT
	TEST_ASSERT_EQUAL(BOOT, state_machine.bms_state);
}

void test_eval_table(void)
{
	// Set up test data for the fault evaluation
	hv_plate.pack_current =
		100.0f; // Set a high current to trigger the fault
	bms_algos.cont_DCL = 50.0f; // Set a lower continuous discharge limit

	/* will ignore any logic having to do with debouncing and will fault the coniditon if present*/
	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(true);
	cancel_timer_Ignore();
	start_timer_Ignore();
	send_bms_fault_timers_IgnoreAndReturn(0);
	send_bms_critically_faulted_IgnoreAndReturn(0);

	// Update the evaluation table with the test data
	sm_fault_return(&args);

    // Check that the discharge limit enforcement fault is active
	TEST_ASSERT_TRUE(get_fault(DISCHARGE_LIMIT_ENFORCEMENT_FAULT));

    hv_plate.pack_current =
		40.0f; // Make current obey DCL

    sm_fault_return(&args);

    // Check that the discharge limit enforcement fault is active
	TEST_ASSERT_FALSE(get_fault(DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
}

void test_long_charge_cycle(void)
{
	state_machine.charging_stage = LONG_CHARGE_UP;
	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					true);
	start_timer_Expect(&state_machine.charging_stage_timer, 60U * 1000U);

	TEST_ASSERT_FALSE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(LONG_SETTLE, state_machine.charging_stage);

	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					true);
	start_timer_Expect(&state_machine.charging_stage_timer,
			   15U * 60U * 1000U);

	TEST_ASSERT_TRUE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(LONG_CHARGE_UP, state_machine.charging_stage);
}

void test_short_charge_cycle(void)
{
	state_machine.charging_stage = LONG_CHARGE_UP;
	analyzer.max_voltage.val = MAX_CHARGE_VOLT;
	start_timer_Expect(&state_machine.charging_stage_timer, 60U * 1000U);

	TEST_ASSERT_FALSE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(SHORT_SETTLE, state_machine.charging_stage);

	analyzer.max_voltage.val = 4.0f;
	analyzer.max_ocv.val = MAX_CHARGE_VOLT - 0.01f;
	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					true);
	start_timer_Expect(&state_machine.charging_stage_timer, 20U * 1000U);

	TEST_ASSERT_TRUE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(SHORT_CHARGE_UP, state_machine.charging_stage);
}

void test_charge_done(void)
{
	state_machine.charging_stage = SHORT_SETTLE;
	analyzer.max_ocv.val = MAX_CHARGE_VOLT;
	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					true);

	TEST_ASSERT_FALSE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(DONE, state_machine.charging_stage);
}

void test_charge_fault(void)
{
	state_machine.bms_state = CHARGING;
	state_machine.charging_stage = LONG_CHARGE_UP;
	analyzer.max_voltage.val = MAX_CHARGE_VOLT_FLT;

	TEST_ASSERT_FALSE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(FAULT, state_machine.charging_stage);
	TEST_ASSERT_EQUAL(CHARGING, state_machine.bms_state);
}

void test_handle_faulted_sends_zero_current_limits(void)
{
	compute_set_fault_Expect(true);
	send_max_dc_current_command_ExpectAndReturn(0.0f, 0U);
	send_max_dc_brake_current_command_ExpectAndReturn(0.0f, 0U);

	handle_faulted(&args);
}

void test_handle_charging_sends_zero_current_limits(void)
{
	state_machine.bms_state = CHARGING;
	state_machine.charging_stage = LONG_SETTLE;

	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					false);
	send_bms_charge_message_send_ExpectAndReturn(0.0f, 0.0f, 0xFFU, 0U);
	send_max_dc_current_command_ExpectAndReturn(0.0f, 0U);
	send_max_dc_brake_current_command_ExpectAndReturn(0.0f, 0U);

	handle_charging(&args);
}

void test_balancing(void)
{
	// Balancing allowed
	state_machine.charging_stage = LONG_CHARGE_UP;
	TEST_ASSERT_TRUE(sm_balancing_check(&args));

	// Long settle
	state_machine.charging_stage = LONG_SETTLE;
	TEST_ASSERT_FALSE(sm_balancing_check(&args));

	// Short settle
	state_machine.charging_stage = SHORT_SETTLE;
	TEST_ASSERT_FALSE(sm_balancing_check(&args));

	// Low voltage
	state_machine.charging_stage = LONG_CHARGE_UP;
	analyzer.max_voltage.val = BAL_MIN_V;
	TEST_ASSERT_FALSE(sm_balancing_check(&args));
	analyzer.max_voltage.val = 4.0f;

	// Low voltage delta
	analyzer.delta_voltage = MAX_DELTA_V;
	TEST_ASSERT_FALSE(sm_balancing_check(&args));
	analyzer.delta_voltage = MAX_DELTA_V + 0.01f;

	// Shutdown inactive
	peripherals.shutdown_active = false;
	TEST_ASSERT_FALSE(sm_balancing_check(&args));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_initial_state);
	RUN_TEST(test_eval_table);
	RUN_TEST(test_long_charge_cycle);
	RUN_TEST(test_short_charge_cycle);
	RUN_TEST(test_charge_done);
	RUN_TEST(test_charge_fault);
	RUN_TEST(test_handle_faulted_sends_zero_current_limits);
	RUN_TEST(test_handle_charging_sends_zero_current_limits);
	RUN_TEST(test_balancing);

	return UNITY_END();
}
