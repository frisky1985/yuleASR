/**
 * @file test_watchdog_manager.c
 * @brief Swc_WatchdogManager host unit tests (main + assert style)
 *
 * Covers init/deinit, entity registration/limits, checkpoint handling,
 * activation, alive supervision (correct/incorrect/expired), timeout based
 * hardware triggering, RTE alive indications and expiration handling.
 * RTE port APIs and Det are mocked in this file.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Swc_WatchdogManager.h"

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))

static int tests_run = 0;
static int tests_passed = 0;

/* ============================================================================
 * RTE / DET mocks
 * ============================================================================ */
static uint8 mockAliveEntityId;
static Std_ReturnType mockReadResult;
static Std_ReturnType mockWriteResult;
static uint32 mockTime;

static Swc_WatchdogStatusType lastWrittenStatus;
static int statusWrites;
static int triggerWrites;
static int entityStatusWrites;
static int detCalls;
static uint8 lastDetApi;
static uint8 lastDetError;

Std_ReturnType Rte_Read_SWC_WATCHDOGMANAGER_PORT_ALIVE_INDICATION_R(void* data)
{
    if (mockReadResult != E_OK) { return mockReadResult; }
    *(uint8*)data = mockAliveEntityId;
    return E_OK;
}

Std_ReturnType Rte_Write_SWC_WATCHDOGMANAGER_PORT_WDG_STATUS_P(const void* data)
{
    lastWrittenStatus = *(const Swc_WatchdogStatusType*)data;
    statusWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_WATCHDOGMANAGER_PORT_ENTITY_STATUS_P(const void* data)
{
    (void)data;
    entityStatusWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_WATCHDOGMANAGER_PORT_WDG_TRIGGER_P(const void* data)
{
    (void)data;
    triggerWrites++;
    return mockWriteResult;
}

uint32 Rte_GetTime(void)
{
    return mockTime;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId,
                               uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    detCalls++;
    lastDetApi = ApiId;
    lastDetError = ErrorId;
    return E_OK;
}

/* ============================================================================
 * Helpers
 * ============================================================================ */
static void fresh_init(void)
{
    Swc_WatchdogManager_Deinit();
    Swc_WatchdogManager_Init();
}

static void mock_reset(void)
{
    mockAliveEntityId = 0U;
    mockReadResult = E_NOT_OK;
    mockWriteResult = E_OK;
    mockTime = 50U;
    lastWrittenStatus = WDG_STATUS_STOPPED;
    statusWrites = 0;
    triggerWrites = 0;
    entityStatusWrites = 0;
    detCalls = 0;
    lastDetApi = 0U;
    lastDetError = 0U;
}

static void make_config(Swc_SupervisedEntityConfigType* config, uint8 id,
                        uint16 aliveTimeout, uint16 minInd, uint16 maxInd,
                        boolean active)
{
    memset(config, 0, sizeof(*config));
    config->entityId = id;
    config->aliveTimeout = aliveTimeout;
    config->expectedAliveIndications = maxInd;
    config->minCorrectIndications = minInd;
    config->maxCorrectIndications = maxInd;
    config->isActive = active;
}

static int register_active_entity(uint8 id, uint16 aliveTimeout,
                                  uint16 minInd, uint16 maxInd)
{
    Swc_SupervisedEntityConfigType config;
    make_config(&config, id, aliveTimeout, minInd, maxInd, TRUE);
    return (Swc_WatchdogManager_RegisterEntity(&config) == RTE_E_OK) ? 0 : -1;
}

/* ============================================================================
 * Tests
 * ============================================================================ */
static int test_uninitialized_paths(void)
{
    Swc_SupervisedEntityConfigType config;
    Swc_SupervisedEntityStatusType entityStatus;
    Swc_WatchdogManagerStatusType status;

    mock_reset();
    Swc_WatchdogManager_10ms();
    Swc_WatchdogManager_Trigger();
    Swc_WatchdogManager_MainFunction();
    TEST_ASSERT_EQ(statusWrites, 0);
    TEST_ASSERT_EQ(triggerWrites, 0);

    TEST_ASSERT_EQ(Swc_WatchdogManager_RegisterEntity(NULL_PTR), RTE_E_INVALID);
    make_config(&config, 1U, 100U, 0U, 10U, TRUE);
    TEST_ASSERT_EQ(Swc_WatchdogManager_RegisterEntity(&config), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_WatchdogManager_UnregisterEntity(1U), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_WatchdogManager_CheckpointReached(1U), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_WatchdogManager_SetEntityActive(1U, TRUE), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(1U, NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(1U, &entityStatus), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_UNINIT);

    Swc_WatchdogManager_Deinit(); /* no-op */
    return 0;
}

static int test_init_status_deinit(void)
{
    Swc_WatchdogManagerStatusType status;

    mock_reset();
    Swc_WatchdogManager_Init();
    TEST_ASSERT_EQ(detCalls, 1);
    TEST_ASSERT_EQ(lastDetApi, 0x01U);
    TEST_ASSERT_EQ(lastDetError, RTE_E_OK);
    Swc_WatchdogManager_Init();
    TEST_ASSERT_EQ(detCalls, 1);

    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.watchdogStatus, WDG_STATUS_STOPPED);
    TEST_ASSERT_EQ(status.numSupervisedEntities, 0U);
    TEST_ASSERT_EQ(status.globalSupervisionCycle, 0U);
    TEST_ASSERT(status.isInitialized != 0U);
    TEST_ASSERT_EQ(Swc_WatchdogManager_IsGlobalStatusCorrect(), TRUE);

    Swc_WatchdogManager_Deinit();
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_UNINIT);

    Swc_WatchdogManager_Init();
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.numSupervisedEntities, 0U);
    return 0;
}

