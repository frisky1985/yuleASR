/**
 * @file Gpt.c
 * @brief GPT Driver implementation for i.MX8M Mini (GPT)
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "Gpt.h"
#include "Gpt_Cfg.h"
#include "Det.h"

#define GPT1_BASE_ADDR                  (0x302E0000UL)
#define GPT2_BASE_ADDR                  (0x302F0000UL)

#define GPT_CR                          (0x00)
#define GPT_PR                          (0x04)
#define GPT_SR                          (0x08)
#define GPT_IR                          (0x0C)
#define GPT_OCR1                        (0x10)
#define GPT_OCR2                        (0x14)
#define GPT_OCR3                        (0x18)
#define GPT_ICR1                        (0x1C)
#define GPT_ICR2                        (0x20)
#define GPT_CNT                         (0x24)

#define GPT_CR_EN                       (0x00000001U)
#define GPT_CR_ENMOD                    (0x00000002U)
#define GPT_CR_DBGEN                    (0x00000004U)
#define GPT_CR_WAITEN                   (0x00000008U)
#define GPT_CR_DOZEEN                   (0x00000010U)
#define GPT_CR_STOPEN                   (0x00000020U)
#define GPT_CR_CLKSRC_MASK              (0x000001C0U)
#define GPT_CR_FRR                      (0x00000200U)
#define GPT_CR_SWR                      (0x00010000U)
#define GPT_CR_IM1_MASK                 (0x000C0000U)
#define GPT_CR_IM2_MASK                 (0x00300000U)
#define GPT_CR_OM1_MASK                 (0x00C00000U)
#define GPT_CR_OM2_MASK                 (0x03000000U)
#define GPT_CR_OM3_MASK                 (0x0C000000U)
#define GPT_CR_FO1                      (0x10000000U)
#define GPT_CR_FO2                      (0x20000000U)
#define GPT_CR_FO3                      (0x40000000U)

#define GPT_SR_OF1                      (0x00000001U)
#define GPT_SR_OF2                      (0x00000002U)
#define GPT_SR_OF3                      (0x00000004U)
#define GPT_SR_IF1                      (0x00000008U)
#define GPT_SR_IF2                      (0x00000010U)
#define GPT_SR_ROV                      (0x00000020U)

#define GPT_IR_OF1IE                    (0x00000001U)
#define GPT_IR_OF2IE                    (0x00000002U)
#define GPT_IR_OF3IE                    (0x00000004U)
#define GPT_IR_IF1IE                    (0x00000008U)
#define GPT_IR_IF2IE                    (0x00000010U)
#define GPT_IR_ROVIE                    (0x00000020U)

#define GPT_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Gpt_DriverInitialized = FALSE;
static Gpt_ModeType Gpt_DriverMode = GPT_MODE_NORMAL;
static const Gpt_ConfigType* Gpt_ConfigPtr = NULL_PTR;
static Gpt_ValueType Gpt_ChannelTargetValue[GPT_NUM_CHANNELS];
static Gpt_ValueType Gpt_ChannelElapsedValue[GPT_NUM_CHANNELS];
static boolean Gpt_ChannelRunning[GPT_NUM_CHANNELS];

#define GPT_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static uint32 Gpt_GetBaseAddr(Gpt_ChannelType channel)
{
    uint32 baseAddr;
    switch (channel) {
        case GPT_CHANNEL_0:
        case GPT_CHANNEL_1:
        case GPT_CHANNEL_2:
        case GPT_CHANNEL_3:
            baseAddr = GPT1_BASE_ADDR;
            break;
        case GPT_CHANNEL_4:
        case GPT_CHANNEL_5:
        case GPT_CHANNEL_6:
        case GPT_CHANNEL_7:
            baseAddr = GPT2_BASE_ADDR;
            break;
        default:
            baseAddr = 0U;
            break;
    }
    return baseAddr;
}

static uint8 Gpt_GetChannelOffset(Gpt_ChannelType channel)
{
    return (uint8)(channel % 4U);
}

static void Gpt_EnableClock(Gpt_ChannelType channel)
{
    (void)channel;
}

static void Gpt_DisableClock(Gpt_ChannelType channel)
{
    (void)channel;
}

#define GPT_START_SEC_CODE
#include "MemMap.h"

void Gpt_Init(const Gpt_ConfigType* ConfigPtr)
{
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

    for (uint8 i = 0U; i < GPT_NUM_CHANNELS; i++) {
        const Gpt_ChannelConfigType* chConfig = &ConfigPtr->Channels[i];
        uint32 baseAddr = Gpt_GetBaseAddr(chConfig->ChannelId);
        if (baseAddr == 0U) continue;

        Gpt_EnableClock(chConfig->ChannelId);

        /* Software reset */
        REG_WRITE32(baseAddr + GPT_CR, GPT_CR_SWR);
        while ((REG_READ32(baseAddr + GPT_CR) & GPT_CR_SWR) != 0U);

        /* Configure prescaler */
        uint32 prValue = (1U << chConfig->ClockPrescaler) - 1U;
        REG_WRITE32(baseAddr + GPT_PR, prValue);

        /* Configure control register */
        uint32 crValue = 0U;
        crValue |= GPT_CR_FRR; /* Free-run mode */
        crValue |= (0x01U << 6); /* Peripheral clock source */
        REG_WRITE32(baseAddr + GPT_CR, crValue);

        /* Clear status */
        REG_WRITE32(baseAddr + GPT_SR, 0x3FU);

        /* Disable interrupts */
        REG_WRITE32(baseAddr + GPT_IR, 0U);

        Gpt_ChannelRunning[i] = FALSE;
        Gpt_ChannelTargetValue[i] = 0U;
        Gpt_ChannelElapsedValue[i] = 0U;
    }

    Gpt_DriverMode = ConfigPtr->DefaultMode;
    Gpt_DriverInitialized = TRUE;
}

