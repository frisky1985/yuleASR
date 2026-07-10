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

/**
 * @file EcuM_Cfg.h
 * @brief ECU State Manager Configuration - Multi-Phase Startup Support
 * @version 2.0.0
 */

#ifndef ECUM_CFG_H
#define ECUM_CFG_H
/*******************************************************************************
 *                             General Configuration                           *
 ******************************************************************************/

/* Development Error Detection */
#define ECUM_DEV_ERROR_DETECT               STD_ON

/* Version Info API */
#define ECUM_VERSION_INFO_API               STD_ON

/* Main Function Period in milliseconds */
#define ECUM_MAIN_FUNCTION_PERIOD           10u

/* Number of Users */
#define ECUM_MAX_USERS                      32u

/* Maximum number of wakeup sources */
#define ECUM_MAX_WAKEUP_SOURCES             32u

/* Multi-core support */
#define ECUM_MULTI_CORE_SUPPORT             STD_OFF

/* Fast startup mode */
#define ECUM_FAST_STARTUP_MODE              STD_OFF

/*******************************************************************************
 *                          Wakeup Source Configuration                        *
 ******************************************************************************/

/* Wakeup Sources Bitmask */
#ifndef ECUM_WKSOURCE_POWER
#define ECUM_WKSOURCE_POWER                 0x00000001u
#endif
#ifndef ECUM_WKSOURCE_RESET
#define ECUM_WKSOURCE_RESET                 0x00000002u
#endif
#ifndef ECUM_WKSOURCE_INTERNAL_RESET
#define ECUM_WKSOURCE_INTERNAL_RESET        0x00000004u
#endif
#ifndef ECUM_WKSOURCE_INTERNAL_WDG
#define ECUM_WKSOURCE_INTERNAL_WDG          0x00000008u
#endif
#ifndef ECUM_WKSOURCE_EXTERNAL_WDG
#define ECUM_WKSOURCE_EXTERNAL_WDG          0x00000010u
#endif
#ifndef ECUM_WKSOURCE_TIMER
#define ECUM_WKSOURCE_TIMER                 0x00000020u
#endif
#ifndef ECUM_WKSOURCE_CAN
#define ECUM_WKSOURCE_CAN                   0x00000040u
#endif
#ifndef ECUM_WKSOURCE_CAN0
#define ECUM_WKSOURCE_CAN0                  0x00000040u
#endif
#ifndef ECUM_WKSOURCE_CAN1
#define ECUM_WKSOURCE_CAN1                  0x00000080u
#endif
#ifndef ECUM_WKSOURCE_CAN2
#define ECUM_WKSOURCE_CAN2                  0x00000100u
#endif
#ifndef ECUM_WKSOURCE_CAN3
#define ECUM_WKSOURCE_CAN3                  0x00000200u
#endif
#ifndef ECUM_WKSOURCE_LIN
#define ECUM_WKSOURCE_LIN                   0x00000400u
#endif
#ifndef ECUM_WKSOURCE_ETH
#define ECUM_WKSOURCE_ETH                   0x00000800u
#endif
#ifndef ECUM_WKSOURCE_FLEXRAY
#define ECUM_WKSOURCE_FLEXRAY               0x00001000u
#endif
#ifndef ECUM_WKSOURCE_GPIO
#define ECUM_WKSOURCE_GPIO                  0x00002000u
#endif
#ifndef ECUM_WKSOURCE_SPI
#define ECUM_WKSOURCE_SPI                   0x00004000u
#endif
#ifndef ECUM_WKSOURCE_I2C
#define ECUM_WKSOURCE_I2C                   0x00008000u
#endif
#ifndef ECUM_WKSOURCE_ADC
#define ECUM_WKSOURCE_ADC                   0x00010000u
#endif

/* Configured Wakeup Sources (OR of enabled sources) */
#ifndef ECUM_CONFIGURED_WAKEUP_SOURCES
#define ECUM_CONFIGURED_WAKEUP_SOURCES      \
    (ECUM_WKSOURCE_POWER | ECUM_WKSOURCE_RESET | ECUM_WKSOURCE_TIMER | \
     ECUM_WKSOURCE_CAN | ECUM_WKSOURCE_CAN0 | ECUM_WKSOURCE_GPIO)
#endif

/*******************************************************************************
 *                          Wakeup Validation Configuration                    *
 ******************************************************************************/

/* Wakeup validation timeout in ms */
#define ECUM_WAKEUP_VALIDATION_TIMEOUT      100u

/* Check wakeup timeout in ms */
#define ECUM_CHECK_WAKEUP_TIMEOUT           50u

/* Enable Check Wakeup */
#define ECUM_CHECK_WAKEUP_ENABLED           STD_ON

/*******************************************************************************
 *                          Sleep Mode Configuration                           *
 ******************************************************************************/

/* Default Sleep Mode */
#define ECUM_DEFAULT_SLEEP_MODE             0u

/* Halt mode support */
#define ECUM_HALT_MODE_SUPPORTED            STD_ON

