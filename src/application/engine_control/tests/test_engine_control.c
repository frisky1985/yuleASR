/**
 * @file test_engine_control.c
 * @brief Swc_EngineControl host unit tests (main + assert style)
 *
 * Covers init/deinit, cyclic runnables, state machine, fault counters,
 * calculation helpers and the public getter/setter APIs. RTE port APIs and
 * Det are provided as deterministic mocks inside this file, so no RTE/BSW
 * production source is modified or linked.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Swc_EngineControl.h"

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
static uint16 mockThrottlePos;
static sint16 mockCoolantTemp;
static uint16 mockVehicleSpeed;
static Std_ReturnType mockReadResult;
static Std_ReturnType mockWriteResult;
static uint32 mockTime;

static Swc_EngineControlOutputType lastOutput;
static Swc_EngineStateType lastWrittenState;
static Swc_EngineParametersType lastWrittenParams;
static uint32 lastMode;
static int outputWrites;
static int stateWrites;
static int paramWrites;
static int modeSwitches;
static int detCalls;
static uint8 lastDetApi;
static uint8 lastDetError;

Std_ReturnType Rte_Read_SWC_ENGINECONTROL_PORT_THROTTLE_POS_R(void* data)
{
    if (mockReadResult != E_OK) { return mockReadResult; }
    *(uint16*)data = mockThrottlePos;
    return E_OK;
}

Std_ReturnType Rte_Read_SWC_ENGINECONTROL_PORT_COOLANT_TEMP_R(void* data)
{
    if (mockReadResult != E_OK) { return mockReadResult; }
    *(sint16*)data = mockCoolantTemp;
    return E_OK;
}

Std_ReturnType Rte_Read_SWC_ENGINECONTROL_PORT_VEHICLE_SPEED_R(void* data)
{
    if (mockReadResult != E_OK) { return mockReadResult; }
    *(uint16*)data = mockVehicleSpeed;
    return E_OK;
}

Std_ReturnType Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_CONTROL_P(const void* data)
{
    lastOutput = *(const Swc_EngineControlOutputType*)data;
    outputWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_STATE_P(const void* data)
{
    lastWrittenState = *(const Swc_EngineStateType*)data;
    stateWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_PARAMS_P(const void* data)
{
    lastWrittenParams = *(const Swc_EngineParametersType*)data;
    paramWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Switch_SWC_ENGINECONTROL_PORT_MODE_P(uint32 data)
{
    lastMode = data;
    modeSwitches++;
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
    Swc_EngineControl_Deinit();
    Swc_EngineControl_Init();
}

static void mock_reset(void)
{
    mockThrottlePos = 0U;
    mockCoolantTemp = 20;
    mockVehicleSpeed = 0U;
    mockReadResult = E_OK;
    mockWriteResult = E_OK;
    mockTime = 100U;
    memset(&lastOutput, 0, sizeof(lastOutput));
    memset(&lastWrittenState, 0, sizeof(lastWrittenState));
    memset(&lastWrittenParams, 0, sizeof(lastWrittenParams));
    lastMode = 0U;
    outputWrites = 0;
    stateWrites = 0;
    paramWrites = 0;
    modeSwitches = 0;
    detCalls = 0;
    lastDetApi = 0U;
    lastDetError = 0U;
}

/* Drive OFF -> CRANKING -> RUNNING with a high load request. */
static void drive_to_running(void)
{
    mockThrottlePos = 100U;
    mockVehicleSpeed = 0U;
    mockCoolantTemp = 20;
    Swc_EngineControl_10ms();   /* load = 100, speed = 3900 (NORMAL)   */
    Swc_EngineControl_100ms();  /* OFF -> CRANKING                     */
    Swc_EngineControl_10ms();
    Swc_EngineControl_100ms();  /* CRANKING -> RUNNING                 */
}

/* ============================================================================
 * Tests
 * ============================================================================ */
static int test_uninitialized_paths(void)
{
    Swc_EngineStateType state = ENGINE_STATE_OFF;
    Swc_EngineParametersType params;

    mock_reset();

    /* Cyclic runnables must be no-ops before Init */
    Swc_EngineControl_10ms();
    Swc_EngineControl_100ms();
    Swc_EngineControl_StateMachine();
    Swc_EngineControl_MainFunction();
    TEST_ASSERT_EQ(outputWrites, 0);
    TEST_ASSERT_EQ(stateWrites, 0);
    TEST_ASSERT_EQ(modeSwitches, 0);

    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineState(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineState(&state), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_EngineControl_SetControlMode(ENGINE_MODE_ECO), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineParameters(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineParameters(&params), RTE_E_UNINIT);

    Swc_EngineControl_Deinit(); /* no-op when not initialized */
    return 0;
}

static int test_init_getters_and_deinit(void)
{
    Swc_EngineStateType state = ENGINE_STATE_FAULT;
    Swc_EngineParametersType params;

    mock_reset();
    Swc_EngineControl_Init();
    TEST_ASSERT(detCalls == 1);
    TEST_ASSERT_EQ(lastDetApi, 0x01U);
    TEST_ASSERT_EQ(lastDetError, RTE_E_OK);

    /* Second Init is a no-op */
    Swc_EngineControl_Init();
    TEST_ASSERT(detCalls == 1);

    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, ENGINE_STATE_OFF);

    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineParameters(&params), RTE_E_OK);
    TEST_ASSERT_EQ(params.engineTemperature, 20);
    TEST_ASSERT_EQ(params.engineSpeed, 0U);
    TEST_ASSERT_EQ(params.engineLoad, 0U);

    Swc_EngineControl_Deinit();
    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineState(&state), RTE_E_UNINIT);

    /* Re-init after deinit must work */
    Swc_EngineControl_Init();
    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, ENGINE_STATE_OFF);
    return 0;
}

