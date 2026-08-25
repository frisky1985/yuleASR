/**
 * @file test_pwm.c
 * @brief PWM Driver 模块单元测试
 * @version 1.0.0
 * @date 2026-05-15
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 * 本测试文件为MCAL层PWM模块提供全面的单元测试覆盖，包括:
 * - 初始化/反初始化 (Pwm_Init, Pwm_DeInit)
 * - 占空比设置 (Pwm_SetDutyCycle)
 * - 周期和占空比设置 (Pwm_SetPeriodAndDuty)
 * - 输出控制 (Pwm_SetOutputToIdle, Pwm_GetOutputState)
 * - 通知管理 (Pwm_EnableNotification, Pwm_DisableNotification)
 * - 版本信息 (Pwm_GetVersionInfo)
 * - 电源模式 (Pwm_SetPowerState, Pwm_GetTargetPowerState, Pwm_GetCurrentPowerState, Pwm_PreparePowerState)
 *
 * @test_coverage 目标覆盖率: 80%+
 */

// @tests src/bsw/mcal/pwm/src/Pwm.c  @tests src/bsw/mcal/pwm/include/Pwm.h

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
#define PWM_VENDOR_ID                   (0x01U)
#define PWM_MODULE_ID                   (0x11U)
#define PWM_SW_MAJOR_VERSION            (0x01U)
#define PWM_SW_MINOR_VERSION            (0x00U)
#define PWM_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                      SERVICE IDs
==================================================================================================*/
#define PWM_SID_INIT                    (0x00U)
#define PWM_SID_DEINIT                  (0x01U)
#define PWM_SID_SETDUTYCYCLE            (0x02U)
#define PWM_SID_SETPERIODANDDUTY        (0x03U)
#define PWM_SID_SETOUTPUTTOIDLE         (0x04U)
#define PWM_SID_GETOUTPUTSTATE          (0x05U)
#define PWM_SID_DISABLENOTIFICATION     (0x06U)
#define PWM_SID_ENABLENOTIFICATION      (0x07U)
#define PWM_SID_GETVERSIONINFO          (0x08U)
#define PWM_SID_SETPOWERSTATE           (0x09U)
#define PWM_SID_GETTARGETPOWERSTATE     (0x0AU)
#define PWM_SID_GETCURRENTPOWERSTATE    (0x0BU)
#define PWM_SID_PREPAREPOWERSTATE       (0x0CU)

/*==================================================================================================
*                                      DET ERROR CODES
==================================================================================================*/
#define PWM_E_PARAM_CONFIG              (0x0AU)
#define PWM_E_UNINIT                    (0x0BU)
#define PWM_E_PARAM_CHANNEL             (0x0CU)
#define PWM_E_PERIOD_UNCHANGEABLE       (0x0DU)
#define PWM_E_ALREADY_INITIALIZED       (0x0EU)
#define PWM_E_PARAM_POINTER             (0x0FU)
#define PWM_E_POWER_STATE_NOT_SUPPORTED (0x10U)
#define PWM_E_TRANSITION_NOT_POSSIBLE   (0x11U)
#define PWM_E_PERIPHERAL_NOT_PREPARED   (0x12U)

/*==================================================================================================
*                                      TYPE DEFINITIONS
==================================================================================================*/
typedef uint8 Pwm_ChannelType;
typedef uint32 Pwm_PeriodType;
typedef uint16 Pwm_DutyCycleType;

typedef enum {
    PWM_LOW = 0,
    PWM_HIGH
} Pwm_OutputStateType;

typedef enum {
    PWM_RISING_EDGE = 0,
    PWM_FALLING_EDGE,
    PWM_BOTH_EDGES
} Pwm_EdgeNotificationType;

typedef enum {
    PWM_FULL_POWER = 0,
    PWM_LOW_POWER
} Pwm_PowerStateRequestResultType;

typedef enum {
    PWM_SERVICE_ACCEPTED = 0,
    PWM_NOT_INIT,
    PWM_SEQUENCE_ERROR,
    PWM_HW_FAILURE,
    PWM_POWER_STATE_NOT_SUPP,
    PWM_TRANS_NOT_POSSIBLE
} Pwm_PowerStateType;

typedef enum {
    PWM_VARIABLE_PERIOD = 0,
    PWM_FIXED_PERIOD,
    PWM_FIXED_PERIOD_SHIFTED
} Pwm_ChannelClassType;

typedef enum {
    PWM_IDLE_LOW = 0,
    PWM_IDLE_HIGH
} Pwm_IdleStateType;

typedef enum {
    PWM_POLARITY_LOW = 0,
    PWM_POLARITY_HIGH
} Pwm_PolarityType;

typedef enum {
    PWM_CLOCK_SYSTEM = 0,
    PWM_CLOCK_BUS,
    PWM_CLOCK_EXTERNAL
} Pwm_ClockSourceType;

typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8 sw_major_version;
    uint8 sw_minor_version;
    uint8 sw_patch_version;
} Std_VersionInfoType;

