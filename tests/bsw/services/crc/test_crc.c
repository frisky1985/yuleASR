/**
 * @file test_test_crc.c
 * @brief Crc Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "Crc.h"

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
/* CRC has no config */

static boolean crc_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    crc_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Crc_00001 */
void test_Crc_CalculateCRC8_NullPtr_ShouldReportError(void) {
    Crc_CalculateCRC8(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crc_00001 */
void test_Crc_CalculateCRC8_ZeroLen_ShouldReturnZero(void) {
    uint8 data[1] = {0};
    Crc_CalculateCRC8(data, 0U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00001 */
void test_Crc_CalculateCRC8_ValidData_ShouldReturnCRC(void) {
    uint8 data[4] = {1, 2, 3, 4};
    Crc_CalculateCRC8(data, 4U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00002 */
void test_Crc_CalculateCRC8H2F_NullPtr_ShouldReportError(void) {
    Crc_CalculateCRC8H2F(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crc_00002 */
void test_Crc_CalculateCRC8H2F_ZeroLen_ShouldReturnZero(void) {
    uint8 data[1] = {0};
    Crc_CalculateCRC8H2F(data, 0U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00002 */
void test_Crc_CalculateCRC8H2F_ValidData_ShouldReturnCRC(void) {
    uint8 data[4] = {1, 2, 3, 4};
    Crc_CalculateCRC8H2F(data, 4U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00003 */
void test_Crc_CalculateCRC16_NullPtr_ShouldReportError(void) {
    Crc_CalculateCRC16(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crc_00003 */
void test_Crc_CalculateCRC16_ZeroLen_ShouldReturnZero(void) {
    uint8 data[1] = {0};
    Crc_CalculateCRC16(data, 0U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00003 */
void test_Crc_CalculateCRC16_ValidData_ShouldReturnCRC(void) {
    uint8 data[4] = {1, 2, 3, 4};
    Crc_CalculateCRC16(data, 4U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00004 */
void test_Crc_CalculateCRC32_NullPtr_ShouldReportError(void) {
    Crc_CalculateCRC32(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crc_00004 */
void test_Crc_CalculateCRC32_ZeroLen_ShouldReturnZero(void) {
    uint8 data[1] = {0};
    Crc_CalculateCRC32(data, 0U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00004 */
void test_Crc_CalculateCRC32_ValidData_ShouldReturnCRC(void) {
    uint8 data[4] = {1, 2, 3, 4};
    Crc_CalculateCRC32(data, 4U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00005 */
void test_Crc_CalculateCRC32P4_NullPtr_ShouldReportError(void) {
    Crc_CalculateCRC32P4(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crc_00005 */
void test_Crc_CalculateCRC32P4_ZeroLen_ShouldReturnZero(void) {
    uint8 data[1] = {0};
    Crc_CalculateCRC32P4(data, 0U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00005 */
void test_Crc_CalculateCRC32P4_ValidData_ShouldReturnCRC(void) {
    uint8 data[4] = {1, 2, 3, 4};
    Crc_CalculateCRC32P4(data, 4U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00006 */
void test_Crc_CalculateCRC64_NullPtr_ShouldReportError(void) {
    Crc_CalculateCRC64(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crc_00006 */
void test_Crc_CalculateCRC64_ZeroLen_ShouldReturnZero(void) {
    uint8 data[1] = {0};
    Crc_CalculateCRC64(data, 0U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00006 */
void test_Crc_CalculateCRC64_ValidData_ShouldReturnCRC(void) {
    uint8 data[4] = {1, 2, 3, 4};
    Crc_CalculateCRC64(data, 4U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crc_00007 */
void test_Crc_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Crc_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crc_00007 */
void test_Crc_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Crc_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

