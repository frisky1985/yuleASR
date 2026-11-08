/**
 * @file test_vehicle_dynamics.c
 * @brief Swc_VehicleDynamics host unit tests (main + assert style)
 *
 * Covers init/deinit, 10ms/20ms runnables, stability intervention state
 * machine, traction loss, slip/brake calculations and public APIs.
 * RTE port APIs and Det are mocked in this file.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Swc_VehicleDynamics.h"

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
static uint16 mockWheelSpeed;
static sint16 mockSteeringAngle;
static sint16 mockAccel;
static Std_ReturnType mockReadResult;
static Std_ReturnType mockWriteResult;
static uint32 mockTime;

static Swc_VdcStateType lastWrittenState;
static Swc_VehicleMotionType lastWrittenMotion;
static Swc_VehicleDynamicsOutputType lastOutput;
static int stateWrites;
static int motionWrites;
static int outputWrites;
static int detCalls;

Std_ReturnType Rte_Read_SWC_VEHICLEDYNAMICS_PORT_WHEEL_SPEEDS_R(void* data)
{
    if (mockReadResult != E_OK) { return mockReadResult; }
    *(uint16*)data = mockWheelSpeed;
    return E_OK;
}

Std_ReturnType Rte_Read_SWC_VEHICLEDYNAMICS_PORT_STEERING_ANGLE_R(void* data)
{
    if (mockReadResult != E_OK) { return mockReadResult; }
    *(sint16*)data = mockSteeringAngle;
    return E_OK;
}

Std_ReturnType Rte_Read_SWC_VEHICLEDYNAMICS_PORT_ACCEL_DATA_R(void* data)
{
    if (mockReadResult != E_OK) { return mockReadResult; }
    *(sint16*)data = mockAccel;
    return E_OK;
}

Std_ReturnType Rte_Write_SWC_VEHICLEDYNAMICS_PORT_VDC_STATE_P(const void* data)
{
    lastWrittenState = *(const Swc_VdcStateType*)data;
    stateWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_VEHICLEDYNAMICS_PORT_MOTION_DATA_P(const void* data)
{
    lastWrittenMotion = *(const Swc_VehicleMotionType*)data;
    motionWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_VEHICLEDYNAMICS_PORT_VDC_OUTPUT_P(const void* data)
{
    lastOutput = *(const Swc_VehicleDynamicsOutputType*)data;
    outputWrites++;
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
    (void)ApiId;
    (void)ErrorId;
    detCalls++;
    return E_OK;
}

/* ============================================================================
 * Helpers
 * ============================================================================ */
static void fresh_init(void)
{
    Swc_VehicleDynamics_Deinit();
    Swc_VehicleDynamics_Init();
}

static void mock_reset(void)
{
    mockWheelSpeed = 0U;
    mockSteeringAngle = 0;
    mockAccel = 0;
    mockReadResult = E_OK;
    mockWriteResult = E_OK;
    mockTime = 100U;
    memset(&lastWrittenState, 0, sizeof(lastWrittenState));
    memset(&lastWrittenMotion, 0, sizeof(lastWrittenMotion));
    memset(&lastOutput, 0, sizeof(lastOutput));
    stateWrites = 0;
    motionWrites = 0;
    outputWrites = 0;
    detCalls = 0;
}

/* ============================================================================
 * Tests
 * ============================================================================ */
static int test_uninitialized_paths(void)
{
    Swc_VdcStateType state = VDC_STATE_ACTIVE;
    Swc_VehicleMotionType motion;

    mock_reset();
    Swc_VehicleDynamics_10ms();
    Swc_VehicleDynamics_20ms();
    Swc_VehicleDynamics_MainFunction();
    TEST_ASSERT_EQ(outputWrites, 0);
    TEST_ASSERT_EQ(stateWrites, 0);

    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_SetVdcMode(VDC_MODE_NORMAL), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetMotionData(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetMotionData(&motion), RTE_E_UNINIT);

    Swc_VehicleDynamics_Deinit(); /* no-op */
    return 0;
}

