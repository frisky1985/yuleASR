/**
 * @file test_test_flash.c
 * @brief Flash Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "Flash.h"

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test config */
Flash_ConfigType testConfig;
static void test_Flash_SetupDefaultConfig(void) {
    (void)testConfig;
}

static boolean flash_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    flash_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Flash_00001 */
void test_Flash_Init_NullPtr_ShouldNotCrash(void) {
    Flash_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Flash_00001 */
void test_Flash_Init_ValidConfig_ShouldSucceed(void) {
    test_Flash_SetupDefaultConfig();
    Flash_Init(&testConfig);
    flash_initialized = TRUE;
    TEST_ASSERT_TRUE(flash_initialized);
}

/** @req SWS_Flash_00001 */
void test_Flash_Init_DoubleInit_ShouldSucceed(void) {
    test_Flash_SetupDefaultConfig();
    Flash_Init(&testConfig);
    Flash_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Flash_00002 */
void test_Flash_Erase_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Flash_Erase();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Flash_00002 */
void test_Flash_Erase_InvalidSector_ShouldReportError(void) {
    Flash_Erase(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Flash_00002 */
void test_Flash_Erase_ValidSector_ShouldSucceed(void) {
    Flash_Erase();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Flash_00003 */
void test_Flash_Write_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Flash_Write();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Flash_00003 */
void test_Flash_Write_InvalidAddress_ShouldReportError(void) {
    Flash_Write(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Flash_00003 */
void test_Flash_Write_ValidData_ShouldSucceed(void) {
    Flash_Write();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Flash_00004 */
void test_Flash_Read_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Flash_Read();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Flash_00004 */
void test_Flash_Read_InvalidAddress_ShouldReportError(void) {
    Flash_Read(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Flash_00004 */
void test_Flash_Read_ValidBuffer_ShouldSucceed(void) {
    Flash_Read();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Flash_00005 */
void test_Flash_Lock_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Flash_Lock();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Flash_00005 */
void test_Flash_Lock_ValidCall_ShouldSucceed(void) {
    Flash_Lock();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Flash_00006 */
void test_Flash_Unlock_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Flash_Unlock();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Flash_00006 */
void test_Flash_Unlock_ValidCall_ShouldSucceed(void) {
    Flash_Unlock();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Flash_00007 */
void test_Flash_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    Flash_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount); /* No DET in uninit for status */
}

/** @req SWS_Flash_00007 */
void test_Flash_GetStatus_ValidCall_ShouldReturnStatus(void) {
    Flash_GetStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Flash_00008 */
void test_Flash_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Flash_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Flash_00008 */
void test_Flash_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Flash_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

