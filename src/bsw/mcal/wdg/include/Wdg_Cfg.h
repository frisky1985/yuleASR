/**
 * @file Wdg_Cfg.h
 * @brief WDG Driver configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef WDG_CFG_H
#define WDG_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define WDG_DEV_ERROR_DETECT            (STD_ON)
#define WDG_VERSION_INFO_API            (STD_ON)
#define WDG_DISABLE_ALLOWED             (STD_OFF)

/*==================================================================================================
*                                    WATCHDOG MODE
==================================================================================================*/
#define WDG_INITIAL_MODE                (WDGIF_FAST_MODE)
#define WDG_DEFAULT_TIMEOUT             ((Wdg_TimeoutType)100U)  /* 100ms */

/*==================================================================================================
*                                    FAST MODE SETTINGS
==================================================================================================*/
#define WDG_FAST_MODE_TIMEOUT           ((Wdg_TimeoutType)50U)   /* 50ms */
#define WDG_FAST_MODE_PRESCALER         (WDG_PRESCALER_64)
#define WDG_FAST_MODE_WINDOW_ENABLED    (STD_OFF)
#define WDG_FAST_MODE_WINDOW_START      ((Wdg_TimeoutType)10U)
#define WDG_FAST_MODE_WINDOW_END        ((Wdg_TimeoutType)40U)
#define WDG_FAST_MODE_INTERRUPT         (STD_OFF)

/*==================================================================================================
*                                    SLOW MODE SETTINGS
==================================================================================================*/
#define WDG_SLOW_MODE_TIMEOUT           ((Wdg_TimeoutType)500U)  /* 500ms */
#define WDG_SLOW_MODE_PRESCALER         (WDG_PRESCALER_256)
#define WDG_SLOW_MODE_WINDOW_ENABLED    (STD_OFF)
#define WDG_SLOW_MODE_WINDOW_START      ((Wdg_TimeoutType)100U)
#define WDG_SLOW_MODE_WINDOW_END        ((Wdg_TimeoutType)400U)
#define WDG_SLOW_MODE_INTERRUPT         (STD_OFF)

/*==================================================================================================
*                                    HARDWARE BASE ADDRESS
==================================================================================================*/
#define WDG_BASE_ADDRESS                (0x30280000U)  /* i.MX8 WDOG base */

/*==================================================================================================
*                                    CLOCK CONFIGURATION
==================================================================================================*/
#define WDG_CLOCK_FREQUENCY_HZ          (32000U)  /* 32kHz low power oscillator */

/*==================================================================================================
*                                    TRIGGER CONDITION
==================================================================================================*/
#define WDG_MAX_TIMEOUT                 ((uint16)1000U)  /* 1000ms max */
#define WDG_MIN_TIMEOUT                 ((uint16)1U)     /* 1ms min */

#endif /* WDG_CFG_H */
