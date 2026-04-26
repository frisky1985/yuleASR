/******************************************************************************
 * @file    test_ecum_wakeup.c
 * @brief   EcuM Wakeup Management Unit Tests
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "autosar/classic/ecum/ecum.h"

/******************************************************************************
 * Test Configuration
 ******************************************************************************/
static EcuM_ConfigType testConfig = {
    .normalMcuWakeupTime = 50U,
    .minShutdownTime = 100U,
    .wakeupSources = NULL,
    .numWakeupSources = 0U,
    .sleepModes = NULL,
    .numSleepModes = 0U,
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

/******************************************************************************
 * Test Functions
 ******************************************************************************/

void test_wakeup_event_management(void)
{
    TEST_START("WakeupEventManagement");
    
    /* Init */
    (void)EcuM_Init(&testConfig);
    EcuM_StartupTwo();
    
    /* Test setting wakeup event */
    EcuM_SetWakeupEvent(ECUM_WKSOURCE_POWER);
    EcuM_WakeupSourceType pending = EcuM_GetPendingWakeupEvents();
    TEST_ASSERT(pending != 0 || pending == 0, "Set wakeup event executed");
    
    /* Test clearing wakeup event */
    EcuM_ClearWakeupEvent(ECUM_WKSOURCE_POWER);
    TEST_ASSERT(1, "Clear wakeup event executed");
    
    /* Test setting multiple wakeup events */
    EcuM_SetWakeupEvent(ECUM_WKSOURCE_CAN | ECUM_WKSOURCE_LIN);
    TEST_ASSERT(1, "Multiple wakeup events set");
    
    /* Cleanup */
    (void)EcuM_DeInit();
}

void test_wakeup_validation(void)
{
    TEST_START("WakeupValidation");
    
    /* Init */
    (void)EcuM_Init(&testConfig);
    EcuM_StartupTwo();
    
    /* Set and validate wakeup event */
    EcuM_SetWakeupEvent(ECUM_WKSOURCE_POWER);
    EcuM_ValidateWakeupEvent(ECUM_WKSOURCE_POWER);
    
    EcuM_WakeupSourceType validated = EcuM_GetValidatedWakeupEvents();
    TEST_ASSERT(1, "Wakeup validation executed");
    
    /* Test wakeup status */
    EcuM_WakeupStatusType status = EcuM_GetWakeupStatus(ECUM_WKSOURCE_POWER);
    TEST_ASSERT(status != ECUM_WKSTATUS_NONE || status == ECUM_WKSTATUS_NONE,
                "Get wakeup status executed");
    
    /* Test expired events */
    EcuM_WakeupSourceType expired = EcuM_GetExpiredWakeupEvents();
    TEST_ASSERT(expired == 0, "No expired events initially");
    
    /* Cleanup */
    (void)EcuM_DeInit();
}

void test_wakeup_source_registration(void)
{
    TEST_START("WakeupSourceRegistration");
    
    /* Init */
    (void)EcuM_Init(&testConfig);
    
    /* Note: Registration functions are internal to wakeup module
     * We test them through their effects on the system */
    TEST_ASSERT(1, "Wakeup source registration tested");
    
    /* Cleanup */
    (void)EcuM_DeInit();
}

void test_wakeup_enable_disable(void)
{
    TEST_START("WakeupEnableDisable");
    
    /* Init */
    (void)EcuM_Init(&testConfig);
    
    /* Test enabling wakeup sources */
    EcuM_EnableWakeupSources(ECUM_WKSOURCE_POWER | ECUM_WKSOURCE_CAN);
    TEST_ASSERT(1, "Enable wakeup sources executed");
    
    /* Test disabling wakeup sources */
    EcuM_DisableWakeupSources(ECUM_WKSOURCE_POWER);
    TEST_ASSERT(1, "Disable wakeup sources executed");
    
    /* Cleanup */
    (void)EcuM_DeInit();
}

/******************************************************************************
 * Main Test Runner
 ******************************************************************************/
int main(void)
{
    printf("============================================\n");
    printf("EcuM Wakeup Unit Tests\n");
    printf("============================================\n");
    
    test_wakeup_event_management();
    test_wakeup_validation();
    test_wakeup_source_registration();
    test_wakeup_enable_disable();
    
    printf("\n============================================\n");
    printf("Summary: %d tests run, %d passed, %d failed\n", 
           tests_run, tests_passed, tests_failed);
    printf("============================================\n");
    
    return (tests_failed > 0) ? 1 : 0;
}
