/**
 * @file test_crc.c
 * @brief Crc Unit Tests
 * @version 1.0.0
 * @date 2026-09-12
 */

// @tests src/bsw/services/crc/src/Crc.c  @tests src/bsw/services/crc/include/Crc.h

#include "unity.h"
#include "Crc.h"

void setUp(void)
{
    Crc_Init(NULL_PTR);
}

void tearDown(void)
{
}

/** @req SWS_Crc_00001 */
void test_Crc_Init_NullConfig_ShouldSucceed(void)
{
    Crc_Init(NULL_PTR);
    /* No crash and subsequent CRC calls work. */
}

/** @req SWS_Crc_00001 */
void test_Crc_CalculateCRC8_ZeroLen_ShouldReturnInitXorOut(void)
{
    uint8 data[1] = {0};
    uint8 result = Crc_CalculateCRC8(data, 0U, 0U, TRUE);
    TEST_ASSERT_EQUAL_HEX8(0x00U, result);
}

/** @req SWS_Crc_00001 */
void test_Crc_CalculateCRC8_ValidData_ShouldReturnExpectedCRC(void)
{
    uint8 data[4] = {1U, 2U, 3U, 4U};
    uint8 result = Crc_CalculateCRC8(data, 4U, 0U, TRUE);
    TEST_ASSERT_EQUAL_HEX8(0x67U, result);
}

/** @req SWS_Crc_00001 */
void test_Crc_CalculateCRC8_NotFirstCall_ShouldUseStartValue(void)
{
    uint8 data[2] = {1U, 2U};
    uint8 first = Crc_CalculateCRC8(data, 2U, 0U, TRUE);
    uint8 chained = Crc_CalculateCRC8(data, 2U, first, FALSE);
    TEST_ASSERT_NOT_EQUAL(first, chained);
}

/** @req SWS_Crc_00003 */
void test_Crc_CalculateCRC16_ZeroLen_ShouldReturnInitXorOut(void)
{
    uint8 data[1] = {0};
    uint16 result = Crc_CalculateCRC16(data, 0U, 0U, TRUE);
    TEST_ASSERT_EQUAL_HEX16(0xFFFFU, result);
}

/** @req SWS_Crc_00003 */
void test_Crc_CalculateCRC16_ValidData_ShouldReturnExpectedCRC(void)
{
    uint8 data[4] = {1U, 2U, 3U, 4U};
    uint16 result = Crc_CalculateCRC16(data, 4U, 0U, TRUE);
    TEST_ASSERT_EQUAL_HEX16(0x89C3U, result);
}

/** @req SWS_Crc_00003 */
void test_Crc_CalculateCRC16_NotFirstCall_ShouldUseStartValue(void)
{
    uint8 data[2] = {1U, 2U};
    uint16 first = Crc_CalculateCRC16(data, 2U, 0U, TRUE);
    uint16 chained = Crc_CalculateCRC16(data, 2U, first, FALSE);
    TEST_ASSERT_NOT_EQUAL(first, chained);
}

/** @req SWS_Crc_00004 */
void test_Crc_CalculateCRC32_ZeroLen_ShouldReturnInitXorOut(void)
{
    uint8 data[1] = {0};
    uint32 result = Crc_CalculateCRC32(data, 0U, 0U, TRUE);
    TEST_ASSERT_EQUAL_HEX32(0x00000000U, result);
}

/** @req SWS_Crc_00004 */
void test_Crc_CalculateCRC32_ValidData_ShouldReturnExpectedCRC(void)
{
    uint8 data[4] = {1U, 2U, 3U, 4U};
    uint32 result = Crc_CalculateCRC32(data, 4U, 0U, TRUE);
    TEST_ASSERT_EQUAL_HEX32(0x86C8C832U, result);
}

/** @req SWS_Crc_00004 */
void test_Crc_CalculateCRC32_NotFirstCall_ShouldUseStartValue(void)
{
    uint8 data[2] = {1U, 2U};
    uint32 first = Crc_CalculateCRC32(data, 2U, 0U, TRUE);
    uint32 chained = Crc_CalculateCRC32(data, 2U, first, FALSE);
    TEST_ASSERT_NOT_EQUAL(first, chained);
}

/** @req SWS_Crc_00002 */
void test_Crc_GetVersionInfo_ValidPtr_ShouldReturnCorrectVersion(void)
{
    Std_VersionInfoType versionInfo;
    Crc_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQUAL_HEX16(CRC_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL_HEX16(CRC_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL(CRC_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL(CRC_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL(CRC_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}

/** @req SWS_Crc_00002 */
void test_Crc_GetVersionInfo_Twice_ShouldReturnSameVersion(void)
{
    Std_VersionInfoType v1;
    Std_VersionInfoType v2;
    Crc_GetVersionInfo(&v1);
    Crc_GetVersionInfo(&v2);
    TEST_ASSERT_EQUAL(v1.vendorID, v2.vendorID);
    TEST_ASSERT_EQUAL(v1.moduleID, v2.moduleID);
    TEST_ASSERT_EQUAL(v1.sw_major_version, v2.sw_major_version);
    TEST_ASSERT_EQUAL(v1.sw_minor_version, v2.sw_minor_version);
    TEST_ASSERT_EQUAL(v1.sw_patch_version, v2.sw_patch_version);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Crc_Init_NullConfig_ShouldSucceed);
    RUN_TEST(test_Crc_CalculateCRC8_ZeroLen_ShouldReturnInitXorOut);
    RUN_TEST(test_Crc_CalculateCRC8_ValidData_ShouldReturnExpectedCRC);
    RUN_TEST(test_Crc_CalculateCRC8_NotFirstCall_ShouldUseStartValue);
    RUN_TEST(test_Crc_CalculateCRC16_ZeroLen_ShouldReturnInitXorOut);
    RUN_TEST(test_Crc_CalculateCRC16_ValidData_ShouldReturnExpectedCRC);
    RUN_TEST(test_Crc_CalculateCRC16_NotFirstCall_ShouldUseStartValue);
    RUN_TEST(test_Crc_CalculateCRC32_ZeroLen_ShouldReturnInitXorOut);
    RUN_TEST(test_Crc_CalculateCRC32_ValidData_ShouldReturnExpectedCRC);
    RUN_TEST(test_Crc_CalculateCRC32_NotFirstCall_ShouldUseStartValue);
    RUN_TEST(test_Crc_GetVersionInfo_ValidPtr_ShouldReturnCorrectVersion);
    RUN_TEST(test_Crc_GetVersionInfo_Twice_ShouldReturnSameVersion);

    return UnityEnd();
}
