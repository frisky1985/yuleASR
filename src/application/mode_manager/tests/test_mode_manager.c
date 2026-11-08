/**
 * @file test_mode_manager.c
 * @brief Swc_ModeManager host unit tests (main + assert style)
 *
 * Covers init/deinit, mode request validation, the full mode transition state
 * machine (notification -> acknowledgement -> completion), all system-state
 * mappings, transition timeout, forced transitions and component APIs.
 * RTE port APIs and Det are mocked in this file.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Swc_ModeManager.h"

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
static Swc_ModeTransitionRequestType mockRequest;
static Std_ReturnType mockReadResult;
static Std_ReturnType mockWriteResult;
static uint32 mockTime;

static Swc_SystemModeType lastWrittenMode;
static Swc_SystemStateType lastWrittenState;
static Swc_ComponentModeType lastNotification;
static int modeWrites;
static int stateWrites;
static int notificationWrites;
static int detCalls;
static uint8 lastDetApi;
static uint8 lastDetError;

Std_ReturnType Rte_Read_SWC_MODEMANAGER_PORT_MODE_REQUEST_R(void* data)
{
    if (mockReadResult != E_OK) { return mockReadResult; }
    *(Swc_ModeTransitionRequestType*)data = mockRequest;
    return E_OK;
}

Std_ReturnType Rte_Write_SWC_MODEMANAGER_PORT_SYSTEM_MODE_P(const void* data)
{
    lastWrittenMode = *(const Swc_SystemModeType*)data;
    modeWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_MODEMANAGER_PORT_SYSTEM_STATE_P(const void* data)
{
    lastWrittenState = *(const Swc_SystemStateType*)data;
    stateWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_MODEMANAGER_PORT_MODE_NOTIFICATION_P(const void* data)
{
    lastNotification = *(const Swc_ComponentModeType*)data;
    notificationWrites++;
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
    Swc_ModeManager_Deinit();
    Swc_ModeManager_Init();
}

static void mock_reset(void)
{
    memset(&mockRequest, 0, sizeof(mockRequest));
    memset(&lastNotification, 0, sizeof(lastNotification));
    mockReadResult = E_NOT_OK;
    mockWriteResult = E_OK;
    mockTime = 1000U;
    lastWrittenMode = SYSTEM_MODE_OFF;
    lastWrittenState = SYSTEM_STATE_OFF;
    modeWrites = 0;
    stateWrites = 0;
    notificationWrites = 0;
    detCalls = 0;
    lastDetApi = 0U;
    lastDetError = 0U;
}

static void make_request(Swc_SystemModeType target, uint8 priority, boolean forced)
{
    memset(&mockRequest, 0, sizeof(mockRequest));
    mockRequest.targetMode = target;
    mockRequest.requestSource = 0x10U;
    mockRequest.requestTime = mockTime;
    mockRequest.priority = priority;
    mockRequest.isForced = forced;
}

static void ack_all_components(void)
{
    uint8 id;
    for (id = 0U; id < 16U; id++) {
        (void)Swc_ModeManager_AcknowledgeModeChange(id, TRUE);
    }
}

/* Queue a request (RTE read path disabled) and start the transition. */
static int start_transition(Swc_SystemModeType target)
{
    Swc_ModeTransitionRequestType request;

    memset(&request, 0, sizeof(request));
    request.targetMode = target;
    request.requestTime = mockTime;
    request.priority = 0U;
    request.isForced = FALSE;
    mockReadResult = E_NOT_OK;
    if (Swc_ModeManager_RequestModeTransition(&request) != MODE_TRANSITION_OK) {
        return -1;
    }
    Swc_ModeManager_50ms();
    return 0;
}

/* Complete a transition: start, acknowledge all, execute on next 50ms. */
static int complete_transition(Swc_SystemModeType target)
{
    if (start_transition(target) != 0) { return -1; }
    ack_all_components();
    Swc_ModeManager_50ms();
    return 0;
}

/* ============================================================================
 * Tests
 * ============================================================================ */
