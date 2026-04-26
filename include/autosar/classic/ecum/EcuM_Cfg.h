/******************************************************************************
 * @file    EcuM_Cfg.h
 * @brief   ECU State Manager (EcuM) Configuration
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ECUM_CFG_H
#define ECUM_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/autosar_types.h"
#include "ecum.h"

/******************************************************************************
 * EcuM Configuration Version
 ******************************************************************************/
#define ECUM_CFG_VENDOR_ID              0x01U
#define ECUM_CFG_MODULE_ID              0x0CU
#define ECUM_CFG_SW_MAJOR_VERSION       1U
#define ECUM_CFG_SW_MINOR_VERSION       0U
#define ECUM_CFG_SW_PATCH_VERSION       0U

/******************************************************************************
 * Driver Initialization Sequence Configuration
 ******************************************************************************/

/* Driver Init Zero - Pre-OS initialization (no OS services available) */
#define ECUM_DRIVER_ZERO_WDGM           STD_ON
#define ECUM_DRIVER_ZERO_MCAL           STD_ON

/* Driver Init One - Post-OS initialization (OS services available) */
#define ECUM_DRIVER_ONE_DET             STD_ON
#define ECUM_DRIVER_ONE_DEM             STD_ON
#define ECUM_DRIVER_ONE_NVM             STD_ON
#define ECUM_DRIVER_ONE_WDGIF           STD_ON

/* Driver Init Two - Basic BSW initialization */
#define ECUM_DRIVER_TWO_ETHIF           STD_ON
#define ECUM_DRIVER_TWO_ETHTRCV         STD_ON
#define ECUM_DRIVER_TWO_SOAD            STD_ON
#define ECUM_DRIVER_TWO_PDUR            STD_ON
#define ECUM_DRIVER_TWO_CANIF           STD_OFF
#define ECUM_DRIVER_TWO_CANTRCV         STD_OFF
#define ECUM_DRIVER_TWO_FRIF            STD_OFF
#define ECUM_DRIVER_TWO_FRTRCV          STD_OFF
#define ECUM_DRIVER_TWO_LINIF           STD_OFF
#define ECUM_DRIVER_TWO_LINTRCV         STD_OFF

/* Driver Init Three - Complex drivers and communication */
#define ECUM_DRIVER_THREE_DDS           STD_ON
#define ECUM_DRIVER_THREE_COM           STD_OFF
#define ECUM_DRIVER_THREE_COMM          STD_ON
#define ECUM_DRIVER_THREE_BSWM          STD_ON
#define ECUM_DRIVER_THREE_STBM          STD_OFF

/******************************************************************************
 * Maximum Driver Instances
 ******************************************************************************/
#define ECUM_MAX_ETH_IF_CONTROLLERS     2U
#define ECUM_MAX_ETH_TRCV_DRIVERS       2U
#define ECUM_MAX_SOAD_CONNECTIONS       16U
#define ECUM_MAX_PDUR_ROUTING_PATHS     32U

/******************************************************************************
 * Wakeup Source Configuration
 ******************************************************************************/

/* Number of configured wakeup sources */
#define ECUM_NUM_WAKEUP_SOURCES         8U

/* Wakeup Source 0: Power On */
#define ECUM_WKSOURCE_POWER_ID          0U
#define ECUM_WKSOURCE_POWER_TIMEOUT     100U    /* ms */
#define ECUM_WKSOURCE_POWER_COUNTER     1U

/* Wakeup Source 1: Reset */
#define ECUM_WKSOURCE_RESET_ID          1U
#define ECUM_WKSOURCE_RESET_TIMEOUT     0U      /* Immediate */
#define ECUM_WKSOURCE_RESET_COUNTER     0U

/* Wakeup Source 2: Internal Watchdog */
#define ECUM_WKSOURCE_INTERNAL_WDG_ID   2U
#define ECUM_WKSOURCE_INTERNAL_WDG_TO   0U
#define ECUM_WKSOURCE_INTERNAL_WDG_CNT  0U

/* Wakeup Source 3: External Watchdog */
#define ECUM_WKSOURCE_EXTERNAL_WDG_ID   3U
#define ECUM_WKSOURCE_EXTERNAL_WDG_TO   0U
#define ECUM_WKSOURCE_EXTERNAL_WDG_CNT  0U

/* Wakeup Source 4: CAN */
#define ECUM_WKSOURCE_CAN_ID            4U
#define ECUM_WKSOURCE_CAN_TIMEOUT       200U    /* ms */
#define ECUM_WKSOURCE_CAN_COUNTER       3U
#define ECUM_WKSOURCE_CAN_COMM_CH       0U

/* Wakeup Source 5: Ethernet */
#define ECUM_WKSOURCE_ETH_ID            5U
#define ECUM_WKSOURCE_ETH_TIMEOUT       300U    /* ms */
#define ECUM_WKSOURCE_ETH_COUNTER       5U
#define ECUM_WKSOURCE_ETH_COMM_CH       1U

/* Wakeup Source 6: System Timer */
#define ECUM_WKSOURCE_TIMER_ID          6U
#define ECUM_WKSOURCE_TIMER_TIMEOUT     500U    /* ms */
#define ECUM_WKSOURCE_TIMER_COUNTER     10U

