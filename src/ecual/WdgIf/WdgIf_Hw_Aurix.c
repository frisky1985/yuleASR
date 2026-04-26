/******************************************************************************
 * @file    WdgIf_Hw_Aurix.c
 * @brief   Watchdog Interface Hardware Implementation for Infineon Aurix
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
#include "ecual/wdgIf/WdgIf_Hw_Aurix.h"
#include <string.h>

/******************************************************************************
 * Compile-time Checks
 ******************************************************************************/
#ifndef AURIX_WDG_BASE_ADDR
#define AURIX_WDG_BASE_ADDR 0xF0036000U
#endif

/******************************************************************************
 * Local Variables
 ******************************************************************************/
static WdgIf_AurixHwContextType WdgIf_AurixContexts[WDGIF_MAX_DEVICES];
static boolean WdgIf_AurixInitialized = FALSE;

/* Default Aurix hardware configuration */
static const WdgIf_AurixHwConfigType WdgIf_AurixDefaultConfig = {
    .baseAddress = AURIX_WDG_BASE_ADDR,
    .clockFrequency = AURIX_WDG_FOSC,
    .prescaler = 256U,
    .useWindowMode = FALSE,
    .enableResetOutput = TRUE,
    .enableTimeCheck = FALSE,
    .wdtType = 0U  /* Safety WDT */
};

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static Std_ReturnType WdgIf_Aurix_InitSafetyWdt(const WdgIf_DeviceConfigType *config);
static uint32 WdgIf_Aurix_CalculatePassword(WdgIf_AurixHwContextType *context);

/******************************************************************************
 * Hardware Interface Structure
 ******************************************************************************/
const WdgIf_HwInterfaceType WdgIf_AurixHwInterface = {
    .init = WdgIf_Aurix_Init,
    .deinit = WdgIf_Aurix_DeInit,
    .setMode = WdgIf_Aurix_SetMode,
    .trigger = WdgIf_Aurix_Trigger,
    .getVersion = WdgIf_Aurix_GetVersion,
    .reset = WdgIf_Aurix_Reset,
    .getStatus = WdgIf_Aurix_GetStatus
};

/******************************************************************************
 * API Implementation
 ******************************************************************************/

/**
 * @brief Initialize Aurix watchdog hardware
 */
Std_ReturnType WdgIf_Aurix_Init(const WdgIf_DeviceConfigType *config)
{
    Std_ReturnType result = E_OK;

    if (NULL_PTR == config) {
        return E_NOT_OK;
    }

    /* Initialize context for internal watchdog */
    if (WDGIF_INTERNAL_WDG == config->deviceId) {
        result = WdgIf_Aurix_InitSafetyWdt(config);
    }
#ifdef WDGIF_EXTERNAL_WDG_SUPPORTED
    else if ((WDGIF_EXTERNAL_WDG1 == config->deviceId) ||
             (WDGIF_EXTERNAL_WDG2 == config->deviceId)) {
        result = WdgIf_Aurix_InitExternalWdg(config->deviceId);
    }
#endif
    else {
        result = E_NOT_OK;
    }

    if (E_OK == result) {
        WdgIf_AurixInitialized = TRUE;
    }

    return result;
}

/**
 * @brief Initialize Safety Watchdog Timer
 */