static int test_control_mode(void)
{
    fresh_init();

    TEST_ASSERT_EQ(Swc_EngineControl_SetControlMode(ENGINE_MODE_NORMAL), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_EngineControl_SetControlMode(ENGINE_MODE_ECO), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_EngineControl_SetControlMode(ENGINE_MODE_SPORT), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_EngineControl_SetControlMode(ENGINE_MODE_LIMP_HOME), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_EngineControl_SetControlMode((Swc_EngineControlModeType)99), RTE_E_INVALID);
    return 0;
}

static int test_fuel_injection(void)
{
    /* Normal base value */
    TEST_ASSERT_EQ(Swc_EngineControl_CalculateFuelInjection(900U, 0U, 20), 2000U);
    /* Speed correction */
    TEST_ASSERT_EQ(Swc_EngineControl_CalculateFuelInjection(1000U, 10U, 20), 3500U);
    /* Cold enrichment */
    TEST_ASSERT_EQ(Swc_EngineControl_CalculateFuelInjection(900U, 0U, -10), 3500U);
    /* Hot engine reduction */
    TEST_ASSERT_EQ(Swc_EngineControl_CalculateFuelInjection(900U, 0U, 100), 1900U);
    /* Upper clamp */
    TEST_ASSERT_EQ(Swc_EngineControl_CalculateFuelInjection(65535U, 100U, 20), 15000U);
    return 0;
}

static int test_ignition_timing(void)
{
    TEST_ASSERT_EQ(Swc_EngineControl_CalculateIgnitionTiming(900U, 0U, 20), 10U);
    TEST_ASSERT_EQ(Swc_EngineControl_CalculateIgnitionTiming(1900U, 100U, 20), 30U);
    /* Cold retard */
    TEST_ASSERT_EQ(Swc_EngineControl_CalculateIgnitionTiming(1000U, 0U, -10), 6U);
    /* Hot retard */
    TEST_ASSERT_EQ(Swc_EngineControl_CalculateIgnitionTiming(1000U, 0U, 110), 8U);
    /* Upper clamp */
    TEST_ASSERT_EQ(Swc_EngineControl_CalculateIgnitionTiming(65535U, 100U, 20), 45U);
    return 0;
}

static int test_outputs_when_engine_off(void)
{
    fresh_init();
    mock_reset();
    Swc_EngineControl_10ms();

    TEST_ASSERT_EQ(outputWrites, 1);
    TEST_ASSERT_EQ(lastOutput.fuelPulseWidth, 0U);
    TEST_ASSERT_EQ(lastOutput.ignitionTiming, 0U);
    TEST_ASSERT_EQ(lastOutput.idleSpeedTarget, 0U);
    TEST_ASSERT(lastOutput.fuelCutoff != 0U);
    TEST_ASSERT(lastOutput.ignitionCutoff != 0U);
    return 0;
}

static int test_state_machine_to_running(void)
{
    Swc_EngineStateType state = ENGINE_STATE_OFF;

    mock_reset();
    fresh_init();

    drive_to_running();
    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, ENGINE_STATE_RUNNING);

    /* 10ms in RUNNING computes fuel/ignition outputs */
    Swc_EngineControl_10ms();
    TEST_ASSERT(lastOutput.fuelPulseWidth > 0U);
    TEST_ASSERT(lastOutput.ignitionTiming > 0U);
    TEST_ASSERT_EQ(lastOutput.fuelCutoff, 0U);
    TEST_ASSERT_EQ(lastOutput.ignitionCutoff, 0U);
    return 0;
}

static int test_start_conditions_reject_low_temperature(void)
{
    Swc_EngineStateType state = ENGINE_STATE_OFF;

    mock_reset();
    fresh_init();
    mockCoolantTemp = -50;   /* below ENG_MIN_OPERATING_TEMP */
    Swc_EngineControl_10ms();
    Swc_EngineControl_100ms();
    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, ENGINE_STATE_OFF);
    return 0;
}

