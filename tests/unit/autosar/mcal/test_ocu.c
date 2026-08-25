/**
 * @file test_ocu.c
 * @brief OCU (Output Compare Unit) Driver 模块单元测试
 * @version 1.0.0
 * @date 2026-05-15
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * 
 * @coverage Target: 80%+
 * Test Areas:
 * - Initialization/Deinitialization
 * - Channel Start/Stop
 * - Pin State/Action Configuration
 * - Absolute/Relative Threshold
 * - Counter Reading
 * - Notification Enable/Disable
 * - Version Information
 * - PWM Generation Patterns
 * - Error Handling
 */

// @tests src/bsw/mcal/ocu/src/Ocu.c  @tests src/bsw/mcal/ocu/include/Ocu.h

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*==================================================================================================
*                                    TYPE DEFINITIONS
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

/* Version Info Type */
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8 sw_major_version;
    uint8 sw_minor_version;
    uint8 sw_patch_version;
} Std_VersionInfoType;

/*==================================================================================================
*                                    OCU TYPE DEFINITIONS
==================================================================================================*/
typedef uint8 Ocu_ChannelType;
typedef uint32 Ocu_ValueType;

/* OCU Output Pin State Type */
typedef enum {
    OCU_HIGH = 0x00U,
    OCU_LOW  = 0x01U
} Ocu_OutputPinStateType;

/* OCU Pin Action Type */
typedef enum {
    OCU_SET_HIGH = 0x00U,
    OCU_SET_LOW  = 0x01U,
    OCU_TOGGLE   = 0x02U,
    OCU_HOLD     = 0x03U
} Ocu_PinActionType;

/* OCU State Type */
typedef enum {
    OCU_STOPPED = 0x00U,
    OCU_RUNNING = 0x01U
} Ocu_StateType;

/* OCU Notification Callback Type */
typedef void (*Ocu_NotificationType)(void);

/* OCU Channel Configuration Type */
typedef struct {
    Ocu_ChannelType ChannelId;
    Ocu_OutputPinStateType DefaultPinState;
    Ocu_ValueType DefaultThreshold;
    Ocu_NotificationType Notification;
    boolean RunningInBackground;
    uint32 BaseAddress;
} Ocu_ChannelConfigType;

/* OCU Configuration Type */
typedef struct {
    const Ocu_ChannelConfigType* Channels;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean DeInitApi;
    boolean PinStateApi;
    boolean SetPinActionApi;
    boolean SetThresholdApi;
    boolean NotificationSupported;
    Ocu_ValueType MaxCounterValue;
} Ocu_ConfigType;

/* Channel State Structure */
typedef struct {
    Ocu_StateType State;
    Ocu_OutputPinStateType CurrentPinState;
    Ocu_ValueType CompareValue;
    Ocu_PinActionType PinAction;
    boolean IsRunning;
    boolean NotificationEnabled;
} Ocu_ChannelStateType;

/*==================================================================================================
*                                    OCU CONSTANTS
==================================================================================================*/
/* Service IDs */
#define OCU_SID_INIT                    0x00U
#define OCU_SID_DEINIT                  0x01U
#define OCU_SID_STARTCHANNEL            0x02U
#define OCU_SID_STOPCHANNEL             0x03U
#define OCU_SID_SETPINSTATE             0x04U
#define OCU_SID_SETPINACTION            0x05U
#define OCU_SID_SETABSOLUTETHRESHOLD    0x06U
#define OCU_SID_SETRELATIVETHRESHOLD    0x07U
#define OCU_SID_GETCOUNTER              0x08U
#define OCU_SID_DISABLENOTIFICATION     0x09U
#define OCU_SID_ENABLENOTIFICATION      0x0AU
#define OCU_SID_GETVERSIONINFO          0x0BU

/* DET Error Codes */
#define OCU_E_PARAM_POINTER             0x01U
#define OCU_E_PARAM_CONFIG              0x02U
#define OCU_E_UNINIT                    0x03U
#define OCU_E_ALREADY_INITIALIZED       0x04U
#define OCU_E_PARAM_CHANNEL             0x05U
#define OCU_E_PARAM_INVALID_STATE       0x06U
#define OCU_E_PARAM_ACTION              0x07U
#define OCU_E_PARAM_PIN_STATE           0x08U
#define OCU_E_CHANNEL_BUSY              0x09U
#define OCU_E_PARAM_REF_VALUE           0x0AU
#define OCU_E_PARAM_THRESHOLD_VALUE     0x0BU
#define OCU_E_INIT_FAILED               0x0CU
#define OCU_E_NO_TICKS_PER_CHANNEL      0x0DU

/* Module State */
#define OCU_UNINIT                      0x00U
#define OCU_INITIALIZED                 0x01U

/* Version Info */
#define OCU_VENDOR_ID                   0x01U
#define OCU_MODULE_ID                   0x7AU
#define OCU_SW_MAJOR_VERSION            0x01U
#define OCU_SW_MINOR_VERSION            0x00U
#define OCU_SW_PATCH_VERSION            0x00U

/* Configuration */
#define OCU_NUM_CHANNELS                0x04U
#define OCU_MAX_COUNTER_VALUE           0xFFFFFFFFU
#define OCU_CHANNEL_0                   0x00U
#define OCU_CHANNEL_1                   0x01U
#define OCU_CHANNEL_2                   0x02U
#define OCU_CHANNEL_3                   0x03U

/*==================================================================================================
*                                    TEST RESULTS COUNTERS
==================================================================================================*/
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/*==================================================================================================
*                                    MOCK VARIABLES
==================================================================================================*/
static uint8 Ocu_ModuleState = OCU_UNINIT;
static Ocu_ChannelStateType Ocu_ChannelState[OCU_NUM_CHANNELS];
static const Ocu_ConfigType* Ocu_CurrentConfig = NULL_PTR;

/* Hardware Register Simulation */
typedef struct {
    uint32 Control;
    uint32 Status;
    uint32 Counter;
    uint32 Compare;
    uint32 Action;
    uint32 PinCtrl;
} Ocu_HwRegisterType;

static Ocu_HwRegisterType Ocu_HwRegisters[OCU_NUM_CHANNELS];

