/******************************************************************************
 * @file    WdgIf_Hw_S32K3.c
 * @brief   Watchdog Interface Hardware Implementation for NXP S32K3
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ecual/wdgIf/WdgIf_Hw_S32K3.h"
#include <string.h>

/******************************************************************************
 * Local Variables
 ******************************************************************************/
static WdgIf_S32K3HwContextType WdgIf_S32K3Contexts[WDGIF_MAX_DEVICES];
static boolean WdgIf_S32K3Initialized = FALSE;

/* SWT base addresses */
static const uint32 WdgIf_S32K3_SwtBaseAddresses[S32K3_SWT_MAX] = {
    S32K3_SWT0_BASE_ADDR,
    S32K3_SWT1_BASE_ADDR,
    S32K3_SWT2_BASE_ADDR
};

/* Default S32K3 hardware configuration */
static const WdgIf_S32K3HwConfigType WdgIf_S32K3DefaultConfig = {
    .swtBaseAddress = S32K3_SWT0_BASE_ADDR,
    .clockSource = 0U,          /* FIRC */
    .clockFrequency = S32K3_WDG_FIRC_FREQ,
    .clockDivider = 128U,
    .useWindowMode = FALSE,
    .serviceMode = S32K3_SWT_SMD_FIXED_KEY,
    .enableFreeze = TRUE,
    .enableStop = FALSE,
    .interruptThenReset = FALSE,
    .masterAccessMask = 0xFFFFFFFFU
};

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static Std_ReturnType WdgIf_S32K3_InitSwt(
    WdgIf_DeviceType device,
    const WdgIf_DeviceConfigType *config
);
static void WdgIf_S32K3_DisableWatchdog(WdgIf_S32K3HwContextType *context);
static uint32 WdgIf_S32K3_CalculateTimeoutValue(
    uint32 timeoutMs,
    uint32 clockFreq,
    uint16 divider
);

/******************************************************************************
 * Hardware Interface Structure
 ******************************************************************************/
const WdgIf_HwInterfaceType WdgIf_S32K3HwInterface = {
    .init = WdgIf_S32K3_Init,
    .deinit = WdgIf_S32K3_DeInit,
    .setMode = WdgIf_S32K3_SetMode,
    .trigger = WdgIf_S32K3_Trigger,
    .getVersion = WdgIf_S32K3_GetVersion,
    .reset = WdgIf_S32K3_Reset,
    .getStatus = WdgIf_S32K3_GetStatus
};

/******************************************************************************
 * API Implementation
 ******************************************************************************/

/**
 * @brief Initialize S32K3 watchdog hardware
 */
Std_ReturnType WdgIf_S32K3_Init(const WdgIf_DeviceConfigType *config)
{
    Std_ReturnType result = E_OK;

    if (NULL_PTR == config) {
        return E_NOT_OK;
    }

    /* Initialize SWT for internal watchdog */
    if (WDGIF_INTERNAL_WDG == config->deviceId) {
        result = WdgIf_S32K3_InitSwt(config->deviceId, config);
    }
#ifdef WDGIF_EXTERNAL_WDG_SUPPORTED
    else if ((WDGIF_EXTERNAL_WDG1 == config->deviceId) ||
             (WDGIF_EXTERNAL_WDG2 == config->deviceId)) {
        result = WdgIf_S32K3_InitExternalWdg(config->deviceId, 0U);
    }
#endif
    else {
        result = E_NOT_OK;
    }

    if (E_OK == result) {
        WdgIf_S32K3Initialized = TRUE;
    }

    return result;
}

/**
 * @brief Initialize SWT module
 */
