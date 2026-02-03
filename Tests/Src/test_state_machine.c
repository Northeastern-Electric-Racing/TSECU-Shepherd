
#include "test_state_machine.h"
#include  "stm32xx_hal.h"
#include <stdlib.h>

SPI_HandleTypeDef hspi2;

void setUp(void)
{
}

void tearDown(void)
{
}

// A simple random test
void test_sm_balance_cells(void)
{
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_sm_balance_cells);
	return UNITY_END();
}