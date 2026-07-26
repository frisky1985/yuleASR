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

/**
 * @file Icu.c
 * @brief ICU (Input Capture Unit) Driver implementation for S32K312 (eMIOS)
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "Icu.h"
#include "Icu_Cfg.h"
#include "Det.h"

/* REG_MODIFY32 convenience macro - uses global REG_READ32/REG_WRITE32 from Std_Types.h */
#define REG_MODIFY32(address, mask, value)      (REG_WRITE32((address), (REG_READ32(address) & ~(mask)) | ((value) & (mask))))

/*==================================================================================================
*                                    EMIOS REGISTER DEFINITIONS
==================================================================================================*/
/* eMIOS Module Registers (per module) */
#define EMIOS_MCR                               (0x0000U)  /* Module Configuration Register */
#define EMIOS_GFR                               (0x0004U)  /* Global Flag Register */
#define EMIOS_OUDR                              (0x0008U)  /* Output Update Disable Register */
#define EMIOS_UCDIS                             (0x000CU)  /* Unified Channel Disable Register */

/* eMIOS Channel Registers (per channel - offset from channel base) */
#define EMIOS_C_A                               (0x0000U)  /* Channel A Register */
#define EMIOS_C_B                               (0x0004U)  /* Channel B Register */
#define EMIOS_C_CNT                             (0x0008U)  /* Channel Counter Register */
#define EMIOS_C_C                               (0x000CU)  /* Channel Control Register */
#define EMIOS_C_S                               (0x0010U)  /* Channel Status Register */
#define EMIOS_C_ALTA                            (0x0014U)  /* Channel Alternate A Register */

/* Channel size: 0x20 bytes per channel */
#define EMIOS_CHANNEL_SIZE                      (0x20U)

/* eMIOS Channel Control Register (C) Bit Definitions */
#define EMIOS_C_FEN                             (0x00000001U)  /* Flag Enable */
#define EMIOS_C_FCK                             (0x00000002U)  /* Filter Clock */
#define EMIOS_C_IF_MASK                         (0x0000000CU)  /* Input Filter */
#define EMIOS_IF_SHIFT                          (2U)
#define EMIOS_C_EDPOL                           (0x00000040U)  /* Edge Polarity */
#define EMIOS_C_EDSEL                           (0x00000080U)  /* Edge Select */
#define EMIOS_C_MODE_MASK                       (0x0000F800U)  /* Mode Select */
#define EMIOS_MODE_SHIFT                        (11U)
#define EMIOS_C_DMA                             (0x00010000U)  /* DMA Enable */
#define EMIOS_C_UCPRE_MASK                      (0x00060000U)  /* Prescaler */
#define EMIOS_UCPRE_SHIFT                       (17U)
#define EMIOS_C_UCPREN                          (0x00100000U)  /* Prescaler Enable */
#define EMIOS_C_FREN                            (0x00200000U)  /* Freeze Enable */

/* eMIOS Channel Status Register (S) Bit Definitions */
#define EMIOS_S_FLAG                            (0x00000001U)  /* Flag */
#define EMIOS_S_UCOUT                           (0x00010000U)  /* Output State */
#define EMIOS_S_UCIN                            (0x00020000U)  /* Input State */

/* eMIOS Mode Values */
#define EMIOS_MODE_GPIO_INPUT                   (0x00U)
#define EMIOS_MODE_GPIO_OUTPUT                  (0x01U)
#define EMIOS_MODE_SAIC                         (0x03U)  /* Single Action Input Capture */
#define EMIOS_MODE_IPWM                         (0x04U)  /* Input Pulse Width Measurement */
#define EMIOS_MODE_IPM                          (0x05U)  /* Input Period Measurement */
#define EMIOS_MODE_DAOC                         (0x06U)  /* Double Action Output Compare */
#define EMIOS_MODE_SAOC                         (0x07U)  /* Single Action Output Compare */
#define EMIOS_MODE_EMCB                         (0x10U)  /* Event Counting - Buffered */

/*==================================================================================================
*                                    INTERNAL DEFINITIONS
==================================================================================================*/
#define ICU_EMIOS_NUM_CHANNELS                  (24U)
#define ICU_EMIOS_CHANNELS_PER_INSTANCE         (12U)

/*==================================================================================================
*                                    INTERNAL VARIABLES
==================================================================================================*/
#define ICU_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Icu_DriverInitialized = FALSE;
static Icu_ModeType Icu_DriverMode = ICU_MODE_NORMAL;
static const Icu_ConfigType* Icu_ConfigPtr = NULL_PTR;

