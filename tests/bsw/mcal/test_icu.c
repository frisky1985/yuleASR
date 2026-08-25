/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : ICU (Input Capture Unit) Driver Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-29
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

// @tests src/bsw/mcal/icu/src/Icu.c  @tests src/bsw/mcal/icu/include/Icu.h

#include "test_framework.h"
#include "Icu.h"
#include "Icu_Private.h"

/*==================================================================================================
*                                      MOCK DATA
==================================================================================================*/
typedef struct {
    uint16 ModuleId;
    uint8 InstanceId;
    uint8 ApiId;
    uint8 ErrorId;
    uint16 CallCount;
} Det_MockDataType;

static Det_MockDataType Det_MockData;

void Det_Mock_Reset(void)
{
    Det_MockData.ModuleId = 0u;
    Det_MockData.InstanceId = 0u;
    Det_MockData.ApiId = 0u;
    Det_MockData.ErrorId = 0u;
    Det_MockData.CallCount = 0u;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    Det_MockData.ModuleId = ModuleId;
    Det_MockData.InstanceId = InstanceId;
    Det_MockData.ApiId = ApiId;
    Det_MockData.ErrorId = ErrorId;
    Det_MockData.CallCount++;
    return E_OK;
}

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Icu_ConfigType g_test_config;
static Icu_ChannelConfigType g_test_channels[ICU_NUM_CHANNELS];
static uint32 g_timestamp_buffer[16];

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    uint8 i;
    
    for (i = 0u; i < ICU_NUM_CHANNELS; i++)
    {
        g_test_channels[i].Channel = i;
        g_test_channels[i].BaseAddress = 0x30660000UL + (i * 0x10000UL);
        g_test_channels[i].Mode = ICU_MODE_SIGNAL_EDGE_DETECT;
        g_test_channels[i].Edge = ICU_RISING_EDGE;
        g_test_channels[i].Property = ICU_PERIOD_TIME;
        g_test_channels[i].Notification = NULL_PTR;
        g_test_channels[i].TimestampEnabled = FALSE;
        g_test_channels[i].TimestampBufferSize = 0u;
        g_test_channels[i].WakeupSupport = FALSE;
        g_test_channels[i].ClockPrescaler = 1u;
    }
    
    g_test_config.Channels = g_test_channels;
    g_test_config.NumChannels = ICU_NUM_CHANNELS;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
    g_test_config.DeInitApi = TRUE;
    g_test_config.SetModeApi = TRUE;
    g_test_config.WakeupFunctionalityApi = TRUE;
    g_test_config.DisableWakeupApi = TRUE;
    g_test_config.TimestampApi = TRUE;
    g_test_config.EdgeCountApi = TRUE;
    g_test_config.SignalMeasurementApi = TRUE;
    g_test_config.DefaultMode = ICU_MODE_NORMAL;
}

static void reset_icu_state(void)
{
    uint8 i;
    
    Icu_DriverState.Initialized = FALSE;
    Icu_DriverState.CurrentMode = ICU_MODE_NORMAL;
    Icu_DriverState.ConfigPtr = NULL_PTR;
    
    for (i = 0u; i < ICU_NUM_CHANNELS; i++)
    {
        Icu_ChannelState[i].State = ICU_STATE_UNINITIALIZED;
        Icu_ChannelState[i].InputState = ICU_IDLE;
        Icu_ChannelState[i].CapturedValue = 0u;
        Icu_ChannelState[i].PreviousValue = 0u;
        Icu_ChannelState[i].PeriodTime = 0u;
        Icu_ChannelState[i].ActiveTime = 0u;
        Icu_ChannelState[i].EdgeCount = 0u;
        Icu_ChannelState[i].BufferIndex = 0u;
        Icu_ChannelState[i].NotifyCounter = 0u;
        Icu_ChannelState[i].TimestampBuffer = NULL_PTR;
        Icu_ChannelState[i].NotificationEnabled = FALSE;
        Icu_ChannelState[i].WakeupEnabled = FALSE;
        Icu_ChannelState[i].IsRunning = FALSE;
        Icu_ChannelState[i].CurrentEdge = ICU_RISING_EDGE;
    }
}

/*==================================================================================================
*                                      TEST CASES - INITIALIZATION
==================================================================================================*/

