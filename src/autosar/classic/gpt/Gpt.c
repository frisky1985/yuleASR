/******************************************************************************
 * @file    Gpt.c
 * @brief   GPT (General Purpose Timer) Driver Implementation
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024-2026
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "autosar/classic/gpt/gpt.h"
#include "autosar/classic/gpt/Gpt_Cfg.h"

#if (GPT_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

#if (GPT_REPORT_WAKEUP_SOURCE == STD_ON)
#include "Mcu.h"
#endif

/******************************************************************************
 * Hardware Register Definitions (i.MX8M Mini GPT)
 ******************************************************************************/
#define GPT_CR                                  (0x00U)   /* Control Register */
#define GPT_PR                                  (0x04U)   /* Prescaler Register */
#define GPT_SR                                  (0x08U)   /* Status Register */
#define GPT_IR                                  (0x0CU)   /* Interrupt Register */
#define GPT_OCR1                                (0x10U)   /* Output Compare 1 */
#define GPT_OCR2                                (0x14U)   /* Output Compare 2 */
#define GPT_OCR3                                (0x18U)   /* Output Compare 3 */
#define GPT_ICR1                                (0x1CU)   /* Input Capture 1 */
#define GPT_ICR2                                (0x20U)   /* Input Capture 2 */
#define GPT_CNT                                 (0x24U)   /* Counter Register */

/* Control Register Bits */
#define GPT_CR_EN                               (0x00000001U)  /* Enable */
#define GPT_CR_ENMOD                            (0x00000002U)  /* Enable Mode */
#define GPT_CR_DBGEN                            (0x00000004U)  /* Debug Enable */
#define GPT_CR_WAITEN                           (0x00000008U)  /* Wait Enable */
#define GPT_CR_DOZEEN                           (0x00000010U)  /* Doze Enable */
#define GPT_CR_STOPEN                           (0x00000020U)  /* Stop Enable */
#define GPT_CR_CLKSRC_SHIFT                     (6U)
#define GPT_CR_CLKSRC_MASK                      (0x000001C0U)
#define GPT_CR_FRR                              (0x00000200U)  /* Free-Run/Restart */
#define GPT_CR_SWR                              (0x00010000U)  /* Software Reset */
#define GPT_CR_IM1_SHIFT                        (18U)
#define GPT_CR_IM1_MASK                         (0x000C0000U)
#define GPT_CR_IM2_SHIFT                        (20U)
#define GPT_CR_IM2_MASK                         (0x00300000U)
#define GPT_CR_OM1_SHIFT                        (22U)
#define GPT_CR_OM1_MASK                         (0x00C00000U)
#define GPT_CR_OM2_SHIFT                        (24U)
#define GPT_CR_OM2_MASK                         (0x03000000U)
#define GPT_CR_OM3_SHIFT                        (26U)
#define GPT_CR_OM3_MASK                         (0x0C000000U)
#define GPT_CR_FO1                              (0x10000000U)  /* Force Output 1 */
#define GPT_CR_FO2                              (0x20000000U)  /* Force Output 2 */
#define GPT_CR_FO3                              (0x40000000U)  /* Force Output 3 */

/* Status Register Bits */
#define GPT_SR_OF1                              (0x00000001U)  /* Output Compare 1 Flag */
#define GPT_SR_OF2                              (0x00000002U)  /* Output Compare 2 Flag */
#define GPT_SR_OF3                              (0x00000004U)  /* Output Compare 3 Flag */
#define GPT_SR_IF1                              (0x00000008U)  /* Input Capture 1 Flag */
#define GPT_SR_IF2                              (0x00000010U)  /* Input Capture 2 Flag */
#define GPT_SR_ROV                              (0x00000020U)  /* Rollover Flag */

/* Interrupt Register Bits */
#define GPT_IR_OF1IE                            (0x00000001U)  /* Output Compare 1 Int Enable */
#define GPT_IR_OF2IE                            (0x00000002U)  /* Output Compare 2 Int Enable */
#define GPT_IR_OF3IE                            (0x00000004U)  /* Output Compare 3 Int Enable */
#define GPT_IR_IF1IE                            (0x00000008U)  /* Input Capture 1 Int Enable */
#define GPT_IR_IF2IE                            (0x00000010U)  /* Input Capture 2 Int Enable */
#define GPT_IR_ROVIE                            (0x00000020U)  /* Rollover Int Enable */

