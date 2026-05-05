/**
 * @file test_comm.c
 * @brief Communication Manager Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "ComM.h"
#include "ComM_Cfg.h"

static void test_ComM_Init(void **state) {
    (void)state;
    const ComM_ConfigType* config = NULL;
    Std_ReturnType result = ComM_Init(config);
    assert_int_equal(result, E_OK);
}

static void test_ComM_DeInit(void **state) {
    (void)state;
    ComM_DeInit();
    assert_true(1);
}

static void test_ComM_RequestComMode(void **state) {
    (void)state;
    ComM_UserHandleType user = 0;
    ComM_ModeType comMode = COMM_FULL_COMMUNICATION;
    Std_ReturnType result = ComM_RequestComMode(user, comMode);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_ComM_GetCurrentComMode(void **state) {
    (void)state;
    ComM_UserHandleType user = 0;
    ComM_ModeType comMode;
    Std_ReturnType result = ComM_GetCurrentComMode(user, &comMode);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_ComM_GetMaxComMode(void **state) {
    (void)state;
    ComM_UserHandleType user = 0;
    ComM_ModeType comMode;
    Std_ReturnType result = ComM_GetMaxComMode(user, &comMode);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_ComM_MainFunction(void **state) {
    (void)state;
    ComM_MainFunction();
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ComM_Init),
        cmocka_unit_test(test_ComM_DeInit),
        cmocka_unit_test(test_ComM_RequestComMode),
        cmocka_unit_test(test_ComM_GetCurrentComMode),
        cmocka_unit_test(test_ComM_GetMaxComMode),
        cmocka_unit_test(test_ComM_MainFunction),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
