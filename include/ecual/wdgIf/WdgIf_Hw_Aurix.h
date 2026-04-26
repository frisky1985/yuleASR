/******************************************************************************
 * @file    WdgIf_Hw_Aurix.h
 * @brief   Watchdog Interface Hardware Abstraction for Infineon Aurix
 *
 * Hardware-specific interface for Aurix TC3xx/TC4xx microcontrollers.
 * Provides low-level access to the internal watchdog (WWDG/SCU) and
 * external watchdog interfaces (if supported).
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef WDGIF_HW_AURIX_H
#define WDGIF_HW_AURIX_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ecual/wdgIf/WdgIf_Internal.h"

/******************************************************************************
 * Aurix Watchdog Hardware Constants
 ******************************************************************************/

/* Aurix SCU Watchdog Registers */
#define AURIX_WDG_BASE_ADDR             0xF0036000U
#define AURIX_WDG_CON0_OFFSET           0x00U
#define AURIX_WDG_CON1_OFFSET           0x04U
#define AURIX_WDG_SR_OFFSET             0x08U

/* Watchdog Control Register 0 (CON0) bits */
#define AURIX_WDG_CON0_ENDINIT          0x00000001U
#define AURIX_WDG_CON0_WDTLCK           0x00000002U
#define AURIX_WDG_CON0_WDTHPW0          0x0000000CU
#define AURIX_WDG_CON0_WDTHPW1          0xFFFFFFF0U
#define AURIX_WDG_CON0_REL              0xFFFF0000U

/* Watchdog Control Register 1 (CON1) bits */
#define AURIX_WDG_CON1_WDTPRE           0x00000004U
#define AURIX_WDG_CON1_WDTIR            0x00000008U
#define AURIX_WDG_CON1_WDTDR            0x00000010U
#define AURIX_WDG_CON1_WDTEN            0x00000001U

/* Watchdog Status Register (SR) bits */
#define AURIX_WDG_SR_WDTTIM             0x0000FFFFU
#define AURIX_WDG_SR_WDTTO              0x00010000U
#define AURIX_WDG_SR_WDTPR              0x00020000U
#define AURIX_WDG_SR_WDTDS              0x00040000U
#define AURIX_WDG_SR_WDTAE              0x00080000U

/* Password access sequence */
#define AURIX_WDG_PASSWORD_SEQ1         0x000000F0U
#define AURIX_WDG_PASSWORD_SEQ2         0x000000F1U

/* Timeout limits */
#define AURIX_WDG_MIN_TIMEOUT_MS        1U
#define AURIX_WDG_MAX_TIMEOUT_MS        10000U  /* 10 seconds max */
#define AURIX_WDG_DEFAULT_TIMEOUT_MS    100U

/* Clock configuration */
#define AURIX_WDG_FOSC                  20000000U  /* 20 MHz crystal */
#define AURIX_WDG_DIVIDER_MIN           64U
#define AURIX_WDG_DIVIDER_MAX           65536U

/******************************************************************************
 * Aurix Watchdog Hardware Types
 ******************************************************************************/

/* Hardware-specific configuration */
typedef struct {
    uint32 baseAddress;
    uint32 clockFrequency;
    uint16 prescaler;
    boolean useWindowMode;
    boolean enableResetOutput;
    boolean enableTimeCheck;
    uint8 wdtType;              /* 0=Safety WDT, 1=CPU WDT */
} WdgIf_AurixHwConfigType;

/* Hardware context */
typedef struct {
    volatile uint32 *con0Reg;
    volatile uint32 *con1Reg;
    volatile uint32 *srReg;
    uint32 currentPassword;
    uint32 reloadValue;
    boolean endinitState;
    uint32 cpuId;
} WdgIf_AurixHwContextType;

