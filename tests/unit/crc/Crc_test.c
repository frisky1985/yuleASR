/**
 * @file Crc_test.c
 * @brief CRC Module Unit Tests (Unity Framework)
 *
 * Test coverage:
 * - Crc_Init module initialization
 * - Crc_CalculateCRC8 with known values, NULL pointer, zero length
 * - Crc_CalculateCRC16 with known values
 * - Crc_CalculateCRC32 with known values
 * - Crc_GetVersionInfo version information
 *
 * Compile with -DCRC_DEV_ERROR_DETECT=STD_ON to enable DET error path testing.
 * Compile with table-mode overrides as needed (default uses pre-compile config).
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

/*==================================================================================================
 * INCLUDES
 *=================================================================================================*/
#include "unity.h"
#include "Crc.h"
#include "Crc_Cfg.h"
#include <string.h>

/*==================================================================================================
 * NULL_PTR GUARD
 *
 * Std_Types.h defines NULL_PTR inside #ifndef NULL, which is skipped on
 * platforms where NULL is already defined by system headers (macOS, etc.).
 *=================================================================================================*/
#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/*==================================================================================================
 * DET STUB (inline)
 *
 * When CRC_DEV_ERROR_DETECT is STD_ON, Crc.c calls Det_ReportError.
 * Provide a minimal stub so the test compiles cleanly.
 *=================================================================================================*/
#include "Std_Types.h"

#ifndef CRC_DEV_ERROR_DETECT_IS_ON
#  if (CRC_DEV_ERROR_DETECT == STD_ON)
#    define CRC_DEV_ERROR_DETECT_IS_ON 1
#  else
#    define CRC_DEV_ERROR_DETECT_IS_ON 0
#  endif
#endif

#if CRC_DEV_ERROR_DETECT_IS_ON
/* For unit testing, override the STUB_ON to ensure Det calls are harmless */
#  ifndef DET_STUB_H
#    define DET_STUB_H
static int Det_ReportError_CallCount = 0;
static uint16 Det_ReportError_LastModuleId = 0;
static uint16 Det_ReportError_LastErrorId = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)InstanceId;
    (void)ApiId;
    Det_ReportError_CallCount++;
    Det_ReportError_LastModuleId = ModuleId;
    Det_ReportError_LastErrorId = ErrorId;
    return E_OK;
}
#  endif /* DET_STUB_H */
#endif /* CRC_DEV_ERROR_DETECT_IS_ON */

/*==================================================================================================
 * KNOWN TEST VECTORS
 *
 * Verified against standard CRC algorithm implementations:
 *
 * CRC8 SAE J1850 (poly=0x1D, init=0xFF, xor=0xFF):
 *   "123" = {0x31,0x32,0x33} → 0x62
 *   "Hello"                  → 0x68
 *   "123456789"              → 0x4B  (standard test vector)
 *
 * CRC16 CCITT-FALSE (poly=0x1021, init=0xFFFF, xor=0x0000):
 *   "123" = {0x31,0x32,0x33} → 0x5BCE
 *   "Hello"                  → 0xDADA
 *
 * CRC32 IEEE 802.3 (poly=0x04C11DB7, init=0xFFFFFFFF, xor=0xFFFFFFFF):
 *   "123" = {0x31,0x32,0x33} → 0x26AD0E9B
 *   "Hello"                  → 0x1A546492
 *=================================================================================================*/
static const uint8 TestData_123[] = {0x31, 0x32, 0x33};
static const uint8 TestData_Hello[] = {'H', 'e', 'l', 'l', 'o'};

/*==================================================================================================
 * SETUP / TEARDOWN
 *=================================================================================================*/
void setUp(void)
{
#if CRC_DEV_ERROR_DETECT_IS_ON
    Det_ReportError_CallCount = 0;
    Det_ReportError_LastModuleId = 0;
    Det_ReportError_LastErrorId = 0;
#endif
}

void tearDown(void)
{
    /* Nothing to clean up after each test */
}

/*==================================================================================================
 * TEST: Crc_Init
 *=================================================================================================*/

/**
 * @brief Verify Crc_Init sets module state to initialized.
 *
 * The function accepts a NULL configPtr (pre-compile config).
 * After calling Crc_Init, subsequent CRC calculations should succeed.
 */
void test_Crc_Init_SetsModuleState(void)
{
    uint8 result;

    /* Initialize with NULL config (pre-compile = no runtime config needed) */
    Crc_Init(NULL_PTR);

    /* After init, CRC calculation should work */
    result = Crc_CalculateCRC8(TestData_123, sizeof(TestData_123), 0xFFU, TRUE);
    TEST_ASSERT_EQUAL_UINT8(0x62U, result);
}

/*==================================================================================================
 * TEST: Crc_CalculateCRC8
 *=================================================================================================*/

