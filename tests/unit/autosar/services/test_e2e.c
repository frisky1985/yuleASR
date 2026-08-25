/**
 * @file test_e2e.c
 * @brief E2E (End-to-End) Protection Library Comprehensive Unit Tests
 * @details Tests covering Profile 1-7, validation, protection, checking,
 *          status mapping, and edge cases. Target coverage: 80%+
 * 
 * AUTOSAR Standard: R22-11
 * ASIL Level: D
 * Coverage Target: >80%
 * 
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

#include "E2E.h"
#include "E2E_Cfg.h"
#include "E2E_P01.h"
#include "E2E_P02.h"
#include "E2E_P04.h"
#include "E2E_P05.h"
#include "E2E_P06.h"
#include "E2E_P07.h"
#include "Crc.h"

/*==================================================================================================
 *                                    Test Constants
 *================================================================================================*/
#define TEST_DATA_SIZE      256
#define TEST_DATAID_P01     0x1234U
#define TEST_DATAID_P02     0x5678U
#define TEST_DATAID_P04     0x12345678U
#define TEST_DATAID_P05     0x9ABCDEF0U
#define TEST_DATAID_P06     0xDEADBEEFU
#define TEST_DATAID_P07     0xCAFEBABEU

/*==================================================================================================
 *                                    Test Fixtures
 *================================================================================================*/
static uint8 testData[TEST_DATA_SIZE];

/* Profile 01 fixtures */
static E2E_P01ConfigType p01Config;
static E2E_P01ProtectStateType p01ProtectState;
static E2E_P01CheckStateType p01CheckState;

/* Profile 02 fixtures */
static E2E_P02ConfigType p02Config;
static E2E_P02ProtectStateType p02ProtectState;
static E2E_P02CheckStateType p02CheckState;

/* Profile 04 fixtures */
static E2E_P04ConfigType p04Config;
static E2E_P04ProtectStateType p04ProtectState;
static E2E_P04CheckStateType p04CheckState;

/* Profile 05 fixtures */
static E2E_P05ConfigType p05Config;
static E2E_P05ProtectStateType p05ProtectState;
static E2E_P05CheckStateType p05CheckState;

/* Profile 06 fixtures */
static E2E_P06ConfigType p06Config;
static E2E_P06ProtectStateType p06ProtectState;
static E2E_P06CheckStateType p06CheckState;

/* Profile 07 fixtures */
static E2E_P07ConfigType p07Config;
static E2E_P07ProtectStateType p07ProtectState;
static E2E_P07CheckStateType p07CheckState;

/*==================================================================================================
 *                                    Setup/Teardown
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    
    /* Clear test data */
    memset(testData, 0, sizeof(testData));
    
    /* Profile 01 setup */
    p01Config.DataID = TEST_DATAID_P01;
    p01Config.DataLength = 16U;
    p01Config.DataIDMode = E2E_P01_DATAID_BOTH;
    p01Config.CounterOffset = 1U;
    p01Config.CRCOffset = 0U;
    p01Config.DataIDNibbleOffset = 2U;
    
    memset(&p01ProtectState, 0, sizeof(p01ProtectState));
    memset(&p01CheckState, 0, sizeof(p01CheckState));
    p01CheckState.MaxDeltaCounterInit = 3U;
    
    /* Profile 02 setup */
    p02Config.DataID = TEST_DATAID_P02;
    p02Config.DataLength = 16U;
    p02Config.CounterOffset = 1U;
    p02Config.CRCOffset = 0U;
    p02Config.DataIDNibbleOffset = 2U;
    p02Config.DualPathEnabled = FALSE;
    
    memset(&p02ProtectState, 0, sizeof(p02ProtectState));
    memset(&p02CheckState, 0, sizeof(p02CheckState));
    p02CheckState.MaxDeltaCounterInit = 3U;
    
    /* Profile 04 setup */
    p04Config.DataID = TEST_DATAID_P04;
    p04Config.DataLength = 32U;
    p04Config.CounterOffset = 4U;
    p04Config.CRCOffset = 0U;
    p04Config.IncludeDataID = TRUE;
    
    memset(&p04ProtectState, 0, sizeof(p04ProtectState));
    memset(&p04CheckState, 0, sizeof(p04CheckState));
    p04CheckState.MaxDeltaCounterInit = 3U;
    
    /* Profile 05 setup */
    p05Config.DataLength = 64U;
    p05Config.DataID = TEST_DATAID_P05;
    p05Config.CounterOffset = 0U;
    p05Config.CRCOffset = 32U;
    p05Config.DataIDOffset = 64U;
    p05Config.MaxDeltaCounterInit = 3U;
    p05Config.IncludeDataID = TRUE;
    
    memset(&p05ProtectState, 0, sizeof(p05ProtectState));
    memset(&p05CheckState, 0, sizeof(p05CheckState));
    p05CheckState.MaxDeltaCounterInit = 3U;
    
    /* Profile 06 setup */
    p06Config.DataID = TEST_DATAID_P06;
    p06Config.CounterOffset = 0U;
    p06Config.CRCOffset = 16U;
    p06Config.LengthOffset = 80U;
    p06Config.MaxDeltaCounterInit = 3U;
    p06Config.MinDataLength = 32U;
    p06Config.MaxDataLength = 128U;
    p06Config.IncludeDataID = TRUE;
    
    memset(&p06ProtectState, 0, sizeof(p06ProtectState));
    memset(&p06CheckState, 0, sizeof(p06CheckState));
    p06CheckState.MaxDeltaCounterInit = 3U;
    
    /* Profile 07 setup */
    p07Config.DataID = TEST_DATAID_P07;
    p07Config.CounterOffset = 0U;
    p07Config.CRCOffset = 8U;
    p07Config.MaxDeltaCounterInit = 3U;
    p07Config.MinDataLength = 16U;
    p07Config.MaxDataLength = 64U;
    p07Config.IncludeDataID = TRUE;
    
    memset(&p07ProtectState, 0, sizeof(p07ProtectState));
    memset(&p07CheckState, 0, sizeof(p07CheckState));
    p07CheckState.MaxDeltaCounterInit = 3U;
    
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    /* Cleanup if needed */
    return 0;
}

