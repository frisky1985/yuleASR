/**
 * @file EcuC_test.c
 * @brief EcuC (ECU Configuration) Module Unit Tests (Unity Framework)
 *
 * Test coverage:
 * - EcuC_Init with NULL config → returns early, module remains uninitialized
 * - EcuC_Init with valid config → state set to initialized
 * - EcuC_GetConfigValue for CoreFrequency
 * - EcuC_SetConfigValue updates configuration value
 * - EcuC_GetVersionInfo returns correct version information
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

/*==================================================================================================
 * INCLUDES
 *=================================================================================================*/
#include "unity.h"
#include "EcuC.h"
#include "EcuC_Cfg.h"
#include <string.h>

/*==================================================================================================
 * NULL_PTR GUARD
 *
 * Std_Types.h defines NULL_PTR inside #ifndef NULL, which is skipped on
 * platforms where NULL is already defined by system headers.
 *=================================================================================================*/
#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/*==================================================================================================
 * DET STUB (inline)
 *
 * EcuC calls Det_ReportError when ECUC_DEV_ERROR_DETECT is STD_ON.
 * Provide a minimal stub for test isolation.
 *=================================================================================================*/
#include "Std_Types.h"

static int EcuC_Det_CallCount = 0;
static uint16 EcuC_Det_LastModuleId = 0;
static uint8 EcuC_Det_LastApiId = 0;
static uint8 EcuC_Det_LastErrorId = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)InstanceId;
    EcuC_Det_CallCount++;
    EcuC_Det_LastModuleId = ModuleId;
    EcuC_Det_LastApiId = ApiId;
    EcuC_Det_LastErrorId = ErrorId;
    return E_OK;
}

/*==================================================================================================
 * TEST DATA
 *=================================================================================================*/

/**
 * @brief Sample configuration for testing.
 */
static EcuC_ConfigType TestConfig;

/**
 * @brief A "reduced" config for testing just the core fields.
 * Only CoreFrequency needs to be non-zero for the core-freq test.
 */
static EcuC_ConfigType MinimalConfig;

/*==================================================================================================
 * SETUP / TEARDOWN
 *=================================================================================================*/
void setUp(void)
{
    /* Reset DET call tracking */
    EcuC_Det_CallCount = 0;
    EcuC_Det_LastModuleId = 0;
    EcuC_Det_LastApiId = 0;
    EcuC_Det_LastErrorId = 0;

    /* Initialize test config with known values */
    memset(&TestConfig, 0, sizeof(TestConfig));
    TestConfig.CoreFrequency = 120000000UL; /* 120 MHz */
    TestConfig.BusFrequency  = 100000000UL; /* 100 MHz */
    TestConfig.RamSize       = 0x20000000UL; /* 512 MB */
    TestConfig.FlashSize     = 0x10000000UL; /* 256 MB */
    TestConfig.EepromSize    = 0x00010000UL; /* 64 KB */
    TestConfig.CanBaudrate   = 500000UL;     /* 500 kbps */
    TestConfig.LinBaudrate   = 19200UL;      /* 19.2 kbps */
    TestConfig.SpiFrequency  = 10000000UL;   /* 10 MHz */

    /* Minimal config: just core frequency */
    memset(&MinimalConfig, 0, sizeof(MinimalConfig));
    MinimalConfig.CoreFrequency = 80000000UL; /* 80 MHz */
}

void tearDown(void)
{
    /* Ensure module is de-initialized after each test */
    EcuC_DeInit();
}

/*==================================================================================================
 * TEST: EcuC_Init
 *=================================================================================================*/

/**
 * @brief Verify EcuC_Init with NULL config pointer returns immediately
 *        and reports a DET error (when DEV_ERROR_DETECT is enabled).
 */
void test_EcuC_Init_NullConfig_ReturnsEarly(void)
{
    Std_ReturnType result;

    /* Try to get a config value BEFORE init — should fail (uninit state) */
    uint32 value = 0xAAAAAAAAU;
    result = EcuC_GetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, &value);

    /* System is uninit, should return E_NOT_OK and value unchanged
     * (but some implementations may still write; we primarily check return) */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);

    /* Now call init with NULL — DET should fire */
    EcuC_Init(NULL_PTR);