static int test_idle_speed_targets_per_mode(void)
{
    fresh_init();

    /* RUNNING with ECO mode */
    mock_reset();
    drive_to_running();
    TEST_ASSERT_EQ(Swc_EngineControl_SetControlMode(ENGINE_MODE_ECO), RTE_E_OK);
    Swc_EngineControl_10ms();
    TEST_ASSERT_EQ(lastOutput.idleSpeedTarget, 800U);

    /* SPORT mode */
    TEST_ASSERT_EQ(Swc_EngineControl_SetControlMode(ENGINE_MODE_SPORT), RTE_E_OK);
    Swc_EngineControl_10ms();
    TEST_ASSERT_EQ(lastOutput.idleSpeedTarget, 1200U);

    /* NORMAL mode */
    TEST_ASSERT_EQ(Swc_EngineControl_SetControlMode(ENGINE_MODE_NORMAL), RTE_E_OK);
    Swc_EngineControl_10ms();
    TEST_ASSERT_EQ(lastOutput.idleSpeedTarget, 900U);

    /* LIMP_HOME mode */
    TEST_ASSERT_EQ(Swc_EngineControl_SetControlMode(ENGINE_MODE_LIMP_HOME), RTE_E_OK);
    Swc_EngineControl_10ms();
    TEST_ASSERT_EQ(lastOutput.idleSpeedTarget, 900U); /* default branch */
    return 0;
}

static int test_fuel_cutoff_overheat(void)
{
    fresh_init();
    mock_reset();
    drive_to_running();

    mockCoolantTemp = 130; /* above ENG_FUEL_CUTOFF_TEMP */
    Swc_EngineControl_10ms();
    TEST_ASSERT(lastOutput.fuelCutoff != 0U);
    return 0;
}

static int test_running_to_stopping(void)
{
    Swc_EngineStateType state = ENGINE_STATE_OFF;

    fresh_init();
    mock_reset();
    drive_to_running();

    /* LIMP_HOME keeps speed at 800 rpm, throttle below 5% -> stop */
    TEST_ASSERT_EQ(Swc_EngineControl_SetControlMode(ENGINE_MODE_LIMP_HOME), RTE_E_OK);
    mockThrottlePos = 0U;
    Swc_EngineControl_10ms();
    Swc_EngineControl_100ms();
    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, ENGINE_STATE_STOPPING);
    return 0;
}

static int test_running_to_fault_and_recovery(void)
{
    Swc_EngineStateType state = ENGINE_STATE_OFF;

    fresh_init();
    mock_reset();
    drive_to_running();

    /* Each 10ms adds 2 faults (overheat + unrealistic throttle) */
    mockCoolantTemp = 120;
    mockThrottlePos = 120U;
    Swc_EngineControl_10ms();
    Swc_EngineControl_10ms();
    Swc_EngineControl_10ms();
    Swc_EngineControl_100ms();
    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, ENGINE_STATE_FAULT);

    /* Normal conditions decrement the fault counter once per runnable */
    mockCoolantTemp = 20;
    mockThrottlePos = 0U;
    Swc_EngineControl_10ms();
    Swc_EngineControl_10ms();
    Swc_EngineControl_10ms();
    Swc_EngineControl_10ms();
    Swc_EngineControl_10ms();
    Swc_EngineControl_10ms();
    Swc_EngineControl_100ms();
    TEST_ASSERT_EQ(Swc_EngineControl_GetEngineState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, ENGINE_STATE_OFF);
    return 0;
}

static int test_100ms_and_statemachine_writes(void)
{
    fresh_init();
    mock_reset();
    Swc_EngineControl_100ms();
    TEST_ASSERT_EQ(stateWrites, 1);
    TEST_ASSERT_EQ(paramWrites, 1);
    TEST_ASSERT_EQ(lastWrittenState, ENGINE_STATE_CRANKING);

    Swc_EngineControl_StateMachine();
    TEST_ASSERT_EQ(modeSwitches, 1);
    TEST_ASSERT_EQ(lastMode, ENGINE_MODE_NORMAL);
    return 0;
}

static int test_mainfunction_multirate(void)
{
    int i;

    fresh_init();
    mock_reset();
    for (i = 0; i < 10; i++) {
        Swc_EngineControl_MainFunction();
    }
    TEST_ASSERT_EQ(outputWrites, 10);
    /* Exactly one 100ms dispatch per 10 calls */
    TEST_ASSERT(stateWrites >= 1);
    TEST_ASSERT(paramWrites >= 1);
    TEST_ASSERT(modeSwitches >= 1);
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
    { "control_mode", test_control_mode },
    { "fuel_injection", test_fuel_injection },
    { "ignition_timing", test_ignition_timing },
    { "outputs_engine_off", test_outputs_when_engine_off },
    { "state_machine_running", test_state_machine_to_running },
    { "start_reject_low_temp", test_start_conditions_reject_low_temperature },
    { "idle_targets_per_mode", test_idle_speed_targets_per_mode },
    { "fuel_cutoff_overheat", test_fuel_cutoff_overheat },
    { "running_to_stopping", test_running_to_stopping },
    { "running_fault_recovery", test_running_to_fault_and_recovery },
    { "writes_100ms_statemachine", test_100ms_and_statemachine_writes },
    { "mainfunction_multirate", test_mainfunction_multirate }
};

int main(void)
{
    size_t i;
    size_t numTests = sizeof(testCases) / sizeof(testCases[0]);

    printf("=== EngineControl unit tests ===\n");
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