static Std_ReturnType WdgIf_Aurix_InitSafetyWdt(const WdgIf_DeviceConfigType *config)
{
    WdgIf_AurixHwContextType *context = &WdgIf_AurixContexts[WDGIF_INTERNAL_WDG];
    const WdgIf_AurixHwConfigType *hwConfig;
    uint32 reloadValue;
    uint32 con0Value;
    uint32 con1Value;

    /* Get hardware configuration */
    if ((NULL_PTR != config->hardwareConfig) &&
        (((WdgIf_AurixHwConfigType*)config->hardwareConfig)->baseAddress != 0U)) {
        hwConfig = (const WdgIf_AurixHwConfigType*)config->hardwareConfig;
    } else {
        hwConfig = &WdgIf_AurixDefaultConfig;
    }

    /* Initialize context */
    (void)memset(context, 0, sizeof(WdgIf_AurixHwContextType));

    /* Setup register pointers for Safety WDT at SCU base */
    context->con0Reg = (volatile uint32 *)(hwConfig->baseAddress + AURIX_WDG_CON0_OFFSET);
    context->con1Reg = (volatile uint32 *)(hwConfig->baseAddress + AURIX_WDG_CON1_OFFSET);
    context->srReg = (volatile uint32 *)(hwConfig->baseAddress + AURIX_WDG_SR_OFFSET);
    context->cpuId = 0U;  /* Safety WDT is system-wide */

    /* Calculate reload value based on timeout configuration */
    if ((NULL_PTR != config->timeoutConfig) &&
        (config->timeoutConfig->slowModeTimeout > 0U)) {
        reloadValue = WdgIf_Aurix_CalculateReload(
            config->timeoutConfig->slowModeTimeout,
            hwConfig->prescaler
        );
    } else {
        reloadValue = WdgIf_Aurix_CalculateReload(
            AURIX_WDG_DEFAULT_TIMEOUT_MS,
            hwConfig->prescaler
        );
    }

    /* Limit reload value to valid range (16 bits) */
    if (reloadValue > 0xFFFFU) {
        reloadValue = 0xFFFFU;
    }
    context->reloadValue = reloadValue;

    /* Calculate password */
    context->currentPassword = WdgIf_Aurix_CalculatePassword(context);

    /* Disable ENDINIT protection temporarily */
    if (E_OK != WdgIf_Aurix_UnlockAccess(context)) {
        return E_NOT_OK;
    }

    /* Configure CON1: Enable watchdog, disable halt/stop sensitivity */
    con1Value = AURIX_WDG_CON1_WDTEN;  /* Enable WDT */

    if (hwConfig->enableResetOutput) {
        /* Keep default reset behavior */
    }

    *context->con1Reg = con1Value;

    /* Configure CON0 with reload value */
    con0Value = (reloadValue << 16U) | (context->currentPassword & 0x0000FFFFU);
    *context->con0Reg = con0Value;

    /* Re-enable ENDINIT protection */
    WdgIf_Aurix_LockAccess(context);

    return E_OK;
}

/**
 * @brief Deinitialize Aurix watchdog hardware
 */
Std_ReturnType WdgIf_Aurix_DeInit(WdgIf_DeviceType device)
{
    WdgIf_AurixHwContextType *context;

    if (device >= WDGIF_MAX_DEVICES) {
        return E_NOT_OK;
    }

    context = &WdgIf_AurixContexts[device];

    if ((NULL_PTR != context->con0Reg) && (NULL_PTR != context->con1Reg)) {
        /* Unlock access */
        if (E_OK != WdgIf_Aurix_UnlockAccess(context)) {
            return E_NOT_OK;
        }

        /* Disable watchdog by clearing WDTEN in CON1 */
        *context->con1Reg = 0U;

        /* Clear CON0 */
        *context->con0Reg = 0U;

        /* Lock access */
        WdgIf_Aurix_LockAccess(context);
    }

    /* Clear context */
    (void)memset(context, 0, sizeof(WdgIf_AurixHwContextType));

    return E_OK;
}

/**
 * @brief Set Aurix watchdog mode
 */
