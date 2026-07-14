#include "unity.h"

#include "state_machine.h"
#include "datastructs.h"
#include "mock_u_tx_mutex.h"
#include "mock_timer.h"
#include "mock_can_messages_tx.h"

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

void setUp(void)
{
    analyzer.min_ocv.val = 2.6;
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

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_initial_state);
	RUN_TEST(test_eval_table);

	return UNITY_END();
}