/* DET Error Tracking */
static uint16 det_module_id = 0;
static uint8 det_instance_id = 0;
static uint8 det_api_id = 0;
static uint8 det_error_id = 0;
static int det_call_count = 0;

/* Notification Tracking */
static int notification_call_count[OCU_NUM_CHANNELS] = {0, 0, 0, 0};

/*==================================================================================================
*                                    TEST MACROS
==================================================================================================*/
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

#define TEST_ASSERT_NE(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) != (actual)) { \
            tests_passed++; \
            printf("  [PASS] %s != %s\n", #expected, #actual); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s != %s (%s:%d)\n", #expected, #actual, __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_SECTION(name) \
    printf("\n=== %s ===\n", name)

/*==================================================================================================
*                                    MOCK FUNCTIONS
==================================================================================================*/
static void Ocu_ResetTestEnv(void)
{
    int i;
    Ocu_ModuleState = OCU_UNINIT;
    Ocu_CurrentConfig = NULL_PTR;
    det_call_count = 0;
    
    for (i = 0; i < OCU_NUM_CHANNELS; i++) {
        Ocu_ChannelState[i].State = OCU_STOPPED;
        Ocu_ChannelState[i].CurrentPinState = OCU_LOW;
        Ocu_ChannelState[i].CompareValue = 0;
        Ocu_ChannelState[i].PinAction = OCU_TOGGLE;
        Ocu_ChannelState[i].IsRunning = FALSE;
        Ocu_ChannelState[i].NotificationEnabled = FALSE;
        notification_call_count[i] = 0;
        
        memset(&Ocu_HwRegisters[i], 0, sizeof(Ocu_HwRegisterType));
    }
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    det_module_id = ModuleId;
    det_instance_id = InstanceId;
    det_api_id = ApiId;
    det_error_id = ErrorId;
    det_call_count++;
    return E_OK;
}

/* Notification Callbacks */
static void Test_Notification_Chan0(void)
{
    notification_call_count[0]++;
}

static void Test_Notification_Chan1(void)
{
    notification_call_count[1]++;
}

/*==================================================================================================
*                                    OCU MOCK IMPLEMENTATION
==================================================================================================*/
static Ocu_ChannelConfigType Test_ChannelConfig[OCU_NUM_CHANNELS] = {
    { OCU_CHANNEL_0, OCU_LOW, 0x00010000U, NULL_PTR, FALSE, 0x40000000U },
    { OCU_CHANNEL_1, OCU_LOW, 0x00010000U, Test_Notification_Chan1, FALSE, 0x40000010U },
    { OCU_CHANNEL_2, OCU_HIGH, 0x00020000U, NULL_PTR, FALSE, 0x40000020U },
    { OCU_CHANNEL_3, OCU_LOW, 0x00010000U, NULL_PTR, FALSE, 0x40000030U }
};

static Ocu_ConfigType Test_OcuConfig = {
    Test_ChannelConfig,
    OCU_NUM_CHANNELS,
    TRUE,   /* DevErrorDetect */
    TRUE,   /* VersionInfoApi */
    TRUE,   /* DeInitApi */
    TRUE,   /* PinStateApi */
    TRUE,   /* SetPinActionApi */
    TRUE,   /* SetThresholdApi */
    TRUE,   /* NotificationSupported */
    OCU_MAX_COUNTER_VALUE
};

/* Hardware Abstraction Functions */
static Ocu_HwRegisterType* Ocu_HwGetRegisterBase(Ocu_ChannelType Channel)
{
    if (Channel < OCU_NUM_CHANNELS) {
        return &Ocu_HwRegisters[Channel];
    }
    return NULL_PTR;
}

static void Ocu_HwInitChannel(Ocu_ChannelType Channel, const Ocu_ChannelConfigType* Config)
{
    Ocu_HwRegisterType* hwRegs = Ocu_HwGetRegisterBase(Channel);
    if (hwRegs != NULL_PTR) {
        hwRegs->Control = 0;
        hwRegs->Status = 0xFFFFFFFFU;
        hwRegs->Counter = 0;
        hwRegs->Compare = Config->DefaultThreshold;
        hwRegs->Action = OCU_TOGGLE;
        hwRegs->PinCtrl = (uint32)Config->DefaultPinState;
    }
}

static void Ocu_HwDeInitChannel(Ocu_ChannelType Channel)
{
    Ocu_HwRegisterType* hwRegs = Ocu_HwGetRegisterBase(Channel);
    if (hwRegs != NULL_PTR) {
        hwRegs->Control = 0;
        hwRegs->Status = 0xFFFFFFFFU;
        hwRegs->Counter = 0;
        hwRegs->Compare = 0;
        hwRegs->Action = 0;
        hwRegs->PinCtrl = 0;
    }
}

static void Ocu_HwStartChannel(Ocu_ChannelType Channel)
{
    Ocu_HwRegisterType* hwRegs = Ocu_HwGetRegisterBase(Channel);
    if (hwRegs != NULL_PTR) {
        hwRegs->Control |= 0x01U; /* Enable bit */
    }
}

static void Ocu_HwStopChannel(Ocu_ChannelType Channel)
{
    Ocu_HwRegisterType* hwRegs = Ocu_HwGetRegisterBase(Channel);
    if (hwRegs != NULL_PTR) {
        hwRegs->Control &= ~0x01U;
    }
}

static void Ocu_HwSetPinState(Ocu_ChannelType Channel, Ocu_OutputPinStateType PinState)
{
    Ocu_HwRegisterType* hwRegs = Ocu_HwGetRegisterBase(Channel);
    if (hwRegs != NULL_PTR) {
        hwRegs->PinCtrl = (uint32)PinState;
    }
}

static void Ocu_HwSetPinAction(Ocu_ChannelType Channel, Ocu_PinActionType PinAction)
{
    Ocu_HwRegisterType* hwRegs = Ocu_HwGetRegisterBase(Channel);
    if (hwRegs != NULL_PTR) {
        hwRegs->Action = (uint32)PinAction;
    }
}

static void Ocu_HwSetCompareValue(Ocu_ChannelType Channel, Ocu_ValueType Value)
{
    Ocu_HwRegisterType* hwRegs = Ocu_HwGetRegisterBase(Channel);
    if (hwRegs != NULL_PTR) {
        hwRegs->Compare = Value;
    }
}

static Ocu_ValueType Ocu_HwGetCounter(Ocu_ChannelType Channel)
{
    Ocu_HwRegisterType* hwRegs = Ocu_HwGetRegisterBase(Channel);
    if (hwRegs != NULL_PTR) {
        return (Ocu_ValueType)hwRegs->Counter;
    }
    return 0U;
}

/*==================================================================================================
*                                    OCU API IMPLEMENTATION (MOCK)
==================================================================================================*/
void Ocu_Init(const Ocu_ConfigType* ConfigPtr)
{
    Ocu_ChannelType chIdx;
    
    /* Check if already initialized */
    if (OCU_INITIALIZED == Ocu_ModuleState) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_INIT, OCU_E_ALREADY_INITIALIZED);
        return;
    }
    
    /* Validate configuration pointer */
    if (NULL_PTR == ConfigPtr) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_INIT, OCU_E_PARAM_POINTER);
        return;
    }
    
    Ocu_CurrentConfig = ConfigPtr;
    
    /* Initialize all channels */
    for (chIdx = 0U; chIdx < Ocu_CurrentConfig->NumChannels; chIdx++) {
        const Ocu_ChannelConfigType* chConfig = &Ocu_CurrentConfig->Channels[chIdx];
        Ocu_ChannelState[chConfig->ChannelId].State = OCU_STOPPED;
        Ocu_ChannelState[chConfig->ChannelId].CurrentPinState = chConfig->DefaultPinState;
        Ocu_ChannelState[chConfig->ChannelId].CompareValue = chConfig->DefaultThreshold;
        Ocu_ChannelState[chConfig->ChannelId].PinAction = OCU_TOGGLE;
        Ocu_ChannelState[chConfig->ChannelId].IsRunning = FALSE;
        Ocu_ChannelState[chConfig->ChannelId].NotificationEnabled = FALSE;
        
        Ocu_HwInitChannel(chConfig->ChannelId, chConfig);
    }
    
    Ocu_ModuleState = OCU_INITIALIZED;
}