/*==================================================================================================
 *                              E2E Library API Tests
 *================================================================================================*/
/** @req SWS_E2E_00001 */
static void test_E2E_Init_Valid(void **state)
{
    (void)state;
    /* E2E_Init returns OK with NULL config */
    Std_ReturnType result = E2E_Init(NULL);
    assert_int_equal(result, E_OK);
}

/** @req SWS_E2E_00001 */
static void test_E2E_DeInit(void **state)
{
    (void)state;
    Std_ReturnType result = E2E_DeInit();
    assert_int_equal(result, E_OK);
}

/*==================================================================================================
 *                              Profile 01 Tests (CRC8)
 *================================================================================================*/
static void test_E2E_P01_Protect_NullParams(void **state)
{
    (void)state;
    uint8 data[16];
    
    /* Test NULL Config */
    Std_ReturnType result = E2E_P01Protect(NULL, &p01ProtectState, data);
    assert_int_equal(result, E_NOT_OK);
    
    /* Test NULL State */
    result = E2E_P01Protect(&p01Config, NULL, data);
    assert_int_equal(result, E_NOT_OK);
    
    /* Test NULL Data */
    result = E2E_P01Protect(&p01Config, &p01ProtectState, NULL);
    assert_int_equal(result, E_NOT_OK);
}

static void test_E2E_P01_Protect_Check_RoundTrip(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* Protect data */
    Std_ReturnType result = E2E_P01Protect(&p01Config, &p01ProtectState, data);
    assert_int_equal(result, E_OK);
    
    /* Verify CRC was written */
    assert_true(data[p01Config.CRCOffset] != 0 || data[p01Config.CRCOffset + 1] != 0);
    
    /* Check data */
    result = E2E_P01Check(&p01Config, &p01CheckState, data);
    assert_int_equal(result, E_OK);
    
    /* First check should be INITIAL */
    assert_int_equal(p01CheckState.Status, E2E_P_INITIAL);
}

static void test_E2E_P01_CounterIncrement(void **state)
{
    (void)state;
    uint8 data1[16] = {0};
    uint8 data2[16] = {0};
    
    /* Protect twice */
    E2E_P01Protect(&p01Config, &p01ProtectState, data1);
    uint8 counter1 = p01ProtectState.Counter;
    
    E2E_P01Protect(&p01Config, &p01ProtectState, data2);
    uint8 counter2 = p01ProtectState.Counter;
    
    /* Counter should have incremented */
    assert_int_equal(counter2, counter1 + 1);
}

static void test_E2E_P01_CounterWrapAround(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* Set counter near wrap-around (max is 14, wraps to 0) */
    p01ProtectState.Counter = 14;
    
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    assert_int_equal(p01ProtectState.Counter, 15);
    
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    assert_int_equal(p01ProtectState.Counter, 0); /* Wrapped */
}

static void test_E2E_P01_Check_NullParams(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* Test NULL Config */
    Std_ReturnType result = E2E_P01Check(NULL, &p01CheckState, data);
    assert_int_equal(result, E_NOT_OK);
    
    /* Test NULL State */
    result = E2E_P01Check(&p01Config, NULL, data);
    assert_int_equal(result, E_NOT_OK);
    
    /* Test NULL Data */
    result = E2E_P01Check(&p01Config, &p01CheckState, NULL);
    assert_int_equal(result, E_NOT_OK);
}

static void test_E2E_P01_Check_WrongCRC(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* Protect and corrupt */
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    data[5] ^= 0xFF; /* Corrupt data */
    
    /* Check should detect wrong CRC */
    E2E_P01Check(&p01Config, &p01CheckState, data);
    assert_int_equal(p01CheckState.Status, E2E_P_WRONGCRC);
}

static void test_E2E_P01_Check_ConsecutiveOK(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* First protect and check */
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    E2E_P01Check(&p01Config, &p01CheckState, data);
    
    /* Second message with consecutive counter */
    memset(data, 0, sizeof(data));
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    E2E_P01Check(&p01Config, &p01CheckState, data);
    
    assert_int_equal(p01CheckState.Status, E2E_P_OK);
}