static Std_ReturnType WdgIf_S32K3_InitSwt(
    WdgIf_DeviceType device,
    const WdgIf_DeviceConfigType *config)
{
    WdgIf_S32K3HwContextType *context;
    const WdgIf_S32K3HwConfigType *hwConfig;
    uint32 timeoutValue;
    uint32 crValue;
    uint8 swtInstance;

    if (device >= WDGIF_MAX_DEVICES) {
        return E_NOT_OK;
    }

    context = &WdgIf_S32K3Contexts[device];

    /* Get hardware configuration */
    if ((NULL_PTR != config->hardwareConfig) &&
        (((WdgIf_S32K3HwConfigType*)config->hardwareConfig)->swtBaseAddress != 0U)) {
        hwConfig = (const WdgIf_S32K3HwConfigType*)config->hardwareConfig;
    } else {
        hwConfig = &WdgIf_S32K3DefaultConfig;
    }

    /* Determine SWT instance from base address */
    swtInstance = 0U;
    for (swtInstance = 0U; swtInstance < S32K3_SWT_MAX; swtInstance++) {
        if (WdgIf_S32K3_SwtBaseAddresses[swtInstance] == hwConfig->swtBaseAddress) {
            break;
        }
    }
    if (swtInstance >= S32K3_SWT_MAX) {
        swtInstance = 0U;  /* Default to SWT0 */
    }

    /* Initialize context */
    (void)memset(context, 0, sizeof(WdgIf_S32K3HwContextType));
    context->swtInstance = swtInstance;

    /* Setup register pointers */
    context->crReg = (volatile uint32 *)(hwConfig->swtBaseAddress + S32K3_SWT_CR_OFFSET);
    context->irReg = (volatile uint32 *)(hwConfig->swtBaseAddress + S32K3_SWT_IR_OFFSET);
    context->toReg = (volatile uint32 *)(hwConfig->swtBaseAddress + S32K3_SWT_TO_OFFSET);
    context->wnReg = (volatile uint32 *)(hwConfig->swtBaseAddress + S32K3_SWT_WN_OFFSET);
    context->srReg = (volatile uint32 *)(hwConfig->swtBaseAddress + S32K3_SWT_SR_OFFSET);
    context->coReg = (volatile uint32 *)(hwConfig->swtBaseAddress + S32K3_SWT_CO_OFFSET);
    context->skReg = (volatile uint32 *)(hwConfig->swtBaseAddress + S32K3_SWT_SK_OFFSET);

    /* Check if watchdog is hard-locked */
    if ((*context->crReg & S32K3_SWT_CR_HLK) != 0U) {
        /* Cannot modify configuration - watchdog is hard-locked */
        return E_NOT_OK;
    }

    /* Calculate timeout value */
    if ((NULL_PTR != config->timeoutConfig) &&
        (config->timeoutConfig->slowModeTimeout > 0U)) {
        timeoutValue = WdgIf_S32K3_CalculateTimeoutValue(
            config->timeoutConfig->slowModeTimeout,
            hwConfig->clockFrequency,
            hwConfig->clockDivider
        );
    } else {
        timeoutValue = WdgIf_S32K3_CalculateTimeoutValue(
            S32K3_WDG_DEFAULT_TIMEOUT_MS,
            hwConfig->clockFrequency,
            hwConfig->clockDivider
        );
    }
    context->timeoutValue = timeoutValue;

    /* Disable watchdog temporarily for configuration */
    WdgIf_S32K3_DisableWatchdog(context);

    /* Configure timeout register */
    *context->toReg = timeoutValue;

    /* Configure window register if window mode enabled */
    if (hwConfig->useWindowMode) {
        /* Window start at 25% of timeout */
        context->windowValue = timeoutValue / 4U;
        *context->wnReg = context->windowValue;
    } else {
        *context->wnReg = 0U;
    }

    /* Clear any pending interrupt flags */
    *context->irReg = S32K3_SWT_IR_TIF;

    /* Build control register value */
    crValue = S32K3_SWT_CR_WEN;  /* Enable watchdog */

    if (hwConfig->enableFreeze) {
        crValue |= S32K3_SWT_CR_FRZ;
    }

    if (hwConfig->enableStop) {
        crValue |= S32K3_SWT_CR_STP;
    }

    if (hwConfig->useWindowMode) {
        crValue |= S32K3_SWT_CR_WND;
    }

    if (hwConfig->interruptThenReset) {
        crValue |= S32K3_SWT_CR_ITR;
    }

    /* Set service mode */
    crValue |= ((uint32)hwConfig->serviceMode << S32K3_SWT_CR_SMD_SHIFT) &
               S32K3_SWT_CR_SMD_MASK;

    /* Set master access protection */
    crValue |= ((uint32)hwConfig->masterAccessMask << S32K3_SWT_CR_MAP_SHIFT) &
               S32K3_SWT_CR_MAP_MASK;

    /* Write control register */
    *context->crReg = crValue;

    /* Perform initial service sequence */
    (void)WdgIf_S32K3_Service(context);

    /* Soft lock the configuration if not in debug */
    crValue |= S32K3_SWT_CR_SLK;
    *context->crReg = crValue;

    context->isLocked = TRUE;

    return E_OK;
}

