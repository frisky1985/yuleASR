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

/*==================================================================================================
 *                                      WATCHDOG HARDWARE ABSTRACTION
 *==================================================================================================
 * FILENAME: Wdg_Hw.c
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_WatchdogDriver.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Hardware abstraction layer implementation for Watchdog Driver
 *              Supports: Independent Watchdog (IWDG), Window Watchdog (WWDG)
 *              Platforms: STM32 (F4/F7/H7), NXP i.MX RT, NXP S32K, Generic (mock)
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Wdg_Hw.h"
#include "Det.h"

/* Version checks */
#if defined(WDG_HW_AR_RELEASE_MAJOR_VERSION) && (WDG_HW_AR_RELEASE_MAJOR_VERSION != 4u)
    #error "Wdg_Hw.c: Mismatch in AUTOSAR major version"
#endif

#if defined(WDG_HW_SW_MAJOR_VERSION) && (WDG_HW_SW_MAJOR_VERSION != 1u)
    #error "Wdg_Hw.c: Mismatch in software major version"
#endif

/*==================================================================================================
 *                                    PLATFORM SPECIFIC DEFINITIONS
 *==================================================================================================*/

/*==================================================================================================
 * STM32 PLATFORM DEFINITIONS
 *==================================================================================================*/
#if defined(STM32) || defined(STM32H7)

/* IWDG base address */
#define WDG_HW_IWDG_BASE                (0x40003000u)

/* WWDG base address */
#define WDG_HW_WWDG_BASE                (0x40002C00u)

/* IWDG registers */
#define WDG_HW_IWDG_KR                  (*(volatile uint32*)(WDG_HW_IWDG_BASE + 0x00u))
#define WDG_HW_IWDG_PR                  (*(volatile uint32*)(WDG_HW_IWDG_BASE + 0x04u))
#define WDG_HW_IWDG_RLR                 (*(volatile uint32*)(WDG_HW_IWDG_BASE + 0x08u))
#define WDG_HW_IWDG_SR                  (*(volatile uint32*)(WDG_HW_IWDG_BASE + 0x0Cu))
#define WDG_HW_IWDG_WINR                (*(volatile uint32*)(WDG_HW_IWDG_BASE + 0x10u))

/* WWDG registers */
#define WDG_HW_WWDG_CR                  (*(volatile uint32*)(WDG_HW_WWDG_BASE + 0x00u))
#define WDG_HW_WWDG_CFR                 (*(volatile uint32*)(WDG_HW_WWDG_BASE + 0x04u))
#define WDG_HW_WWDG_SR                  (*(volatile uint32*)(WDG_HW_WWDG_BASE + 0x08u))

/* IWDG key values */
#define WDG_HW_IWDG_KEY_RELOAD          (0x0000AAAAu)
#define WDG_HW_IWDG_KEY_ENABLE          (0x0000CCCCu)
#define WDG_HW_IWDG_KEY_ACCESS          (0x00005555u)

/* IWDG status flags */
#define WDG_HW_IWDG_SR_PVU              (0x00000001u)  /* Prescaler value update */
#define WDG_HW_IWDG_SR_RVU              (0x00000002u)  /* Reload value update */
#define WDG_HW_IWDG_SR_WVU              (0x00000004u)  /* Window value update */

/* IWDG prescaler values */
#define WDG_HW_IWDG_PR_DIV4             (0u)
#define WDG_HW_IWDG_PR_DIV8             (1u)
#define WDG_HW_IWDG_PR_DIV16            (2u)
#define WDG_HW_IWDG_PR_DIV32            (3u)
#define WDG_HW_IWDG_PR_DIV64            (4u)
#define WDG_HW_IWDG_PR_DIV128           (5u)
#define WDG_HW_IWDG_PR_DIV256           (6u)

/* WWDG control register bits */
#define WDG_HW_WWDG_CR_T                (0x0000007Fu)  /* 7-bit counter */
#define WDG_HW_WWDG_CR_WDGA             (0x00000080u)  /* Activation bit */

/* WWDG configuration register bits */
#define WDG_HW_WWDG_CFR_W               (0x0000007Fu)  /* 7-bit window value */
#define WDG_HW_WWDG_CFR_WDGTB0          (0x00000080u)  /* Timer base bit 0 */
#define WDG_HW_WWDG_CFR_WDGTB1          (0x00000100u)  /* Timer base bit 1 */
#define WDG_HW_WWDG_CFR_EWI             (0x00000200u)  /* Early wakeup interrupt */

/* WWDG timer base */
#define WDG_HW_WWDG_PRESCALER_DIV1      (0u)
#define WDG_HW_WWDG_PRESCALER_DIV2      (1u)
#define WDG_HW_WWDG_PRESCALER_DIV4      (2u)
#define WDG_HW_WWDG_PRESCALER_DIV8      (3u)

/* LSI clock frequency (typical) */
#define WDG_HW_LSI_FREQ_HZ              (32000u)

/*==================================================================================================
 * NXP i.MX RT PLATFORM DEFINITIONS (WDOG)
 *==================================================================================================*/
#elif defined(NXP_IMXRT)

/* WDOG base addresses */
#define WDG_HW_WDOG1_BASE               (0x30280000u)
#define WDG_HW_WDOG2_BASE               (0x30290000u)
#define WDG_HW_WDOG3_BASE               (0x302A0000u)

/* WDOG registers */
#define WDG_HW_WDOG_WCR(base)           (*(volatile uint16*)((base) + 0x00u))
#define WDG_HW_WDOG_WSR(base)           (*(volatile uint16*)((base) + 0x02u))
#define WDG_HW_WDOG_WRSR(base)          (*(volatile uint16*)((base) + 0x04u))
#define WDG_HW_WDOG_WICR(base)          (*(volatile uint16*)((base) + 0x06u))
#define WDG_HW_WDOG_WMCR(base)          (*(volatile uint16*)((base) + 0x08u))

