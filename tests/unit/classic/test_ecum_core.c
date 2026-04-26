/******************************************************************************
 * @file    test_ecum_core.c
 * @brief   EcuM Core Unit Tests
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "autosar/classic/ecum/ecum.h"

/******************************************************************************
 * Test Configuration
 ******************************************************************************/
static EcuM_WakeupSourceConfigType testWakeupSources[] = {
    {
        .source = ECUM_WKSOURCE_POWER,
        .validationTimeout = 100U,
        .validationCounter = 5U,
        .checkWakeupSupported = TRUE,
        .comMChannelSupported = FALSE,
        .comMChannel = 0U
    },
    {
        .source = ECUM_WKSOURCE_CAN,
        .validationTimeout = 200U,
        .validationCounter = 10U,
        .checkWakeupSupported = TRUE,
        .comMChannelSupported = TRUE,
        .comMChannel = 0U
    }
};

static EcuM_SleepModeConfigType testSleepModes[] = {
    {
        .sleepMode = ECUM_SLEEP_MODE_POLLING,
        .wakeupSourceMask = ECUM_WKSOURCE_CAN | ECUM_WKSOURCE_POWER,
        .mcuModeSupported = TRUE,
        .mcuMode = 0U,
        .pollWakeupSupported = TRUE,
        .sleepCallbackSupported = TRUE
    },
    {
        .sleepMode = ECUM_SLEEP_MODE_HALT,
        .wakeupSourceMask = ECUM_WKSOURCE_POWER,
        .mcuModeSupported = TRUE,
        .mcuMode = 1U,
        .pollWakeupSupported = FALSE,
        .sleepCallbackSupported = TRUE
    }
};

static EcuM_ConfigType testConfig = {
    .normalMcuWakeupTime = 50U,
    .minShutdownTime = 100U,
    .wakeupSources = testWakeupSources,
    .numWakeupSources = 2U,
    .sleepModes = testSleepModes,
    .numSleepModes = 2U,
    .defaultTarget = ECUM_TARGET_OFF,
    .defaultSleepMode = ECUM_SLEEP_MODE_POLLING,
    .defaultResetMode = ECUM_WKSOURCE_INTERNAL_RESET,
    .numModeRequestPorts = 4U
};

/******************************************************************************
 * Test Counters
 ******************************************************************************/
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/******************************************************************************
 * Test Macros
 ******************************************************************************/
#define TEST_ASSERT(condition, msg) do { \
    tests_run++; \
    if (!(condition)) { \
        printf("  [FAIL] %s\n", msg); \
        tests_failed++; \
    } else { \
        printf("  [PASS] %s\n", msg); \
        tests_passed++; \
    } \
} while(0)

#define TEST_START(name) printf("\nTest: %s\n", name)
#define TEST_END() printf("  Total: %d run, %d passed, %d failed\n", tests_run, tests_passed, tests_failed)

/******************************************************************************
 * Test Functions
 ******************************************************************************/

void test_ecum_init(void)
{
    TEST_START("EcuM_Init");
    Std_ReturnType result;
    
    /* Test NULL config */
    result = EcuM_Init(NULL);
    TEST_ASSERT(result == E_NOT_OK, "Init with NULL config should fail");
    
    /* Test valid init */
    result = EcuM_Init(&testConfig);
    TEST_ASSERT(result == E_OK, "Init with valid config should succeed");
    TEST_ASSERT(EcuM_IsInitialized() == TRUE, "Should be initialized");
    TEST_ASSERT(EcuM_GetState() == ECUM_STATE_STARTUP, "Should be in STARTUP state");
    
    /* Test double init */
    result = EcuM_Init(&testConfig);
    TEST_ASSERT(result == E_NOT_OK, "Double init should fail");
    
    /* Cleanup */
    (void)EcuM_DeInit();
}

void test_ecum_deinit(void)
{
    TEST_START("EcuM_DeInit");
    Std_ReturnType result;
    
    /* Init first */
    (void)EcuM_Init(&testConfig);
    
    /* Test deinit */
    result = EcuM_DeInit();
    TEST_ASSERT(result == E_OK, "DeInit should succeed");
    TEST_ASSERT(EcuM_IsInitialized() == FALSE, "Should not be initialized");
    
    /* Test deinit without init */
    result = EcuM_DeInit();
    TEST_ASSERT(result == E_NOT_OK, "DeInit without init should fail");
}