/* Poll mode support */
#define ECUM_POLL_MODE_SUPPORTED            STD_ON

/*******************************************************************************
 *                          Module Integration Configuration                   *
 ******************************************************************************/

/* NvM Integration */
#define ECUM_NVM_ENABLED                    STD_ON
#define ECUM_NVM_READALL_TIMEOUT            10000u  /* ms */
#define ECUM_NVM_WRITEALL_TIMEOUT           10000u  /* ms */

/* WdgM Integration */
#define ECUM_WDGM_ENABLED                   STD_ON

/* ComM Integration */
#define ECUM_COMM_ENABLED                   STD_ON

/* BswM Integration */
#define ECUM_BSWM_ENABLED                   STD_ON

/* SchM Integration */
#define ECUM_SCHM_ENABLED                   STD_ON

/* RTE Integration */
#define ECUM_RTE_ENABLED                    STD_ON

/*******************************************************************************
 *                          Startup Configuration                              *
 ******************************************************************************/

/* Startup Timeout */
#define ECUM_STARTUP_TIMEOUT                5000u  /* ms */

/* Enable Startup One */
#define ECUM_STARTUP_ONE_ENABLED            STD_ON

/* Enable Startup Two */
#define ECUM_STARTUP_TWO_ENABLED            STD_ON

/* Enable Startup Three (SWC init) */
#define ECUM_STARTUP_THREE_ENABLED          STD_ON

/*******************************************************************************
 *                          Reset Configuration                                *
 ******************************************************************************/

/* Reset Types */
#ifndef ECUM_RESET_TYPE_DEFINED
    #define ECUM_RESET_TYPE_DEFINED
    typedef uint8 EcuM_ResetType;
#endif
#define ECUM_RESET_MCU                      0x00u
#define ECUM_RESET_WDG                      0x01u
#define ECUM_RESET_IO                       0x02u
#define ECUM_RESET_SW                       0x03u

/* Default Reset Type */
#ifndef ECUM_DEFAULT_RESET_TYPE
#define ECUM_DEFAULT_RESET_TYPE             ECUM_RESET_MCU
#endif

/*******************************************************************************
 *                          Timing Configuration                               *
 ******************************************************************************/

/* Sleep transition timeout */
#define ECUM_SLEEP_TRANSITION_TIMEOUT       1000u  /* ms */

/* Wakeup restart timeout */
#define ECUM_WAKEUP_RESTART_TIMEOUT         5000u  /* ms */

/* Shutdown transition timeout */
#define ECUM_SHUTDOWN_TRANSITION_TIMEOUT    1000u  /* ms */

/*******************************************************************************
 *                          Alarm Clock Configuration                          *
 ******************************************************************************/

#define ECUM_ALARM_CLOCK_ENABLED            STD_OFF
#define ECUM_ALARM_CLOCK_TIME_BASE          1000u  /* ms */

/*******************************************************************************
 *                          Fixed Blocks Configuration                         *
 ******************************************************************************/

#define ECUM_FIXED_BLOCKS_ENABLED           STD_OFF

/*******************************************************************************
 *                          User Callback Configuration                        *
 ******************************************************************************/

/* Enable Pre-OS Callbacks */
#define ECUM_PREOS_CALLBACK_ENABLED         STD_ON

/* Enable Post-OS Callbacks */
#define ECUM_POSTOS_CALLBACK_ENABLED        STD_ON

/*******************************************************************************
 *                          Service IDs for Det                                *
 ******************************************************************************/

#ifndef ECUM_STARTUPONE_SID
#define ECUM_STARTUPONE_SID                 0x0Fu
#endif
#ifndef ECUM_STARTUPTWO_SID
#define ECUM_STARTUPTWO_SID                 0x10u
#endif
#ifndef ECUM_SLEEP_SID
#define ECUM_SLEEP_SID                      0x11u
#endif
#ifndef ECUM_HALT_SID
#define ECUM_HALT_SID                       0x12u
#endif
#ifndef ECUM_POLL_SID
#define ECUM_POLL_SID                       0x13u
#endif
#ifndef ECUM_WAKEUPRESTART_SID
#define ECUM_WAKEUPRESTART_SID              0x14u
#endif
#ifndef ECUM_CLEARWAKEUPEVENT_SID
#define ECUM_CLEARWAKEUPEVENT_SID           0x15u
#endif
#ifndef ECUM_CHECKWAKEUP_SID
#define ECUM_CHECKWAKEUP_SID                0x16u
#endif
#ifndef ECUM_ENABLEWAKEUPSOURCES_SID
#define ECUM_ENABLEWAKEUPSOURCES_SID        0x17u
#endif
#ifndef ECUM_DISABLEWAKEUPSOURCES_SID
#define ECUM_DISABLEWAKEUPSOURCES_SID       0x18u
#endif
#ifndef ECUM_GETSTATUSOFWAKEUPSOURCE_SID
#define ECUM_GETSTATUSOFWAKEUPSOURCE_SID    0x19u
#endif

#endif /* ECUM_CFG_H */