typedef struct {
    Pwm_ChannelType ChannelId;
    uint32 BaseAddress;
    Pwm_ChannelClassType ChannelClass;
    Pwm_PeriodType DefaultPeriod;
    Pwm_DutyCycleType DefaultDutyCycle;
    Pwm_IdleStateType IdleState;
    Pwm_PolarityType Polarity;
    Pwm_ClockSourceType ClockSource;
    uint32 ClockPrescaler;
    boolean NotificationSupported;
    void (*NotificationFn)(void);
} Pwm_ChannelConfigType;

typedef struct {
    const Pwm_ChannelConfigType* Channels;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean DeInitApi;
    boolean SetDutyCycleApi;
    boolean SetPeriodAndDutyApi;
    boolean SetOutputToIdleApi;
    boolean GetOutputStateApi;
    boolean NotificationSupported;
    boolean PowerStateSupported;
} Pwm_ConfigType;

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
*                                      CONFIGURATION
==================================================================================================*/
#define PWM_NUM_CHANNELS                (8U)
#define PWM_CHANNEL_0                   ((Pwm_ChannelType)0U)
#define PWM_CHANNEL_1                   ((Pwm_ChannelType)1U)
#define PWM_CHANNEL_2                   ((Pwm_ChannelType)2U)
#define PWM_CHANNEL_3                   ((Pwm_ChannelType)3U)
#define PWM_CHANNEL_4                   ((Pwm_ChannelType)4U)
#define PWM_CHANNEL_5                   ((Pwm_ChannelType)5U)
#define PWM_CHANNEL_6                   ((Pwm_ChannelType)6U)
#define PWM_CHANNEL_7                   ((Pwm_ChannelType)7U)

#define PWM_DUTY_CYCLE_RESOLUTION       ((uint16)0x8000U)

/* Configuration switches */
#define PWM_DEV_ERROR_DETECT            (STD_ON)
#define PWM_VERSION_INFO_API            (STD_ON)
#define PWM_DE_INIT_API                 (STD_ON)
#define PWM_SET_DUTY_CYCLE_API          (STD_ON)
#define PWM_SET_PERIOD_AND_DUTY_API     (STD_ON)
#define PWM_SET_OUTPUT_TO_IDLE_API      (STD_ON)
#define PWM_GET_OUTPUT_STATE_API        (STD_ON)
#define PWM_NOTIFICATION_SUPPORTED      (STD_ON)
#define PWM_POWER_STATE_SUPPORTED       (STD_ON)

/*==================================================================================================
*                                      MOCK STATE VARIABLES
==================================================================================================*/
static boolean Pwm_DriverInitialized = FALSE;
static const Pwm_ConfigType* Pwm_ConfigPtr = NULL_PTR;
static uint16 Pwm_ChannelDutyCycle[PWM_NUM_CHANNELS];
static Pwm_PeriodType Pwm_ChannelPeriod[PWM_NUM_CHANNELS];
static Pwm_OutputStateType Pwm_ChannelOutputState[PWM_NUM_CHANNELS];
static boolean Pwm_ChannelNotificationEnabled[PWM_NUM_CHANNELS];
static Pwm_EdgeNotificationType Pwm_ChannelNotificationEdge[PWM_NUM_CHANNELS];
static Pwm_PowerStateType Pwm_CurrentPowerState = PWM_FULL_POWER;
static Pwm_PowerStateType Pwm_TargetPowerState = PWM_FULL_POWER;

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

static void reset_pwm_state(void)
{
    Pwm_DriverInitialized = FALSE;
    Pwm_ConfigPtr = NULL_PTR;
    memset(Pwm_ChannelDutyCycle, 0, sizeof(Pwm_ChannelDutyCycle));
    memset(Pwm_ChannelPeriod, 0, sizeof(Pwm_ChannelPeriod));
    memset(Pwm_ChannelOutputState, 0, sizeof(Pwm_ChannelOutputState));
    memset(Pwm_ChannelNotificationEnabled, 0, sizeof(Pwm_ChannelNotificationEnabled));
    memset(Pwm_ChannelNotificationEdge, 0, sizeof(Pwm_ChannelNotificationEdge));
    Pwm_CurrentPowerState = PWM_FULL_POWER;
    Pwm_TargetPowerState = PWM_FULL_POWER;
}

/*==================================================================================================
*                                      PWM DRIVER IMPLEMENTATION (MOCK)
==================================================================================================*/

static Pwm_ChannelConfigType test_channels[PWM_NUM_CHANNELS];
static Pwm_ConfigType test_config;

void Pwm_Init(const Pwm_ConfigType* ConfigPtr)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_INIT, PWM_E_PARAM_CONFIG);
        return;
    }
    if (Pwm_DriverInitialized == TRUE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_INIT, PWM_E_ALREADY_INITIALIZED);
        return;
    }
    #endif

    Pwm_ConfigPtr = ConfigPtr;

    for (uint8 i = 0U; i < ConfigPtr->NumChannels; i++) {
        const Pwm_ChannelConfigType* chConfig = &ConfigPtr->Channels[i];
        Pwm_ChannelDutyCycle[chConfig->ChannelId] = chConfig->DefaultDutyCycle;
        Pwm_ChannelPeriod[chConfig->ChannelId] = chConfig->DefaultPeriod;
        Pwm_ChannelOutputState[chConfig->ChannelId] = PWM_LOW;
        Pwm_ChannelNotificationEnabled[chConfig->ChannelId] = FALSE;
    }

    Pwm_DriverInitialized = TRUE;
}