/* Channel runtime data */
static Icu_InputStateType Icu_ChannelInputState[ICU_NUM_CHANNELS];
static Icu_ActivationType Icu_ChannelActivation[ICU_NUM_CHANNELS];
static boolean Icu_ChannelNotificationEnabled[ICU_NUM_CHANNELS];
static boolean Icu_ChannelRunning[ICU_NUM_CHANNELS];
static boolean Icu_ChannelWakeupEnabled[ICU_NUM_CHANNELS];

/* Timestamp data */
static uint32* Icu_TimestampBuffer[ICU_NUM_CHANNELS];
static uint16 Icu_TimestampBufferSize[ICU_NUM_CHANNELS];
static Icu_IndexType Icu_TimestampIndex[ICU_NUM_CHANNELS];
static uint16 Icu_TimestampNotifyInterval[ICU_NUM_CHANNELS];
static uint16 Icu_TimestampCaptureCount[ICU_NUM_CHANNELS];

/* Edge counting data */
static uint16 Icu_EdgeCount[ICU_NUM_CHANNELS];
static boolean Icu_EdgeCountEnabled[ICU_NUM_CHANNELS];

/* Signal measurement data */
static uint16 Icu_SignalPeriodTime[ICU_NUM_CHANNELS];
static uint16 Icu_SignalActiveTime[ICU_NUM_CHANNELS];
static uint16 Icu_SignalLastCapture[ICU_NUM_CHANNELS];
static boolean Icu_SignalMeasurementRunning[ICU_NUM_CHANNELS];

#define ICU_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    INTERNAL FUNCTIONS
==================================================================================================*/
#define ICU_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Get eMIOS base address for a channel
 */
static uint32 Icu_GetEmiosBaseAddr(Icu_ChannelType channel)
{
    if (channel < (ICU_NUM_CHANNELS / 2U)) {
        return ICU_EMIOS_0_BASE_ADDR;
    } else {
        return ICU_EMIOS_1_BASE_ADDR;
    }
}

/**
 * @brief Get eMIOS channel number within an instance
 */
static uint8 Icu_GetEmiosChannelNum(Icu_ChannelType channel)
{
    if (channel < (ICU_NUM_CHANNELS / 2U)) {
        return (uint8)channel;
    } else {
        return (uint8)(channel - (ICU_NUM_CHANNELS / 2U));
    }
}

/**
 * @brief Get channel register address
 */
static uint32 Icu_GetChannelRegAddr(uint32 emiosBase, uint8 chNum)
{
    return emiosBase + 0x20U + (chNum * EMIOS_CHANNEL_SIZE);
}

/**
 * @brief Configure eMIOS mode for ICU
 */
static void Icu_ConfigureEmiosMode(Icu_ChannelType channel, uint8 mode)
{
    uint32 emiosBase = Icu_GetEmiosBaseAddr(channel);
    uint8 chNum = Icu_GetEmiosChannelNum(channel);
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    uint32 cReg = REG_READ32(chAddr + EMIOS_C_C);
    
    /* Clear mode bits and set new mode */
    cReg &= ~EMIOS_C_MODE_MASK;
    cReg |= ((uint32)mode << EMIOS_MODE_SHIFT);
    REG_WRITE32(chAddr + EMIOS_C_C, cReg);
}

/**
 * @brief Enable channel interrupt
 */
static void Icu_EnableChannelInterrupt(Icu_ChannelType channel)
{
    uint32 emiosBase = Icu_GetEmiosBaseAddr(channel);
    uint8 chNum = Icu_GetEmiosChannelNum(channel);
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    uint32 cReg = REG_READ32(chAddr + EMIOS_C_C);
    
    cReg |= EMIOS_C_FEN;  /* Enable flag (interrupt) */
    REG_WRITE32(chAddr + EMIOS_C_C, cReg);
}

/**
 * @brief Disable channel interrupt
 */
static void Icu_DisableChannelInterrupt(Icu_ChannelType channel)
{
    uint32 emiosBase = Icu_GetEmiosBaseAddr(channel);
    uint8 chNum = Icu_GetEmiosChannelNum(channel);
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    uint32 cReg = REG_READ32(chAddr + EMIOS_C_C);
    
    cReg &= ~EMIOS_C_FEN;  /* Disable flag (interrupt) */
    REG_WRITE32(chAddr + EMIOS_C_C, cReg);
}

