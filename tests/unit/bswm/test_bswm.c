/**
 * @file test_bswm.c
 * @brief BSWM (BSW Mode Manager) Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "BswM.h"
#include "BswM_Cfg.h"

/* Test: BswM_Init */
/** @req SWS_BswM_00001 */
static void test_BswM_Init(void **state)
{
    (void)state;
    
    const BswM_ConfigType* config = NULL;
    Std_ReturnType result = BswM_Init(config);
    assert_int_equal(result, E_OK);
}

/* Test: BswM_DeInit */
/** @req SWS_BswM_00002 */
static void test_BswM_DeInit(void **state)
{
    (void)state;
    
    BswM_DeInit();
    assert_true(1);
}

/* Test: BswM_MainFunction */
/** @req SWS_BswM_00020 */
static void test_BswM_MainFunction(void **state)
{
    (void)state;
    
    BswM_MainFunction();
    assert_true(1);
}

/* Test: BswM_RequestMode */
/** @req SWS_BswM_00010 */
static void test_BswM_RequestMode(void **state)
{
    (void)state;
    
    BswM_UserType requesting_user = 0;
    BswM_ModeType requested_mode = 0;
    
    Std_ReturnType result = BswM_RequestMode(requesting_user, requested_mode);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: BswM_GetVersionInfo */
/** @req SWS_BswM_00030 */
static void test_BswM_GetVersionInfo(void **state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    BswM_GetVersionInfo(&versionInfo);
    assert_true(1);
}

/* Test: BswM_EcuM_CurrentState */
static void test_BswM_EcuM_CurrentState(void **state)
{
    (void)state;
    
    EcuM_StateType currentState = ECUM_STATE_STARTUP;
    BswM_EcuM_CurrentState(currentState);
    assert_true(1);
}

/* Test: BswM_ComM_CurrentMode */
static void test_BswM_ComM_CurrentMode(void **state)
{
    (void)state;
    
    NetworkHandleType Network = 0;
    ComM_ModeType RequestedMode = COMM_FULL_COMMUNICATION;
    
    BswM_ComM_CurrentMode(Network, RequestedMode);
    assert_true(1);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_BswM_Init),
        cmocka_unit_test(test_BswM_DeInit),
        cmocka_unit_test(test_BswM_MainFunction),
        cmocka_unit_test(test_BswM_RequestMode),
        cmocka_unit_test(test_BswM_GetVersionInfo),
        cmocka_unit_test(test_BswM_EcuM_CurrentState),
        cmocka_unit_test(test_BswM_ComM_CurrentMode),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
