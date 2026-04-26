/******************************************************************************
 * @file    WdgIf_Hw_S32K3.h
 * @brief   Watchdog Interface Hardware Abstraction for NXP S32K3
 *
 * Hardware-specific interface for NXP S32K3xx microcontrollers.
 * Provides low-level access to the SWT (Software Watchdog Timer) and
 * external watchdog interfaces.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef WDGIF_HW_S32K3_H
#define WDGIF_HW_S32K3_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ecual/wdgIf/WdgIf_Internal.h"

/******************************************************************************
 * S32K3 Watchdog Hardware Constants
 ******************************************************************************/

/* SWT (Software Watchdog Timer) base addresses */
#define S32K3_SWT0_BASE_ADDR            0x40270000U
#define S32K3_SWT1_BASE_ADDR            0x4046C000U
#define S32K3_SWT2_BASE_ADDR            0x40470000U

/* SWT Register offsets */
#define S32K3_SWT_CR_OFFSET             0x00U  /* Control Register */
#define S32K3_SWT_IR_OFFSET             0x04U  /* Interrupt Register */
#define S32K3_SWT_TO_OFFSET             0x08U  /* Timeout Register */
#define S32K3_SWT_WN_OFFSET             0x0CU  /* Window Register */
#define S32K3_SWT_SR_OFFSET             0x10U  /* Service Register */
#define S32K3_SWT_CO_OFFSET             0x14U  /* Counter Output Register */
#define S32K3_SWT_SK_OFFSET             0x18U  /* Service Key Register */

/* SWT Control Register (CR) bits */
#define S32K3_SWT_CR_WEN                0x00000001U  /* Watchdog Enable */
#define S32K3_SWT_CR_FRZ                0x00000002U  /* Freeze in Debug */
#define S32K3_SWT_CR_STP                0x00000004U  /* Stop mode control */
#define S32K3_SWT_CR_SLK                0x00000010U  /* Soft Lock */
#define S32K3_SWT_CR_HLK                0x00000020U  /* Hard Lock */
#define S32K3_SWT_CR_ITR                0x00000400U  /* Interrupt then Reset */
#define S32K3_SWT_CR_WND                0x00000800U  /* Window mode */
#define S32K3_SWT_CR_RIA                0x00001000U  /* Reset on Invalid Access */
#define S32K3_SWT_CR_SMD_MASK           0x00006000U  /* Service Mode */
#define S32K3_SWT_CR_SMD_SHIFT          13U
#define S32K3_SWT_CR_MAP_MASK           0xFF000000U  /* Master Access Protection */
#define S32K3_SWT_CR_MAP_SHIFT          24U

/* Service modes */
#define S32K3_SWT_SMD_FIXED_KEY         0U
#define S32K3_SWT_SMD_INCREMENT_KEY     1U
#define S32K3_SWT_SMD_INDEXED_KEY       2U

/* SWT Service Register (SR) values */
#define S32K3_SWT_SR_SERVICE_KEY1       0x0000A602U
#define S32K3_SWT_SR_SERVICE_KEY2       0x0000B480U

/* SWT Interrupt Register bits */
#define S32K3_SWT_IR_TIF                0x00000001U  /* Time-out Interrupt Flag */

/* Timeout limits */
#define S32K3_WDG_MIN_TIMEOUT_MS        1U
#define S32K3_WDG_MAX_TIMEOUT_MS        20000U  /* 20 seconds max */
#define S32K3_WDG_DEFAULT_TIMEOUT_MS    100U

/* Clock configuration */
#define S32K3_WDG_FIRC_FREQ             48000000U  /* 48 MHz FIRC */
#define S32K3_WDG_SIRC_FREQ             32000U     /* 32 kHz SIRC */
#define S32K3_WDG_MAX_DIVIDER           65536U

/******************************************************************************
 * S32K3 Watchdog Hardware Types
 ******************************************************************************/

/* Hardware-specific configuration */
typedef struct {
    uint32 swtBaseAddress;
    uint32 clockSource;         /* 0=FIRC, 1=SIRC, 2=FXOSC */
    uint32 clockFrequency;
    uint16 clockDivider;
    boolean useWindowMode;
    uint8 serviceMode;
    boolean enableFreeze;
    boolean enableStop;
    boolean interruptThenReset;
    uint32 masterAccessMask;
} WdgIf_S32K3HwConfigType;

/* Hardware context */
typedef struct {
    volatile uint32 *crReg;
    volatile uint32 *irReg;
    volatile uint32 *toReg;
    volatile uint32 *wnReg;
    volatile uint32 *srReg;
    volatile uint32 *coReg;
    volatile uint32 *skReg;
    uint32 timeoutValue;
    uint32 windowValue;
    uint32 serviceKey;
    uint8 swtInstance;
    boolean isLocked;
} WdgIf_S32K3HwContextType;

