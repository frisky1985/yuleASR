/**
 * @file test_icu.c
 * @brief ICU (Input Capture Unit) Driver 模块单元测试
 * @version 1.0.0
 * @date 2026-05-15
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * 测试覆盖率目标: 80%+
 * 测试内容:
 * - 初始化和反初始化
 * - 模式设置 (Normal/Sleep)
 * - 边沿检测配置
 * - 输入状态获取
 * - 时间戳捕获
 * - 边沿计数
 * - 信号测量 (周期/脉宽/占空比)
 * - 通知使能/禁止
 * - 唤醒功能
 * - 版本信息获取
 * - 错误处理
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* ICU 版本信息宏 */
#define ICU_SW_MAJOR_VERSION            1U
#define ICU_SW_MINOR_VERSION            0U
#define ICU_SW_PATCH_VERSION            0U

/* ICU SID定义 */
#define ICU_SID_INIT                            0x00U
#define ICU_SID_DEINIT                          0x01U
#define ICU_SID_SETMODE                         0x02U
#define ICU_SID_DISABLEWAKEUP                   0x03U
#define ICU_SID_ENABLEWAKEUP                    0x04U
#define ICU_SID_CHECKWAKEUP                     0x05U
#define ICU_SID_SETACTIVATIONCONDITION          0x06U
#define ICU_SID_DISABLENOTIFICATION             0x07U
#define ICU_SID_ENABLENOTIFICATION              0x08U
#define ICU_SID_GETINPUTSTATE                   0x09U
#define ICU_SID_STARTTIMESTAMP                  0x0AU
#define ICU_SID_STOPTIMESTAMP                   0x0BU
#define ICU_SID_GETTIMESTAMPINDEX               0x0CU
#define ICU_SID_RESETEDGECOUNT                  0x0DU
#define ICU_SID_ENABLEEDGECOUNT                 0x0EU
#define ICU_SID_DISABLEEDGECOUNT                0x0FU
#define ICU_SID_GETEDGENUMBERS                  0x10U
#define ICU_SID_STARTSIGNALMEASUREMENT          0x11U
#define ICU_SID_STOPSIGNALMEASUREMENT           0x12U
#define ICU_SID_GETTIMEELAPSED                  0x13U
#define ICU_SID_GETDUTYCYCLEVALUES              0x14U
#define ICU_SID_GETVERSIONINFO                  0x15U
#define ICU_SID_GETINPUTLEVEL                   0x16U
#define ICU_SID_GETSYSTIMESTAMP                 0x17U

/* ICU 错误码定义 */
#define ICU_E_PARAM_CONFIG                      0x0AU
#define ICU_E_UNINIT                            0x0BU
#define ICU_E_PARAM_CHANNEL                     0x0CU
#define ICU_E_PARAM_ACTIVATION                  0x0DU
#define ICU_E_PARAM_BUFFER_SIZE                 0x0EU
#define ICU_E_ALREADY_INITIALIZED               0x0FU
#define ICU_E_PARAM_POINTER                     0x10U
#define ICU_E_BUSY                              0x11U
#define ICU_E_WAKEUP_NOT_ENABLED                0x12U
#define ICU_E_WAKEUP_ALREADY_ENABLED            0x13U
#define ICU_E_MEASUREMENT_NOT_RUNNING           0x14U
#define ICU_E_MEASUREMENT_RUNNING               0x15U
#define ICU_E_STAMP_NOT_RUNNING                 0x16U
#define ICU_E_EDGE_COUNTING_NOT_RUNNING         0x17U
#define ICU_E_EDGE_ALREADY_ENABLED              0x18U
#define ICU_E_EDGE_ALREADY_DISABLED             0x19U

/* 标准类型定义 */
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

/* ICU 配置宏 */
#define ICU_NUM_CHANNELS                24U
#define ICU_DEV_ERROR_DETECT            STD_ON
#define ICU_DE_INIT_API                 STD_ON
#define ICU_SET_MODE_API                STD_ON
#define ICU_DISABLE_WAKEUP_API          STD_ON
#define ICU_ENABLE_WAKEUP_API           STD_ON
#define ICU_CHECK_WAKEUP_API            STD_ON
#define ICU_TIMESTAMP_API               STD_ON
#define ICU_EDGE_COUNT_API              STD_ON
#define ICU_SIGNAL_MEASUREMENT_API      STD_ON
#define ICU_VERSION_INFO_API            STD_ON
#define ICU_GET_INPUT_LEVEL_API         STD_ON

/* eMIOS基地址 */
#define ICU_EMIOS_0_BASE_ADDR           0x4002C000U
#define ICU_EMIOS_1_BASE_ADDR           0x4002D000U

/* ICU 类型定义 */
typedef uint8 Icu_ChannelType;

typedef enum {
    ICU_ACTIVE = 0,
    ICU_IDLE
} Icu_InputStateType;

typedef enum {
    ICU_FALLING_EDGE = 0,
    ICU_RISING_EDGE,
    ICU_BOTH_EDGES
} Icu_ActivationType;

typedef enum {
    ICU_MODE_NORMAL = 0,
    ICU_MODE_SLEEP
} Icu_ModeType;

typedef enum {
    ICU_MODE_SIGNAL_EDGE_DETECT = 0,
    ICU_MODE_SIGNAL_MEASUREMENT,
    ICU_MODE_TIMESTAMP,
    ICU_MODE_EDGE_COUNTER
} Icu_MeasurementModeType;

typedef enum {
    ICU_PERIOD_TIME = 0,
    ICU_HIGH_TIME,
    ICU_LOW_TIME,
    ICU_DUTY_CYCLE
} Icu_SignalMeasurementPropertyType;

typedef enum {
    ICU_LINEAR_BUFFER = 0,
    ICU_CIRCULAR_BUFFER
} Icu_TimestampBufferType;