/**
 * @brief Clear channel flag
 */
static void Icu_ClearChannelFlag(Icu_ChannelType channel)
{
    uint32 emiosBase = Icu_GetEmiosBaseAddr(channel);
    uint8 chNum = Icu_GetEmiosChannelNum(channel);
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    
    REG_WRITE32(chAddr + EMIOS_C_S, EMIOS_S_FLAG);
}

/**
 * @brief Get channel input state
 */
static uint32 Icu_GetChannelFlag(Icu_ChannelType channel)
{
    uint32 emiosBase = Icu_GetEmiosBaseAddr(channel);
    uint8 chNum = Icu_GetEmiosChannelNum(channel);
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    uint32 sReg = REG_READ32(chAddr + EMIOS_C_S);
    
    return (sReg & EMIOS_S_FLAG);
}

/**
 * @brief Process signal measurement for a channel
 */
static void Icu_ProcessSignalMeasurement(Icu_ChannelType channel, uint16 currentCapture)
{
    uint16 period;
    uint16 activeTime;
    
    if (Icu_SignalLastCapture[channel] == 0U) {
        /* First capture - just store it */
        Icu_SignalLastCapture[channel] = currentCapture;
        return;
    }
    
    /* Calculate period */
    if (currentCapture >= Icu_SignalLastCapture[channel]) {
        period = currentCapture - Icu_SignalLastCapture[channel];
    } else {
        /* Counter overflow */
        period = (0xFFFFU - Icu_SignalLastCapture[channel]) + currentCapture;
    }
    
    Icu_SignalLastCapture[channel] = currentCapture;
    
    /* Store results based on measurement property */
    switch (Icu_ConfigPtr->Channels[channel].SignalMeasurementProperty) {
        case ICU_PERIOD_TIME:
            Icu_SignalPeriodTime[channel] = period;
            break;
            
        case ICU_HIGH_TIME:
            Icu_SignalActiveTime[channel] = period;
            break;
            
        case ICU_LOW_TIME:
            Icu_SignalActiveTime[channel] = period;
            break;
            
        case ICU_DUTY_CYCLE:
            /* For duty cycle, we need both period and active time */
            Icu_SignalPeriodTime[channel] = period;
            /* Active time is calculated from edge to edge based on polarity */
            activeTime = period / 2U;  /* Simplified - actual implementation would track edges */
            Icu_SignalActiveTime[channel] = activeTime;
            break;
            
        default:
            /* Do nothing */
            break;
    }
}

/*==================================================================================================
*                                    API IMPLEMENTATION
==================================================================================================*/