static void test_E2E_P01_Check_Repeated(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* Protect twice (same counter for test setup) */
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    
    /* First check - initial */
    E2E_P01Check(&p01Config, &p01CheckState, data);
    
    /* Same data again - should be repeated */
    E2E_P01Check(&p01Config, &p01CheckState, data);
    assert_int_equal(p01CheckState.Status, E2E_P_REPEATED);
}

static void test_E2E_P01_Check_SomeLost(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* First message */
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    E2E_P01Check(&p01Config, &p01CheckState, data);
    
    /* Skip 2 counters (counter jumps by 3) */
    p01ProtectState.Counter += 2;
    
    /* Next message */
    memset(data, 0, sizeof(data));
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    E2E_P01Check(&p01Config, &p01CheckState, data);
    
    /* Should detect some lost within MaxDeltaCounterInit */
    assert_int_equal(p01CheckState.Status, E2E_P_OKSOMELOST);
}

static void test_E2E_P01_Check_Sync(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* First message */
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    E2E_P01Check(&p01Config, &p01CheckState, data);
    
    /* Skip many counters (beyond MaxDeltaCounterInit) */
    p01ProtectState.Counter += 10;
    
    /* Next message */
    memset(data, 0, sizeof(data));
    E2E_P01Protect(&p01Config, &p01ProtectState, data);
    E2E_P01Check(&p01Config, &p01CheckState, data);
    
    /* Should require sync */
    assert_int_equal(p01CheckState.Status, E2E_P_SYNC);
}

static void test_E2E_P01_DataIDModes(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    E2E_P01ConfigType testConfig = p01Config;
    
    /* Test DATAID_LOW mode */
    testConfig.DataIDMode = E2E_P01_DATAID_LOW;
    memset(&p01ProtectState, 0, sizeof(p01ProtectState));
    Std_ReturnType result = E2E_P01Protect(&testConfig, &p01ProtectState, data);
    assert_int_equal(result, E_OK);
    
    /* Test DATAID_ALT mode */
    testConfig.DataIDMode = E2E_P01_DATAID_ALT;
    memset(data, 0, sizeof(data));
    memset(&p01ProtectState, 0, sizeof(p01ProtectState));
    result = E2E_P01Protect(&testConfig, &p01ProtectState, data);
    assert_int_equal(result, E_OK);
    
    /* Test DATAID_NIBBLE mode */
    testConfig.DataIDMode = E2E_P01_DATAID_NIBBLE;
    memset(data, 0, sizeof(data));
    memset(&p01ProtectState, 0, sizeof(p01ProtectState));
    result = E2E_P01Protect(&testConfig, &p01ProtectState, data);
    assert_int_equal(result, E_OK);
}

static void test_E2E_P01_MapStatusToSM(void **state)
{
    (void)state;
    E2E_SMStateType smState;
    boolean error;
    
    /* E2E_P_OK -> VALID */
    E2E_P01MapStatusToSM(E2E_P_OK, &smState, &error);
    assert_int_equal(smState, E2E_SM_VALID);
    assert_false(error);
    
    /* E2E_P_OKSOMELOST -> VALID */
    E2E_P01MapStatusToSM(E2E_P_OKSOMELOST, &smState, &error);
    assert_int_equal(smState, E2E_SM_VALID);
    assert_false(error);
    
    /* E2E_P_WRONGCRC -> INVALID */
    E2E_P01MapStatusToSM(E2E_P_WRONGCRC, &smState, &error);
    assert_int_equal(smState, E2E_SM_INVALID);
    assert_true(error);
    
    /* E2E_P_WRONGSEQUENCE -> INVALID */
    E2E_P01MapStatusToSM(E2E_P_WRONGSEQUENCE, &smState, &error);
    assert_int_equal(smState, E2E_SM_INVALID);
    assert_true(error);
    
    /* E2E_P_REPEATED -> INVALID */
    E2E_P01MapStatusToSM(E2E_P_REPEATED, &smState, &error);
    assert_int_equal(smState, E2E_SM_INVALID);
    assert_true(error);
    
    /* E2E_P_SYNC -> INIT */
    E2E_P01MapStatusToSM(E2E_P_SYNC, &smState, &error);
    assert_int_equal(smState, E2E_SM_INIT);
    assert_false(error);
    
    /* E2E_P_INITIAL -> NODATA */
    E2E_P01MapStatusToSM(E2E_P_INITIAL, &smState, &error);
    assert_int_equal(smState, E2E_SM_NODATA);
    assert_false(error);
    
    /* E2E_P_NONEWDATA -> INVALID */
    E2E_P01MapStatusToSM(E2E_P_NONEWDATA, &smState, &error);
    assert_int_equal(smState, E2E_SM_INVALID);
}

/*==================================================================================================
 *                              Profile 02 Tests (Dual Path)
 *================================================================================================*/