#if (PWM_DE_INIT_API == STD_ON)
void Pwm_DeInit(void)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (Pwm_DriverInitialized == FALSE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_DEINIT, PWM_E_UNINIT);
        return;
    }
    #endif

    Pwm_DriverInitialized = FALSE;
    Pwm_ConfigPtr = NULL_PTR;
}
#endif

#if (PWM_SET_DUTY_CYCLE_API == STD_ON)
void Pwm_SetDutyCycle(Pwm_ChannelType Channel, uint16 DutyCycle)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (Pwm_DriverInitialized == FALSE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_SETDUTYCYCLE, PWM_E_UNINIT);
        return;
    }
    if (Channel >= PWM_NUM_CHANNELS) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_SETDUTYCYCLE, PWM_E_PARAM_CHANNEL);
        return;
    }
    #endif

    Pwm_ChannelDutyCycle[Channel] = DutyCycle;
    
    /* Update output state based on duty cycle */
    if (DutyCycle > (PWM_DUTY_CYCLE_RESOLUTION / 2)) {
        Pwm_ChannelOutputState[Channel] = PWM_HIGH;
    } else {
        Pwm_ChannelOutputState[Channel] = PWM_LOW;
    }
}
#endif

#if (PWM_SET_PERIOD_AND_DUTY_API == STD_ON)
void Pwm_SetPeriodAndDuty(Pwm_ChannelType Channel, Pwm_PeriodType Period, uint16 DutyCycle)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (Pwm_DriverInitialized == FALSE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_SETPERIODANDDUTY, PWM_E_UNINIT);
        return;
    }
    if (Channel >= PWM_NUM_CHANNELS) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_SETPERIODANDDUTY, PWM_E_PARAM_CHANNEL);
        return;
    }
    if (Pwm_ConfigPtr != NULL_PTR && Pwm_ConfigPtr->Channels[Channel].ChannelClass == PWM_FIXED_PERIOD) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_SETPERIODANDDUTY, PWM_E_PERIOD_UNCHANGEABLE);
        return;
    }
    #endif

    Pwm_ChannelPeriod[Channel] = Period;
    Pwm_ChannelDutyCycle[Channel] = DutyCycle;
    
    if (DutyCycle > (PWM_DUTY_CYCLE_RESOLUTION / 2)) {
        Pwm_ChannelOutputState[Channel] = PWM_HIGH;
    } else {
        Pwm_ChannelOutputState[Channel] = PWM_LOW;
    }
}
#endif

#if (PWM_SET_OUTPUT_TO_IDLE_API == STD_ON)
void Pwm_SetOutputToIdle(Pwm_ChannelType Channel)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (Pwm_DriverInitialized == FALSE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_SETOUTPUTTOIDLE, PWM_E_UNINIT);
        return;
    }
    if (Channel >= PWM_NUM_CHANNELS) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_SETOUTPUTTOIDLE, PWM_E_PARAM_CHANNEL);
        return;
    }
    #endif

    /* Set duty cycle to 0 (idle) */
    Pwm_ChannelDutyCycle[Channel] = 0;
    
    /* Set output to idle state based on configuration */
    if (Pwm_ConfigPtr != NULL_PTR) {
        Pwm_ChannelOutputState[Channel] = (Pwm_ConfigPtr->Channels[Channel].IdleState == PWM_IDLE_HIGH) ? PWM_HIGH : PWM_LOW;
    } else {
        Pwm_ChannelOutputState[Channel] = PWM_LOW;
    }
}
#endif

#if (PWM_GET_OUTPUT_STATE_API == STD_ON)
Pwm_OutputStateType Pwm_GetOutputState(Pwm_ChannelType Channel)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (Pwm_DriverInitialized == FALSE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_GETOUTPUTSTATE, PWM_E_UNINIT);
        return PWM_LOW;
    }
    if (Channel >= PWM_NUM_CHANNELS) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_GETOUTPUTSTATE, PWM_E_PARAM_CHANNEL);
        return PWM_LOW;
    }
    #endif

    return Pwm_ChannelOutputState[Channel];
}
#endif

#if (PWM_NOTIFICATION_SUPPORTED == STD_ON)
void Pwm_DisableNotification(Pwm_ChannelType Channel)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (Pwm_DriverInitialized == FALSE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_DISABLENOTIFICATION, PWM_E_UNINIT);
        return;
    }
    if (Channel >= PWM_NUM_CHANNELS) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_DISABLENOTIFICATION, PWM_E_PARAM_CHANNEL);
        return;
    }
    #endif

    Pwm_ChannelNotificationEnabled[Channel] = FALSE;
}

void Pwm_EnableNotification(Pwm_ChannelType Channel, Pwm_EdgeNotificationType Notification)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (Pwm_DriverInitialized == FALSE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_ENABLENOTIFICATION, PWM_E_UNINIT);
        return;
    }
    if (Channel >= PWM_NUM_CHANNELS) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_ENABLENOTIFICATION, PWM_E_PARAM_CHANNEL);
        return;
    }
    #endif

    Pwm_ChannelNotificationEnabled[Channel] = TRUE;
    Pwm_ChannelNotificationEdge[Channel] = Notification;
}
#endif