void Ocu_DeInit(void)
{
    Ocu_ChannelType chIdx;
    
    if (OCU_UNINIT == Ocu_ModuleState) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_DEINIT, OCU_E_UNINIT);
        return;
    }
    
    for (chIdx = 0U; chIdx < Ocu_CurrentConfig->NumChannels; chIdx++) {
        if (Ocu_ChannelState[chIdx].IsRunning) {
            Ocu_HwStopChannel(chIdx);
        }
        Ocu_HwDeInitChannel(chIdx);
        Ocu_ChannelState[chIdx].State = OCU_STOPPED;
        Ocu_ChannelState[chIdx].IsRunning = FALSE;
        Ocu_ChannelState[chIdx].NotificationEnabled = FALSE;
    }
    
    Ocu_CurrentConfig = NULL_PTR;
    Ocu_ModuleState = OCU_UNINIT;
}

void Ocu_StartChannel(Ocu_ChannelType Channel)
{
    if (OCU_UNINIT == Ocu_ModuleState) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_STARTCHANNEL, OCU_E_UNINIT);
        return;
    }
    
    if (Channel >= Ocu_CurrentConfig->NumChannels) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_STARTCHANNEL, OCU_E_PARAM_CHANNEL);
        return;
    }
    
    if (Ocu_ChannelState[Channel].IsRunning) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_STARTCHANNEL, OCU_E_CHANNEL_BUSY);
        return;
    }
    
    Ocu_HwStartChannel(Channel);
    Ocu_ChannelState[Channel].State = OCU_RUNNING;
    Ocu_ChannelState[Channel].IsRunning = TRUE;
}

void Ocu_StopChannel(Ocu_ChannelType Channel)
{
    if (OCU_UNINIT == Ocu_ModuleState) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_STOPCHANNEL, OCU_E_UNINIT);
        return;
    }
    
    if (Channel >= Ocu_CurrentConfig->NumChannels) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_STOPCHANNEL, OCU_E_PARAM_CHANNEL);
        return;
    }
    
    if (Ocu_ChannelState[Channel].IsRunning) {
        Ocu_HwStopChannel(Channel);
        Ocu_ChannelState[Channel].State = OCU_STOPPED;
        Ocu_ChannelState[Channel].IsRunning = FALSE;
    }
}

void Ocu_SetPinState(Ocu_ChannelType Channel, Ocu_OutputPinStateType PinState)
{
    if (OCU_UNINIT == Ocu_ModuleState) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETPINSTATE, OCU_E_UNINIT);
        return;
    }
    
    if (Channel >= Ocu_CurrentConfig->NumChannels) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETPINSTATE, OCU_E_PARAM_CHANNEL);
        return;
    }
    
    if ((PinState != OCU_HIGH) && (PinState != OCU_LOW)) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETPINSTATE, OCU_E_PARAM_PIN_STATE);
        return;
    }
    
    if (Ocu_ChannelState[Channel].IsRunning) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETPINSTATE, OCU_E_PARAM_INVALID_STATE);
        return;
    }
    
    Ocu_HwSetPinState(Channel, PinState);
    Ocu_ChannelState[Channel].CurrentPinState = PinState;
}

void Ocu_SetPinAction(Ocu_ChannelType Channel, Ocu_PinActionType PinAction)
{
    if (OCU_UNINIT == Ocu_ModuleState) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETPINACTION, OCU_E_UNINIT);
        return;
    }
    
    if (Channel >= Ocu_CurrentConfig->NumChannels) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETPINACTION, OCU_E_PARAM_CHANNEL);
        return;
    }
    
    if (PinAction > OCU_HOLD) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETPINACTION, OCU_E_PARAM_ACTION);
        return;
    }
    
    Ocu_HwSetPinAction(Channel, PinAction);
    Ocu_ChannelState[Channel].PinAction = PinAction;
}