void test_ecum_state_management(void)
{
    TEST_START("EcuM_StateManagement");
    
    /* Init */
    (void)EcuM_Init(&testConfig);
    
    /* Test initial state */
    TEST_ASSERT(EcuM_GetState() == ECUM_STATE_STARTUP, "Initial state should be STARTUP");
    
    /* Test state transition to RUN */
    EcuM_StartupTwo();
    TEST_ASSERT(EcuM_GetState() == ECUM_STATE_RUN, "Should transition to RUN");
    
    /* Test state request */
    Std_ReturnType result = EcuM_RequestRUN(0U);
    TEST_ASSERT(result == E_OK, "RUN request should succeed");
    
    result = EcuM_ReleaseRUN(0U);
    TEST_ASSERT(result == E_OK, "RUN release should succeed");
    
    /* Test invalid user */
    result = EcuM_RequestRUN(ECUM_MAX_USERS);
    TEST_ASSERT(result == E_NOT_OK, "Request with invalid user should fail");
    
    /* Cleanup */
    (void)EcuM_DeInit();
}

void test_ecum_run_postrun_requests(void)
{
    TEST_START("EcuM_RUN_and_POSTRUN_Requests");
    Std_ReturnType result;
    
    /* Init and enter RUN */
    (void)EcuM_Init(&testConfig);
    EcuM_StartupTwo();
    
    /* Test RUN request */
    result = EcuM_RequestRUN(1U);
    TEST_ASSERT(result == E_OK, "RUN request should succeed");
    
    /* Test POST RUN request */
    result = EcuM_RequestPOST_RUN(2U);
    TEST_ASSERT(result == E_OK, "POST RUN request should succeed");
    
    /* Test release POST RUN */
    result = EcuM_ReleasePOST_RUN(2U);
    TEST_ASSERT(result == E_OK, "POST RUN release should succeed");
    
    /* Test KillAllRUNRequests */
    result = EcuM_KillAllRUNRequests();
    TEST_ASSERT(result == E_OK, "KillAllRUNRequests should succeed");
    
    /* Cleanup */
    (void)EcuM_DeInit();
}

void test_ecum_shutdown_target(void)
{
    TEST_START("EcuM_ShutdownTarget");
    Std_ReturnType result;
    EcuM_ShutdownTargetType target;
    uint8 mode;
    
    /* Init */
    (void)EcuM_Init(&testConfig);
    
    /* Test select shutdown target */
    result = EcuM_SelectShutdownTarget(ECUM_TARGET_SLEEP, ECUM_SLEEP_MODE_POLLING);
    TEST_ASSERT(result == E_OK, "Select SLEEP target should succeed");
    
    /* Test get shutdown target */
    result = EcuM_GetShutdownTarget(&target, &mode);
    TEST_ASSERT(result == E_OK, "Get shutdown target should succeed");
    TEST_ASSERT(target == ECUM_TARGET_SLEEP, "Target should be SLEEP");
    TEST_ASSERT(mode == ECUM_SLEEP_MODE_POLLING, "Mode should be POLLING");
    
    /* Test get last shutdown target */
    result = EcuM_SelectShutdownTarget(ECUM_TARGET_OFF, 0U);
    TEST_ASSERT(result == E_OK, "Select OFF target should succeed");
    
    result = EcuM_GetLastShutdownTarget(&target, &mode);
    TEST_ASSERT(result == E_OK, "Get last shutdown target should succeed");
    TEST_ASSERT(target == ECUM_TARGET_SLEEP, "Last target should be SLEEP");
    
    /* Test NULL pointer */
    result = EcuM_GetShutdownTarget(NULL, &mode);
    TEST_ASSERT(result == E_NOT_OK, "Get target with NULL should fail");
    
    /* Cleanup */
    (void)EcuM_DeInit();
}