/* Clock Sources */
#define GPT_CLKSRC_OFF                          (0U)
#define GPT_CLKSRC_PERIPHERAL                   (1U)
#define GPT_CLKSRC_HIGH_FREQ                    (2U)
#define GPT_CLKSRC_EXT                          (3U)
#define GPT_CLKSRC_LOW_FREQ                     (4U)

/******************************************************************************
 * Register Access Macros
 ******************************************************************************/
#define REG_READ32(addr)                        (*(volatile uint32*)(addr))
#define REG_WRITE32(addr, val)                  (*(volatile uint32*)(addr) = (val))
#define REG_RMW32(addr, mask, val)              (REG_WRITE32((addr), ((REG_READ32(addr) & ~(mask)) | (val))))

/******************************************************************************
 * Module Variables
 ******************************************************************************/
#define GPT_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Driver initialization state */
static boolean Gpt_DriverInitialized = FALSE;

/* Driver operation mode */
static Gpt_ModeType Gpt_DriverMode = GPT_MODE_NORMAL;

/* Pointer to configuration */
static const Gpt_ConfigType* Gpt_ConfigPtr = NULL_PTR;

/* Channel runtime states */
static Gpt_ChannelStatusType Gpt_ChannelStatus[GPT_MAX_CHANNELS];
static Gpt_ValueType Gpt_ChannelTargetValue[GPT_MAX_CHANNELS];
static Gpt_ValueType Gpt_ChannelStartValue[GPT_MAX_CHANNELS];
static boolean Gpt_ChannelRunning[GPT_MAX_CHANNELS];
static boolean Gpt_ChannelNotificationEnabled[GPT_MAX_CHANNELS];
static Gpt_ControlType Gpt_ChannelControlMode[GPT_MAX_CHANNELS];

/* Capture and PWM states */
static Gpt_CaptureModeType Gpt_ChannelCaptureMode[GPT_MAX_CHANNELS];
static Gpt_ValueType Gpt_ChannelCaptureValue[GPT_MAX_CHANNELS];
static boolean Gpt_ChannelPwmEnabled[GPT_MAX_CHANNELS];
static uint16 Gpt_ChannelPwmDutyCycle[GPT_MAX_CHANNELS];

/* Predefined timer values */
static uint32 Gpt_PredefTimer1usValue;
static uint32 Gpt_PredefTimer100usValue;

#define GPT_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/******************************************************************************
 * Internal Functions
 ******************************************************************************/
#define GPT_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Gets base address for a channel
 */
static uint32 Gpt_GetBaseAddr(Gpt_ChannelType channel)
{
    uint32 baseAddr;
    
    if (channel < 4U) {
        baseAddr = GPT1_BASE_ADDR;
    } else if (channel < 8U) {
        baseAddr = GPT2_BASE_ADDR;
    } else {
        baseAddr = 0U;
    }
    
    return baseAddr;
}

/**
 * @brief Gets channel offset within a module
 */
static uint8 Gpt_GetChannelOffset(Gpt_ChannelType channel)
{
    return (uint8)(channel % GPT_CHANNELS_PER_MODULE);
}

/**
 * @brief Gets clock source value
 */
static uint32 Gpt_GetClockSourceValue(Gpt_ClockSourceType source)
{
    uint32 clkSrc;
    
    switch (source) {
        case GPT_CLOCK_SOURCE_SYSTEM:
            clkSrc = GPT_CLKSRC_HIGH_FREQ;
            break;
        case GPT_CLOCK_SOURCE_PERIPHERAL:
            clkSrc = GPT_CLKSRC_PERIPHERAL;
            break;
        case GPT_CLOCK_SOURCE_EXTERNAL:
            clkSrc = GPT_CLKSRC_EXT;
            break;
        case GPT_CLOCK_SOURCE_LOW_FREQ:
            clkSrc = GPT_CLKSRC_LOW_FREQ;
            break;
        default:
            clkSrc = GPT_CLKSRC_PERIPHERAL;
            break;
    }
    
    return clkSrc;
}

/**
 * @brief Checks if channel is valid
 */
static boolean Gpt_IsChannelValid(Gpt_ChannelType channel)
{
    boolean valid = FALSE;
    
    if ((channel < GPT_MAX_CHANNELS) && 
        (Gpt_ConfigPtr != NULL_PTR) &&
        (channel < Gpt_ConfigPtr->NumChannels)) {
        valid = TRUE;
    }
    
    return valid;
}

/**
 * @brief Calculates prescaler value
 */
static uint32 Gpt_CalculatePrescaler(Gpt_ClockPrescalerType prescaler)
{
    return ((uint32)1U << (uint32)prescaler) - 1U;
}