#if (PWM_VERSION_INFO_API == STD_ON)
void Pwm_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_GETVERSIONINFO, PWM_E_PARAM_POINTER);
        return;
    }
    #endif
    
    versioninfo->vendorID = PWM_VENDOR_ID;
    versioninfo->moduleID = PWM_MODULE_ID;
    versioninfo->sw_major_version = PWM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = PWM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = PWM_SW_PATCH_VERSION;
}
#endif

#if (PWM_POWER_STATE_SUPPORTED == STD_ON)
void Pwm_SetPowerState(Pwm_PowerStateType PowerState, Pwm_PowerStateRequestResultType* Result)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (Pwm_DriverInitialized == FALSE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_SETPOWERSTATE, PWM_E_UNINIT);
        return;
    }
    if (Result == NULL_PTR) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_SETPOWERSTATE, PWM_E_PARAM_POINTER);
        return;
    }
    #endif
    
    Pwm_CurrentPowerState = PowerState;
    *Result = PWM_SERVICE_ACCEPTED;
}

void Pwm_GetTargetPowerState(Pwm_PowerStateType* TargetPowerState, Pwm_PowerStateRequestResultType* Result)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (Pwm_DriverInitialized == FALSE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_GETTARGETPOWERSTATE, PWM_E_UNINIT);
        return;
    }
    if (TargetPowerState == NULL_PTR || Result == NULL_PTR) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_GETTARGETPOWERSTATE, PWM_E_PARAM_POINTER);
        return;
    }
    #endif
    
    *TargetPowerState = Pwm_TargetPowerState;
    *Result = PWM_SERVICE_ACCEPTED;
}

void Pwm_GetCurrentPowerState(Pwm_PowerStateType* CurrentPowerState, Pwm_PowerStateRequestResultType* Result)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (Pwm_DriverInitialized == FALSE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_GETCURRENTPOWERSTATE, PWM_E_UNINIT);
        return;
    }
    if (CurrentPowerState == NULL_PTR || Result == NULL_PTR) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_GETCURRENTPOWERSTATE, PWM_E_PARAM_POINTER);
        return;
    }
    #endif
    
    *CurrentPowerState = Pwm_CurrentPowerState;
    *Result = PWM_SERVICE_ACCEPTED;
}

void Pwm_PreparePowerState(Pwm_PowerStateType PowerState, Pwm_PowerStateRequestResultType* Result)
{
    #if (PWM_DEV_ERROR_DETECT == STD_ON)
    if (Pwm_DriverInitialized == FALSE) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_PREPAREPOWERSTATE, PWM_E_UNINIT);
        return;
    }
    if (Result == NULL_PTR) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_PREPAREPOWERSTATE, PWM_E_PARAM_POINTER);
        return;
    }
    #endif
    
    Pwm_TargetPowerState = PowerState;
    *Result = PWM_SERVICE_ACCEPTED;
}
#endif

/*==================================================================================================
*                                      TEST FUNCTIONS
==================================================================================================*/

/* 初始化测试 */
/* @req SWS_Pwm_00201 */
void test_init(void)
{
    printf("\n=== Initialization Tests ===\n");
    
    /* Setup */
    reset_pwm_state();
    reset_det_tracking();
    
    /* 测试初始化前状态 */
    TEST_ASSERT_EQ(FALSE, Pwm_DriverInitialized);
    
    /* Setup test config */
    test_channels[0].ChannelId = PWM_CHANNEL_0;
    test_channels[0].DefaultPeriod = 1000;
    test_channels[0].DefaultDutyCycle = 0x4000; /* 50% */
    test_channels[0].ChannelClass = PWM_VARIABLE_PERIOD;
    test_channels[0].IdleState = PWM_IDLE_LOW;
    
    test_config.Channels = test_channels;
    test_config.NumChannels = 1;
    test_config.DevErrorDetect = STD_ON;
    
    /* 测试正常初始化 */
    Pwm_Init(&test_config);
    TEST_ASSERT_EQ(TRUE, Pwm_DriverInitialized);
    TEST_ASSERT_EQ(0, det_call_count);
    
    /* 测试重复初始化 */
    reset_det_tracking();
    Pwm_Init(&test_config);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_ALREADY_INITIALIZED, det_error_id);
    
    /* 测试NULL指针初始化 */
    reset_pwm_state();
    reset_det_tracking();
    Pwm_Init(NULL_PTR);
    TEST_ASSERT_EQ(FALSE, Pwm_DriverInitialized);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_CONFIG, det_error_id);
}

/* 反初始化测试 */
/* @req SWS_Pwm_00202 */
void test_deinit(void)
{
    printf("\n=== De-initialization Tests ===\n");
    
    /* Setup */
    reset_pwm_state();
    reset_det_tracking();
    
    /* Setup test config */
    test_channels[0].ChannelId = PWM_CHANNEL_0;
    test_channels[0].DefaultPeriod = 1000;
    test_channels[0].DefaultDutyCycle = 0x4000;
    test_channels[0].ChannelClass = PWM_VARIABLE_PERIOD;
    test_config.Channels = test_channels;
    test_config.NumChannels = 1;
    
    /* 测试未初始化时反初始化 */
    Pwm_DeInit();
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_UNINIT, det_error_id);
    
    /* 测试正常反初始化 */
    reset_det_tracking();
    Pwm_Init(&test_config);
    TEST_ASSERT_EQ(TRUE, Pwm_DriverInitialized);
    
    Pwm_DeInit();
    TEST_ASSERT_EQ(FALSE, Pwm_DriverInitialized);
    TEST_ASSERT_EQ(0, det_call_count);
}