void Icu_Init(const Icu_ConfigType* ConfigPtr)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_INIT, ICU_E_PARAM_POINTER);
        return;
    }
    if (Icu_DriverInitialized == TRUE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_INIT, ICU_E_ALREADY_INITIALIZED);
        return;
    }
    #endif
    
    Icu_ConfigPtr = ConfigPtr;
    
    /* Initialize all configured channels */
    for (uint8 i = 0U; i < ConfigPtr->NumChannels; i++) {
        const Icu_ChannelConfigType* chConfig = &ConfigPtr->Channels[i];
        Icu_ChannelType channel = chConfig->ChannelId;
        
        if (channel >= ICU_NUM_CHANNELS) {
            continue;
        }
        
        uint32 emiosBase = Icu_GetEmiosBaseAddr(channel);
        uint8 chNum = Icu_GetEmiosChannelNum(channel);
        uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
        
        /* Reset channel */
        REG_WRITE32(chAddr + EMIOS_C_C, 0U);
        REG_WRITE32(chAddr + EMIOS_C_S, EMIOS_S_FLAG);  /* Clear flag */
        REG_WRITE32(chAddr + EMIOS_C_A, 0U);
        REG_WRITE32(chAddr + EMIOS_C_B, 0U);
        
        /* Configure channel based on measurement mode */
        uint32 cReg = 0U;
        
        /* Set prescaler */
        if (chConfig->ClockPrescaler > 0U) {
            cReg |= EMIOS_C_UCPREN;
            cReg |= (((chConfig->ClockPrescaler - 1U) & 0x03U) << EMIOS_UCPRE_SHIFT);
        }
        
        /* Set input filter */
        cReg |= (0x02U << EMIOS_IF_SHIFT);  /* Medium filter */
        
        /* Set activation condition (edge) */
        switch (chConfig->DefaultActivation) {
            case ICU_RISING_EDGE:
                cReg |= EMIOS_C_EDPOL;  /* Rising edge */
                cReg &= ~EMIOS_C_EDSEL;
                break;
            case ICU_FALLING_EDGE:
                cReg &= ~EMIOS_C_EDPOL;  /* Falling edge */
                cReg &= ~EMIOS_C_EDSEL;
                break;
            case ICU_BOTH_EDGES:
                cReg |= EMIOS_C_EDPOL;  /* Both edges */
                cReg |= EMIOS_C_EDSEL;
                break;
            default:
                cReg |= EMIOS_C_EDPOL;
                break;
        }
        
        /* Set mode based on measurement type */
        switch (chConfig->MeasurementMode) {
            case ICU_MODE_SIGNAL_EDGE_DETECT:
                cReg |= ((uint32)EMIOS_MODE_SAIC << EMIOS_MODE_SHIFT);
                break;
                
            case ICU_MODE_SIGNAL_MEASUREMENT:
                if (chConfig->SignalMeasurementProperty == ICU_DUTY_CYCLE) {
                    cReg |= ((uint32)EMIOS_MODE_IPWM << EMIOS_MODE_SHIFT);
                } else {
                    cReg |= ((uint32)EMIOS_MODE_IPM << EMIOS_MODE_SHIFT);
                }
                break;
                
            case ICU_MODE_TIMESTAMP:
                cReg |= ((uint32)EMIOS_MODE_SAIC << EMIOS_MODE_SHIFT);
                break;
                
            case ICU_MODE_EDGE_COUNTER:
                cReg |= ((uint32)EMIOS_MODE_EMCB << EMIOS_MODE_SHIFT);
                break;
                
            default:
                cReg |= ((uint32)EMIOS_MODE_GPIO_INPUT << EMIOS_MODE_SHIFT);
                break;
        }
        
        REG_WRITE32(chAddr + EMIOS_C_C, cReg);
        
        /* Initialize runtime data */
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
        
        /* Clear any pending flag */
        Icu_ClearChannelFlag(channel);
    }
    
    /* Enable eMIOS modules */
    REG_WRITE32(ICU_EMIOS_0_BASE_ADDR + EMIOS_MCR, 0x00000001U);  /* Global enable */
    REG_WRITE32(ICU_EMIOS_1_BASE_ADDR + EMIOS_MCR, 0x00000001U);
    
    Icu_DriverMode = ConfigPtr->DefaultMode;
    Icu_DriverInitialized = TRUE;
}

#if (ICU_DE_INIT_API == STD_ON)
void Icu_DeInit(void)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DEINIT, ICU_E_UNINIT);
        return;
    }
    #endif
    
    /* Deinitialize all channels */
    for (uint8 i = 0U; i < Icu_ConfigPtr->NumChannels; i++) {
        Icu_ChannelType channel = Icu_ConfigPtr->Channels[i].ChannelId;
        uint32 emiosBase = Icu_GetEmiosBaseAddr(channel);
        uint8 chNum = Icu_GetEmiosChannelNum(channel);
        uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
        
        /* Disable channel */
        REG_WRITE32(chAddr + EMIOS_C_C, 0U);
        REG_WRITE32(chAddr + EMIOS_C_S, EMIOS_S_FLAG);
        
        Icu_ChannelRunning[channel] = FALSE;
    }
    
    /* Disable eMIOS modules */
    REG_WRITE32(ICU_EMIOS_0_BASE_ADDR + EMIOS_MCR, 0x00000000U);
    REG_WRITE32(ICU_EMIOS_1_BASE_ADDR + EMIOS_MCR, 0x00000000U);
    
    Icu_DriverInitialized = FALSE;
}
#endif

#if (ICU_SET_MODE_API == STD_ON)
void Icu_SetMode(Icu_ModeType Mode)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_SETMODE, ICU_E_UNINIT);
        return;
    }
    if ((Mode != ICU_MODE_NORMAL) && (Mode != ICU_MODE_SLEEP)) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_SETMODE, ICU_E_PARAM_POINTER);
        return;
    }
    #endif
    
    if (Mode == ICU_MODE_SLEEP) {
        /* Stop all running channels */
        for (uint8 i = 0U; i < Icu_ConfigPtr->NumChannels; i++) {
            Icu_ChannelType channel = Icu_ConfigPtr->Channels[i].ChannelId;
            if (Icu_ChannelRunning[channel]) {
                Icu_DisableChannelInterrupt(channel);
                Icu_ChannelRunning[channel] = FALSE;
            }
        }
    } else {
        /* Resume channels if needed */
        for (uint8 i = 0U; i < Icu_ConfigPtr->NumChannels; i++) {
            Icu_ChannelType channel = Icu_ConfigPtr->Channels[i].ChannelId;
            if (Icu_ChannelNotificationEnabled[channel]) {
                Icu_EnableChannelInterrupt(channel);
            }
        }
    }
    
    Icu_DriverMode = Mode;
}
#endif

