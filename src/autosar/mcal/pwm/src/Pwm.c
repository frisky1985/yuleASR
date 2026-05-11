/**
 * @file Pwm.c
 * @brief PWM Driver implementation for i.MX8M Mini (PWM)
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "Pwm.h"
#include "Pwm_Cfg.h"
#include "Det.h"

#define PWM1_BASE_ADDR                  (0x30660000UL)
#define PWM2_BASE_ADDR                  (0x30670000UL)
#define PWM3_BASE_ADDR                  (0x30680000UL)
#define PWM4_BASE_ADDR                  (0x30690000UL)

#define PWM_CR                          (0x00)
#define PWM_SR                          (0x04)
#define PWM_IR                          (0x08)
#define PWM_SAR                         (0x0C)
#define PWM_PR                          (0x10)
#define PWM_CNR                         (0x14)

#define PWM_CR_EN                       (0x00000001U)
#define PWM_CR_REPEAT_MASK              (0x00000006U)
#define PWM_CR_SWR                      (0x00000008U)
#define PWM_CR_PRESCALER_MASK           (0x00000FF0U)
#define PWM_CR_CLKSRC_MASK              (0x00003000U)
#define PWM_CR_POUTC_MASK               (0x0000C000U)
#define PWM_CR_HCTR                     (0x00010000U)
#define PWM_CR_BCTR                     (0x00020000U)
#define PWM_CR_DBGEN                    (0x00040000U)
#define PWM_CR_WAITEN                   (0x00080000U)
#define PWM_CR_DOZEEN                   (0x00100000U)
#define PWM_CR_STOPEN                   (0x00200000U)
#define PWM_CR_FWM_MASK                 (0x03000000U)

#define PWM_SR_FE                       (0x00000001U)
#define PWM_SR_ROV                      (0x00000002U)
#define PWM_SR_CMP                      (0x00000004U)
#define PWM_SR_FWE                      (0x00000008U)

#define PWM_IR_FIE                      (0x00000001U)
#define PWM_IR_RIE                      (0x00000002U)
#define PWM_IR_CIE                      (0x00000004U)

#define PWM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Pwm_DriverInitialized = FALSE;
static const Pwm_ConfigType* Pwm_ConfigPtr = NULL_PTR;
static uint16 Pwm_ChannelDutyCycle[PWM_NUM_CHANNELS];

#define PWM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static uint32 Pwm_GetBaseAddr(Pwm_ChannelType channel)
{
    uint32 baseAddr;
    switch (channel) {
        case PWM_CHANNEL_0: baseAddr = PWM1_BASE_ADDR; break;
        case PWM_CHANNEL_1: baseAddr = PWM2_BASE_ADDR; break;
        case PWM_CHANNEL_2: baseAddr = PWM3_BASE_ADDR; break;
        case PWM_CHANNEL_3: baseAddr = PWM4_BASE_ADDR; break;
        default: baseAddr = 0U; break;
    }
    return baseAddr;
}

static void Pwm_EnableClock(Pwm_ChannelType channel)
{
    (void)channel;
}

static void Pwm_DisableClock(Pwm_ChannelType channel)
{
    (void)channel;
}

#define PWM_START_SEC_CODE
#include "MemMap.h"

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

    for (uint8 i = 0U; i < PWM_NUM_CHANNELS; i++) {
        const Pwm_ChannelConfigType* chConfig = &ConfigPtr->Channels[i];
        uint32 baseAddr = Pwm_GetBaseAddr(chConfig->ChannelId);
        if (baseAddr == 0U) continue;

        Pwm_EnableClock(chConfig->ChannelId);

        /* Software reset */
        REG_WRITE32(baseAddr + PWM_CR, PWM_CR_SWR);
        while ((REG_READ32(baseAddr + PWM_CR) & PWM_CR_SWR) != 0U);

        /* Configure period */
        REG_WRITE32(baseAddr + PWM_PR, chConfig->DefaultPeriod);

        /* Configure sample (duty cycle) */
        uint32 sample = (chConfig->DefaultDutyCycle * chConfig->DefaultPeriod) / PWM_DUTY_CYCLE_RESOLUTION;
        REG_WRITE32(baseAddr + PWM_SAR, sample);

        /* Configure control register */
        uint32 crValue = 0U;
        crValue |= ((uint32)chConfig->ClockPrescaler << 4) & PWM_CR_PRESCALER_MASK;
        crValue |= PWM_CR_EN;
        REG_WRITE32(baseAddr + PWM_CR, crValue);

        Pwm_ChannelDutyCycle[i] = chConfig->DefaultDutyCycle;
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

    for (uint8 i = 0U; i < PWM_NUM_CHANNELS; i++) {
        uint32 baseAddr = Pwm_GetBaseAddr(Pwm_ConfigPtr->Channels[i].ChannelId);
        if (baseAddr == 0U) continue;

        /* Disable PWM */
        REG_WRITE32(baseAddr + PWM_CR, 0U);

        Pwm_DisableClock(Pwm_ConfigPtr->Channels[i].ChannelId);
    }

    Pwm_DriverInitialized = FALSE;
}
#endif

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

    uint32 baseAddr = Pwm_GetBaseAddr(Channel);
    uint32 period = REG_READ32(baseAddr + PWM_PR);

    /* Calculate sample value from duty cycle */
    uint32 sample = ((uint32)DutyCycle * period) / PWM_DUTY_CYCLE_RESOLUTION;

    REG_WRITE32(baseAddr + PWM_SAR, sample);
    Pwm_ChannelDutyCycle[Channel] = DutyCycle;
}

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
    if (Pwm_ConfigPtr->Channels[Channel].ChannelClass == PWM_FIXED_PERIOD) {
        Det_ReportError(PWM_MODULE_ID, 0U, PWM_SID_SETPERIODANDDUTY, PWM_E_PERIOD_UNCHANGEABLE);
        return;
    }
    #endif

    uint32 baseAddr = Pwm_GetBaseAddr(Channel);

    /* Set new period */
    REG_WRITE32(baseAddr + PWM_PR, Period);

    /* Set duty cycle */
    uint32 sample = ((uint32)DutyCycle * Period) / PWM_DUTY_CYCLE_RESOLUTION;
    REG_WRITE32(baseAddr + PWM_SAR, sample);

    Pwm_ChannelDutyCycle[Channel] = DutyCycle;
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

    uint32 baseAddr = Pwm_GetBaseAddr(Channel);

    /* Set duty cycle to 0 (idle) */
    REG_WRITE32(baseAddr + PWM_SAR, 0U);
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

    /* Read current counter value and compare */
    uint32 baseAddr = Pwm_GetBaseAddr(Channel);
    uint32 cnr = REG_READ32(baseAddr + PWM_CNR);
    uint32 sar = REG_READ32(baseAddr + PWM_SAR);

    if (cnr < sar) {
        return PWM_HIGH;
    }
    return PWM_LOW;
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

    uint32 baseAddr = Pwm_GetBaseAddr(Channel);
    REG_WRITE32(baseAddr + PWM_IR, 0U);
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

    uint32 baseAddr = Pwm_GetBaseAddr(Channel);
    uint32 irValue = 0U;

    if (Notification == PWM_RISING_EDGE || Notification == PWM_BOTH_EDGES) {
        irValue |= PWM_IR_FIE;
    }
    if (Notification == PWM_FALLING_EDGE || Notification == PWM_BOTH_EDGES) {
        irValue |= PWM_IR_CIE;
    }

    REG_WRITE32(baseAddr + PWM_IR, irValue);
    (void)Notification;
}
#endif

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
    (void)PowerState;
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
    *TargetPowerState = PWM_FULL_POWER;
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
    *CurrentPowerState = PWM_FULL_POWER;
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
    (void)PowerState;
    *Result = PWM_SERVICE_ACCEPTED;
}
#endif

#define PWM_STOP_SEC_CODE
#include "MemMap.h"