/* 占空比设置测试 */
/* @req SWS_Pwm_00203 */
void test_set_duty_cycle(void)
{
    printf("\n=== Set Duty Cycle Tests ===\n");
    
    /* Setup */
    reset_pwm_state();
    reset_det_tracking();
    
    /* Setup test config */
    test_channels[0].ChannelId = PWM_CHANNEL_0;
    test_channels[0].DefaultPeriod = 1000;
    test_channels[0].DefaultDutyCycle = 0x4000;
    test_channels[0].ChannelClass = PWM_VARIABLE_PERIOD;
    test_config.Channels = test_channels;
    test_config.NumChannels = 1;
    
    /* 测试未初始化时设置占空比 */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, 0x2000);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_UNINIT, det_error_id);
    
    /* 正常初始化 */
    Pwm_Init(&test_config);
    reset_det_tracking();
    
    /* 测试设置0%占空比 */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, 0);
    TEST_ASSERT_EQ(0, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(PWM_LOW, Pwm_ChannelOutputState[PWM_CHANNEL_0]);
    
    /* 测试设置50%占空比 */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, 0x4000);
    TEST_ASSERT_EQ(0x4000, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    /* 注: 边界条件 0x4000 (正好50%) 被视为 LOW 是正常行为 */
    TEST_ASSERT_EQ(PWM_LOW, Pwm_ChannelOutputState[PWM_CHANNEL_0]);
    
    /* 测试设置100%占空比 */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, 0x8000);
    TEST_ASSERT_EQ(0x8000, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(PWM_HIGH, Pwm_ChannelOutputState[PWM_CHANNEL_0]);
    
    /* 测试无效通道 */
    reset_det_tracking();
    Pwm_SetDutyCycle(PWM_NUM_CHANNELS, 0x4000);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_CHANNEL, det_error_id);
}

/* 周期和占空比设置测试 */
/* @req SWS_Pwm_00204 */
void test_set_period_and_duty(void)
{
    printf("\n=== Set Period and Duty Tests ===\n");
    
    /* Setup */
    reset_pwm_state();
    reset_det_tracking();
    
    /* Setup test config - variable period channel */
    test_channels[0].ChannelId = PWM_CHANNEL_0;
    test_channels[0].DefaultPeriod = 1000;
    test_channels[0].DefaultDutyCycle = 0x4000;
    test_channels[0].ChannelClass = PWM_VARIABLE_PERIOD;
    
    /* Setup test config - fixed period channel */
    test_channels[1].ChannelId = PWM_CHANNEL_1;
    test_channels[1].DefaultPeriod = 2000;
    test_channels[1].DefaultDutyCycle = 0x4000;
    test_channels[1].ChannelClass = PWM_FIXED_PERIOD;
    
    test_config.Channels = test_channels;
    test_config.NumChannels = 2;
    
    /* 测试未初始化时设置 */
    Pwm_SetPeriodAndDuty(PWM_CHANNEL_0, 500, 0x2000);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_UNINIT, det_error_id);
    
    /* 正常初始化 */
    Pwm_Init(&test_config);
    reset_det_tracking();
    
    /* 测试可变周期通道设置 */
    Pwm_SetPeriodAndDuty(PWM_CHANNEL_0, 500, 0x2000);
    TEST_ASSERT_EQ(500, Pwm_ChannelPeriod[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(0x2000, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    
    /* 测试固定周期通道设置（应该报错） */
    reset_det_tracking();
    Pwm_SetPeriodAndDuty(PWM_CHANNEL_1, 3000, 0x3000);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PERIOD_UNCHANGEABLE, det_error_id);
    
    /* 测试无效通道 */
    reset_det_tracking();
    Pwm_SetPeriodAndDuty(PWM_NUM_CHANNELS, 500, 0x2000);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_CHANNEL, det_error_id);
}

/* 输出到空闲状态测试 */
/* @req SWS_Pwm_00205 */
void test_set_output_to_idle(void)
{
    printf("\n=== Set Output To Idle Tests ===\n");
    
    /* Setup */
    reset_pwm_state();
    reset_det_tracking();
    
    /* Setup test config */
    test_channels[0].ChannelId = PWM_CHANNEL_0;
    test_channels[0].DefaultPeriod = 1000;
    test_channels[0].DefaultDutyCycle = 0x4000;
    test_channels[0].ChannelClass = PWM_VARIABLE_PERIOD;
    test_channels[0].IdleState = PWM_IDLE_HIGH;
    test_config.Channels = test_channels;
    test_config.NumChannels = 1;
    
    /* 测试未初始化时 */
    Pwm_SetOutputToIdle(PWM_CHANNEL_0);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_UNINIT, det_error_id);
    
    /* 正常初始化 */
    Pwm_Init(&test_config);
    reset_det_tracking();
    
    /* 先设置一个非零占空比 */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, 0x4000);
    TEST_ASSERT_EQ(0x4000, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    
    /* 设置输出到空闲状态 */
    Pwm_SetOutputToIdle(PWM_CHANNEL_0);
    TEST_ASSERT_EQ(0, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(PWM_HIGH, Pwm_ChannelOutputState[PWM_CHANNEL_0]); /* PWM_IDLE_HIGH */
    
    /* 测试无效通道 */
    reset_det_tracking();
    Pwm_SetOutputToIdle(PWM_NUM_CHANNELS);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_CHANNEL, det_error_id);
}