/* WCR bits */
#define WDG_HW_WCR_WDE                  (0x0004u)  /* Watchdog enable */
#define WDG_HW_WCR_WDZST                (0x0008u)  /* Suspend in doze/DBG */
#define WDG_HW_WCR_WDBG                 (0x0010u)  /* Debug enable */
#define WDG_HW_WCR_WDT                  (0x0020u)  /* Assert WDOG_B */
#define WDG_HW_WCR_SRS                  (0x0040u)  /* Software reset signal */
#define WDG_HW_WCR_WDA                  (0x0080u)  /* WDOG_B assertion */
#define WDG_HW_WCR_WT_MASK              (0xFF00u)  /* Timeout value */

/* WSR sequence values */
#define WDG_HW_WSR_SEQ1                 (0x5555u)
#define WDG_HW_WSR_SEQ2                 (0xAAAAu)

/* WRSR bits */
#define WDG_HW_WRSR_SFTW                (0x0001u)  /* Software reset */
#define WDG_HW_WRSR_TOUT                (0x0002u)  /* Timeout reset */
#define WDG_HW_WRSR_POR                 (0x0010u)  /* Power-on reset */

/* WICR bits */
#define WDG_HW_WICR_WIE                 (0x0001u)  /* Interrupt enable */
#define WDG_HW_WICR_WTIS                (0x0002u)  /* Interrupt status */
#define WDG_HW_WICR_WICT_MASK           (0xFF00u)  /* Interrupt count */

/* WDOG clock frequency */
#define WDG_HW_WDOG_FREQ_HZ             (32000u)

/*==================================================================================================
 * NXP S32K PLATFORM DEFINITIONS
 *==================================================================================================*/
#elif defined(NXP_S32K)

/* WDOG base address */
#define WDG_HW_WDOG_BASE                (0x40052000u)

/* WDOG registers */
#define WDG_HW_WDOG_CS                  (*(volatile uint32*)(WDG_HW_WDOG_BASE + 0x00u))
#define WDG_HW_WDOG_CNT                 (*(volatile uint32*)(WDG_HW_WDOG_BASE + 0x04u))
#define WDG_HW_WDOG_TOVAL               (*(volatile uint32*)(WDG_HW_WDOG_BASE + 0x08u))
#define WDG_HW_WDOG_WIN                 (*(volatile uint32*)(WDG_HW_WDOG_BASE + 0x0Cu))

/* CS bits */
#define WDG_HW_WDOG_CS_STOP             (0x00000001u)
#define WDG_HW_WDOG_CS_WAIT             (0x00000002u)
#define WDG_HW_WDOG_CS_DBG              (0x00000004u)
#define WDG_HW_WDOG_CS_TST_MASK         (0x00000018u)
#define WDG_HW_WDOG_CS_UPDATE           (0x00000020u)
#define WDG_HW_WDOG_CS_INT              (0x00000040u)
#define WDG_HW_WDOG_CS_EN               (0x00000080u)
#define WDG_HW_WDOG_CS_CLK_MASK         (0x00000300u)
#define WDG_HW_WDOG_CS_PRES             (0x00000400u)
#define WDG_HW_WDOG_CS_ULK              (0x00000800u)
#define WDG_HW_WDOG_CS_RCS              (0x00001000u)

/* CNT unlock values */
#define WDG_HW_WDOG_UNLOCK_SEQ1         (0xA602u)
#define WDG_HW_WDOG_UNLOCK_SEQ2         (0xB480u)

/* Refresh values */
#define WDG_HW_WDOG_REFRESH_SEQ1        (0xA602u)
#define WDG_HW_WDOG_REFRESH_SEQ2        (0xB480u)

/*==================================================================================================
 * GENERIC/MOCK PLATFORM
 *==================================================================================================*/
#else

/* Mock registers for testing */
static volatile uint32 Wdg_Hw_Mock_IWDG_KR = 0u;
static volatile uint32 Wdg_Hw_Mock_IWDG_PR = 0u;
static volatile uint32 Wdg_Hw_Mock_IWDG_RLR = 0u;
static volatile uint32 Wdg_Hw_Mock_IWDG_SR = 0u;
static volatile uint32 Wdg_Hw_Mock_IWDG_WINR = 0u;

static volatile uint32 Wdg_Hw_Mock_WWDG_CR = 0u;
static volatile uint32 Wdg_Hw_Mock_WWDG_CFR = 0u;
static volatile uint32 Wdg_Hw_Mock_WWDG_SR = 0u;

#define WDG_HW_IWDG_KR                  (Wdg_Hw_Mock_IWDG_KR)
#define WDG_HW_IWDG_PR                  (Wdg_Hw_Mock_IWDG_PR)
#define WDG_HW_IWDG_RLR                 (Wdg_Hw_Mock_IWDG_RLR)
#define WDG_HW_IWDG_SR                  (Wdg_Hw_Mock_IWDG_SR)
#define WDG_HW_IWDG_WINR                (Wdg_Hw_Mock_IWDG_WINR)

#define WDG_HW_WWDG_CR                  (Wdg_Hw_Mock_WWDG_CR)
#define WDG_HW_WWDG_CFR                 (Wdg_Hw_Mock_WWDG_CFR)
#define WDG_HW_WWDG_SR                  (Wdg_Hw_Mock_WWDG_SR)

#define WDG_HW_IWDG_KEY_RELOAD          (0x0000AAAAu)
#define WDG_HW_IWDG_KEY_ENABLE          (0x0000CCCCu)
#define WDG_HW_IWDG_KEY_ACCESS          (0x00005555u)

