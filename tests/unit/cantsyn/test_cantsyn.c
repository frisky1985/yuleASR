/**
 * @file test_cantsyn.c
 * @brief CAN Time Synchronization Unit Tests
 */

// @tests src/bsw/services/cantsyn/src/CanTSyn.c  @tests src/bsw/services/cantsyn/include/CanTSyn.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "CanTSyn.h"
#include "CanTSyn_Cfg.h"

/** @req SWS_CanTSyn_00001 */
static void test_CanTSyn_Init(void **state) {
    (void)state;
    const CanTSyn_ConfigType* config = NULL;
    Std_ReturnType result = CanTSyn_Init(config);
    assert_int_equal(result, E_OK);
}

/** @req SWS_CanTSyn_00004 */
static void test_CanTSyn_MainFunction(void **state) {
    (void)state;
    CanTSyn_MainFunction();
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_CanTSyn_Init),
        cmocka_unit_test(test_CanTSyn_MainFunction),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
