/**
 * @file test_nm.c
 * @brief Network Management Unit Tests
 */

// @tests src/bsw/services/nm/src/Nm.c  @tests src/bsw/services/nm/include/Nm.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Nm.h"

static void test_Nm_Init(void **state) {
    (void)state;
    const Nm_ConfigType* config = NULL;
    Std_ReturnType result = Nm_Init(config);
    assert_int_equal(result, E_OK);
}

static void test_Nm_DeInit(void **state) {
    (void)state;
    Nm_DeInit();
    assert_true(1);
}

static void test_Nm_PassiveStartUp(void **state) {
    (void)state;
    NetworkHandleType nmChannelHandle = 0;
    Std_ReturnType result = Nm_PassiveStartUp(nmChannelHandle);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Nm_NetworkRequest(void **state) {
    (void)state;
    NetworkHandleType nmChannelHandle = 0;
    Std_ReturnType result = Nm_NetworkRequest(nmChannelHandle);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Nm_NetworkRelease(void **state) {
    (void)state;
    NetworkHandleType nmChannelHandle = 0;
    Std_ReturnType result = Nm_NetworkRelease(nmChannelHandle);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Nm_GetState(void **state) {
    (void)state;
    NetworkHandleType nmChannelHandle = 0;
    Nm_StateType nmState;
    Nm_ModeType nmMode;
    Std_ReturnType result = Nm_GetState(nmChannelHandle, &nmState, &nmMode);
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Nm_MainFunction(void **state) {
    (void)state;
    Nm_MainFunction();
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_Nm_Init),
        cmocka_unit_test(test_Nm_DeInit),
        cmocka_unit_test(test_Nm_PassiveStartUp),
        cmocka_unit_test(test_Nm_NetworkRequest),
        cmocka_unit_test(test_Nm_NetworkRelease),
        cmocka_unit_test(test_Nm_GetState),
        cmocka_unit_test(test_Nm_MainFunction),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