#define WDG_HW_IWDG_SR_PVU              (0x00000001u)
#define WDG_HW_IWDG_SR_RVU              (0x00000002u)
#define WDG_HW_IWDG_SR_WVU              (0x00000004u)

#define WDG_HW_IWDG_PR_DIV4             (0u)
#define WDG_HW_IWDG_PR_DIV8             (1u)
#define WDG_HW_IWDG_PR_DIV16            (2u)
#define WDG_HW_IWDG_PR_DIV32            (3u)
#define WDG_HW_IWDG_PR_DIV64            (4u)
#define WDG_HW_IWDG_PR_DIV128           (5u)
#define WDG_HW_IWDG_PR_DIV256           (6u)

#define WDG_HW_WWDG_CR_T                (0x0000007Fu)
#define WDG_HW_WWDG_CR_WDGA             (0x00000080u)
#define WDG_HW_WWDG_CFR_W               (0x0000007Fu)
#define WDG_HW_WWDG_CFR_WDGTB0          (0x00000080u)
#define WDG_HW_WWDG_CFR_WDGTB1          (0x00000100u)
#define WDG_HW_WWDG_CFR_EWI             (0x00000200u)

#define WDG_HW_WWDG_PRESCALER_DIV1      (0u)
#define WDG_HW_WWDG_PRESCALER_DIV2      (1u)
#define WDG_HW_WWDG_PRESCALER_DIV4      (2u)
#define WDG_HW_WWDG_PRESCALER_DIV8      (3u)

#define WDG_HW_LSI_FREQ_HZ              (32000u)

#endif /* Platform selection */

/*==================================================================================================
 *                                    LOCAL DEFINES
 *==================================================================================================*/
#define WDG_HW_MAX_TIMEOUT_MS           (10000u)
#define WDG_HW_MIN_TIMEOUT_MS           (1u)
#define WDG_HW_MAX_RELOAD_VALUE         (0x0FFFu)  /* 12-bit reload */

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
#define WDG_HW_ENTER_CRITICAL()         /* OS integration: Disable interrupts */
#define WDG_HW_EXIT_CRITICAL()          /* OS integration: Enable interrupts */

/*==================================================================================================
 *                                    LOCAL VARIABLES
 *==================================================================================================*/
#define WDG_HW_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Wdg_Hw_Initialized = FALSE;
static Wdg_Hw_StatusType Wdg_Hw_Status = WDG_HW_STATUS_UNINIT;
static Wdg_Hw_ConfigType Wdg_Hw_Config = {0};
static Wdg_Hw_ResetReasonType Wdg_Hw_LastResetReason = WDG_HW_RESET_NONE;
static uint16 Wdg_Hw_CurrentTimeout = 0u;
static boolean Wdg_Hw_EarlyWarningEnabled = FALSE;

#define WDG_HW_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
#define WDG_HW_START_SEC_CODE
#include "MemMap.h"

static uint32 Wdg_Hw_CalculateReloadValue(uint32 TimeoutMs, uint8 Prescaler);
static uint8 Wdg_Hw_CalculatePrescaler(uint32 TimeoutMs, uint32* ReloadValue);
static void Wdg_Hw_SetStatus(Wdg_Hw_StatusType Status);

#if defined(STM32) || defined(GENERIC)
static Std_ReturnType Wdg_Hw_InitIWDG(const Wdg_Hw_IwdgConfigType* ConfigPtr);
static Std_ReturnType Wdg_Hw_InitWWDG(const Wdg_Hw_WwdgConfigType* ConfigPtr);
static Std_ReturnType Wdg_Hw_TriggerIWDG(void);
static Std_ReturnType Wdg_Hw_TriggerWWDG(void);
static uint32 Wdg_Hw_GetResetReasonSTM32(void);
#endif

#if defined(NXP_IMXRT)
static Std_ReturnType Wdg_Hw_InitWDOG(const Wdg_Hw_ConfigType* ConfigPtr);
static uint32 Wdg_Hw_GetBaseAddress(void);
static uint16 Wdg_Hw_CalculateTimeoutValue(uint16 TimeoutMs);
#endif

#if defined(NXP_S32K)
static Std_ReturnType Wdg_Hw_InitWDOG_S32K(const Wdg_Hw_ConfigType* ConfigPtr);
static void Wdg_Hw_UnlockS32K(void);
static void Wdg_Hw_RefreshS32K(void);
#endif

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Set watchdog hardware status
 */
static void Wdg_Hw_SetStatus(Wdg_Hw_StatusType Status)
{
    Wdg_Hw_Status = Status;
}

/**
 * @brief Calculate IWDG reload value
 */
static uint32 Wdg_Hw_CalculateReloadValue(uint32 TimeoutMs, uint8 Prescaler)
{
    uint32 lsiFreq = WDG_HW_LSI_FREQ_HZ;
    uint32 divider = 4u << Prescaler;  /* 4, 8, 16, 32, 64, 128, 256 */
    uint32 ticks = ((TimeoutMs * lsiFreq) / (divider * 1000u));

    /* Clamp to max value */
    if (ticks > WDG_HW_MAX_RELOAD_VALUE) {
        ticks = WDG_HW_MAX_RELOAD_VALUE;
    }

    return ticks;
}

/**
 * @brief Calculate prescaler for desired timeout
 */
static uint8 Wdg_Hw_CalculatePrescaler(uint32 TimeoutMs, uint32* ReloadValue)
{
    uint8 prescaler;
    uint32 reload;

    for (prescaler = WDG_HW_IWDG_PR_DIV4; prescaler <= WDG_HW_IWDG_PR_DIV256; prescaler++) {
        reload = Wdg_Hw_CalculateReloadValue(TimeoutMs, prescaler);
        if (reload <= WDG_HW_MAX_RELOAD_VALUE) {
            *ReloadValue = reload;
            return prescaler;
        }
    }

    /* Use maximum settings */
    *ReloadValue = WDG_HW_MAX_RELOAD_VALUE;
    return WDG_HW_IWDG_PR_DIV256;
}