/**
 * @brief Gets clock frequency for a channel
 */
static uint32 Gpt_GetChannelClockFreq(Gpt_ChannelType channel)
{
    uint32 freq = 0U;
    
    if (Gpt_IsChannelValid(channel)) {
        const Gpt_ChannelConfigType* chCfg = &Gpt_ConfigPtr->Channels[channel];
        
        switch (chCfg->ClockSource) {
            case GPT_CLOCK_SOURCE_SYSTEM:
                freq = GPT_SYSTEM_CLOCK_FREQUENCY_HZ;
                break;
            case GPT_CLOCK_SOURCE_PERIPHERAL:
                freq = GPT_PERIPHERAL_CLOCK_FREQUENCY_HZ;
                break;
            case GPT_CLOCK_SOURCE_EXTERNAL:
                freq = GPT_EXTERNAL_CLOCK_FREQUENCY_HZ;
                break;
            case GPT_CLOCK_SOURCE_LOW_FREQ:
                freq = GPT_LOW_FREQ_CLOCK_HZ;
                break;
            default:
                freq = GPT_PERIPHERAL_CLOCK_FREQUENCY_HZ;
                break;
        }
    }
    
    return freq;
}

/******************************************************************************
 * API Implementation
 ******************************************************************************/

/**
 * @brief Initializes the GPT driver
 */
void Gpt_Init(const Gpt_ConfigType* ConfigPtr)
{
    uint8 i;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_INIT, GPT_E_PARAM_POINTER);
        return;
    }
    
    if (Gpt_DriverInitialized == TRUE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_INIT, GPT_E_ALREADY_INITIALIZED);
        return;
    }
    #endif
    
    Gpt_ConfigPtr = ConfigPtr;
    
    /* Initialize all channels */
    for (i = 0U; i < ConfigPtr->NumChannels; i++) {
        const Gpt_ChannelConfigType* chConfig = &ConfigPtr->Channels[i];
        uint32 baseAddr = Gpt_GetBaseAddr(chConfig->ChannelId);
        uint32 crValue;
        uint32 prValue;
        
        if (baseAddr == 0U) {
            continue;
        }
        
        /* Software reset */
        REG_WRITE32(baseAddr + GPT_CR, GPT_CR_SWR);
        while ((REG_READ32(baseAddr + GPT_CR) & GPT_CR_SWR) != 0U) {
            /* Wait for reset to complete */
        }
        
        /* Calculate and set prescaler */
        prValue = Gpt_CalculatePrescaler(chConfig->ClockPrescaler);
        REG_WRITE32(baseAddr + GPT_PR, prValue);
        
        /* Configure control register */
        crValue = 0U;
        
        /* Set clock source */
        crValue |= (Gpt_GetClockSourceValue(chConfig->ClockSource) << GPT_CR_CLKSRC_SHIFT);
        
        /* Set free-run or restart mode */
        if (chConfig->ChannelMode == GPT_CH_MODE_CONTINUOUS) {
            crValue |= GPT_CR_FRR;  /* Free-run mode for continuous */
        }
        
        /* Enable in doze and wait modes */
        crValue |= GPT_CR_DOZEEN | GPT_CR_WAITEN;
        
        REG_WRITE32(baseAddr + GPT_CR, crValue);
        
        /* Clear status flags */
        REG_WRITE32(baseAddr + GPT_SR, 0x3FU);
        
        /* Disable all interrupts */
        REG_WRITE32(baseAddr + GPT_IR, 0U);
        
        /* Initialize channel state */
        Gpt_ChannelStatus[i] = GPT_CH_STATUS_READY;
        Gpt_ChannelRunning[i] = FALSE;
        Gpt_ChannelTargetValue[i] = 0U;
        Gpt_ChannelStartValue[i] = 0U;
        Gpt_ChannelNotificationEnabled[i] = chConfig->NotificationEnabled;
        Gpt_ChannelControlMode[i] = GPT_CONTROL_SYNC;
        Gpt_ChannelCaptureMode[i] = GPT_CAPTURE_DISABLE;
        Gpt_ChannelCaptureValue[i] = 0U;
        Gpt_ChannelPwmEnabled[i] = FALSE;
        Gpt_ChannelPwmDutyCycle[i] = 0U;
    }
    
    Gpt_DriverMode = ConfigPtr->DefaultMode;
    Gpt_DriverInitialized = TRUE;
}

#if (GPT_DEINIT_API == STD_ON)
/**
 * @brief Deinitializes the GPT driver
 */