/**
 * @brief Disable SWT watchdog
 */
static void WdgIf_S32K3_DisableWatchdog(WdgIf_S32K3HwContextType *context)
{
    uint32 crValue;

    if (NULL_PTR == context->crReg) {
        return;
    }

    /* Read current control register */
    crValue = *context->crReg;

    /* Check if soft-locked */
    if ((crValue & S32K3_SWT_CR_SLK) != 0U) {
        /* Need to unlock first */
        (void)WdgIf_S32K3_Unlock(context);
    }

    /* Disable watchdog */
    crValue &= ~S32K3_SWT_CR_WEN;
    *context->crReg = crValue;
}

/**
 * @brief Deinitialize S32K3 watchdog hardware
 */
Std_ReturnType WdgIf_S32K3_DeInit(WdgIf_DeviceType device)
{
    WdgIf_S32K3HwContextType *context;

    if (device >= WDGIF_MAX_DEVICES) {
        return E_NOT_OK;
    }

    context = &WdgIf_S32K3Contexts[device];

    if (NULL_PTR != context->crReg) {
        /* Disable watchdog */
        WdgIf_S32K3_DisableWatchdog(context);
    }

    /* Clear context */
    (void)memset(context, 0, sizeof(WdgIf_S32K3HwContextType));

    return E_OK;
}

/**
 * @brief Set S32K3 watchdog mode
 */
Std_ReturnType WdgIf_S32K3_SetMode(WdgIf_DeviceType device, WdgIf_ModeType mode)
{
    WdgIf_S32K3HwContextType *context;
    uint32 timeoutValue;
    uint32 crValue;
    uint16 timeoutMs = S32K3_WDG_DEFAULT_TIMEOUT_MS;

    if (device >= WDGIF_MAX_DEVICES) {
        return E_NOT_OK;
    }

    context = &WdgIf_S32K3Contexts[device];

    if ((NULL_PTR == context->crReg) || (NULL_PTR == context->toReg)) {
        return E_NOT_OK;
    }

    /* Determine timeout based on mode */
    switch (mode) {
        case WDGIF_OFF_MODE:
            /* Disable watchdog */
            WdgIf_S32K3_DisableWatchdog(context);
            return E_OK;

        case WDGIF_SLOW_MODE:
            timeoutMs = 500U;  /* 500ms for slow mode */
            break;

        case WDGIF_FAST_MODE:
            timeoutMs = 50U;   /* 50ms for fast mode */
            break;

        default:
            return E_NOT_OK;
    }

    /* Calculate new timeout value */
    timeoutValue = WdgIf_S32K3_CalculateTimeoutValue(
        timeoutMs,
        S32K3_WDG_FIRC_FREQ,
        128U
    );

    /* Unlock if needed */
    if (context->isLocked) {
        if (E_OK != WdgIf_S32K3_Unlock(context)) {
            return E_NOT_OK;
        }
    }

    /* Update timeout register */
    *context->toReg = timeoutValue;
    context->timeoutValue = timeoutValue;

    /* Enable watchdog if not enabled */
    crValue = *context->crReg;
    if ((crValue & S32K3_SWT_CR_WEN) == 0U) {
        crValue |= S32K3_SWT_CR_WEN;
        *context->crReg = crValue;
    }

    /* Re-lock if was locked */
    if (context->isLocked) {
        crValue |= S32K3_SWT_CR_SLK;
        *context->crReg = crValue;
    }

    /* Service the watchdog with new timeout */
    (void)WdgIf_S32K3_Service(context);

    return E_OK;
}

/**
 * @brief Trigger S32K3 watchdog
 */