static int test_uninitialized_paths(void)
{
    Swc_SystemModeType mode;
    Swc_SystemStateType state;
    Swc_ModeManagerStatusType status;
    Swc_ModeTransitionRequestType request;

    mock_reset();

    Swc_ModeManager_50ms();
    Swc_ModeManager_ModeSwitch();
    Swc_ModeManager_MainFunction();
    TEST_ASSERT_EQ(modeWrites, 0);
    TEST_ASSERT_EQ(stateWrites, 0);

    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_ModeManager_GetPreviousMode(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_ModeManager_GetPreviousMode(&mode), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(&state), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_ModeManager_GetStatus(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_ModeManager_GetStatus(&status), RTE_E_UNINIT);

    TEST_ASSERT_EQ(Swc_ModeManager_RequestModeTransition(NULL_PTR), MODE_TRANSITION_REJECTED);
    memset(&request, 0, sizeof(request));
    request.targetMode = SYSTEM_MODE_INIT;
    TEST_ASSERT_EQ(Swc_ModeManager_RequestModeTransition(&request), MODE_TRANSITION_REJECTED);

    TEST_ASSERT_EQ(Swc_ModeManager_NotifyComponentMode(0U, SYSTEM_MODE_NORMAL), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_ModeManager_AcknowledgeModeChange(0U, TRUE), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_ModeManager_ForceModeTransition(SYSTEM_MODE_INIT), RTE_E_UNINIT);

    Swc_ModeManager_Deinit(); /* no-op */
    return 0;
}

static int test_init_getters_and_deinit(void)
{
    Swc_SystemModeType mode = SYSTEM_MODE_NORMAL;
    Swc_SystemModeType prev = SYSTEM_MODE_NORMAL;
    Swc_SystemStateType state = SYSTEM_STATE_RUNNING;
    Swc_ModeManagerStatusType status;

    mock_reset();
    Swc_ModeManager_Init();
    TEST_ASSERT_EQ(detCalls, 1);
    TEST_ASSERT_EQ(lastDetApi, 0x01U);
    TEST_ASSERT_EQ(lastDetError, RTE_E_OK);
    Swc_ModeManager_Init();
    TEST_ASSERT_EQ(detCalls, 1);

    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_OFF);
    TEST_ASSERT_EQ(Swc_ModeManager_GetPreviousMode(&prev), RTE_E_OK);
    TEST_ASSERT_EQ(prev, SYSTEM_MODE_OFF);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, SYSTEM_STATE_OFF);
    TEST_ASSERT_EQ(Swc_ModeManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.transitionInProgress, FALSE);
    TEST_ASSERT_EQ(status.modeEntryTime, 1000U);
    TEST_ASSERT_EQ(status.modeDuration, 0U);

    /* All 16 initial components are not ready */
    TEST_ASSERT_EQ(Swc_ModeManager_AreAllComponentsReady(), FALSE);

    Swc_ModeManager_Deinit();
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_UNINIT);

    Swc_ModeManager_Init();
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_OFF);
    return 0;
}

static int test_request_validation(void)
{
    Swc_ModeTransitionRequestType request;

    mock_reset();
    fresh_init();

    /* Out-of-range target */
    memset(&request, 0, sizeof(request));
    request.targetMode = (Swc_SystemModeType)99;
    TEST_ASSERT_EQ(Swc_ModeManager_RequestModeTransition(&request), MODE_TRANSITION_REJECTED);

    /* Invalid transition OFF -> NORMAL */
    request.targetMode = SYSTEM_MODE_NORMAL;
    TEST_ASSERT_EQ(Swc_ModeManager_RequestModeTransition(&request), MODE_TRANSITION_REJECTED);

    /* Valid transition OFF -> INIT */
    request.targetMode = SYSTEM_MODE_INIT;
    TEST_ASSERT_EQ(Swc_ModeManager_RequestModeTransition(&request), MODE_TRANSITION_OK);
    return 0;
}