static int test_register_unregister(void)
{
    Swc_SupervisedEntityConfigType config;
    Swc_SupervisedEntityStatusType entityStatus;
    Swc_WatchdogManagerStatusType status;

    mock_reset();
    fresh_init();

    make_config(&config, 5U, 100U, 1U, 3U, TRUE);
    TEST_ASSERT_EQ(Swc_WatchdogManager_RegisterEntity(&config), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_WatchdogManager_RegisterEntity(&config), RTE_E_INVALID); /* duplicate */

    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.numSupervisedEntities, 1U);

    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(5U, &entityStatus), RTE_E_OK);
    TEST_ASSERT_EQ(entityStatus.entityId, 5U);
    TEST_ASSERT_EQ(entityStatus.state, ALIVE_STATE_CORRECT);
    TEST_ASSERT_EQ(entityStatus.aliveIndications, 0U);
    TEST_ASSERT_EQ(entityStatus.missedIndications, 0U);

    TEST_ASSERT_EQ(Swc_WatchdogManager_UnregisterEntity(9U), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_WatchdogManager_UnregisterEntity(5U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.numSupervisedEntities, 0U);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(5U, &entityStatus), RTE_E_INVALID);

    /* Unregistering again is invalid */
    TEST_ASSERT_EQ(Swc_WatchdogManager_UnregisterEntity(5U), RTE_E_INVALID);
    return 0;
}

static int test_register_capacity_limit(void)
{
    uint8 id;
    Swc_SupervisedEntityConfigType config;

    mock_reset();
    fresh_init();

    for (id = 0U; id < 16U; id++) {
        TEST_ASSERT_EQ(register_active_entity(id, 100U, 0U, 10U), 0);
    }
    make_config(&config, 16U, 100U, 0U, 10U, TRUE);
    TEST_ASSERT_EQ(Swc_WatchdogManager_RegisterEntity(&config), RTE_E_LIMIT);

    /* Freeing a slot allows a new registration */
    TEST_ASSERT_EQ(Swc_WatchdogManager_UnregisterEntity(3U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_WatchdogManager_RegisterEntity(&config), RTE_E_OK);
    return 0;
}

static int test_checkpoint_and_activation(void)
{
    Swc_SupervisedEntityStatusType entityStatus;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(register_active_entity(2U, 1000U, 0U, 10U), 0);

    mockTime = 111U;
    TEST_ASSERT_EQ(Swc_WatchdogManager_CheckpointReached(2U), RTE_E_OK);
    mockTime = 222U;
    TEST_ASSERT_EQ(Swc_WatchdogManager_CheckpointReached(2U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_WatchdogManager_CheckpointReached(9U), RTE_E_INVALID);

    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(2U, &entityStatus), RTE_E_OK);
    TEST_ASSERT_EQ(entityStatus.aliveIndications, 2U);
    TEST_ASSERT_EQ(entityStatus.lastAliveTime, 222U);

    /* Deactivate -> checkpoint not active */
    TEST_ASSERT_EQ(Swc_WatchdogManager_SetEntityActive(2U, FALSE), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_WatchdogManager_CheckpointReached(2U), RTE_E_NOT_ACTIVE);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(2U, &entityStatus), RTE_E_OK);
    TEST_ASSERT_EQ(entityStatus.state, ALIVE_STATE_DEACTIVATED);

    TEST_ASSERT_EQ(Swc_WatchdogManager_SetEntityActive(9U, TRUE), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_WatchdogManager_SetEntityActive(2U, TRUE), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(2U, &entityStatus), RTE_E_OK);
    TEST_ASSERT_EQ(entityStatus.state, ALIVE_STATE_CORRECT);
    return 0;
}