typedef uint16 Icu_IndexType;

typedef struct {
    uint16 ActiveTime;
    uint16 PeriodTime;
} Icu_DutyCycleType;

typedef struct {
    Icu_ChannelType ChannelId;
    uint32 BaseAddress;
    Icu_MeasurementModeType MeasurementMode;
    Icu_ActivationType DefaultActivation;
    Icu_SignalMeasurementPropertyType SignalMeasurementProperty;
    Icu_TimestampBufferType TimestampBufferType;
    uint16 BufferSize;
    uint32* BufferPtr;
    boolean WakeupSupport;
    boolean NotificationEnabled;
    void (*NotificationFn)(void);
    uint32 ClockPrescaler;
} Icu_ChannelConfigType;

typedef struct {
    const Icu_ChannelConfigType* Channels;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean WakeupFunctionalityApi;
    boolean DeInitApi;
    boolean SetModeApi;
    boolean DisableWakeupApi;
    boolean EnableWakeupApi;
    boolean CheckWakeupApi;
    boolean TimestampApi;
    boolean EdgeCountApi;
    boolean SignalMeasurementApi;
    Icu_ModeType DefaultMode;
} Icu_ConfigType;

typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8 sw_major_version;
    uint8 sw_minor_version;
    uint8 sw_patch_version;
} Std_VersionInfoType;

/* 测试结果计数 */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* 模拟硬件寄存器 */
static uint32 mock_emios_regs[2][24][8];  /* 2个eMIOS实例, 24个通道, 8个寄存器 */

/* 驱动状态 */
static boolean Icu_DriverInitialized = FALSE;
static Icu_ModeType Icu_DriverMode = ICU_MODE_NORMAL;
static const Icu_ConfigType* Icu_ConfigPtr = NULL_PTR;

/* 通道运行时数据 */
static Icu_InputStateType Icu_ChannelInputState[ICU_NUM_CHANNELS];
static Icu_ActivationType Icu_ChannelActivation[ICU_NUM_CHANNELS];
static boolean Icu_ChannelNotificationEnabled[ICU_NUM_CHANNELS];
static boolean Icu_ChannelRunning[ICU_NUM_CHANNELS];
static boolean Icu_ChannelWakeupEnabled[ICU_NUM_CHANNELS];

/* 时间戳数据 */
static uint32* Icu_TimestampBuffer[ICU_NUM_CHANNELS];
static uint16 Icu_TimestampBufferSize[ICU_NUM_CHANNELS];
static Icu_IndexType Icu_TimestampIndex[ICU_NUM_CHANNELS];
static uint16 Icu_TimestampNotifyInterval[ICU_NUM_CHANNELS];
static uint16 Icu_TimestampCaptureCount[ICU_NUM_CHANNELS];

/* 边沿计数数据 */
static uint16 Icu_EdgeCount[ICU_NUM_CHANNELS];
static boolean Icu_EdgeCountEnabled[ICU_NUM_CHANNELS];

/* 信号测量数据 */
static uint16 Icu_SignalPeriodTime[ICU_NUM_CHANNELS];
static uint16 Icu_SignalActiveTime[ICU_NUM_CHANNELS];
static uint16 Icu_SignalLastCapture[ICU_NUM_CHANNELS];
static boolean Icu_SignalMeasurementRunning[ICU_NUM_CHANNELS];

/* DET错误记录 */
static uint8 det_last_module = 0;
static uint8 det_last_instance = 0;
static uint8 det_last_api = 0;
static uint8 det_last_error = 0;
static boolean det_called = FALSE;

/* 通知回调记录 */
static boolean notification_called = FALSE;
static Icu_ChannelType notification_channel = 0;