static void test_E2E_P02_Protect_NullParams(void **state)
{
    (void)state;
    uint8 data[16];
    
    /* Test NULL Config */
    Std_ReturnType result = E2E_P02Protect(NULL, &p02ProtectState, data);
    assert_int_equal(result, E_NOT_OK);
    
    /* Test NULL State */
    result = E2E_P02Protect(&p02Config, NULL, data);
    assert_int_equal(result, E_NOT_OK);
    
    /* Test NULL Data */
    result = E2E_P02Protect(&p02Config, &p02ProtectState, NULL);
    assert_int_equal(result, E_NOT_OK);
}

static void test_E2E_P02_Protect_Check_RoundTrip(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* Protect data */
    Std_ReturnType result = E2E_P02Protect(&p02Config, &p02ProtectState, data);
    assert_int_equal(result, E_OK);
    
    /* Check data */
    result = E2E_P02Check(&p02Config, &p02CheckState, data, 0);
    assert_int_equal(result, E_OK);
    
    /* First check should be INITIAL */
    assert_int_equal(p02CheckState.Status, E2E_P_INITIAL);
}

static void test_E2E_P02_DualPath(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* Enable dual path */
    p02Config.DualPathEnabled = TRUE;
    
    /* Protect should alternate path */
    E2E_P02Protect(&p02Config, &p02ProtectState, data);
    assert_int_equal(p02ProtectState.PathId, 1); /* Path toggled to 1 */
    
    memset(data, 0, sizeof(data));
    E2E_P02Protect(&p02Config, &p02ProtectState, data);
    assert_int_equal(p02ProtectState.PathId, 0); /* Path toggled to 0 */
}

static void test_E2E_P02_Check_InvalidPath(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* Protect data */
    E2E_P02Protect(&p02Config, &p02ProtectState, data);
    
    /* Check with invalid path ID (must be 0 or 1) */
    Std_ReturnType result = E2E_P02Check(&p02Config, &p02CheckState, data, 2);
    assert_int_equal(result, E_NOT_OK);
}

static void test_E2E_P02_Check_WrongCRC(void **state)
{
    (void)state;
    uint8 data[16] = {0};
    
    /* Protect and corrupt */
    E2E_P02Protect(&p02Config, &p02ProtectState, data);
    data[5] ^= 0xFF; /* Corrupt data */
    
    /* Check should detect wrong CRC */
    E2E_P02Check(&p02Config, &p02CheckState, data, 0);
    assert_int_equal(p02CheckState.Status, E2E_P_WRONGCRC);
}

static void test_E2E_P02_MapStatusToSM(void **state)
{
    (void)state;
    E2E_SMStateType smState;
    boolean error;
    
    /* E2E_P_OK -> VALID */
    E2E_P02MapStatusToSM(E2E_P_OK, &smState, &error);
    assert_int_equal(smState, E2E_SM_VALID);
    assert_false(error);
    
    /* E2E_P_WRONGCRC -> INVALID */
    E2E_P02MapStatusToSM(E2E_P_WRONGCRC, &smState, &error);
    assert_int_equal(smState, E2E_SM_INVALID);
    assert_true(error);
}

/*==================================================================================================
 *                              Profile 04 Tests (CRC32)
 *================================================================================================*/
static void test_E2E_P04_Protect_NullParams(void **state)
{
    (void)state;
    uint8 data[32];
    
    /* Test NULL Config */
    Std_ReturnType result = E2E_P04Protect(NULL, &p04ProtectState, data);
    assert_int_equal(result, E_NOT_OK);
    
    /* Test NULL State */
    result = E2E_P04Protect(&p04Config, NULL, data);
    assert_int_equal(result, E_NOT_OK);
    
    /* Test NULL Data */
    result = E2E_P04Protect(&p04Config, &p04ProtectState, NULL);
    assert_int_equal(result, E_NOT_OK);
}

static void test_E2E_P04_Protect_Check_RoundTrip(void **state)
{
    (void)state;
    uint8 data[32] = {0};
    
    /* Protect data */
    Std_ReturnType result = E2E_P04Protect(&p04Config, &p04ProtectState, data);
    assert_int_equal(result, E_OK);
    
    /* Verify CRC was written */
    assert_true(data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 0);
    
    /* Check data */
    result = E2E_P04Check(&p04Config, &p04CheckState, data);
    assert_int_equal(result, E_OK);
    
    /* First check should be INITIAL */
    assert_int_equal(p04CheckState.Status, E2E_P_INITIAL);
}

static void test_E2E_P04_Counter16Bit(void **state)
{
    (void)state;
    uint8 data1[32] = {0};
    uint8 data2[32] = {0};
    
    /* Set counter to high value */
    p04ProtectState.Counter = 0xFF00;
    
    /* Protect twice */
    E2E_P04Protect(&p04Config, &p04ProtectState, data1);
    uint16 counter1 = p04ProtectState.Counter;
    
    E2E_P04Protect(&p04Config, &p04ProtectState, data2);
    uint16 counter2 = p04ProtectState.Counter;
    
    /* 16-bit counter should have incremented */
    assert_int_equal(counter2, counter1 + 1);
}

