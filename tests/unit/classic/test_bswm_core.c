/******************************************************************************
 * @file    test_bswm_core.c
 * @brief   BswM Core Unit Tests
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "autosar/classic/bswm/bswm.h"

/******************************************************************************
 * Test Configuration
 ******************************************************************************/
static BswM_ModeRequestPortType testModeRequestPorts[] = {
    {
        .type = BSWM_MRP_GENERIC_REQUEST,
        .id = 0U,
        .currentMode = 0U,
        .requestedMode = 0U,
        .requestPending = FALSE,
        .isInitialized = TRUE
    },
    {
        .type = BSWM_MRP_ECUM_INDICATION,
        .id = 1U,
        .currentMode = 0U,
        .requestedMode = 0U,
        .requestPending = FALSE,
        .isInitialized = TRUE
    },
    {
        .type = BSWM_MRP_COMM_INDICATION,
        .id = 2U,
        .currentMode = 0U,
        .requestedMode = 0U,
        .requestPending = FALSE,
        .isInitialized = TRUE
    }
};

static BswM_ConfigType testConfig = {
    .rules = NULL,
    .numRules = 0U,
    .actionLists = NULL,
    .numActionLists = 0U,
    .modeRequestPorts = testModeRequestPorts,
    .numModeRequestPorts = 3U,
    .modeConditions = NULL,
    .numModeConditions = 0U,
    .expressions = NULL,
    .numExpressions = 0U,
    .modeRequestProcessingEnabled = TRUE,
    .rulesProcessingEnabled = TRUE
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

/******************************************************************************
 * Test Functions
 ******************************************************************************/

void test_bswm_init(void)
{
    TEST_START("BswM_Init");
    
    /* Test init with valid config */
    BswM_Init(&testConfig);
    TEST_ASSERT(BswM_IsInitialized() == TRUE, "Should be initialized");
    
    /* Test deinit */
    BswM_Deinit();
    TEST_ASSERT(BswM_IsInitialized() == FALSE, "Should not be initialized after DeInit");
    
    /* Test init with NULL config */
    BswM_Init(NULL);
    TEST_ASSERT(BswM_IsInitialized() == FALSE, "Init with NULL should not initialize");
}

void test_bswm_mode_request(void)
{
    TEST_START("BswM_ModeRequest");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Test mode request */
    BswM_RequestMode(BSWM_MRP_GENERIC_REQUEST, 1U);
    TEST_ASSERT(1, "Mode request executed");
    
    /* Test generic mode request */
    BswM_ModeRequest(0U, 2U);
    TEST_ASSERT(1, "Generic mode request executed");
    
    /* Cleanup */
    BswM_Deinit();
}

void test_bswm_mode_switch(void)
{
    TEST_START("BswM_ModeSwitch");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Test mode switch */
    BswM_SwitchMode(0U, 5U);
    
    BswM_ModeType mode = BswM_GetMode(0U);
    TEST_ASSERT(mode == 5U, "Mode should be switched to 5");
    
    /* Test get mode for unassigned mode ID */
    mode = BswM_GetMode(100U);
    TEST_ASSERT(mode == 0U, "Unassigned mode should return 0");
    
    /* Cleanup */
    BswM_Deinit();
}

void test_bswm_mode_request_port(void)
{
    TEST_START("BswM_ModeRequestPort");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Test update mode request port */
    BswM_UpdateModeRequestPort(0U, 10U);
    
    BswM_ModeType value = BswM_GetModeRequestPortValue(0U);
    TEST_ASSERT(value == 10U, "Mode request port value should be 10");
    
    /* Test invalid port ID */
    value = BswM_GetModeRequestPortValue(100U);
    TEST_ASSERT(value == 0U, "Invalid port should return 0");
    
    /* Cleanup */
    BswM_Deinit();
}

void test_bswm_mainfunction(void)
{
    TEST_START("BswM_MainFunction");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Test main function */
    BswM_MainFunction();
    TEST_ASSERT(1, "Main function executed");
    
    /* Cleanup */
    BswM_Deinit();
}

void test_bswm_version(void)
{
    TEST_START("BswM_Version");
    Std_VersionInfoType version;
    
    BswM_GetVersionInfo(&version);
    
    TEST_ASSERT(version.vendorID == BSWM_VENDOR_ID, "Vendor ID should match");
    TEST_ASSERT(version.moduleID == BSWM_MODULE_ID, "Module ID should match");
    TEST_ASSERT(version.sw_major_version == BSWM_SW_MAJOR_VERSION, "Major version should match");
    
    /* Test NULL pointer */
    BswM_GetVersionInfo(NULL);
    TEST_ASSERT(1, "Version info with NULL should not crash");
}

void test_bswm_dcm_integration(void)
{
    TEST_START("BswM_DCM_Integration");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Test DCM session mode request */
    BswM_Dcm_RequestSessionMode(0x03U);
    TEST_ASSERT(1, "DCM session mode request executed");
    
    /* Test DCM reset mode request */
    BswM_Dcm_RequestResetMode(0x01U);
    TEST_ASSERT(1, "DCM reset mode request executed");
    
    /* Test DCM communication mode current state */
    BswM_Dcm_CommunicationMode_CurrentState(0U, 1U);
    TEST_ASSERT(1, "DCM communication mode current state executed");
    
    /* Test DCM application updated */
    BswM_Dcm_ApplicationUpdated(TRUE);
    TEST_ASSERT(1, "DCM application updated executed");
    
    /* Cleanup */
    BswM_Deinit();
}

void test_bswm_comm_integration(void)
{
    TEST_START("BswM_ComM_Integration");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Test ComM current mode */
    BswM_ComM_CurrentMode(0U, COMM_FULL_COMMUNICATION);
    TEST_ASSERT(1, "ComM current mode executed");
    
    /* Test ComM request mode */
    BswM_ComM_RequestMode(0U, COMM_SILENT_COMMUNICATION);
    TEST_ASSERT(1, "ComM request mode executed");
    
    /* Cleanup */
    BswM_Deinit();
}

void test_bswm_ecum_integration(void)
{
    TEST_START("BswM_EcuM_Integration");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Test EcuM current state */
    BswM_EcuM_CurrentState(ECUM_STATE_RUN);
    TEST_ASSERT(1, "EcuM current state executed");
    
    /* Test EcuM wakeup status */
    BswM_EcuM_WakeupStatus(ECUM_WKSOURCE_POWER, ECUM_WKSTATUS_VALIDATED);
    TEST_ASSERT(1, "EcuM wakeup status executed");
    
    /* Cleanup */
    BswM_Deinit();
}

/******************************************************************************
 * Main Test Runner
 ******************************************************************************/
int main(void)
{
    printf("============================================\n");
    printf("BswM Core Unit Tests\n");
    printf("============================================\n");
    
    test_bswm_init();
    test_bswm_mode_request();
    test_bswm_mode_switch();
    test_bswm_mode_request_port();
    test_bswm_mainfunction();
    test_bswm_version();
    test_bswm_dcm_integration();
    test_bswm_comm_integration();
    test_bswm_ecum_integration();
    
    printf("\n============================================\n");
    printf("Summary: %d tests run, %d passed, %d failed\n", 
           tests_run, tests_passed, tests_failed);
    printf("============================================\n");
    
    return (tests_failed > 0) ? 1 : 0;
}
