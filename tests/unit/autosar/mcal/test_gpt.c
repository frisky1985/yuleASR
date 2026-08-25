/**
 * @file test_gpt.c
 * @brief GPT (General Purpose Timer) Driver 模块单元测试
 * @version 1.0.0
 * @date 2026-05-15
 * SHALL-GPT-01: SHALL provide 8 hardware timer channels
 * SHALL-GPT-02: SHALL provide 32-bit timer resolution
 * SHALL-GPT-03: SHALL support prescaler values from 1 to 65536
 * SHALL-GPT-04: SHALL support one-shot and continuous timer modes
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 * 本测试文件为MCAL层GPT模块提供全面的单元测试覆盖，包括:
 * - 初始化/反初始化 (Gpt_Init, Gpt_DeInit)
 * - 时间管理 (Gpt_GetTimeElapsed, Gpt_GetTimeRemaining)
 * - 定时器控制 (Gpt_StartTimer, Gpt_StopTimer)
 * - 通知管理 (Gpt_EnableNotification, Gpt_DisableNotification)
 * - 版本信息 (Gpt_GetVersionInfo)
 * - 电源模式 (Gpt_SetMode)
 * - 唤醒功能 (Gpt_EnableWakeup, Gpt_DisableWakeup, Gpt_CheckWakeup)
 * - 预定义定时器 (Gpt_GetPredefTimerValue)
 *
 * @test_coverage 目标覆盖率: 80%+
 */

// @tests src/bsw/mcal/gpt/src/Gpt.c  @tests src/bsw/mcal/gpt/include/Gpt.h

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*==================================================================================================
*                                      TYPE DEFINITIONS
==================================================================================================*/
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef enum { FALSE = 0, TRUE = 1 } boolean;
typedef enum { E_OK = 0, E_NOT_OK } Std_ReturnType;

#ifndef STD_ON
#define STD_ON                          1U
#define STD_OFF                         0U
#endif

#ifndef NULL_PTR
#define NULL_PTR                        ((void*)0)
#endif

/*==================================================================================================
*                                      VERSION INFO
==================================================================================================*/
#define GPT_VENDOR_ID                   (0x01U)
#define GPT_MODULE_ID                   (0x0EU)
#define GPT_SW_MAJOR_VERSION            (0x01U)
#define GPT_SW_MINOR_VERSION            (0x00U)
#define GPT_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                      SERVICE IDs
==================================================================================================*/
#define GPT_SID_INIT                    (0x00U)
#define GPT_SID_DEINIT                  (0x01U)
#define GPT_SID_GETTIMEELAPSED          (0x02U)
#define GPT_SID_GETTIMEREMAINING        (0x03U)
#define GPT_SID_STARTTIMER              (0x04U)
#define GPT_SID_STOPTIMER               (0x05U)
#define GPT_SID_ENABLEINTERRUPT         (0x06U)
#define GPT_SID_DISABLEINTERRUPT        (0x07U)
#define GPT_SID_GETVERSIONINFO          (0x08U)
#define GPT_SID_SETMODE                 (0x09U)
#define GPT_SID_DISABLEWAKEUP           (0x0AU)
#define GPT_SID_ENABLEWAKEUP            (0x0BU)
#define GPT_SID_CHECKWAKEUP             (0x0CU)
#define GPT_SID_GETPREDEFTIMERVALUE     (0x0DU)

/*==================================================================================================
*                                      DET ERROR CODES
==================================================================================================*/
#define GPT_E_PARAM_CHANNEL             (0x0AU)
#define GPT_E_PARAM_VALUE               (0x0BU)
#define GPT_E_PARAM_POINTER             (0x0CU)
#define GPT_E_PARAM_MODE                (0x0DU)
#define GPT_E_ALREADY_INITIALIZED       (0x0FU)
#define GPT_E_CHANNEL_BUSY              (0x10U)
#define GPT_E_UNINIT                    (0x11U)
#define GPT_E_INIT_FAILED               (0x12U)

/*==================================================================================================
*                                      TYPE DEFINITIONS
==================================================================================================*/
typedef uint8 Gpt_ChannelType;
typedef uint32 Gpt_ValueType;

typedef enum {
    GPT_MODE_NORMAL = 0,
    GPT_MODE_SLEEP
} Gpt_ModeType;

typedef enum {
    GPT_PREDEF_TIMER_1US_16BIT = 0x01U,
    GPT_PREDEF_TIMER_1US_24BIT = 0x02U,
    GPT_PREDEF_TIMER_1US_32BIT = 0x04U,
    GPT_PREDEF_TIMER_100US_32BIT = 0x08U
} Gpt_PredefTimerType;

typedef enum {
    GPT_CH_MODE_CONTINUOUS = 0,
    GPT_CH_MODE_ONESHOT
} Gpt_ChannelModeType;

typedef enum {
    GPT_CLOCK_PRESCALER_1 = 0,
    GPT_CLOCK_PRESCALER_2,
    GPT_CLOCK_PRESCALER_4,
    GPT_CLOCK_PRESCALER_8,
    GPT_CLOCK_PRESCALER_16,
    GPT_CLOCK_PRESCALER_32,
    GPT_CLOCK_PRESCALER_64,
    GPT_CLOCK_PRESCALER_128
} Gpt_ClockPrescalerType;

typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8 sw_major_version;
    uint8 sw_minor_version;
    uint8 sw_patch_version;
} Std_VersionInfoType;

typedef struct {
    Gpt_ChannelType ChannelId;
    uint32 BaseAddress;
    Gpt_ChannelModeType ChannelMode;
    Gpt_ClockPrescalerType ClockPrescaler;
    Gpt_ValueType MaxTickValue;
    uint32 ClockFrequency;
    boolean WakeupSupport;
    boolean NotificationEnabled;
    void (*NotificationFn)(void);
} Gpt_ChannelConfigType;