/**
 * @brief Verify CRC8 of known data "123" produces the expected SAE J1850 value.
 */
void test_Crc_CalculateCRC8_KnownValue(void)
{
    uint8 result;

    Crc_Init(NULL_PTR);
    result = Crc_CalculateCRC8(TestData_123, sizeof(TestData_123), 0xFFU, TRUE);
    TEST_ASSERT_EQUAL_UINT8(0x62U, result);
}

/**
 * @brief Verify CRC8 of "Hello" produces the expected value.
 */
void test_Crc_CalculateCRC8_Hello(void)
{
    uint8 result;

    Crc_Init(NULL_PTR);
    result = Crc_CalculateCRC8(TestData_Hello, sizeof(TestData_Hello), 0xFFU, TRUE);
    TEST_ASSERT_EQUAL_UINT8(0x68U, result);
}

#if (CRC_DEV_ERROR_DETECT == STD_ON)
/**
 * @brief Verify CRC8 with NULL data pointer returns 0 and reports DET error.
 */
void test_Crc_CalculateCRC8_NullPointer_ReturnsZero(void)
{
    uint8 result;

    Crc_Init(NULL_PTR);
    Det_ReportError_CallCount = 0;

    result = Crc_CalculateCRC8(NULL_PTR, 5U, 0xFFU, TRUE);
    TEST_ASSERT_EQUAL_UINT8(0U, result);
    TEST_ASSERT(Det_ReportError_CallCount > 0);
}

/**
 * @brief Verify CRC8 with zero length returns 0 and reports DET error.
 */
void test_Crc_CalculateCRC8_ZeroLength_ReturnsZero(void)
{
    uint8 result;

    Crc_Init(NULL_PTR);
    Det_ReportError_CallCount = 0;

    result = Crc_CalculateCRC8(TestData_123, 0U, 0xFFU, TRUE);
    TEST_ASSERT_EQUAL_UINT8(0U, result);
    TEST_ASSERT(Det_ReportError_CallCount > 0);
}
#endif /* CRC_DEV_ERROR_DETECT == STD_ON */

/*==================================================================================================
 * TEST: Crc_CalculateCRC16
 *=================================================================================================*/

/**
 * @brief Verify CRC16 CCITT-FALSE of known data "123" produces the expected value.
 */
void test_Crc_CalculateCRC16_KnownValue(void)
{
    uint16 result;

    Crc_Init(NULL_PTR);
    result = Crc_CalculateCRC16(TestData_123, sizeof(TestData_123), 0xFFFFU, TRUE);
    TEST_ASSERT_EQUAL_UINT16(0x5BCEU, result);
}

/**
 * @brief Verify CRC16 CCITT-FALSE of "Hello" produces the expected value.
 */
void test_Crc_CalculateCRC16_Hello(void)
{
    uint16 result;

    Crc_Init(NULL_PTR);
    result = Crc_CalculateCRC16(TestData_Hello, sizeof(TestData_Hello), 0xFFFFU, TRUE);
    TEST_ASSERT_EQUAL_UINT16(0xDADAU, result);
}

/*==================================================================================================
 * TEST: Crc_CalculateCRC32
 *=================================================================================================*/

/**
 * @brief Verify CRC32 IEEE 802.3 of known data "123" produces the expected value.
 */
void test_Crc_CalculateCRC32_KnownValue(void)
{
    uint32 result;

    Crc_Init(NULL_PTR);
    result = Crc_CalculateCRC32(TestData_123, sizeof(TestData_123), 0xFFFFFFFFU, TRUE);
    TEST_ASSERT_EQUAL_UINT32(0x26AD0E9BU, result);
}

/**
 * @brief Verify CRC32 IEEE 802.3 of "Hello" produces the expected value.
 */
void test_Crc_CalculateCRC32_Hello(void)
{
    uint32 result;

    Crc_Init(NULL_PTR);
    result = Crc_CalculateCRC32(TestData_Hello, sizeof(TestData_Hello), 0xFFFFFFFFU, TRUE);
    TEST_ASSERT_EQUAL_UINT32(0x1A546492U, result);
}

/*==================================================================================================
 * TEST: Crc_GetVersionInfo
 *=================================================================================================*/

#if (CRC_VERSION_INFO_API == STD_ON)
/**
 * @brief Verify Crc_GetVersionInfo returns correct vendor/module/version info.
 */
void test_Crc_GetVersionInfo_ReturnsCorrectInfo(void)
{
    Std_VersionInfoType versionInfo;

    memset(&versionInfo, 0, sizeof(versionInfo));
    Crc_GetVersionInfo(&versionInfo);

    TEST_ASSERT_EQUAL_UINT16(CRC_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL_UINT16(CRC_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL_UINT8(CRC_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(CRC_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(CRC_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}
#endif /* CRC_VERSION_INFO_API */