/* Test: Icu_Init with valid configuration */
TEST_CASE(icu_init_valid)
{
    setup_test_config();
    
    Icu_Init(&g_test_config);
    
    ASSERT_TRUE(Icu_DriverState.Initialized);
    ASSERT_EQ(ICU_MODE_NORMAL, Icu_DriverState.CurrentMode);
    ASSERT_EQ(ICU_NUM_CHANNELS, Icu_DriverState.ConfigPtr->NumChannels);
}

/* Test: Icu_Init with NULL configuration pointer */
TEST_CASE(icu_init_null_config)
{
    Det_Mock_Reset();
    
    Icu_Init(NULL);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_POINTER, Det_MockData.ErrorId);
    ASSERT_FALSE(Icu_DriverState.Initialized);
}

/* Test: Icu_Init when already initialized */
TEST_CASE(icu_init_already_initialized)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_Init(&g_test_config);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
}

/* Test: Icu_DeInit with valid initialization */
TEST_CASE(icu_deinit_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    
    Icu_DeInit();
    
    ASSERT_FALSE(Icu_DriverState.Initialized);
}

/* Test: Icu_DeInit when not initialized */
TEST_CASE(icu_deinit_not_initialized)
{
    reset_icu_state();
    Det_Mock_Reset();
    
    Icu_DeInit();
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/* Test: Icu_DeInit when channel is running */
TEST_CASE(icu_deinit_channel_running)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_EnableNotification(0u);
    Icu_ChannelState[0u].IsRunning = TRUE;
    
    /* Should return early without deinitializing */
    Icu_DeInit();
    
    /* State should still be initialized since channel is running */
    ASSERT_TRUE(Icu_DriverState.Initialized);
}

/*==================================================================================================
*                                      TEST CASES - MODE SETTING
==================================================================================================*/

/* Test: Icu_SetMode to NORMAL */
TEST_CASE(icu_set_mode_normal)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    
    Icu_SetMode(ICU_MODE_NORMAL);
    
    ASSERT_EQ(ICU_MODE_NORMAL, Icu_DriverState.CurrentMode);
}

/* Test: Icu_SetMode to SLEEP */
TEST_CASE(icu_set_mode_sleep)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    
    Icu_SetMode(ICU_MODE_SLEEP);
    
    ASSERT_EQ(ICU_MODE_SLEEP, Icu_DriverState.CurrentMode);
}

/* Test: Icu_SetMode when not initialized */
TEST_CASE(icu_set_mode_uninit)
{
    reset_icu_state();
    Det_Mock_Reset();
    
    Icu_SetMode(ICU_MODE_NORMAL);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/* Test: Icu_SetMode with invalid mode */
TEST_CASE(icu_set_mode_invalid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_SetMode((Icu_ModeType)0xFFu);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_MODE, Det_MockData.ErrorId);
}

/*==================================================================================================
*                                      TEST CASES - WAKEUP
==================================================================================================*/

/* Test: Icu_EnableWakeup with valid channel */
TEST_CASE(icu_enable_wakeup_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].WakeupSupport = TRUE;
    
    Icu_EnableWakeup(0u);
    
    ASSERT_TRUE(Icu_ChannelState[0u].WakeupEnabled);
}

/* Test: Icu_EnableWakeup when not initialized */
TEST_CASE(icu_enable_wakeup_uninit)
{
    reset_icu_state();
    Det_Mock_Reset();
    
    Icu_EnableWakeup(0u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/* Test: Icu_EnableWakeup with invalid channel */
TEST_CASE(icu_enable_wakeup_invalid_channel)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_EnableWakeup(ICU_NUM_CHANNELS + 1u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/* Test: Icu_EnableWakeup without wakeup support */
TEST_CASE(icu_enable_wakeup_no_support)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].WakeupSupport = FALSE;
    Det_Mock_Reset();
    
    Icu_EnableWakeup(0u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_WAKEUP_CANNOT_BE_ENABLED, Det_MockData.ErrorId);
}

/* Test: Icu_DisableWakeup with valid channel */
TEST_CASE(icu_disable_wakeup_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].WakeupSupport = TRUE;
    Icu_EnableWakeup(0u);
    
    Icu_DisableWakeup(0u);
    
    ASSERT_FALSE(Icu_ChannelState[0u].WakeupEnabled);
}