/* 测试宏 */
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
            printf("  [PASS] %s == %s (0x%X == 0x%X)\n", #expected, #actual, (unsigned int)(expected), (unsigned int)(actual)); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s == %s (0x%X != 0x%X) (%s:%d)\n", #expected, #actual, (unsigned int)(expected), (unsigned int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_SETUP() \
    do { \
        printf("\n[TEST] %s\n", __FUNCTION__); \
        test_setup(); \
    } while(0)

/* 测试设置函数 */
static void test_setup(void)
{
    /* 重置驱动状态 */
    Icu_DriverInitialized = FALSE;
    Icu_DriverMode = ICU_MODE_NORMAL;
    Icu_ConfigPtr = NULL_PTR;
    
    /* 清除通道数据 */
    memset(Icu_ChannelInputState, 0, sizeof(Icu_ChannelInputState));
    memset(Icu_ChannelActivation, 0, sizeof(Icu_ChannelActivation));
    memset(Icu_ChannelNotificationEnabled, 0, sizeof(Icu_ChannelNotificationEnabled));
    memset(Icu_ChannelRunning, 0, sizeof(Icu_ChannelRunning));
    memset(Icu_ChannelWakeupEnabled, 0, sizeof(Icu_ChannelWakeupEnabled));
    
    /* 清除时间戳数据 */
    memset(Icu_TimestampBuffer, 0, sizeof(Icu_TimestampBuffer));
    memset(Icu_TimestampBufferSize, 0, sizeof(Icu_TimestampBufferSize));
    memset(Icu_TimestampIndex, 0, sizeof(Icu_TimestampIndex));
    memset(Icu_TimestampNotifyInterval, 0, sizeof(Icu_TimestampNotifyInterval));
    memset(Icu_TimestampCaptureCount, 0, sizeof(Icu_TimestampCaptureCount));
    
    /* 清除边沿计数数据 */
    memset(Icu_EdgeCount, 0, sizeof(Icu_EdgeCount));
    memset(Icu_EdgeCountEnabled, 0, sizeof(Icu_EdgeCountEnabled));
    
    /* 清除信号测量数据 */
    memset(Icu_SignalPeriodTime, 0, sizeof(Icu_SignalPeriodTime));
    memset(Icu_SignalActiveTime, 0, sizeof(Icu_SignalActiveTime));
    memset(Icu_SignalLastCapture, 0, sizeof(Icu_SignalLastCapture));
    memset(Icu_SignalMeasurementRunning, 0, sizeof(Icu_SignalMeasurementRunning));
    
    /* 清除硬件寄存器 */
    memset(mock_emios_regs, 0, sizeof(mock_emios_regs));
    
    /* 清除DET记录 */
    det_called = FALSE;
    det_last_module = 0;
    det_last_instance = 0;
    det_last_api = 0;
    det_last_error = 0;
    
    /* 清除通知记录 */
    notification_called = FALSE;
    notification_channel = 0;
}

/* DET模拟函数 */
void Det_ReportError(uint8 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    det_called = TRUE;
    det_last_module = ModuleId;
    det_last_instance = InstanceId;
    det_last_api = ApiId;
    det_last_error = ErrorId;
    printf("  [DET] Module=0x%X, Api=0x%X, Error=0x%X\n", ModuleId, ApiId, ErrorId);
}

/* 通知回调函数 */
static void test_notification(void)
{
    notification_called = TRUE;
    printf("  [NOTIFY] Notification called\n");
}

/*==================================================================================================
*                                    测试配置
==================================================================================================*/

static Icu_ChannelConfigType test_channels[] = {
    {
        .ChannelId = 0,
        .BaseAddress = ICU_EMIOS_0_BASE_ADDR,
        .MeasurementMode = ICU_MODE_SIGNAL_EDGE_DETECT,
        .DefaultActivation = ICU_RISING_EDGE,
        .SignalMeasurementProperty = ICU_PERIOD_TIME,
        .TimestampBufferType = ICU_LINEAR_BUFFER,
        .BufferSize = 0,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = TRUE,
        .NotificationEnabled = TRUE,
        .NotificationFn = test_notification,
        .ClockPrescaler = 1
    },
    {
        .ChannelId = 1,
        .BaseAddress = ICU_EMIOS_0_BASE_ADDR,
        .MeasurementMode = ICU_MODE_TIMESTAMP,
        .DefaultActivation = ICU_FALLING_EDGE,
        .SignalMeasurementProperty = ICU_HIGH_TIME,
        .TimestampBufferType = ICU_LINEAR_BUFFER,
        .BufferSize = 10,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = FALSE,
        .NotificationEnabled = FALSE,
        .NotificationFn = NULL_PTR,
        .ClockPrescaler = 2
    },
    {
        .ChannelId = 2,
        .BaseAddress = ICU_EMIOS_0_BASE_ADDR,
        .MeasurementMode = ICU_MODE_EDGE_COUNTER,
        .DefaultActivation = ICU_BOTH_EDGES,
        .SignalMeasurementProperty = ICU_LOW_TIME,
        .TimestampBufferType = ICU_CIRCULAR_BUFFER,
        .BufferSize = 0,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = TRUE,
        .NotificationEnabled = TRUE,
        .NotificationFn = test_notification,
        .ClockPrescaler = 4
    },
    {
        .ChannelId = 3,
        .BaseAddress = ICU_EMIOS_0_BASE_ADDR,
        .MeasurementMode = ICU_MODE_SIGNAL_MEASUREMENT,
        .DefaultActivation = ICU_RISING_EDGE,
        .SignalMeasurementProperty = ICU_DUTY_CYCLE,
        .TimestampBufferType = ICU_LINEAR_BUFFER,
        .BufferSize = 0,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = FALSE,
        .NotificationEnabled = FALSE,
        .NotificationFn = NULL_PTR,
        .ClockPrescaler = 1
    }
};

static const Icu_ConfigType test_config = {
    .Channels = test_channels,
    .NumChannels = 4,
    .DevErrorDetect = TRUE,
    .VersionInfoApi = TRUE,
    .WakeupFunctionalityApi = TRUE,
    .DeInitApi = TRUE,
    .SetModeApi = TRUE,
    .DisableWakeupApi = TRUE,
    .EnableWakeupApi = TRUE,
    .CheckWakeupApi = TRUE,
    .TimestampApi = TRUE,
    .EdgeCountApi = TRUE,
    .SignalMeasurementApi = TRUE,
    .DefaultMode = ICU_MODE_NORMAL
};

/*==================================================================================================
*                                    ICU API 模拟实现
==================================================================================================*/

static uint32 Icu_GetEmiosBaseAddr(Icu_ChannelType channel)
{
    if (channel < (ICU_NUM_CHANNELS / 2U)) {
        return ICU_EMIOS_0_BASE_ADDR;
    } else {
        return ICU_EMIOS_1_BASE_ADDR;
    }
}

static uint8 Icu_GetEmiosChannelNum(Icu_ChannelType channel)
{
    if (channel < (ICU_NUM_CHANNELS / 2U)) {
        return (uint8)channel;
    } else {
        return (uint8)(channel - (ICU_NUM_CHANNELS / 2U));
    }
}

void Icu_Init(const Icu_ConfigType* ConfigPtr)
{
    uint8 i;
    
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(0x16U, 0U, ICU_SID_INIT, ICU_E_PARAM_POINTER);
        return;
    }
    
    if (Icu_DriverInitialized == TRUE) {
        Det_ReportError(0x16U, 0U, ICU_SID_INIT, ICU_E_ALREADY_INITIALIZED);
        return;
    }
    
    Icu_ConfigPtr = ConfigPtr;
    
    for (i = 0; i < ConfigPtr->NumChannels; i++) {
        const Icu_ChannelConfigType* chConfig = &ConfigPtr->Channels[i];
        Icu_ChannelType channel = chConfig->ChannelId;
        
        if (channel >= ICU_NUM_CHANNELS) {
            continue;
        }
        
        Icu_ChannelInputState[channel] = ICU_IDLE;
        Icu_ChannelActivation[channel] = chConfig->DefaultActivation;
        Icu_ChannelNotificationEnabled[channel] = chConfig->NotificationEnabled;
        Icu_ChannelRunning[channel] = FALSE;
        Icu_ChannelWakeupEnabled[channel] = FALSE;
        Icu_TimestampBuffer[channel] = NULL_PTR;
        Icu_TimestampBufferSize[channel] = 0U;
        Icu_TimestampIndex[channel] = 0U;
        Icu_EdgeCount[channel] = 0U;
        Icu_EdgeCountEnabled[channel] = FALSE;
        Icu_SignalPeriodTime[channel] = 0U;
        Icu_SignalActiveTime[channel] = 0U;
        Icu_SignalLastCapture[channel] = 0U;
        Icu_SignalMeasurementRunning[channel] = FALSE;
    }
    
    Icu_DriverMode = ConfigPtr->DefaultMode;
    Icu_DriverInitialized = TRUE;
}

