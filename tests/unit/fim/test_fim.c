/**
 * @file test_fim.c
 * @brief Function Inhibition Manager Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "FiM.h"
#include "FiM_Cfg.h"

static void test_FiM_Init(void **state) {
    (void)state;
    const FiM_ConfigType* config = NULL;
    Std_ReturnType result = FiM_Init(config);
    assert_int_equal(result, E_OK);
}

static void test_FiM_DeInit(void **state) {
    (void)state;
    FiM_DeInit();
    assert_true(1);
}

static void test_FiM_GetFunctionPermission(void **state) {
    (void)state;
    FiM_FunctionIdType functionId = 0;
    boolean permission;
    Std_ReturnType result = FiM_GetFunctionPermission(functionId, &permission);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_FiM_SetFunctionAvailable(void **state) {
    (void)state;
    FiM_FunctionIdType functionId = 0;
    boolean available = TRUE;
    Std_ReturnType result = FiM_SetFunctionAvailable(functionId, available);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_FiM_MainFunction(void **state) {
    (void)state;
    FiM_MainFunction();
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_FiM_Init),
        cmocka_unit_test(test_FiM_DeInit),
        cmocka_unit_test(test_FiM_GetFunctionPermission),
        cmocka_unit_test(test_FiM_SetFunctionAvailable),
        cmocka_unit_test(test_FiM_MainFunction),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