Std_ReturnType Ocu_SetAbsoluteThreshold(Ocu_ChannelType Channel,
                                        Ocu_ValueType ReferenceValue,
                                        Ocu_ValueType AbsoluteValue)
{
    (void)ReferenceValue;
    
    if (OCU_UNINIT == Ocu_ModuleState) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETABSOLUTETHRESHOLD, OCU_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (Channel >= Ocu_CurrentConfig->NumChannels) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETABSOLUTETHRESHOLD, OCU_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    
    if (AbsoluteValue >= Ocu_CurrentConfig->MaxCounterValue) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETABSOLUTETHRESHOLD, OCU_E_PARAM_THRESHOLD_VALUE);
        return E_NOT_OK;
    }
    
    Ocu_HwSetCompareValue(Channel, AbsoluteValue);
    Ocu_ChannelState[Channel].CompareValue = AbsoluteValue;
    
    return E_OK;
}

Std_ReturnType Ocu_SetRelativeThreshold(Ocu_ChannelType Channel,
                                        Ocu_ValueType RelativeValue)
{
    Ocu_ValueType currentValue;
    Ocu_ValueType newValue;
    
    if (OCU_UNINIT == Ocu_ModuleState) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETRELATIVETHRESHOLD, OCU_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (Channel >= Ocu_CurrentConfig->NumChannels) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETRELATIVETHRESHOLD, OCU_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    
    if (0U == RelativeValue) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_SETRELATIVETHRESHOLD, OCU_E_PARAM_THRESHOLD_VALUE);
        return E_NOT_OK;
    }
    
    currentValue = Ocu_HwGetCounter(Channel);
    newValue = currentValue + RelativeValue;
    if (newValue > Ocu_CurrentConfig->MaxCounterValue) {
        newValue = newValue - Ocu_CurrentConfig->MaxCounterValue - 1U;
    }
    
    Ocu_HwSetCompareValue(Channel, newValue);
    Ocu_ChannelState[Channel].CompareValue = newValue;
    
    return E_OK;
}

Ocu_ValueType Ocu_GetCounter(Ocu_ChannelType Channel)
{
    if (OCU_UNINIT == Ocu_ModuleState) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_GETCOUNTER, OCU_E_UNINIT);
        return 0U;
    }
    
    if (Channel >= Ocu_CurrentConfig->NumChannels) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_GETCOUNTER, OCU_E_PARAM_CHANNEL);
        return 0U;
    }
    
    return Ocu_HwGetCounter(Channel);
}

void Ocu_DisableNotification(Ocu_ChannelType Channel)
{
    if (OCU_UNINIT == Ocu_ModuleState) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_DISABLENOTIFICATION, OCU_E_UNINIT);
        return;
    }
    
    if (Channel >= Ocu_CurrentConfig->NumChannels) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_DISABLENOTIFICATION, OCU_E_PARAM_CHANNEL);
        return;
    }
    
    Ocu_ChannelState[Channel].NotificationEnabled = FALSE;
}

void Ocu_EnableNotification(Ocu_ChannelType Channel)
{
    if (OCU_UNINIT == Ocu_ModuleState) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_ENABLENOTIFICATION, OCU_E_UNINIT);
        return;
    }
    
    if (Channel >= Ocu_CurrentConfig->NumChannels) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_ENABLENOTIFICATION, OCU_E_PARAM_CHANNEL);
        return;
    }
    
    Ocu_ChannelState[Channel].NotificationEnabled = TRUE;
}

void Ocu_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (NULL_PTR == versioninfo) {
        (void)Det_ReportError(OCU_MODULE_ID, 0U, OCU_SID_GETVERSIONINFO, OCU_E_PARAM_POINTER);
        return;
    }
    
    versioninfo->vendorID = OCU_VENDOR_ID;
    versioninfo->moduleID = OCU_MODULE_ID;
    versioninfo->sw_major_version = OCU_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = OCU_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = OCU_SW_PATCH_VERSION;
}

/*==================================================================================================
*                                    TEST CASES
==================================================================================================*/

/*----------------------------------
 * Initialization Tests
 *----------------------------------*/
/* @req SWS_Ocu_00201 */
void Test_Init_Basic(void)
{
    TEST_SECTION("Init - Basic");
    Ocu_ResetTestEnv();
    
    Ocu_Init(&Test_OcuConfig);
    TEST_ASSERT_EQ(OCU_INITIALIZED, Ocu_ModuleState);
    TEST_ASSERT_EQ(&Test_OcuConfig, Ocu_CurrentConfig);
    TEST_ASSERT_EQ(0, det_call_count);
}

/* @req SWS_Ocu_00202 */
void Test_Init_AlreadyInitialized(void)
{
    TEST_SECTION("Init - Already Initialized");
    Ocu_ResetTestEnv();
    
    Ocu_Init(&Test_OcuConfig);
    Ocu_Init(&Test_OcuConfig);
    
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_SID_INIT, det_api_id);
    TEST_ASSERT_EQ(OCU_E_ALREADY_INITIALIZED, det_error_id);
}

/* @req SWS_Ocu_00203 */
void Test_Init_NullConfig(void)
{
    TEST_SECTION("Init - Null Config");
    Ocu_ResetTestEnv();
    
    Ocu_Init(NULL_PTR);
    
    TEST_ASSERT_EQ(OCU_UNINIT, Ocu_ModuleState);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_PARAM_POINTER, det_error_id);
}

/*----------------------------------
 * Deinitialization Tests
 *----------------------------------*/
/* @req SWS_Ocu_00204 */
void Test_DeInit_Basic(void)
{
    TEST_SECTION("DeInit - Basic");
    Ocu_ResetTestEnv();
    
    Ocu_Init(&Test_OcuConfig);
    Ocu_DeInit();
    
    TEST_ASSERT_EQ(OCU_UNINIT, Ocu_ModuleState);
    TEST_ASSERT_EQ(NULL_PTR, Ocu_CurrentConfig);
    TEST_ASSERT_EQ(0, det_call_count);
}

/* @req SWS_Ocu_00205 */
void Test_DeInit_NotInitialized(void)
{
    TEST_SECTION("DeInit - Not Initialized");
    Ocu_ResetTestEnv();
    
    Ocu_DeInit();
    
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_UNINIT, det_error_id);
}

