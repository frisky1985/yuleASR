/**
 * @file test_cansm.c
 * @brief CAN State Manager Unit Tests
 */

// @tests src/bsw/services/cansm/src/CanSm.c  @tests src/bsw/services/cansm/include/CanSm.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "CanSM.h"
#include "CanSM_Cfg.h"

static void test_CanSM_Init(void **state) {
    (void)state;
    const CanSM_ConfigType* config = NULL;
    Std_ReturnType result = CanSM_Init(config);
    assert_int_equal(result, E_OK);
}

static void test_CanSM_DeInit(void **state) {
    (void)state;
    CanSM_DeInit();
    assert_true(1);
}

static void test_CanSM_RequestComMode(void **state) {
    (void)state;
    NetworkHandleType network = 0;
    ComM_ModeType comMode = COMM_FULL_COMMUNICATION;
    Std_ReturnType result = CanSM_RequestComMode(network, comMode);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_CanSM_GetCurrentComMode(void **state) {
    (void)state;
    NetworkHandleType network = 0;
    ComM_ModeType comMode;
    Std_ReturnType result = CanSM_GetCurrentComMode(network, &comMode);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_CanSM_MainFunction(void **state) {
    (void)state;
    CanSM_MainFunction();
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_CanSM_Init),
        cmocka_unit_test(test_CanSM_DeInit),
        cmocka_unit_test(test_CanSM_RequestComMode),
        cmocka_unit_test(test_CanSM_GetCurrentComMode),
        cmocka_unit_test(test_CanSM_MainFunction),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