static int test_init_getters_and_deinit(void)
{
    Swc_VdcStateType state = VDC_STATE_INACTIVE;
    Swc_VehicleMotionType motion;

    mock_reset();
    Swc_VehicleDynamics_Init();
    TEST_ASSERT_EQ(detCalls, 1);
    Swc_VehicleDynamics_Init(); /* idempotent */
    TEST_ASSERT_EQ(detCalls, 1);

    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, VDC_STATE_ACTIVE);

    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetMotionData(&motion), RTE_E_OK);
    TEST_ASSERT_EQ(motion.vehicleSpeed, 0U);
    TEST_ASSERT_EQ(motion.steeringAngle, 0);
    TEST_ASSERT_EQ(motion.wheelSpeedFL, 0U);

    Swc_VehicleDynamics_Deinit();
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetMotionData(&motion), RTE_E_UNINIT);

    Swc_VehicleDynamics_Init();
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_OK);
    return 0;
}

static int test_set_vdc_mode(void)
{
    Swc_VdcStateType state = VDC_STATE_ACTIVE;

    fresh_init();
    TEST_ASSERT_EQ(Swc_VehicleDynamics_SetVdcMode(VDC_MODE_SPORT), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, VDC_STATE_ACTIVE);

    TEST_ASSERT_EQ(Swc_VehicleDynamics_SetVdcMode(VDC_MODE_DISABLED), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, VDC_STATE_INACTIVE);

    /* Leaving DISABLED re-activates from INACTIVE */
    TEST_ASSERT_EQ(Swc_VehicleDynamics_SetVdcMode(VDC_MODE_NORMAL), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, VDC_STATE_ACTIVE);

    TEST_ASSERT_EQ(Swc_VehicleDynamics_SetVdcMode((Swc_VdcModeType)99), RTE_E_INVALID);
    return 0;
}

static int test_slip_ratio(void)
{
    TEST_ASSERT_EQ(Swc_VehicleDynamics_CalculateSlipRatio(100U, 0U), 0);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_CalculateSlipRatio(110U, 100U), 10);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_CalculateSlipRatio(90U, 100U), -10);
    /* Clamp to +/-100 */
    TEST_ASSERT_EQ(Swc_VehicleDynamics_CalculateSlipRatio(300U, 100U), 100);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_CalculateSlipRatio(0U, 100U), -100);
    return 0;
}

static int test_brake_intervention(void)
{
    /* (|error| * 2 + |lat| / 10) * 90 / 100 */
    TEST_ASSERT_EQ(Swc_VehicleDynamics_CalculateBrakeIntervention(100, 0, 0), 100);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_CalculateBrakeIntervention(1, 0, 0), 1);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_CalculateBrakeIntervention(0, 0, 500), 45);
    /* Saturated at 100 */
    TEST_ASSERT_EQ(Swc_VehicleDynamics_CalculateBrakeIntervention(1000, 0, 2000), 100);
    return 0;
}

static int test_stable_operation(void)
{
    Swc_VdcStateType state = VDC_STATE_FAULT;

    fresh_init();
    mock_reset();

    Swc_VehicleDynamics_10ms();
    Swc_VehicleDynamics_20ms();

    TEST_ASSERT_EQ(outputWrites, 1);
    TEST_ASSERT_EQ(stateWrites, 1);
    TEST_ASSERT_EQ(motionWrites, 1);
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, VDC_STATE_ACTIVE);
    TEST_ASSERT_EQ(lastOutput.stabilityIntervention, FALSE);
    TEST_ASSERT_EQ(lastOutput.brakeForceFront, 0);
    return 0;
}

static int test_stability_intervention(void)
{
    Swc_VdcStateType state = VDC_STATE_ACTIVE;

    fresh_init();
    mock_reset();
    mockWheelSpeed = 200U; /* FL=200, FR=0 -> avg speed = 100 */
    mockSteeringAngle = 100;
    mockAccel = 50;

    /* 10ms updates motion (speed=100), 20ms computes target yaw = 100 */
    Swc_VehicleDynamics_10ms();
    Swc_VehicleDynamics_20ms();
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, VDC_STATE_INTERVENING);
    TEST_ASSERT_EQ(lastWrittenState, VDC_STATE_INTERVENING);

    /* Next 10ms applies brake intervention and torque reduction */
    Swc_VehicleDynamics_10ms();
    TEST_ASSERT_EQ(lastOutput.stabilityIntervention, TRUE);
    TEST_ASSERT_EQ(lastOutput.tractionControlActive, TRUE);
    TEST_ASSERT(lastOutput.torqueReduction > 0);
    TEST_ASSERT(lastOutput.brakeForceFront > 0);
    TEST_ASSERT(lastOutput.brakeForceRear > 0);
    return 0;
}