#if defined(STM32) || defined(GENERIC)
/**
 * @brief Initialize IWDG
 */
static Std_ReturnType Wdg_Hw_InitIWDG(const Wdg_Hw_IwdgConfigType* ConfigPtr)
{
    uint32 reloadValue;
    uint8 prescaler;
    uint32 timeout;

    if (ConfigPtr == NULL_PTR) {
        return E_NOT_OK;
    }

    /* Calculate prescaler and reload value */
    timeout = ConfigPtr->windowEnd;
    if (timeout == 0u) {
        timeout = 1000u;  /* Default 1 second */
    }

    prescaler = Wdg_Hw_CalculatePrescaler(timeout, &reloadValue);

    /* Enable IWDG by writing 0xCCCC */
    WDG_HW_IWDG_KR = WDG_HW_IWDG_KEY_ENABLE;

    /* Enable register access */
    WDG_HW_IWDG_KR = WDG_HW_IWDG_KEY_ACCESS;

    /* Wait for PVU to be cleared */
    timeout = 100000u;
    while ((WDG_HW_IWDG_SR & WDG_HW_IWDG_SR_PVU) != 0u) {
        if (timeout-- == 0u) {
            return E_NOT_OK;
        }
    }

    /* Set prescaler */
    WDG_HW_IWDG_PR = prescaler;

    /* Wait for RVU to be cleared */
    timeout = 100000u;
    while ((WDG_HW_IWDG_SR & WDG_HW_IWDG_SR_RVU) != 0u) {
        if (timeout-- == 0u) {
            return E_NOT_OK;
        }
    }

    /* Set reload value */
    WDG_HW_IWDG_RLR = reloadValue;

    /* Wait for RVU to be cleared */
    timeout = 100000u;
    while ((WDG_HW_IWDG_SR & WDG_HW_IWDG_SR_RVU) != 0u) {
        if (timeout-- == 0u) {
            return E_NOT_OK;
        }
    }

    /* Configure window if enabled */
    if (ConfigPtr->windowModeEnabled) {
        uint32 windowValue = Wdg_Hw_CalculateReloadValue(ConfigPtr->windowStart, prescaler);

        /* Wait for WVU to be cleared */
        timeout = 100000u;
        while ((WDG_HW_IWDG_SR & WDG_HW_IWDG_SR_WVU) != 0u) {
            if (timeout-- == 0u) {
                return E_NOT_OK;
            }
        }

        WDG_HW_IWDG_WINR = windowValue;
    }

    /* Reload watchdog */
    WDG_HW_IWDG_KR = WDG_HW_IWDG_KEY_RELOAD;

    return E_OK;
}

/**
 * @brief Initialize WWDG
 */
static Std_ReturnType Wdg_Hw_InitWWDG(const Wdg_Hw_WwdgConfigType* ConfigPtr)
{
    uint32 counter;
    uint32 window;
    uint8 prescaler;

    if (ConfigPtr == NULL_PTR) {
        return E_NOT_OK;
    }

    /* Calculate prescaler */
    if (ConfigPtr->prescaler <= WDG_HW_WWDG_PRESCALER_DIV8) {
        prescaler = ConfigPtr->prescaler;
    } else {
        prescaler = WDG_HW_WWDG_PRESCALER_DIV8;
    }

    /* Calculate counter and window values (approximate) */
    counter = ConfigPtr->windowValue & WDG_HW_WWDG_CR_T;
    window = (ConfigPtr->windowValue >> 1) & WDG_HW_WWDG_CFR_W;

    /* Set prescaler and window */
    WDG_HW_WWDG_CFR = (prescaler << 7u) | window;

    /* Enable early warning interrupt if requested */
    if (ConfigPtr->useInterrupt) {
        WDG_HW_WWDG_CFR |= WDG_HW_WWDG_CFR_EWI;
    }

    /* Set counter and enable */
    WDG_HW_WWDG_CR = WDG_HW_WWDG_CR_WDGA | (counter & WDG_HW_WWDG_CR_T);

    return E_OK;
}

/**
 * @brief Trigger IWDG
 */
static Std_ReturnType Wdg_Hw_TriggerIWDG(void)
{
    WDG_HW_IWDG_KR = WDG_HW_IWDG_KEY_RELOAD;
    return E_OK;
}

/**
 * @brief Trigger WWDG
 */
static Std_ReturnType Wdg_Hw_TriggerWWDG(void)
{
    uint32 cr = WDG_HW_WWDG_CR;
    cr &= ~WDG_HW_WWDG_CR_T;
    cr |= (0x7Fu & WDG_HW_WWDG_CR_T);  /* Set to max value */
    WDG_HW_WWDG_CR = cr;
    return E_OK;
}

/**
 * @brief Get reset reason for STM32
 */
static uint32 Wdg_Hw_GetResetReasonSTM32(void)
{
    /* Read RCC_CSR register */
    volatile uint32* rcc_csr = (volatile uint32*)(0x40023800u + 0x74u);
    uint32 csr = *rcc_csr;

    if (csr & 0x40000000u) {  /* WWDGRSTF */
        return WDG_HW_RESET_WWDG;
    } else if (csr & 0x20000000u) {  /* IWDGRSTF */
        return WDG_HW_RESET_IWDG;
    } else if (csr & 0x10000000u) {  /* SFTRSTF */
        return WDG_HW_RESET_SOFTWARE;
    } else if (csr & 0x08000000u) {  /* PORRSTF */
        return WDG_HW_RESET_POWER_ON;
    }

    return WDG_HW_RESET_NONE;
}
#endif /* STM32 || GENERIC */