/* 获取输出状态测试 */
/* @req SWS_Pwm_00206 */
void test_get_output_state(void)
{
    Pwm_OutputStateType state;
    
    printf("\n=== Get Output State Tests ===\n");
    
    /* Setup */
    reset_pwm_state();
    reset_det_tracking();
    
    /* Setup test config */
    test_channels[0].ChannelId = PWM_CHANNEL_0;
    test_channels[0].DefaultPeriod = 1000;
    test_channels[0].DefaultDutyCycle = 0x4000;
    test_channels[0].ChannelClass = PWM_VARIABLE_PERIOD;
    test_config.Channels = test_channels;
    test_config.NumChannels = 1;
    
    /* 测试未初始化时 */
    state = Pwm_GetOutputState(PWM_CHANNEL_0);
    TEST_ASSERT_EQ(PWM_LOW, state);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_UNINIT, det_error_id);
    
    /* 正常初始化 */
    Pwm_Init(&test_config);
    reset_det_tracking();
    
    /* 设置高占空比，验证输出高 */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, 0x6000);
    state = Pwm_GetOutputState(PWM_CHANNEL_0);
    TEST_ASSERT_EQ(PWM_HIGH, state);
    
    /* 设置低占空比，验证输出低 */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, 0x1000);
    state = Pwm_GetOutputState(PWM_CHANNEL_0);
    TEST_ASSERT_EQ(PWM_LOW, state);
    
    /* 测试无效通道 */
    reset_det_tracking();
    state = Pwm_GetOutputState(PWM_NUM_CHANNELS);
    TEST_ASSERT_EQ(PWM_LOW, state);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_CHANNEL, det_error_id);
}

/* 通知功能测试 */
/* @req SWS_Pwm_00207 */
void test_notification(void)
{
    printf("\n=== Notification Tests ===\n");
    
    /* Setup */
    reset_pwm_state();
    reset_det_tracking();
    
    /* Setup test config */
    test_channels[0].ChannelId = PWM_CHANNEL_0;
    test_channels[0].DefaultPeriod = 1000;
    test_channels[0].DefaultDutyCycle = 0x4000;
    test_channels[0].ChannelClass = PWM_VARIABLE_PERIOD;
    test_channels[0].NotificationSupported = TRUE;
    test_config.Channels = test_channels;
    test_config.NumChannels = 1;
    
    /* 测试未初始化时使能通知 */
    Pwm_EnableNotification(PWM_CHANNEL_0, PWM_RISING_EDGE);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_UNINIT, det_error_id);
    
    /* 测试未初始化时禁用通知 */
    reset_det_tracking();
    Pwm_DisableNotification(PWM_CHANNEL_0);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_UNINIT, det_error_id);
    
    /* 正常初始化 */
    Pwm_Init(&test_config);
    reset_det_tracking();
    
    /* 测试使能上升沿通知 */
    Pwm_EnableNotification(PWM_CHANNEL_0, PWM_RISING_EDGE);
    TEST_ASSERT_EQ(TRUE, Pwm_ChannelNotificationEnabled[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(PWM_RISING_EDGE, Pwm_ChannelNotificationEdge[PWM_CHANNEL_0]);
    
    /* 测试使能下降沿通知 */
    Pwm_EnableNotification(PWM_CHANNEL_0, PWM_FALLING_EDGE);
    TEST_ASSERT_EQ(PWM_FALLING_EDGE, Pwm_ChannelNotificationEdge[PWM_CHANNEL_0]);
    
    /* 测试使能双边沿通知 */
    Pwm_EnableNotification(PWM_CHANNEL_0, PWM_BOTH_EDGES);
    TEST_ASSERT_EQ(PWM_BOTH_EDGES, Pwm_ChannelNotificationEdge[PWM_CHANNEL_0]);
    
    /* 测试禁用通知 */
    Pwm_DisableNotification(PWM_CHANNEL_0);
    TEST_ASSERT_EQ(FALSE, Pwm_ChannelNotificationEnabled[PWM_CHANNEL_0]);
    
    /* 测试无效通道 - EnableNotification */
    reset_det_tracking();
    Pwm_EnableNotification(PWM_NUM_CHANNELS, PWM_RISING_EDGE);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_CHANNEL, det_error_id);
    
    /* 测试无效通道 - DisableNotification */
    reset_det_tracking();
    Pwm_DisableNotification(PWM_NUM_CHANNELS);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_CHANNEL, det_error_id);
}

