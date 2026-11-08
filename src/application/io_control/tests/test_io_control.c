/**
 * @file test_io_control.c
 * @brief Swc_IOControl host unit tests (main + assert style)
 *
 * Covers init/deinit, channel state machine, digital/analog/PWM output
 * creation and capacity limits, RTE error propagation, statistics and the
 * cyclic runnables. RTE port APIs and Det are mocked in this file.
 *
 * Note: this component has no public API to create input channels, so the
 * input read paths only reach the "unknown channel" branch; that limitation is
 * asserted explicitly below.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Swc_IOControl.h"

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
static Std_ReturnType mockWriteResult;
static uint32 mockTime;

static Swc_DigitalIOValueType lastDigitalOut;
static Swc_AnalogIOValueType lastAnalogOut;
static Swc_PwmIOValueType lastPwmOut;
static Swc_IOStateType lastWrittenState;
static int digitalWrites;
static int analogWrites;
static int pwmWrites;
static int stateWrites;
static int detCalls;

Std_ReturnType Rte_Read_SWC_IOCONTROL_PORT_DIGITAL_INPUT_R(void* data)
{
    Swc_DigitalIOValueType* value = (Swc_DigitalIOValueType*)data;
    memset(value, 0, sizeof(*value));
    return E_NOT_OK;
}

Std_ReturnType Rte_Read_SWC_IOCONTROL_PORT_ANALOG_INPUT_R(void* data)
{
    Swc_AnalogIOValueType* value = (Swc_AnalogIOValueType*)data;
    memset(value, 0, sizeof(*value));
    return E_NOT_OK;
}

Std_ReturnType Rte_Read_SWC_IOCONTROL_PORT_PWM_INPUT_R(void* data)
{
    Swc_PwmIOValueType* value = (Swc_PwmIOValueType*)data;
    memset(value, 0, sizeof(*value));
    return E_NOT_OK;
}

Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_DIGITAL_OUTPUT_P(const void* data)
{
    lastDigitalOut = *(const Swc_DigitalIOValueType*)data;
    if (mockWriteResult == E_OK) { digitalWrites++; }
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_ANALOG_OUTPUT_P(const void* data)
{
    lastAnalogOut = *(const Swc_AnalogIOValueType*)data;
    if (mockWriteResult == E_OK) { analogWrites++; }
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_PWM_OUTPUT_P(const void* data)
{
    lastPwmOut = *(const Swc_PwmIOValueType*)data;
    if (mockWriteResult == E_OK) { pwmWrites++; }
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_IO_STATE_P(const void* data)
{
    lastWrittenState = *(const Swc_IOStateType*)data;
    stateWrites++;
    return E_OK;
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
    Swc_IOControl_Deinit();
    Swc_IOControl_Init();
}

static void mock_reset(void)
{
    memset(&lastDigitalOut, 0, sizeof(lastDigitalOut));
    memset(&lastAnalogOut, 0, sizeof(lastAnalogOut));
    memset(&lastPwmOut, 0, sizeof(lastPwmOut));
    mockWriteResult = E_OK;
    mockTime = 500U;
    digitalWrites = 0;
    analogWrites = 0;
    pwmWrites = 0;
    stateWrites = 0;
    lastWrittenState = IO_STATE_INACTIVE;
    detCalls = 0;
}

/* ============================================================================
 * Tests
 * ============================================================================ */
