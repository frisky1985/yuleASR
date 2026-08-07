/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*******************************************************************************
 * File Name          : LinTrcv.c
 * Description        : AUTOSAR LIN Transceiver Driver implementation
 *                      Supports TJA1021/TJA1022 transceivers with DIO/SPI control
 ******************************************************************************/

/*=============================================================================
 * Includes
 ============================================================================*/
#include "LinTrcv.h"
#include "Det.h"
#include "Dio.h"
#if (LINTRCV_SPI_SUPPORT == STD_ON)
#include "Spi.h"
#endif
#include "EcuM.h"

/*=============================================================================
 * Version Check
 ============================================================================*/
#ifndef DET_AR_RELEASE_MAJOR_VERSION
#error "DET version not defined"
#endif

#if (DET_AR_RELEASE_MAJOR_VERSION != LINTRCV_AR_RELEASE_MAJOR_VERSION)
#error "DET and LinTrcv major version mismatch"
#endif

/*=============================================================================
 * Macros
 ============================================================================*/
#define LINTRCV_IS_VALID_CHANNEL(ch)         ((ch) < LINTRCV_NUM_CHANNELS)
#define LINTRCV_IS_VALID_OPMODE(mode)        ((mode) <= LINTRCV_OPMODE_SLEEP)

/*=============================================================================
 * Internal Type Definitions
 ============================================================================*/

/* Channel runtime state structure */
typedef struct
{
    LinTrcv_ChannelStateType State;           /* Channel initialization state */
    LinTrcv_OpmodeType CurrentMode;           /* Current operation mode */
    LinTrcv_WakeupReasonType LastWuReason;    /* Last wake-up reason detected */
    boolean WakeupEventPending;               /* Wake-up event pending flag */
    boolean ModeTransitionPending;            /* Mode transition in progress */
    uint32 ModeTransitionStartTime;           /* Mode transition start timestamp */
} LinTrcv_ChannelStateStructType;

/*=============================================================================
 * Internal Variables
 ============================================================================*/
#define LINTRCV_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Pointer to the configuration */
const LinTrcv_ConfigType *LinTrcv_ConfigPtr = NULL_PTR;

/* Channel runtime states */
static LinTrcv_ChannelStateStructType LinTrcv_ChannelState[LINTRCV_MAX_CHANNELS];

/* Module initialization state */
static boolean LinTrcv_ModuleInitialized = FALSE;

#define LINTRCV_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*=============================================================================
 * Internal Function Prototypes
 ============================================================================*/
#define LINTRCV_START_SEC_CODE
#include "MemMap.h"

static Std_ReturnType LinTrcv_SetTja1021Mode(uint8 Channel, LinTrcv_OpmodeType OpMode);
static Std_ReturnType LinTrcv_GetTja1021Mode(uint8 Channel, LinTrcv_OpmodeType *OpMode);
static Std_ReturnType LinTrcv_DetectWakeupReason(uint8 Channel);
static void LinTrcv_SetDioPin(uint16 DioChannel, LinTrcv_PinStateType State);
static LinTrcv_PinStateType LinTrcv_ReadDioPin(uint16 DioChannel);
static void LinTrcv_DelayUs(uint32 DelayUs);
static Std_ReturnType LinTrcv_ValidateModeTransition(uint8 Channel, LinTrcv_OpmodeType TargetMode);

/*=============================================================================
 * Internal Function Implementations
 ============================================================================*/

/*******************************************************************************
 * Function Name : LinTrcv_SetDioPin
 * Description   : Sets a DIO pin to the specified state
 * Parameters    : DioChannel - DIO channel ID
 *               : State      - Pin state (HIGH or LOW)
 * Return        : None
 ******************************************************************************/
static void LinTrcv_SetDioPin(uint16 DioChannel, LinTrcv_PinStateType State)
{
    if (State == LINTRCV_PIN_HIGH)
    {
        Dio_WriteChannel(DioChannel, STD_HIGH);
    }
    else
    {
        Dio_WriteChannel(DioChannel, STD_LOW);
    }
}