void Gpt_DeInit(void)
{
    uint8 i;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DEINIT, GPT_E_UNINIT);
        return;
    }
    #endif
    
    /* Check if any channel is running */
    for (i = 0U; i < Gpt_ConfigPtr->NumChannels; i++) {
        if (Gpt_ChannelRunning[i]) {
            return; /* Cannot deinitialize if channels are running */
        }
    }
    
    /* Deinitialize all channels */
    for (i = 0U; i < Gpt_ConfigPtr->NumChannels; i++) {
        uint32 baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[i].ChannelId);
        
        if (baseAddr == 0U) {
            continue;
        }
        
        /* Disable timer */
        REG_RMW32(baseAddr + GPT_CR, GPT_CR_EN, 0U);
        
        /* Disable interrupts */
        REG_WRITE32(baseAddr + GPT_IR, 0U);
        
        Gpt_ChannelStatus[i] = GPT_CH_STATUS_UNINIT;
    }
    
    Gpt_DriverInitialized = FALSE;
    Gpt_ConfigPtr = NULL_PTR;
}
#endif

#if (GPT_TIME_ELAPSED_API == STD_ON)
/**
 * @brief Gets time elapsed for a channel
 */
Gpt_ValueType Gpt_GetTimeElapsed(Gpt_ChannelType Channel)
{
    Gpt_ValueType elapsed = 0U;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEELAPSED, GPT_E_UNINIT);
        return 0U;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEELAPSED, GPT_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif
    
    if (Gpt_ChannelRunning[Channel]) {
        uint32 baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
        Gpt_ValueType current = REG_READ32(baseAddr + GPT_CNT);
        
        if (current >= Gpt_ChannelStartValue[Channel]) {
            elapsed = current - Gpt_ChannelStartValue[Channel];
        } else {
            /* Counter wrapped around */
            elapsed = (GPT_MAX_TICK_VALUE - Gpt_ChannelStartValue[Channel]) + current + 1U;
        }
    } else {
        elapsed = Gpt_ChannelElapsedValue[Channel];
    }
    
    return elapsed;
}
#endif

#if (GPT_TIME_REMAINING_API == STD_ON)
/**
 * @brief Gets time remaining for a channel
 */
Gpt_ValueType Gpt_GetTimeRemaining(Gpt_ChannelType Channel)
{
    Gpt_ValueType remaining = 0U;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEREMAINING, GPT_E_UNINIT);
        return 0U;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEREMAINING, GPT_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif
    
    if (Gpt_ChannelRunning[Channel]) {
        Gpt_ValueType elapsed = Gpt_GetTimeElapsed(Channel);
        
        if (Gpt_ChannelTargetValue[Channel] > elapsed) {
            remaining = Gpt_ChannelTargetValue[Channel] - elapsed;
        }
    }
    
    return remaining;
}
#endif

/**
 * @brief Starts a timer channel
 */
void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value)
{
    uint32 baseAddr;
    uint8 chOffset;
    uint32 crValue;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_UNINIT);
        return;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_PARAM_CHANNEL);
        return;
    }
    
    if ((Value == 0U) || (Value > Gpt_ConfigPtr->Channels[Channel].MaxTickValue)) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_PARAM_VALUE);
        return;
    }
    
    if (Gpt_ChannelRunning[Channel]) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_CHANNEL_BUSY);
        return;
    }
    #endif
    
    baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    chOffset = Gpt_GetChannelOffset(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (baseAddr == 0U) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    /* Reset counter */
    REG_WRITE32(baseAddr + GPT_CNT, 0U);
    
    /* Set output compare value */
    REG_WRITE32(baseAddr + GPT_OCR1 + (chOffset * 4U), Value);
    
    /* Store target value */
    Gpt_ChannelTargetValue[Channel] = Value;
    Gpt_ChannelStartValue[Channel] = 0U;
    Gpt_ChannelRunning[Channel] = TRUE;
    Gpt_ChannelStatus[Channel] = GPT_CH_STATUS_RUNNING;
    
    /* Enable interrupt if notification is enabled */
    if (Gpt_ChannelNotificationEnabled[Channel]) {
        uint32 irValue = REG_READ32(baseAddr + GPT_IR);
        irValue |= (GPT_IR_OF1IE << chOffset);
        REG_WRITE32(baseAddr + GPT_IR, irValue);
    }
    
    /* Enable timer */
    crValue = REG_READ32(baseAddr + GPT_CR);
    crValue |= GPT_CR_EN;
    REG_WRITE32(baseAddr + GPT_CR, crValue);
}