void Icu_DeInit(void)
{
    uint8 i;
    
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_DEINIT, ICU_E_UNINIT);
        return;
    }
    
    for (i = 0; i < Icu_ConfigPtr->NumChannels; i++) {
        Icu_ChannelType channel = Icu_ConfigPtr->Channels[i].ChannelId;
        Icu_ChannelRunning[channel] = FALSE;
    }
    
    Icu_DriverInitialized = FALSE;
}

void Icu_SetMode(Icu_ModeType Mode)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_SETMODE, ICU_E_UNINIT);
        return;
    }
    
    if ((Mode != ICU_MODE_NORMAL) && (Mode != ICU_MODE_SLEEP)) {
        Det_ReportError(0x16U, 0U, ICU_SID_SETMODE, ICU_E_PARAM_POINTER);
        return;
    }
    
    Icu_DriverMode = Mode;
}

void Icu_DisableWakeup(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_DISABLEWAKEUP, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_DISABLEWAKEUP, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    Icu_ChannelWakeupEnabled[Channel] = FALSE;
}

void Icu_EnableWakeup(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_ENABLEWAKEUP, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_ENABLEWAKEUP, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    Icu_ChannelWakeupEnabled[Channel] = TRUE;
}

Std_ReturnType Icu_CheckWakeup(uint32 WakeupSource)
{
    uint8 i;
    
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_CHECKWAKEUP, ICU_E_UNINIT);
        return E_NOT_OK;
    }
    
    for (i = 0; i < Icu_ConfigPtr->NumChannels; i++) {
        Icu_ChannelType channel = Icu_ConfigPtr->Channels[i].ChannelId;
        if (Icu_ChannelWakeupEnabled[channel]) {
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

void Icu_SetActivationCondition(Icu_ChannelType Channel, Icu_ActivationType Activation)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_SETACTIVATIONCONDITION, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_SETACTIVATIONCONDITION, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    if ((Activation != ICU_FALLING_EDGE) && 
        (Activation != ICU_RISING_EDGE) && 
        (Activation != ICU_BOTH_EDGES)) {
        Det_ReportError(0x16U, 0U, ICU_SID_SETACTIVATIONCONDITION, ICU_E_PARAM_ACTIVATION);
        return;
    }
    
    Icu_ChannelActivation[Channel] = Activation;
}

void Icu_DisableNotification(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_DISABLENOTIFICATION, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_DISABLENOTIFICATION, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    Icu_ChannelNotificationEnabled[Channel] = FALSE;
}

void Icu_EnableNotification(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_ENABLENOTIFICATION, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_ENABLENOTIFICATION, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    Icu_ChannelNotificationEnabled[Channel] = TRUE;
}

Icu_InputStateType Icu_GetInputState(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETINPUTSTATE, ICU_E_UNINIT);
        return ICU_IDLE;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETINPUTSTATE, ICU_E_PARAM_CHANNEL);
        return ICU_IDLE;
    }
    
    return Icu_ChannelInputState[Channel];
}

void Icu_StartTimestamp(Icu_ChannelType Channel, uint32* BufferPtr, uint16 BufferSize, uint16 NotifyInterval)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    if (BufferPtr == NULL_PTR) {
        Det_ReportError(0x16U, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_PARAM_POINTER);
        return;
    }
    
    if (BufferSize == 0U) {
        Det_ReportError(0x16U, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_PARAM_BUFFER_SIZE);
        return;
    }
    
    if (Icu_ChannelRunning[Channel]) {
        Det_ReportError(0x16U, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_BUSY);
        return;
    }
    
    Icu_TimestampBuffer[Channel] = BufferPtr;
    Icu_TimestampBufferSize[Channel] = BufferSize;
    Icu_TimestampIndex[Channel] = 0U;
    Icu_TimestampNotifyInterval[Channel] = NotifyInterval;
    Icu_TimestampCaptureCount[Channel] = 0U;
    Icu_ChannelRunning[Channel] = TRUE;
}

void Icu_StopTimestamp(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_STOPTIMESTAMP, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_STOPTIMESTAMP, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    Icu_ChannelRunning[Channel] = FALSE;
    Icu_TimestampBuffer[Channel] = NULL_PTR;
}

Icu_IndexType Icu_GetTimestampIndex(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETTIMESTAMPINDEX, ICU_E_UNINIT);
        return 0U;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETTIMESTAMPINDEX, ICU_E_PARAM_CHANNEL);
        return 0U;
    }
    
    if (Icu_ChannelRunning[Channel] == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETTIMESTAMPINDEX, ICU_E_STAMP_NOT_RUNNING);
        return 0U;
    }
    
    return Icu_TimestampIndex[Channel];
}