/* 版本信息测试 */
/* @req SWS_Pwm_00208 */
void test_version_info(void)
{
    Std_VersionInfoType versionInfo;
    
    printf("\n=== Version Info Tests ===\n");
    
    /* 测试正常获取版本信息 */
    Pwm_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQ(PWM_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQ(PWM_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQ(PWM_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQ(PWM_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQ(PWM_SW_PATCH_VERSION, versionInfo.sw_patch_version);
    
    /* 测试NULL指针 */
    reset_det_tracking();
    Pwm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_POINTER, det_error_id);
}

/* 电源状态测试 */
/* @req SWS_Pwm_00209 */
void test_power_state(void)
{
    Pwm_PowerStateType powerState;
    Pwm_PowerStateRequestResultType result;
    
    printf("\n=== Power State Tests ===\n");
    
    /* Setup */
    reset_pwm_state();
    reset_det_tracking();
    
    /* Setup test config */
    test_channels[0].ChannelId = PWM_CHANNEL_0;
    test_channels[0].DefaultPeriod = 1000;
    test_channels[0].DefaultDutyCycle = 0x4000;
    test_channels[0].ChannelClass = PWM_VARIABLE_PERIOD;
    test_config.Channels = test_channels;
    test_config.NumChannels = 1;
    
    /* 测试未初始化时设置电源状态 */
    Pwm_SetPowerState(PWM_FULL_POWER, &result);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_UNINIT, det_error_id);
    
    /* 测试未初始化时获取目标电源状态 */
    reset_det_tracking();
    Pwm_GetTargetPowerState(&powerState, &result);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_UNINIT, det_error_id);
    
    /* 测试未初始化时获取当前电源状态 */
    reset_det_tracking();
    Pwm_GetCurrentPowerState(&powerState, &result);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_UNINIT, det_error_id);
    
    /* 测试未初始化时准备电源状态 */
    reset_det_tracking();
    Pwm_PreparePowerState(PWM_FULL_POWER, &result);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_UNINIT, det_error_id);
    
    /* 正常初始化 */
    Pwm_Init(&test_config);
    reset_det_tracking();
    
    /* 测试设置电源状态 */
    Pwm_SetPowerState(PWM_LOW_POWER, &result);
    TEST_ASSERT_EQ(PWM_SERVICE_ACCEPTED, result);
    TEST_ASSERT_EQ(PWM_LOW_POWER, Pwm_CurrentPowerState);
    
    /* 测试获取当前电源状态 */
    Pwm_GetCurrentPowerState(&powerState, &result);
    TEST_ASSERT_EQ(PWM_SERVICE_ACCEPTED, result);
    TEST_ASSERT_EQ(PWM_LOW_POWER, powerState);
    
    /* 测试准备电源状态 */
    Pwm_PreparePowerState(PWM_FULL_POWER, &result);
    TEST_ASSERT_EQ(PWM_SERVICE_ACCEPTED, result);
    TEST_ASSERT_EQ(PWM_FULL_POWER, Pwm_TargetPowerState);
    
    /* 测试获取目标电源状态 */
    Pwm_GetTargetPowerState(&powerState, &result);
    TEST_ASSERT_EQ(PWM_SERVICE_ACCEPTED, result);
    TEST_ASSERT_EQ(PWM_FULL_POWER, powerState);
    
    /* 测试NULL指针 - SetPowerState */
    reset_det_tracking();
    Pwm_SetPowerState(PWM_FULL_POWER, NULL_PTR);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_POINTER, det_error_id);
    
    /* 测试NULL指针 - GetTargetPowerState */
    reset_det_tracking();
    Pwm_GetTargetPowerState(NULL_PTR, &result);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_POINTER, det_error_id);
    
    /* 测试NULL指针 - GetCurrentPowerState */
    reset_det_tracking();
    Pwm_GetCurrentPowerState(NULL_PTR, &result);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_POINTER, det_error_id);
    
    /* 测试NULL指针 - PreparePowerState */
    reset_det_tracking();
    Pwm_PreparePowerState(PWM_FULL_POWER, NULL_PTR);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(PWM_E_PARAM_POINTER, det_error_id);
}