#if defined(NXP_IMXRT)
/**
 * @brief Get WDOG base address
 */
static uint32 Wdg_Hw_GetBaseAddress(void)
{
    return WDG_HW_WDOG1_BASE;
}

/**
 * @brief Calculate timeout value for WDOG
 */
static uint16 Wdg_Hw_CalculateTimeoutValue(uint16 TimeoutMs)
{
    /* WDOG timeout = (WCR[WT] + 1) * 2 / WDOG clock frequency */
    /* WCR[WT] = (timeoutMs * clockFreq / 2000) - 1 */
    uint32 wtValue = ((uint32)TimeoutMs * WDG_HW_WDOG_FREQ_HZ / 2000u) - 1u;
    if (wtValue > 0xFFu) {
        wtValue = 0xFFu;
    }
    return (uint16)wtValue;
}

/**
 * @brief Initialize WDOG for i.MX RT
 */
static Std_ReturnType Wdg_Hw_InitWDOG(const Wdg_Hw_ConfigType* ConfigPtr)
{
    uint32 baseAddr = Wdg_Hw_GetBaseAddress();
    uint16 wcrValue;
    uint16 timeoutValue;

    if (ConfigPtr == NULL_PTR) {
        return E_NOT_OK;
    }

    /* Disable watchdog during configuration */
    wcrValue = WDG_HW_WDOG_WCR(baseAddr);
    wcrValue &= ~WDG_HW_WCR_WDE;
    WDG_HW_WDOG_WCR(baseAddr) = wcrValue;

    /* Calculate timeout */
    timeoutValue = Wdg_Hw_CalculateTimeoutValue(ConfigPtr->config.iwdg.windowEnd);

    /* Configure timeout */
    wcrValue = WDG_HW_WDOG_WCR(baseAddr);
    wcrValue &= ~WDG_HW_WCR_WT_MASK;
    wcrValue |= ((uint16)timeoutValue << 8) & WDG_HW_WCR_WT_MASK;
    WDG_HW_WDOG_WCR(baseAddr) = wcrValue;

    /* Configure interrupt if enabled */
    if (ConfigPtr->config.iwdg.useInterrupt) {
        uint16 wicrValue = WDG_HW_WICR_WIE;
        wicrValue |= ((uint16)timeoutValue << 8) & WDG_HW_WICR_WICT_MASK;
        WDG_HW_WDOG_WICR(baseAddr) = wicrValue;
    }

    /* Enable watchdog */
    if (ConfigPtr->wdgType != WDG_HW_TYPE_NONE) {
        wcrValue = WDG_HW_WDOG_WCR(baseAddr);
        wcrValue |= WDG_HW_WCR_WDE;
        wcrValue |= WDG_HW_WCR_WDT;
        wcrValue |= WDG_HW_WCR_SRS;  /* Enable software reset */
        WDG_HW_WDOG_WCR(baseAddr) = wcrValue;
    }

    return E_OK;
}
#endif /* NXP_IMXRT */

#if defined(NXP_S32K)
/**
 * @brief Unlock WDOG for S32K
 */
static void Wdg_Hw_UnlockS32K(void)
{
    WDG_HW_WDOG_CNT = WDG_HW_WDOG_UNLOCK_SEQ1;
    WDG_HW_WDOG_CNT = WDG_HW_WDOG_UNLOCK_SEQ2;
}

/**
 * @brief Refresh WDOG for S32K
 */
static void Wdg_Hw_RefreshS32K(void)
{
    WDG_HW_WDOG_CNT = WDG_HW_WDOG_REFRESH_SEQ1;
    WDG_HW_WDOG_CNT = WDG_HW_WDOG_REFRESH_SEQ2;
}

/**
 * @brief Initialize WDOG for S32K
 */
static Std_ReturnType Wdg_Hw_InitWDOG_S32K(const Wdg_Hw_ConfigType* ConfigPtr)
{
    uint32 timeoutValue;
    uint32 csValue;

    if (ConfigPtr == NULL_PTR) {
        return E_NOT_OK;
    }

    /* Unlock for configuration */
    Wdg_Hw_UnlockS32K();

    /* Wait for unlock */
    while ((WDG_HW_WDOG_CS & WDG_HW_WDOG_CS_ULK) == 0u) {
        /* Wait */
    }

    /* Calculate timeout value */
    timeoutValue = (uint32)ConfigPtr->config.iwdg.windowEnd * 1000u;  /* Convert to ticks */
    if (timeoutValue > 0xFFFFFFFFu) {
        timeoutValue = 0xFFFFFFFFu;
    }

    /* Set timeout */
    WDG_HW_WDOG_TOVAL = timeoutValue;

    /* Configure and enable */
    csValue = WDG_HW_WDOG_CS;
    csValue |= WDG_HW_WDOG_CS_EN;  /* Enable */
    csValue |= WDG_HW_WDOG_CS_UPDATE;  /* Allow updates */

    if (ConfigPtr->config.iwdg.useInterrupt) {
        csValue |= WDG_HW_WDOG_CS_INT;
    }

    if (ConfigPtr->config.iwdg.windowModeEnabled) {
        WDG_HW_WDOG_WIN = ConfigPtr->config.iwdg.windowStart;
    }

    WDG_HW_WDOG_CS = csValue;

    /* Initial refresh */
    Wdg_Hw_RefreshS32K();

    return E_OK;
}
#endif /* NXP_S32K */

/*==================================================================================================
 *                                    API IMPLEMENTATIONS
 *==================================================================================================*/

/**
 * @brief Initialize watchdog hardware
 */