static int test_uninitialized_paths(void)
{
    boolean boolValue = FALSE;
    uint16 value = 0U;
    uint16 duty = 0U;
    uint16 freq = 0U;
    uint16 channel;
    Swc_IOStateType state;
    Swc_IOStatisticsType stats;

    mock_reset();
    Swc_IOControl_10ms();
    Swc_IOControl_50ms();
    Swc_IOControl_MainFunction();
    TEST_ASSERT_EQ(stateWrites, 0);

    TEST_ASSERT_EQ(Swc_IOControl_ReadDigitalInput(0U, NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_IOControl_ReadDigitalInput(0U, &boolValue), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_IOControl_WriteDigitalOutput(0U, TRUE), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_IOControl_ReadAnalogInput(0U, NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_IOControl_ReadAnalogInput(0U, &value), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_IOControl_WriteAnalogOutput(0U, 1U), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_IOControl_ReadPwmInput(0U, NULL_PTR, &freq), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_IOControl_ReadPwmInput(0U, &duty, NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_IOControl_ReadPwmInput(0U, &duty, &freq), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_IOControl_WritePwmOutput(0U, 1U, 2U), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_IOControl_SetChannelState(0U, IO_STATE_ACTIVE), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_IOControl_GetChannelState(0U, NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_IOControl_GetChannelState(0U, &state), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_IOControl_GetStatistics(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_IOControl_GetStatistics(&stats), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_IOControl_ResetStatistics(), RTE_E_UNINIT);

    for (channel = 0U; channel < 4U; channel++) {
        TEST_ASSERT_EQ(Swc_IOControl_ReadDigitalInput(channel, &boolValue), RTE_E_UNINIT);
    }
    Swc_IOControl_Deinit(); /* no-op */
    return 0;
}

static int test_init_state_and_deinit(void)
{
    Swc_IOStateType state = IO_STATE_INACTIVE;

    mock_reset();
    Swc_IOControl_Init();
    TEST_ASSERT_EQ(detCalls, 1);
    Swc_IOControl_Init();
    TEST_ASSERT_EQ(detCalls, 1);

    TEST_ASSERT_EQ(Swc_IOControl_GetChannelState(0U, &state), RTE_E_OK);
    TEST_ASSERT_EQ(state, IO_STATE_ACTIVE);

    TEST_ASSERT_EQ(Swc_IOControl_SetChannelState(0U, IO_STATE_FAULT), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_GetChannelState(0U, &state), RTE_E_OK);
    TEST_ASSERT_EQ(state, IO_STATE_FAULT);
    TEST_ASSERT_EQ(Swc_IOControl_SetChannelState(0U, (Swc_IOStateType)99), RTE_E_INVALID);

    /* Non-active state gates cyclic processing */
    mock_reset();
    Swc_IOControl_10ms();
    Swc_IOControl_50ms();
    Swc_IOControl_MainFunction();
    TEST_ASSERT_EQ(stateWrites, 0);

    TEST_ASSERT_EQ(Swc_IOControl_SetChannelState(0U, IO_STATE_TEST), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_SetChannelState(0U, IO_STATE_ACTIVE), RTE_E_OK);

    Swc_IOControl_Deinit();
    TEST_ASSERT_EQ(Swc_IOControl_GetChannelState(0U, &state), RTE_E_UNINIT);

    Swc_IOControl_Init();
    TEST_ASSERT_EQ(Swc_IOControl_GetChannelState(0U, &state), RTE_E_OK);
    TEST_ASSERT_EQ(state, IO_STATE_ACTIVE);
    return 0;
}

static int test_digital_output(void)
{
    Swc_IOStatisticsType stats;

    mock_reset();
    fresh_init();

    mockTime = 777U;
    TEST_ASSERT_EQ(Swc_IOControl_WriteDigitalOutput(3U, TRUE), RTE_E_OK);
    TEST_ASSERT_EQ(digitalWrites, 1);
    TEST_ASSERT_EQ(lastDigitalOut.channelId, 3U);
    TEST_ASSERT(lastDigitalOut.value != 0U);
    TEST_ASSERT_EQ(lastDigitalOut.timestamp, 777U);
    TEST_ASSERT(lastDigitalOut.isValid != 0U);

    /* Same channel is updated, not duplicated */
    TEST_ASSERT_EQ(Swc_IOControl_WriteDigitalOutput(3U, FALSE), RTE_E_OK);
    TEST_ASSERT_EQ(lastDigitalOut.value, FALSE);

    /* RTE write failure */
    mockWriteResult = E_NOT_OK;
    TEST_ASSERT_EQ(Swc_IOControl_WriteDigitalOutput(3U, TRUE), RTE_E_NOT_OK);

    TEST_ASSERT_EQ(Swc_IOControl_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT_EQ(stats.digitalWrites, 2U);
    TEST_ASSERT_EQ(stats.errorCount, 1U);
    return 0;
}

static int test_digital_output_limit(void)
{
    uint16 channel;

    mock_reset();
    fresh_init();

    for (channel = 0U; channel < 16U; channel++) {
        TEST_ASSERT_EQ(Swc_IOControl_WriteDigitalOutput(channel, TRUE), RTE_E_OK);
    }
    TEST_ASSERT_EQ(Swc_IOControl_WriteDigitalOutput(16U, TRUE), RTE_E_LIMIT);
    /* Existing channels still writable */
    TEST_ASSERT_EQ(Swc_IOControl_WriteDigitalOutput(0U, FALSE), RTE_E_OK);
    return 0;
}

static int test_analog_output(void)
{
    Swc_IOStatisticsType stats;

    mock_reset();
    fresh_init();

    TEST_ASSERT_EQ(Swc_IOControl_WriteAnalogOutput(2U, 1234U), RTE_E_OK);
    TEST_ASSERT_EQ(analogWrites, 1);
    TEST_ASSERT_EQ(lastAnalogOut.channelId, 2U);
    TEST_ASSERT_EQ(lastAnalogOut.rawValue, 1234U);
    TEST_ASSERT_EQ(lastAnalogOut.physicalValue, 1234U);
    TEST_ASSERT(lastAnalogOut.isValid != 0U);

    mockWriteResult = E_NOT_OK;
    TEST_ASSERT_EQ(Swc_IOControl_WriteAnalogOutput(2U, 1U), RTE_E_NOT_OK);

    TEST_ASSERT_EQ(Swc_IOControl_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT_EQ(stats.analogWrites, 1U);
    TEST_ASSERT_EQ(stats.errorCount, 1U);

    /* Capacity: 4 analog outputs */
    mockWriteResult = E_OK;
    fresh_init();
    TEST_ASSERT_EQ(Swc_IOControl_WriteAnalogOutput(10U, 1U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_WriteAnalogOutput(11U, 1U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_WriteAnalogOutput(12U, 1U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_WriteAnalogOutput(13U, 1U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_WriteAnalogOutput(14U, 1U), RTE_E_LIMIT);
    return 0;
}

static int test_pwm_output(void)
{
    Swc_IOStatisticsType stats;

    mock_reset();
    fresh_init();

    TEST_ASSERT_EQ(Swc_IOControl_WritePwmOutput(1U, 50U, 1000U), RTE_E_OK);
    TEST_ASSERT_EQ(pwmWrites, 1);
    TEST_ASSERT_EQ(lastPwmOut.channelId, 1U);
    TEST_ASSERT_EQ(lastPwmOut.dutyCycle, 50U);
    TEST_ASSERT_EQ(lastPwmOut.frequency, 1000U);
    TEST_ASSERT(lastPwmOut.isValid != 0U);

    mockWriteResult = E_NOT_OK;
    TEST_ASSERT_EQ(Swc_IOControl_WritePwmOutput(1U, 10U, 100U), RTE_E_NOT_OK);
    TEST_ASSERT_EQ(Swc_IOControl_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT_EQ(stats.pwmWrites, 1U);
    TEST_ASSERT_EQ(stats.errorCount, 1U);

    /* Capacity: 4 PWM outputs */
    mockWriteResult = E_OK;
    fresh_init();
    TEST_ASSERT_EQ(Swc_IOControl_WritePwmOutput(20U, 1U, 1U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_WritePwmOutput(21U, 1U, 1U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_WritePwmOutput(22U, 1U, 1U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_WritePwmOutput(23U, 1U, 1U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_WritePwmOutput(24U, 1U, 1U), RTE_E_LIMIT);
    return 0;
}

static int test_write_gated_by_state(void)
{
    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_IOControl_SetChannelState(0U, IO_STATE_INACTIVE), RTE_E_OK);

    TEST_ASSERT_EQ(Swc_IOControl_WriteDigitalOutput(1U, TRUE), RTE_E_NOT_ACTIVE);
    TEST_ASSERT_EQ(Swc_IOControl_WriteAnalogOutput(1U, 1U), RTE_E_NOT_ACTIVE);
    TEST_ASSERT_EQ(Swc_IOControl_WritePwmOutput(1U, 1U, 1U), RTE_E_NOT_ACTIVE);
    return 0;
}

static int test_inputs_have_no_channel_source(void)
{
    boolean boolValue = TRUE;
    uint16 value = 0U;
    uint16 duty = 0U;
    uint16 freq = 0U;

    mock_reset();
    fresh_init();
    Swc_IOControl_10ms();
    Swc_IOControl_50ms();

    /* The component offers no API to create input channels: reads always fail */
    TEST_ASSERT_EQ(Swc_IOControl_ReadDigitalInput(0U, &boolValue), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_IOControl_ReadAnalogInput(0U, &value), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_IOControl_ReadPwmInput(0U, &duty, &freq), RTE_E_INVALID);
    TEST_ASSERT_EQ(boolValue, TRUE); /* untouched */
    return 0;
}

static int test_cyclic_runnables(void)
{
    int i;

    mock_reset();
    fresh_init();

    Swc_IOControl_50ms();
    TEST_ASSERT_EQ(stateWrites, 1);
    TEST_ASSERT_EQ(lastWrittenState, IO_STATE_ACTIVE);

    for (i = 0; i < 10; i++) {
        Swc_IOControl_MainFunction();
    }
    /* slowCounter increments 10 times -> two 50ms dispatches */
    TEST_ASSERT_EQ(stateWrites, 3);

    Swc_IOControl_10ms();
    TEST_ASSERT_EQ(stateWrites, 3);
    return 0;
}

static int test_statistics_reset(void)
{
    Swc_IOStatisticsType stats;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_IOControl_WriteDigitalOutput(1U, TRUE), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_WriteAnalogOutput(1U, 10U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_WritePwmOutput(1U, 10U, 100U), RTE_E_OK);

    TEST_ASSERT_EQ(Swc_IOControl_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT_EQ(stats.digitalWrites, 1U);
    TEST_ASSERT_EQ(stats.analogWrites, 1U);
    TEST_ASSERT_EQ(stats.pwmWrites, 1U);

    TEST_ASSERT_EQ(Swc_IOControl_ResetStatistics(), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_IOControl_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT_EQ(stats.digitalWrites, 0U);
    TEST_ASSERT_EQ(stats.analogWrites, 0U);
    TEST_ASSERT_EQ(stats.pwmWrites, 0U);
    TEST_ASSERT_EQ(stats.errorCount, 0U);
    return 0;
}

static int test_deinit_clears_channels(void)
{
    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_IOControl_WriteDigitalOutput(5U, TRUE), RTE_E_OK);

    Swc_IOControl_Deinit();
    Swc_IOControl_Init();
    /* After re-init the channel is re-created from scratch, still writable */
    TEST_ASSERT_EQ(Swc_IOControl_WriteDigitalOutput(5U, FALSE), RTE_E_OK);
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
    { "init_state_deinit", test_init_state_and_deinit },
    { "digital_output", test_digital_output },
    { "digital_output_limit", test_digital_output_limit },
    { "analog_output", test_analog_output },
    { "pwm_output", test_pwm_output },
    { "write_gated_by_state", test_write_gated_by_state },
    { "inputs_no_channel_source", test_inputs_have_no_channel_source },
    { "cyclic_runnables", test_cyclic_runnables },
    { "statistics_reset", test_statistics_reset },
    { "deinit_clears_channels", test_deinit_clears_channels }
};

int main(void)
{
    size_t i;
    size_t numTests = sizeof(testCases) / sizeof(testCases[0]);

    printf("=== IOControl unit tests ===\n");
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
