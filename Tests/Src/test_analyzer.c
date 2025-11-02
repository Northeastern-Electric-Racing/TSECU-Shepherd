
#include "unity.h"
#include "mock_stub_functions.h"
#include "datastructs.h"
#include "analyzer.h"
#include <stdbool.h>
#include <stdlib.h>

acc_data_t *bmsdata;
chipdata_t *chipdata;


void setUp(void) {
    bmsdata = malloc(sizeof(acc_data_t));
    chipdata = malloc(sizeof(chipdata_t));
}

void tearDown(void) {
    free(bmsdata);
    free(chipdata);
}

// A simple random test
void test_random(void) {
    chipdata->alpha = true;
    TEST_ASSERT_EQUAL_INT(get_num_cells(chipdata), NUM_CELLS_ALPHA); 
    chipdata->alpha = false;
    TEST_ASSERT_EQUAL_INT(get_num_cells(chipdata), NUM_CELLS_BETA); 
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_random);
    return UNITY_END();
}