#if (GPT_DEINIT_API == STD_ON)
void Gpt_DeInit(void)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DEINIT, GPT_E_UNINIT);
        return;
    }
    #endif

    for (uint8 i = 0U; i < GPT_NUM_CHANNELS; i++) {
        if (Gpt_ChannelRunning[i]) {
            return; /* Cannot deinit if any channel is running */
        }
    }

    for (uint8 i = 0U; i < GPT_NUM_CHANNELS; i++) {
        uint32 baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[i].ChannelId);
        if (baseAddr == 0U) continue;

        /* Disable timer */
        uint32 crValue = REG_READ32(baseAddr + GPT_CR);
        crValue &= ~GPT_CR_EN;
        REG_WRITE32(baseAddr + GPT_CR, crValue);

        Gpt_DisableClock(Gpt_ConfigPtr->Channels[i].ChannelId);
    }

    Gpt_DriverInitialized = FALSE;
}
#endif

#if (GPT_TIME_ELAPSED_API == STD_ON)
Gpt_ValueType Gpt_GetTimeElapsed(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEELAPSED, GPT_E_UNINIT);
        return 0U;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEELAPSED, GPT_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif

    uint32 baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    Gpt_ValueType elapsed = REG_READ32(baseAddr + GPT_CNT);

    return elapsed;
}
#endif

#if (GPT_TIME_REMAINING_API == STD_ON)
Gpt_ValueType Gpt_GetTimeRemaining(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEREMAINING, GPT_E_UNINIT);
        return 0U;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_GETTIMEREMAINING, GPT_E_PARAM_CHANNEL);
        return 0U;
    }
    #endif

    if (!Gpt_ChannelRunning[Channel]) {
        return 0U;
    }

    uint32 baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    Gpt_ValueType current = REG_READ32(baseAddr + GPT_CNT);

    if (Gpt_ChannelTargetValue[Channel] > current) {
        return Gpt_ChannelTargetValue[Channel] - current;
    }
    return 0U;
}
#endif

void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_PARAM_CHANNEL);
        return;
    }
    if (Value == 0U || Value > Gpt_ConfigPtr->Channels[Channel].MaxTickValue) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_PARAM_VALUE);
        return;
    }
    if (Gpt_ChannelRunning[Channel]) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STARTTIMER, GPT_E_CHANNEL_BUSY);
        return;
    }
    #endif

    uint32 baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    uint8 chOffset = Gpt_GetChannelOffset(Gpt_ConfigPtr->Channels[Channel].ChannelId);

    Gpt_ChannelTargetValue[Channel] = Value;
    Gpt_ChannelRunning[Channel] = TRUE;

    /* Set output compare value */
    REG_WRITE32(baseAddr + GPT_OCR1 + (chOffset * 4U), Value);

    /* Enable interrupt if notification is enabled */
    if (Gpt_ConfigPtr->Channels[Channel].NotificationEnabled) {
        uint32 irValue = REG_READ32(baseAddr + GPT_IR);
        irValue |= (GPT_IR_OF1IE << chOffset);
        REG_WRITE32(baseAddr + GPT_IR, irValue);
    }

    /* Enable timer */
    uint32 crValue = REG_READ32(baseAddr + GPT_CR);
    crValue |= GPT_CR_EN;
    REG_WRITE32(baseAddr + GPT_CR, crValue);
}