static void test_E2E_P04_Check_WrongCRC(void **state)
{
    (void)state;
    uint8 data[32] = {0};
    
    /* Protect and corrupt */
    E2E_P04Protect(&p04Config, &p04ProtectState, data);
    data[10] ^= 0xFF; /* Corrupt data */
    
    /* Check should detect wrong CRC */
    E2E_P04Check(&p04Config, &p04CheckState, data);
    assert_int_equal(p04CheckState.Status, E2E_P_WRONGCRC);
}

static void test_E2E_P04_WithoutDataID(void **state)
{
    (void)state;
    uint8 data[32] = {0};
    
    /* Disable DataID inclusion */
    p04Config.IncludeDataID = FALSE;
    
    /* Protect should still work */
    Std_ReturnType result = E2E_P04Protect(&p04Config, &p04ProtectState, data);
    assert_int_equal(result, E_OK);
    
    /* Check should pass */
    result = E2E_P04Check(&p04Config, &p04CheckState, data);
    assert_int_equal(result, E_OK);
}

static void test_E2E_P04_MapStatusToSM(void **state)
{
    (void)state;
    E2E_SMStateType smState;
    boolean error;
    
    /* E2E_P_OK -> VALID */
    E2E_P04MapStatusToSM(E2E_P_OK, &smState, &error);
    assert_int_equal(smState, E2E_SM_VALID);
    assert_false(error);
    
    /* E2E_P_SYNC -> INIT */
    E2E_P04MapStatusToSM(E2E_P_SYNC, &smState, &error);
    assert_int_equal(smState, E2E_SM_INIT);
    assert_false(error);
}

/*==================================================================================================
 *                              Profile 05 Tests (CRC64)
 *================================================================================================*/
static void test_E2E_P05_Protect_NullParams(void **state)
{
    (void)state;
    uint8 data[64];
    
    /* Test NULL Config */
    Std_ReturnType result = E2E_P05Protect(NULL, &p05ProtectState, data);
    assert_int_equal(result, E2E_E_INPUTERR_NULL);
    
    /* Test NULL State */
    result = E2E_P05Protect(&p05Config, NULL, data);
    assert_int_equal(result, E2E_E_INPUTERR_NULL);
    
    /* Test NULL Data */
    result = E2E_P05Protect(&p05Config, &p05ProtectState, NULL);
    assert_int_equal(result, E2E_E_INPUTERR_NULL);
}

static void test_E2E_P05_Protect_Check_RoundTrip(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* Protect data */
    Std_ReturnType result = E2E_P05Protect(&p05Config, &p05ProtectState, data);
    assert_int_equal(result, E_OK);
    
    /* Verify CRC was written (check first few bytes) */
    assert_true(data[32] != 0 || data[33] != 0 || data[34] != 0 || data[35] != 0);
    
    /* Check data */
    result = E2E_P05Check(&p05Config, &p05CheckState, data);
    assert_int_equal(result, E_OK);
    
    /* First check should be INITIAL */
    assert_int_equal(p05CheckState.Status, E2E_P_INITIAL);
}

static void test_E2E_P05_Counter32Bit(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* Set counter to high value */
    p05ProtectState.Counter = 0xFFFFFF00U;
    
    /* Protect */
    E2E_P05Protect(&p05Config, &p05ProtectState, data);
    assert_int_equal(p05ProtectState.Counter, 0xFFFFFF01U);
    
    /* Protect at wrap-around */
    p05ProtectState.Counter = 0xFFFFFFFFU;
    memset(data, 0, sizeof(data));
    E2E_P05Protect(&p05Config, &p05ProtectState, data);
    assert_int_equal(p05ProtectState.Counter, 0U);
}

static void test_E2E_P05_Check_WrongCRC(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* Protect and corrupt */
    E2E_P05Protect(&p05Config, &p05ProtectState, data);
    data[10] ^= 0xFF; /* Corrupt data */
    
    /* Check should detect wrong CRC */
    E2E_P05Check(&p05Config, &p05CheckState, data);
    assert_int_equal(p05CheckState.Status, E2E_P_WRONGCRC);
}

static void test_E2E_P05_Check_ConsecutiveOK(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* First message */
    E2E_P05Protect(&p05Config, &p05ProtectState, data);
    E2E_P05Check(&p05Config, &p05CheckState, data);
    
    /* Second consecutive message */
    memset(data, 0, sizeof(data));
    E2E_P05Protect(&p05Config, &p05ProtectState, data);
    E2E_P05Check(&p05Config, &p05CheckState, data);
    
    assert_int_equal(p05CheckState.Status, E2E_P_OK);
}

static void test_E2E_P05_Check_SomeLost(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* First message */
    E2E_P05Protect(&p05Config, &p05ProtectState, data);
    E2E_P05Check(&p05Config, &p05CheckState, data);
    
    /* Skip 2 counters */
    p05ProtectState.Counter += 2;
    
    /* Next message */
    memset(data, 0, sizeof(data));
    E2E_P05Protect(&p05Config, &p05ProtectState, data);
    E2E_P05Check(&p05Config, &p05CheckState, data);
    
    assert_int_equal(p05CheckState.Status, E2E_P_OKSOMELOST);
}