/**
 * @brief Stops a timer channel
 */
void Gpt_StopTimer(Gpt_ChannelType Channel)
{
    uint32 baseAddr;
    uint8 chOffset;
    Gpt_ValueType elapsed;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STOPTIMER, GPT_E_UNINIT);
        return;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STOPTIMER, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    chOffset = Gpt_GetChannelOffset(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    
    /* Disable interrupt */
    REG_RMW32(baseAddr + GPT_IR, (GPT_IR_OF1IE << chOffset), 0U);
    
    /* Disable timer */
    REG_RMW32(baseAddr + GPT_CR, GPT_CR_EN, 0U);
    
    /* Save elapsed time */
    if (Gpt_ChannelRunning[Channel]) {
        Gpt_ValueType current = REG_READ32(baseAddr + GPT_CNT);
        if (current >= Gpt_ChannelStartValue[Channel]) {
            elapsed = current - Gpt_ChannelStartValue[Channel];
        } else {
            elapsed = (GPT_MAX_TICK_VALUE - Gpt_ChannelStartValue[Channel]) + current + 1U;
        }
        Gpt_ChannelElapsedValue[Channel] = elapsed;
    }
    
    Gpt_ChannelRunning[Channel] = FALSE;
    Gpt_ChannelStatus[Channel] = GPT_CH_STATUS_STOPPED;
}

#if (GPT_ENABLE_DISABLE_NOTIFICATION_API == STD_ON)
/**
 * @brief Enables notification for a channel
 */
void Gpt_EnableNotification(Gpt_ChannelType Channel)
{
    uint32 baseAddr;
    uint8 chOffset;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEINTERRUPT, GPT_E_UNINIT);
        return;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEINTERRUPT, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    Gpt_ChannelNotificationEnabled[Channel] = TRUE;
    
    /* Enable interrupt if channel is running */
    if (Gpt_ChannelRunning[Channel]) {
        baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
        chOffset = Gpt_GetChannelOffset(Gpt_ConfigPtr->Channels[Channel].ChannelId);
        
        REG_RMW32(baseAddr + GPT_IR, 0U, (GPT_IR_OF1IE << chOffset));
    }
}

/**
 * @brief Disables notification for a channel
 */
void Gpt_DisableNotification(Gpt_ChannelType Channel)
{
    uint32 baseAddr;
    uint8 chOffset;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEINTERRUPT, GPT_E_UNINIT);
        return;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEINTERRUPT, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    Gpt_ChannelNotificationEnabled[Channel] = FALSE;
    
    /* Disable interrupt */
    baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    chOffset = Gpt_GetChannelOffset(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    
    REG_RMW32(baseAddr + GPT_IR, (GPT_IR_OF1IE << chOffset), 0U);
}
#endif

/**
 * @brief Gets version information
 */
void Gpt_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETVERSIONINFO, GPT_E_PARAM_POINTER);
        return;
    }
    #endif
    
    versioninfo->vendorID = GPT_VENDOR_ID;
    versioninfo->moduleID = GPT_MODULE_ID;
    versioninfo->sw_major_version = GPT_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = GPT_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = GPT_SW_PATCH_VERSION;
}

/**
 * @brief Sets the operation mode
 */
void Gpt_SetMode(Gpt_ModeType Mode)
{
    uint8 i;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_SETMODE, GPT_E_UNINIT);
        return;
    }
    
    if ((Mode != GPT_MODE_NORMAL) && (Mode != GPT_MODE_SLEEP)) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_SETMODE, GPT_E_PARAM_MODE);
        return;
    }
    #endif
    
    if (Mode == GPT_MODE_SLEEP) {
        /* Stop all non-wakeup channels */
        for (i = 0U; i < Gpt_ConfigPtr->NumChannels; i++) {
            if (Gpt_ChannelRunning[i] && !Gpt_ConfigPtr->Channels[i].WakeupSupport) {
                Gpt_StopTimer(i);
            }
        }
    }
    
    Gpt_DriverMode = Mode;
}

#if (GPT_WAKEUP_FUNCTIONALITY_API == STD_ON)
/**
 * @brief Disables wakeup for a channel
 */
void Gpt_DisableWakeup(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEWAKEUP, GPT_E_UNINIT);
        return;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEWAKEUP, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    (void)Channel;
}

/**
 * @brief Enables wakeup for a channel
 */
void Gpt_EnableWakeup(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEWAKEUP, GPT_E_UNINIT);
        return;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEWAKEUP, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    (void)Channel;
}

/**
 * @brief Checks for wakeup events
 */