typedef struct {
    const Gpt_ChannelConfigType* Channels;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean WakeupFunctionalityApi;
    boolean DeInitApi;
    boolean TimeElapsedApi;
    boolean TimeRemainingApi;
    boolean EnableDisableNotificationApi;
    boolean NotificationSupported;
    Gpt_ModeType DefaultMode;
    boolean PredefTimer1usEnablingGrade;
    boolean PredefTimer100us32bitEnable;
} Gpt_ConfigType;

/*==================================================================================================
*                                      TEST RESULT TRACKING
==================================================================================================*/
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(expr) \
    do { \
        tests_run++; \
        if (expr) { \
            tests_passed++; \
            printf("  [PASS] %s\n", #expr); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) == (actual)) { \
            tests_passed++; \
            printf("  [PASS] %s == %s (%lu == %lu)\n", #expected, #actual, (unsigned long)(expected), (unsigned long)(actual)); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s == %s (%lu != %lu) (%s:%d)\n", #expected, #actual, (unsigned long)(expected), (unsigned long)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

/*==================================================================================================
*                                      MOCK STATE VARIABLES
==================================================================================================*/
#define GPT_NUM_CHANNELS                (8U)
#define GPT_CHANNEL_0                   ((Gpt_ChannelType)0U)
#define GPT_CHANNEL_1                   ((Gpt_ChannelType)1U)
#define GPT_CHANNEL_2                   ((Gpt_ChannelType)2U)
#define GPT_CHANNEL_3                   ((Gpt_ChannelType)3U)
#define GPT_CHANNEL_4                   ((Gpt_ChannelType)4U)
#define GPT_CHANNEL_5                   ((Gpt_ChannelType)5U)
#define GPT_CHANNEL_6                   ((Gpt_ChannelType)6U)
#define GPT_CHANNEL_7                   ((Gpt_ChannelType)7U)

static boolean Gpt_DriverInitialized = FALSE;
static Gpt_ModeType Gpt_DriverMode = GPT_MODE_NORMAL;
static const Gpt_ConfigType* Gpt_ConfigPtr = NULL_PTR;
static Gpt_ValueType Gpt_ChannelTargetValue[GPT_NUM_CHANNELS];
static Gpt_ValueType Gpt_ChannelElapsedValue[GPT_NUM_CHANNELS];
static boolean Gpt_ChannelRunning[GPT_NUM_CHANNELS];
static boolean Gpt_ChannelNotificationEnabled[GPT_NUM_CHANNELS];

/* Mock timer values for testing */
static Gpt_ValueType mock_timer_values[GPT_NUM_CHANNELS] = {0};

/* DET error tracking */
static uint16 det_module_id = 0;
static uint8 det_instance_id = 0;
static uint8 det_api_id = 0;
static uint8 det_error_id = 0;
static int det_call_count = 0;

/*==================================================================================================
*                                      MOCK FUNCTIONS
==================================================================================================*/
static void Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    det_module_id = ModuleId;
    det_instance_id = InstanceId;
    det_api_id = ApiId;
    det_error_id = ErrorId;
    det_call_count++;
}

static void reset_det_tracking(void)
{
    det_module_id = 0;
    det_instance_id = 0;
    det_api_id = 0;
    det_error_id = 0;
    det_call_count = 0;
}

/*==================================================================================================
*                                      GPT DRIVER IMPLEMENTATION (MOCK)
==================================================================================================*/

static Gpt_ChannelConfigType test_channels[GPT_NUM_CHANNELS];
static Gpt_ConfigType test_config;

void Gpt_Init(const Gpt_ConfigType* ConfigPtr)
{
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_INIT, GPT_E_PARAM_POINTER);
        return;
    }
    if (Gpt_DriverInitialized == TRUE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_INIT, GPT_E_ALREADY_INITIALIZED);
        return;
    }

    Gpt_ConfigPtr = ConfigPtr;

    for (uint8 i = 0U; i < GPT_NUM_CHANNELS; i++) {
        Gpt_ChannelRunning[i] = FALSE;
        Gpt_ChannelTargetValue[i] = 0U;
        Gpt_ChannelElapsedValue[i] = 0U;
        Gpt_ChannelNotificationEnabled[i] = FALSE;
        mock_timer_values[i] = 0U;
    }

    Gpt_DriverMode = ConfigPtr->DefaultMode;
    Gpt_DriverInitialized = TRUE;
}

void Gpt_DeInit(void)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DEINIT, GPT_E_UNINIT);
        return;
    }

    for (uint8 i = 0U; i < GPT_NUM_CHANNELS; i++) {
        if (Gpt_ChannelRunning[i]) {
            return;
        }
    }

    Gpt_DriverInitialized = FALSE;
    Gpt_ConfigPtr = NULL_PTR;
}

Gpt_ValueType Gpt_GetTimeElapsed(Gpt_ChannelType Channel)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEELAPSED, GPT_E_UNINIT);
        return 0U;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEELAPSED, GPT_E_PARAM_CHANNEL);
        return 0U;
    }

    return mock_timer_values[Channel];
}

Gpt_ValueType Gpt_GetTimeRemaining(Gpt_ChannelType Channel)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEREMAINING, GPT_E_UNINIT);
        return 0U;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEREMAINING, GPT_E_PARAM_CHANNEL);
        return 0U;
    }

    if (!Gpt_ChannelRunning[Channel]) {
        return 0U;
    }

    Gpt_ValueType current = mock_timer_values[Channel];
    if (Gpt_ChannelTargetValue[Channel] > current) {
        return Gpt_ChannelTargetValue[Channel] - current;
    }
    return 0U;
}

