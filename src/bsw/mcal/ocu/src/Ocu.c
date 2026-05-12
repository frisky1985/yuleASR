/**
 * @file Ocu.c
 * @brief OCU (Output Compare Unit) Driver implementation
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: OCU Driver
 * Layer: MCAL (Microcontroller Driver Layer)
 * ASIL Level: D
 * MISRA C:2012 Compliant
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Ocu.h"
#include "Ocu_Private.h"

#if (OCU_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
/**
 * @brief Static assertion for compile-time checks
 */
#define OCU_STATIC_ASSERT(condition, msg) \
    typedef char msg[(condition) ? 1 : -1]

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* None */

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static void Ocu_ValidateInitConfig(const Ocu_ConfigType* ConfigPtr);
static void Ocu_ChannelInit(const Ocu_ChannelConfigType* ChannelConfig);

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define OCU_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief Module initialization state
 */
uint8 Ocu_ModuleState = OCU_UNINIT;

#define OCU_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

#define OCU_START_SEC_VAR_NO_INIT_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief Channel runtime states
 */
Ocu_ChannelStateType Ocu_ChannelState[OCU_NUM_CHANNELS];

/**
 * @brief Pointer to current configuration
 */
const Ocu_ConfigType* Ocu_CurrentConfig = NULL_PTR;

#define OCU_STOP_SEC_VAR_NO_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    GLOBAL VARIABLES
==================================================================================================*/
#define OCU_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief Channel configuration array (link-time configuration)
 */
const Ocu_ChannelConfigType Ocu_ChannelConfig[OCU_NUM_CHANNELS] = {
#if (OCU_CHANNEL_0_ENABLE == STD_ON)
    {
        OCU_CHANNEL_0,
        OCU_CHANNEL_0_DEFAULT_PIN_STATE,
        OCU_CHANNEL_0_DEFAULT_THRESHOLD,
        OCU_CHANNEL_0_NOTIFICATION,
        OCU_CHANNEL_0_BACKGROUND_MODE,
        OCU_CHANNEL_0_BASE_ADDRESS
    },
#endif
#if (OCU_CHANNEL_1_ENABLE == STD_ON)
    {
        OCU_CHANNEL_1,
        OCU_CHANNEL_1_DEFAULT_PIN_STATE,
        OCU_CHANNEL_1_DEFAULT_THRESHOLD,
        OCU_CHANNEL_1_NOTIFICATION,
        OCU_CHANNEL_1_BACKGROUND_MODE,
        OCU_CHANNEL_1_BASE_ADDRESS
    },
#endif
#if (OCU_CHANNEL_2_ENABLE == STD_ON)
    {
        OCU_CHANNEL_2,
        OCU_CHANNEL_2_DEFAULT_PIN_STATE,
        OCU_CHANNEL_2_DEFAULT_THRESHOLD,
        OCU_CHANNEL_2_NOTIFICATION,
        OCU_CHANNEL_2_BACKGROUND_MODE,
        OCU_CHANNEL_2_BASE_ADDRESS
    },
#endif
#if (OCU_CHANNEL_3_ENABLE == STD_ON)
    {
        OCU_CHANNEL_3,
        OCU_CHANNEL_3_DEFAULT_PIN_STATE,
        OCU_CHANNEL_3_DEFAULT_THRESHOLD,
        OCU_CHANNEL_3_NOTIFICATION,
        OCU_CHANNEL_3_BACKGROUND_MODE,
        OCU_CHANNEL_3_BASE_ADDRESS
    }
#endif
};

/**
 * @brief OCU module configuration
 */
const Ocu_ConfigType Ocu_Config = {
    Ocu_ChannelConfig,
    OCU_NUM_CHANNELS,
    (boolean)OCU_DEV_ERROR_DETECT,
    (boolean)OCU_VERSION_INFO_API,
    (boolean)OCU_DE_INIT_API,
    (boolean)OCU_SET_PIN_STATE_API,
    (boolean)OCU_SET_PIN_ACTION_API,
    (boolean)OCU_SET_THRESHOLD_API,
    (boolean)OCU_NOTIFICATION_SUPPORTED,
    OCU_MAX_COUNTER_VALUE
};

#define OCU_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
/**
 * @brief Validates initialization configuration
 * @param ConfigPtr Pointer to configuration structure
 */