static void test_E2E_P05_MapStatusToSM(void **state)
{
    (void)state;
    E2E_SMStateType smState;
    boolean error;
    
    /* E2E_P_OK -> VALID */
    E2E_P05MapStatusToSM(E2E_P_OK, &smState, &error);
    assert_int_equal(smState, E2E_SM_VALID);
    assert_false(error);
    
    /* E2E_P_OKSOMELOST -> VALID */
    E2E_P05MapStatusToSM(E2E_P_OKSOMELOST, &smState, &error);
    assert_int_equal(smState, E2E_SM_VALID);
    assert_false(error);
    
    /* E2E_P_WRONGCRC -> INVALID */
    E2E_P05MapStatusToSM(E2E_P_WRONGCRC, &smState, &error);
    assert_int_equal(smState, E2E_SM_INVALID);
    assert_true(error);
    
    /* E2E_P_SYNC -> INIT */
    E2E_P05MapStatusToSM(E2E_P_SYNC, &smState, &error);
    assert_int_equal(smState, E2E_SM_INIT);
    assert_false(error);
    
    /* E2E_P_INITIAL -> NODATA */
    E2E_P05MapStatusToSM(E2E_P_INITIAL, &smState, &error);
    assert_int_equal(smState, E2E_SM_NODATA);
    assert_false(error);
}

/*==================================================================================================
 *                              Profile 06 Tests (CRC64 + Dynamic Length)
 *================================================================================================*/
static void test_E2E_P06_Protect_NullParams(void **state)
{
    (void)state;
    uint8 data[128];
    
    /* Test NULL Config */
    Std_ReturnType result = E2E_P06Protect(NULL, &p06ProtectState, data, 64);
    assert_int_equal(result, E2E_E_INPUTERR_NULL);
    
    /* Test NULL State */
    result = E2E_P06Protect(&p06Config, NULL, data, 64);
    assert_int_equal(result, E2E_E_INPUTERR_NULL);
    
    /* Test NULL Data */
    result = E2E_P06Protect(&p06Config, &p06ProtectState, NULL, 64);
    assert_int_equal(result, E2E_E_INPUTERR_NULL);
}

static void test_E2E_P06_Protect_InvalidLength(void **state)
{
    (void)state;
    uint8 data[128] = {0};
    
    /* Test length too short */
    Std_ReturnType result = E2E_P06Protect(&p06Config, &p06ProtectState, data, 16);
    assert_int_equal(result, E2E_E_INPUTERR_WRONG);
    
    /* Test length too long */
    result = E2E_P06Protect(&p06Config, &p06ProtectState, data, 256);
    assert_int_equal(result, E2E_E_INPUTERR_WRONG);
}

static void test_E2E_P06_Protect_Check_RoundTrip(void **state)
{
    (void)state;
    uint8 data[128] = {0};
    
    /* Protect data with valid length */
    Std_ReturnType result = E2E_P06Protect(&p06Config, &p06ProtectState, data, 64);
    assert_int_equal(result, E_OK);
    
    /* Check data */
    result = E2E_P06Check(&p06Config, &p06CheckState, data, 64);
    assert_int_equal(result, E_OK);
    
    /* First check should be INITIAL */
    assert_int_equal(p06CheckState.Status, E2E_P_INITIAL);
}

static void test_E2E_P06_VariableLengths(void **state)
{
    (void)state;
    uint8 data[128] = {0};
    
    /* Test different valid lengths */
    uint32 lengths[] = {32, 48, 64, 96, 128};
    
    for (size_t i = 0; i < sizeof(lengths)/sizeof(lengths[0]); i++) {
        memset(&p06ProtectState, 0, sizeof(p06ProtectState));
        memset(&p06CheckState, 0, sizeof(p06CheckState));
        memset(data, 0, sizeof(data));
        
        Std_ReturnType result = E2E_P06Protect(&p06Config, &p06ProtectState, data, lengths[i]);
        assert_int_equal(result, E_OK);
        
        result = E2E_P06Check(&p06Config, &p06CheckState, data, lengths[i]);
        assert_int_equal(result, E_OK);
        assert_int_equal(p06CheckState.Status, E2E_P_INITIAL);
    }
}

static void test_E2E_P06_Check_InvalidLength(void **state)
{
    (void)state;
    uint8 data[128] = {0};
    
    /* Protect with valid length */
    E2E_P06Protect(&p06Config, &p06ProtectState, data, 64);
    
    /* Check with invalid length - should still process but CRC won't match */
    Std_ReturnType result = E2E_P06Check(&p06Config, &p06CheckState, data, 256);
    assert_int_equal(result, E_OK);
    assert_int_equal(p06CheckState.Status, E2E_P_WRONGCRC);
}

static void test_E2E_P06_Check_WrongCRC(void **state)
{
    (void)state;
    uint8 data[128] = {0};
    
    /* Protect and corrupt */
    E2E_P06Protect(&p06Config, &p06ProtectState, data, 64);
    data[10] ^= 0xFF; /* Corrupt data */
    
    /* Check should detect wrong CRC */
    E2E_P06Check(&p06Config, &p06CheckState, data, 64);
    assert_int_equal(p06CheckState.Status, E2E_P_WRONGCRC);
}

