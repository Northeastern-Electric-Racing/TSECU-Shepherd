#include "unity.h"

#include "state_machine.h"
#include "datastructs.h"
#include "mock_u_tx_mutex.h"
#include "mock_timer.h"
#include "mock_can_messages_tx.h"
#include "mock_charging.h"
#include "mock_compute.h"
#include <string.h>

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
	memset(&state_machine, 0, sizeof(state_machine));
	memset(&analyzer, 0, sizeof(analyzer));
	memset(&hv_plate, 0, sizeof(hv_plate));
	memset(&acc_data, 0, sizeof(acc_data));
	memset(&bms_algos, 0, sizeof(bms_algos));
	memset(&sanitizer, 0, sizeof(sanitizer));
	memset(&peripherals, 0, sizeof(peripherals));

	analyzer.min_ocv.val = 2.6f;
	analyzer.max_ocv.val = 4.0f;
	analyzer.max_voltage.val = 4.0f;
	peripherals.shutdown_active = true;
	mutex_get_IgnoreAndReturn(0);
	mutex_put_IgnoreAndReturn(0);
	cancel_timer_Expect(&state_machine.balancing_active_timer);
	cancel_timer_Expect(&state_machine.balancing_cooldown_timer);
	cancel_timer_Expect(&state_machine.charger_message_timer);
	init_boot(&args);
}

void tearDown(void)
{
}

void test_initial_state(void)
{
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
	TEST_ASSERT_EQUAL(SETTLE, state_machine.charging_stage);

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
	cancel_timer_Expect(&state_machine.charging_stage_timer);

	TEST_ASSERT_TRUE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(SHORT_CHARGE_UP, state_machine.charging_stage);
	TEST_ASSERT_EQUAL_FLOAT(CHARGING_CURRENT,
				state_machine.charge_current_request);

	state_machine.charge_current_request = 1.0f;
	is_timer_active_ExpectAndReturn(&state_machine.charging_stage_timer,
				       false);
	start_timer_Expect(&state_machine.charging_stage_timer, 60U * 1000U);

	TEST_ASSERT_FALSE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(SETTLE, state_machine.charging_stage);
}

void test_short_charge_current_step_delay(void)
{
	const float pack_voltage =
		MAX_CHARGE_VOLT * (NUM_CELLS_PER_CHIP * 2) * NUM_SEGMENTS;

	state_machine.bms_state = CHARGING;
	state_machine.charging_stage = SHORT_CHARGE_UP;
	state_machine.charge_current_request = CHARGING_CURRENT;
	analyzer.max_ocv.val = MAX_CHARGE_VOLT - 0.01f;
	analyzer.max_voltage.val = MAX_CHARGE_VOLT;

	// The first step from 5 A to 4 A is immediate because the step timer is inactive.
	is_timer_active_ExpectAndReturn(&state_machine.charging_stage_timer,
				       false);
	is_timer_expired_ExpectAndReturn(&state_machine.charger_message_timer,
					false);
	is_timer_active_ExpectAndReturn(&state_machine.charger_message_timer,
				       false);
	is_timer_active_ExpectAndReturn(&state_machine.charging_stage_timer,
				       false);
	start_timer_Expect(&state_machine.charging_stage_timer, 5000U);
	send_bms_charge_message_send_ExpectAndReturn(pack_voltage, 4.0f, 0x0U,
						     0U);
	start_timer_Expect(&state_machine.charger_message_timer, 1000U);
	send_max_dc_current_command_ExpectAndReturn(0.0f, 0U);
	send_max_dc_brake_current_command_ExpectAndReturn(0.0f, 0U);

	handle_charging(&args);
	TEST_ASSERT_EQUAL_FLOAT(4.0f,
				state_machine.charge_current_request);

	// Before five seconds expires, the requested current remains at 4 A.
	is_timer_active_ExpectAndReturn(&state_machine.charging_stage_timer,
				       true);
	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					false);
	is_timer_expired_ExpectAndReturn(&state_machine.charger_message_timer,
					true);
	is_timer_active_ExpectAndReturn(&state_machine.charging_stage_timer,
				       true);
	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					false);
	send_bms_charge_message_send_ExpectAndReturn(pack_voltage, 4.0f, 0x0U,
						     0U);
	start_timer_Expect(&state_machine.charger_message_timer, 1000U);
	send_max_dc_current_command_ExpectAndReturn(0.0f, 0U);
	send_max_dc_brake_current_command_ExpectAndReturn(0.0f, 0U);

	handle_charging(&args);
	TEST_ASSERT_EQUAL_FLOAT(4.0f,
				state_machine.charge_current_request);

	// Once five seconds expires, take the next 1 A step and restart the timer.
	is_timer_active_ExpectAndReturn(&state_machine.charging_stage_timer,
				       true);
	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					true);
	is_timer_expired_ExpectAndReturn(&state_machine.charger_message_timer,
					false);
	is_timer_active_ExpectAndReturn(&state_machine.charger_message_timer,
				       true);
	is_timer_active_ExpectAndReturn(&state_machine.charging_stage_timer,
				       true);
	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					true);
	start_timer_Expect(&state_machine.charging_stage_timer, 5000U);
	send_bms_charge_message_send_ExpectAndReturn(pack_voltage, 3.0f, 0x0U,
						     0U);
	start_timer_Expect(&state_machine.charger_message_timer, 1000U);
	send_max_dc_current_command_ExpectAndReturn(0.0f, 0U);
	send_max_dc_brake_current_command_ExpectAndReturn(0.0f, 0U);

	handle_charging(&args);
	TEST_ASSERT_EQUAL_FLOAT(3.0f,
				state_machine.charge_current_request);
}