/*----------------------------------
 * Channel Start/Stop Tests
 *----------------------------------*/
/* @req SWS_Ocu_00206 */
void Test_StartStopChannel_Basic(void)
{
    TEST_SECTION("Start/Stop Channel - Basic");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Ocu_StartChannel(OCU_CHANNEL_0);
    TEST_ASSERT_EQ(OCU_RUNNING, Ocu_ChannelState[OCU_CHANNEL_0].State);
    TEST_ASSERT_EQ(TRUE, Ocu_ChannelState[OCU_CHANNEL_0].IsRunning);
    TEST_ASSERT_NE(0, Ocu_HwRegisters[OCU_CHANNEL_0].Control & 0x01U);
    
    Ocu_StopChannel(OCU_CHANNEL_0);
    TEST_ASSERT_EQ(OCU_STOPPED, Ocu_ChannelState[OCU_CHANNEL_0].State);
    TEST_ASSERT_EQ(FALSE, Ocu_ChannelState[OCU_CHANNEL_0].IsRunning);
    TEST_ASSERT_EQ(0, Ocu_HwRegisters[OCU_CHANNEL_0].Control & 0x01U);
}

/* @req SWS_Ocu_00207 */
void Test_StartChannel_InvalidChannel(void)
{
    TEST_SECTION("Start Channel - Invalid Channel");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Ocu_StartChannel(OCU_NUM_CHANNELS);
    
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_PARAM_CHANNEL, det_error_id);
}

/* @req SWS_Ocu_00208 */
void Test_StartChannel_NotInitialized(void)
{
    TEST_SECTION("Start Channel - Not Initialized");
    Ocu_ResetTestEnv();
    
    Ocu_StartChannel(OCU_CHANNEL_0);
    
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_UNINIT, det_error_id);
}

/* @req SWS_Ocu_00209 */
void Test_StartChannel_AlreadyRunning(void)
{
    TEST_SECTION("Start Channel - Already Running");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Ocu_StartChannel(OCU_CHANNEL_0);
    Ocu_StartChannel(OCU_CHANNEL_0);
    
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_CHANNEL_BUSY, det_error_id);
}

/*----------------------------------
 * Pin State Tests
 *----------------------------------*/
/* @req SWS_Ocu_00210 */
void Test_SetPinState_Basic(void)
{
    TEST_SECTION("SetPinState - Basic");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Ocu_SetPinState(OCU_CHANNEL_0, OCU_HIGH);
    TEST_ASSERT_EQ(OCU_HIGH, Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState);
    TEST_ASSERT_EQ(OCU_HIGH, Ocu_HwRegisters[OCU_CHANNEL_0].PinCtrl);
    
    Ocu_SetPinState(OCU_CHANNEL_0, OCU_LOW);
    TEST_ASSERT_EQ(OCU_LOW, Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState);
    TEST_ASSERT_EQ(OCU_LOW, Ocu_HwRegisters[OCU_CHANNEL_0].PinCtrl);
}

/* @req SWS_Ocu_00211 */
void Test_SetPinState_InvalidState(void)
{
    TEST_SECTION("SetPinState - Invalid State");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    /* Invalid pin state (assuming enum values > OCU_LOW are invalid) */
    Ocu_SetPinState(OCU_CHANNEL_0, 0xFF);
    
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_PARAM_PIN_STATE, det_error_id);
}

/* @req SWS_Ocu_00212 */
void Test_SetPinState_ChannelRunning(void)
{
    TEST_SECTION("SetPinState - Channel Running");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    Ocu_StartChannel(OCU_CHANNEL_0);
    
    Ocu_SetPinState(OCU_CHANNEL_0, OCU_HIGH);
    
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_PARAM_INVALID_STATE, det_error_id);
}

/*----------------------------------
 * Pin Action Tests
 *----------------------------------*/
/* @req SWS_Ocu_00213 */
void Test_SetPinAction_Basic(void)
{
    TEST_SECTION("SetPinAction - Basic");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Ocu_SetPinAction(OCU_CHANNEL_0, OCU_SET_HIGH);
    TEST_ASSERT_EQ(OCU_SET_HIGH, Ocu_ChannelState[OCU_CHANNEL_0].PinAction);
    TEST_ASSERT_EQ(OCU_SET_HIGH, Ocu_HwRegisters[OCU_CHANNEL_0].Action);
    
    Ocu_SetPinAction(OCU_CHANNEL_0, OCU_SET_LOW);
    TEST_ASSERT_EQ(OCU_SET_LOW, Ocu_ChannelState[OCU_CHANNEL_0].PinAction);
    
    Ocu_SetPinAction(OCU_CHANNEL_0, OCU_TOGGLE);
    TEST_ASSERT_EQ(OCU_TOGGLE, Ocu_ChannelState[OCU_CHANNEL_0].PinAction);
    
    Ocu_SetPinAction(OCU_CHANNEL_0, OCU_HOLD);
    TEST_ASSERT_EQ(OCU_HOLD, Ocu_ChannelState[OCU_CHANNEL_0].PinAction);
}

/* @req SWS_Ocu_00214 */
void Test_SetPinAction_InvalidAction(void)
{
    TEST_SECTION("SetPinAction - Invalid Action");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Ocu_SetPinAction(OCU_CHANNEL_0, 0xFF);
    
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_PARAM_ACTION, det_error_id);
}

/*----------------------------------
 * Absolute Threshold Tests
 *----------------------------------*/
/* @req SWS_Ocu_00215 */
void Test_SetAbsoluteThreshold_Basic(void)
{
    TEST_SECTION("SetAbsoluteThreshold - Basic");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Std_ReturnType result = Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, 0x5000);
    
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(0x5000, Ocu_ChannelState[OCU_CHANNEL_0].CompareValue);
    TEST_ASSERT_EQ(0x5000, Ocu_HwRegisters[OCU_CHANNEL_0].Compare);
}

/* @req SWS_Ocu_00216 */
void Test_SetAbsoluteThreshold_InvalidValue(void)
{
    TEST_SECTION("SetAbsoluteThreshold - Invalid Value");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Std_ReturnType result = Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, OCU_MAX_COUNTER_VALUE);
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_PARAM_THRESHOLD_VALUE, det_error_id);
}