void test_ecum_sleep_mode(void)
{
    TEST_START("EcuM_SleepMode");
    Std_ReturnType result;
    EcuM_SleepModeType sleepMode;
    
    /* Init */
    (void)EcuM_Init(&testConfig);
    
    /* Test select sleep mode */
    result = EcuM_SelectSleepMode(ECUM_SLEEP_MODE_HALT);
    TEST_ASSERT(result == E_OK, "Select HALT mode should succeed");
    
    /* Test get sleep mode */
    sleepMode = EcuM_GetSleepMode();
    TEST_ASSERT(sleepMode == ECUM_SLEEP_MODE_HALT, "Should get HALT mode");
    
    /* Test invalid sleep mode */
    result = EcuM_SelectSleepMode(ECUM_MAX_SLEEP_MODES);
    TEST_ASSERT(result == E_NOT_OK, "Select invalid mode should fail");
    
    /* Cleanup */
    (void)EcuM_DeInit();
}

void test_ecum_callbacks(void)
{
    TEST_START("EcuM_Callbacks");
    
    static int stateChangeCount = 0;
    static int wakeupCount = 0;
    
    void testStateCallback(EcuM_StateType oldState, EcuM_StateType newState)
    {
        (void)oldState;
        (void)newState;
        stateChangeCount++;
    }
    
    void testWakeupCallback(EcuM_WakeupSourceType source, EcuM_WakeupStatusType status)
    {
        (void)source;
        (void)status;
        wakeupCount++;
    }
    
    /* Init */
    (void)EcuM_Init(&testConfig);
    
    /* Register callbacks */
    Std_ReturnType result = EcuM_RegisterStateChangeCallback(testStateCallback);
    TEST_ASSERT(result == E_OK, "Register state callback should succeed");
    
    result = EcuM_RegisterWakeupCallback(testWakeupCallback);
    TEST_ASSERT(result == E_OK, "Register wakeup callback should succeed");
    
    /* Trigger state change */
    stateChangeCount = 0;
    EcuM_StartupTwo();
    TEST_ASSERT(stateChangeCount == 1, "State callback should be called");
    
    /* Unregister callbacks */
    result = EcuM_UnregisterStateChangeCallback(testStateCallback);
    TEST_ASSERT(result == E_OK, "Unregister state callback should succeed");
    
    result = EcuM_UnregisterWakeupCallback(testWakeupCallback);
    TEST_ASSERT(result == E_OK, "Unregister wakeup callback should succeed");
    
    /* Test unregister non-registered callback */
    result = EcuM_UnregisterStateChangeCallback(testStateCallback);
    TEST_ASSERT(result == E_NOT_OK, "Unregister non-registered should fail");
    
    /* Cleanup */
    (void)EcuM_DeInit();
}

void test_ecum_version(void)
{
    TEST_START("EcuM_Version");
    Std_VersionInfoType version;
    
    EcuM_GetVersionInfo(&version);
    
    TEST_ASSERT(version.vendorID == ECUM_VENDOR_ID, "Vendor ID should match");
    TEST_ASSERT(version.moduleID == ECUM_MODULE_ID, "Module ID should match");
    TEST_ASSERT(version.sw_major_version == ECUM_SW_MAJOR_VERSION, "Major version should match");
    TEST_ASSERT(version.sw_minor_version == ECUM_SW_MINOR_VERSION, "Minor version should match");
    
    /* Test NULL pointer */
    EcuM_GetVersionInfo(NULL);
    /* Should not crash */
    TEST_ASSERT(1, "Version info with NULL should not crash");
}

/******************************************************************************
 * Main Test Runner
 ******************************************************************************/
int main(void)
{
    printf("============================================\n");
    printf("EcuM Core Unit Tests\n");
    printf("============================================\n");
    
    test_ecum_init();
    test_ecum_deinit();
    test_ecum_state_management();
    test_ecum_run_postrun_requests();
    test_ecum_shutdown_target();
    test_ecum_sleep_mode();
    test_ecum_callbacks();
    test_ecum_version();
    
    printf("\n============================================\n");
    printf("Summary: %d tests run, %d passed, %d failed\n", 
           tests_run, tests_passed, tests_failed);
    printf("============================================\n");
    
    return (tests_failed > 0) ? 1 : 0;
}