#if (ICU_DISABLE_WAKEUP_API == STD_ON)
void Icu_DisableWakeup(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLEWAKEUP, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLEWAKEUP, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    Icu_ChannelWakeupEnabled[Channel] = FALSE;
}
#else
void Icu_DisableWakeup(Icu_ChannelType Channel)
{
    (void)Channel;
}
#endif

#if (ICU_ENABLE_WAKEUP_API == STD_ON)
void Icu_EnableWakeup(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLEWAKEUP, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLEWAKEUP, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    Icu_ChannelWakeupEnabled[Channel] = TRUE;
}
#else
void Icu_EnableWakeup(Icu_ChannelType Channel)
{
    (void)Channel;
}
#endif

#if (ICU_CHECK_WAKEUP_API == STD_ON)
Std_ReturnType Icu_CheckWakeup(uint32 WakeupSource)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_CHECKWAKEUP, ICU_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    /* Check if the wakeup source matches any ICU channel */
    for (uint8 i = 0U; i < Icu_ConfigPtr->NumChannels; i++) {
        Icu_ChannelType channel = Icu_ConfigPtr->Channels[i].ChannelId;
        if (Icu_ChannelWakeupEnabled[channel]) {
            uint32 flag = Icu_GetChannelFlag(channel);
            if (flag != 0U) {
                return E_OK;
            }
        }
    }
    
    return E_NOT_OK;
}
#else
Std_ReturnType Icu_CheckWakeup(uint32 WakeupSource)
{
    (void)WakeupSource;
    return E_NOT_OK;
}
#endif

void Icu_SetActivationCondition(Icu_ChannelType Channel, Icu_ActivationType Activation)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_SETACTIVATIONCONDITION, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_SETACTIVATIONCONDITION, ICU_E_PARAM_CHANNEL);
        return;
    }
    if ((Activation != ICU_FALLING_EDGE) && 
        (Activation != ICU_RISING_EDGE) && 
        (Activation != ICU_BOTH_EDGES)) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_SETACTIVATIONCONDITION, ICU_E_PARAM_ACTIVATION);
        return;
    }
    #endif
    
    uint32 emiosBase = Icu_GetEmiosBaseAddr(Channel);
    uint8 chNum = Icu_GetEmiosChannelNum(Channel);
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    uint32 cReg = REG_READ32(chAddr + EMIOS_C_C);
    
    /* Update edge selection */
    switch (Activation) {
        case ICU_RISING_EDGE:
            cReg |= EMIOS_C_EDPOL;
            cReg &= ~EMIOS_C_EDSEL;
            break;
        case ICU_FALLING_EDGE:
            cReg &= ~EMIOS_C_EDPOL;
            cReg &= ~EMIOS_C_EDSEL;
            break;
        case ICU_BOTH_EDGES:
            cReg |= EMIOS_C_EDPOL;
            cReg |= EMIOS_C_EDSEL;
            break;
        default:
            break;
    }
    
    REG_WRITE32(chAddr + EMIOS_C_C, cReg);
    Icu_ChannelActivation[Channel] = Activation;
}

void Icu_DisableNotification(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLENOTIFICATION, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLENOTIFICATION, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    Icu_ChannelNotificationEnabled[Channel] = FALSE;
    Icu_DisableChannelInterrupt(Channel);
}

void Icu_EnableNotification(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLENOTIFICATION, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLENOTIFICATION, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    Icu_ChannelNotificationEnabled[Channel] = TRUE;
    Icu_EnableChannelInterrupt(Channel);
}

Icu_InputStateType Icu_GetInputState(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETINPUTSTATE, ICU_E_UNINIT);
        return ICU_IDLE;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETINPUTSTATE, ICU_E_PARAM_CHANNEL);
        return ICU_IDLE;
    }
    #endif
    
    uint32 emiosBase = Icu_GetEmiosBaseAddr(Channel);
    uint8 chNum = Icu_GetEmiosChannelNum(Channel);
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    uint32 sReg = REG_READ32(chAddr + EMIOS_C_S);
    
    if ((sReg & EMIOS_S_FLAG) != 0U) {
        Icu_ChannelInputState[Channel] = ICU_ACTIVE;
        Icu_ClearChannelFlag(Channel);
    } else {
        Icu_ChannelInputState[Channel] = ICU_IDLE;
    }
    
    return Icu_ChannelInputState[Channel];
}