static int test_transition_completion(void)
{
    Swc_SystemModeType mode = SYSTEM_MODE_OFF;
    Swc_SystemStateType state = SYSTEM_STATE_OFF;
    Swc_ModeManagerStatusType status;
    uint8 id;

    mock_reset();
    fresh_init();
    mockReadResult = E_NOT_OK;

    TEST_ASSERT_EQ(start_transition(SYSTEM_MODE_INIT), 0);

    /* Transition started: INITIALIZING, 16 notifications, components not ready */
    TEST_ASSERT_EQ(Swc_ModeManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT(status.transitionInProgress != 0U);
    TEST_ASSERT_EQ(status.requestedMode, SYSTEM_MODE_INIT);
    TEST_ASSERT_EQ(status.systemState, SYSTEM_STATE_INITIALIZING);
    TEST_ASSERT_EQ(status.currentMode, SYSTEM_MODE_OFF);
    TEST_ASSERT_EQ(notificationWrites, 16);
    TEST_ASSERT_EQ(lastNotification.componentId, 15U);
    TEST_ASSERT_EQ(lastNotification.currentMode, SYSTEM_MODE_INIT);
    TEST_ASSERT_EQ(lastNotification.modeReady, FALSE);

    /* Not complete until every component acknowledges */
    Swc_ModeManager_50ms();
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_OFF);
    TEST_ASSERT_EQ(Swc_ModeManager_AreAllComponentsReady(), FALSE);

    /* Acknowledge 0..14 only */
    for (id = 0U; id < 15U; id++) {
        TEST_ASSERT_EQ(Swc_ModeManager_AcknowledgeModeChange(id, TRUE), RTE_E_OK);
    }
    TEST_ASSERT_EQ(Swc_ModeManager_AreAllComponentsReady(), FALSE);
    Swc_ModeManager_50ms();
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_OFF);

    /* Last acknowledgement completes the transition */
    TEST_ASSERT_EQ(Swc_ModeManager_AcknowledgeModeChange(15U, TRUE), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_ModeManager_AreAllComponentsReady(), TRUE);
    Swc_ModeManager_50ms();

    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_INIT);
    TEST_ASSERT_EQ(Swc_ModeManager_GetPreviousMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_OFF);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, SYSTEM_STATE_INITIALIZING);
    TEST_ASSERT_EQ(Swc_ModeManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.transitionInProgress, FALSE);
    return 0;
}

static int test_state_mappings(void)
{
    Swc_SystemStateType state = SYSTEM_STATE_OFF;
    Swc_SystemModeType mode = SYSTEM_MODE_OFF;

    mock_reset();
    fresh_init();
    mockReadResult = E_NOT_OK;

    TEST_ASSERT_EQ(complete_transition(SYSTEM_MODE_INIT), 0);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, SYSTEM_STATE_INITIALIZING);

    TEST_ASSERT_EQ(complete_transition(SYSTEM_MODE_STANDBY), 0);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, SYSTEM_STATE_RUNNING);

    TEST_ASSERT_EQ(complete_transition(SYSTEM_MODE_NORMAL), 0);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, SYSTEM_STATE_RUNNING);

    TEST_ASSERT_EQ(complete_transition(SYSTEM_MODE_DIAGNOSTIC), 0);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, SYSTEM_STATE_READY);

    TEST_ASSERT_EQ(complete_transition(SYSTEM_MODE_NORMAL), 0);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, SYSTEM_STATE_RUNNING);

    TEST_ASSERT_EQ(complete_transition(SYSTEM_MODE_EMERGENCY), 0);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, SYSTEM_STATE_DEGRADED);
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_EMERGENCY);

    TEST_ASSERT_EQ(complete_transition(SYSTEM_MODE_OFF), 0);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, SYSTEM_STATE_OFF);

    /* SLEEP -> SHUTDOWN path */
    TEST_ASSERT_EQ(complete_transition(SYSTEM_MODE_INIT), 0);
    TEST_ASSERT_EQ(complete_transition(SYSTEM_MODE_STANDBY), 0);
    TEST_ASSERT_EQ(complete_transition(SYSTEM_MODE_NORMAL), 0);
    TEST_ASSERT_EQ(complete_transition(SYSTEM_MODE_SLEEP), 0);
    TEST_ASSERT_EQ(Swc_ModeManager_GetSystemState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, SYSTEM_STATE_SHUTDOWN);
    return 0;
}