static void test_E2E_P06_MapStatusToSM(void **state)
{
    (void)state;
    E2E_SMStateType smState;
    boolean error;
    
    /* E2E_P_OK -> VALID */
    E2E_P06MapStatusToSM(E2E_P_OK, &smState, &error);
    assert_int_equal(smState, E2E_SM_VALID);
    assert_false(error);
    
    /* E2E_P_WRONGCRC -> INVALID */
    E2E_P06MapStatusToSM(E2E_P_WRONGCRC, &smState, &error);
    assert_int_equal(smState, E2E_SM_INVALID);
    assert_true(error);
    
    /* E2E_P_SYNC -> INIT */
    E2E_P06MapStatusToSM(E2E_P_SYNC, &smState, &error);
    assert_int_equal(smState, E2E_SM_INIT);
    assert_false(error);
}

/*==================================================================================================
 *                              Profile 07 Tests (CRC32 + Dynamic Length)
 *================================================================================================*/
static void test_E2E_P07_Protect_NullParams(void **state)
{
    (void)state;
    uint8 data[64];
    
    /* Test NULL Config */
    Std_ReturnType result = E2E_P07Protect(NULL, &p07ProtectState, data, 32);
    assert_int_equal(result, E2E_E_INPUTERR_NULL);
    
    /* Test NULL State */
    result = E2E_P07Protect(&p07Config, NULL, data, 32);
    assert_int_equal(result, E2E_E_INPUTERR_NULL);
    
    /* Test NULL Data */
    result = E2E_P07Protect(&p07Config, &p07ProtectState, NULL, 32);
    assert_int_equal(result, E2E_E_INPUTERR_NULL);
}

static void test_E2E_P07_Protect_InvalidLength(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* Test length too short */
    Std_ReturnType result = E2E_P07Protect(&p07Config, &p07ProtectState, data, 8);
    assert_int_equal(result, E2E_E_INPUTERR_WRONG);
    
    /* Test length too long */
    result = E2E_P07Protect(&p07Config, &p07ProtectState, data, 128);
    assert_int_equal(result, E2E_E_INPUTERR_WRONG);
}

static void test_E2E_P07_Protect_Check_RoundTrip(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* Protect data with valid length */
    Std_ReturnType result = E2E_P07Protect(&p07Config, &p07ProtectState, data, 32);
    assert_int_equal(result, E_OK);
    
    /* Check data */
    result = E2E_P07Check(&p07Config, &p07CheckState, data, 32);
    assert_int_equal(result, E_OK);
    
    /* First check should be INITIAL */
    assert_int_equal(p07CheckState.Status, E2E_P_INITIAL);
}

static void test_E2E_P07_Counter8Bit(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* Set counter near wrap-around */
    p07ProtectState.Counter = 254;
    
    E2E_P07Protect(&p07Config, &p07ProtectState, data, 32);
    assert_int_equal(p07ProtectState.Counter, 255);
    
    memset(data, 0, sizeof(data));
    E2E_P07Protect(&p07Config, &p07ProtectState, data, 32);
    assert_int_equal(p07ProtectState.Counter, 0);
}

static void test_E2E_P07_Check_WrongCRC(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* Protect and corrupt */
    E2E_P07Protect(&p07Config, &p07ProtectState, data, 32);
    data[20] ^= 0xFF; /* Corrupt data */
    
    /* Check should detect wrong CRC */
    E2E_P07Check(&p07Config, &p07CheckState, data, 32);
    assert_int_equal(p07CheckState.Status, E2E_P_WRONGCRC);
}

static void test_E2E_P07_Check_ConsecutiveOK(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* First message */
    E2E_P07Protect(&p07Config, &p07ProtectState, data, 32);
    E2E_P07Check(&p07Config, &p07CheckState, data, 32);
    
    /* Second consecutive */
    memset(data, 0, sizeof(data));
    E2E_P07Protect(&p07Config, &p07ProtectState, data, 32);
    E2E_P07Check(&p07Config, &p07CheckState, data, 32);
    
    assert_int_equal(p07CheckState.Status, E2E_P_OK);
}

static void test_E2E_P07_Check_SomeLost(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* First message */
    E2E_P07Protect(&p07Config, &p07ProtectState, data, 32);
    E2E_P07Check(&p07Config, &p07CheckState, data, 32);
    
    /* Skip 2 counters */
    p07ProtectState.Counter += 2;
    
    /* Next message */
    memset(data, 0, sizeof(data));
    E2E_P07Protect(&p07Config, &p07ProtectState, data, 32);
    E2E_P07Check(&p07Config, &p07CheckState, data, 32);
    
    assert_int_equal(p07CheckState.Status, E2E_P_OKSOMELOST);
}

static void test_E2E_P07_Check_Sync(void **state)
{
    (void)state;
    uint8 data[64] = {0};
    
    /* First message */
    E2E_P07Protect(&p07Config, &p07ProtectState, data, 32);
    E2E_P07Check(&p07CheckState, &p07CheckState, data, 32);
    
    /* Skip many counters */
    p07ProtectState.Counter += 10;
    
    /* Next message */
    memset(data, 0, sizeof(data));
    E2E_P07Protect(&p07Config, &p07ProtectState, data, 32);
    E2E_P07Check(&p07Config, &p07CheckState, data, 32);
    
    assert_int_equal(p07CheckState.Status, E2E_P_SYNC);
}