/* Wakeup Source 7: IO Pin */
#define ECUM_WKSOURCE_IO_ID             7U
#define ECUM_WKSOURCE_IO_TIMEOUT        100U    /* ms */
#define ECUM_WKSOURCE_IO_COUNTER        2U

/******************************************************************************
 * Sleep Mode Configuration
 ******************************************************************************/

/* Number of configured sleep modes */
#define ECUM_NUM_SLEEP_MODES            3U

/* Sleep Mode 0: Polling Mode */
#define ECUM_SLEEP_MODE_POLLING_ID      0U
#define ECUM_SLEEP_MODE_POLLING_WKSRC   (ECUM_WKSOURCE_TIMER | ECUM_WKSOURCE_IO)
#define ECUM_SLEEP_MODE_POLLING_MCU     0U      /* MCU specific mode */
#define ECUM_SLEEP_MODE_POLLING_MIN_MS  100U    /* Minimum sleep time ms */
#define ECUM_SLEEP_MODE_POLLING_MAX_MS  0U      /* 0 = unlimited */

/* Sleep Mode 1: Halt Mode */
#define ECUM_SLEEP_MODE_HALT_ID         1U
#define ECUM_SLEEP_MODE_HALT_WKSRC      (ECUM_WKSOURCE_ETHERNET | ECUM_WKSOURCE_CAN | ECUM_WKSOURCE_IO)
#define ECUM_SLEEP_MODE_HALT_MCU        1U      /* MCU specific mode */
#define ECUM_SLEEP_MODE_HALT_MIN_MS     50U
#define ECUM_SLEEP_MODE_HALT_MAX_MS     60000U  /* 60 seconds max */

/* Sleep Mode 2: Deep Halt Mode */
#define ECUM_SLEEP_MODE_DEEP_HALT_ID    2U
#define ECUM_SLEEP_MODE_DEEP_HALT_WKSRC (ECUM_WKSOURCE_POWER | ECUM_WKSOURCE_RESET | ECUM_WKSOURCE_IO)
#define ECUM_SLEEP_MODE_DEEP_HALT_MCU   2U      /* MCU specific mode */
#define ECUM_SLEEP_MODE_DEEP_HALT_MIN   1000U   /* 1 second min */
#define ECUM_SLEEP_MODE_DEEP_HALT_MAX   0U      /* unlimited */

/******************************************************************************
 * Default Shutdown Target
 ******************************************************************************/

/* Default shutdown target at startup */
#define ECUM_DEFAULT_SHUTDOWN_TARGET    ECUM_TARGET_SLEEP
#define ECUM_DEFAULT_SLEEP_MODE         ECUM_SLEEP_MODE_HALT
#define ECUM_DEFAULT_RESET_MODE         ECUM_WKSOURCE_INTERNAL_RESET

/******************************************************************************
 * Timing Configuration
 ******************************************************************************/

/* Normal MCU wake-up time in ms */
#define ECUM_NORMAL_MCU_WAKEUP_TIME     50U

/* Minimum shutdown time in ms */
#define ECUM_MIN_SHUTDOWN_TIME          100U

/* Wakeup validation timeout default in ms */
#define ECUM_VALIDATION_TIMEOUT_DEFAULT 500U

/* Main function period in ms */
#define ECUM_MAINFUNCTION_PERIOD_MS     10U

/******************************************************************************
 * Mode Request Ports
 ******************************************************************************/

/* Number of mode request ports */
#define ECUM_NUM_MODE_REQUEST_PORTS     16U

/* User IDs for RUN/POST_RUN requests */
#define ECUM_USER_BSWM                  0U
#define ECUM_USER_COMM                  1U
#define ECUM_USER_DCM                   2U
#define ECUM_USER_DEM                   3U
#define ECUM_USER_NVM                   4U
#define ECUM_USER_WDGM                  5U
#define ECUM_USER_SWC_0                 6U
#define ECUM_USER_SWC_1                 7U
#define ECUM_USER_APP_0                 8U
#define ECUM_USER_APP_1                 9U
#define ECUM_USER_APP_2                 10U
#define ECUM_USER_DDS                   11U
#define ECUM_USER_ETHIF                 12U
#define ECUM_USER_SOAD                  13U
#define ECUM_USER_PDUR                  14U
#define ECUM_USER_MAX                   15U

/******************************************************************************
 * Feature Switches
 ******************************************************************************/

/* Sleep support */
#define ECUM_SLEEP_SUPPORT              STD_ON

/* Wakeup support */
#define ECUM_WAKEUP_SUPPORT             STD_ON

/* Multicore support */
#define ECUM_MULTICORE_SUPPORT          STD_OFF

/* Safety support (E2E protection) */
#define ECUM_SAFETY_SUPPORT             STD_OFF

/* Shutdown hook support */
#define ECUM_SHUTDOWN_HOOK_SUPPORT      STD_ON

/* Version info API */
#define ECUM_VERSION_INFO_API           STD_ON

/* Development error detection */
#define ECUM_DEV_ERROR_DETECT           STD_ON

/* Development error trace */
#define ECUM_DEV_ERROR_TRACE            STD_ON

/******************************************************************************
 * External Configuration Declaration
 ******************************************************************************/

/* External reference to the EcuM configuration structure */
extern const EcuM_ConfigType EcuM_Config;

#ifdef __cplusplus
}
#endif

#endif /* ECUM_CFG_H */