/*******************************************************************************
 * Function Name : LinTrcv_ReadDioPin
 * Description   : Reads the state of a DIO pin
 * Parameters    : DioChannel - DIO channel ID
 * Return        : Pin state (HIGH or LOW)
 ******************************************************************************/
static LinTrcv_PinStateType LinTrcv_ReadDioPin(uint16 DioChannel)
{
    Dio_LevelType pinLevel = Dio_ReadChannel(DioChannel);
    
    if (pinLevel == STD_HIGH)
    {
        return LINTRCV_PIN_HIGH;
    }
    else
    {
        return LINTRCV_PIN_LOW;
    }
}

/*******************************************************************************
 * Function Name : LinTrcv_DelayUs
 * Description   : Delays for specified microseconds (platform specific)
 * Parameters    : DelayUs - Delay in microseconds
 * Return        : None
 ******************************************************************************/
static void LinTrcv_DelayUs(uint32 DelayUs)
{
    /* Platform-specific delay implementation */
    /* In a real implementation, this would use a timer or OS delay service */
    volatile uint32 i;
    for (i = 0; i < (DelayUs * 10U); i++)
    {
        /* Simple busy-wait loop - should be replaced with proper timing */
        __asm("nop");
    }
}

/*******************************************************************************
 * Function Name : LinTrcv_ValidateModeTransition
 * Description   : Validates if mode transition is allowed
 * Parameters    : Channel    - Transceiver channel
 *               : TargetMode - Target operation mode
 * Return        : E_OK     - Transition valid
 *               : E_NOT_OK - Transition invalid
 ******************************************************************************/
static Std_ReturnType LinTrcv_ValidateModeTransition(uint8 Channel, LinTrcv_OpmodeType TargetMode)
{
    Std_ReturnType retVal = E_OK;
    LinTrcv_OpmodeType currentMode = LinTrcv_ChannelState[Channel].CurrentMode;
    
    /* Check for invalid transitions */
    if ((currentMode == LINTRCV_OPMODE_SLEEP) && (TargetMode == LINTRCV_OPMODE_STANDBY))
    {
        /* Sleep to Standby is not a direct transition, must go via Normal */
        /* But TJA1021 allows this via EN pin toggling */
        retVal = E_OK;  /* TJA1021 specific - allowed */
    }
    
    return retVal;
}

/*******************************************************************************
 * Function Name : LinTrcv_SetTja1021Mode
 * Description   : Sets TJA1021 operation mode using EN pin
 *                 EN=1: Normal Mode
 *                 EN=0: Standby Mode (if NWake=1) or Sleep Mode (if NWake=0)
 * Parameters    : Channel - Transceiver channel
 *               : OpMode  - Target operation mode
 * Return        : E_OK     - Mode set successfully
 *               : E_NOT_OK - Mode set failed
 ******************************************************************************/
