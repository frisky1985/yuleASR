/******************************************************************************
 * @file    wdgM_Cfg.h
 * @brief   Watchdog Manager (WdgM) Configuration
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef WDGM_CFG_H
#define WDGM_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wdgM.h"

/******************************************************************************
 * Module Configuration Switches
 ******************************************************************************/
#ifndef STD_ON
#define STD_ON      1U
#endif

#ifndef STD_OFF
#define STD_OFF     0U
#endif

#define WDGM_VERSION_INFO_API           STD_ON
#define WDGM_DEV_ERROR_DETECT           STD_ON
#define WDGM_OFF_MODE_ENABLED           STD_ON
#define WDGM_EXT_SUPERVISION_ENABLED    STD_ON
#define WDGM_DEM_REPORTING_ENABLED      STD_ON
#define WDGM_IMMEDIATE_RESET_ENABLED    STD_OFF
#define WDGM_TRIGGER_WATCHDOG_ENABLED   STD_ON

/******************************************************************************
 * Configuration Parameter Values
 ******************************************************************************/
#define WDGM_MAIN_FUNCTION_PERIOD_MS    10U     /* 10ms period */
#define WDGM_EXPIRATION_TIME_MS         100U    /* 100ms expiration */
#define WDGM_INITIAL_MODE               WDGM_MODE_FAST

/******************************************************************************
 * Supervised Entity Configuration
 ******************************************************************************/
/* SE IDs */
#define WDGM_SE_ID_OS_TASK_1MS          0U
#define WDGM_SE_ID_OS_TASK_10MS         1U
#define WDGM_SE_ID_OS_TASK_100MS        2U
#define WDGM_SE_ID_COM_MAIN             3U
#define WDGM_SE_ID_DCM_MAIN             4U
#define WDGM_SE_ID_DEM_MAIN             5U
#define WDGM_SE_ID_NVM_MAIN             6U
#define WDGM_SE_ID_BSWM_MAIN            7U
#define WDGM_SE_ID_APP_TASK_1           8U
#define WDGM_SE_ID_APP_TASK_2           9U
#define WDGM_SE_ID_ETH_RX               10U
#define WDGM_SE_ID_ETH_TX               11U
#define WDGM_SE_ID_DDS_MAIN             12U
#define WDGM_SE_ID_SECOC_MAIN           13U
#define WDGM_SE_ID_MAX                  14U    /* Actual number of SEs used */

/* Checkpoint IDs for SE 0 - OS Task 1ms */
#define WDGM_CP_OS_1MS_START            0U
#define WDGM_CP_OS_1MS_END              1U

/* Checkpoint IDs for SE 1 - OS Task 10ms */
#define WDGM_CP_OS_10MS_START           0U
#define WDGM_CP_OS_10MS_CHECKPOINT_1    1U
#define WDGM_CP_OS_10MS_CHECKPOINT_2    2U
#define WDGM_CP_OS_10MS_END             3U

/* Checkpoint IDs for SE 2 - OS Task 100ms */
#define WDGM_CP_OS_100MS_START          0U
#define WDGM_CP_OS_100MS_CHECKPOINT_1   1U
#define WDGM_CP_OS_100MS_CHECKPOINT_2   2U
#define WDGM_CP_OS_100MS_CHECKPOINT_3   3U
#define WDGM_CP_OS_100MS_END            4U

/* Checkpoint IDs for SE 3 - Com Main */
#define WDGM_CP_COM_START               0U
#define WDGM_CP_COM_MAIN_PROCESS        1U
#define WDGM_CP_COM_END                 2U

/* Checkpoint IDs for SE 4 - Dcm Main */
#define WDGM_CP_DCM_START               0U
#define WDGM_CP_DCM_MAIN_PROCESS        1U
#define WDGM_CP_DCM_END                 2U

/* Checkpoint IDs for SE 5 - Dem Main */
#define WDGM_CP_DEM_START               0U
#define WDGM_CP_DEM_MAIN_PROCESS        1U
#define WDGM_CP_DEM_END                 2U

/* Checkpoint IDs for SE 6 - NvM Main */
#define WDGM_CP_NVM_START               0U
#define WDGM_CP_NVM_MAIN_PROCESS        1U
#define WDGM_CP_NVM_END                 2U

/* Checkpoint IDs for SE 7 - BswM Main */
#define WDGM_CP_BSWM_START              0U
#define WDGM_CP_BSWM_MAIN_PROCESS       1U
#define WDGM_CP_BSWM_END                2U

