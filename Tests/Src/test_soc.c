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

void setUp(void) {}

void tearDown(void) {}

/* -------------------------------------------------
 * Test: SOC initializes from OCV
 * ------------------------------------------------- */

void test_update_soc_initializes_from_ocv(void)
{
	analyzer.min_ocv.val = 3.8f;

	update_soc(&analyzer, &hv_plate);

	TEST_ASSERT_EQUAL_FLOAT(0.615728f, analyzer.soc);
}

/* -------------------------------------------------
 * Test: SOC decreases during discharge
 * ------------------------------------------------- */

void test_update_soc_coulomb_discharge(void)
{
	hv_plate.pack_current = 100.0f;

	test_tx_ticks += 10U;

	update_soc(&analyzer, &hv_plate);

	TEST_ASSERT_EQUAL_FLOAT(0.615172f, analyzer.soc);
}

/* -------------------------------------------------
 * Test: SOC increases during charge
 * ------------------------------------------------- */

void test_update_soc_coulomb_charge(void)
{
	update_soc(&analyzer, &hv_plate); /* initialize */

	hv_plate.pack_current = -10.0f;

	test_tx_ticks += 10U;

	update_soc(&analyzer, &hv_plate);

	TEST_ASSERT_EQUAL_FLOAT(0.615228f, analyzer.soc);
}

/* -------------------------------------------------
 * Test Runner
 * ------------------------------------------------- */

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_update_soc_initializes_from_ocv);
	RUN_TEST(test_update_soc_coulomb_discharge);
	RUN_TEST(test_update_soc_coulomb_charge);

	return UNITY_END();
}