static Std_ReturnType LinTrcv_SetTja1021Mode(uint8 Channel, LinTrcv_OpmodeType OpMode)
{
    Std_ReturnType retVal = E_OK;
    const LinTrcv_ChannelConfigType *channelCfg = &LinTrcv_ConfigPtr->ChannelCfg[Channel];
    
    /* Validate mode transition */
    if (LinTrcv_ValidateModeTransition(Channel, OpMode) != E_OK)
    {
        return E_NOT_OK;
    }
    
    switch (OpMode)
    {
        case LINTRCV_OPMODE_NORMAL:
            /* Set EN pin HIGH for Normal mode */
            LinTrcv_SetDioPin(channelCfg->EnPinDio, LINTRCV_PIN_HIGH);
            
            /* Wait for mode transition */
            if (LinTrcv_ChannelState[Channel].CurrentMode == LINTRCV_OPMODE_SLEEP)
            {
                LinTrcv_DelayUs(channelCfg->SleepToNormalDelay);
            }
            else if (LinTrcv_ChannelState[Channel].CurrentMode == LINTRCV_OPMODE_STANDBY)
            {
                LinTrcv_DelayUs(channelCfg->StandbyToNormalDelay);
            }
            break;
            
        case LINTRCV_OPMODE_STANDBY:
            /* Set EN pin LOW, NWake pin HIGH for Standby mode */
            LinTrcv_SetDioPin(channelCfg->EnPinDio, LINTRCV_PIN_LOW);
            
            /* Ensure NWake is HIGH (inactive) to enter Standby, not Sleep */
            if ((channelCfg->WakeupByPinEnabled) != 0U)
            {
                /* NWake is input, so we rely on external pull-up */
                /* For TJA1021, Standby requires EN=0 and sufficient time */
            }
            
            LinTrcv_DelayUs(channelCfg->NormalToStandbyDelay);
            break;
            
        case LINTRCV_OPMODE_SLEEP:
            /* Set EN pin LOW for Sleep mode */
            LinTrcv_SetDioPin(channelCfg->EnPinDio, LINTRCV_PIN_LOW);
            
            /* TJA1021 enters Sleep mode when EN=0 and NWake=0 or after timeout */
            /* NWake is input, Sleep is automatic after standby timeout or NWake=0 */
            
            LinTrcv_DelayUs(channelCfg->NormalToSleepDelay);
            break;
            
        default:
            retVal = E_NOT_OK;
            break;
    }
    
    if (retVal == E_OK)
    {
        /* Update current mode */
        LinTrcv_ChannelState[Channel].CurrentMode = OpMode;
        LinTrcv_ChannelState[Channel].ModeTransitionPending = FALSE;
    }
    
    return retVal;
}

/*******************************************************************************
 * Function Name : LinTrcv_GetTja1021Mode
 * Description   : Gets TJA1021 current operation mode by reading status
 *                 Infers mode from EN pin and error pin states
 * Parameters    : Channel - Transceiver channel
 *               : OpMode  - Pointer to store operation mode
 * Return        : E_OK     - Mode read successfully
 *               : E_NOT_OK - Failed to read mode
 ******************************************************************************/
static Std_ReturnType LinTrcv_GetTja1021Mode(uint8 Channel, LinTrcv_OpmodeType *OpMode)
{
    Std_ReturnType retVal = E_OK;
    const LinTrcv_ChannelConfigType *channelCfg = &LinTrcv_ConfigPtr->ChannelCfg[Channel];
    LinTrcv_PinStateType enPinState;
    
    /* Read EN pin state */
    enPinState = LinTrcv_ReadDioPin(channelCfg->EnPinDio);
    
    if (enPinState == LINTRCV_PIN_HIGH)
    {
        /* EN=1 indicates Normal mode */
        *OpMode = LINTRCV_OPMODE_NORMAL;
    }
    else
    {
        /* EN=0 could be Standby or Sleep mode */
        /* TJA1021 doesn't provide direct status indication */
        /* We rely on the last known mode or infer from NWake/NERR */
        
        /* For now, use the tracked state */
        *OpMode = LinTrcv_ChannelState[Channel].CurrentMode;
        
        /* If last mode was Normal and EN=0, we're in Standby (transition state) */
        if (*OpMode == LINTRCV_OPMODE_NORMAL)
        {
            *OpMode = LINTRCV_OPMODE_STANDBY;
        }
    }
    
    return retVal;
}

/*******************************************************************************
 * Function Name : LinTrcv_DetectWakeupReason
 * Description   : Detects the wake-up reason by checking pins and status
 * Parameters    : Channel - Transceiver channel
 * Return        : E_OK     - Detection successful
 *               : E_NOT_OK - Detection failed
 ******************************************************************************/
