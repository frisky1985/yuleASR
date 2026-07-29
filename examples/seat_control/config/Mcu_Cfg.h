/**
 * @file Mcu_Cfg.h
 * @brief MCU Driver configuration for S32K312 Seat Control Demo
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Clock configuration:
 *   - Core clock:   80 MHz
 *   - Bus clock:    40 MHz
 *   - Cycle time:   12.5 ns
 *   - Lockstep:     enabled (safety)
 */

#ifndef MCU_CFG_H
#define MCU_CFG_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 * Pre-compile Configuration
 *==================================================================================================*/
#define MCU_DEV_ERROR_DETECT            (STD_ON)
#define MCU_VERSION_INFO_API            (STD_ON)
#define MCU_LOCKSTEP_SUPPORT            (STD_ON)
#define MCU_PERFORMANCE_RESET_API       (STD_ON)

/*==================================================================================================
 * Clock Definitions
 *==================================================================================================*/
#define MCU_CORE_CLOCK_HZ               (80000000U)     /* 80 MHz core clock */
#define MCU_BUS_CLOCK_HZ                (40000000U)     /* 40 MHz bus clock */
#define MCU_CYCLE_TIME_NS               (12U)           /* ~12.5ns cycle */

#define MCU_PLL_REF_CLOCK_HZ            (8000000U)      /* 8 MHz external oscillator */
#define MCU_PLL_VCO_FREQ_HZ             (320000000U)    /* 320 MHz VCO */
#define MCU_PLL_REF_DIV                 (1U)            /* PLL reference divider */
#define MCU_PLL_MULT                    (40U)           /* PLL multiplication factor */
#define MCU_PLL_POST_DIV                (4U)            /* PLL post divider */

#define MCU_NUM_CLOCK_TREES             (3U)            /* Core, Bus, Slow */
#define MCU_NUM_RESET_REASONS           (8U)            /* Supported reset reasons */

/*==================================================================================================
 * Reset Reason
 *==================================================================================================*/
typedef enum {
    MCU_RESET_POR           = 0U,       /* Power-on reset */
    MCU_RESET_WATCHDOG      = 1U,       /* Watchdog reset */
    MCU_RESET_EXTERNAL      = 2U,       /* External reset pin */
    MCU_RESET_SOFTWARE      = 3U,       /* Software triggered reset */
    MCU_RESET_LOCKUP        = 4U,       /* Lockup reset */
    MCU_RESET_CLOCK_FAIL    = 5U,       /* Clock failure reset */
    MCU_RESET_DEBUG         = 6U,       /* Debug reset */
    MCU_RESET_OTHER         = 7U        /* Other */
} Mcu_ResetReasonType;

/*==================================================================================================
 * Lockstep Configuration
 *==================================================================================================*/
typedef enum {
    MCU_LOCKSTEP_DISABLED = 0,
    MCU_LOCKSTEP_ENABLED  = 1
} Mcu_LockstepConfigType;

/*==================================================================================================
 * MCU Configuration Type
 *==================================================================================================*/
typedef struct {
    uint32                  coreClockHz;        /* Core clock frequency */
    uint32                  busClockHz;         /* Bus clock frequency */
    uint32                  pllRefClockHz;      /* PLL reference clock */
    uint8                   pllRefDiv;
    uint8                   pllMult;
    uint8                   pllPostDiv;
    Mcu_LockstepConfigType  lockstepConfig;
    uint8                   wdtDisable;         /* WDT disable after reset */
} Mcu_ClockConfigType;

typedef struct {
    Mcu_ClockConfigType     clockConfig;
    Mcu_ResetReasonType     resetReason;
} Mcu_ConfigType;

/*==================================================================================================
 * External Configuration Reference
 *==================================================================================================*/
extern const Mcu_ConfigType Mcu_Config;

#endif /* MCU_CFG_H */