/* Checkpoint IDs for SE 8 - Application Task 1 */
#define WDGM_CP_APP1_START              0U
#define WDGM_CP_APP1_PROCESS            1U
#define WDGM_CP_APP1_SEND               2U
#define WDGM_CP_APP1_END                3U

/* Checkpoint IDs for SE 9 - Application Task 2 */
#define WDGM_CP_APP2_START              0U
#define WDGM_CP_APP2_PROCESS            1U
#define WDGM_CP_APP2_RECEIVE            2U
#define WDGM_CP_APP2_END                3U

/* Checkpoint IDs for SE 10 - Ethernet RX */
#define WDGM_CP_ETHRX_START             0U
#define WDGM_CP_ETHRX_PROCESS           1U
#define WDGM_CP_ETHRX_END               2U

/* Checkpoint IDs for SE 11 - Ethernet TX */
#define WDGM_CP_ETHTX_START             0U
#define WDGM_CP_ETHTX_PROCESS           1U
#define WDGM_CP_ETHTX_END               2U

/* Checkpoint IDs for SE 12 - DDS Main */
#define WDGM_CP_DDS_START               0U
#define WDGM_CP_DDS_DISCOVERY           1U
#define WDGM_CP_DDS_PUBLISH             2U
#define WDGM_CP_DDS_SUBSCRIBE           3U
#define WDGM_CP_DDS_END                 4U

/* Checkpoint IDs for SE 13 - SecOC Main */
#define WDGM_CP_SECOC_START             0U
#define WDGM_CP_SECOC_VERIFY            1U
#define WDGM_CP_SECOC_AUTHENTICATE      2U
#define WDGM_CP_SECOC_END               3U

/******************************************************************************
 * Mode-Specific Configuration
 ******************************************************************************/

/* Mode: OFF - All supervision deactivated */
#define WDGM_MODE_OFF_ID                WDGM_MODE_OFF

/* Mode: SLOW - Reduced supervision for low power */
#define WDGM_MODE_SLOW_ID               WDGM_MODE_SLOW
#define WDGM_MODE_SLOW_EXP_TIME         500U    /* 500ms expiration */
#define WDGM_MODE_SLOW_TRIGGER          1U      /* WDGIF_SLOW_MODE */

/* Mode: FAST - Full supervision */
#define WDGM_MODE_FAST_ID               WDGM_MODE_FAST
#define WDGM_MODE_FAST_EXP_TIME         100U    /* 100ms expiration */
#define WDGM_MODE_FAST_TRIGGER          2U      /* WDGIF_FAST_MODE */

/******************************************************************************
 * Failure Tolerance Configuration
 ******************************************************************************/
#define WDGM_FAILURE_TOLERANCE_DEFAULT  3U      /* 3 failures before expired */
#define WDGM_FAILURE_TOLERANCE_CRITICAL 1U      /* 1 failure for critical SEs */
#define WDGM_MAX_FAILED_CYCLES          3U      /* Max failed cycles before reset */

/******************************************************************************
 * Alive Supervision Configuration
 ******************************************************************************/
/* SE 0 - OS Task 1ms */
#define WDGM_AS_SE0_EXPECTED            10U     /* 10 indications per 100ms */
#define WDGM_AS_SE0_MIN_MARGIN          1U
#define WDGM_AS_SE0_MAX_MARGIN          2U
#define WDGM_AS_SE0_CYCLE               10U     /* Every 10 main function cycles */

/* SE 1 - OS Task 10ms */
#define WDGM_AS_SE1_EXPECTED            10U     /* 10 indications per 100ms */
#define WDGM_AS_SE1_MIN_MARGIN          1U
#define WDGM_AS_SE1_MAX_MARGIN          2U
#define WDGM_AS_SE1_CYCLE               10U

/* SE 2 - OS Task 100ms */
#define WDGM_AS_SE2_EXPECTED            1U      /* 1 indication per 100ms */
#define WDGM_AS_SE2_MIN_MARGIN          0U
#define WDGM_AS_SE2_MAX_MARGIN          1U
#define WDGM_AS_SE2_CYCLE               10U

/* SE 3 - Com Main */
#define WDGM_AS_SE3_EXPECTED            1U
#define WDGM_AS_SE3_MIN_MARGIN          0U
#define WDGM_AS_SE3_MAX_MARGIN          1U
#define WDGM_AS_SE3_CYCLE               10U