static Std_ReturnType LinTrcv_DetectWakeupReason(uint8 Channel)
{
    Std_ReturnType retVal = E_OK;
    const LinTrcv_ChannelConfigType *channelCfg = &LinTrcv_ConfigPtr->ChannelCfg[Channel];
    LinTrcv_WakeupReasonType wakeupReason = LINTRCV_WU_ERROR;
    
    if ((channelCfg->WakeupByBusEnabled) != 0U)
    {
        /* Check for bus wake-up - detected via mode change from Standby/Sleep to Normal */
        /* In TJA1021, bus activity wakes up the transceiver */
        if ((LinTrcv_ChannelState[Channel].WakeupEventPending) != 0U)
        {
            wakeupReason = LINTRCV_WU_BY_BUS;
        }
    }
    
    if ((channelCfg->WakeupByPinEnabled) != 0U)
    {
        /* Check NWake pin for local wake-up */
        LinTrcv_PinStateType nWakeState = LinTrcv_ReadDioPin(channelCfg->NwadrsPinDio);
        
        if (nWakeState == LINTRCV_TJA1021_NWAKE_ACTIVE)
        {
            /* NWake is active low - local wake-up detected */
            wakeupReason = LINTRCV_WU_BY_PIN;
        }
    }
    
    /* Check for error conditions */
    if (channelCfg->NerrPinDio != 0xFFFFU)
    {
        LinTrcv_PinStateType nErrState = LinTrcv_ReadDioPin(channelCfg->NerrPinDio);
        
        if (nErrState == LINTRCV_TJA1021_NERR_ERROR)
        {
            /* Error condition detected */
            wakeupReason = LINTRCV_WU_BY_SYSERR;
        }
    }
    
    /* Store the detected wake-up reason */
    LinTrcv_ChannelState[Channel].LastWuReason = wakeupReason;
    
    return retVal;
}

/*=============================================================================
 * API Function Implementations
 ============================================================================*/

/*******************************************************************************
 * Function Name : LinTrcv_Init
 * Description   : Initializes the LIN transceiver driver
 ******************************************************************************/
void LinTrcv_Init(const LinTrcv_ConfigType *ConfigPtr)
{
    uint8 i;
    
#if (LINTRCV_DEV_ERROR_DETECT == STD_ON)
    /* Check for NULL_PTR pointer if static config is not used */
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_INIT, LINTRCV_E_PARAM_CONFIG);
        return;
    }
#endif
    
    /* Store configuration pointer */
    LinTrcv_ConfigPtr = ConfigPtr;
    
    /* Initialize all channels */
    for (i = 0U; i < ConfigPtr->NumChannels; i++)
    {
        const LinTrcv_ChannelConfigType *channelCfg = &ConfigPtr->ChannelCfg[i];
        
        /* Initialize channel state */
        LinTrcv_ChannelState[i].State = LINTRCV_CHANNEL_INIT;
        LinTrcv_ChannelState[i].LastWuReason = LINTRCV_WU_RESET;
        LinTrcv_ChannelState[i].WakeupEventPending = FALSE;
        LinTrcv_ChannelState[i].ModeTransitionPending = FALSE;
        
        /* Set initial mode */
        if (channelCfg->InitialMode == LINTRCV_OPMODE_NORMAL)
        {
            (void)LinTrcv_SetTja1021Mode(i, LINTRCV_OPMODE_NORMAL);
        }
        else if (channelCfg->InitialMode == LINTRCV_OPMODE_STANDBY)
        {
            (void)LinTrcv_SetTja1021Mode(i, LINTRCV_OPMODE_STANDBY);
        }
        else if (channelCfg->InitialMode == LINTRCV_OPMODE_SLEEP)
        {
            (void)LinTrcv_SetTja1021Mode(i, LINTRCV_OPMODE_SLEEP);
        }
        else
        {
            /* Default to Normal mode */
            (void)LinTrcv_SetTja1021Mode(i, LINTRCV_OPMODE_NORMAL);
        }
        
        LinTrcv_ChannelState[i].CurrentMode = channelCfg->InitialMode;
    }
    
    /* Mark module as initialized */
    LinTrcv_ModuleInitialized = TRUE;
}

/*******************************************************************************
 * Function Name : LinTrcv_DeInit
 * Description   : De-initializes the LIN transceiver driver
 ******************************************************************************/