void test_charge_done(void)
{
	state_machine.charging_stage = SETTLE;
	analyzer.max_ocv.val = MAX_CHARGE_VOLT;
	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					true);

	TEST_ASSERT_FALSE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(DONE, state_machine.charging_stage);

	// DONE is terminal even if imbalance is detected later.
	TEST_ASSERT_FALSE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(DONE, state_machine.charging_stage);
}

void test_balance_only_at_charge_limit(void)
{
	// Start in the long phase to verify BALANCE_ONLY advances to short settle.
	state_machine.charging_stage = SETTLE;
	state_machine.charge_current_request = CHARGING_CURRENT;
	analyzer.max_ocv.val = MAX_CHARGE_VOLT;
	analyzer.max_voltage.val = MAX_CHARGE_VOLT - 0.01f;

	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					true);
	handle_balance_cells_ExpectAndReturn(&analyzer, &acc_data, true);
	cancel_timer_Expect(&state_machine.balancing_active_timer);
	cancel_timer_Expect(&state_machine.balancing_cooldown_timer);
	start_timer_Expect(&state_machine.balancing_active_timer, 60U * 1000U);
	cancel_timer_Expect(&state_machine.charging_stage_timer);

	TEST_ASSERT_FALSE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(BALANCE_ONLY, state_machine.charging_stage);
	TEST_ASSERT_TRUE(state_machine.balancing_active);
	TEST_ASSERT_EQUAL_FLOAT(0.0f,
				state_machine.charge_current_request);

	// Finish the active balancing window and begin cooldown.
	is_timer_expired_ExpectAndReturn(&state_machine.balancing_active_timer,
					true);
	cancel_timer_Expect(&state_machine.balancing_active_timer);
	start_timer_Expect(&state_machine.balancing_cooldown_timer,
			  3U * 60U * 1000U);

	TEST_ASSERT_FALSE(sm_charging_check(&args));
	TEST_ASSERT_FALSE(state_machine.balancing_active);
	TEST_ASSERT_EQUAL(BALANCE_ONLY, state_machine.charging_stage);

	// Finish cooldown and enter the shared settle stage.
	is_timer_expired_ExpectAndReturn(&state_machine.balancing_active_timer,
					false);
	is_timer_expired_ExpectAndReturn(&state_machine.balancing_cooldown_timer,
					true);
	cancel_timer_Expect(&state_machine.balancing_cooldown_timer);
	start_timer_Expect(&state_machine.charging_stage_timer, 60U * 1000U);

	TEST_ASSERT_FALSE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(SETTLE, state_machine.charging_stage);

	// BALANCE_ONLY changed the resume target from long to short charging.
	analyzer.max_ocv.val = MAX_CHARGE_VOLT - 0.01f;
	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					true);
	cancel_timer_Expect(&state_machine.charging_stage_timer);

	TEST_ASSERT_TRUE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(SHORT_CHARGE_UP, state_machine.charging_stage);
}