static void test_E2E_P07_MapStatusToSM(void **state)
{
    (void)state;
    E2E_SMStateType smState;
    boolean error;
    
    /* E2E_P_OK -> VALID */
    E2E_P07MapStatusToSM(E2E_P_OK, &smState, &error);
    assert_int_equal(smState, E2E_SM_VALID);
    assert_false(error);
    
    /* E2E_P_WRONGCRC -> INVALID */
    E2E_P07MapStatusToSM(E2E_P_WRONGCRC, &smState, &error);
    assert_int_equal(smState, E2E_SM_INVALID);
    assert_true(error);
    
    /* E2E_P_SYNC -> INIT */
    E2E_P07MapStatusToSM(E2E_P_SYNC, &smState, &error);
    assert_int_equal(smState, E2E_SM_INIT);
    assert_false(error);
    
    /* E2E_P_INITIAL -> NODATA */
    E2E_P07MapStatusToSM(E2E_P_INITIAL, &smState, &error);
    assert_int_equal(smState, E2E_SM_NODATA);
    assert_false(error);
}

/*==================================================================================================
 *                              Error Code Tests
 *================================================================================================*/
static void test_E2E_ErrorCodes(void **state)
{
    (void)state;
    
    /* Verify error code values */
    assert_int_equal(E2E_E_OK, 0x00U);
    assert_int_equal(E2E_E_NOT_OK, 0x01U);
    assert_int_equal(E2E_E_INPUTERR_NULL, 0x13U);
    assert_int_equal(E2E_E_INPUTERR_WRONG, 0x15U);
    assert_int_equal(E2E_E_INTERR, 0x19U);
    assert_int_equal(E2E_E_OK_SOMELOST, 0x26U);
}

/*==================================================================================================
 *                              Version Info Tests
 *================================================================================================*/
static void test_E2E_VersionInfo(void **state)
{
    (void)state;
    
    /* Verify version defines exist */
    #if (E2E_VERSION_INFO_API == STD_ON)
    assert_int_equal(E2E_VENDOR_ID, 0x0001U);
    assert_int_equal(E2E_MODULE_ID, 0xF0U);
    #endif
}

/*==================================================================================================
 *                                      Main Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        /* E2E Library Tests */
        cmocka_unit_test_setup_teardown(test_E2E_Init_Valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_DeInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_VersionInfo, setup, teardown),
        
        /* Error Codes */
        cmocka_unit_test(test_E2E_ErrorCodes),
        
        /* Profile 01 Tests */
        cmocka_unit_test_setup_teardown(test_E2E_P01_Protect_NullParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P01_Protect_Check_RoundTrip, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P01_CounterIncrement, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P01_CounterWrapAround, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P01_Check_NullParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P01_Check_WrongCRC, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P01_Check_ConsecutiveOK, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P01_Check_Repeated, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P01_Check_SomeLost, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P01_Check_Sync, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P01_DataIDModes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P01_MapStatusToSM, setup, teardown),
        
        /* Profile 02 Tests */
        cmocka_unit_test_setup_teardown(test_E2E_P02_Protect_NullParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P02_Protect_Check_RoundTrip, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P02_DualPath, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P02_Check_InvalidPath, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P02_Check_WrongCRC, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P02_MapStatusToSM, setup, teardown),
        
        /* Profile 04 Tests */
        cmocka_unit_test_setup_teardown(test_E2E_P04_Protect_NullParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P04_Protect_Check_RoundTrip, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P04_Counter16Bit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P04_Check_WrongCRC, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P04_WithoutDataID, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P04_MapStatusToSM, setup, teardown),
        
        /* Profile 05 Tests */
        cmocka_unit_test_setup_teardown(test_E2E_P05_Protect_NullParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P05_Protect_Check_RoundTrip, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P05_Counter32Bit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P05_Check_WrongCRC, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P05_Check_ConsecutiveOK, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P05_Check_SomeLost, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P05_MapStatusToSM, setup, teardown),
        
        /* Profile 06 Tests */
        cmocka_unit_test_setup_teardown(test_E2E_P06_Protect_NullParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P06_Protect_InvalidLength, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P06_Protect_Check_RoundTrip, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P06_VariableLengths, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P06_Check_InvalidLength, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P06_Check_WrongCRC, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P06_MapStatusToSM, setup, teardown),
        
        /* Profile 07 Tests */
        cmocka_unit_test_setup_teardown(test_E2E_P07_Protect_NullParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P07_Protect_InvalidLength, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P07_Protect_Check_RoundTrip, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P07_Counter8Bit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P07_Check_WrongCRC, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P07_Check_ConsecutiveOK, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P07_Check_SomeLost, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P07_Check_Sync, setup, teardown),
        cmocka_unit_test_setup_teardown(test_E2E_P07_MapStatusToSM, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}