Std_ReturnType Gpt_CheckWakeup(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_CHECKWAKEUP, GPT_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_CHECKWAKEUP, GPT_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    (void)Channel;
    return E_NOT_OK;
}
#endif

/**
 * @brief Gets predefined timer value
 */
Std_ReturnType Gpt_GetPredefTimerValue(Gpt_PredefTimerType PredefTimer, uint32* TimeValuePtr)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETPREDEFTIMERVALUE, GPT_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (TimeValuePtr == NULL_PTR) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETPREDEFTIMERVALUE, GPT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    switch (PredefTimer) {
        case GPT_PREDEF_TIMER_1US_16BIT:
        case GPT_PREDEF_TIMER_1US_24BIT:
        case GPT_PREDEF_TIMER_1US_32BIT:
            *TimeValuePtr = Gpt_PredefTimer1usValue;
            break;
            
        case GPT_PREDEF_TIMER_100US_32BIT:
            *TimeValuePtr = Gpt_PredefTimer100usValue;
            break;
            
        default:
            #if (GPT_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETPREDEFTIMERVALUE, GPT_E_PARAM_PREDEF_TIMER);
            #endif
            return E_NOT_OK;
    }
    
    return E_OK;
}

/******************************************************************************
 * Extended API Implementation
 ******************************************************************************/

/**
 * @brief Gets channel status
 */
Gpt_ChannelStatusType Gpt_GetChannelStatus(Gpt_ChannelType Channel)
{
    Gpt_ChannelStatusType status = GPT_CH_STATUS_UNINIT;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x10U, GPT_E_UNINIT);
        return GPT_CH_STATUS_UNINIT;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x10U, GPT_E_PARAM_CHANNEL);
        return GPT_CH_STATUS_UNINIT;
    }
    #endif
    
    status = Gpt_ChannelStatus[Channel];
    
    return status;
}

/**
 * @brief Sets channel control mode (sync/async)
 */
Std_ReturnType Gpt_SetControlMode(Gpt_ChannelType Channel, Gpt_ControlType ControlMode)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x11U, GPT_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x11U, GPT_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    if ((ControlMode == GPT_CONTROL_SYNC) || (ControlMode == GPT_CONTROL_ASYNC)) {
        Gpt_ChannelControlMode[Channel] = ControlMode;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Enables capture mode for a channel
 */
Std_ReturnType Gpt_EnableCapture(Gpt_ChannelType Channel, Gpt_CaptureModeType CaptureMode)
{
    uint32 baseAddr;
    uint32 crValue;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x12U, GPT_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x12U, GPT_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    #if (GPT_CAPTURE_MODE_ENABLE == STD_ON)
    if (CaptureMode != GPT_CAPTURE_DISABLE) {
        baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
        
        /* Configure input capture mode */
        crValue = REG_READ32(baseAddr + GPT_CR);
        
        switch (CaptureMode) {
            case GPT_CAPTURE_RISING_EDGE:
                crValue &= ~GPT_CR_IM1_MASK;
                crValue |= (1U << GPT_CR_IM1_SHIFT);
                break;
            case GPT_CAPTURE_FALLING_EDGE:
                crValue &= ~GPT_CR_IM1_MASK;
                crValue |= (2U << GPT_CR_IM1_SHIFT);
                break;
            case GPT_CAPTURE_BOTH_EDGES:
                crValue &= ~GPT_CR_IM1_MASK;
                crValue |= (3U << GPT_CR_IM1_SHIFT);
                break;
            default:
                break;
        }
        
        REG_WRITE32(baseAddr + GPT_CR, crValue);
        
        /* Enable capture interrupt */
        REG_RMW32(baseAddr + GPT_IR, 0U, GPT_IR_IF1IE);
        
        Gpt_ChannelCaptureMode[Channel] = CaptureMode;
        return E_OK;
    }
    #else
    (void)baseAddr;
    (void)crValue;
    #endif
    
    return E_NOT_OK;
}

/**
 * @brief Disables capture mode for a channel
 */
void Gpt_DisableCapture(Gpt_ChannelType Channel)
{
    uint32 baseAddr;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x13U, GPT_E_UNINIT);
        return;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x13U, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    #if (GPT_CAPTURE_MODE_ENABLE == STD_ON)
    baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    
    /* Disable capture interrupt */
    REG_RMW32(baseAddr + GPT_IR, GPT_IR_IF1IE, 0U);
    
    Gpt_ChannelCaptureMode[Channel] = GPT_CAPTURE_DISABLE;
    #else
    (void)baseAddr;
    #endif
    
    (void)Channel;
}