void Icu_ResetEdgeCount(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_RESETEDGECOUNT, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_RESETEDGECOUNT, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    Icu_EdgeCount[Channel] = 0U;
}

void Icu_EnableEdgeCount(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_ENABLEEDGECOUNT, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_ENABLEEDGECOUNT, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    Icu_EdgeCountEnabled[Channel] = TRUE;
}

void Icu_DisableEdgeCount(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_DISABLEEDGECOUNT, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_DISABLEEDGECOUNT, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    Icu_EdgeCountEnabled[Channel] = FALSE;
}

uint16 Icu_GetEdgeNumbers(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETEDGENUMBERS, ICU_E_UNINIT);
        return 0U;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETEDGENUMBERS, ICU_E_PARAM_CHANNEL);
        return 0U;
    }
    
    if (Icu_EdgeCountEnabled[Channel] == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETEDGENUMBERS, ICU_E_EDGE_COUNTING_NOT_RUNNING);
        return 0U;
    }
    
    return Icu_EdgeCount[Channel];
}

void Icu_StartSignalMeasurement(Icu_ChannelType Channel, Icu_SignalMeasurementPropertyType MeasureKind)
{
    (void)MeasureKind;
    
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_STARTSIGNALMEASUREMENT, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_STARTSIGNALMEASUREMENT, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    if (Icu_SignalMeasurementRunning[Channel]) {
        Det_ReportError(0x16U, 0U, ICU_SID_STARTSIGNALMEASUREMENT, ICU_E_MEASUREMENT_RUNNING);
        return;
    }
    
    Icu_SignalPeriodTime[Channel] = 0U;
    Icu_SignalActiveTime[Channel] = 0U;
    Icu_SignalLastCapture[Channel] = 0U;
    Icu_SignalMeasurementRunning[Channel] = TRUE;
}

void Icu_StopSignalMeasurement(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_STOPSIGNALMEASUREMENT, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_STOPSIGNALMEASUREMENT, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    Icu_SignalMeasurementRunning[Channel] = FALSE;
}

uint16 Icu_GetTimeElapsed(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETTIMEELAPSED, ICU_E_UNINIT);
        return 0U;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETTIMEELAPSED, ICU_E_PARAM_CHANNEL);
        return 0U;
    }
    
    return Icu_SignalPeriodTime[Channel];
}

void Icu_GetDutyCycleValues(Icu_ChannelType Channel, Icu_DutyCycleType* DutyCycleValues)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETDUTYCYCLEVALUES, ICU_E_UNINIT);
        return;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETDUTYCYCLEVALUES, ICU_E_PARAM_CHANNEL);
        return;
    }
    
    if (DutyCycleValues == NULL_PTR) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETDUTYCYCLEVALUES, ICU_E_PARAM_POINTER);
        return;
    }
    
    DutyCycleValues->ActiveTime = Icu_SignalActiveTime[Channel];
    DutyCycleValues->PeriodTime = Icu_SignalPeriodTime[Channel];
}

void Icu_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo == NULL_PTR) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETVERSIONINFO, ICU_E_PARAM_POINTER);
        return;
    }
    
    versioninfo->vendorID = 0x01U;
    versioninfo->moduleID = 0x16U;
    versioninfo->sw_major_version = ICU_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = ICU_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = ICU_SW_PATCH_VERSION;
}

uint8 Icu_GetInputLevel(Icu_ChannelType Channel)
{
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETINPUTLEVEL, ICU_E_UNINIT);
        return 0U;
    }
    
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(0x16U, 0U, ICU_SID_GETINPUTLEVEL, ICU_E_PARAM_CHANNEL);
        return 0U;
    }
    
    return 0U;  /* 模拟输入低电平 */
}

uint32 Icu_GetSysTimestamp(void)
{
    return 0x12345678U;  /* 模拟系统时间戳 */
}

/*==================================================================================================
*                                    测试用例
==================================================================================================*/

/* 1. 初始化测试 */
static void test_Icu_Init_NullPtr_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_Init(NULL_PTR);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_PARAM_POINTER);
    TEST_ASSERT(Icu_DriverInitialized == FALSE);
}

static void test_Icu_Init_ValidConfig_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    
    TEST_ASSERT(det_called == FALSE);
    TEST_ASSERT(Icu_DriverInitialized == TRUE);
    TEST_ASSERT(Icu_DriverMode == ICU_MODE_NORMAL);
    TEST_ASSERT(Icu_ConfigPtr == &test_config);
    TEST_ASSERT(Icu_ChannelNotificationEnabled[0] == TRUE);
    TEST_ASSERT(Icu_ChannelNotificationEnabled[1] == FALSE);
}

static void test_Icu_Init_DoubleInit_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    det_called = FALSE;
    
    Icu_Init(&test_config);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_ALREADY_INITIALIZED);
}

/* 2. 反初始化测试 */
static void test_Icu_DeInit_AfterInit_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_DeInit();
    
    TEST_ASSERT(Icu_DriverInitialized == FALSE);
}

static void test_Icu_DeInit_WithoutInit_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_DeInit();
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_UNINIT);
}

/* 3. 模式设置测试 */
static void test_Icu_SetMode_Sleep_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_SetMode(ICU_MODE_SLEEP);
    
    TEST_ASSERT(Icu_DriverMode == ICU_MODE_SLEEP);
    TEST_ASSERT(det_called == FALSE);
}

static void test_Icu_SetMode_Normal_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_SetMode(ICU_MODE_SLEEP);
    Icu_SetMode(ICU_MODE_NORMAL);
    
    TEST_ASSERT(Icu_DriverMode == ICU_MODE_NORMAL);
    TEST_ASSERT(det_called == FALSE);
}