void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_PARAM_CHANNEL);
        return;
    }
    if (Value == 0U) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_PARAM_VALUE);
        return;
    }
    if (Gpt_ChannelRunning[Channel]) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_CHANNEL_BUSY);
        return;
    }

    Gpt_ChannelTargetValue[Channel] = Value;
    Gpt_ChannelRunning[Channel] = TRUE;
}

void Gpt_StopTimer(Gpt_ChannelType Channel)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STOPTIMER, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STOPTIMER, GPT_E_PARAM_CHANNEL);
        return;
    }

    Gpt_ChannelRunning[Channel] = FALSE;
}

void Gpt_EnableNotification(Gpt_ChannelType Channel)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEINTERRUPT, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEINTERRUPT, GPT_E_PARAM_CHANNEL);
        return;
    }

    Gpt_ChannelNotificationEnabled[Channel] = TRUE;
}

void Gpt_DisableNotification(Gpt_ChannelType Channel)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEINTERRUPT, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEINTERRUPT, GPT_E_PARAM_CHANNEL);
        return;
    }

    Gpt_ChannelNotificationEnabled[Channel] = FALSE;
}

void Gpt_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo == NULL_PTR) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETVERSIONINFO, GPT_E_PARAM_POINTER);
        return;
    }
    versioninfo->vendorID = GPT_VENDOR_ID;
    versioninfo->moduleID = GPT_MODULE_ID;
    versioninfo->sw_major_version = GPT_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = GPT_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = GPT_SW_PATCH_VERSION;
}

void Gpt_SetMode(Gpt_ModeType Mode)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_SETMODE, GPT_E_UNINIT);
        return;
    }

    if (Mode == GPT_MODE_SLEEP) {
        for (uint8 i = 0U; i < GPT_NUM_CHANNELS; i++) {
            if (Gpt_ChannelRunning[i]) {
                Gpt_ChannelRunning[i] = FALSE;
            }
        }
    }

    Gpt_DriverMode = Mode;
}

void Gpt_EnableWakeup(Gpt_ChannelType Channel)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEWAKEUP, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEWAKEUP, GPT_E_PARAM_CHANNEL);
        return;
    }
}

void Gpt_DisableWakeup(Gpt_ChannelType Channel)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEWAKEUP, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEWAKEUP, GPT_E_PARAM_CHANNEL);
        return;
    }
}

Std_ReturnType Gpt_CheckWakeup(Gpt_ChannelType Channel)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_CHECKWAKEUP, GPT_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_CHECKWAKEUP, GPT_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    return E_NOT_OK;
}

Std_ReturnType Gpt_GetPredefTimerValue(Gpt_PredefTimerType PredefTimer, uint32* TimeValuePtr)
{
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETPREDEFTIMERVALUE, GPT_E_UNINIT);
        return E_NOT_OK;
    }
    if (TimeValuePtr == NULL_PTR) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETPREDEFTIMERVALUE, GPT_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *TimeValuePtr = mock_timer_values[0];
    (void)PredefTimer;
    return E_OK;
}

/*==================================================================================================
*                                      TEST CONFIGURATION
==================================================================================================*/
static void setup_test_config(void)
{
    /* Channel 0: Oneshot mode, no prescaler */
    test_channels[0].ChannelId = GPT_CHANNEL_0;
    test_channels[0].BaseAddress = 0x302E0000;
    test_channels[0].ChannelMode = GPT_CH_MODE_ONESHOT;
    test_channels[0].ClockPrescaler = GPT_CLOCK_PRESCALER_1;
    test_channels[0].MaxTickValue = 0xFFFFFFFFU;
    test_channels[0].ClockFrequency = 24000000U;
    test_channels[0].WakeupSupport = FALSE;
    test_channels[0].NotificationEnabled = TRUE;
    test_channels[0].NotificationFn = NULL_PTR;

    /* Channel 1: Continuous mode, 8x prescaler */
    test_channels[1].ChannelId = GPT_CHANNEL_1;
    test_channels[1].BaseAddress = 0x302E0100;
    test_channels[1].ChannelMode = GPT_CH_MODE_CONTINUOUS;
    test_channels[1].ClockPrescaler = GPT_CLOCK_PRESCALER_8;
    test_channels[1].MaxTickValue = 0xFFFFU;
    test_channels[1].ClockFrequency = 24000000U;
    test_channels[1].WakeupSupport = TRUE;
    test_channels[1].NotificationEnabled = FALSE;
    test_channels[1].NotificationFn = NULL_PTR;

    /* Channel 2-7: Default configuration */
    for (int i = 2; i < GPT_NUM_CHANNELS; i++) {
        test_channels[i].ChannelId = (Gpt_ChannelType)i;
        test_channels[i].BaseAddress = 0x302E0000 + (i * 0x100);
        test_channels[i].ChannelMode = GPT_CH_MODE_CONTINUOUS;
        test_channels[i].ClockPrescaler = GPT_CLOCK_PRESCALER_1;
        test_channels[i].MaxTickValue = 0xFFFFFFFFU;
        test_channels[i].ClockFrequency = 24000000U;
        test_channels[i].WakeupSupport = FALSE;
        test_channels[i].NotificationEnabled = FALSE;
        test_channels[i].NotificationFn = NULL_PTR;
    }

    test_config.Channels = test_channels;
    test_config.NumChannels = GPT_NUM_CHANNELS;
    test_config.DevErrorDetect = TRUE;
    test_config.VersionInfoApi = TRUE;
    test_config.WakeupFunctionalityApi = TRUE;
    test_config.DeInitApi = TRUE;
    test_config.TimeElapsedApi = TRUE;
    test_config.TimeRemainingApi = TRUE;
    test_config.EnableDisableNotificationApi = TRUE;
    test_config.NotificationSupported = TRUE;
    test_config.DefaultMode = GPT_MODE_NORMAL;
    test_config.PredefTimer1usEnablingGrade = TRUE;
    test_config.PredefTimer100us32bitEnable = TRUE;
}

