#include "unity.h"

#include "state_machine.h"
#include "datastructs.h"
#include <string.h>
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

state_machine_args_t args = { .state_machine = &state_machine,
			      .analyzer = &analyzer,
			      .hv_plate = &hv_plate,
			      .acc_data = &acc_data,
			      .bms_algos = &bms_algos,
			      .sanitizer = &sanitizer };

static void set_safe_defaults(void)
{
	memset(&state_machine, 0, sizeof(state_machine));
	memset(&analyzer, 0, sizeof(analyzer));
	memset(&hv_plate, 0, sizeof(hv_plate));
	memset(&acc_data, 0, sizeof(acc_data));
	memset(&bms_algos, 0, sizeof(bms_algos));
	memset(&sanitizer, 0, sizeof(sanitizer));

	state_machine.bms_state = BOOT;

	bms_algos.cont_DCL = 200.0f;
	bms_algos.cont_CCL = 200.0f;

	analyzer.min_ocv.val = 3.6f;
	analyzer.max_ocv.val = 3.7f;
	analyzer.max_voltage.val = 3.7f;
	analyzer.max_chiptemp.val = 20.0f;

	sanitizer.max_sanitized_temp.val = 25.0f;

	hv_plate.pack_current = 0.0f;

	state_machine.segment_comms_fault_flag = false;
	state_machine.hv_plate_comms_fault_flag = false;
}

static void clear_all_faults(void)
{
	is_timer_active_IgnoreAndReturn(false);
	is_timer_expired_IgnoreAndReturn(false);
	sm_fault_return(&args);
	is_timer_active_StopIgnore();
	is_timer_expired_StopIgnore();
}

static void allow_fault_eval_side_effects(void)
{
	start_timer_Ignore();
	cancel_timer_Ignore();
}

void setUp(void)
{
	mock_u_tx_mutex_Init();
	mock_timer_Init();
	mock_can_messages_tx_Init();
	mock_compute_Init();

	mutex_get_IgnoreAndReturn(0);
	mutex_put_IgnoreAndReturn(0);

	set_safe_defaults();
	init_boot(&args);
	clear_all_faults();
}

void tearDown(void)
{
	mock_compute_Verify();
	mock_can_messages_tx_Verify();
	mock_timer_Verify();
	mock_u_tx_mutex_Verify();

	mock_compute_Destroy();
	mock_can_messages_tx_Destroy();
	mock_timer_Destroy();
	mock_u_tx_mutex_Destroy();
}

void test_boot_initialization_populates_critical_severity_mask(void)
{
	allow_fault_eval_side_effects();
	send_bms_fault_timers_IgnoreAndReturn(0);

	state_machine.segment_comms_fault_flag = true;
	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(true);
	sm_fault_return(&args);
	TEST_ASSERT_FALSE(are_critical_faults_active());

	state_machine.segment_comms_fault_flag = false;
	hv_plate.pack_current = 300.0f;
	bms_algos.cont_DCL = 10.0f;
	sm_fault_return(&args);
	TEST_ASSERT_TRUE(are_critical_faults_active());
}

void test_boot_initialization_has_no_fault_flags_by_default(void)
{
	int i;
	for (i = 0; i < NUM_FAULTS; i++) {
		TEST_ASSERT_FALSE(get_fault((fault_code_t)i));
	}
}

void test_handle_boot_transitions_to_ready(void)
{
	compute_set_fault_Expect(false);
	handle_boot(&args);
	TEST_ASSERT_EQUAL(READY, get_current_state(&state_machine));
}

void test_request_transition_to_current_state_is_noop(void)
{
	state_machine.bms_state = READY;
	handle_boot(&args);
	TEST_ASSERT_EQUAL(READY, get_current_state(&state_machine));
}

void test_invalid_transition_is_rejected(void)
{
	state_machine.bms_state = READY;
	handle_faulted(&args);
	TEST_ASSERT_EQUAL(READY, get_current_state(&state_machine));
}

void test_valid_transition_runs_destination_init(void)
{
	start_timer_ExpectAnyArgs();
	charger_message_recieved(&args);
	TEST_ASSERT_EQUAL(CHARGING, get_current_state(&state_machine));
}

void test_critical_fault_forces_faulted_state_in_sm_handle_state(void)
{
	allow_fault_eval_side_effects();
	send_bms_fault_timers_IgnoreAndReturn(0);

	state_machine.bms_state = READY;
	hv_plate.pack_current = 300.0f;
	bms_algos.cont_DCL = 10.0f;

	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(true);

	send_max_dc_current_command_ExpectAnyArgsAndReturn(0);
	send_max_dc_brake_current_command_ExpectAnyArgsAndReturn(0);
	send_bms_charge_message_send_ExpectAnyArgsAndReturn(0);

	sm_handle_state(&args);
	TEST_ASSERT_EQUAL(FAULTED, get_current_state(&state_machine));
}