/* SE 4 - Dcm Main */
#define WDGM_AS_SE4_EXPECTED            1U
#define WDGM_AS_SE4_MIN_MARGIN          0U
#define WDGM_AS_SE4_MAX_MARGIN          1U
#define WDGM_AS_SE4_CYCLE               10U

/* SE 5 - Dem Main */
#define WDGM_AS_SE5_EXPECTED            1U
#define WDGM_AS_SE5_MIN_MARGIN          0U
#define WDGM_AS_SE5_MAX_MARGIN          1U
#define WDGM_AS_SE5_CYCLE               10U

/* SE 6 - NvM Main */
#define WDGM_AS_SE6_EXPECTED            1U
#define WDGM_AS_SE6_MIN_MARGIN          0U
#define WDGM_AS_SE6_MAX_MARGIN          1U
#define WDGM_AS_SE6_CYCLE               10U

/* SE 7 - BswM Main */
#define WDGM_AS_SE7_EXPECTED            10U
#define WDGM_AS_SE7_MIN_MARGIN          1U
#define WDGM_AS_SE7_MAX_MARGIN          2U
#define WDGM_AS_SE7_CYCLE               10U

/* SE 8 - Application Task 1 */
#define WDGM_AS_SE8_EXPECTED            5U
#define WDGM_AS_SE8_MIN_MARGIN          1U
#define WDGM_AS_SE8_MAX_MARGIN          2U
#define WDGM_AS_SE8_CYCLE               10U

/* SE 9 - Application Task 2 */
#define WDGM_AS_SE9_EXPECTED            5U
#define WDGM_AS_SE9_MIN_MARGIN          1U
#define WDGM_AS_SE9_MAX_MARGIN          2U
#define WDGM_AS_SE9_CYCLE               10U

/* SE 10 - Ethernet RX */
#define WDGM_AS_SE10_EXPECTED           10U
#define WDGM_AS_SE10_MIN_MARGIN         2U
#define WDGM_AS_SE10_MAX_MARGIN         4U
#define WDGM_AS_SE10_CYCLE              10U

/* SE 11 - Ethernet TX */
#define WDGM_AS_SE11_EXPECTED           10U
#define WDGM_AS_SE11_MIN_MARGIN         2U
#define WDGM_AS_SE11_MAX_MARGIN         4U
#define WDGM_AS_SE11_CYCLE              10U

/* SE 12 - DDS Main */
#define WDGM_AS_SE12_EXPECTED           2U
#define WDGM_AS_SE12_MIN_MARGIN         0U
#define WDGM_AS_SE12_MAX_MARGIN         1U
#define WDGM_AS_SE12_CYCLE              10U

/* SE 13 - SecOC Main */
#define WDGM_AS_SE13_EXPECTED           2U
#define WDGM_AS_SE13_MIN_MARGIN         0U
#define WDGM_AS_SE13_MAX_MARGIN         1U
#define WDGM_AS_SE13_CYCLE              10U

/******************************************************************************
 * Deadline Supervision Configuration (in ms)
 ******************************************************************************/
/* SE 8 - Application Task 1 */
#define WDGM_DS_APP1_MIN                8U      /* 8ms minimum */
#define WDGM_DS_APP1_MAX                20U     /* 20ms maximum */

/* SE 9 - Application Task 2 */
#define WDGM_DS_APP2_MIN                8U
#define WDGM_DS_APP2_MAX                20U

/* SE 12 - DDS Main */
#define WDGM_DS_DDS_MIN                 45U
#define WDGM_DS_DDS_MAX                 100U

/******************************************************************************
 * DEM Event IDs (Production Errors)
 ******************************************************************************/
#define WDGM_E_ALIVE_SUPERVISION_DEM_ID     0x01U
#define WDGM_E_DEADLINE_SUPERVISION_DEM_ID  0x02U
#define WDGM_E_LOGICAL_SUPERVISION_DEM_ID   0x03U
#define WDGM_E_SET_MODE_DEM_ID              0x04U

/******************************************************************************
 * RTE Interface Generation Switches
 ******************************************************************************/
#define WDGM_RTE_MODE_INTERFACE_ENABLED     STD_ON
#define WDGM_RTE_STATUS_INTERFACE_ENABLED   STD_ON
#define WDGM_RTE_SE_STATUS_API_ENABLED      STD_ON

#ifdef __cplusplus
}
#endif

#endif /* WDGM_CFG_H */