void LinTrcv_DeInit(void)
{
    uint8 i;
    
    if (LinTrcv_ModuleInitialized == FALSE)
    {
        return;
    }
    
    /* Put all channels in Standby mode */
    for (i = 0U; i < LinTrcv_ConfigPtr->NumChannels; i++)
    {
        (void)LinTrcv_SetTja1021Mode(i, LINTRCV_OPMODE_STANDBY);
        LinTrcv_ChannelState[i].State = LINTRCV_CHANNEL_UNINIT;
    }
    
    /* Clear configuration pointer */
    LinTrcv_ConfigPtr = NULL_PTR;
    LinTrcv_ModuleInitialized = FALSE;
}

/*******************************************************************************
 * Function Name : LinTrcv_SetOpMode
 * Description   : Sets the operation mode of a LIN transceiver channel
 ******************************************************************************/
Std_ReturnType LinTrcv_SetOpMode(uint8 Channel, LinTrcv_OpmodeType OpMode)
{
    Std_ReturnType retVal = E_OK;
    
#if (LINTRCV_DEV_ERROR_DETECT == STD_ON)
    /* Check module initialization */
    if (LinTrcv_ModuleInitialized == FALSE)
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_SETOPMODE, LINTRCV_E_UNINIT);
        return E_NOT_OK;
    }
    
    /* Check channel validity */
    if (!LINTRCV_IS_VALID_CHANNEL(Channel))
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_SETOPMODE, LINTRCV_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    
    /* Check operation mode validity */
    if (!LINTRCV_IS_VALID_OPMODE(OpMode))
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_SETOPMODE, LINTRCV_E_INVALID_OPMODE);
        return E_NOT_OK;
    }
#endif
    
    /* Set the operation mode based on hardware type */
    const LinTrcv_ChannelConfigType *channelCfg = &LinTrcv_ConfigPtr->ChannelCfg[Channel];
    
    switch (channelCfg->HwType)
    {
        case LINTRCV_TJA1021:
        case LINTRCV_TJA1022:
            retVal = LinTrcv_SetTja1021Mode(Channel, OpMode);
            break;
            
        case LINTRCV_TJA1028:
        case LINTRCV_GENERIC:
            /* Use same control method as TJA1021 for generic devices */
            retVal = LinTrcv_SetTja1021Mode(Channel, OpMode);
            break;
            
        default:
            retVal = E_NOT_OK;
            break;
    }
    
    return retVal;
}

/*******************************************************************************
 * Function Name : LinTrcv_GetOpMode
 * Description   : Gets the current operation mode of a LIN transceiver channel
 ******************************************************************************/
Std_ReturnType LinTrcv_GetOpMode(uint8 Channel, LinTrcv_OpmodeType *OpMode)
{
    Std_ReturnType retVal = E_OK;
    
#if (LINTRCV_DEV_ERROR_DETECT == STD_ON)
    /* Check module initialization */
    if (LinTrcv_ModuleInitialized == FALSE)
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_GETOPMODE, LINTRCV_E_UNINIT);
        return E_NOT_OK;
    }
    
    /* Check channel validity */
    if (!LINTRCV_IS_VALID_CHANNEL(Channel))
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_GETOPMODE, LINTRCV_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    
    /* Check NULL_PTR pointer */
    if (OpMode == NULL_PTR)
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_GETOPMODE, LINTRCV_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    
    /* Get the operation mode based on hardware type */
    const LinTrcv_ChannelConfigType *channelCfg = &LinTrcv_ConfigPtr->ChannelCfg[Channel];
    
    switch (channelCfg->HwType)
    {
        case LINTRCV_TJA1021:
        case LINTRCV_TJA1022:
            retVal = LinTrcv_GetTja1021Mode(Channel, OpMode);
            break;
            
        case LINTRCV_TJA1028:
        case LINTRCV_GENERIC:
            /* Use same method as TJA1021 for generic devices */
            retVal = LinTrcv_GetTja1021Mode(Channel, OpMode);
            break;
            
        default:
            *OpMode = LinTrcv_ChannelState[Channel].CurrentMode;
            break;
    }
    
    return retVal;
}

/*******************************************************************************
 * Function Name : LinTrcv_GetBusWuReason
 * Description   : Gets the wake-up reason for the specified channel
 ******************************************************************************/