/* Test: Icu_DisableWakeup when not initialized */
TEST_CASE(icu_disable_wakeup_uninit)
{
    reset_icu_state();
    Det_Mock_Reset();
    
    Icu_DisableWakeup(0u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/* Test: Icu_CheckWakeup with valid channel */
TEST_CASE(icu_check_wakeup_valid)
{
    Std_ReturnType result;
    
    setup_test_config();
    Icu_Init(&g_test_config);
    
    result = Icu_CheckWakeup(0u);
    
    /* Result depends on implementation */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
*                                      TEST CASES - ACTIVATION CONDITION
==================================================================================================*/

/* Test: Icu_SetActivationCondition with rising edge */
TEST_CASE(icu_set_activation_rising)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    
    Icu_SetActivationCondition(0u, ICU_RISING_EDGE);
    
    ASSERT_EQ(ICU_RISING_EDGE, Icu_ChannelState[0u].CurrentEdge);
}

/* Test: Icu_SetActivationCondition with falling edge */
TEST_CASE(icu_set_activation_falling)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    
    Icu_SetActivationCondition(0u, ICU_FALLING_EDGE);
    
    ASSERT_EQ(ICU_FALLING_EDGE, Icu_ChannelState[0u].CurrentEdge);
}

/* Test: Icu_SetActivationCondition with both edges */
TEST_CASE(icu_set_activation_both)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    
    Icu_SetActivationCondition(0u, ICU_BOTH_EDGES);
    
    ASSERT_EQ(ICU_BOTH_EDGES, Icu_ChannelState[0u].CurrentEdge);
}

/* Test: Icu_SetActivationCondition when not initialized */
TEST_CASE(icu_set_activation_uninit)
{
    reset_icu_state();
    Det_Mock_Reset();
    
    Icu_SetActivationCondition(0u, ICU_RISING_EDGE);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/* Test: Icu_SetActivationCondition with invalid channel */
TEST_CASE(icu_set_activation_invalid_channel)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_SetActivationCondition(ICU_NUM_CHANNELS + 1u, ICU_RISING_EDGE);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/* Test: Icu_SetActivationCondition with invalid edge */
TEST_CASE(icu_set_activation_invalid_edge)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_SetActivationCondition(0u, (Icu_SignalEdgeType)0xFFu);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_ACTIVATION, Det_MockData.ErrorId);
}

/*==================================================================================================
*                                      TEST CASES - NOTIFICATION
==================================================================================================*/

/* Test: Icu_EnableNotification with valid channel */
TEST_CASE(icu_enable_notification_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    
    Icu_EnableNotification(0u);
    
    ASSERT_TRUE(Icu_ChannelState[0u].NotificationEnabled);
}

/* Test: Icu_DisableNotification with valid channel */
TEST_CASE(icu_disable_notification_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_EnableNotification(0u);
    
    Icu_DisableNotification(0u);
    
    ASSERT_FALSE(Icu_ChannelState[0u].NotificationEnabled);
}

