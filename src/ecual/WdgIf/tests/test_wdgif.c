/******************************************************************************
 * @file    test_wdgif.c
 * @brief   Watchdog Interface Module Tests
 *
 * Unit tests for the Watchdog Interface module.
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ecual/wdgIf/WdgIf.h"
#include "ecual/wdgIf/WdgIf_Hw_Emulator.h"

/******************************************************************************
 * Test Configuration
 ******************************************************************************/
static const WdgIf_TimeoutConfigType testTimeoutConfig = {
    .slowModeTimeout = 500U,
    .fastModeTimeout = 50U,
    .windowStartPercent = 25U,
    .windowEndPercent = 75U
};

static const WdgIf_SafetyConfigType testSafetyConfig = {
    .enableSafetyChecks = TRUE,
    .maxConsecutiveErrors = 3U,
    .maxTriggerIntervalMs = 100U,
    .enableTimeWindowCheck = FALSE,
    .enableResetOnFailure = FALSE
};

static WdgIf_EmulatorConfigType testEmuHwConfig = {
    .simulatedClockFreq = 1000000U,
    .simulateWindowViolation = FALSE,
    .simulateTimeout = FALSE,
    .triggerFailureRate = 0U
};

static const WdgIf_DeviceConfigType testDeviceConfigs[WDGIF_MAX_DEVICES] = {
    /* Internal WDG */
    {
        .deviceId = WDGIF_INTERNAL_WDG,
        .enabled = TRUE,
        .hardwareType = WDGIF_HW_EMULATOR,
        .timeoutConfig = &testTimeoutConfig,
        .safetyConfig = &testSafetyConfig,
        .demConfig = NULL_PTR,
        .hardwareConfig = &testEmuHwConfig
    },
    /* External WDG1 */
    {
        .deviceId = WDGIF_EXTERNAL_WDG1,
        .enabled = TRUE,
        .hardwareType = WDGIF_HW_EMULATOR,
        .timeoutConfig = &testTimeoutConfig,
        .safetyConfig = &testSafetyConfig,
        .demConfig = NULL_PTR,
        .hardwareConfig = &testEmuHwConfig
    },
    /* External WDG2 */
    {
        .deviceId = WDGIF_EXTERNAL_WDG2,
        .enabled = TRUE,
        .hardwareType = WDGIF_HW_EMULATOR,
        .timeoutConfig = &testTimeoutConfig,
        .safetyConfig = &testSafetyConfig,
        .demConfig = NULL_PTR,
        .hardwareConfig = &testEmuHwConfig
    }
};

static const WdgIf_ConfigType testWdgIfConfig = {
    .numDevices = WDGIF_MAX_DEVICES,
    .deviceConfigs = {
        &testDeviceConfigs[0],
        &testDeviceConfigs[1],
        &testDeviceConfigs[2]
    },
    .enableVersionCheck = TRUE,
    .enableMulticoreSupport = FALSE
};

/******************************************************************************
 * Test Statistics
 ******************************************************************************/
static uint32 testsPassed = 0U;
static uint32 testsFailed = 0U;

/******************************************************************************
 * Test Macros
 ******************************************************************************/
#define TEST_ASSERT(condition, msg) \
    do { \
        if (condition) { \
            printf("  [PASS] %s\n", msg); \
            testsPassed++; \
        } else { \
            printf("  [FAIL] %s (line %d)\n", msg, __LINE__); \
            testsFailed++; \
        } \
    } while (0)