Std_ReturnType LinTrcv_GetBusWuReason(uint8 Channel, LinTrcv_WakeupReasonType *WuReason)
{
    Std_ReturnType retVal = E_OK;
    
#if (LINTRCV_DEV_ERROR_DETECT == STD_ON)
    /* Check module initialization */
    if (LinTrcv_ModuleInitialized == FALSE)
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_GETBUSWUREASON, LINTRCV_E_UNINIT);
        return E_NOT_OK;
    }
    
    /* Check channel validity */
    if (!LINTRCV_IS_VALID_CHANNEL(Channel))
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_GETBUSWUREASON, LINTRCV_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    
    /* Check NULL_PTR pointer */
    if (WuReason == NULL_PTR)
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_GETBUSWUREASON, LINTRCV_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    
    /* Return the last detected wake-up reason */
    *WuReason = LinTrcv_ChannelState[Channel].LastWuReason;
    
    return retVal;
}

/*******************************************************************************
 * Function Name : LinTrcv_GetVersionInfo
 * Description   : Returns version information of the LIN transceiver driver
 ******************************************************************************/
#if (LINTRCV_VERSION_INFO_API == STD_ON)
void LinTrcv_GetVersionInfo(Std_VersionInfoType *VersionInfo)
{
#if (LINTRCV_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfo == NULL_PTR)
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_GETVERSIONINFO, LINTRCV_E_PARAM_POINTER);
        return;
    }
#endif
    
    VersionInfo->vendorID = LINTRCV_VENDOR_ID;
    VersionInfo->moduleID = LINTRCV_MODULE_ID;
    VersionInfo->sw_major_version = LINTRCV_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = LINTRCV_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = LINTRCV_SW_PATCH_VERSION;
}
#endif

/*******************************************************************************
 * Function Name : LinTrcv_Wakeup
 * Description   : Initiates wake-up on the specified channel
 ******************************************************************************/
Std_ReturnType LinTrcv_Wakeup(uint8 Channel)
{
    Std_ReturnType retVal = E_OK;
    
#if (LINTRCV_DEV_ERROR_DETECT == STD_ON)
    /* Check module initialization */
    if (LinTrcv_ModuleInitialized == FALSE)
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_WAKEUP, LINTRCV_E_UNINIT);
        return E_NOT_OK;
    }
    
    /* Check channel validity */
    if (!LINTRCV_IS_VALID_CHANNEL(Channel))
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_WAKEUP, LINTRCV_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif
    
    /* Wake-up from Sleep or Standby to Normal mode */
    retVal = LinTrcv_SetTja1021Mode(Channel, LINTRCV_OPMODE_NORMAL);
    
    return retVal;
}

/*******************************************************************************
 * Function Name : LinTrcv_CheckWakeup
 * Description   : Checks if wake-up event occurred on specified channel
 ******************************************************************************/
Std_ReturnType LinTrcv_CheckWakeup(uint8 Channel)
{
    Std_ReturnType retVal = E_NOT_OK;
    
#if (LINTRCV_DEV_ERROR_DETECT == STD_ON)
    /* Check module initialization */
    if (LinTrcv_ModuleInitialized == FALSE)
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_CHECKWAKEUP, LINTRCV_E_UNINIT);
        return E_NOT_OK;
    }
    
    /* Check channel validity */
    if (!LINTRCV_IS_VALID_CHANNEL(Channel))
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_CHECKWAKEUP, LINTRCV_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif
    
    /* Detect wake-up reason */
    (void)LinTrcv_DetectWakeupReason(Channel);
    
    /* Check if wake-up event is pending */
    if ((LinTrcv_ChannelState[Channel].WakeupEventPending) != 0U)
    {
        retVal = E_OK;
        
        /* Notify EcuM about wake-up event */
        const LinTrcv_ChannelConfigType *channelCfg = &LinTrcv_ConfigPtr->ChannelCfg[Channel];
        if (channelCfg->WakeupSourceRef != 0U)
        {
            EcuM_SetWakeupEvent(channelCfg->WakeupSourceRef);
        }
        
        /* Clear pending flag */
        LinTrcv_ChannelState[Channel].WakeupEventPending = FALSE;
    }
    
    return retVal;
}