void test_balance_and_charge_uses_instantaneous_limit(void)
{
	state_machine.charging_stage = BALANCE_AND_CHARGE_UP;
	state_machine.charge_current_request = CHARGING_CURRENT;
	state_machine.balancing_active = true;
	analyzer.max_ocv.val = MAX_CHARGE_VOLT;
	analyzer.max_voltage.val = MAX_CHARGE_VOLT - 0.01f;

	// A stored OCV at the target does not stop active charging by itself.
	is_timer_expired_ExpectAndReturn(&state_machine.balancing_active_timer,
					false);
	is_timer_expired_ExpectAndReturn(&state_machine.balancing_cooldown_timer,
					false);
	is_timer_active_ExpectAndReturn(&state_machine.balancing_active_timer,
				       true);

	TEST_ASSERT_TRUE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(BALANCE_AND_CHARGE_UP,
			  state_machine.charging_stage);

	// The live voltage reaching the target changes to balance-only.
	analyzer.max_voltage.val = MAX_CHARGE_VOLT;
	cancel_timer_Expect(&state_machine.charging_stage_timer);

	TEST_ASSERT_FALSE(sm_charging_check(&args));
	TEST_ASSERT_EQUAL(BALANCE_ONLY, state_machine.charging_stage);
	TEST_ASSERT_EQUAL_FLOAT(0.0f,
				state_machine.charge_current_request);
}

void test_charge_fault(void)
{
	state_machine.bms_state = CHARGING;
	state_machine.charging_stage = LONG_CHARGE_UP;
	analyzer.max_voltage.val = MAX_CHARGE_VOLT_FLT;
	cancel_timer_Expect(&state_machine.balancing_active_timer);
	cancel_timer_Expect(&state_machine.balancing_cooldown_timer);

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
	mock_can_messages_tx_Verify();
}

void test_handle_charging_sends_zero_current_limits(void)
{
	state_machine.bms_state = CHARGING;
	state_machine.charging_stage = SETTLE;

	is_timer_expired_ExpectAndReturn(&state_machine.charging_stage_timer,
					false);
	send_bms_charge_message_send_ExpectAndReturn(0.0f, 0.0f, 0xFFU, 0U);
	send_max_dc_current_command_ExpectAndReturn(0.0f, 0U);
	send_max_dc_brake_current_command_ExpectAndReturn(0.0f, 0U);

	handle_charging(&args);
	mock_can_messages_tx_Verify();
}

void test_balancing(void)
{
	// Balancing allowed
	state_machine.charging_stage = LONG_CHARGE_UP;
	TEST_ASSERT_TRUE(sm_balancing_check(&args));

	// The settle stage can start a new balancing cycle.
	state_machine.charging_stage = SETTLE;
	TEST_ASSERT_TRUE(sm_balancing_check(&args));

	// Charging-stage fault
	state_machine.charging_stage = FAULT;
	TEST_ASSERT_FALSE(sm_balancing_check(&args));

	// Low voltage
	state_machine.charging_stage = LONG_CHARGE_UP;
	analyzer.max_ocv.val = BAL_MIN_V - 0.01f;
	TEST_ASSERT_FALSE(sm_balancing_check(&args));
	analyzer.max_ocv.val = BAL_MIN_V;

	// Balancing remains available above the charge limit until hard fault.
	analyzer.max_voltage.val = MAX_CHARGE_VOLT + 0.01f;
	TEST_ASSERT_TRUE(sm_balancing_check(&args));
	analyzer.max_voltage.val = MAX_CHARGE_VOLT_FLT;
	TEST_ASSERT_FALSE(sm_balancing_check(&args));
	analyzer.max_voltage.val = BAL_MIN_V;

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
	RUN_TEST(test_short_charge_current_step_delay);
	RUN_TEST(test_charge_done);
	RUN_TEST(test_balance_only_at_charge_limit);
	RUN_TEST(test_balance_and_charge_uses_instantaneous_limit);
	RUN_TEST(test_charge_fault);
	RUN_TEST(test_handle_faulted_sends_zero_current_limits);
	RUN_TEST(test_handle_charging_sends_zero_current_limits);
	RUN_TEST(test_balancing);

	return UNITY_END();
}