Std_ReturnType Wdg_Hw_Init(const Wdg_Hw_ConfigType* ConfigPtr)
{
    Std_ReturnType result = E_NOT_OK;

#if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (Wdg_Hw_Initialized) {
        (void)Det_ReportError(WDG_HW_MODULE_ID, WDG_HW_INSTANCE_ID, WDG_HW_SID_INIT,
                              WDG_HW_E_ALREADY_INITIALIZED);
        return E_NOT_OK;
    }

    if (ConfigPtr == NULL_PTR) {
        (void)Det_ReportError(WDG_HW_MODULE_ID, WDG_HW_INSTANCE_ID, WDG_HW_SID_INIT,
                              WDG_HW_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    WDG_HW_ENTER_CRITICAL();

    /* Store configuration */
    if (ConfigPtr != NULL_PTR) {
        Wdg_Hw_Config = *ConfigPtr;
    }

    /* Initialize based on watchdog type */
    switch (ConfigPtr->wdgType) {
#if defined(STM32) || defined(GENERIC)
        case WDG_HW_TYPE_IWDG:
            result = Wdg_Hw_InitIWDG(&ConfigPtr->config.iwdg);
            break;

        case WDG_HW_TYPE_WWDG:
            result = Wdg_Hw_InitWWDG(&ConfigPtr->config.wwdg);
            break;
#endif

#if defined(NXP_IMXRT)
        case WDG_HW_TYPE_EXTERNAL:  /* WDOG is considered external to core */
            result = Wdg_Hw_InitWDOG(ConfigPtr);
            break;
#endif

#if defined(NXP_S32K)
        case WDG_HW_TYPE_EXTERNAL:
            result = Wdg_Hw_InitWDOG_S32K(ConfigPtr);
            break;
#endif

        case WDG_HW_TYPE_NONE:
        default:
            result = E_OK;
            break;
    }

    if (result == E_OK) {
        Wdg_Hw_Initialized = TRUE;
        Wdg_Hw_SetStatus(WDG_HW_STATUS_RUNNING);

        /* Get reset reason */
#if defined(STM32) || defined(GENERIC)
        Wdg_Hw_LastResetReason = (Wdg_Hw_ResetReasonType)Wdg_Hw_GetResetReasonSTM32();
#elif defined(NXP_IMXRT)
        {
            uint32 baseAddr = Wdg_Hw_GetBaseAddress();
            uint16 wrsr = WDG_HW_WDOG_WRSR(baseAddr);
            if (wrsr & WDG_HW_WRSR_TOUT) {
                Wdg_Hw_LastResetReason = WDG_HW_RESET_EXTERNAL;
            } else if (wrsr & WDG_HW_WRSR_SFTW) {
                Wdg_Hw_LastResetReason = WDG_HW_RESET_SOFTWARE;
            } else if (wrsr & WDG_HW_WRSR_POR) {
                Wdg_Hw_LastResetReason = WDG_HW_RESET_POWER_ON;
            }
        }
#endif
    } else {
        Wdg_Hw_SetStatus(WDG_HW_STATUS_ERROR);
    }

    WDG_HW_EXIT_CRITICAL();

    return result;
}

/**
 * @brief Deinitialize watchdog hardware
 */
Std_ReturnType Wdg_Hw_DeInit(void)
{
#if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (!Wdg_Hw_Initialized) {
        (void)Det_ReportError(WDG_HW_MODULE_ID, WDG_HW_INSTANCE_ID, WDG_HW_SID_DEINIT,
                              WDG_HW_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    WDG_HW_ENTER_CRITICAL();

    /* Attempt to disable watchdog if allowed */
    if (Wdg_Hw_Config.disableAllowed) {
        (void)Wdg_Hw_Disable();
    }

    Wdg_Hw_Initialized = FALSE;
    Wdg_Hw_SetStatus(WDG_HW_STATUS_UNINIT);

    WDG_HW_EXIT_CRITICAL();

    return E_OK;
}

/**
 * @brief Set trigger condition (timeout)
 */
Std_ReturnType Wdg_Hw_SetTriggerCondition(uint16 Timeout)
{
    Std_ReturnType result = E_NOT_OK;

#if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (!Wdg_Hw_Initialized) {
        (void)Det_ReportError(WDG_HW_MODULE_ID, WDG_HW_INSTANCE_ID, WDG_HW_SID_SETTRIGGERCONDITION,
                              WDG_HW_E_UNINIT);
        return E_NOT_OK;
    }

    if ((Timeout < WDG_HW_MIN_TIMEOUT_MS) || (Timeout > WDG_HW_MAX_TIMEOUT_MS)) {
        (void)Det_ReportError(WDG_HW_MODULE_ID, WDG_HW_INSTANCE_ID, WDG_HW_SID_SETTRIGGERCONDITION,
                              WDG_HW_E_PARAM_TIMEOUT);
        return E_NOT_OK;
    }
#endif

    WDG_HW_ENTER_CRITICAL();

    Wdg_Hw_CurrentTimeout = Timeout;

#if defined(STM32) || defined(GENERIC)
    if (Wdg_Hw_Config.wdgType == WDG_HW_TYPE_IWDG) {
        uint32 reloadValue;
        uint8 prescaler;
        uint32 timeout;

        /* Calculate new reload value */
        prescaler = Wdg_Hw_CalculatePrescaler((uint32)Timeout, &reloadValue);

        /* Enable register access */
        WDG_HW_IWDG_KR = WDG_HW_IWDG_KEY_ACCESS;

        /* Wait for PVU to be cleared */
        timeout = 100000u;
        while ((WDG_HW_IWDG_SR & WDG_HW_IWDG_SR_PVU) != 0u) {
            if (timeout-- == 0u) {
                WDG_HW_EXIT_CRITICAL();
                return E_NOT_OK;
            }
        }

        /* Set prescaler */
        WDG_HW_IWDG_PR = prescaler;

        /* Wait for RVU to be cleared */
        timeout = 100000u;
        while ((WDG_HW_IWDG_SR & WDG_HW_IWDG_SR_RVU) != 0u) {
            if (timeout-- == 0u) {
                WDG_HW_EXIT_CRITICAL();
                return E_NOT_OK;
            }
        }

        /* Set reload value */
        WDG_HW_IWDG_RLR = reloadValue;

        /* Trigger to apply */
        WDG_HW_IWDG_KR = WDG_HW_IWDG_KEY_RELOAD;

        result = E_OK;
    } else {
        /* WWDG doesn't support timeout change after initialization on STM32 */
        result = E_NOT_OK;
    }
#elif defined(NXP_IMXRT)
    {
        uint32 baseAddr = Wdg_Hw_GetBaseAddress();
        uint16 wtValue = Wdg_Hw_CalculateTimeoutValue(Timeout);
        uint16 wcrValue = WDG_HW_WDOG_WCR(baseAddr);

        wcrValue &= ~WDG_HW_WCR_WT_MASK;
        wcrValue |= ((uint16)wtValue << 8) & WDG_HW_WCR_WT_MASK;
        WDG_HW_WDOG_WCR(baseAddr) = wcrValue;

        result = E_OK;
    }
#elif defined(NXP_S32K)
    {
        /* Unlock for update */
        Wdg_Hw_UnlockS32K();

        /* Update timeout */
        WDG_HW_WDOG_TOVAL = (uint32)Timeout * 1000u;

        result = E_OK;
    }
#else
    result = E_OK;
#endif

    WDG_HW_EXIT_CRITICAL();

    return result;
}

/**
 * @brief Trigger (refresh) the watchdog
 */
Std_ReturnType Wdg_Hw_Trigger(void)
{
    Std_ReturnType result = E_NOT_OK;

#if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (!Wdg_Hw_Initialized) {
        (void)Det_ReportError(WDG_HW_MODULE_ID, WDG_HW_INSTANCE_ID, WDG_HW_SID_TRIGGER,
                              WDG_HW_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    WDG_HW_ENTER_CRITICAL();

    if (Wdg_Hw_Status == WDG_HW_STATUS_RUNNING) {
        switch (Wdg_Hw_Config.wdgType) {
#if defined(STM32) || defined(GENERIC)
            case WDG_HW_TYPE_IWDG:
                result = Wdg_Hw_TriggerIWDG();
                break;

            case WDG_HW_TYPE_WWDG:
                result = Wdg_Hw_TriggerWWDG();
                break;
#endif

#if defined(NXP_IMXRT)
            case WDG_HW_TYPE_EXTERNAL:
                {
                    uint32 baseAddr = Wdg_Hw_GetBaseAddress();
                    WDG_HW_WDOG_WSR(baseAddr) = WDG_HW_WSR_SEQ1;
                    WDG_HW_WDOG_WSR(baseAddr) = WDG_HW_WSR_SEQ2;
                    result = E_OK;
                }
                break;
#endif

#if defined(NXP_S32K)
            case WDG_HW_TYPE_EXTERNAL:
                Wdg_Hw_RefreshS32K();
                result = E_OK;
                break;
#endif

            default:
                result = E_NOT_OK;
                break;
        }
    }

    WDG_HW_EXIT_CRITICAL();

    return result;
}

/**
 * @brief Disable the watchdog
 */
Std_ReturnType Wdg_Hw_Disable(void)
{
#if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (!Wdg_Hw_Initialized) {
        (void)Det_ReportError(WDG_HW_MODULE_ID, WDG_HW_INSTANCE_ID, WDG_HW_SID_DISABLE,
                              WDG_HW_E_UNINIT);
        return E_NOT_OK;
    }

    if (!Wdg_Hw_Config.disableAllowed) {
        (void)Det_ReportError(WDG_HW_MODULE_ID, WDG_HW_INSTANCE_ID, WDG_HW_SID_DISABLE,
                              WDG_HW_E_DISABLE_NOT_ALLOWED);
        return E_NOT_OK;
    }
#endif

    WDG_HW_ENTER_CRITICAL();

    /* IWDG cannot be disabled once enabled on STM32 */
#if defined(STM32) || defined(GENERIC)
    if (Wdg_Hw_Config.wdgType == WDG_HW_TYPE_WWDG) {
        /* Disable WWDG */
        WDG_HW_WWDG_CR = 0u;
        Wdg_Hw_SetStatus(WDG_HW_STATUS_STOPPED);
    }
#elif defined(NXP_IMXRT)
    {
        uint32 baseAddr = Wdg_Hw_GetBaseAddress();
        uint16 wcrValue = WDG_HW_WDOG_WCR(baseAddr);
        wcrValue &= ~WDG_HW_WCR_WDE;
        WDG_HW_WDOG_WCR(baseAddr) = wcrValue;
        Wdg_Hw_SetStatus(WDG_HW_STATUS_STOPPED);
    }
#elif defined(NXP_S32K)
    {
        /* Disable WDOG */
        Wdg_Hw_UnlockS32K();
        WDG_HW_WDOG_CS &= ~WDG_HW_WDOG_CS_EN;
        Wdg_Hw_SetStatus(WDG_HW_STATUS_STOPPED);
    }
#endif

    WDG_HW_EXIT_CRITICAL();

    return E_OK;
}

/**
 * @brief Get watchdog status
 */
Wdg_Hw_StatusType Wdg_Hw_GetStatus(void)
{
    return Wdg_Hw_Status;
}

/**
 * @brief Set window mode
 */
Std_ReturnType Wdg_Hw_SetWindow(uint32 StartValue, uint32 EndValue)
{
    (void)StartValue;
    (void)EndValue;

#if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (!Wdg_Hw_Initialized) {
        (void)Det_ReportError(WDG_HW_MODULE_ID, WDG_HW_INSTANCE_ID, WDG_HW_SID_SETWINDOW,
                              WDG_HW_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    /* Window mode typically needs to be set during initialization */
    /* Dynamic window change may not be supported on all platforms */

    return E_NOT_OK;
}

/**
 * @brief Watchdog interrupt handler
 */
void Wdg_Hw_IRQHandler(void)
{
    if (!Wdg_Hw_Initialized) {
        return;
    }

#if defined(STM32) || defined(GENERIC)
    if (Wdg_Hw_Config.wdgType == WDG_HW_TYPE_WWDG) {
        /* Check early wakeup interrupt */
        if ((WDG_HW_WWDG_SR & 0x01u) != 0u) {
            /* Clear interrupt flag */
            WDG_HW_WWDG_SR = 0x00u;

            /* Application callback would be called here */
            /* Wdg_Hw_EarlyWarningCallback(); */
        }
    }
#elif defined(NXP_IMXRT)
    {
        uint32 baseAddr = Wdg_Hw_GetBaseAddress();
        uint16 wicr = WDG_HW_WDOG_WICR(baseAddr);

        if (wicr & WDG_HW_WICR_WTIS) {
            /* Clear interrupt status */
            WDG_HW_WDOG_WICR(baseAddr) = wicr | WDG_HW_WICR_WTIS;

            /* Trigger watchdog to prevent reset */
            Wdg_Hw_Trigger();
        }
    }
#endif
}

/**
 * @brief Get reset reason
 */
Wdg_Hw_ResetReasonType Wdg_Hw_GetResetReason(void)
{
    return Wdg_Hw_LastResetReason;
}

/**
 * @brief Check if watchdog is enabled
 */
boolean Wdg_Hw_IsEnabled(void)
{
    if (!Wdg_Hw_Initialized) {
        return FALSE;
    }

#if defined(STM32) || defined(GENERIC)
    if (Wdg_Hw_Config.wdgType == WDG_HW_TYPE_IWDG) {
        /* IWDG is enabled by writing ENABLE key - cannot be read back */
        return (Wdg_Hw_Status == WDG_HW_STATUS_RUNNING);
    } else if (Wdg_Hw_Config.wdgType == WDG_HW_TYPE_WWDG) {
        return ((WDG_HW_WWDG_CR & WDG_HW_WWDG_CR_WDGA) != 0u);
    }
#elif defined(NXP_IMXRT)
    {
        uint32 baseAddr = Wdg_Hw_GetBaseAddress();
        return ((WDG_HW_WDOG_WCR(baseAddr) & WDG_HW_WCR_WDE) != 0u);
    }
#elif defined(NXP_S32K)
    return ((WDG_HW_WDOG_CS & WDG_HW_WDOG_CS_EN) != 0u);
#endif

    return FALSE;
}

/**
 * @brief Get current counter value
 */
uint32 Wdg_Hw_GetCounter(void)
{
    if (!Wdg_Hw_Initialized) {
        return 0u;
    }

#if defined(STM32) || defined(GENERIC)
    if (Wdg_Hw_Config.wdgType == WDG_HW_TYPE_WWDG) {
        return (WDG_HW_WWDG_CR & WDG_HW_WWDG_CR_T);
    } else if (Wdg_Hw_Config.wdgType == WDG_HW_TYPE_IWDG) {
        /* IWDG counter not directly readable */
        return 0u;
    }
#elif defined(NXP_IMXRT)
    /* Counter not directly accessible on WDOG */
    return 0u;
#elif defined(NXP_S32K)
    return WDG_HW_WDOG_CNT;
#endif

    return 0u;
}

/**
 * @brief Set early warning interrupt
 */
Std_ReturnType Wdg_Hw_SetEarlyWarningInterrupt(uint32 Threshold)
{
    (void)Threshold;

#if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (!Wdg_Hw_Initialized) {
        (void)Det_ReportError(WDG_HW_MODULE_ID, WDG_HW_INSTANCE_ID, WDG_HW_SID_SETTRIGGERCONDITION,
                              WDG_HW_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    WDG_HW_ENTER_CRITICAL();

#if defined(STM32) || defined(GENERIC)
    if (Wdg_Hw_Config.wdgType == WDG_HW_TYPE_WWDG) {
        /* Enable early wakeup interrupt */
        WDG_HW_WWDG_CFR |= WDG_HW_WWDG_CFR_EWI;
        Wdg_Hw_EarlyWarningEnabled = TRUE;
    }
#elif defined(NXP_IMXRT)
    {
        uint32 baseAddr = Wdg_Hw_GetBaseAddress();
        uint16 wicrValue = WDG_HW_WDOG_WICR(baseAddr);
        wicrValue |= WDG_HW_WICR_WIE;
        wicrValue |= ((uint16)Threshold << 8) & WDG_HW_WICR_WICT_MASK;
        WDG_HW_WDOG_WICR(baseAddr) = wicrValue;
        Wdg_Hw_EarlyWarningEnabled = TRUE;
    }
#endif

    WDG_HW_EXIT_CRITICAL();

    return E_OK;
}

/**
 * @brief Clear interrupt flag
 */
void Wdg_Hw_ClearInterruptFlag(void)
{
    if (!Wdg_Hw_Initialized) {
        return;
    }

#if defined(STM32) || defined(GENERIC)
    if (Wdg_Hw_Config.wdgType == WDG_HW_TYPE_WWDG) {
        WDG_HW_WWDG_SR = 0x00u;
    }
#elif defined(NXP_IMXRT)
    {
        uint32 baseAddr = Wdg_Hw_GetBaseAddress();
        uint16 wicr = WDG_HW_WDOG_WICR(baseAddr);
        WDG_HW_WDOG_WICR(baseAddr) = wicr | WDG_HW_WICR_WTIS;
    }
#endif
}

#define WDG_HW_STOP_SEC_CODE
#include "MemMap.h"