void Gpt_StopTimer(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STOPTIMER, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_STOPTIMER, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif

    uint32 baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);

    /* Disable interrupt */
    uint32 irValue = REG_READ32(baseAddr + GPT_IR);
    irValue &= ~GPT_IR_OF1IE;
    REG_WRITE32(baseAddr + GPT_IR, irValue);

    /* Disable timer */
    uint32 crValue = REG_READ32(baseAddr + GPT_CR);
    crValue &= ~GPT_CR_EN;
    REG_WRITE32(baseAddr + GPT_CR, crValue);

    Gpt_ChannelRunning[Channel] = FALSE;
}

#if (GPT_ENABLE_DISABLE_NOTIFICATION_API == STD_ON)
void Gpt_EnableNotification(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEINTERRUPT, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEINTERRUPT, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif

    if (Gpt_ChannelRunning[Channel]) {
        uint32 baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
        uint8 chOffset = Gpt_GetChannelOffset(Gpt_ConfigPtr->Channels[Channel].ChannelId);

        uint32 irValue = REG_READ32(baseAddr + GPT_IR);
        irValue |= (GPT_IR_OF1IE << chOffset);
        REG_WRITE32(baseAddr + GPT_IR, irValue);
    }
}

void Gpt_DisableNotification(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEINTERRUPT, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEINTERRUPT, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif

    uint32 baseAddr = Gpt_GetBaseAddr(Gpt_ConfigPtr->Channels[Channel].ChannelId);
    uint8 chOffset = Gpt_GetChannelOffset(Gpt_ConfigPtr->Channels[Channel].ChannelId);

    uint32 irValue = REG_READ32(baseAddr + GPT_IR);
    irValue &= ~(GPT_IR_OF1IE << chOffset);
    REG_WRITE32(baseAddr + GPT_IR, irValue);
}
#endif

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

void Gpt_SetMode(Gpt_ModeType Mode)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_SETMODE, GPT_E_UNINIT);
        return;
    }
    #endif

    if (Mode == GPT_MODE_SLEEP) {
        /* Stop all channels */
        for (uint8 i = 0U; i < GPT_NUM_CHANNELS; i++) {
            if (Gpt_ChannelRunning[i]) {
                Gpt_StopTimer(i);
            }
        }
    }

    Gpt_DriverMode = Mode;
}

#if (GPT_WAKEUP_FUNCTIONALITY_API == STD_ON)
void Gpt_DisableWakeup(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEWAKEUP, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_DISABLEWAKEUP, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif
    (void)Channel;
}

void Gpt_EnableWakeup(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEWAKEUP, GPT_E_UNINIT);
        return;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_ENABLEWAKEUP, GPT_E_PARAM_CHANNEL);
        return;
    }
    #endif
    (void)Channel;
}

Std_ReturnType Gpt_CheckWakeup(Gpt_ChannelType Channel)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    if (Gpt_DriverInitialized == FALSE) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_CHECKWAKEUP, GPT_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= GPT_NUM_CHANNELS) {
        Det_ReportError(GPT_MODULE_ID, 0U, GPT_SID_CHECKWAKEUP, GPT_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
    #endif
    (void)Channel;
    return E_NOT_OK;
}
#endif

#if (GPT_PREDEF_TIMER_1US_16BIT_ENABLE == STD_ON) || \
    (GPT_PREDEF_TIMER_1US_24BIT_ENABLE == STD_ON) || \
    (GPT_PREDEF_TIMER_1US_32BIT_ENABLE == STD_ON) || \
    (GPT_PREDEF_TIMER_100US_32BIT_ENABLE == STD_ON)
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

    uint32 baseAddr = GPT1_BASE_ADDR;
    *TimeValuePtr = REG_READ32(baseAddr + GPT_CNT);

    (void)PredefTimer;
    return E_OK;
}
#endif

#define GPT_STOP_SEC_CODE
#include "MemMap.h"