static void Ocu_ValidateInitConfig(const Ocu_ConfigType* ConfigPtr)
{
    /* MISRA C:2012 Rule 15.5 - Single exit point not required for simple validation */
    /* Check if NULL_PTR, but pre-compile config uses &Ocu_Config directly */
    #if (OCU_CONFIGURATION_VARIANT == OCU_VARIANT_PRE_COMPILE)
    (void)ConfigPtr; /* Suppress unused parameter warning */
    #else
    if (NULL_PTR == ConfigPtr)
    {
        OCU_REPORT_ERROR(OCU_SID_INIT, OCU_E_PARAM_POINTER);
    }
    #endif
}

/**
 * @brief Initializes a single channel
 * @param ChannelConfig Channel configuration pointer
 */
static void Ocu_ChannelInit(const Ocu_ChannelConfigType* ChannelConfig)
{
    Ocu_ChannelType channel;
    Ocu_ChannelStateType* statePtr;

    channel = ChannelConfig->ChannelId;
    statePtr = &Ocu_ChannelState[channel];

    /* Initialize channel state */
    statePtr->State = OCU_STOPPED;
    statePtr->CurrentPinState = ChannelConfig->DefaultPinState;
    statePtr->CompareValue = ChannelConfig->DefaultThreshold;
    statePtr->PinAction = OCU_TOGGLE;
    statePtr->IsRunning = FALSE;
    statePtr->NotificationEnabled = FALSE;

    /* Initialize hardware */
    Ocu_HwInitChannel(channel, ChannelConfig);

    /* Set initial pin state */
    Ocu_HwSetPinState(channel, ChannelConfig->DefaultPinState);
    Ocu_HwSetCompareValue(channel, ChannelConfig->DefaultThreshold);
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
#define OCU_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the OCU driver
 * @param ConfigPtr Pointer to configuration structure
 * @implements Ocu_Init
 */
void Ocu_Init(const Ocu_ConfigType* ConfigPtr)
{
    Ocu_ChannelType chIdx;
    const Ocu_ChannelConfigType* chConfig;

    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    /* Check if already initialized */
    if (OCU_INITIALIZED == Ocu_ModuleState)
    {
        OCU_REPORT_ERROR(OCU_SID_INIT, OCU_E_ALREADY_INITIALIZED);
        return;
    }

    /* Validate configuration pointer */
    #if (OCU_CONFIGURATION_VARIANT != OCU_VARIANT_PRE_COMPILE)
    if (NULL_PTR == ConfigPtr)
    {
        OCU_REPORT_ERROR(OCU_SID_INIT, OCU_E_PARAM_POINTER);
        return;
    }
    #endif
    #endif /* OCU_DEV_ERROR_DETECT */

    /* Use pre-compile configuration if not provided */
    #if (OCU_CONFIGURATION_VARIANT == OCU_VARIANT_PRE_COMPILE)
    (void)ConfigPtr;
    Ocu_CurrentConfig = &Ocu_Config;
    #else
    Ocu_CurrentConfig = ConfigPtr;
    #endif

    /* Validate configuration */
    Ocu_ValidateInitConfig(Ocu_CurrentConfig);

    /* Initialize all channels */
    for (chIdx = 0U; chIdx < Ocu_CurrentConfig->NumChannels; chIdx++)
    {
        chConfig = &Ocu_CurrentConfig->Channels[chIdx];
        Ocu_ChannelInit(chConfig);
    }

    /* Set module as initialized */
    Ocu_ModuleState = OCU_INITIALIZED;
}

/**
 * @brief Deinitializes the OCU driver
 * @implements Ocu_DeInit
 */
void Ocu_DeInit(void)
{
    Ocu_ChannelType chIdx;

    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    /* Check if initialized */
    if (OCU_UNINIT == Ocu_ModuleState)
    {
        OCU_REPORT_ERROR(OCU_SID_DEINIT, OCU_E_UNINIT);
        return;
    }
    #endif /* OCU_DEV_ERROR_DETECT */

    /* Deinitialize all channels */
    for (chIdx = 0U; chIdx < Ocu_CurrentConfig->NumChannels; chIdx++)
    {
        /* Stop channel if running */
        if (Ocu_ChannelState[chIdx].IsRunning)
        {
            Ocu_HwStopChannel(chIdx);
        }

        /* Deinitialize hardware */
        Ocu_HwDeInitChannel(chIdx);

        /* Reset channel state */
        Ocu_ChannelState[chIdx].State = OCU_STOPPED;
        Ocu_ChannelState[chIdx].IsRunning = FALSE;
        Ocu_ChannelState[chIdx].NotificationEnabled = FALSE;
    }

    /* Clear configuration pointer */
    Ocu_CurrentConfig = NULL_PTR;

    /* Set module as uninitialized */
    Ocu_ModuleState = OCU_UNINIT;
}

/**
 * @brief Starts an OCU channel
 * @param Channel Channel to start
 * @implements Ocu_StartChannel
 */
void Ocu_StartChannel(Ocu_ChannelType Channel)
{
    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    /* Check if initialized */
    if (OCU_UNINIT == Ocu_ModuleState)
    {
        OCU_REPORT_ERROR(OCU_SID_STARTCHANNEL, OCU_E_UNINIT);
        return;
    }

    /* Validate channel */
    if (Channel >= Ocu_CurrentConfig->NumChannels)
    {
        OCU_REPORT_ERROR(OCU_SID_STARTCHANNEL, OCU_E_PARAM_CHANNEL);
        return;
    }
    #endif /* OCU_DEV_ERROR_DETECT */

    /* Check if already running */
    if (!Ocu_ChannelState[Channel].IsRunning)
    {
        /* Start hardware */
        Ocu_HwStartChannel(Channel);

        /* Update state */
        Ocu_ChannelState[Channel].State = OCU_RUNNING;
        Ocu_ChannelState[Channel].IsRunning = TRUE;
    }
    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    else
    {
        OCU_REPORT_ERROR(OCU_SID_STARTCHANNEL, OCU_E_CHANNEL_BUSY);
    }
    #endif
}

/**
 * @brief Stops an OCU channel
 * @param Channel Channel to stop
 * @implements Ocu_StopChannel
 */
void Ocu_StopChannel(Ocu_ChannelType Channel)
{
    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    /* Check if initialized */
    if (OCU_UNINIT == Ocu_ModuleState)
    {
        OCU_REPORT_ERROR(OCU_SID_STOPCHANNEL, OCU_E_UNINIT);
        return;
    }

    /* Validate channel */
    if (Channel >= Ocu_CurrentConfig->NumChannels)
    {
        OCU_REPORT_ERROR(OCU_SID_STOPCHANNEL, OCU_E_PARAM_CHANNEL);
        return;
    }
    #endif /* OCU_DEV_ERROR_DETECT */

    /* Check if running */
    if (Ocu_ChannelState[Channel].IsRunning)
    {
        /* Stop hardware */
        Ocu_HwStopChannel(Channel);

        /* Update state */
        Ocu_ChannelState[Channel].State = OCU_STOPPED;
        Ocu_ChannelState[Channel].IsRunning = FALSE;
    }
}

/**
 * @brief Sets the pin state directly
 * @param Channel Channel to set
 * @param PinState Pin state to set
 * @implements Ocu_SetPinState
 */
void Ocu_SetPinState(Ocu_ChannelType Channel, Ocu_OutputPinStateType PinState)
{
    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    /* Check if initialized */
    if (OCU_UNINIT == Ocu_ModuleState)
    {
        OCU_REPORT_ERROR(OCU_SID_SETPINSTATE, OCU_E_UNINIT);
        return;
    }

    /* Validate channel */
    if (Channel >= Ocu_CurrentConfig->NumChannels)
    {
        OCU_REPORT_ERROR(OCU_SID_SETPINSTATE, OCU_E_PARAM_CHANNEL);
        return;
    }

    /* Validate pin state */
    if ((PinState != OCU_HIGH) && (PinState != OCU_LOW))
    {
        OCU_REPORT_ERROR(OCU_SID_SETPINSTATE, OCU_E_PARAM_PIN_STATE);
        return;
    }

    /* Check if channel is running - cannot set pin state while running */
    if (Ocu_ChannelState[Channel].IsRunning)
    {
        OCU_REPORT_ERROR(OCU_SID_SETPINSTATE, OCU_E_PARAM_INVALID_STATE);
        return;
    }
    #endif /* OCU_DEV_ERROR_DETECT */

    /* Set hardware pin state */
    Ocu_HwSetPinState(Channel, PinState);

    /* Update state */
    Ocu_ChannelState[Channel].CurrentPinState = PinState;
}

/**
 * @brief Sets the pin action on compare match
 * @param Channel Channel to configure
 * @param PinAction Action to perform
 * @implements Ocu_SetPinAction
 */
void Ocu_SetPinAction(Ocu_ChannelType Channel, Ocu_PinActionType PinAction)
{
    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    /* Check if initialized */
    if (OCU_UNINIT == Ocu_ModuleState)
    {
        OCU_REPORT_ERROR(OCU_SID_SETPINACTION, OCU_E_UNINIT);
        return;
    }

    /* Validate channel */
    if (Channel >= Ocu_CurrentConfig->NumChannels)
    {
        OCU_REPORT_ERROR(OCU_SID_SETPINACTION, OCU_E_PARAM_CHANNEL);
        return;
    }

    /* Validate pin action */
    if (PinAction > OCU_HOLD)
    {
        OCU_REPORT_ERROR(OCU_SID_SETPINACTION, OCU_E_PARAM_ACTION);
        return;
    }
    #endif /* OCU_DEV_ERROR_DETECT */

    /* Set hardware pin action */
    Ocu_HwSetPinAction(Channel, PinAction);

    /* Update state */
    Ocu_ChannelState[Channel].PinAction = PinAction;
}

/**
 * @brief Sets an absolute threshold value
 * @param Channel Channel to configure
 * @param ReferenceValue Reference counter value
 * @param AbsoluteValue Absolute compare value
 * @return Result of operation
 * @implements Ocu_SetAbsoluteThreshold
 */
Std_ReturnType Ocu_SetAbsoluteThreshold(Ocu_ChannelType Channel,
                                        Ocu_ValueType ReferenceValue,
                                        Ocu_ValueType AbsoluteValue)
{
    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    /* Check if initialized */
    if (OCU_UNINIT == Ocu_ModuleState)
    {
        OCU_REPORT_ERROR(OCU_SID_SETABSOLUTETHRESHOLD, OCU_E_UNINIT);
        return E_NOT_OK;
    }

    /* Validate channel */
    if (Channel >= Ocu_CurrentConfig->NumChannels)
    {
        OCU_REPORT_ERROR(OCU_SID_SETABSOLUTETHRESHOLD, OCU_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }

    /* Validate reference value */
    if (ReferenceValue >= Ocu_CurrentConfig->MaxCounterValue)
    {
        OCU_REPORT_ERROR(OCU_SID_SETABSOLUTETHRESHOLD, OCU_E_PARAM_REF_VALUE);
        return E_NOT_OK;
    }

    /* Validate absolute value */
    if (AbsoluteValue >= Ocu_CurrentConfig->MaxCounterValue)
    {
        OCU_REPORT_ERROR(OCU_SID_SETABSOLUTETHRESHOLD, OCU_E_PARAM_THRESHOLD_VALUE);
        return E_NOT_OK;
    }
    #else
    (void)ReferenceValue;
    #endif /* OCU_DEV_ERROR_DETECT */

    /* Set hardware compare value */
    Ocu_HwSetCompareValue(Channel, AbsoluteValue);

    /* Update state */
    Ocu_ChannelState[Channel].CompareValue = AbsoluteValue;

    return E_OK;
}

/**
 * @brief Sets a relative threshold value
 * @param Channel Channel to configure
 * @param RelativeValue Relative value to add
 * @return Result of operation
 * @implements Ocu_SetRelativeThreshold
 */
Std_ReturnType Ocu_SetRelativeThreshold(Ocu_ChannelType Channel,
                                        Ocu_ValueType RelativeValue)
{
    Ocu_ValueType currentValue;
    Ocu_ValueType newValue;

    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    /* Check if initialized */
    if (OCU_UNINIT == Ocu_ModuleState)
    {
        OCU_REPORT_ERROR(OCU_SID_SETRELATIVETHRESHOLD, OCU_E_UNINIT);
        return E_NOT_OK;
    }

    /* Validate channel */
    if (Channel >= Ocu_CurrentConfig->NumChannels)
    {
        OCU_REPORT_ERROR(OCU_SID_SETRELATIVETHRESHOLD, OCU_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }

    /* Validate relative value - must be greater than 0 */
    if (0U == RelativeValue)
    {
        OCU_REPORT_ERROR(OCU_SID_SETRELATIVETHRESHOLD, OCU_E_PARAM_THRESHOLD_VALUE);
        return E_NOT_OK;
    }
    #endif /* OCU_DEV_ERROR_DETECT */

    /* Get current counter value */
    currentValue = Ocu_HwGetCounter(Channel);

    /* Calculate new value with overflow handling */
    newValue = currentValue + RelativeValue;
    if (newValue > Ocu_CurrentConfig->MaxCounterValue)
    {
        newValue = newValue - Ocu_CurrentConfig->MaxCounterValue - 1U;
    }

    /* Set hardware compare value */
    Ocu_HwSetCompareValue(Channel, newValue);

    /* Update state */
    Ocu_ChannelState[Channel].CompareValue = newValue;

    return E_OK;
}

/**
 * @brief Gets the current counter value
 * @param Channel Channel to read
 * @return Current counter value
 * @implements Ocu_GetCounter
 */
Ocu_ValueType Ocu_GetCounter(Ocu_ChannelType Channel)
{
    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    /* Check if initialized */
    if (OCU_UNINIT == Ocu_ModuleState)
    {
        OCU_REPORT_ERROR(OCU_SID_GETCOUNTER, OCU_E_UNINIT);
        return 0U;
    }

    /* Validate channel */
    if (Channel >= Ocu_CurrentConfig->NumChannels)
    {
        OCU_REPORT_ERROR(OCU_SID_GETCOUNTER, OCU_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif /* OCU_DEV_ERROR_DETECT */

    return Ocu_HwGetCounter(Channel);
}

/**
 * @brief Disables notification for a channel
 * @param Channel Channel to disable
 * @implements Ocu_DisableNotification
 */
void Ocu_DisableNotification(Ocu_ChannelType Channel)
{
    #if (OCU_NOTIFICATION_SUPPORTED == STD_ON)
    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    /* Check if initialized */
    if (OCU_UNINIT == Ocu_ModuleState)
    {
        OCU_REPORT_ERROR(OCU_SID_DISABLENOTIFICATION, OCU_E_UNINIT);
        return;
    }

    /* Validate channel */
    if (Channel >= Ocu_CurrentConfig->NumChannels)
    {
        OCU_REPORT_ERROR(OCU_SID_DISABLENOTIFICATION, OCU_E_PARAM_CHANNEL);
        return;
    }

    /* Check if notification is supported for this channel */
    if (NULL_PTR == Ocu_CurrentConfig->Channels[Channel].Notification)
    {
        OCU_REPORT_ERROR(OCU_SID_DISABLENOTIFICATION, OCU_E_PARAM_CHANNEL);
        return;
    }
    #endif /* OCU_DEV_ERROR_DETECT */

    /* Disable notification in state */
    Ocu_ChannelState[Channel].NotificationEnabled = FALSE;
    #else
    (void)Channel;
    #endif /* OCU_NOTIFICATION_SUPPORTED */
}

/**
 * @brief Enables notification for a channel
 * @param Channel Channel to enable
 * @implements Ocu_EnableNotification
 */
void Ocu_EnableNotification(Ocu_ChannelType Channel)
{
    #if (OCU_NOTIFICATION_SUPPORTED == STD_ON)
    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    /* Check if initialized */
    if (OCU_UNINIT == Ocu_ModuleState)
    {
        OCU_REPORT_ERROR(OCU_SID_ENABLENOTIFICATION, OCU_E_UNINIT);
        return;
    }

    /* Validate channel */
    if (Channel >= Ocu_CurrentConfig->NumChannels)
    {
        OCU_REPORT_ERROR(OCU_SID_ENABLENOTIFICATION, OCU_E_PARAM_CHANNEL);
        return;
    }

    /* Check if notification is supported for this channel */
    if (NULL_PTR == Ocu_CurrentConfig->Channels[Channel].Notification)
    {
        OCU_REPORT_ERROR(OCU_SID_ENABLENOTIFICATION, OCU_E_PARAM_CHANNEL);
        return;
    }
    #endif /* OCU_DEV_ERROR_DETECT */

    /* Enable notification in state */
    Ocu_ChannelState[Channel].NotificationEnabled = TRUE;
    #else
    (void)Channel;
    #endif /* OCU_NOTIFICATION_SUPPORTED */
}

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @implements Ocu_GetVersionInfo
 */
void Ocu_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (OCU_VERSION_INFO_API == STD_ON)
    #if (OCU_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo)
    {
        OCU_REPORT_ERROR(OCU_SID_GETVERSIONINFO, OCU_E_PARAM_POINTER);
        return;
    }
    #endif /* OCU_DEV_ERROR_DETECT */

    versioninfo->vendorID = OCU_VENDOR_ID;
    versioninfo->moduleID = OCU_MODULE_ID;
    versioninfo->sw_major_version = OCU_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = OCU_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = OCU_SW_PATCH_VERSION;
    #else
    (void)versioninfo;
    #endif /* OCU_VERSION_INFO_API */
}

#define OCU_STOP_SEC_CODE
#include "MemMap.h"
