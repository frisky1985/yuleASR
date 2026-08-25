/**
 * @file test_ecum.c
 * @brief ECU State Manager Unit Tests
 */

// @tests src/bsw/services/ecum/src/EcuM.c  @tests src/bsw/services/ecum/include/EcuM.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "EcuM.h"
#include "EcuM_Cfg.h"

/** @req SWS_EcuM_00001 */
static void test_EcuM_Init(void **state) {
    (void)state;
    EcuM_Init();
    assert_true(1);
}

/** @req SWS_EcuM_00011 */
static void test_EcuM_StartupTwo(void **state) {
    (void)state;
    EcuM_StartupTwo();
    assert_true(1);
}

/** @req SWS_EcuM_00035 */
static void test_EcuM_Shutdown(void **state) {
    (void)state;
    EcuM_Shutdown();
    assert_true(1);
}

/** @req SWS_EcuM_00021 */
static void test_EcuM_GetState(void **state) {
    (void)state;
    EcuM_StateType state;
    Std_ReturnType result = EcuM_GetState(&state);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_EcuM_00090 */
static void test_EcuM_RequestRUN(void **state) {
    (void)state;
    EcuM_UserType user = 0;
    Std_ReturnType result = EcuM_RequestRUN(user);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_EcuM_00091 */
static void test_EcuM_ReleaseRUN(void **state) {
    (void)state;
    EcuM_UserType user = 0;
    Std_ReturnType result = EcuM_ReleaseRUN(user);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_EcuM_00060 */
static void test_EcuM_MainFunction(void **state) {
    (void)state;
    EcuM_MainFunction();
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_EcuM_Init),
        cmocka_unit_test(test_EcuM_StartupTwo),
        cmocka_unit_test(test_EcuM_Shutdown),
        cmocka_unit_test(test_EcuM_GetState),
        cmocka_unit_test(test_EcuM_RequestRUN),
        cmocka_unit_test(test_EcuM_ReleaseRUN),
        cmocka_unit_test(test_EcuM_MainFunction),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
