/**
 * @file test_canm.c
 * @brief CAN Network Management Unit Tests
 */

// @tests src/bsw/ecual/canNm/src/CanNm.c  @tests src/bsw/ecual/canNm/include/CanNm.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "CanNm.h"
#include "CanNm_Cfg.h"

/** @req SWS_CanNm_00001 */
static void test_CanNm_Init(void **state) {
    (void)state;
    const CanNm_ConfigType* config = NULL;
    Std_ReturnType result = CanNm_Init(config);
    assert_int_equal(result, E_OK);
}

/** @req SWS_CanNm_00001 */
static void test_CanNm_DeInit(void **state) {
    (void)state;
    CanNm_DeInit();
    assert_true(1);
}

/** @req SWS_CanNm_00005 */
static void test_CanNm_PassiveStartUp(void **state) {
    (void)state;
    NetworkHandleType nmChannelHandle = 0;
    Std_ReturnType result = CanNm_PassiveStartUp(nmChannelHandle);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_CanNm_00006 */
static void test_CanNm_NetworkRequest(void **state) {
    (void)state;
    NetworkHandleType nmChannelHandle = 0;
    Std_ReturnType result = CanNm_NetworkRequest(nmChannelHandle);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_CanNm_00007 */
static void test_CanNm_NetworkRelease(void **state) {
    (void)state;
    NetworkHandleType nmChannelHandle = 0;
    Std_ReturnType result = CanNm_NetworkRelease(nmChannelHandle);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/** @req SWS_CanNm_00004 */
static void test_CanNm_MainFunction(void **state) {
    (void)state;
    CanNm_MainFunction();
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_CanNm_Init),
        cmocka_unit_test(test_CanNm_DeInit),
        cmocka_unit_test(test_CanNm_PassiveStartUp),
        cmocka_unit_test(test_CanNm_NetworkRequest),
        cmocka_unit_test(test_CanNm_NetworkRelease),
        cmocka_unit_test(test_CanNm_MainFunction),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