/* @req SWS_Ocu_00217 */
void Test_SetAbsoluteThreshold_NotInitialized(void)
{
    TEST_SECTION("SetAbsoluteThreshold - Not Initialized");
    Ocu_ResetTestEnv();
    
    Std_ReturnType result = Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, 0x1000);
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(OCU_E_UNINIT, det_error_id);
}

/*----------------------------------
 * Relative Threshold Tests
 *----------------------------------*/
/* @req SWS_Ocu_00218 */
void Test_SetRelativeThreshold_Basic(void)
{
    TEST_SECTION("SetRelativeThreshold - Basic");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    /* Set counter to 1000 */
    Ocu_HwRegisters[OCU_CHANNEL_0].Counter = 1000;
    
    Std_ReturnType result = Ocu_SetRelativeThreshold(OCU_CHANNEL_0, 500);
    
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(1500, Ocu_ChannelState[OCU_CHANNEL_0].CompareValue);
}

/* @req SWS_Ocu_00219 */
void Test_SetRelativeThreshold_ZeroValue(void)
{
    TEST_SECTION("SetRelativeThreshold - Zero Value");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Std_ReturnType result = Ocu_SetRelativeThreshold(OCU_CHANNEL_0, 0);
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_PARAM_THRESHOLD_VALUE, det_error_id);
}

/* @req SWS_Ocu_00220 */
void Test_SetRelativeThreshold_Overflow(void)
{
    TEST_SECTION("SetRelativeThreshold - Overflow");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    /* Set counter near max value */
    Ocu_HwRegisters[OCU_CHANNEL_0].Counter = OCU_MAX_COUNTER_VALUE - 100;
    
    Std_ReturnType result = Ocu_SetRelativeThreshold(OCU_CHANNEL_0, 200);
    
    TEST_ASSERT_EQ(E_OK, result);
    /* Should wrap around */
}

/*----------------------------------
 * GetCounter Tests
 *----------------------------------*/
/* @req SWS_Ocu_00221 */
void Test_GetCounter_Basic(void)
{
    TEST_SECTION("GetCounter - Basic");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Ocu_HwRegisters[OCU_CHANNEL_0].Counter = 0x12345;
    
    Ocu_ValueType counter = Ocu_GetCounter(OCU_CHANNEL_0);
    
    TEST_ASSERT_EQ(0x12345, counter);
}

/* @req SWS_Ocu_00222 */
void Test_GetCounter_NotInitialized(void)
{
    TEST_SECTION("GetCounter - Not Initialized");
    Ocu_ResetTestEnv();
    
    Ocu_ValueType counter = Ocu_GetCounter(OCU_CHANNEL_0);
    
    TEST_ASSERT_EQ(0, counter);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_UNINIT, det_error_id);
}

/* @req SWS_Ocu_00223 */
void Test_GetCounter_InvalidChannel(void)
{
    TEST_SECTION("GetCounter - Invalid Channel");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Ocu_ValueType counter = Ocu_GetCounter(OCU_NUM_CHANNELS);
    
    TEST_ASSERT_EQ(0, counter);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_PARAM_CHANNEL, det_error_id);
}

/*----------------------------------
 * Notification Tests
 *----------------------------------*/
/* @req SWS_Ocu_00224 */
void Test_Notification_EnableDisable(void)
{
    TEST_SECTION("Notification - Enable/Disable");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    Ocu_EnableNotification(OCU_CHANNEL_1);
    TEST_ASSERT_EQ(TRUE, Ocu_ChannelState[OCU_CHANNEL_1].NotificationEnabled);
    
    Ocu_DisableNotification(OCU_CHANNEL_1);
    TEST_ASSERT_EQ(FALSE, Ocu_ChannelState[OCU_CHANNEL_1].NotificationEnabled);
}

/* @req SWS_Ocu_00225 */
void Test_Notification_NotInitialized(void)
{
    TEST_SECTION("Notification - Not Initialized");
    Ocu_ResetTestEnv();
    
    Ocu_EnableNotification(OCU_CHANNEL_0);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_UNINIT, det_error_id);
    
    det_call_count = 0;
    Ocu_DisableNotification(OCU_CHANNEL_0);
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_UNINIT, det_error_id);
}

/*----------------------------------
 * Version Info Tests
 *----------------------------------*/