Std_ReturnType WdgIf_S32K3_Trigger(WdgIf_DeviceType device)
{
    WdgIf_S32K3HwContextType *context;

    if (device >= WDGIF_MAX_DEVICES) {
        return E_NOT_OK;
    }

    context = &WdgIf_S32K3Contexts[device];

    return WdgIf_S32K3_Service(context);
}

/**
 * @brief Perform service sequence
 */
Std_ReturnType WdgIf_S32K3_Service(WdgIf_S32K3HwContextType *context)
{
    uint32 crValue;
    uint8 serviceMode;

    if ((NULL_PTR == context) || (NULL_PTR == context->srReg)) {
        return E_NOT_OK;
    }

    /* Read control register to get service mode */
    crValue = *context->crReg;
    serviceMode = (uint8)((crValue & S32K3_SWT_CR_SMD_MASK) >>
                          S32K3_SWT_CR_SMD_SHIFT);

    /* Perform service based on mode */
    switch (serviceMode) {
        case S32K3_SWT_SMD_FIXED_KEY:
            /* Fixed key sequence: 0xA602, 0xB480 */
            *context->srReg = S32K3_SWT_SR_SERVICE_KEY1;
            *context->srReg = S32K3_SWT_SR_SERVICE_KEY2;
            break;

        case S32K3_SWT_SMD_INCREMENT_KEY:
            /* Incrementing key - not implemented in this version */
            return E_NOT_OK;

        case S32K3_SWT_SMD_INDEXED_KEY:
            /* Indexed key - not implemented in this version */
            return E_NOT_OK;

        default:
            return E_NOT_OK;
    }

    return E_OK;
}

/**
 * @brief Get S32K3 watchdog hardware version
 */
Std_ReturnType WdgIf_S32K3_GetVersion(uint16 *version)
{
    if (NULL_PTR == version) {
        return E_NOT_OK;
    }

    /* Return version for S32K3 SWT */
    *version = 0x0300U;  /* Version 3.0 for S32K3 */

    return E_OK;
}

/**
 * @brief Force reset via S32K3 watchdog
 */
Std_ReturnType WdgIf_S32K3_Reset(WdgIf_DeviceType device)
{
    WdgIf_S32K3HwContextType *context;
    uint32 crValue;

    if (device >= WDGIF_MAX_DEVICES) {
        return E_NOT_OK;
    }

    context = &WdgIf_S32K3Contexts[device];

    if (NULL_PTR == context->crReg) {
        return E_NOT_OK;
    }

    /* Unlock if needed */
    if (context->isLocked) {
        (void)WdgIf_S32K3_Unlock(context);
    }

    /* Set timeout to minimum value (1 tick) */
    *context->toReg = 1U;

    /* Enable watchdog */
    crValue = *context->crReg;
    crValue |= S32K3_SWT_CR_WEN;
    *context->crReg = crValue;

    /* Don't service - let it timeout */
    /* Wait for reset (should not reach here) */
    while (1) {
        /* Infinite loop - reset should occur */
    }

    return E_OK;
}

/**
 * @brief Get S32K3 watchdog status
 */
WdgIf_DriverStatusType WdgIf_S32K3_GetStatus(WdgIf_DeviceType device)
{
    WdgIf_S32K3HwContextType *context;
    uint32 crValue;

    if (device >= WDGIF_MAX_DEVICES) {
        return WDGIF_STATUS_ERROR;
    }

    if (!WdgIf_S32K3Initialized) {
        return WDGIF_STATUS_UNINIT;
    }

    context = &WdgIf_S32K3Contexts[device];

    if (NULL_PTR == context->crReg) {
        return WDGIF_STATUS_ERROR;
    }

    /* Read control register */
    crValue = *context->crReg;

    /* Check if watchdog is enabled */
    if ((crValue & S32K3_SWT_CR_WEN) == 0U) {
        return WDGIF_STATUS_IDLE;
    }

    /* Check for timeout interrupt flag */
    if ((NULL_PTR != context->irReg) &&
        ((*context->irReg & S32K3_SWT_IR_TIF) != 0U)) {
        return WDGIF_STATUS_ERROR;
    }

    return WDGIF_STATUS_BUSY;
}

/******************************************************************************
 * Low-Level Functions
 ******************************************************************************/

/**
 * @brief Calculate timeout register value
 */