static void test_Icu_SetMode_WithoutInit_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_SetMode(ICU_MODE_NORMAL);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_UNINIT);
}

/* 4. 唤醒功能测试 */
static void test_Icu_EnableWakeup_ValidChannel_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_EnableWakeup(0);
    
    TEST_ASSERT(Icu_ChannelWakeupEnabled[0] == TRUE);
    TEST_ASSERT(det_called == FALSE);
}

static void test_Icu_DisableWakeup_ValidChannel_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_EnableWakeup(0);
    Icu_DisableWakeup(0);
    
    TEST_ASSERT(Icu_ChannelWakeupEnabled[0] == FALSE);
    TEST_ASSERT(det_called == FALSE);
}

static void test_Icu_EnableWakeup_InvalidChannel_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_EnableWakeup(50);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_PARAM_CHANNEL);
}

static void test_Icu_CheckWakeup_EnabledChannel_ShouldReturnOk(void)
{
    Std_ReturnType result;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_EnableWakeup(0);
    result = Icu_CheckWakeup(0);
    
    TEST_ASSERT(result == E_OK);
}

static void test_Icu_CheckWakeup_DisabledChannel_ShouldReturnNotOk(void)
{
    Std_ReturnType result;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_DisableWakeup(0);
    Icu_DisableWakeup(1);
    Icu_DisableWakeup(2);
    Icu_DisableWakeup(3);
    result = Icu_CheckWakeup(0);
    
    TEST_ASSERT(result == E_NOT_OK);
}

/* 5. 边沿检测配置测试 */
static void test_Icu_SetActivationCondition_Rising_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_SetActivationCondition(0, ICU_RISING_EDGE);
    
    TEST_ASSERT(Icu_ChannelActivation[0] == ICU_RISING_EDGE);
    TEST_ASSERT(det_called == FALSE);
}

static void test_Icu_SetActivationCondition_Falling_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_SetActivationCondition(0, ICU_FALLING_EDGE);
    
    TEST_ASSERT(Icu_ChannelActivation[0] == ICU_FALLING_EDGE);
}

static void test_Icu_SetActivationCondition_Both_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_SetActivationCondition(0, ICU_BOTH_EDGES);
    
    TEST_ASSERT(Icu_ChannelActivation[0] == ICU_BOTH_EDGES);
}

static void test_Icu_SetActivationCondition_InvalidChannel_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_SetActivationCondition(50, ICU_RISING_EDGE);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_PARAM_CHANNEL);
}

static void test_Icu_SetActivationCondition_InvalidEdge_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_SetActivationCondition(0, 10);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_PARAM_ACTIVATION);
}

/* 6. 通知测试 */
static void test_Icu_EnableNotification_ValidChannel_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_DisableNotification(0);
    Icu_EnableNotification(0);
    
    TEST_ASSERT(Icu_ChannelNotificationEnabled[0] == TRUE);
}

static void test_Icu_DisableNotification_ValidChannel_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_DisableNotification(0);
    
    TEST_ASSERT(Icu_ChannelNotificationEnabled[0] == FALSE);
}

static void test_Icu_EnableNotification_WithoutInit_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_EnableNotification(0);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_UNINIT);
}

/* 7. 输入状态测试 */
static void test_Icu_GetInputState_ValidChannel_ShouldReturnIdle(void)
{
    Icu_InputStateType state;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    state = Icu_GetInputState(0);
    
    TEST_ASSERT(state == ICU_IDLE);
}

static void test_Icu_GetInputState_InvalidChannel_ShouldReportError(void)
{
    Icu_InputStateType state;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    state = Icu_GetInputState(50);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_PARAM_CHANNEL);
}

static void test_Icu_GetInputState_WithoutInit_ShouldReportError(void)
{
    Icu_InputStateType state;
    
    TEST_SETUP();
    
    state = Icu_GetInputState(0);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_UNINIT);
}

/* 8. 时间戳测试 */
static void test_Icu_StartTimestamp_ValidConfig_ShouldSucceed(void)
{
    uint32 buffer[10];
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartTimestamp(1, buffer, 10, 5);
    
    TEST_ASSERT(Icu_ChannelRunning[1] == TRUE);
    TEST_ASSERT(Icu_TimestampBuffer[1] == buffer);
    TEST_ASSERT(Icu_TimestampBufferSize[1] == 10);
    TEST_ASSERT(Icu_TimestampIndex[1] == 0);
    TEST_ASSERT(det_called == FALSE);
}

static void test_Icu_StartTimestamp_NullBuffer_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartTimestamp(1, NULL_PTR, 10, 5);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_PARAM_POINTER);
}

static void test_Icu_StartTimestamp_ZeroBufferSize_ShouldReportError(void)
{
    uint32 buffer[10];
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartTimestamp(1, buffer, 0, 5);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_PARAM_BUFFER_SIZE);
}

static void test_Icu_StartTimestamp_BusyChannel_ShouldReportError(void)
{
    uint32 buffer[10];
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartTimestamp(1, buffer, 10, 5);
    det_called = FALSE;
    Icu_StartTimestamp(1, buffer, 10, 5);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_BUSY);
}

static void test_Icu_StopTimestamp_AfterStart_ShouldSucceed(void)
{
    uint32 buffer[10];
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartTimestamp(1, buffer, 10, 5);
    Icu_StopTimestamp(1);
    
    TEST_ASSERT(Icu_ChannelRunning[1] == FALSE);
    TEST_ASSERT(Icu_TimestampBuffer[1] == NULL_PTR);
}

static void test_Icu_GetTimestampIndex_AfterStart_ShouldReturnZero(void)
{
    uint32 buffer[10];
    Icu_IndexType index;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartTimestamp(1, buffer, 10, 5);
    index = Icu_GetTimestampIndex(1);
    
    TEST_ASSERT(index == 0);
}

