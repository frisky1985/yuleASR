/**
 * @file test_dcm.c
 * @brief DCM Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Dcm.h"
#include "Dcm_Cfg.h"

static void test_Dcm_Init(void **state) {
    (void)state;
    const Dcm_ConfigType* config = NULL;
    Std_ReturnType result = Dcm_Init(config);
    assert_int_equal(result, E_OK);
}

static void test_Dcm_DeInit(void **state) {
    (void)state;
    Dcm_DeInit();
    assert_true(1);
}

static void test_Dcm_MainFunction(void **state) {
    (void)state;
    Dcm_MainFunction();
    assert_true(1);
}

static void test_Dcm_GetVersionInfo(void **state) {
    (void)state;
    Std_VersionInfoType versionInfo;
    Dcm_GetVersionInfo(&versionInfo);
    assert_true(1);
}

static void test_Dcm_ComM_NoComModeEntered(void **state) {
    (void)state;
    uint8 channelId = 0;
    Dcm_ComM_NoComModeEntered(channelId);
    assert_true(1);
}

static void test_Dcm_ComM_FullComModeEntered(void **state) {
    (void)state;
    uint8 channelId = 0;
    Dcm_ComM_FullComModeEntered(channelId);
    assert_true(1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_Dcm_Init),
        cmocka_unit_test(test_Dcm_DeInit),
        cmocka_unit_test(test_Dcm_MainFunction),
        cmocka_unit_test(test_Dcm_GetVersionInfo),
        cmocka_unit_test(test_Dcm_ComM_NoComModeEntered),
        cmocka_unit_test(test_Dcm_ComM_FullComModeEntered),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