uint32 WdgIf_S32K3_CalculateTimeoutValue(
    uint32 timeoutMs,
    uint32 clockFreq,
    uint16 divider)
{
    uint32 ticks;

    /* Calculate ticks: timeout_ms * clock_freq / (divider * 1000) */
    ticks = (timeoutMs * clockFreq) / ((uint32)divider * 1000U);

    /* Limit to 32-bit value */
    if (ticks > 0xFFFFFFFFU) {
        ticks = 0xFFFFFFFFU;
    }

    /* Ensure minimum of 1 tick */
    if (ticks == 0U) {
        ticks = 1U;
    }

    return ticks;
}

/**
 * @brief Unlock SWT registers
 */
Std_ReturnType WdgIf_S32K3_Unlock(WdgIf_S32K3HwContextType *context)
{
    if ((NULL_PTR == context) || (NULL_PTR == context->srReg)) {
        return E_NOT_OK;
    }

    /* Write unlock sequence to service register */
    *context->srReg = S32K3_SWT_SR_SERVICE_KEY1;
    *context->srReg = S32K3_SWT_SR_SERVICE_KEY2;

    context->isLocked = FALSE;

    return E_OK;
}

/**
 * @brief Lock SWT registers
 */
void WdgIf_S32K3_Lock(WdgIf_S32K3HwContextType *context)
{
    uint32 crValue;

    if ((NULL_PTR == context) || (NULL_PTR == context->crReg)) {
        return;
    }

    /* Set soft lock bit */
    crValue = *context->crReg;
    crValue |= S32K3_SWT_CR_SLK;
    *context->crReg = crValue;

    context->isLocked = TRUE;
}

/**
 * @brief Get current counter value
 */
uint32 WdgIf_S32K3_GetCounter(WdgIf_S32K3HwContextType *context)
{
    if ((NULL_PTR == context) || (NULL_PTR == context->coReg)) {
        return 0U;
    }

    return *context->coReg;
}

/**
 * @brief Check if interrupt flag is set
 */
boolean WdgIf_S32K3_IsInterruptFlagSet(WdgIf_S32K3HwContextType *context)
{
    if ((NULL_PTR == context) || (NULL_PTR == context->irReg)) {
        return FALSE;
    }

    return ((*context->irReg & S32K3_SWT_IR_TIF) != 0U);
}

/**
 * @brief Clear interrupt flag
 */
void WdgIf_S32K3_ClearInterruptFlag(WdgIf_S32K3HwContextType *context)
{
    if ((NULL_PTR == context) || (NULL_PTR == context->irReg)) {
        return;
    }

    *context->irReg = S32K3_SWT_IR_TIF;
}

/******************************************************************************
 * External Watchdog Support
 ******************************************************************************/

#ifdef WDGIF_EXTERNAL_WDG_SUPPORTED

/**
 * @brief Initialize external watchdog
 */
Std_ReturnType WdgIf_S32K3_InitExternalWdg(
    WdgIf_DeviceType device,
    uint8 interfaceType)
{
    /* TODO: Implement SPI/I2C-based external watchdog initialization */
    (void)device;
    (void)interfaceType;
    return E_OK;
}

/**
 * @brief Trigger external watchdog
 */
Std_ReturnType WdgIf_S32K3_TriggerExternalWdg(WdgIf_DeviceType device)
{
    /* TODO: Implement SPI/I2C-based external watchdog trigger */
    (void)device;
    return E_OK;
}

/**
 * @brief Set external watchdog mode
 */
Std_ReturnType WdgIf_S32K3_SetExternalWdgMode(
    WdgIf_DeviceType device,
    WdgIf_ModeType mode)
{
    /* TODO: Implement SPI/I2C-based external watchdog mode setting */
    (void)device;
    (void)mode;
    return E_OK;
}

#endif /* WDGIF_EXTERNAL_WDG_SUPPORTED */

/******************************************************************************
 * Safety and Monitoring Functions
 ******************************************************************************/

/**
 * @brief Check if watchdog caused reset
 */
boolean WdgIf_S32K3_IsWatchdogReset(void)
{
    /* TODO: Check MC_RGM (Reset Generation Module) registers */
    return FALSE;
}