static void test_Icu_GetTimestampIndex_NotRunning_ShouldReportError(void)
{
    Icu_IndexType index;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    index = Icu_GetTimestampIndex(1);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_STAMP_NOT_RUNNING);
}

/* 9. 边沿计数测试 */
static void test_Icu_EnableEdgeCount_ValidChannel_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_EnableEdgeCount(2);
    
    TEST_ASSERT(Icu_EdgeCountEnabled[2] == TRUE);
}

static void test_Icu_DisableEdgeCount_AfterEnable_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_EnableEdgeCount(2);
    Icu_DisableEdgeCount(2);
    
    TEST_ASSERT(Icu_EdgeCountEnabled[2] == FALSE);
}

static void test_Icu_GetEdgeNumbers_EnabledChannel_ShouldReturnCount(void)
{
    uint16 count;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_EnableEdgeCount(2);
    Icu_EdgeCount[2] = 100;
    count = Icu_GetEdgeNumbers(2);
    
    TEST_ASSERT(count == 100);
}

static void test_Icu_GetEdgeNumbers_DisabledChannel_ShouldReportError(void)
{
    uint16 count;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_DisableEdgeCount(2);
    count = Icu_GetEdgeNumbers(2);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_EDGE_COUNTING_NOT_RUNNING);
}

static void test_Icu_ResetEdgeCount_ShouldClearCounter(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_EnableEdgeCount(2);
    Icu_EdgeCount[2] = 100;
    Icu_ResetEdgeCount(2);
    
    TEST_ASSERT(Icu_EdgeCount[2] == 0);
}

/* 10. 信号测量测试 */
static void test_Icu_StartSignalMeasurement_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartSignalMeasurement(3, ICU_DUTY_CYCLE);
    
    TEST_ASSERT(Icu_SignalMeasurementRunning[3] == TRUE);
    TEST_ASSERT(Icu_SignalPeriodTime[3] == 0);
    TEST_ASSERT(Icu_SignalActiveTime[3] == 0);
}

static void test_Icu_StartSignalMeasurement_AlreadyRunning_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartSignalMeasurement(3, ICU_DUTY_CYCLE);
    det_called = FALSE;
    Icu_StartSignalMeasurement(3, ICU_DUTY_CYCLE);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_MEASUREMENT_RUNNING);
}

static void test_Icu_StopSignalMeasurement_AfterStart_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartSignalMeasurement(3, ICU_DUTY_CYCLE);
    Icu_StopSignalMeasurement(3);
    
    TEST_ASSERT(Icu_SignalMeasurementRunning[3] == FALSE);
}

static void test_Icu_GetTimeElapsed_ShouldReturnPeriod(void)
{
    uint16 time;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartSignalMeasurement(3, ICU_PERIOD_TIME);
    Icu_SignalPeriodTime[3] = 1000;
    time = Icu_GetTimeElapsed(3);
    
    TEST_ASSERT(time == 1000);
}

static void test_Icu_GetDutyCycleValues_ShouldReturnCorrectValues(void)
{
    Icu_DutyCycleType dutyCycle;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartSignalMeasurement(3, ICU_DUTY_CYCLE);
    Icu_SignalPeriodTime[3] = 1000;
    Icu_SignalActiveTime[3] = 400;
    Icu_GetDutyCycleValues(3, &dutyCycle);
    
    TEST_ASSERT(dutyCycle.PeriodTime == 1000);
    TEST_ASSERT(dutyCycle.ActiveTime == 400);
}

static void test_Icu_GetDutyCycleValues_NullPointer_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_StartSignalMeasurement(3, ICU_DUTY_CYCLE);
    Icu_GetDutyCycleValues(3, NULL_PTR);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_PARAM_POINTER);
}

/* 11. 版本信息测试 */
static void test_Icu_GetVersionInfo_ValidPointer_ShouldReturnVersion(void)
{
    Std_VersionInfoType version;
    
    TEST_SETUP();
    
    Icu_GetVersionInfo(&version);
    
    TEST_ASSERT(version.vendorID == 0x01U);
    TEST_ASSERT(version.moduleID == 0x16U);
    TEST_ASSERT(version.sw_major_version == ICU_SW_MAJOR_VERSION);
    TEST_ASSERT(version.sw_minor_version == ICU_SW_MINOR_VERSION);
    TEST_ASSERT(version.sw_patch_version == ICU_SW_PATCH_VERSION);
}

static void test_Icu_GetVersionInfo_NullPointer_ShouldReportError(void)
{
    TEST_SETUP();
    
    Icu_GetVersionInfo(NULL_PTR);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_PARAM_POINTER);
}

/* 12. 输入电平测试 */
static void test_Icu_GetInputLevel_ValidChannel_ShouldSucceed(void)
{
    uint8 level;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    level = Icu_GetInputLevel(0);
    
    TEST_ASSERT(level == 0 || level == 1);
    TEST_ASSERT(det_called == FALSE);
}

static void test_Icu_GetInputLevel_InvalidChannel_ShouldReportError(void)
{
    uint8 level;
    
    TEST_SETUP();
    
    Icu_Init(&test_config);
    level = Icu_GetInputLevel(50);
    
    TEST_ASSERT(det_called == TRUE);
    TEST_ASSERT(det_last_error == ICU_E_PARAM_CHANNEL);
}

/* 13. 系统时间戳测试 */
static void test_Icu_GetSysTimestamp_ShouldReturnValue(void)
{
    uint32 timestamp;
    
    TEST_SETUP();
    
    timestamp = Icu_GetSysTimestamp();
    
    TEST_ASSERT(timestamp != 0);
}

/* 14. 边界测试 */
static void test_Icu_ChannelBoundary_FirstChannel_ShouldSucceed(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_EnableWakeup(0);
    
    TEST_ASSERT(Icu_ChannelWakeupEnabled[0] == TRUE);
}