static int test_transition_timeout(void)
{
    Swc_ModeManagerStatusType status;

    mock_reset();
    fresh_init();
    mockTime = 1000U;
    TEST_ASSERT_EQ(start_transition(SYSTEM_MODE_INIT), 0);

    detCalls = 0;
    mockTime = 7000U; /* > MODE_TRANSITION_TIMEOUT_MS since modeEntryTime */
    Swc_ModeManager_50ms();

    TEST_ASSERT_EQ(Swc_ModeManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.transitionInProgress, FALSE);
    TEST_ASSERT_EQ(status.systemState, SYSTEM_STATE_ERROR);
    TEST_ASSERT_EQ(status.currentMode, SYSTEM_MODE_OFF);
    TEST_ASSERT(detCalls >= 1);
    TEST_ASSERT_EQ(lastDetApi, 0x50U);
    TEST_ASSERT_EQ(lastDetError, RTE_E_NOT_OK);
    return 0;
}

static int test_priority_and_forced_rejection(void)
{
    Swc_ModeTransitionRequestType request;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(start_transition(SYSTEM_MODE_INIT), 0);

    /* Transition already in progress: lower/equal priority is rejected */
    make_request(SYSTEM_MODE_INIT, 0U, FALSE);
    TEST_ASSERT_EQ(Swc_ModeManager_RequestModeTransition(&mockRequest), MODE_TRANSITION_REJECTED);

    /* Higher priority is accepted */
    make_request(SYSTEM_MODE_INIT, 5U, FALSE);
    TEST_ASSERT_EQ(Swc_ModeManager_RequestModeTransition(&mockRequest), MODE_TRANSITION_OK);

    /* Forced request bypasses the priority check */
    make_request(SYSTEM_MODE_INIT, 0U, TRUE);
    TEST_ASSERT_EQ(Swc_ModeManager_RequestModeTransition(&mockRequest), MODE_TRANSITION_OK);
    return 0;
}

static int test_force_transition(void)
{
    Swc_SystemModeType mode = SYSTEM_MODE_STANDBY;
    Swc_SystemModeType prev = SYSTEM_MODE_STANDBY;

    mock_reset();
    fresh_init();

    TEST_ASSERT_EQ(Swc_ModeManager_ForceModeTransition((Swc_SystemModeType)99), RTE_E_INVALID);

    TEST_ASSERT_EQ(Swc_ModeManager_ForceModeTransition(SYSTEM_MODE_INIT), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_INIT);
    TEST_ASSERT_EQ(Swc_ModeManager_GetPreviousMode(&prev), RTE_E_OK);
    TEST_ASSERT_EQ(prev, SYSTEM_MODE_OFF);

    TEST_ASSERT_EQ(Swc_ModeManager_ForceModeTransition(SYSTEM_MODE_STANDBY), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_STANDBY);

    /* Invalid transition STANDBY -> INIT is refused */
    TEST_ASSERT_EQ(Swc_ModeManager_ForceModeTransition(SYSTEM_MODE_INIT), RTE_E_NOT_OK);
    TEST_ASSERT_EQ(Swc_ModeManager_ForceModeTransition(SYSTEM_MODE_OFF), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_OFF);
    return 0;
}