#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    TEST_ASSERT(EcuC_Det_CallCount > 0);
#endif
}

/**
 * @brief Verify EcuC_Init with a valid config sets the module state to
 *        initialized and config values are readable.
 */
void test_EcuC_Init_ValidConfig_SetsState(void)
{
    Std_ReturnType result;
    uint32 value;

    /* Initialize with valid config */
    EcuC_Init(&TestConfig);

    /* Read back core frequency — should succeed */
    result = EcuC_GetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, &value);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT32(120000000UL, value);
}

/*==================================================================================================
 * TEST: EcuC_GetConfigValue
 *=================================================================================================*/

/**
 * @brief Verify EcuC_GetConfigValue retrieves the correct CoreFrequency.
 */
void test_EcuC_GetConfigValue_CoreFreq_ReturnsValue(void)
{
    Std_ReturnType result;
    uint32 value;

    EcuC_Init(&TestConfig);

    result = EcuC_GetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, &value);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT32(120000000UL, value);
}

/**
 * @brief Verify EcuC_GetConfigValue retrieves the correct BusFrequency.
 */
void test_EcuC_GetConfigValue_BusFreq_ReturnsValue(void)
{
    Std_ReturnType result;
    uint32 value;

    EcuC_Init(&TestConfig);

    result = EcuC_GetConfigValue(ECUC_CONFIG_ID_BUS_FREQ, &value);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT32(100000000UL, value);
}

/*==================================================================================================
 * TEST: EcuC_SetConfigValue
 *=================================================================================================*/

/**
 * @brief Verify EcuC_SetConfigValue updates a configuration value that can
 *        then be read back.
 */
void test_EcuC_SetConfigValue_UpdatesValue(void)
{
    Std_ReturnType result;
    uint32 value;

    EcuC_Init(&TestConfig);

    /* Change CoreFrequency from 120 MHz to 150 MHz */
    result = EcuC_SetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, 150000000UL);
    TEST_ASSERT_EQUAL(E_OK, result);

    /* Read back — should reflect the new value */
    result = EcuC_GetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, &value);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT32(150000000UL, value);
}

/**
 * @brief Verify EcuC_SetConfigValue with an unknown ConfigId returns E_NOT_OK.
 */
void test_EcuC_SetConfigValue_InvalidId_ReturnsError(void)
{
    Std_ReturnType result;

    EcuC_Init(&TestConfig);

    /* Use a ConfigId that doesn't exist in the switch */
    result = EcuC_SetConfigValue(0xFFU, 12345UL);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================================================================================================
 * TEST: EcuC_GetVersionInfo
 *=================================================================================================*/

/**
 * @brief Verify EcuC_GetVersionInfo returns correct vendor, module, and
 *        software version information.
 */
void test_EcuC_GetVersionInfo_ReturnsCorrectInfo(void)
{
    Std_VersionInfoType versionInfo;

    memset(&versionInfo, 0, sizeof(versionInfo));
    EcuC_GetVersionInfo(&versionInfo);

    TEST_ASSERT_EQUAL_UINT16(ECUC_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL_UINT16(ECUC_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL_UINT8(1U, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(0U, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(0U, versionInfo.sw_patch_version);
}

/*==================================================================================================
 * TEST: EcuC_GetConfigValue — Error Paths
 *=================================================================================================*/

#if (ECUC_DEV_ERROR_DETECT == STD_ON)
/**
 * @brief Verify EcuC_GetConfigValue with NULL value pointer returns E_NOT_OK
 *        and reports a DET error.
 */
void test_EcuC_GetConfigValue_NullPointer_ReturnsError(void)
{
    Std_ReturnType result;

    EcuC_Init(&TestConfig);
    EcuC_Det_CallCount = 0;

    /* Call with NULL output pointer */
    result = EcuC_GetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    TEST_ASSERT(EcuC_Det_CallCount > 0);
}
#endif /* ECUC_DEV_ERROR_DETECT == STD_ON */