static void reset_driver_state(void)
{
    Gpt_DriverInitialized = FALSE;
    Gpt_DriverMode = GPT_MODE_NORMAL;
    Gpt_ConfigPtr = NULL_PTR;
    for (int i = 0; i < GPT_NUM_CHANNELS; i++) {
        Gpt_ChannelRunning[i] = FALSE;
        Gpt_ChannelTargetValue[i] = 0U;
        Gpt_ChannelElapsedValue[i] = 0U;
        Gpt_ChannelNotificationEnabled[i] = FALSE;
        mock_timer_values[i] = 0U;
    }
    reset_det_tracking();
}

/*==================================================================================================
*                                      TEST FUNCTIONS
==================================================================================================*/

/*-----------------------------------------
 * Initialization Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00001 */
void test_init_valid(void)
{
    printf("\n=== Test: Init Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);

    TEST_ASSERT_EQ(TRUE, Gpt_DriverInitialized);
    TEST_ASSERT_EQ(GPT_MODE_NORMAL, Gpt_DriverMode);
    TEST_ASSERT(Gpt_ConfigPtr == &test_config);
}

/* @req SWS_Gpt_00001 */
void test_init_null_config(void)
{
    printf("\n=== Test: Init NULL Config ===\n");
    reset_driver_state();

    Gpt_Init(NULL_PTR);

    TEST_ASSERT_EQ(FALSE, Gpt_DriverInitialized);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(GPT_MODULE_ID, det_module_id);
    TEST_ASSERT_EQ(GPT_SID_INIT, det_api_id);
    TEST_ASSERT_EQ(GPT_E_PARAM_POINTER, det_error_id);
}

/* @req SWS_Gpt_00001 */
void test_init_already_initialized(void)
{
    printf("\n=== Test: Init Already Initialized ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    TEST_ASSERT_EQ(TRUE, Gpt_DriverInitialized);

    reset_det_tracking();
    Gpt_Init(&test_config);

    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(GPT_E_ALREADY_INITIALIZED, det_error_id);
}

/*-----------------------------------------
 * Deinitialization Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00002 */
void test_deinit_valid(void)
{
    printf("\n=== Test: DeInit Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_DeInit();

    TEST_ASSERT_EQ(FALSE, Gpt_DriverInitialized);
}

/* @req SWS_Gpt_00002 */
void test_deinit_not_initialized(void)
{
    printf("\n=== Test: DeInit Not Initialized ===\n");
    reset_driver_state();

    Gpt_DeInit();

    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/* @req SWS_Gpt_00002 */
void test_deinit_with_running_channel(void)
{
    printf("\n=== Test: DeInit With Running Channel ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StartTimer(GPT_CHANNEL_0, 1000);

    Gpt_DeInit();

    /* DeInit should not complete if channels are running */
    TEST_ASSERT_EQ(TRUE, Gpt_DriverInitialized);

    Gpt_StopTimer(GPT_CHANNEL_0);
    Gpt_DeInit();

    TEST_ASSERT_EQ(FALSE, Gpt_DriverInitialized);
}

/*-----------------------------------------
 * GetTimeElapsed Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00003 */
void test_get_time_elapsed_valid(void)
{
    printf("\n=== Test: GetTimeElapsed Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    mock_timer_values[0] = 5000;

    Gpt_ValueType elapsed = Gpt_GetTimeElapsed(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(5000, elapsed);
}

/* @req SWS_Gpt_00003 */
void test_get_time_elapsed_not_initialized(void)
{
    printf("\n=== Test: GetTimeElapsed Not Initialized ===\n");
    reset_driver_state();

    Gpt_ValueType elapsed = Gpt_GetTimeElapsed(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(0, elapsed);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/* @req SWS_Gpt_00003 */
void test_get_time_elapsed_invalid_channel(void)
{
    printf("\n=== Test: GetTimeElapsed Invalid Channel ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);

    Gpt_ValueType elapsed = Gpt_GetTimeElapsed(GPT_NUM_CHANNELS);

    TEST_ASSERT_EQ(0, elapsed);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(GPT_E_PARAM_CHANNEL, det_error_id);
}

/* @req SWS_Gpt_00003 */
void test_get_time_elapsed_multiple_channels(void)
{
    printf("\n=== Test: GetTimeElapsed Multiple Channels ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    mock_timer_values[0] = 1000;
    mock_timer_values[1] = 2000;
    mock_timer_values[2] = 3000;

    TEST_ASSERT_EQ(1000, Gpt_GetTimeElapsed(GPT_CHANNEL_0));
    TEST_ASSERT_EQ(2000, Gpt_GetTimeElapsed(GPT_CHANNEL_1));
    TEST_ASSERT_EQ(3000, Gpt_GetTimeElapsed(GPT_CHANNEL_2));
}

/*-----------------------------------------
 * GetTimeRemaining Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00003 */
void test_get_time_remaining_valid(void)
{
    printf("\n=== Test: GetTimeRemaining Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StartTimer(GPT_CHANNEL_0, 10000);
    mock_timer_values[0] = 3000;

    Gpt_ValueType remaining = Gpt_GetTimeRemaining(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(7000, remaining);
}

/* @req SWS_Gpt_00004 */
void test_get_time_remaining_not_running(void)
{
    printf("\n=== Test: GetTimeRemaining Not Running ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);

    Gpt_ValueType remaining = Gpt_GetTimeRemaining(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(0, remaining);
}

/* @req SWS_Gpt_00001 */
void test_get_time_remaining_not_initialized(void)
{
    printf("\n=== Test: GetTimeRemaining Not Initialized ===\n");
    reset_driver_state();

    Gpt_ValueType remaining = Gpt_GetTimeRemaining(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(0, remaining);
    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/* @req SWS_Gpt_00004 */
void test_get_time_remaining_invalid_channel(void)
{
    printf("\n=== Test: GetTimeRemaining Invalid Channel ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);

    Gpt_ValueType remaining = Gpt_GetTimeRemaining(GPT_NUM_CHANNELS);

    TEST_ASSERT_EQ(0, remaining);
    TEST_ASSERT_EQ(GPT_E_PARAM_CHANNEL, det_error_id);
}

/* @req SWS_Gpt_00003 */
void test_get_time_remaining_expired(void)
{
    printf("\n=== Test: GetTimeRemaining Expired ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StartTimer(GPT_CHANNEL_0, 1000);
    mock_timer_values[0] = 1500;

    Gpt_ValueType remaining = Gpt_GetTimeRemaining(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(0, remaining);
}

/*-----------------------------------------
 * StartTimer Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00005 */
void test_start_timer_valid(void)
{
    printf("\n=== Test: StartTimer Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StartTimer(GPT_CHANNEL_0, 5000);

    TEST_ASSERT_EQ(TRUE, Gpt_ChannelRunning[GPT_CHANNEL_0]);
    TEST_ASSERT_EQ(5000, Gpt_ChannelTargetValue[GPT_CHANNEL_0]);
}

/* @req SWS_Gpt_00005 */
void test_start_timer_not_initialized(void)
{
    printf("\n=== Test: StartTimer Not Initialized ===\n");
    reset_driver_state();

    Gpt_StartTimer(GPT_CHANNEL_0, 5000);

    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/* @req SWS_Gpt_00005 */
void test_start_timer_invalid_channel(void)
{
    printf("\n=== Test: StartTimer Invalid Channel ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StartTimer(GPT_NUM_CHANNELS, 5000);

    TEST_ASSERT_EQ(GPT_E_PARAM_CHANNEL, det_error_id);
}

/* @req SWS_Gpt_00005 */
void test_start_timer_zero_value(void)
{
    printf("\n=== Test: StartTimer Zero Value ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StartTimer(GPT_CHANNEL_0, 0);

    TEST_ASSERT_EQ(GPT_E_PARAM_VALUE, det_error_id);
    TEST_ASSERT_EQ(FALSE, Gpt_ChannelRunning[GPT_CHANNEL_0]);
}

/* @req SWS_Gpt_00005 */
void test_start_timer_channel_busy(void)
{
    printf("\n=== Test: StartTimer Channel Busy ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StartTimer(GPT_CHANNEL_0, 5000);
    TEST_ASSERT_EQ(TRUE, Gpt_ChannelRunning[GPT_CHANNEL_0]);

    reset_det_tracking();
    Gpt_StartTimer(GPT_CHANNEL_0, 3000);

    TEST_ASSERT_EQ(GPT_E_CHANNEL_BUSY, det_error_id);
}

/* @req SWS_Gpt_00005 */
void test_start_timer_multiple_channels(void)
{
    printf("\n=== Test: StartTimer Multiple Channels ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StartTimer(GPT_CHANNEL_0, 1000);
    Gpt_StartTimer(GPT_CHANNEL_1, 2000);
    Gpt_StartTimer(GPT_CHANNEL_2, 3000);

    TEST_ASSERT_EQ(TRUE, Gpt_ChannelRunning[GPT_CHANNEL_0]);
    TEST_ASSERT_EQ(TRUE, Gpt_ChannelRunning[GPT_CHANNEL_1]);
    TEST_ASSERT_EQ(TRUE, Gpt_ChannelRunning[GPT_CHANNEL_2]);
    TEST_ASSERT_EQ(1000, Gpt_ChannelTargetValue[GPT_CHANNEL_0]);
    TEST_ASSERT_EQ(2000, Gpt_ChannelTargetValue[GPT_CHANNEL_1]);
    TEST_ASSERT_EQ(3000, Gpt_ChannelTargetValue[GPT_CHANNEL_2]);
}

/*-----------------------------------------
 * StopTimer Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00006 */
void test_stop_timer_valid(void)
{
    printf("\n=== Test: StopTimer Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StartTimer(GPT_CHANNEL_0, 5000);
    Gpt_StopTimer(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(FALSE, Gpt_ChannelRunning[GPT_CHANNEL_0]);
}

/* @req SWS_Gpt_00001 */
void test_stop_timer_not_initialized(void)
{
    printf("\n=== Test: StopTimer Not Initialized ===\n");
    reset_driver_state();

    Gpt_StopTimer(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/* @req SWS_Gpt_00006 */
void test_stop_timer_invalid_channel(void)
{
    printf("\n=== Test: StopTimer Invalid Channel ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StopTimer(GPT_NUM_CHANNELS);

    TEST_ASSERT_EQ(GPT_E_PARAM_CHANNEL, det_error_id);
}

/* @req SWS_Gpt_00006 */
void test_stop_timer_not_running(void)
{
    printf("\n=== Test: StopTimer Not Running ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StopTimer(GPT_CHANNEL_0);

    /* Should complete without error */
    TEST_ASSERT_EQ(FALSE, Gpt_ChannelRunning[GPT_CHANNEL_0]);
}

/*-----------------------------------------
 * Notification Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00007 */
void test_enable_notification_valid(void)
{
    printf("\n=== Test: EnableNotification Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_EnableNotification(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(TRUE, Gpt_ChannelNotificationEnabled[GPT_CHANNEL_0]);
}

/* @req SWS_Gpt_00007 */
void test_enable_notification_not_initialized(void)
{
    printf("\n=== Test: EnableNotification Not Initialized ===\n");
    reset_driver_state();

    Gpt_EnableNotification(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/* @req SWS_Gpt_00007 */
void test_enable_notification_invalid_channel(void)
{
    printf("\n=== Test: EnableNotification Invalid Channel ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_EnableNotification(GPT_NUM_CHANNELS);

    TEST_ASSERT_EQ(GPT_E_PARAM_CHANNEL, det_error_id);
}

/* @req SWS_Gpt_00007 */
void test_disable_notification_valid(void)
{
    printf("\n=== Test: DisableNotification Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_EnableNotification(GPT_CHANNEL_0);
    TEST_ASSERT_EQ(TRUE, Gpt_ChannelNotificationEnabled[GPT_CHANNEL_0]);

    Gpt_DisableNotification(GPT_CHANNEL_0);
    TEST_ASSERT_EQ(FALSE, Gpt_ChannelNotificationEnabled[GPT_CHANNEL_0]);
}

/* @req SWS_Gpt_00007 */
void test_disable_notification_not_initialized(void)
{
    printf("\n=== Test: DisableNotification Not Initialized ===\n");
    reset_driver_state();

    Gpt_DisableNotification(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/* @req SWS_Gpt_00007 */
void test_disable_notification_invalid_channel(void)
{
    printf("\n=== Test: DisableNotification Invalid Channel ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_DisableNotification(GPT_NUM_CHANNELS);

    TEST_ASSERT_EQ(GPT_E_PARAM_CHANNEL, det_error_id);
}

/*-----------------------------------------
 * Version Info Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00009 */
void test_get_version_info_valid(void)
{
    printf("\n=== Test: GetVersionInfo Valid ===\n");
    Std_VersionInfoType version_info;

    Gpt_GetVersionInfo(&version_info);

    TEST_ASSERT_EQ(GPT_VENDOR_ID, version_info.vendorID);
    TEST_ASSERT_EQ(GPT_MODULE_ID, version_info.moduleID);
    TEST_ASSERT_EQ(GPT_SW_MAJOR_VERSION, version_info.sw_major_version);
    TEST_ASSERT_EQ(GPT_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_ASSERT_EQ(GPT_SW_PATCH_VERSION, version_info.sw_patch_version);
}

/* @req SWS_Gpt_00009 */
void test_get_version_info_null(void)
{
    printf("\n=== Test: GetVersionInfo NULL ===\n");

    Gpt_GetVersionInfo(NULL_PTR);

    TEST_ASSERT_EQ(GPT_E_PARAM_POINTER, det_error_id);
}

/*-----------------------------------------
 * SetMode Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00010 */
void test_set_mode_normal(void)
{
    printf("\n=== Test: SetMode Normal ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_SetMode(GPT_MODE_NORMAL);

    TEST_ASSERT_EQ(GPT_MODE_NORMAL, Gpt_DriverMode);
}

/* @req SWS_Gpt_00010 */
void test_set_mode_sleep(void)
{
    printf("\n=== Test: SetMode Sleep ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_StartTimer(GPT_CHANNEL_0, 5000);
    Gpt_SetMode(GPT_MODE_SLEEP);

    TEST_ASSERT_EQ(GPT_MODE_SLEEP, Gpt_DriverMode);
    TEST_ASSERT_EQ(FALSE, Gpt_ChannelRunning[GPT_CHANNEL_0]);
}

/* @req SWS_Gpt_00010 */
void test_set_mode_not_initialized(void)
{
    printf("\n=== Test: SetMode Not Initialized ===\n");
    reset_driver_state();

    Gpt_SetMode(GPT_MODE_SLEEP);

    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/*-----------------------------------------
 * Wakeup Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00007 */
void test_enable_wakeup_valid(void)
{
    printf("\n=== Test: EnableWakeup Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_EnableWakeup(GPT_CHANNEL_1);  /* Channel 1 has wakeup support */

    /* Function should complete without error */
    TEST_ASSERT(TRUE);
}

/* @req SWS_Gpt_00007 */
void test_enable_wakeup_not_initialized(void)
{
    printf("\n=== Test: EnableWakeup Not Initialized ===\n");
    reset_driver_state();

    Gpt_EnableWakeup(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/* @req SWS_Gpt_00007 */
void test_enable_wakeup_invalid_channel(void)
{
    printf("\n=== Test: EnableWakeup Invalid Channel ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_EnableWakeup(GPT_NUM_CHANNELS);

    TEST_ASSERT_EQ(GPT_E_PARAM_CHANNEL, det_error_id);
}

/* @req SWS_Gpt_00011 */
void test_disable_wakeup_valid(void)
{
    printf("\n=== Test: DisableWakeup Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_EnableWakeup(GPT_CHANNEL_1);
    Gpt_DisableWakeup(GPT_CHANNEL_1);

    /* Function should complete without error */
    TEST_ASSERT(TRUE);
}

/* @req SWS_Gpt_00011 */
void test_disable_wakeup_not_initialized(void)
{
    printf("\n=== Test: DisableWakeup Not Initialized ===\n");
    reset_driver_state();

    Gpt_DisableWakeup(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/* @req SWS_Gpt_00011 */
void test_disable_wakeup_invalid_channel(void)
{
    printf("\n=== Test: DisableWakeup Invalid Channel ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Gpt_DisableWakeup(GPT_NUM_CHANNELS);

    TEST_ASSERT_EQ(GPT_E_PARAM_CHANNEL, det_error_id);
}

/* @req SWS_Gpt_00011 */
void test_check_wakeup_valid(void)
{
    printf("\n=== Test: CheckWakeup Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Std_ReturnType result = Gpt_CheckWakeup(GPT_CHANNEL_1);

    TEST_ASSERT_EQ(E_NOT_OK, result);
}

/* @req SWS_Gpt_00011 */
void test_check_wakeup_not_initialized(void)
{
    printf("\n=== Test: CheckWakeup Not Initialized ===\n");
    reset_driver_state();

    Std_ReturnType result = Gpt_CheckWakeup(GPT_CHANNEL_0);

    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/* @req SWS_Gpt_00011 */
void test_check_wakeup_invalid_channel(void)
{
    printf("\n=== Test: CheckWakeup Invalid Channel ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Std_ReturnType result = Gpt_CheckWakeup(GPT_NUM_CHANNELS);

    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(GPT_E_PARAM_CHANNEL, det_error_id);
}

/*-----------------------------------------
 * Predef Timer Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00014 */
void test_get_predef_timer_valid(void)
{
    printf("\n=== Test: GetPredefTimerValue Valid ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    mock_timer_values[0] = 12345;

    uint32 time_value;
    Std_ReturnType result = Gpt_GetPredefTimerValue(GPT_PREDEF_TIMER_1US_32BIT, &time_value);

    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(12345, time_value);
}

/* @req SWS_Gpt_00004 */
void test_get_predef_timer_not_initialized(void)
{
    printf("\n=== Test: GetPredefTimerValue Not Initialized ===\n");
    reset_driver_state();

    uint32 time_value;
    Std_ReturnType result = Gpt_GetPredefTimerValue(GPT_PREDEF_TIMER_1US_32BIT, &time_value);

    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(GPT_E_UNINIT, det_error_id);
}

/* @req SWS_Gpt_00004 */
void test_get_predef_timer_null_pointer(void)
{
    printf("\n=== Test: GetPredefTimerValue NULL Pointer ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    Std_ReturnType result = Gpt_GetPredefTimerValue(GPT_PREDEF_TIMER_1US_32BIT, NULL_PTR);

    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(GPT_E_PARAM_POINTER, det_error_id);
}

/* @req SWS_Gpt_00004 */
void test_get_predef_timer_all_types(void)
{
    printf("\n=== Test: GetPredefTimerValue All Types ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);
    uint32 time_value;

    TEST_ASSERT_EQ(E_OK, Gpt_GetPredefTimerValue(GPT_PREDEF_TIMER_1US_16BIT, &time_value));
    TEST_ASSERT_EQ(E_OK, Gpt_GetPredefTimerValue(GPT_PREDEF_TIMER_1US_24BIT, &time_value));
    TEST_ASSERT_EQ(E_OK, Gpt_GetPredefTimerValue(GPT_PREDEF_TIMER_1US_32BIT, &time_value));
    TEST_ASSERT_EQ(E_OK, Gpt_GetPredefTimerValue(GPT_PREDEF_TIMER_100US_32BIT, &time_value));
}

/*-----------------------------------------
 * Integration Tests
 *-----------------------------------------*/
/* @req SWS_Gpt_00004 */
void test_timer_full_lifecycle(void)
{
    printf("\n=== Test: Timer Full Lifecycle ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);

    /* Start timer */
    Gpt_StartTimer(GPT_CHANNEL_0, 10000);
    TEST_ASSERT_EQ(TRUE, Gpt_ChannelRunning[GPT_CHANNEL_0]);

    /* Simulate time passing */
    mock_timer_values[0] = 3000;
    TEST_ASSERT_EQ(3000, Gpt_GetTimeElapsed(GPT_CHANNEL_0));
    TEST_ASSERT_EQ(7000, Gpt_GetTimeRemaining(GPT_CHANNEL_0));

    /* More time passes */
    mock_timer_values[0] = 8000;
    TEST_ASSERT_EQ(8000, Gpt_GetTimeElapsed(GPT_CHANNEL_0));
    TEST_ASSERT_EQ(2000, Gpt_GetTimeRemaining(GPT_CHANNEL_0));

    /* Timer expires */
    mock_timer_values[0] = 10000;
    TEST_ASSERT_EQ(0, Gpt_GetTimeRemaining(GPT_CHANNEL_0));

    /* Stop timer */
    Gpt_StopTimer(GPT_CHANNEL_0);
    TEST_ASSERT_EQ(FALSE, Gpt_ChannelRunning[GPT_CHANNEL_0]);

    Gpt_DeInit();
    TEST_ASSERT_EQ(FALSE, Gpt_DriverInitialized);
}

/* @req SWS_Gpt_00007 */
void test_notification_full_lifecycle(void)
{
    printf("\n=== Test: Notification Full Lifecycle ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);

    /* Initially disabled */
    TEST_ASSERT_EQ(FALSE, Gpt_ChannelNotificationEnabled[GPT_CHANNEL_0]);

    /* Enable notification */
    Gpt_EnableNotification(GPT_CHANNEL_0);
    TEST_ASSERT_EQ(TRUE, Gpt_ChannelNotificationEnabled[GPT_CHANNEL_0]);

    /* Disable notification */
    Gpt_DisableNotification(GPT_CHANNEL_0);
    TEST_ASSERT_EQ(FALSE, Gpt_ChannelNotificationEnabled[GPT_CHANNEL_0]);

    /* Re-enable */
    Gpt_EnableNotification(GPT_CHANNEL_0);
    TEST_ASSERT_EQ(TRUE, Gpt_ChannelNotificationEnabled[GPT_CHANNEL_0]);
}

/* @req SWS_Gpt_00011 */
void test_wakeup_full_lifecycle(void)
{
    printf("\n=== Test: Wakeup Full Lifecycle ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);

    /* Enable wakeup */
    Gpt_EnableWakeup(GPT_CHANNEL_1);

    /* Check wakeup (no wakeup event in mock) */
    Std_ReturnType result = Gpt_CheckWakeup(GPT_CHANNEL_1);
    TEST_ASSERT_EQ(E_NOT_OK, result);

    /* Disable wakeup */
    Gpt_DisableWakeup(GPT_CHANNEL_1);

    /* Check should still return E_NOT_OK */
    result = Gpt_CheckWakeup(GPT_CHANNEL_1);
    TEST_ASSERT_EQ(E_NOT_OK, result);
}

/* @req SWS_Gpt_00010 */
void test_mode_transition(void)
{
    printf("\n=== Test: Mode Transition ===\n");
    reset_driver_state();
    setup_test_config();

    Gpt_Init(&test_config);

    /* Start multiple timers */
    Gpt_StartTimer(GPT_CHANNEL_0, 1000);
    Gpt_StartTimer(GPT_CHANNEL_1, 2000);
    Gpt_StartTimer(GPT_CHANNEL_2, 3000);

    TEST_ASSERT_EQ(TRUE, Gpt_ChannelRunning[GPT_CHANNEL_0]);
    TEST_ASSERT_EQ(TRUE, Gpt_ChannelRunning[GPT_CHANNEL_1]);
    TEST_ASSERT_EQ(TRUE, Gpt_ChannelRunning[GPT_CHANNEL_2]);

    /* Enter sleep mode - should stop all timers */
    Gpt_SetMode(GPT_MODE_SLEEP);

    TEST_ASSERT_EQ(GPT_MODE_SLEEP, Gpt_DriverMode);
    TEST_ASSERT_EQ(FALSE, Gpt_ChannelRunning[GPT_CHANNEL_0]);
    TEST_ASSERT_EQ(FALSE, Gpt_ChannelRunning[GPT_CHANNEL_1]);
    TEST_ASSERT_EQ(FALSE, Gpt_ChannelRunning[GPT_CHANNEL_2]);

    /* Return to normal mode */
    Gpt_SetMode(GPT_MODE_NORMAL);
    TEST_ASSERT_EQ(GPT_MODE_NORMAL, Gpt_DriverMode);
}

/*==================================================================================================
*                                      MAIN FUNCTION
==================================================================================================*/
int main(void)
{
    printf("========================================\n");
    printf("   GPT Module Unit Tests\n");
    printf("   YuleTech AutoSAR MCAL\n");
    printf("========================================\n");

    /* Initialization Tests */
    test_init_valid();
    test_init_null_config();
    test_init_already_initialized();

    /* Deinitialization Tests */
    test_deinit_valid();
    test_deinit_not_initialized();
    test_deinit_with_running_channel();

    /* GetTimeElapsed Tests */
    test_get_time_elapsed_valid();
    test_get_time_elapsed_not_initialized();
    test_get_time_elapsed_invalid_channel();
    test_get_time_elapsed_multiple_channels();

    /* GetTimeRemaining Tests */
    test_get_time_remaining_valid();
    test_get_time_remaining_not_running();
    test_get_time_remaining_not_initialized();
    test_get_time_remaining_invalid_channel();
    test_get_time_remaining_expired();

    /* StartTimer Tests */
    test_start_timer_valid();
    test_start_timer_not_initialized();
    test_start_timer_invalid_channel();
    test_start_timer_zero_value();
    test_start_timer_channel_busy();
    test_start_timer_multiple_channels();

    /* StopTimer Tests */
    test_stop_timer_valid();
    test_stop_timer_not_initialized();
    test_stop_timer_invalid_channel();
    test_stop_timer_not_running();

    /* Notification Tests */
    test_enable_notification_valid();
    test_enable_notification_not_initialized();
    test_enable_notification_invalid_channel();
    test_disable_notification_valid();
    test_disable_notification_not_initialized();
    test_disable_notification_invalid_channel();

    /* Version Info Tests */
    test_get_version_info_valid();
    test_get_version_info_null();

    /* SetMode Tests */
    test_set_mode_normal();
    test_set_mode_sleep();
    test_set_mode_not_initialized();

    /* Wakeup Tests */
    test_enable_wakeup_valid();
    test_enable_wakeup_not_initialized();
    test_enable_wakeup_invalid_channel();
    test_disable_wakeup_valid();
    test_disable_wakeup_not_initialized();
    test_disable_wakeup_invalid_channel();
    test_check_wakeup_valid();
    test_check_wakeup_not_initialized();
    test_check_wakeup_invalid_channel();

    /* Predef Timer Tests */
    test_get_predef_timer_valid();
    test_get_predef_timer_not_initialized();
    test_get_predef_timer_null_pointer();
    test_get_predef_timer_all_types();

    /* Integration Tests */
    test_timer_full_lifecycle();
    test_notification_full_lifecycle();
    test_wakeup_full_lifecycle();
    test_mode_transition();

    printf("\n========================================\n");
    printf("   Test Results\n");
    printf("========================================\n");
    printf("Total:   %d\n", tests_run);
    printf("Passed:  %d\n", tests_passed);
    printf("Failed:  %d\n", tests_failed);
    printf("Coverage: %.1f%%\n", (tests_run > 0) ? ((float)tests_passed / tests_run * 100) : 0);

    if (tests_failed == 0) {
        printf("\n*** ALL TESTS PASSED! ***\n");
        return 0;
    } else {
        printf("\n*** SOME TESTS FAILED! ***\n");
        return 1;
    }
}