Std_ReturnType WdgIf_Aurix_SetMode(WdgIf_DeviceType device, WdgIf_ModeType mode)
{
    WdgIf_AurixHwContextType *context;
    uint32 reloadValue;
    uint32 con0Value;
    uint32 con1Value;
    uint16 timeoutMs = AURIX_WDG_DEFAULT_TIMEOUT_MS;

    if (device >= WDGIF_MAX_DEVICES) {
        return E_NOT_OK;
    }

    context = &WdgIf_AurixContexts[device];

    if ((NULL_PTR == context->con0Reg) || (NULL_PTR == context->con1Reg)) {
        return E_NOT_OK;
    }

    /* Determine timeout based on mode */
    switch (mode) {
        case WDGIF_OFF_MODE:
            /* Disable watchdog */
            if (E_OK != WdgIf_Aurix_UnlockAccess(context)) {
                return E_NOT_OK;
            }
            *context->con1Reg = 0U;  /* Clear WDTEN */
            WdgIf_Aurix_LockAccess(context);
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

    /* Calculate new reload value */
    reloadValue = WdgIf_Aurix_CalculateReload(timeoutMs, 256U);
    if (reloadValue > 0xFFFFU) {
        reloadValue = 0xFFFFU;
    }

    /* Unlock access */
    if (E_OK != WdgIf_Aurix_UnlockAccess(context)) {
        return E_NOT_OK;
    }

    /* Enable watchdog if not already enabled */
    con1Value = *context->con1Reg;
    if ((con1Value & AURIX_WDG_CON1_WDTEN) == 0U) {
        con1Value |= AURIX_WDG_CON1_WDTEN;
        *context->con1Reg = con1Value;
    }

    /* Update reload value in CON0 */
    con0Value = (reloadValue << 16U) | (context->currentPassword & 0x0000FFFFU);
    *context->con0Reg = con0Value;

    /* Lock access */
    WdgIf_Aurix_LockAccess(context);

    /* Update context */
    context->reloadValue = reloadValue;

    return E_OK;
}

/**
 * @brief Trigger Aurix watchdog
 */
Std_ReturnType WdgIf_Aurix_Trigger(WdgIf_DeviceType device)
{
    WdgIf_AurixHwContextType *context;
    uint32 con0Value;

    if (device >= WDGIF_MAX_DEVICES) {
        return E_NOT_OK;
    }

    context = &WdgIf_AurixContexts[device];

    if (NULL_PTR == context->con0Reg) {
        return E_NOT_OK;
    }

    /* Unlock access */
    if (E_OK != WdgIf_Aurix_UnlockAccess(context)) {
        return E_NOT_OK;
    }

    /* Calculate new password (Aurix requires password change on each access) */
    context->currentPassword = WdgIf_Aurix_CalculatePassword(context);

    /* Write CON0 with reload value and new password */
    con0Value = (context->reloadValue << 16U) |
                (context->currentPassword & 0x0000FFFFU);
    *context->con0Reg = con0Value;

    /* Lock access (this also reloads the timer) */
    WdgIf_Aurix_LockAccess(context);

    return E_OK;
}

/**
 * @brief Get Aurix watchdog hardware version
 */
Std_ReturnType WdgIf_Aurix_GetVersion(uint16 *version)
{
    if (NULL_PTR == version) {
        return E_NOT_OK;
    }

    /* Return a fixed version for Aurix SCU WDT */
    *version = 0x0100U;  /* Version 1.0 */

    return E_OK;
}

/**
 * @brief Force reset via Aurix watchdog
 */
Std_ReturnType WdgIf_Aurix_Reset(WdgIf_DeviceType device)
{
    WdgIf_AurixHwContextType *context;
    uint32 con0Value;

    if (device >= WDGIF_MAX_DEVICES) {
        return E_NOT_OK;
    }

    context = &WdgIf_AurixContexts[device];

    if (NULL_PTR == context->con0Reg) {
        return E_NOT_OK;
    }

    /* Unlock access */
    if (E_OK != WdgIf_Aurix_UnlockAccess(context)) {
        return E_NOT_OK;
    }

    /* Set reload value to minimum (immediate timeout) */
    con0Value = (1U << 16U) | (context->currentPassword & 0x0000FFFFU);
    *context->con0Reg = con0Value;

    /* Lock access - this will cause immediate watchdog timeout and reset */
    WdgIf_Aurix_LockAccess(context);

    /* Wait for reset (should not reach here) */
    while (1) {
        /* Infinite loop - reset should occur */
    }

    /* Should never reach here */
    return E_OK;
}

/**
 * @brief Get Aurix watchdog status
 */
WdgIf_DriverStatusType WdgIf_Aurix_GetStatus(WdgIf_DeviceType device)
{
    WdgIf_AurixHwContextType *context;
    uint32 srValue;

    if (device >= WDGIF_MAX_DEVICES) {
        return WDGIF_STATUS_ERROR;
    }

    if (!WdgIf_AurixInitialized) {
        return WDGIF_STATUS_UNINIT;
    }

    context = &WdgIf_AurixContexts[device];

    if (NULL_PTR == context->srReg) {
        return WDGIF_STATUS_ERROR;
    }

    /* Read status register */
    srValue = *context->srReg;

    /* Check for timeout flag */
    if ((srValue & AURIX_WDG_SR_WDTTO) != 0U) {
        return WDGIF_STATUS_ERROR;
    }

    return WDGIF_STATUS_IDLE;
}

/******************************************************************************
 * Low-Level Functions
 ******************************************************************************/

/**
 * @brief Calculate reload value from timeout
 */
uint32 WdgIf_Aurix_CalculateReload(uint32 timeoutMs, uint16 prescaler)
{
    uint32 ticks;
    uint32 clockFreq = AURIX_WDG_FOSC;

    /* Calculate ticks: timeout_ms * clock_freq / (prescaler * 1000) */
    ticks = (timeoutMs * clockFreq) / ((uint32)prescaler * 1000U);

    /* Aurix reload value is timer + 1, so subtract 1 */
    if (ticks > 0U) {
        ticks--;
    }

    /* Limit to 16-bit value */
    if (ticks > 0xFFFFU) {
        ticks = 0xFFFFU;
    }

    return ticks;
}

/**
 * @brief Calculate password for Aurix WDT access
 */
static uint32 WdgIf_Aurix_CalculatePassword(WdgIf_AurixHwContextType *context)
{
    uint32 password;
    uint32 srValue;

    if ((NULL_PTR == context) || (NULL_PTR == context->srReg)) {
        return 0U;
    }

    /* Read current timer value from status register */
    srValue = *context->srReg;

    /* Password is based on timer value and fixed pattern */
    password = ((srValue & 0x000000FFU) << 8U) | 0x000000F0U;

    return password;
}

/**
 * @brief Unlock watchdog access (Aurix password sequence)
 */
Std_ReturnType WdgIf_Aurix_UnlockAccess(WdgIf_AurixHwContextType *context)
{
    uint32 con0Value;

    if ((NULL_PTR == context) || (NULL_PTR == context->con0Reg)) {
        return E_NOT_OK;
    }

    /* Step 1: Write with ENDINIT=1, WDTHPW1=0xFF (inverted password high) */
    con0Value = AURIX_WDG_CON0_ENDINIT | AURIX_WDG_CON0_WDTLCK |
                0x0000FFF0U | ((~context->currentPassword) & 0x0000FF00U);
    *context->con0Reg = con0Value;

    /* Step 2: Write with ENDINIT=0, password */
    con0Value = (context->currentPassword & 0x0000FFFFU);
    *context->con0Reg = con0Value;

    /* Verify unlock */
    con0Value = *context->con0Reg;
    if ((con0Value & AURIX_WDG_CON0_ENDINIT) != 0U) {
        return E_NOT_OK;
    }

    context->endinitState = FALSE;

    return E_OK;
}

/**
 * @brief Lock watchdog access (set ENDINIT)
 */
void WdgIf_Aurix_LockAccess(WdgIf_AurixHwContextType *context)
{
    uint32 con0Value;

    if ((NULL_PTR == context) || (NULL_PTR == context->con0Reg)) {
        return;
    }

    /* Write CON0 with ENDINIT=1, current password, and reload value */
    con0Value = AURIX_WDG_CON0_ENDINIT | AURIX_WDG_CON0_WDTLCK |
                (context->reloadValue << 16U) |
                (context->currentPassword & 0x0000FFFFU);

    *context->con0Reg = con0Value;

    context->endinitState = TRUE;
}

/******************************************************************************
 * External Watchdog Support
 ******************************************************************************/

#ifdef WDGIF_EXTERNAL_WDG_SUPPORTED

/**
 * @brief Initialize external watchdog via QSPI
 */
Std_ReturnType WdgIf_Aurix_InitExternalWdg(WdgIf_DeviceType device)
{
    /* TODO: Implement QSPI-based external watchdog initialization */
    (void)device;
    return E_OK;
}

#endif /* WDGIF_EXTERNAL_WDG_SUPPORTED */

/******************************************************************************
 * Safety and Monitoring Functions
 ******************************************************************************/

/**
 * @brief Check if watchdog caused reset
 */
boolean WdgIf_Aurix_IsWatchdogReset(WdgIf_DeviceType device)
{
    /* TODO: Check SCU reset status registers */
    (void)device;
    return FALSE;
}