/**
 * @brief Gets captured value for a channel
 */
Gpt_ValueType Gpt_GetCaptureValue(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x14U, GPT_E_UNINIT);
        return 0U;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x14U, GPT_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif
    
    return Gpt_ChannelCaptureValue[Channel];
}

/**
 * @brief Enables PWM mode for a channel
 */
Std_ReturnType Gpt_EnablePwm(Gpt_ChannelType Channel, Gpt_ValueType Period, uint16 DutyCycle)
{
    uint32 baseAddr;
    uint8 chOffset;
    uint32 crValue;
    Gpt_ValueType dutyValue;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x15U, GPT_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x15U, GPT_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    #if (GPT_PWM_MODE_ENABLE == STD_ON)
    if (DutyCycle > 10000U) {
        return E_NOT_OK;
    }
    
    baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    chOffset = Gpt_GetChannelOffset(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    
    /* Calculate duty cycle value */
    dutyValue = (Period * DutyCycle) / 10000U;
    
    /* Set period (OCR1) and duty cycle (OCR2) */
    REG_WRITE32(baseAddr + GPT_OCR1, Period);
    REG_WRITE32(baseAddr + GPT_OCR2, dutyValue);
    
    /* Configure PWM output mode */
    crValue = REG_READ32(baseAddr + GPT_CR);
    crValue &= ~GPT_CR_OM1_MASK;
    crValue |= (3U << GPT_CR_OM1_SHIFT);  /* Toggle output */
    REG_WRITE32(baseAddr + GPT_CR, crValue);
    
    /* Enable timer */
    REG_RMW32(baseAddr + GPT_CR, 0U, GPT_CR_EN);
    
    Gpt_ChannelPwmEnabled[Channel] = TRUE;
    Gpt_ChannelPwmDutyCycle[Channel] = DutyCycle;
    
    return E_OK;
    #else
    (void)baseAddr;
    (void)chOffset;
    (void)crValue;
    (void)dutyValue;
    (void)Period;
    (void)DutyCycle;
    return E_NOT_OK;
    #endif
}

/**
 * @brief Disables PWM mode for a channel
 */
void Gpt_DisablePwm(Gpt_ChannelType Channel)
{
    uint32 baseAddr;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x16U, GPT_E_UNINIT);
        return;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x16U, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif
    
    #if (GPT_PWM_MODE_ENABLE == STD_ON)
    baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    
    /* Disable output */
    REG_RMW32(baseAddr + GPT_CR, GPT_CR_OM1_MASK, 0U);
    
    Gpt_ChannelPwmEnabled[Channel] = FALSE;
    Gpt_ChannelPwmDutyCycle[Channel] = 0U;
    #else
    (void)baseAddr;
    #endif
    
    (void)Channel;
}

/**
 * @brief Sets PWM duty cycle for a channel
 */
Std_ReturnType Gpt_SetPwmDutyCycle(Gpt_ChannelType Channel, uint16 DutyCycle)
{
    uint32 baseAddr;
    Gpt_ValueType period;
    Gpt_ValueType dutyValue;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x17U, GPT_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x17U, GPT_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    
    #if (GPT_PWM_MODE_ENABLE == STD_ON)
    if (!Gpt_ChannelPwmEnabled[Channel]) {
        return E_NOT_OK;
    }
    
    if (DutyCycle > 10000U) {
        return E_NOT_OK;
    }
    
    baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    
    /* Get current period */
    period = REG_READ32(baseAddr + GPT_OCR1);
    
    /* Calculate new duty cycle value */
    dutyValue = (period * DutyCycle) / 10000U;
    
    /* Update duty cycle */
    REG_WRITE32(baseAddr + GPT_OCR2, dutyValue);
    
    Gpt_ChannelPwmDutyCycle[Channel] = DutyCycle;
    
    return E_OK;
    #else
    (void)baseAddr;
    (void)period;
    (void)dutyValue;
    (void)DutyCycle;
    return E_NOT_OK;
    #endif
}

/**
 * @brief Synchronizes all running channels
 */