static int test_return_to_active(void)
{
    Swc_VdcStateType state = VDC_STATE_FAULT;

    fresh_init();
    mock_reset();
    mockWheelSpeed = 200U; /* FL=200, FR=0 -> avg speed = 100 */
    mockSteeringAngle = 100;
    Swc_VehicleDynamics_10ms();
    Swc_VehicleDynamics_20ms();
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, VDC_STATE_INTERVENING);

    /* Stable again: yaw error returns to zero, counter decrements to 0 */
    mockSteeringAngle = 0;
    Swc_VehicleDynamics_10ms();
    Swc_VehicleDynamics_20ms();
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, VDC_STATE_ACTIVE);
    return 0;
}

static int test_disabled_mode_no_intervention(void)
{
    Swc_VdcStateType state = VDC_STATE_ACTIVE;

    fresh_init();
    mock_reset();
    TEST_ASSERT_EQ(Swc_VehicleDynamics_SetVdcMode(VDC_MODE_DISABLED), RTE_E_OK);
    mockWheelSpeed = 200U; /* FL=200, FR=0 -> avg speed = 100 */
    mockSteeringAngle = 100;

    Swc_VehicleDynamics_10ms();
    Swc_VehicleDynamics_20ms();
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetVdcState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, VDC_STATE_INACTIVE);
    return 0;
}

static int test_traction_loss_low_speed(void)
{
    fresh_init();
    mock_reset();
    mockWheelSpeed = 4U;   /* vehicle speed below 5 km/h */

    Swc_VehicleDynamics_10ms();
    TEST_ASSERT_EQ(lastOutput.tractionControlActive, FALSE);
    return 0;
}

static int test_mainfunction_dispatch(void)
{
    fresh_init();
    mock_reset();

    Swc_VehicleDynamics_MainFunction();
    TEST_ASSERT_EQ(outputWrites, 1);
    TEST_ASSERT_EQ(stateWrites, 0); /* 20ms branch on every 2nd call */

    Swc_VehicleDynamics_MainFunction();
    TEST_ASSERT_EQ(outputWrites, 2);
    TEST_ASSERT_EQ(stateWrites, 1);
    TEST_ASSERT_EQ(motionWrites, 1);
    return 0;
}

static int test_motion_readback(void)
{
    Swc_VehicleMotionType motion;

    fresh_init();
    mock_reset();
    mockWheelSpeed = 120U;
    mockSteeringAngle = 15;
    mockAccel = 25;

    Swc_VehicleDynamics_10ms();
    TEST_ASSERT_EQ(Swc_VehicleDynamics_GetMotionData(&motion), RTE_E_OK);
    TEST_ASSERT_EQ(motion.vehicleSpeed, 60U); /* average of FL=120, FR=0 */
    TEST_ASSERT_EQ(motion.wheelSpeedFL, 120U);
    TEST_ASSERT_EQ(motion.steeringAngle, 15);
    TEST_ASSERT_EQ(motion.longitudinalAccel, 25);
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
    { "set_vdc_mode", test_set_vdc_mode },
    { "slip_ratio", test_slip_ratio },
    { "brake_intervention", test_brake_intervention },
    { "stable_operation", test_stable_operation },
    { "stability_intervention", test_stability_intervention },
    { "return_to_active", test_return_to_active },
    { "disabled_mode", test_disabled_mode_no_intervention },
    { "traction_loss_low_speed", test_traction_loss_low_speed },
    { "mainfunction_dispatch", test_mainfunction_dispatch },
    { "motion_readback", test_motion_readback }
};

int main(void)
{
    size_t i;
    size_t numTests = sizeof(testCases) / sizeof(testCases[0]);

    printf("=== VehicleDynamics unit tests ===\n");
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