/* SWT instances */
typedef enum {
    S32K3_SWT_0 = 0,
    S32K3_SWT_1,
    S32K3_SWT_2,
    S32K3_SWT_MAX
} WdgIf_S32K3SwtType;

/******************************************************************************
 * S32K3 Hardware Interface Functions
 ******************************************************************************/

/**
 * @brief Initialize S32K3 watchdog hardware
 * @param config Device configuration
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_S32K3_Init(const WdgIf_DeviceConfigType *config);

/**
 * @brief Deinitialize S32K3 watchdog hardware
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_S32K3_DeInit(WdgIf_DeviceType device);

/**
 * @brief Set S32K3 watchdog mode
 * @param device Device index
 * @param mode Watchdog mode
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_S32K3_SetMode(WdgIf_DeviceType device, WdgIf_ModeType mode);

/**
 * @brief Trigger S32K3 watchdog (service)
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_S32K3_Trigger(WdgIf_DeviceType device);

/**
 * @brief Get S32K3 watchdog hardware version
 * @param version Pointer to store version
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_S32K3_GetVersion(uint16 *version);

/**
 * @brief Force reset via S32K3 watchdog
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_S32K3_Reset(WdgIf_DeviceType device);

/**
 * @brief Get S32K3 watchdog status
 * @param device Device index
 * @return Driver status
 */
WdgIf_DriverStatusType WdgIf_S32K3_GetStatus(WdgIf_DeviceType device);

/******************************************************************************
 * Low-Level S32K3 Hardware Access Functions
 ******************************************************************************/

/**
 * @brief Calculate timeout register value
 * @param timeoutMs Timeout in milliseconds
 * @param clockFreq Clock frequency in Hz
 * @param divider Clock divider
 * @return Timeout register value
 */
uint32 WdgIf_S32K3_CalculateTimeout(
    uint32 timeoutMs,
    uint32 clockFreq,
    uint16 divider
);

/**
 * @brief Unlock SWT registers (soft unlock)
 * @param context Hardware context
 * @return E_OK if unlock successful
 */
Std_ReturnType WdgIf_S32K3_Unlock(WdgIf_S32K3HwContextType *context);

/**
 * @brief Lock SWT registers
 * @param context Hardware context
 */
void WdgIf_S32K3_Lock(WdgIf_S32K3HwContextType *context);

/**
 * @brief Perform service sequence (kick watchdog)
 * @param context Hardware context
 * @return E_OK if service successful
 */
Std_ReturnType WdgIf_S32K3_Service(WdgIf_S32K3HwContextType *context);

/**
 * @brief Get current counter value
 * @param context Hardware context
 * @return Current counter value
 */
uint32 WdgIf_S32K3_GetCounter(WdgIf_S32K3HwContextType *context);

/**
 * @brief Check if interrupt flag is set
 * @param context Hardware context
 * @return TRUE if timeout interrupt occurred
 */
boolean WdgIf_S32K3_IsInterruptFlagSet(WdgIf_S32K3HwContextType *context);

/**
 * @brief Clear interrupt flag
 * @param context Hardware context
 */
void WdgIf_S32K3_ClearInterruptFlag(WdgIf_S32K3HwContextType *context);

/**
 * @brief Check if watchdog caused reset
 * @return TRUE if watchdog reset occurred
 */
boolean WdgIf_S32K3_IsWatchdogReset(void);

/******************************************************************************
 * External Watchdog Interface (LPSPI/LPI2C based)
 ******************************************************************************/

#ifdef WDGIF_EXTERNAL_WDG_SUPPORTED

/**
 * @brief Initialize external watchdog
 * @param device Device index
 * @param interfaceType 0=SPI, 1=I2C
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_S32K3_InitExternalWdg(
    WdgIf_DeviceType device,
    uint8 interfaceType
);

/**
 * @brief Trigger external watchdog
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_S32K3_TriggerExternalWdg(WdgIf_DeviceType device);

/**
 * @brief Set external watchdog mode
 * @param device Device index
 * @param mode Watchdog mode
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_S32K3_SetExternalWdgMode(
    WdgIf_DeviceType device,
    WdgIf_ModeType mode
);

#endif /* WDGIF_EXTERNAL_WDG_SUPPORTED */

/******************************************************************************
 * Hardware Interface Structure
 ******************************************************************************/
extern const WdgIf_HwInterfaceType WdgIf_S32K3HwInterface;

#ifdef __cplusplus
}
#endif

#endif /* WDGIF_HW_S32K3_H */