#if (ICU_TIMESTAMP_API == STD_ON)
void Icu_StartTimestamp(Icu_ChannelType Channel, uint32* BufferPtr, uint16 BufferSize, uint16 NotifyInterval)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_PARAM_CHANNEL);
        return;
    }
    if (BufferPtr == NULL_PTR) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_PARAM_POINTER);
        return;
    }
    if (BufferSize == 0U) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_PARAM_BUFFER_SIZE);
        return;
    }
    if (Icu_ChannelRunning[Channel]) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTTIMESTAMP, ICU_E_BUSY);
        return;
    }
    #endif
    
    /* Configure for timestamp mode */
    Icu_ConfigureEmiosMode(Channel, EMIOS_MODE_SAIC);
    
    Icu_TimestampBuffer[Channel] = BufferPtr;
    Icu_TimestampBufferSize[Channel] = BufferSize;
    Icu_TimestampIndex[Channel] = 0U;
    Icu_TimestampNotifyInterval[Channel] = NotifyInterval;
    Icu_TimestampCaptureCount[Channel] = 0U;
    Icu_ChannelRunning[Channel] = TRUE;
    
    /* Clear buffer */
    for (uint16 i = 0U; i < BufferSize; i++) {
        BufferPtr[i] = 0U;
    }
    
    /* Enable interrupt if notification is enabled */
    if (Icu_ChannelNotificationEnabled[Channel]) {
        Icu_EnableChannelInterrupt(Channel);
    }
}

void Icu_StopTimestamp(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STOPTIMESTAMP, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STOPTIMESTAMP, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    Icu_DisableChannelInterrupt(Channel);
    Icu_ChannelRunning[Channel] = FALSE;
    Icu_TimestampBuffer[Channel] = NULL_PTR;
}

Icu_IndexType Icu_GetTimestampIndex(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETTIMESTAMPINDEX, ICU_E_UNINIT);
        return 0U;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETTIMESTAMPINDEX, ICU_E_PARAM_CHANNEL);
        return 0U;
    }
    if (Icu_ChannelRunning[Channel] == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETTIMESTAMPINDEX, ICU_E_STAMP_NOT_RUNNING);
        return 0U;
    }
    #endif
    
    return Icu_TimestampIndex[Channel];
}
#endif

#if (ICU_EDGE_COUNT_API == STD_ON)
void Icu_ResetEdgeCount(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_RESETEDGECOUNT, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_RESETEDGECOUNT, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    uint32 emiosBase = Icu_GetEmiosBaseAddr(Channel);
    uint8 chNum = Icu_GetEmiosChannelNum(Channel);
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    
    /* Reset counter register */
    REG_WRITE32(chAddr + EMIOS_C_CNT, 0U);
    Icu_EdgeCount[Channel] = 0U;
}

void Icu_EnableEdgeCount(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLEEDGECOUNT, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_ENABLEEDGECOUNT, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    /* Configure for edge counting mode */
    Icu_ConfigureEmiosMode(Channel, EMIOS_MODE_EMCB);
    Icu_EdgeCountEnabled[Channel] = TRUE;
    
    /* Enable interrupt if notification is enabled */
    if (Icu_ChannelNotificationEnabled[Channel]) {
        Icu_EnableChannelInterrupt(Channel);
    }
}

void Icu_DisableEdgeCount(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLEEDGECOUNT, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_DISABLEEDGECOUNT, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    Icu_DisableChannelInterrupt(Channel);
    Icu_ConfigureEmiosMode(Channel, EMIOS_MODE_GPIO_INPUT);
    Icu_EdgeCountEnabled[Channel] = FALSE;
}

uint16 Icu_GetEdgeNumbers(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETEDGENUMBERS, ICU_E_UNINIT);
        return 0U;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETEDGENUMBERS, ICU_E_PARAM_CHANNEL);
        return 0U;
    }
    if (Icu_EdgeCountEnabled[Channel] == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETEDGENUMBERS, ICU_E_EDGE_COUNTING_NOT_RUNNING);
        return 0U;
    }
    #endif
    
    uint32 emiosBase = Icu_GetEmiosBaseAddr(Channel);
    uint8 chNum = Icu_GetEmiosChannelNum(Channel);
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    
    /* Read counter register */
    uint32 count = REG_READ32(chAddr + EMIOS_C_CNT);
    Icu_EdgeCount[Channel] = (uint16)count;
    
    return Icu_EdgeCount[Channel];
}
#endif