#define RUN_TEST(testFunc) \
    do { \
        printf("\nRunning: %s\n", #testFunc); \
        testFunc(); \
    } while (0)

/******************************************************************************
 * Test Functions
 ******************************************************************************/

/**
 * @brief Test initialization and deinitialization
 */
static void test_init_deinit(void)
{
    Std_ReturnType result;

    /* Test NULL pointer check */
    result = WdgIf_Init(NULL_PTR);
    TEST_ASSERT(result == E_NOT_OK, "Init with NULL should fail");

    /* Test successful initialization */
    result = WdgIf_Init(&testWdgIfConfig);
    TEST_ASSERT(result == E_OK, "Init should succeed");

    /* Test double initialization */
    result = WdgIf_Init(&testWdgIfConfig);
    TEST_ASSERT(result == E_NOT_OK, "Double init should fail");

    /* Test deinitialization */
    result = WdgIf_DeInit();
    TEST_ASSERT(result == E_OK, "DeInit should succeed");

    /* Test double deinitialization */
    result = WdgIf_DeInit();
    TEST_ASSERT(result == E_NOT_OK, "DeInit without init should fail");
}

/**
 * @brief Test version information
 */
static void test_version_info(void)
{
    Std_VersionInfoType versionInfo;

    /* Get version info */
    WdgIf_GetVersionInfo(&versionInfo);

    TEST_ASSERT(versionInfo.vendorID == 0x01U, "Vendor ID should be 0x01");
    TEST_ASSERT(versionInfo.moduleID == 0x43U, "Module ID should be 0x43");
    TEST_ASSERT(versionInfo.sw_major_version == 1U, "Major version should be 1");
}

/**
 * @brief Test mode setting
 */
static void test_set_mode(void)
{
    Std_ReturnType result;

    /* Initialize first */
    result = WdgIf_Init(&testWdgIfConfig);
    TEST_ASSERT(result == E_OK, "Init should succeed");

    /* Test valid mode changes */
    result = WdgIf_SetMode(WDGIF_INTERNAL_WDG, WDGIF_SLOW_MODE);
    TEST_ASSERT(result == E_OK, "Set slow mode should succeed");
    TEST_ASSERT(WdgIf_GetCurrentMode(WDGIF_INTERNAL_WDG) == WDGIF_SLOW_MODE,
                "Mode should be slow");

    result = WdgIf_SetMode(WDGIF_INTERNAL_WDG, WDGIF_FAST_MODE);
    TEST_ASSERT(result == E_OK, "Set fast mode should succeed");
    TEST_ASSERT(WdgIf_GetCurrentMode(WDGIF_INTERNAL_WDG) == WDGIF_FAST_MODE,
                "Mode should be fast");

    /* Test invalid device */
    result = WdgIf_SetMode(WDGIF_MAX_DEVICES, WDGIF_SLOW_MODE);
    TEST_ASSERT(result == E_NOT_OK, "Set mode for invalid device should fail");

    /* Test invalid mode */
    result = WdgIf_SetMode(WDGIF_INTERNAL_WDG, 0xFFU);
    TEST_ASSERT(result == E_NOT_OK, "Set invalid mode should fail");

    /* Test OFF mode */
    result = WdgIf_SetMode(WDGIF_INTERNAL_WDG, WDGIF_OFF_MODE);
    TEST_ASSERT(result == E_OK, "Set off mode should succeed");

    WdgIf_DeInit();
}

/**
 * @brief Test watchdog triggering
 */
static void test_trigger(void)
{
    Std_ReturnType result;

    /* Initialize */
    result = WdgIf_Init(&testWdgIfConfig);
    TEST_ASSERT(result == E_OK, "Init should succeed");

    /* Set to slow mode */
    result = WdgIf_SetMode(WDGIF_INTERNAL_WDG, WDGIF_SLOW_MODE);
    TEST_ASSERT(result == E_OK, "Set mode should succeed");

    /* Test trigger */
    result = WdgIf_SetTriggerCondition(WDGIF_INTERNAL_WDG, 0U);
    TEST_ASSERT(result == E_OK, "Trigger should succeed");

    /* Test trigger with specific timeout */
    result = WdgIf_SetTriggerCondition(WDGIF_INTERNAL_WDG, 200U);
    TEST_ASSERT(result == E_OK, "Trigger with timeout should succeed");

    /* Test trigger for disabled watchdog */
    result = WdgIf_SetMode(WDGIF_INTERNAL_WDG, WDGIF_OFF_MODE);
    TEST_ASSERT(result == E_OK, "Set off mode should succeed");

    result = WdgIf_SetTriggerCondition(WDGIF_INTERNAL_WDG, 0U);
    TEST_ASSERT(result == E_OK, "Trigger for disabled WDG should return OK");

    WdgIf_DeInit();
}

/**
 * @brief Test error handling
 */
static void test_error_handling(void)
{
    /* Initialize */
    WdgIf_Init(&testWdgIfConfig);
    WdgIf_SetMode(WDGIF_INTERNAL_WDG, WDGIF_SLOW_MODE);

    /* Clear any existing errors */
    WdgIf_ClearErrorCount(WDGIF_INTERNAL_WDG);
    TEST_ASSERT(WdgIf_GetErrorCount(WDGIF_INTERNAL_WDG) == 0U,
                "Error count should be 0");

    /* Report some errors */
    WdgIf_ReportError(WDGIF_INTERNAL_WDG, WDGIF_ERROR_TRIGGER_FAIL);
    WdgIf_ReportError(WDGIF_INTERNAL_WDG, WDGIF_ERROR_TRIGGER_FAIL);
    TEST_ASSERT(WdgIf_GetErrorCount(WDGIF_INTERNAL_WDG) == 2U,
                "Error count should be 2");

    /* Check safety threshold (default is 3) */
    TEST_ASSERT(!WdgIf_IsSafetyThresholdExceeded(WDGIF_INTERNAL_WDG),
                "Safety threshold should not be exceeded");

    /* One more error to reach threshold */
    WdgIf_ReportError(WDGIF_INTERNAL_WDG, WDGIF_ERROR_TRIGGER_FAIL);
    TEST_ASSERT(WdgIf_IsSafetyThresholdExceeded(WDGIF_INTERNAL_WDG),
                "Safety threshold should be exceeded");

    /* Clear errors */
    WdgIf_ClearErrorCount(WDGIF_INTERNAL_WDG);
    TEST_ASSERT(!WdgIf_IsSafetyThresholdExceeded(WDGIF_INTERNAL_WDG),
                "Safety threshold should not be exceeded after clear");

    WdgIf_DeInit();
}

/**
 * @brief Test emulator functionality
 */
static void test_emulator(void)
{
    Std_ReturnType result;
    const WdgIf_EmulatorStateType *state;

    /* Initialize */
    WdgIf_Init(&testWdgIfConfig);
    WdgIf_SetMode(WDGIF_INTERNAL_WDG, WDGIF_SLOW_MODE);

    /* Test emulator state */
    state = WdgIf_Emulator_GetState(WDGIF_INTERNAL_WDG);
    TEST_ASSERT(state != NULL_PTR, "Emulator state should be accessible");

    /* Test trigger */
    result = WdgIf_SetTriggerCondition(WDGIF_INTERNAL_WDG, 0U);
    TEST_ASSERT(result == E_OK, "Trigger should succeed");

    state = WdgIf_Emulator_GetState(WDGIF_INTERNAL_WDG);
    TEST_ASSERT(state->triggerCount > 0U, "Trigger count should increment");

    /* Test time advancement and timeout */
    WdgIf_Emulator_ResetState(WDGIF_INTERNAL_WDG);
    WdgIf_SetMode(WDGIF_INTERNAL_WDG, WDGIF_SLOW_MODE);  /* Re-enable watchdog */
    WdgIf_Emulator_AdvanceTime(WDGIF_INTERNAL_WDG, 1000U);  /* 1 second - past 500ms timeout */

    result = WdgIf_SetTriggerCondition(WDGIF_INTERNAL_WDG, 0U);
    TEST_ASSERT(result == E_NOT_OK, "Trigger after timeout should fail");

    WdgIf_DeInit();
}

/**
 * @brief Test multi-instance support
 */
static void test_multi_instance(void)
{
    Std_ReturnType result;

    /* Initialize with all 3 devices */
    result = WdgIf_Init(&testWdgIfConfig);
    TEST_ASSERT(result == E_OK, "Init with multiple devices should succeed");

    /* Configure all devices to different modes */
    result = WdgIf_SetMode(WDGIF_INTERNAL_WDG, WDGIF_SLOW_MODE);
    TEST_ASSERT(result == E_OK, "Set internal WDG mode should succeed");

    result = WdgIf_SetMode(WDGIF_EXTERNAL_WDG1, WDGIF_FAST_MODE);
    TEST_ASSERT(result == E_OK, "Set external WDG1 mode should succeed");

    result = WdgIf_SetMode(WDGIF_EXTERNAL_WDG2, WDGIF_OFF_MODE);
    TEST_ASSERT(result == E_OK, "Set external WDG2 mode should succeed");

    /* Verify modes */
    TEST_ASSERT(WdgIf_GetCurrentMode(WDGIF_INTERNAL_WDG) == WDGIF_SLOW_MODE,
                "Internal WDG should be in slow mode");
    TEST_ASSERT(WdgIf_GetCurrentMode(WDGIF_EXTERNAL_WDG1) == WDGIF_FAST_MODE,
                "External WDG1 should be in fast mode");
    TEST_ASSERT(WdgIf_GetCurrentMode(WDGIF_EXTERNAL_WDG2) == WDGIF_OFF_MODE,
                "External WDG2 should be in off mode");

    /* Trigger all devices */
    result = WdgIf_SetTriggerCondition(WDGIF_INTERNAL_WDG, 0U);
    TEST_ASSERT(result == E_OK, "Trigger internal WDG should succeed");

    result = WdgIf_SetTriggerCondition(WDGIF_EXTERNAL_WDG1, 0U);
    TEST_ASSERT(result == E_OK, "Trigger external WDG1 should succeed");

    result = WdgIf_SetTriggerCondition(WDGIF_EXTERNAL_WDG2, 0U);
    TEST_ASSERT(result == E_OK, "Trigger disabled WDG2 should return OK");

    WdgIf_DeInit();
}

/**
 * @brief Test status functions
 */
static void test_status_functions(void)
{
    /* Initialize */
    WdgIf_Init(&testWdgIfConfig);
    WdgIf_SetMode(WDGIF_INTERNAL_WDG, WDGIF_SLOW_MODE);

    /* Test initialized check */
    TEST_ASSERT(WdgIf_IsInitialized(WDGIF_INTERNAL_WDG) == TRUE,
                "Internal WDG should be initialized");

    /* Test driver status */
    TEST_ASSERT(WdgIf_GetDriverStatus(WDGIF_INTERNAL_WDG) == WDGIF_STATUS_IDLE,
                "Driver status should be IDLE");

    /* Test invalid device */
    TEST_ASSERT(WdgIf_IsInitialized(WDGIF_MAX_DEVICES) == FALSE,
                "Invalid device should not be initialized");
    TEST_ASSERT(WdgIf_GetDriverStatus(WDGIF_MAX_DEVICES) == WDGIF_STATUS_ERROR,
                "Invalid device should return ERROR status");

    WdgIf_DeInit();
}

/******************************************************************************
 * Main Test Entry
 ******************************************************************************/
int main(void)
{
    printf("========================================\n");
    printf("Watchdog Interface Module Tests\n");
    printf("========================================\n");

    RUN_TEST(test_init_deinit);
    RUN_TEST(test_version_info);
    RUN_TEST(test_set_mode);
    RUN_TEST(test_trigger);
    RUN_TEST(test_error_handling);
    RUN_TEST(test_emulator);
    RUN_TEST(test_multi_instance);
    RUN_TEST(test_status_functions);

    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Passed: %u\n", (unsigned int)testsPassed);
    printf("  Failed: %u\n", (unsigned int)testsFailed);
    printf("========================================\n");

    return (testsFailed == 0U) ? 0 : 1;
}