/*******************************************************************************
 * Function Name : LinTrcv_Cbk_WakeupByBus
 * Description   : Callback for wake-up by bus notification
 ******************************************************************************/
void LinTrcv_Cbk_WakeupByBus(uint8 Channel)
{
#if (LINTRCV_DEV_ERROR_DETECT == STD_ON)
    if (LinTrcv_ModuleInitialized == FALSE)
    {
        return;
    }
    
    if (!LINTRCV_IS_VALID_CHANNEL(Channel))
    {
        Det_ReportError(LINTRCV_MODULE_ID, 0U, LINTRCV_SID_CBK_WAKEUPBYBUS, LINTRCV_E_INVALID_CHANNEL);
        return;
    }
#endif
    
    /* Set wake-up event pending */
    LinTrcv_ChannelState[Channel].WakeupEventPending = TRUE;
    LinTrcv_ChannelState[Channel].LastWuReason = LINTRCV_WU_BY_BUS;
    
    /* Notify EcuM if configured */
    const LinTrcv_ChannelConfigType *channelCfg = &LinTrcv_ConfigPtr->ChannelCfg[Channel];
    if (channelCfg->WakeupSourceRef != 0U)
    {
        EcuM_SetWakeupEvent(channelCfg->WakeupSourceRef);
    }
}

/*******************************************************************************
 * Function Name : LinTrcv_MainFunction
 * Description   : Main function for periodic wake-up detection
 ******************************************************************************/
void LinTrcv_MainFunction(void)
{
    uint8 i;
    
    if (LinTrcv_ModuleInitialized == FALSE)
    {
        return;
    }
    
    /* Check all channels for wake-up events */
    for (i = 0U; i < LinTrcv_ConfigPtr->NumChannels; i++)
    {
        const LinTrcv_ChannelConfigType *channelCfg = &LinTrcv_ConfigPtr->ChannelCfg[i];
        
        /* Check for bus wake-up (mode change from Standby/Sleep) */
        if ((LinTrcv_ChannelState[i].CurrentMode == LINTRCV_OPMODE_STANDBY) ||
            (LinTrcv_ChannelState[i].CurrentMode == LINTRCV_OPMODE_SLEEP))
        {
            /* Read actual mode from hardware */
            LinTrcv_OpmodeType actualMode;
            if (LinTrcv_GetTja1021Mode(i, &actualMode) == E_OK)
            {
                if (actualMode == LINTRCV_OPMODE_NORMAL)
                {
                    /* Transceiver woke up */
                    LinTrcv_ChannelState[i].WakeupEventPending = TRUE;
                    LinTrcv_ChannelState[i].LastWuReason = LINTRCV_WU_BY_BUS;
                    LinTrcv_ChannelState[i].CurrentMode = LINTRCV_OPMODE_NORMAL;
                    
                    /* Notify EcuM */
                    if (channelCfg->WakeupSourceRef != 0U)
                    {
                        EcuM_SetWakeupEvent(channelCfg->WakeupSourceRef);
                    }
                }
            }
        }
        
        /* Check NWake pin for local wake-up */
        if ((channelCfg->WakeupByPinEnabled) != 0U)
        {
            LinTrcv_PinStateType nWakeState = LinTrcv_ReadDioPin(channelCfg->NwadrsPinDio);
            
            if (nWakeState == LINTRCV_TJA1021_NWAKE_ACTIVE)
            {
                /* Local wake-up detected */
                LinTrcv_ChannelState[i].WakeupEventPending = TRUE;
                LinTrcv_ChannelState[i].LastWuReason = LINTRCV_WU_BY_PIN;
                
                /* Notify EcuM */
                if (channelCfg->WakeupSourceRef != 0U)
                {
                    EcuM_SetWakeupEvent(channelCfg->WakeupSourceRef);
                }
            }
        }
    }
}

#define LINTRCV_STOP_SEC_CODE
#include "MemMap.h"
