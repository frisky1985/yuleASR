/**
 * @file test_CanTSyn.c
 * @brief CanTSyn Unit Tests
 */

// @tests src/bsw/services/cantsyn/src/CanTSyn.c  @tests src/bsw/services/cantsyn/include/CanTSyn.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "CanTSyn.h"

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    CanTSyn_DeInit();
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    CanTSyn_DeInit();
    return 0;
}

/*==================================================================================================
 *                                    Test Cases
 *================================================================================================*/

static void test_CanTSyn_Init_ValidConfig(void **state)
{
    (void)state;
    /* Initialize with valid config */
    Std_ReturnType result = CanTSyn_Init(NULL);
    assert_int_equal(result, E_OK);
}

static void test_CanTSyn_DeInit(void **state)
{
    (void)state;
    CanTSyn_Init(NULL);
    CanTSyn_DeInit();
    assert_true(1);
}

static void test_CanTSyn_GetVersionInfo(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    
    CanTSyn_Init(NULL);
    CanTSyn_GetVersionInfo(&versionInfo);
    
    /* Verify module IDs */
    assert_int_equal(versionInfo.moduleID, CANTSYN_MODULE_ID);
}

static void test_CanTSyn_MainFunction_Uninit(void **state)
{
    (void)state;
    /* Should not crash when uninitialized */
    CanTSyn_MainFunction();
    assert_true(1);
}

static void test_CanTSyn_MainFunction_Initialized(void **state)
{
    (void)state;
    CanTSyn_Init(NULL);
    CanTSyn_MainFunction();
    assert_true(1);
}

static void test_CanTSyn_MessageTypes(void **state)
{
    (void)state;
    /* Verify message type constants */
    assert_int_equal(CANTSYN_SYNC_MSG_TYPE, 0x10U);
    assert_int_equal(CANTSYN_OFS_MSG_TYPE, 0x20U);
}

static void test_CanTSyn_TimeDomains(void **state)
{
    (void)state;
    /* Verify time domain constants */
    assert_int_equal(CANTSYN_TIME_DOMAIN_0, 0x00U);
    assert_int_equal(CANTSYN_TIME_DOMAIN_1, 0x01U);
    assert_int_equal(CANTSYN_TIME_DOMAIN_2, 0x02U);
    assert_int_equal(CANTSYN_TIME_DOMAIN_3, 0x03U);
}

static void test_CanTSyn_MessageLengths(void **state)
{
    (void)state;
    /* Verify message lengths */
    assert_int_equal(CANTSYN_SYNC_MSG_LENGTH, 16U);
    assert_int_equal(CANTSYN_OFS_MSG_LENGTH, 12U);
}

static void test_CanTSyn_RxIndication_NullPdu(void **state)
{
    (void)state;
    CanTSyn_Init(NULL);
    /* Should not crash with NULL */
    CanTSyn_RxIndication(0, NULL);
    assert_true(1);
}

static void test_CanTSyn_TxConfirmation(void **state)
{
    (void)state;
    CanTSyn_Init(NULL);
    /* Should not crash */
    CanTSyn_TxConfirmation(0, E_OK);
    assert_true(1);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_CanTSyn_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTSyn_DeInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTSyn_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTSyn_MainFunction_Uninit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTSyn_MainFunction_Initialized, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTSyn_MessageTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTSyn_TimeDomains, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTSyn_MessageLengths, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTSyn_RxIndication_NullPdu, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTSyn_TxConfirmation, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}