void test_noncritical_fault_does_not_force_faulted_state(void)
{
	allow_fault_eval_side_effects();
	send_bms_fault_timers_IgnoreAndReturn(0);

	state_machine.bms_state = READY;
	state_machine.segment_comms_fault_flag = true;

	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(true);

	sm_handle_state(&args);
	TEST_ASSERT_EQUAL(READY, get_current_state(&state_machine));
	TEST_ASSERT_TRUE(get_fault(SEGMENT_COMMS_FAULT));
}

void test_faulted_handler_exits_to_boot_only_without_critical_faults(void)
{
	allow_fault_eval_side_effects();
	send_bms_fault_timers_IgnoreAndReturn(0);

	state_machine.bms_state = FAULTED;
	hv_plate.pack_current = 300.0f;
	bms_algos.cont_DCL = 10.0f;

	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(true);
	sm_fault_return(&args);
	handle_faulted(&args);
	TEST_ASSERT_EQUAL(FAULTED, get_current_state(&state_machine));

	set_safe_defaults();
	init_boot(&args);
	clear_all_faults();
	state_machine.bms_state = FAULTED;
	handle_faulted(&args);
	TEST_ASSERT_EQUAL(BOOT, get_current_state(&state_machine));
}

void test_charger_message_received_requests_charging_transition(void)
{
	state_machine.bms_state = READY;
	start_timer_ExpectAnyArgs();
	charger_message_recieved(&args);
	TEST_ASSERT_EQUAL(CHARGING, get_current_state(&state_machine));
}

