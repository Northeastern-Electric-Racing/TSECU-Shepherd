#include "unity.h"

#include "datastructs.h"
#include "soc.h"
#include "u_tx_general.h"

/* -------------------------------------------------
 * Stubs
 * ------------------------------------------------- */

static uint32_t test_tx_ticks = 0U;

/* Stub ThreadX time function */
ULONG _tx_time_get(VOID)
{
	return test_tx_ticks;
}

/* -------------------------------------------------
 * Test Variables
 * ------------------------------------------------- */

static analyzer_t analyzer;
static hv_plate_t hv_plate;

/* -------------------------------------------------
 * Setup / Teardown
 * ------------------------------------------------- */

void setUp(void)
{
	test_tx_ticks = 0U;

	/* Reset inputs */
	analyzer.min_ocv.val = 0.0f;
	analyzer.soc = 0.0f;
	hv_plate.pack_current = 0.0f;

	/* Ensure SoC starts from OCV each test */
	soc_init();
}

void tearDown(void) {}

/* -------------------------------------------------
 * Test: SOC initializes from OCV
 * ------------------------------------------------- */

void test_soc_initializes_from_ocv(void)
{
	analyzer.min_ocv.val = 3.8f;

	soc_handle_state(&analyzer, &hv_plate);

	TEST_ASSERT_EQUAL_FLOAT(0.615728f, analyzer.soc);
}

/* -------------------------------------------------
 * Test: SOC decreases during discharge
 * ------------------------------------------------- */

void test_soc_coulomb_discharge(void)
{
	analyzer.min_ocv.val = 3.8f;

	/* Initial OCV init */
	soc_handle_state(&analyzer, &hv_plate);

	hv_plate.pack_current = 100.0f;
	test_tx_ticks += 100U;

	soc_handle_state(&analyzer, &hv_plate);

	TEST_ASSERT_EQUAL_FLOAT(0.615172f, analyzer.soc);
}

/* -------------------------------------------------
 * Test: SOC increases during charge
 * ------------------------------------------------- */

void test_soc_coulomb_charge(void)
{
	analyzer.min_ocv.val = 3.8f;

	/* Initial OCV init */
	soc_handle_state(&analyzer, &hv_plate);

	hv_plate.pack_current = -10.0f;
	test_tx_ticks += 100U;

	soc_handle_state(&analyzer, &hv_plate);

	TEST_ASSERT_EQUAL_FLOAT(0.615784f, analyzer.soc);
}

/* -------------------------------------------------
 * Test: Invalid OCV does not initialize, then recovers
 * ------------------------------------------------- */

void test_soc_invalid_ocv_then_valid(void)
{
	/* Invalid OCV */
	analyzer.min_ocv.val = 0.5f;

	soc_handle_state(&analyzer, &hv_plate);

	/* Should not initialize */
	TEST_ASSERT_EQUAL_FLOAT(0.0f, analyzer.soc);

	/* Now valid OCV */
	analyzer.min_ocv.val = 3.8f;

	soc_handle_state(&analyzer, &hv_plate);

	TEST_ASSERT_EQUAL_FLOAT(0.615728f, analyzer.soc);
}

/* -------------------------------------------------
 * Test Runner
 * ------------------------------------------------- */

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_soc_initializes_from_ocv);
	RUN_TEST(test_soc_coulomb_discharge);
	RUN_TEST(test_soc_coulomb_charge);
	RUN_TEST(test_soc_invalid_ocv_then_valid);

	return UNITY_END();
}