Std_ReturnType Gpt_Synchronize(void)
{
    uint8 i;
    uint32 baseAddr;
    uint32 syncValue = 0U;
    boolean channelsToSync = FALSE;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x18U, GPT_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    /* Find first running channel to get sync value */
    for (i = 0U; i < Gpt_ConfigPtr->NumChannels; i++) {
        if ((Gpt_ChannelRunning[i]) && (Gpt_ChannelControlMode[i] == GPT_CONTROL_SYNC)) {
            baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[i].ChannelId);
            syncValue = REG_READ32(baseAddr + GPT_CNT);
            channelsToSync = TRUE;
            break;
        }
    }
    
    if (!channelsToSync) {
        return E_OK;
    }
    
    /* Synchronize all other running channels */
    for (i = 0U; i < Gpt_ConfigPtr->NumChannels; i++) {
        if ((Gpt_ChannelRunning[i]) && (Gpt_ChannelControlMode[i] == GPT_CONTROL_SYNC)) {
            baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[i].ChannelId);
            REG_WRITE32(baseAddr + GPT_CNT, syncValue);
        }
    }
    
    return E_OK;
}

/**
 * @brief Converts milliseconds to timer ticks
 */
Gpt_ValueType Gpt_MsToTicks(Gpt_ChannelType Channel, uint32 Milliseconds)
{
    Gpt_ValueType ticks = 0U;
    uint32 clockFreq;
    uint32 prescaler;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x19U, GPT_E_UNINIT);
        return 0U;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x19U, GPT_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif
    
    clockFreq = Gpt_GetChannelClockFreq(Channel);
    prescaler = (uint32)1U << (uint32)Gpt_ConfigPtr->Channels[Channel].ClockPrescaler;
    
    /* Calculate: ticks = (ms * clock_freq) / (1000 * prescaler) */
    if ((clockFreq > 0U) && (prescaler > 0U)) {
        ticks = (Gpt_ValueType)(((uint64)Milliseconds * (uint64)clockFreq) / 
                                ((uint64)1000U * (uint64)prescaler));
    }
    
    return ticks;
}

/**
 * @brief Converts timer ticks to milliseconds
 */
uint32 Gpt_TicksToMs(Gpt_ChannelType Channel, Gpt_ValueType Ticks)
{
    uint32 ms = 0U;
    uint32 clockFreq;
    uint32 prescaler;
    
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x1AU, GPT_E_UNINIT);
        return 0U;
    }
    
    if (!Gpt_IsChannelValid(Channel)) {
        Det_ReportError(GPT_MODULE_ID, 0U, 0x1AU, GPT_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif
    
    clockFreq = Gpt_GetChannelClockFreq(Channel);
    prescaler = (uint32)1U << (uint32)Gpt_ConfigPtr->Channels[Channel].ClockPrescaler;
    
    /* Calculate: ms = (ticks * 1000 * prescaler) / clock_freq */
    if (clockFreq > 0U) {
        ms = (uint32)(((uint64)Ticks * (uint64)1000U * (uint64)prescaler) / 
                      (uint64)clockFreq);
    }
    
    return ms;
}

/******************************************************************************
 * Internal Processing Functions (called from ISR)
 ******************************************************************************/

/**
 * @brief Process timer expiration for a channel (called from ISR)
 */
void Gpt_ProcessExpiration(Gpt_ChannelType Channel)
{
    if (Channel < GPT_MAX_CHANNELS) {
        const Gpt_ChannelConfigType* chCfg = &Gpt_ConfigPtr->Channels[Channel];
        
        /* Call notification callback if enabled */
        if ((Gpt_ChannelNotificationEnabled[Channel]) &&
            (chCfg->NotificationFn != NULL_PTR)) {
            chCfg->NotificationFn();
        }
        
        /* Handle one-shot mode */
        if (chCfg->ChannelMode == GPT_CH_MODE_ONESHOT) {
            Gpt_ChannelRunning[Channel] = FALSE;
            Gpt_ChannelStatus[Channel] = GPT_CH_STATUS_EXPIRED;
        }
    }
}

/**
 * @brief Process capture event for a channel (called from ISR)
 */
void Gpt_ProcessCapture(Gpt_ChannelType Channel)
{
    #if (GPT_CAPTURE_MODE_ENABLE == STD_ON)
    if (Channel < GPT_MAX_CHANNELS) {
        uint32 baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
        
        /* Read captured value */
        Gpt_ChannelCaptureValue[Channel] = REG_READ32(baseAddr + GPT_ICR1);
        
        /* Call notification callback if enabled */
        if ((Gpt_ChannelNotificationEnabled[Channel]) &&
            (Gpt_ConfigPtr->Channels[Channel].NotificationFn != NULL_PTR)) {
            Gpt_ConfigPtr->Channels[Channel].NotificationFn();
        }
    }
    #else
    (void)Channel;
    #endif
}

#define GPT_STOP_SEC_CODE
#include "MemMap.h"