void test_fault_eval_nop_second_comparator_faults_when_first_is_true(void)
{
	fault_eval_t item = { 0 };
	item.data_1 = 10.0f;
	item.lim_1 = 5.0f;
	item.optype_1 = GT;
	item.optype_2 = NOP;

	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(true);
	send_bms_fault_timers_ExpectAnyArgsAndReturn(0);

	TEST_ASSERT_TRUE(sm_fault_eval(&item, DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
}

void test_fault_eval_two_comparators_require_both_conditions(void)
{
	fault_eval_t item = { 0 };
	item.data_1 = 10.0f;
	item.lim_1 = 5.0f;
	item.optype_1 = GT;
	item.data_2 = 10.0f;
	item.lim_2 = 5.0f;
	item.optype_2 = LT;

	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(true);
	cancel_timer_ExpectAnyArgs();
	send_bms_fault_timers_ExpectAnyArgsAndReturn(0);

	TEST_ASSERT_FALSE(sm_fault_eval(&item, DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
}

void test_fault_eval_inactive_timer_and_no_fault_returns_false(void)
{
	fault_eval_t item = { 0 };
	item.data_1 = 3.0f;
	item.lim_1 = 5.0f;
	item.optype_1 = GT;
	item.optype_2 = NOP;

	is_timer_active_IgnoreAndReturn(false);

	TEST_ASSERT_FALSE(sm_fault_eval(&item, DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
}

void test_fault_eval_fault_appearance_starts_timer_and_reports_started(void)
{
	fault_eval_t item = { 0 };
	item.data_1 = 9.0f;
	item.lim_1 = 5.0f;
	item.optype_1 = GT;
	item.optype_2 = NOP;
	item.timeout = 1234;

	is_timer_active_IgnoreAndReturn(false);
	start_timer_ExpectAnyArgs();
	send_bms_fault_timers_ExpectAnyArgsAndReturn(0);

	TEST_ASSERT_FALSE(sm_fault_eval(&item, DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
}

void test_fault_eval_active_timer_with_cleared_condition_cancels_timer(void)
{
	fault_eval_t item = { 0 };
	item.data_1 = 1.0f;
	item.lim_1 = 5.0f;
	item.optype_1 = GT;
	item.optype_2 = NOP;

	is_timer_active_IgnoreAndReturn(true);
	cancel_timer_ExpectAnyArgs();
	send_bms_fault_timers_ExpectAnyArgsAndReturn(0);

	TEST_ASSERT_FALSE(sm_fault_eval(&item, DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
}

void test_fault_eval_active_timer_not_expired_does_not_fault(void)
{
	fault_eval_t item = { 0 };
	item.data_1 = 10.0f;
	item.lim_1 = 5.0f;
	item.optype_1 = GT;
	item.optype_2 = NOP;

	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(false);

	TEST_ASSERT_FALSE(sm_fault_eval(&item, DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
}

void test_fault_eval_active_timer_expired_returns_true(void)
{
	fault_eval_t item = { 0 };
	item.data_1 = 10.0f;
	item.lim_1 = 5.0f;
	item.optype_1 = GT;
	item.optype_2 = NOP;

	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(true);
	send_bms_fault_timers_ExpectAnyArgsAndReturn(0);

	TEST_ASSERT_TRUE(sm_fault_eval(&item, DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
}

void test_fault_flags_set_and_clear_independently(void)
{
	allow_fault_eval_side_effects();
	send_bms_fault_timers_IgnoreAndReturn(0);

	hv_plate.pack_current = 60.0f;
	bms_algos.cont_DCL = 50.0f;
	bms_algos.cont_CCL = 100.0f;

	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(true);
	sm_fault_return(&args);
	TEST_ASSERT_TRUE(get_fault(DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
	TEST_ASSERT_FALSE(get_fault(CHARGE_LIMIT_ENFORCEMENT_FAULT));

	hv_plate.pack_current = 40.0f;
	bms_algos.cont_DCL = 50.0f;
	bms_algos.cont_CCL = 30.0f;
	sm_fault_return(&args);
	TEST_ASSERT_FALSE(get_fault(DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
	TEST_ASSERT_TRUE(get_fault(CHARGE_LIMIT_ENFORCEMENT_FAULT));
}

void test_are_critical_faults_active_true_only_for_critical_faults(void)
{
	allow_fault_eval_side_effects();
	send_bms_fault_timers_IgnoreAndReturn(0);

	state_machine.segment_comms_fault_flag = true;
	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(true);
	sm_fault_return(&args);
	TEST_ASSERT_TRUE(get_fault(SEGMENT_COMMS_FAULT));
	TEST_ASSERT_FALSE(are_critical_faults_active());

	state_machine.segment_comms_fault_flag = false;
	hv_plate.pack_current = 300.0f;
	bms_algos.cont_DCL = 10.0f;
	sm_fault_return(&args);
	TEST_ASSERT_TRUE(get_fault(DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
	TEST_ASSERT_TRUE(are_critical_faults_active());
}

void test_get_fault_reflects_set_and_clear_for_specific_fault(void)
{
	allow_fault_eval_side_effects();
	send_bms_fault_timers_IgnoreAndReturn(0);

	hv_plate.pack_current = 100.0f;
	bms_algos.cont_DCL = 50.0f;

	is_timer_active_IgnoreAndReturn(true);
	is_timer_expired_IgnoreAndReturn(true);
	sm_fault_return(&args);
	TEST_ASSERT_TRUE(get_fault(DISCHARGE_LIMIT_ENFORCEMENT_FAULT));

	hv_plate.pack_current = 10.0f;
	sm_fault_return(&args);
	TEST_ASSERT_FALSE(get_fault(DISCHARGE_LIMIT_ENFORCEMENT_FAULT));
}

void test_initial_state(void)
{

	// Check that the initial state is BOOT
	TEST_ASSERT_EQUAL(BOOT, get_current_state(&state_machine));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_boot_initialization_populates_critical_severity_mask);
	RUN_TEST(test_boot_initialization_has_no_fault_flags_by_default);
	RUN_TEST(test_handle_boot_transitions_to_ready);
	RUN_TEST(test_request_transition_to_current_state_is_noop);
	RUN_TEST(test_invalid_transition_is_rejected);
	RUN_TEST(test_valid_transition_runs_destination_init);
	RUN_TEST(test_critical_fault_forces_faulted_state_in_sm_handle_state);
	RUN_TEST(test_noncritical_fault_does_not_force_faulted_state);
	RUN_TEST(test_faulted_handler_exits_to_boot_only_without_critical_faults);
	RUN_TEST(test_charger_message_received_requests_charging_transition);
	RUN_TEST(test_fault_eval_nop_second_comparator_faults_when_first_is_true);
	RUN_TEST(test_fault_eval_two_comparators_require_both_conditions);
	RUN_TEST(test_fault_eval_inactive_timer_and_no_fault_returns_false);
	RUN_TEST(test_fault_eval_fault_appearance_starts_timer_and_reports_started);
	RUN_TEST(test_fault_eval_active_timer_with_cleared_condition_cancels_timer);
	RUN_TEST(test_fault_eval_active_timer_not_expired_does_not_fault);
	RUN_TEST(test_fault_eval_active_timer_expired_returns_true);
	RUN_TEST(test_fault_flags_set_and_clear_independently);
	RUN_TEST(test_are_critical_faults_active_true_only_for_critical_faults);
	RUN_TEST(test_get_fault_reflects_set_and_clear_for_specific_fault);

	RUN_TEST(test_initial_state);

	return UNITY_END();
}