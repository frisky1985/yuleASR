/**
 * @file test_E2E.c
 * @brief E2E Protection Unit Tests
 */

// @tests src/bsw/services/e2e/src/E2E.c  @tests src/bsw/services/e2e/include/E2E.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "E2E.h"
#include "E2E_P01.h"
#include "E2E_P02.h"
#include "E2E_P04.h"
#include "E2E_P05.h"

/*==================================================================================================
 *                                  E2E Profile 01 Tests
 *================================================================================================*/
static void test_E2E_P01_Protect_Check_RoundTrip(void **state)
{
    (void)state;
    
    E2E_P01ConfigType config = {
        .CounterOffset = 0,
        .CRCOffset = 8,
        .DataID = 0x1234,
        .DataLength = 16,
        .MaxDeltaCounterInit = 1
    };
    
    E2E_P01ProtectStateType txState = {0};
    E2E_P01CheckStateType rxState = {0};
    
    uint8 data[16] = {0};
    
    /* Protect data */
    Std_ReturnType protectResult = E2E_P01Protect(&config, &txState, data);
    assert_int_equal(protectResult, E_OK);
    
    /* Check protected data */
    E2E_P01CheckStatusType checkResult = E2E_P01Check(&config, &rxState, data);
    
    /* First check after protect should be E2E_P_OK */
    assert_true(checkResult == E2E_P_OK || checkResult == E2E_P_NONEWDATA);
}

static void test_E2E_P01_CounterIncrement(void **state)
{
    (void)state;
    
    E2E_P01ConfigType config = {
        .CounterOffset = 0,
        .CRCOffset = 8,
        .DataID = 0x1234,
        .DataLength = 16,
        .MaxDeltaCounterInit = 1
    };
    
    E2E_P01ProtectStateType txState = {0};
    uint8 data1[16] = {0};
    uint8 data2[16] = {0};
    
    /* Protect twice */
    E2E_P01Protect(&config, &txState, data1);
    E2E_P01Protect(&config, &txState, data2);
    
    /* Counter should have incremented */
    assert_int_equal(txState.Counter, 2);
}

static void test_E2E_P01_CounterWrapAround(void **state)
{
    (void)state;
    
    E2E_P01ConfigType config = {
        .CounterOffset = 0,
        .CRCOffset = 8,
        .DataID = 0x1234,
        .DataLength = 16,
        .MaxDeltaCounterInit = 1
    };
    
    E2E_P01ProtectStateType txState = {0};
    txState.Counter = 14; /* Near wrap-around */
    
    uint8 data[16] = {0};
    
    E2E_P01Protect(&config, &txState, data);
    assert_int_equal(txState.Counter, 15);
    
    E2E_P01Protect(&config, &txState, data);
    assert_int_equal(txState.Counter, 0); /* Wrapped */
}

/*==================================================================================================
 *                                  E2E Profile 04 Tests (CRC32)
 *================================================================================================*/
static void test_E2E_P04_Protect_Check_RoundTrip(void **state)
{
    (void)state;
    
    E2E_P04ConfigType config = {
        .CounterOffset = 0,
        .CRCOffset = 16,
        .DataID = 0x12345678,
        .DataLength = 32,
        .MaxDeltaCounterInit = 1
    };
    
    E2E_P04ProtectStateType txState = {0};
    E2E_P04CheckStateType rxState = {0};
    
    uint8 data[32] = {0};
    
    Std_ReturnType protectResult = E2E_P04Protect(&config, &txState, data);
    assert_int_equal(protectResult, E_OK);
    
    E2E_P04CheckStatusType checkResult = E2E_P04Check(&config, &rxState, data);
    assert_true(checkResult == E2E_P_OK || checkResult == E2E_P_NONEWDATA);
}

/*==================================================================================================
 *                                  E2E Profile 05 Tests (CRC64)
 *================================================================================================*/
static void test_E2E_P05_Protect_Check_RoundTrip(void **state)
{
    (void)state;
    
    E2E_P05ConfigType config = {
        .CounterOffset = 0,
        .CRCOffset = 32,
        .DataID = 0x12345678,
        .DataLength = 64,
        .MaxDeltaCounterInit = 1
    };
    
    E2E_P05ProtectStateType txState = {0};
    E2E_P05CheckStateType rxState = {0};
    
    uint8 data[64] = {0};
    
    Std_ReturnType protectResult = E2E_P05Protect(&config, &txState, data);
    assert_int_equal(protectResult, E_OK);
    
    E2E_P05CheckStatusType checkResult = E2E_P05Check(&config, &rxState, data);
    assert_true(checkResult == E2E_P_OK || checkResult == E2E_P_NONEWDATA);
}

/*==================================================================================================
 *                                  E2E Library Tests
 *================================================================================================*/
static void test_E2E_GetVersionInfo(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    
    E2E_GetVersionInfo(&versionInfo);
    
    assert_int_equal(versionInfo.vendorID, 1);
    assert_int_equal(versionInfo.moduleID, 207);
}

static void test_E2E_P01_MapStatusToSM(void **state)
{
    (void)state;
    E2E_SMStateType smState = E2E_SM_DEINIT;
    boolean error = FALSE;
    
    E2E_P01MapStatusToSM(E2E_P_OK, &smState, &error);
    
    /* State should transition from DEINIT to VALID */
    assert_int_equal(smState, E2E_SM_VALID);
    assert_false(error);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        /* Profile 01 tests */
        cmocka_unit_test(test_E2E_P01_Protect_Check_RoundTrip),
        cmocka_unit_test(test_E2E_P01_CounterIncrement),
        cmocka_unit_test(test_E2E_P01_CounterWrapAround),
        
        /* Profile 04 tests */
        cmocka_unit_test(test_E2E_P04_Protect_Check_RoundTrip),
        
        /* Profile 05 tests */
        cmocka_unit_test(test_E2E_P05_Protect_Check_RoundTrip),
        
        /* Library tests */
        cmocka_unit_test(test_E2E_GetVersionInfo),
        cmocka_unit_test(test_E2E_P01_MapStatusToSM),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}