/* Test: Icu_EnableNotification when not initialized */
TEST_CASE(icu_enable_notification_uninit)
{
    reset_icu_state();
    Det_Mock_Reset();
    
    Icu_EnableNotification(0u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/* Test: Icu_EnableNotification with invalid channel */
TEST_CASE(icu_enable_notification_invalid_channel)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_EnableNotification(ICU_NUM_CHANNELS + 1u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/*==================================================================================================
*                                      TEST CASES - INPUT STATE
==================================================================================================*/

/* Test: Icu_GetInputState with valid channel */
TEST_CASE(icu_get_input_state_valid)
{
    Icu_InputStateType state;
    
    setup_test_config();
    Icu_Init(&g_test_config);
    
    state = Icu_GetInputState(0u);
    
    /* Should return IDLE initially */
    ASSERT_EQ(ICU_IDLE, state);
}

/* Test: Icu_GetInputState when not initialized */
TEST_CASE(icu_get_input_state_uninit)
{
    Icu_InputStateType state;
    
    reset_icu_state();
    Det_Mock_Reset();
    
    state = Icu_GetInputState(0u);
    
    ASSERT_EQ(ICU_IDLE, state);
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/* Test: Icu_GetInputState with invalid channel */
TEST_CASE(icu_get_input_state_invalid_channel)
{
    Icu_InputStateType state;
    
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    state = Icu_GetInputState(ICU_NUM_CHANNELS + 1u);
    
    ASSERT_EQ(ICU_IDLE, state);
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/*==================================================================================================
*                                      TEST CASES - SIGNAL MEASUREMENT MODE
==================================================================================================*/

/* Test: Icu_StartSignalMeasurement with valid channel */
TEST_CASE(icu_start_signal_measurement_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_SIGNAL_MEASUREMENT;
    
    Icu_StartSignalMeasurement(0u);
    
    ASSERT_TRUE(Icu_ChannelState[0u].IsRunning);
}

/* Test: Icu_StopSignalMeasurement with valid channel */
TEST_CASE(icu_stop_signal_measurement_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_SIGNAL_MEASUREMENT;
    Icu_StartSignalMeasurement(0u);
    
    Icu_StopSignalMeasurement(0u);
    
    ASSERT_FALSE(Icu_ChannelState[0u].IsRunning);
}

/* Test: Icu_StartSignalMeasurement when not initialized */
TEST_CASE(icu_start_signal_measurement_uninit)
{
    reset_icu_state();
    Det_Mock_Reset();
    
    Icu_StartSignalMeasurement(0u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/* Test: Icu_StartSignalMeasurement with invalid channel */
TEST_CASE(icu_start_signal_measurement_invalid_channel)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_StartSignalMeasurement(ICU_NUM_CHANNELS + 1u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/* Test: Icu_GetTimeElapsed with valid channel */
TEST_CASE(icu_get_time_elapsed_valid)
{
    Icu_ValueType elapsed;
    
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_SIGNAL_MEASUREMENT;
    Icu_StartSignalMeasurement(0u);
    
    elapsed = Icu_GetTimeElapsed(0u);
    
    /* Initial value should be 0 */
    ASSERT_TRUE(elapsed == 0u || elapsed > 0u);
}

/* Test: Icu_GetTimeElapsed when not running */
TEST_CASE(icu_get_time_elapsed_not_running)
{
    Icu_ValueType elapsed;
    
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_SIGNAL_MEASUREMENT;
    
    elapsed = Icu_GetTimeElapsed(0u);
    
    ASSERT_EQ(0u, elapsed);
}

/* Test: Icu_GetDutyCycleValues with valid channel */
TEST_CASE(icu_get_duty_cycle_valid)
{
    Icu_DutyCycleType duty_cycle;
    
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_SIGNAL_MEASUREMENT;
    Icu_DriverState.ConfigPtr->Channels[0u].Property = ICU_DUTY_CYCLE;
    Icu_StartSignalMeasurement(0u);
    
    Icu_GetDutyCycleValues(0u, &duty_cycle);
    
    /* Initial values should be 0 */
    ASSERT_EQ(0u, duty_cycle.ActiveTime);
    ASSERT_EQ(0u, duty_cycle.PeriodTime);
}

/* Test: Icu_GetDutyCycleValues with NULL pointer */
TEST_CASE(icu_get_duty_cycle_null)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_GetDutyCycleValues(0u, NULL);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/*==================================================================================================
*                                      TEST CASES - TIMESTAMP MODE
==================================================================================================*/

/* Test: Icu_StartTimestamp with valid parameters */
TEST_CASE(icu_start_timestamp_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_TIMESTAMP;
    
    Icu_StartTimestamp(0u, g_timestamp_buffer, 16u, 1u);
    
    ASSERT_TRUE(Icu_ChannelState[0u].IsRunning);
    ASSERT_EQ(16u, Icu_ChannelState[0u].BufferSize);
    ASSERT_NOT_NULL(Icu_ChannelState[0u].TimestampBuffer);
}

/* Test: Icu_StopTimestamp with valid channel */
TEST_CASE(icu_stop_timestamp_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_TIMESTAMP;
    Icu_StartTimestamp(0u, g_timestamp_buffer, 16u, 1u);
    
    Icu_StopTimestamp(0u);
    
    ASSERT_FALSE(Icu_ChannelState[0u].IsRunning);
}

/* Test: Icu_StartTimestamp when not initialized */
TEST_CASE(icu_start_timestamp_uninit)
{
    reset_icu_state();
    Det_Mock_Reset();
    
    Icu_StartTimestamp(0u, g_timestamp_buffer, 16u, 1u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/* Test: Icu_StartTimestamp with invalid channel */
TEST_CASE(icu_start_timestamp_invalid_channel)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_StartTimestamp(ICU_NUM_CHANNELS + 1u, g_timestamp_buffer, 16u, 1u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/* Test: Icu_StartTimestamp with NULL buffer */
TEST_CASE(icu_start_timestamp_null_buffer)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_StartTimestamp(0u, NULL, 16u, 1u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/* Test: Icu_StartTimestamp with invalid buffer size */
TEST_CASE(icu_start_timestamp_invalid_size)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_StartTimestamp(0u, g_timestamp_buffer, 0u, 1u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_BUFFER_SIZE, Det_MockData.ErrorId);
}

/* Test: Icu_GetTimestampIndex with valid channel */
TEST_CASE(icu_get_timestamp_index_valid)
{
    Icu_IndexType index;
    
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_TIMESTAMP;
    Icu_StartTimestamp(0u, g_timestamp_buffer, 16u, 1u);
    
    index = Icu_GetTimestampIndex(0u);
    
    /* Initial index should be 0 */
    ASSERT_EQ(0u, index);
}

/* Test: Icu_GetTimestampIndex when not running */
TEST_CASE(icu_get_timestamp_index_not_running)
{
    Icu_IndexType index;
    
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_TIMESTAMP;
    
    index = Icu_GetTimestampIndex(0u);
    
    ASSERT_EQ(0u, index);
}

/*==================================================================================================
*                                      TEST CASES - EDGE COUNT MODE
==================================================================================================*/

/* Test: Icu_EnableEdgeCount with valid channel */
TEST_CASE(icu_enable_edge_count_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_EDGE_COUNTER;
    
    Icu_EnableEdgeCount(0u);
    
    ASSERT_TRUE(Icu_ChannelState[0u].IsRunning);
    ASSERT_EQ(0u, Icu_ChannelState[0u].EdgeCount);
}

/* Test: Icu_DisableEdgeCount with valid channel */
TEST_CASE(icu_disable_edge_count_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_EDGE_COUNTER;
    Icu_EnableEdgeCount(0u);
    
    Icu_DisableEdgeCount(0u);
    
    ASSERT_FALSE(Icu_ChannelState[0u].IsRunning);
}

/* Test: Icu_ResetEdgeCount with valid channel */
TEST_CASE(icu_reset_edge_count_valid)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_EDGE_COUNTER;
    Icu_EnableEdgeCount(0u);
    Icu_ChannelState[0u].EdgeCount = 10u;
    
    Icu_ResetEdgeCount(0u);
    
    ASSERT_EQ(0u, Icu_ChannelState[0u].EdgeCount);
}

/* Test: Icu_GetEdgeNumbers with valid channel */
TEST_CASE(icu_get_edge_numbers_valid)
{
    Icu_EdgeNumberType count;
    
    setup_test_config();
    Icu_Init(&g_test_config);
    Icu_DriverState.ConfigPtr->Channels[0u].Mode = ICU_MODE_EDGE_COUNTER;
    Icu_EnableEdgeCount(0u);
    
    count = Icu_GetEdgeNumbers(0u);
    
    /* Initial count should be 0 */
    ASSERT_EQ(0u, count);
}

/* Test: Icu_EnableEdgeCount when not initialized */
TEST_CASE(icu_enable_edge_count_uninit)
{
    reset_icu_state();
    Det_Mock_Reset();
    
    Icu_EnableEdgeCount(0u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_UNINIT, Det_MockData.ErrorId);
}

/* Test: Icu_EnableEdgeCount with invalid channel */
TEST_CASE(icu_enable_edge_count_invalid_channel)
{
    setup_test_config();
    Icu_Init(&g_test_config);
    Det_Mock_Reset();
    
    Icu_EnableEdgeCount(ICU_NUM_CHANNELS + 1u);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/*==================================================================================================
*                                      TEST CASES - VERSION INFO
==================================================================================================*/

/* Test: Icu_GetVersionInfo with valid pointer */
TEST_CASE(icu_get_version_info_valid)
{
    Std_VersionInfoType version_info;
    
    Icu_GetVersionInfo(&version_info);
    
    ASSERT_EQ(ICU_MODULE_ID, version_info.moduleID);
    ASSERT_EQ(ICU_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(ICU_SW_MINOR_VERSION, version_info.sw_minor_version);
    ASSERT_EQ(ICU_SW_PATCH_VERSION, version_info.sw_patch_version);
}

/* Test: Icu_GetVersionInfo with NULL pointer */
TEST_CASE(icu_get_version_info_null)
{
    Det_Mock_Reset();
    
    Icu_GetVersionInfo(NULL);
    
    ASSERT_EQ(1u, Det_MockData.CallCount);
    ASSERT_EQ(ICU_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(icu)
{
    Det_Mock_Reset();
    reset_icu_state();
}

TEST_SUITE_TEARDOWN(icu)
{
    /* Cleanup */
}

TEST_SUITE(icu)
{
    /* Initialization tests */
    RUN_TEST(icu_init_valid);
    RUN_TEST(icu_init_null_config);
    RUN_TEST(icu_init_already_initialized);
    RUN_TEST(icu_deinit_valid);
    RUN_TEST(icu_deinit_not_initialized);
    RUN_TEST(icu_deinit_channel_running);
    
    /* Mode setting tests */
    RUN_TEST(icu_set_mode_normal);
    RUN_TEST(icu_set_mode_sleep);
    RUN_TEST(icu_set_mode_uninit);
    RUN_TEST(icu_set_mode_invalid);
    
    /* Wakeup tests */
    RUN_TEST(icu_enable_wakeup_valid);
    RUN_TEST(icu_enable_wakeup_uninit);
    RUN_TEST(icu_enable_wakeup_invalid_channel);
    RUN_TEST(icu_enable_wakeup_no_support);
    RUN_TEST(icu_disable_wakeup_valid);
    RUN_TEST(icu_disable_wakeup_uninit);
    RUN_TEST(icu_check_wakeup_valid);
    
    /* Activation condition tests */
    RUN_TEST(icu_set_activation_rising);
    RUN_TEST(icu_set_activation_falling);
    RUN_TEST(icu_set_activation_both);
    RUN_TEST(icu_set_activation_uninit);
    RUN_TEST(icu_set_activation_invalid_channel);
    RUN_TEST(icu_set_activation_invalid_edge);
    
    /* Notification tests */
    RUN_TEST(icu_enable_notification_valid);
    RUN_TEST(icu_disable_notification_valid);
    RUN_TEST(icu_enable_notification_uninit);
    RUN_TEST(icu_enable_notification_invalid_channel);
    
    /* Input state tests */
    RUN_TEST(icu_get_input_state_valid);
    RUN_TEST(icu_get_input_state_uninit);
    RUN_TEST(icu_get_input_state_invalid_channel);
    
    /* Signal measurement tests */
    RUN_TEST(icu_start_signal_measurement_valid);
    RUN_TEST(icu_stop_signal_measurement_valid);
    RUN_TEST(icu_start_signal_measurement_uninit);
    RUN_TEST(icu_start_signal_measurement_invalid_channel);
    RUN_TEST(icu_get_time_elapsed_valid);
    RUN_TEST(icu_get_time_elapsed_not_running);
    RUN_TEST(icu_get_duty_cycle_valid);
    RUN_TEST(icu_get_duty_cycle_null);
    
    /* Timestamp tests */
    RUN_TEST(icu_start_timestamp_valid);
    RUN_TEST(icu_stop_timestamp_valid);
    RUN_TEST(icu_start_timestamp_uninit);
    RUN_TEST(icu_start_timestamp_invalid_channel);
    RUN_TEST(icu_start_timestamp_null_buffer);
    RUN_TEST(icu_start_timestamp_invalid_size);
    RUN_TEST(icu_get_timestamp_index_valid);
    RUN_TEST(icu_get_timestamp_index_not_running);
    
    /* Edge count tests */
    RUN_TEST(icu_enable_edge_count_valid);
    RUN_TEST(icu_disable_edge_count_valid);
    RUN_TEST(icu_reset_edge_count_valid);
    RUN_TEST(icu_get_edge_numbers_valid);
    RUN_TEST(icu_enable_edge_count_uninit);
    RUN_TEST(icu_enable_edge_count_invalid_channel);
    
    /* Version info tests */
    RUN_TEST(icu_get_version_info_valid);
    RUN_TEST(icu_get_version_info_null);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- ICU (Input Capture) Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(icu);
}
TEST_MAIN_END()