#if (ICU_SIGNAL_MEASUREMENT_API == STD_ON)
void Icu_StartSignalMeasurement(Icu_ChannelType Channel, Icu_SignalMeasurementPropertyType MeasureKind)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTSIGNALMEASUREMENT, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTSIGNALMEASUREMENT, ICU_E_PARAM_CHANNEL);
        return;
    }
    if (Icu_SignalMeasurementRunning[Channel]) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STARTSIGNALMEASUREMENT, ICU_E_MEASUREMENT_RUNNING);
        return;
    }
    #endif
    
    /* Reset measurement data */
    Icu_SignalPeriodTime[Channel] = 0U;
    Icu_SignalActiveTime[Channel] = 0U;
    Icu_SignalLastCapture[Channel] = 0U;
    
    /* Configure measurement mode */
    if (MeasureKind == ICU_DUTY_CYCLE) {
        Icu_ConfigureEmiosMode(Channel, EMIOS_MODE_IPWM);
    } else {
        Icu_ConfigureEmiosMode(Channel, EMIOS_MODE_IPM);
    }
    
    Icu_SignalMeasurementRunning[Channel] = TRUE;
    
    /* Enable interrupt if notification is enabled */
    if (Icu_ChannelNotificationEnabled[Channel]) {
        Icu_EnableChannelInterrupt(Channel);
    }
}

void Icu_StopSignalMeasurement(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STOPSIGNALMEASUREMENT, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_STOPSIGNALMEASUREMENT, ICU_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    Icu_DisableChannelInterrupt(Channel);
    Icu_ConfigureEmiosMode(Channel, EMIOS_MODE_GPIO_INPUT);
    Icu_SignalMeasurementRunning[Channel] = FALSE;
}

uint16 Icu_GetTimeElapsed(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETTIMEELAPSED, ICU_E_UNINIT);
        return 0U;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETTIMEELAPSED, ICU_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif
    
    return Icu_SignalPeriodTime[Channel];
}

void Icu_GetDutyCycleValues(Icu_ChannelType Channel, Icu_DutyCycleType* DutyCycleValues)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETDUTYCYCLEVALUES, ICU_E_UNINIT);
        return;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETDUTYCYCLEVALUES, ICU_E_PARAM_CHANNEL);
        return;
    }
    if (DutyCycleValues == NULL_PTR) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETDUTYCYCLEVALUES, ICU_E_PARAM_POINTER);
        return;
    }
    #endif
    
    DutyCycleValues->ActiveTime = Icu_SignalActiveTime[Channel];
    DutyCycleValues->PeriodTime = Icu_SignalPeriodTime[Channel];
}
#endif

void Icu_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETVERSIONINFO, ICU_E_PARAM_POINTER);
        return;
    }
    #endif
    
    versioninfo->vendorID = ICU_VENDOR_ID;
    versioninfo->moduleID = ICU_MODULE_ID;
    versioninfo->sw_major_version = ICU_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = ICU_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = ICU_SW_PATCH_VERSION;
}