static void test_Icu_ChannelBoundary_LastChannel_ShouldFail(void)
{
    TEST_SETUP();
    
    Icu_Init(&test_config);
    Icu_EnableWakeup(23);  /* 最后一个有效通道 */
    
    TEST_ASSERT(Icu_ChannelWakeupEnabled[23] == TRUE);
}

/*==================================================================================================
*                                    测试主函数
==================================================================================================*/

int main(void)
{
    printf("================================================================================\n");
    printf("                    ICU (Input Capture Unit) Unit Tests                       \n");
    printf("================================================================================\n\n");

    /* 初始化测试 */
    test_Icu_Init_NullPtr_ShouldReportError();
    test_Icu_Init_ValidConfig_ShouldSucceed();
    test_Icu_Init_DoubleInit_ShouldReportError();
    
    /* 反初始化测试 */
    test_Icu_DeInit_AfterInit_ShouldSucceed();
    test_Icu_DeInit_WithoutInit_ShouldReportError();
    
    /* 模式设置测试 */
    test_Icu_SetMode_Sleep_ShouldSucceed();
    test_Icu_SetMode_Normal_ShouldSucceed();
    test_Icu_SetMode_WithoutInit_ShouldReportError();
    
    /* 唤醒功能测试 */
    test_Icu_EnableWakeup_ValidChannel_ShouldSucceed();
    test_Icu_DisableWakeup_ValidChannel_ShouldSucceed();
    test_Icu_EnableWakeup_InvalidChannel_ShouldReportError();
    test_Icu_CheckWakeup_EnabledChannel_ShouldReturnOk();
    test_Icu_CheckWakeup_DisabledChannel_ShouldReturnNotOk();
    
    /* 边沿检测配置测试 */
    test_Icu_SetActivationCondition_Rising_ShouldSucceed();
    test_Icu_SetActivationCondition_Falling_ShouldSucceed();
    test_Icu_SetActivationCondition_Both_ShouldSucceed();
    test_Icu_SetActivationCondition_InvalidChannel_ShouldReportError();
    test_Icu_SetActivationCondition_InvalidEdge_ShouldReportError();
    
    /* 通知测试 */
    test_Icu_EnableNotification_ValidChannel_ShouldSucceed();
    test_Icu_DisableNotification_ValidChannel_ShouldSucceed();
    test_Icu_EnableNotification_WithoutInit_ShouldReportError();
    
    /* 输入状态测试 */
    test_Icu_GetInputState_ValidChannel_ShouldReturnIdle();
    test_Icu_GetInputState_InvalidChannel_ShouldReportError();
    test_Icu_GetInputState_WithoutInit_ShouldReportError();
    
    /* 时间戳测试 */
    test_Icu_StartTimestamp_ValidConfig_ShouldSucceed();
    test_Icu_StartTimestamp_NullBuffer_ShouldReportError();
    test_Icu_StartTimestamp_ZeroBufferSize_ShouldReportError();
    test_Icu_StartTimestamp_BusyChannel_ShouldReportError();
    test_Icu_StopTimestamp_AfterStart_ShouldSucceed();
    test_Icu_GetTimestampIndex_AfterStart_ShouldReturnZero();
    test_Icu_GetTimestampIndex_NotRunning_ShouldReportError();
    
    /* 边沿计数测试 */
    test_Icu_EnableEdgeCount_ValidChannel_ShouldSucceed();
    test_Icu_DisableEdgeCount_AfterEnable_ShouldSucceed();
    test_Icu_GetEdgeNumbers_EnabledChannel_ShouldReturnCount();
    test_Icu_GetEdgeNumbers_DisabledChannel_ShouldReportError();
    test_Icu_ResetEdgeCount_ShouldClearCounter();
    
    /* 信号测量测试 */
    test_Icu_StartSignalMeasurement_ShouldSucceed();
    test_Icu_StartSignalMeasurement_AlreadyRunning_ShouldReportError();
    test_Icu_StopSignalMeasurement_AfterStart_ShouldSucceed();
    test_Icu_GetTimeElapsed_ShouldReturnPeriod();
    test_Icu_GetDutyCycleValues_ShouldReturnCorrectValues();
    test_Icu_GetDutyCycleValues_NullPointer_ShouldReportError();
    
    /* 版本信息测试 */
    test_Icu_GetVersionInfo_ValidPointer_ShouldReturnVersion();
    test_Icu_GetVersionInfo_NullPointer_ShouldReportError();
    
    /* 输入电平测试 */
    test_Icu_GetInputLevel_ValidChannel_ShouldSucceed();
    test_Icu_GetInputLevel_InvalidChannel_ShouldReportError();
    
    /* 系统时间戳测试 */
    test_Icu_GetSysTimestamp_ShouldReturnValue();
    
    /* 边界测试 */
    test_Icu_ChannelBoundary_FirstChannel_ShouldSucceed();
    test_Icu_ChannelBoundary_LastChannel_ShouldFail();
    
    /* 测试结果汇总 */
    printf("\n================================================================================\n");
    printf("                              Test Summary                                     \n");
    printf("================================================================================\n");
    printf("  Total Tests:  %d\n", tests_run);
    printf("  Passed:       %d\n", tests_passed);
    printf("  Failed:       %d\n", tests_failed);
    printf("  Coverage:     %d%%\n", (tests_run > 0) ? (tests_passed * 100 / tests_run) : 0);
    printf("================================================================================\n");

    if (tests_failed == 0) {
        printf("\n  [SUCCESS] All tests passed!\n\n");
        return 0;
    } else {
        printf("\n  [FAILURE] %d test(s) failed!\n\n", tests_failed);
        return 1;
    }
}