static int test_supervision_correct(void)
{
    Swc_SupervisedEntityStatusType entityStatus;
    Swc_WatchdogManagerStatusType status;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(register_active_entity(1U, 1000U, 0U, 10U), 0);

    mockTime = 100U;
    Swc_WatchdogManager_Trigger();
    TEST_ASSERT_EQ(triggerWrites, 1);
    TEST_ASSERT_EQ(Swc_WatchdogManager_IsGlobalStatusCorrect(), TRUE);

    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(1U, &entityStatus), RTE_E_OK);
    TEST_ASSERT_EQ(entityStatus.state, ALIVE_STATE_CORRECT);
    TEST_ASSERT_EQ(entityStatus.supervisionCycle, 0U);

    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.globalSupervisionCycle, 1U);
    TEST_ASSERT_EQ(status.watchdogStatus, WDG_STATUS_OK);
    return 0;
}

static int test_supervision_timeout_expired(void)
{
    Swc_SupervisedEntityStatusType entityStatus;
    Swc_WatchdogManagerStatusType status;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(register_active_entity(1U, 10U, 0U, 100U), 0);

    mockTime = 100U; /* lastAliveTime = 0 -> timeout on every trigger */
    Swc_WatchdogManager_Trigger(); /* missed = 1 */
    Swc_WatchdogManager_Trigger(); /* missed = 2 */
    Swc_WatchdogManager_Trigger(); /* missed = 3 */
    TEST_ASSERT_EQ(triggerWrites, 3);

    Swc_WatchdogManager_Trigger(); /* missed = 4 -> EXPIRED */
    TEST_ASSERT_EQ(triggerWrites, 3); /* no further hardware trigger */

    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(1U, &entityStatus), RTE_E_OK);
    TEST_ASSERT_EQ(entityStatus.missedIndications, 4U);
    TEST_ASSERT_EQ(entityStatus.state, ALIVE_STATE_EXPIRED);
    TEST_ASSERT_EQ(Swc_WatchdogManager_IsGlobalStatusCorrect(), FALSE);

    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.watchdogStatus, WDG_STATUS_EXPIRED);
    return 0;
}

static int test_supervision_incorrect_counts(void)
{
    Swc_SupervisedEntityStatusType entityStatus;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(register_active_entity(4U, 1000U, 2U, 3U), 0);

    mockTime = 100U;
    TEST_ASSERT_EQ(Swc_WatchdogManager_CheckpointReached(4U), RTE_E_OK); /* 1 < min */
    Swc_WatchdogManager_Trigger();
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(4U, &entityStatus), RTE_E_OK);
    TEST_ASSERT_EQ(entityStatus.state, ALIVE_STATE_INCORRECT);
    TEST_ASSERT_EQ(entityStatus.aliveIndications, 0U); /* reset for next cycle */
    TEST_ASSERT_EQ(triggerWrites, 0); /* global status incorrect -> no trigger */

    /* Too many indications -> INCORRECT */
    TEST_ASSERT_EQ(Swc_WatchdogManager_CheckpointReached(4U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_WatchdogManager_CheckpointReached(4U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_WatchdogManager_CheckpointReached(4U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_WatchdogManager_CheckpointReached(4U), RTE_E_OK);
    Swc_WatchdogManager_Trigger();
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(4U, &entityStatus), RTE_E_OK);
    TEST_ASSERT_EQ(entityStatus.state, ALIVE_STATE_INCORRECT);
    TEST_ASSERT_EQ(triggerWrites, 0);
    return 0;
}

static int test_10ms_alive_indication(void)
{
    Swc_SupervisedEntityStatusType entityStatus;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(register_active_entity(7U, 1000U, 0U, 10U), 0);

    /* Read failure: nothing changes */
    mockTime = 60U;
    Swc_WatchdogManager_10ms();
    TEST_ASSERT_EQ(statusWrites, 1);

    /* Matching entity id increments the alive counter */
    mockReadResult = E_OK;
    mockAliveEntityId = 7U;
    mockTime = 70U;
    Swc_WatchdogManager_10ms();
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(7U, &entityStatus), RTE_E_OK);
    TEST_ASSERT_EQ(entityStatus.aliveIndications, 1U);
    TEST_ASSERT_EQ(entityStatus.lastAliveTime, 70U);

    /* Unknown entity id is ignored */
    mockAliveEntityId = 8U;
    Swc_WatchdogManager_10ms();
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(7U, &entityStatus), RTE_E_OK);
    TEST_ASSERT_EQ(entityStatus.aliveIndications, 1U);
    return 0;
}