static int test_component_notifications(void)
{
    Swc_SystemModeType mode = SYSTEM_MODE_OFF;

    mock_reset();
    fresh_init();
    notificationWrites = 0;

    /* Unknown component id */
    TEST_ASSERT_EQ(Swc_ModeManager_NotifyComponentMode(99U, SYSTEM_MODE_NORMAL), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_ModeManager_AcknowledgeModeChange(99U, TRUE), RTE_E_INVALID);

    TEST_ASSERT_EQ(Swc_ModeManager_NotifyComponentMode(3U, SYSTEM_MODE_NORMAL), RTE_E_OK);
    TEST_ASSERT_EQ(notificationWrites, 1);
    TEST_ASSERT_EQ(lastNotification.componentId, 3U);
    TEST_ASSERT_EQ(lastNotification.currentMode, SYSTEM_MODE_NORMAL);

    /* Acknowledge one component */
    TEST_ASSERT_EQ(Swc_ModeManager_AcknowledgeModeChange(3U, TRUE), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_ModeManager_AreAllComponentsReady(), FALSE);

    /* Nack clears the ready flag again */
    TEST_ASSERT_EQ(Swc_ModeManager_AcknowledgeModeChange(3U, FALSE), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_ModeManager_AreAllComponentsReady(), FALSE);

    ack_all_components();
    TEST_ASSERT_EQ(Swc_ModeManager_AreAllComponentsReady(), TRUE);
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_OFF);
    return 0;
}

static int test_rte_request_path_and_modeswitch(void)
{
    Swc_ModeManagerStatusType status;
    Swc_SystemModeType mode = SYSTEM_MODE_OFF;

    mock_reset();
    fresh_init();

    /* ModeSwitch picks up the request through the RTE read path */
    make_request(SYSTEM_MODE_INIT, 0U, FALSE);
    mockReadResult = E_OK;
    notificationWrites = 0;
    Swc_ModeManager_ModeSwitch();

    TEST_ASSERT_EQ(notificationWrites, 16);
    TEST_ASSERT_EQ(Swc_ModeManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT(status.transitionInProgress != 0U);
    TEST_ASSERT_EQ(status.systemState, SYSTEM_STATE_INITIALIZING);
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_OFF);

    /* Invalid target received via RTE simply leaves the pending flag cleared */
    mockTime = 1100U;
    make_request(SYSTEM_MODE_NORMAL, 0U, FALSE);
    Swc_ModeManager_ModeSwitch();
    TEST_ASSERT_EQ(Swc_ModeManager_GetCurrentMode(&mode), RTE_E_OK);
    TEST_ASSERT_EQ(mode, SYSTEM_MODE_OFF);
    return 0;
}

static int test_50ms_outputs_and_duration(void)
{
    Swc_ModeManagerStatusType status;
    int i;

    mock_reset();
    mockTime = 2000U;
    fresh_init();
    mockTime = 2000U;

    Swc_ModeManager_50ms();
    TEST_ASSERT_EQ(modeWrites, 1);
    TEST_ASSERT_EQ(stateWrites, 1);
    TEST_ASSERT_EQ(lastWrittenMode, SYSTEM_MODE_OFF);
    TEST_ASSERT_EQ(lastWrittenState, SYSTEM_STATE_OFF);
    TEST_ASSERT_EQ(Swc_ModeManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.modeDuration, 0U);

    mockTime = 2500U;
    Swc_ModeManager_50ms();
    TEST_ASSERT_EQ(Swc_ModeManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.modeDuration, 500U);

    /* MainFunction dispatches the 50ms path every 5th tick */
    modeWrites = 0;
    stateWrites = 0;
    for (i = 0; i < 10; i++) {
        Swc_ModeManager_MainFunction();
    }
    TEST_ASSERT_EQ(modeWrites, 2);
    TEST_ASSERT_EQ(stateWrites, 2);
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
    { "init_getters_deinit", test_init_getters_and_deinit },
    { "request_validation", test_request_validation },
    { "transition_completion", test_transition_completion },
    { "state_mappings", test_state_mappings },
    { "transition_timeout", test_transition_timeout },
    { "priority_forced", test_priority_and_forced_rejection },
    { "force_transition", test_force_transition },
    { "component_notifications", test_component_notifications },
    { "rte_request_modeswitch", test_rte_request_path_and_modeswitch },
    { "outputs_and_duration", test_50ms_outputs_and_duration }
};

int main(void)
{
    size_t i;
    size_t numTests = sizeof(testCases) / sizeof(testCases[0]);

    printf("=== ModeManager unit tests ===\n");
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