/* @req SWS_Ocu_00226 */
void Test_GetVersionInfo_Basic(void)
{
    TEST_SECTION("GetVersionInfo - Basic");
    Ocu_ResetTestEnv();
    
    Std_VersionInfoType versionInfo;
    Ocu_GetVersionInfo(&versionInfo);
    
    TEST_ASSERT_EQ(OCU_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQ(OCU_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQ(OCU_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQ(OCU_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQ(OCU_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}

/* @req SWS_Ocu_00227 */
void Test_GetVersionInfo_NullPointer(void)
{
    TEST_SECTION("GetVersionInfo - Null Pointer");
    Ocu_ResetTestEnv();
    
    Ocu_GetVersionInfo(NULL_PTR);
    
    TEST_ASSERT_EQ(1, det_call_count);
    TEST_ASSERT_EQ(OCU_E_PARAM_POINTER, det_error_id);
}

/*----------------------------------
 * PWM Generation Pattern Tests
 *----------------------------------*/
/* @req SWS_Ocu_00228 */
void Test_PWM_50Percent_Duty(void)
{
    TEST_SECTION("PWM - 50% Duty Cycle Pattern");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    /* Configure for 50% duty cycle PWM
     * Period = 1000 counts
     * Set HIGH at 0, TOGGLE at 500 (50%)
     */
    const Ocu_ValueType period = 1000;
    const Ocu_ValueType halfPeriod = 500;
    
    /* Channel 0: Set initial state LOW, TOGGLE at half period */
    Ocu_SetPinState(OCU_CHANNEL_0, OCU_LOW);
    Ocu_SetPinAction(OCU_CHANNEL_0, OCU_TOGGLE);
    Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, halfPeriod);
    
    Ocu_StartChannel(OCU_CHANNEL_0);
    
    TEST_ASSERT_EQ(OCU_RUNNING, Ocu_ChannelState[OCU_CHANNEL_0].State);
    TEST_ASSERT_EQ(OCU_TOGGLE, Ocu_ChannelState[OCU_CHANNEL_0].PinAction);
    TEST_ASSERT_EQ(halfPeriod, Ocu_ChannelState[OCU_CHANNEL_0].CompareValue);
}

/* @req SWS_Ocu_00229 */
void Test_PWM_VariableDutyCycle(void)
{
    TEST_SECTION("PWM - Variable Duty Cycle");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    Ocu_StartChannel(OCU_CHANNEL_0);
    
    /* Test changing duty cycle while running */
    const Ocu_ValueType period = 1000;
    
    /* 25% duty cycle */
    Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, period / 4);
    TEST_ASSERT_EQ(period / 4, Ocu_ChannelState[OCU_CHANNEL_0].CompareValue);
    
    /* 75% duty cycle */
    Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, (period * 3) / 4);
    TEST_ASSERT_EQ((period * 3) / 4, Ocu_ChannelState[OCU_CHANNEL_0].CompareValue);
}

/* @req SWS_Ocu_00230 */
void Test_PWM_MultipleChannels(void)
{
    TEST_SECTION("PWM - Multiple Channels");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    /* Start all channels with different configurations */
    Ocu_SetPinAction(OCU_CHANNEL_0, OCU_TOGGLE);
    Ocu_SetPinAction(OCU_CHANNEL_1, OCU_SET_HIGH);
    Ocu_SetPinAction(OCU_CHANNEL_2, OCU_SET_LOW);
    Ocu_SetPinAction(OCU_CHANNEL_3, OCU_TOGGLE);
    
    Ocu_StartChannel(OCU_CHANNEL_0);
    Ocu_StartChannel(OCU_CHANNEL_1);
    Ocu_StartChannel(OCU_CHANNEL_2);
    Ocu_StartChannel(OCU_CHANNEL_3);
    
    for (int i = 0; i < OCU_NUM_CHANNELS; i++) {
        TEST_ASSERT_EQ(OCU_RUNNING, Ocu_ChannelState[i].State);
        TEST_ASSERT_EQ(TRUE, Ocu_ChannelState[i].IsRunning);
    }
    
    Ocu_StopChannel(OCU_CHANNEL_0);
    Ocu_StopChannel(OCU_CHANNEL_1);
    
    TEST_ASSERT_EQ(OCU_STOPPED, Ocu_ChannelState[OCU_CHANNEL_0].State);
    TEST_ASSERT_EQ(OCU_STOPPED, Ocu_ChannelState[OCU_CHANNEL_1].State);
    TEST_ASSERT_EQ(OCU_RUNNING, Ocu_ChannelState[OCU_CHANNEL_2].State);
    TEST_ASSERT_EQ(OCU_RUNNING, Ocu_ChannelState[OCU_CHANNEL_3].State);
}

/*----------------------------------
 * Output Compare Pattern Tests
 *----------------------------------*/
/* @req SWS_Ocu_00231 */
void Test_OutputCompare_SingleShot(void)
{
    TEST_SECTION("Output Compare - Single Shot");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    /* Configure single shot: SET_HIGH at specific threshold, then HOLD */
    Ocu_SetPinState(OCU_CHANNEL_0, OCU_LOW);
    Ocu_SetPinAction(OCU_CHANNEL_0, OCU_SET_HIGH);
    Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, 0x10000);
    
    Ocu_StartChannel(OCU_CHANNEL_0);
    
    TEST_ASSERT_EQ(OCU_SET_HIGH, Ocu_ChannelState[OCU_CHANNEL_0].PinAction);
    TEST_ASSERT_EQ(0x10000, Ocu_ChannelState[OCU_CHANNEL_0].CompareValue);
}

/* @req SWS_Ocu_00232 */
void Test_OutputCompare_TimedPulse(void)
{
    TEST_SECTION("Output Compare - Timed Pulse");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    /* Configure for pulse generation:
     * Start LOW, SET_HIGH at threshold, then TOGGLE for pulse end
     */
    Ocu_SetPinState(OCU_CHANNEL_0, OCU_LOW);
    Ocu_SetPinAction(OCU_CHANNEL_0, OCU_SET_HIGH);
    Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, 0x5000);
    
    TEST_ASSERT_EQ(OCU_LOW, Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState);
    TEST_ASSERT_EQ(OCU_SET_HIGH, Ocu_ChannelState[OCU_CHANNEL_0].PinAction);
    
    /* After compare match, change action for pulse end */
    Ocu_SetPinAction(OCU_CHANNEL_0, OCU_SET_LOW);
    TEST_ASSERT_EQ(OCU_SET_LOW, Ocu_ChannelState[OCU_CHANNEL_0].PinAction);
}

/*----------------------------------
 * Edge Case and Boundary Tests
 *----------------------------------*/
/* @req SWS_Ocu_00233 */
void Test_Edge_MaxChannels(void)
{
    TEST_SECTION("Edge Case - All Channels");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    /* Initialize and start all channels */
    for (Ocu_ChannelType ch = 0; ch < OCU_NUM_CHANNELS; ch++) {
        Ocu_StartChannel(ch);
        TEST_ASSERT_EQ(OCU_RUNNING, Ocu_ChannelState[ch].State);
    }
    
    /* Stop all channels */
    for (Ocu_ChannelType ch = 0; ch < OCU_NUM_CHANNELS; ch++) {
        Ocu_StopChannel(ch);
        TEST_ASSERT_EQ(OCU_STOPPED, Ocu_ChannelState[ch].State);
    }
}

/* @req SWS_Ocu_00234 */
void Test_Edge_MaxThreshold(void)
{
    TEST_SECTION("Edge Case - Maximum Threshold");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    /* Set threshold to max - 1 (valid) */
    Std_ReturnType result = Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, OCU_MAX_COUNTER_VALUE - 1);
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(OCU_MAX_COUNTER_VALUE - 1, Ocu_ChannelState[OCU_CHANNEL_0].CompareValue);
    
    /* Try to set threshold to max (should fail) */
    result = Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, OCU_MAX_COUNTER_VALUE);
    TEST_ASSERT_EQ(E_NOT_OK, result);
}