uint8 Icu_GetInputLevel(Icu_ChannelType Channel)
{
    #if (ICU_DEV_ERROR_DETECT == STD_ON)
    if (Icu_DriverInitialized == FALSE) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETINPUTLEVEL, ICU_E_UNINIT);
        return 0U;
    }
    if (Channel >= ICU_NUM_CHANNELS) {
        Det_ReportError(ICU_MODULE_ID, 0U, ICU_SID_GETINPUTLEVEL, ICU_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif
    
    uint32 emiosBase = Icu_GetEmiosBaseAddr(Channel);
    uint8 chNum = Icu_GetEmiosChannelNum(Channel);
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    uint32 sReg = REG_READ32(chAddr + EMIOS_C_S);
    
    return ((sReg & EMIOS_S_UCIN) != 0U) ? 1U : 0U;
}

uint32 Icu_GetSysTimestamp(void)
{
    /* Use EMIOS_0 channel 23 (last channel) as free-running counter for system timestamp */
    uint32 emiosBase = ICU_EMIOS_0_BASE_ADDR;
    uint8 chNum = 23U;  /* Use last channel as timebase */
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    
    return REG_READ32(chAddr + EMIOS_C_CNT);
}

/*==================================================================================================
*                                    INTERRUPT HANDLER
==================================================================================================*/
/**
 * @brief ICU Interrupt Handler
 * @param Channel Channel that triggered the interrupt
 */
void Icu_ProcessInterrupt(Icu_ChannelType Channel)
{
    if (Channel >= ICU_NUM_CHANNELS) {
        return;
    }
    
    uint32 emiosBase = Icu_GetEmiosBaseAddr(Channel);
    uint8 chNum = Icu_GetEmiosChannelNum(Channel);
    uint32 chAddr = Icu_GetChannelRegAddr(emiosBase, chNum);
    
    /* Read capture value */
    uint16 captureValue = (uint16)REG_READ32(chAddr + EMIOS_C_A);
    
    /* Clear flag */
    Icu_ClearChannelFlag(Channel);
    
    /* Process based on measurement mode */
    if (Icu_ConfigPtr->Channels[Channel].MeasurementMode == ICU_MODE_TIMESTAMP &&
        Icu_ChannelRunning[Channel]) {
        /* Store timestamp */
        if (Icu_TimestampBuffer[Channel] != NULL_PTR) {
            Icu_TimestampBuffer[Channel][Icu_TimestampIndex[Channel]] = captureValue;
            Icu_TimestampIndex[Channel]++;
            
            /* Wrap around for circular buffer */
            if (Icu_TimestampIndex[Channel] >= Icu_TimestampBufferSize[Channel]) {
                if (Icu_ConfigPtr->Channels[Channel].TimestampBufferType == ICU_CIRCULAR_BUFFER) {
                    Icu_TimestampIndex[Channel] = 0U;
                } else {
                    /* Linear buffer - stop capturing */
                    Icu_StopTimestamp(Channel);
                }
            }
            
            /* Check for notification */
            if (Icu_TimestampNotifyInterval[Channel] > 0U) {
                Icu_TimestampCaptureCount[Channel]++;
                if (Icu_TimestampCaptureCount[Channel] >= Icu_TimestampNotifyInterval[Channel]) {
                    Icu_TimestampCaptureCount[Channel] = 0U;
                    if (Icu_ConfigPtr->Channels[Channel].NotificationFn != NULL_PTR) {
                        Icu_ConfigPtr->Channels[Channel].NotificationFn();
                    }
                }
            }
        }
    }
    else if (Icu_ConfigPtr->Channels[Channel].MeasurementMode == ICU_MODE_SIGNAL_MEASUREMENT &&
             Icu_SignalMeasurementRunning[Channel]) {
        /* Process signal measurement */
        Icu_ProcessSignalMeasurement(Channel, captureValue);
        
        /* Call notification if enabled */
        if (Icu_ChannelNotificationEnabled[Channel] &&
            Icu_ConfigPtr->Channels[Channel].NotificationFn != NULL_PTR) {
            Icu_ConfigPtr->Channels[Channel].NotificationFn();
        }
    }
    else if (Icu_ConfigPtr->Channels[Channel].MeasurementMode == ICU_MODE_SIGNAL_EDGE_DETECT) {
        /* Update input state */
        Icu_ChannelInputState[Channel] = ICU_ACTIVE;
        
        /* Call notification if enabled */
        if (Icu_ChannelNotificationEnabled[Channel] &&
            Icu_ConfigPtr->Channels[Channel].NotificationFn != NULL_PTR) {
            Icu_ConfigPtr->Channels[Channel].NotificationFn();
        }
    }
    else if (Icu_EdgeCountEnabled[Channel]) {
        /* Edge counting - increment handled by hardware */
        /* Call notification if enabled */
        if (Icu_ChannelNotificationEnabled[Channel] &&
            Icu_ConfigPtr->Channels[Channel].NotificationFn != NULL_PTR) {
            Icu_ConfigPtr->Channels[Channel].NotificationFn();
        }
    }
    
    /* Handle wakeup */
    if (Icu_ChannelWakeupEnabled[Channel] && (Icu_DriverMode == ICU_MODE_SLEEP)) {
        /* Report wakeup to EcuM */
        #if (ICU_REPORT_WAKEUP_SOURCE == STD_ON)
        /* EcuM_SetWakeupEvent(ICU_WAKEUP_SOURCE); */
        #endif
    }
}

#define ICU_STOP_SEC_CODE
#include "MemMap.h"