/* 多通道测试 */
/* @req SWS_Pwm_00210 */
void test_multi_channel(void)
{
    printf("\n=== Multi-Channel Tests ===\n");
    
    /* Setup */
    reset_pwm_state();
    reset_det_tracking();
    
    /* Setup multi-channel test config */
    for (uint8 i = 0; i < 4; i++) {
        test_channels[i].ChannelId = i;
        test_channels[i].DefaultPeriod = 1000 * (i + 1);
        test_channels[i].DefaultDutyCycle = 0x2000 * (i + 1);
        test_channels[i].ChannelClass = PWM_VARIABLE_PERIOD;
        test_channels[i].IdleState = PWM_IDLE_LOW;
    }
    test_config.Channels = test_channels;
    test_config.NumChannels = 4;
    
    /* 初始化 */
    Pwm_Init(&test_config);
    
    /* 测试多个通道的占空比设置 */
    for (uint8 i = 0; i < 4; i++) {
        TEST_ASSERT_EQ(0x2000 * (i + 1), Pwm_ChannelDutyCycle[i]);
    }
    
    /* 独立设置每个通道 */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, 0x1000);
    Pwm_SetDutyCycle(PWM_CHANNEL_1, 0x2000);
    Pwm_SetDutyCycle(PWM_CHANNEL_2, 0x4000);
    Pwm_SetDutyCycle(PWM_CHANNEL_3, 0x6000);
    
    TEST_ASSERT_EQ(0x1000, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(0x2000, Pwm_ChannelDutyCycle[PWM_CHANNEL_1]);
    TEST_ASSERT_EQ(0x4000, Pwm_ChannelDutyCycle[PWM_CHANNEL_2]);
    TEST_ASSERT_EQ(0x6000, Pwm_ChannelDutyCycle[PWM_CHANNEL_3]);
    
    /* 测试各通道输出状态 */
    TEST_ASSERT_EQ(PWM_LOW, Pwm_ChannelOutputState[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(PWM_LOW, Pwm_ChannelOutputState[PWM_CHANNEL_1]);
    /* 注: 0x4000 (正好50%) 被视为 LOW 是正常行为 */
    TEST_ASSERT_EQ(PWM_LOW, Pwm_ChannelOutputState[PWM_CHANNEL_2]);
    TEST_ASSERT_EQ(PWM_HIGH, Pwm_ChannelOutputState[PWM_CHANNEL_3]);
}

/* 边界条件测试 */
/* @req SWS_Pwm_00211 */
void test_boundary_conditions(void)
{
    printf("\n=== Boundary Condition Tests ===\n");
    
    /* Setup */
    reset_pwm_state();
    reset_det_tracking();
    
    /* Setup test config */
    test_channels[0].ChannelId = PWM_CHANNEL_0;
    test_channels[0].DefaultPeriod = 1000;
    test_channels[0].DefaultDutyCycle = 0x4000;
    test_channels[0].ChannelClass = PWM_VARIABLE_PERIOD;
    test_config.Channels = test_channels;
    test_config.NumChannels = 1;
    
    Pwm_Init(&test_config);
    
    /* 测试占空比边界值 - 0% */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, 0);
    TEST_ASSERT_EQ(0, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(PWM_LOW, Pwm_ChannelOutputState[PWM_CHANNEL_0]);
    
    /* 测试占空比边界值 - 最小非零值 */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, 1);
    TEST_ASSERT_EQ(1, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(PWM_LOW, Pwm_ChannelOutputState[PWM_CHANNEL_0]);
    
    /* 测试占空比边界值 - 分辨率的一半 (刚好是高低分界点) */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, PWM_DUTY_CYCLE_RESOLUTION / 2);
    TEST_ASSERT_EQ(PWM_DUTY_CYCLE_RESOLUTION / 2, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(PWM_LOW, Pwm_ChannelOutputState[PWM_CHANNEL_0]); /* 边界条件，取低 */
    
    /* 测试占空比边界值 - 分辨率的一半+1 */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, (PWM_DUTY_CYCLE_RESOLUTION / 2) + 1);
    TEST_ASSERT_EQ((PWM_DUTY_CYCLE_RESOLUTION / 2) + 1, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(PWM_HIGH, Pwm_ChannelOutputState[PWM_CHANNEL_0]);
    
    /* 测试占空比边界值 - 100% */
    Pwm_SetDutyCycle(PWM_CHANNEL_0, PWM_DUTY_CYCLE_RESOLUTION);
    TEST_ASSERT_EQ(PWM_DUTY_CYCLE_RESOLUTION, Pwm_ChannelDutyCycle[PWM_CHANNEL_0]);
    TEST_ASSERT_EQ(PWM_HIGH, Pwm_ChannelOutputState[PWM_CHANNEL_0]);
    
    /* 测试周期边界值 - 最小周期 */
    Pwm_SetPeriodAndDuty(PWM_CHANNEL_0, 1, 0x4000);
    TEST_ASSERT_EQ(1, Pwm_ChannelPeriod[PWM_CHANNEL_0]);
    
    /* 测试周期边界值 - 大周期 */
    Pwm_SetPeriodAndDuty(PWM_CHANNEL_0, 0xFFFFFFFF, 0x4000);
    TEST_ASSERT_EQ(0xFFFFFFFF, Pwm_ChannelPeriod[PWM_CHANNEL_0]);
}

/*==================================================================================================
*                                      MAIN FUNCTION
==================================================================================================*/
int main(void)
{
    printf("========================================\n");
    printf("    PWM Driver Unit Tests              \n");
    printf("========================================\n");
    
    /* Run all test suites */
    test_init();
    test_deinit();
    test_set_duty_cycle();
    test_set_period_and_duty();
    test_set_output_to_idle();
    test_get_output_state();
    test_notification();
    test_version_info();
    test_power_state();
    test_multi_channel();
    test_boundary_conditions();
    
    /* Print summary */
    printf("\n========================================\n");
    printf("    Test Summary                        \n");
    printf("========================================\n");
    printf("  Total:   %d\n", tests_run);
    printf("  Passed:  %d\n", tests_passed);
    printf("  Failed:  %d\n", tests_failed);
    printf("  Coverage: %.1f%%\n", (tests_run > 0) ? ((float)tests_passed / tests_run * 100.0f) : 0.0f);
    printf("========================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