/* Aurix WDT types */
typedef enum {
    AURIX_WDT_SAFETY = 0,
    AURIX_WDT_CPU0,
    AURIX_WDT_CPU1,
    AURIX_WDT_CPU2,
    AURIX_WDT_CPU3,
    AURIX_WDT_CPU4,
    AURIX_WDT_CPU5,
    AURIX_WDT_MAX
} WdgIf_AurixWdtType;

/******************************************************************************
 * Aurix Hardware Interface Functions
 ******************************************************************************/

/**
 * @brief Initialize Aurix watchdog hardware
 * @param config Device configuration
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Aurix_Init(const WdgIf_DeviceConfigType *config);

/**
 * @brief Deinitialize Aurix watchdog hardware
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Aurix_DeInit(WdgIf_DeviceType device);

/**
 * @brief Set Aurix watchdog mode
 * @param device Device index
 * @param mode Watchdog mode
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Aurix_SetMode(WdgIf_DeviceType device, WdgIf_ModeType mode);

/**
 * @brief Trigger Aurix watchdog (service)
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Aurix_Trigger(WdgIf_DeviceType device);

/**
 * @brief Get Aurix watchdog hardware version
 * @param version Pointer to store version
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Aurix_GetVersion(uint16 *version);

/**
 * @brief Force reset via Aurix watchdog
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Aurix_Reset(WdgIf_DeviceType device);

/**
 * @brief Get Aurix watchdog status
 * @param device Device index
 * @return Driver status
 */
WdgIf_DriverStatusType WdgIf_Aurix_GetStatus(WdgIf_DeviceType device);

/******************************************************************************
 * Low-Level Aurix Hardware Access Functions
 ******************************************************************************/

/**
 * @brief Calculate reload value from timeout
 * @param timeoutMs Timeout in milliseconds
 * @param prescaler Clock prescaler
 * @return Reload value for watchdog
 */
uint32 WdgIf_Aurix_CalculateReload(uint32 timeoutMs, uint16 prescaler);

/**
 * @brief Unlock watchdog access (password sequence)
 * @param context Hardware context
 * @return E_OK if unlock successful
 */
Std_ReturnType WdgIf_Aurix_UnlockAccess(WdgIf_AurixHwContextType *context);

/**
 * @brief Lock watchdog access (set ENDINIT)
 * @param context Hardware context
 */
void WdgIf_Aurix_LockAccess(WdgIf_AurixHwContextType *context);

/**
 * @brief Modify CON0 register safely
 * @param context Hardware context
 * @param value New value for CON0
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Aurix_ModifyCon0(
    WdgIf_AurixHwContextType *context,
    uint32 value
);

/**
 * @brief Get current timer value
 * @param context Hardware context
 * @return Current timer value
 */
uint32 WdgIf_Aurix_GetTimerValue(WdgIf_AurixHwContextType *context);

/**
 * @brief Check if watchdog caused reset
 * @param device Device index
 * @return TRUE if watchdog reset occurred
 */
boolean WdgIf_Aurix_IsWatchdogReset(WdgIf_DeviceType device);

/******************************************************************************
 * External Watchdog Interface (QSPI/GPIO based)
 ******************************************************************************/

#ifdef WDGIF_EXTERNAL_WDG_SUPPORTED

/**
 * @brief Initialize external watchdog via QSPI
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Aurix_InitExternalWdg(WdgIf_DeviceType device);

/**
 * @brief Trigger external watchdog
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Aurix_TriggerExternalWdg(WdgIf_DeviceType device);

/**
 * @brief Set external watchdog mode
 * @param device Device index
 * @param mode Watchdog mode
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Aurix_SetExternalWdgMode(
    WdgIf_DeviceType device,
    WdgIf_ModeType mode
);

#endif /* WDGIF_EXTERNAL_WDG_SUPPORTED */

/******************************************************************************
 * Hardware Interface Structure
 ******************************************************************************/
extern const WdgIf_HwInterfaceType WdgIf_AurixHwInterface;

#ifdef __cplusplus
}
#endif

#endif /* WDGIF_HW_AURIX_H */