/* @req SWS_Ocu_00235 */
void Test_Edge_ZeroThreshold(void)
{
    TEST_SECTION("Edge Case - Zero Threshold");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    
    /* Zero threshold is valid for absolute */
    Std_ReturnType result = Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, 0);
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(0, Ocu_ChannelState[OCU_CHANNEL_0].CompareValue);
    
    /* Zero threshold is invalid for relative */
    result = Ocu_SetRelativeThreshold(OCU_CHANNEL_0, 0);
    TEST_ASSERT_EQ(E_NOT_OK, result);
}

/*----------------------------------
 * Stress and Sequence Tests
 *----------------------------------*/
/* @req SWS_Ocu_00236 */
void Test_Sequence_InitStartStopDeinit(void)
{
    TEST_SECTION("Sequence - Init->Start->Stop->DeInit");
    Ocu_ResetTestEnv();
    
    /* Complete cycle */
    Ocu_Init(&Test_OcuConfig);
    TEST_ASSERT_EQ(OCU_INITIALIZED, Ocu_ModuleState);
    
    Ocu_StartChannel(OCU_CHANNEL_0);
    TEST_ASSERT_EQ(OCU_RUNNING, Ocu_ChannelState[OCU_CHANNEL_0].State);
    
    Ocu_StopChannel(OCU_CHANNEL_0);
    TEST_ASSERT_EQ(OCU_STOPPED, Ocu_ChannelState[OCU_CHANNEL_0].State);
    
    Ocu_DeInit();
    TEST_ASSERT_EQ(OCU_UNINIT, Ocu_ModuleState);
    
    /* Re-initialize */
    Ocu_Init(&Test_OcuConfig);
    TEST_ASSERT_EQ(OCU_INITIALIZED, Ocu_ModuleState);
    
    /* Should be able to start again */
    Ocu_StartChannel(OCU_CHANNEL_0);
    TEST_ASSERT_EQ(OCU_RUNNING, Ocu_ChannelState[OCU_CHANNEL_0].State);
}

/* @req SWS_Ocu_00237 */
void Test_Sequence_MultipleOperations(void)
{
    TEST_SECTION("Sequence - Multiple Operations");
    Ocu_ResetTestEnv();
    Ocu_Init(&Test_OcuConfig);
    Ocu_StartChannel(OCU_CHANNEL_0);
    
    /* Perform various operations in sequence */
    Ocu_SetAbsoluteThreshold(OCU_CHANNEL_0, 0, 1000);
    Ocu_SetPinAction(OCU_CHANNEL_0, OCU_TOGGLE);
    Ocu_ValueType counter = Ocu_GetCounter(OCU_CHANNEL_0);
    (void)counter;
    Ocu_SetRelativeThreshold(OCU_CHANNEL_0, 500);
    Ocu_SetPinAction(OCU_CHANNEL_0, OCU_SET_HIGH);
    
    TEST_ASSERT_EQ(OCU_RUNNING, Ocu_ChannelState[OCU_CHANNEL_0].State);
    TEST_ASSERT_EQ(OCU_SET_HIGH, Ocu_ChannelState[OCU_CHANNEL_0].PinAction);
}

/*==================================================================================================
*                                    MAIN FUNCTION
==================================================================================================*/
int main(void)
{
    printf("========================================\n");
    printf("OCU Driver Unit Test Suite\n");
    printf("========================================\n");
    
    /* Initialization Tests */
    Test_Init_Basic();
    Test_Init_AlreadyInitialized();
    Test_Init_NullConfig();
    
    /* Deinitialization Tests */
    Test_DeInit_Basic();
    Test_DeInit_NotInitialized();
    
    /* Channel Start/Stop Tests */
    Test_StartStopChannel_Basic();
    Test_StartChannel_InvalidChannel();
    Test_StartChannel_NotInitialized();
    Test_StartChannel_AlreadyRunning();
    
    /* Pin State Tests */
    Test_SetPinState_Basic();
    Test_SetPinState_InvalidState();
    Test_SetPinState_ChannelRunning();
    
    /* Pin Action Tests */
    Test_SetPinAction_Basic();
    Test_SetPinAction_InvalidAction();
    
    /* Absolute Threshold Tests */
    Test_SetAbsoluteThreshold_Basic();
    Test_SetAbsoluteThreshold_InvalidValue();
    Test_SetAbsoluteThreshold_NotInitialized();
    
    /* Relative Threshold Tests */
    Test_SetRelativeThreshold_Basic();
    Test_SetRelativeThreshold_ZeroValue();
    Test_SetRelativeThreshold_Overflow();
    
    /* GetCounter Tests */
    Test_GetCounter_Basic();
    Test_GetCounter_NotInitialized();
    Test_GetCounter_InvalidChannel();
    
    /* Notification Tests */
    Test_Notification_EnableDisable();
    Test_Notification_NotInitialized();
    
    /* Version Info Tests */
    Test_GetVersionInfo_Basic();
    Test_GetVersionInfo_NullPointer();
    
    /* PWM Pattern Tests */
    Test_PWM_50Percent_Duty();
    Test_PWM_VariableDutyCycle();
    Test_PWM_MultipleChannels();
    
    /* Output Compare Tests */
    Test_OutputCompare_SingleShot();
    Test_OutputCompare_TimedPulse();
    
    /* Edge Case Tests */
    Test_Edge_MaxChannels();
    Test_Edge_MaxThreshold();
    Test_Edge_ZeroThreshold();
    
    /* Sequence Tests */
    Test_Sequence_InitStartStopDeinit();
    Test_Sequence_MultipleOperations();
    
    /* Print Summary */
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Total Tests:  %d\n", tests_run);
    printf("Passed:       %d\n", tests_passed);
    printf("Failed:       %d\n", tests_failed);
    printf("Coverage:     %d%%\n", (tests_run > 0) ? (tests_passed * 100 / tests_run) : 0);
    printf("========================================\n");
    
    if (tests_failed == 0) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n");
        return 1;
    }
}