static int test_timeout_check_triggers_hardware(void)
{
    Swc_WatchdogManagerStatusType status;

    mock_reset();
    fresh_init();

    mockTime = 50U;
    Swc_WatchdogManager_10ms(); /* 50 - 0 <= 100 -> no trigger */
    TEST_ASSERT_EQ(triggerWrites, 0);

    mockTime = 200U;
    Swc_WatchdogManager_10ms(); /* 200 - 0 > 100 -> trigger */
    TEST_ASSERT_EQ(triggerWrites, 1);
    TEST_ASSERT_EQ(lastWrittenStatus, WDG_STATUS_OK);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.watchdogStatus, WDG_STATUS_OK);

    mockTime = 250U;
    Swc_WatchdogManager_10ms(); /* 250 - 200 <= 100 -> no trigger */
    TEST_ASSERT_EQ(triggerWrites, 1);
    return 0;
}

static int test_timeout_check_incorrect_entity(void)
{
    Swc_WatchdogManagerStatusType status;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(register_active_entity(6U, 1000U, 5U, 10U), 0);

    /* No indications -> INCORRECT after supervision */
    mockTime = 10U;
    Swc_WatchdogManager_Trigger();
    TEST_ASSERT_EQ(Swc_WatchdogManager_IsGlobalStatusCorrect(), FALSE);

    /* Advance past the trigger timeout: watchdog must not be refreshed */
    mockTime = 500U;
    Swc_WatchdogManager_10ms();
    TEST_ASSERT_EQ(triggerWrites, 0);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.watchdogStatus, WDG_STATUS_EXPIRED);
    return 0;
}

static int test_mainfunction_dispatch(void)
{
    int i;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(register_active_entity(1U, 100U, 0U, 100U), 0);

    mockTime = 50U;
    for (i = 0; i < 10; i++) {
        Swc_WatchdogManager_MainFunction();
    }
    TEST_ASSERT_EQ(statusWrites, 10);
    /* 100ms branch runs once: supervision + hardware trigger */
    TEST_ASSERT_EQ(triggerWrites, 1);

    {
        Swc_WatchdogManagerStatusType status;
        TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_OK);
        TEST_ASSERT_EQ(status.globalSupervisionCycle, 1U);
    }
    return 0;
}

static int test_handle_expiration(void)
{
    Swc_WatchdogManagerStatusType status;

    mock_reset();
    fresh_init();
    detCalls = 0;

    Swc_WatchdogManager_HandleExpiration();
    TEST_ASSERT_EQ(detCalls, 1);
    TEST_ASSERT_EQ(lastDetApi, 0x50U);
    TEST_ASSERT_EQ(lastDetError, RTE_E_NOT_OK);
    TEST_ASSERT_EQ(lastWrittenStatus, WDG_STATUS_EXPIRED);
    TEST_ASSERT_EQ(statusWrites, 1);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.watchdogStatus, WDG_STATUS_EXPIRED);
    return 0;
}

static int test_deinit_resets_entities(void)
{
    Swc_WatchdogManagerStatusType status;
    Swc_SupervisedEntityStatusType entityStatus;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(register_active_entity(1U, 100U, 0U, 10U), 0);
    Swc_WatchdogManager_Deinit();

    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_UNINIT);
    Swc_WatchdogManager_Init();
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetEntityStatus(1U, &entityStatus), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_WatchdogManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.numSupervisedEntities, 0U);
    TEST_ASSERT_EQ(status.watchdogStatus, WDG_STATUS_STOPPED);
    return 0;
}

/* ============================================================================
 * Runner
 * ============================================================================ */
typedef struct {
    const char* name;
    int (*func)(void);
} TestCaseType;

static const TestCaseType testCases[] = {
    { "uninitialized_paths", test_uninitialized_paths },
    { "init_status_deinit", test_init_status_deinit },
    { "register_unregister", test_register_unregister },
    { "register_capacity_limit", test_register_capacity_limit },
    { "checkpoint_activation", test_checkpoint_and_activation },
    { "supervision_correct", test_supervision_correct },
    { "supervision_expired", test_supervision_timeout_expired },
    { "supervision_incorrect", test_supervision_incorrect_counts },
    { "alive_indication_10ms", test_10ms_alive_indication },
    { "timeout_triggers_hardware", test_timeout_check_triggers_hardware },
    { "timeout_incorrect_entity", test_timeout_check_incorrect_entity },
    { "mainfunction_dispatch", test_mainfunction_dispatch },
    { "handle_expiration", test_handle_expiration },
    { "deinit_resets", test_deinit_resets_entities }
};

int main(void)
{
    size_t i;
    size_t numTests = sizeof(testCases) / sizeof(testCases[0]);

    printf("=== WatchdogManager unit tests ===\n");
    for (i = 0; i < numTests; i++) {
        int result;
        tests_run++;
        printf("[%zu/%zu] %s ... ", i + 1, numTests, testCases[i].name);
        result = testCases[i].func();
        if (result == 0) {
            tests_passed++;
            printf("PASS\n");
        } else {
            printf("FAIL\n");
        }
    }
    printf("=== %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
