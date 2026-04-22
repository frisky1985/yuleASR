/**
 * @file EcuM.h
 * @brief ECU State Manager module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-22
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: ECU State Manager (ECUM)
 * Layer: Integration Layer
 */

#ifndef ECUM_H
#define ECUM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define ECUM_VENDOR_ID                  (0x01U) /* YuleTech Vendor ID */
#define ECUM_MODULE_ID                  (0x0AU) /* ECUM Module ID */
#define ECUM_AR_RELEASE_MAJOR_VERSION   (0x04U)
#define ECUM_AR_RELEASE_MINOR_VERSION   (0x04U)
#define ECUM_AR_RELEASE_REVISION_VERSION (0x00U)
#define ECUM_SW_MAJOR_VERSION           (0x01U)
#define ECUM_SW_MINOR_VERSION           (0x00U)
#define ECUM_SW_PATCH_VERSION           (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define ECUM_SERVICE_ID_INIT            (0x01U)
#define ECUM_SERVICE_ID_STARTUP         (0x02U)
#define ECUM_SERVICE_ID_SHUTDOWN        (0x03U)
#define ECUM_SERVICE_ID_GETVERSIONINFO  (0x04U)
#define ECUM_SERVICE_ID_SETWAKEUPEVENT  (0x05U)
#define ECUM_SERVICE_ID_VALIDATEWAKEUPEVENT (0x06U)
#define ECUM_SERVICE_ID_GETSTATE        (0x07U)
#define ECUM_SERVICE_ID_REQUESTSHUTDOWN (0x08U)
#define ECUM_SERVICE_ID_REQUESTSLEEP    (0x09U)
#define ECUM_SERVICE_ID_MAINFUNCTION    (0x0AU)
#define ECUM_SERVICE_ID_GETPENDINGWAKEUPEVENTS (0x0BU)
#define ECUM_SERVICE_ID_GETVALIDATEDWAKEUPEVENTS (0x0CU)
#define ECUM_SERVICE_ID_CLEARWAKEUPEVENT (0x0DU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define ECUM_E_NO_ERROR                 (0x00U)
#define ECUM_E_PARAM_CONFIG             (0x01U)
#define ECUM_E_PARAM_POINTER            (0x02U)
#define ECUM_E_UNINIT                   (0x03U)
#define ECUM_E_STATE_TRANSITION         (0x04U)
#define ECUM_E_PARAM_INVALID            (0x05U)

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define ECUM_DEV_ERROR_DETECT           (STD_ON)
#define ECUM_VERSION_INFO_API           (STD_ON)

/*==================================================================================================
*                                    ECU STATE TYPE
==================================================================================================*/
typedef uint8 EcuM_StateType;

#define ECUM_STATE_STARTUP              (0x01U)
#define ECUM_STATE_RUN                  (0x02U)
#define ECUM_STATE_POST_RUN             (0x03U)
#define ECUM_STATE_SHUTDOWN             (0x04U)
#define ECUM_STATE_SLEEP                (0x05U)
#define ECUM_STATE_OFF                  (0x06U)
#define ECUM_STATE_RESET                (0x07U)

/*==================================================================================================
*                                    WAKEUP SOURCE TYPE
==================================================================================================*/
typedef uint32 EcuM_WakeupSourceType;

#define ECUM_WKSOURCE_NONE              ((EcuM_WakeupSourceType)0x00000000U)
#define ECUM_WKSOURCE_POWER             ((EcuM_WakeupSourceType)0x00000001U)
#define ECUM_WKSOURCE_RESET             ((EcuM_WakeupSourceType)0x00000002U)
#define ECUM_WKSOURCE_INTERNAL_RESET    ((EcuM_WakeupSourceType)0x00000004U)
#define ECUM_WKSOURCE_INTERNAL_WDG      ((EcuM_WakeupSourceType)0x00000008U)
#define ECUM_WKSOURCE_EXTERNAL_WDG      ((EcuM_WakeupSourceType)0x00000010U)
#define ECUM_WKSOURCE_CAN               ((EcuM_WakeupSourceType)0x00000100U)
#define ECUM_WKSOURCE_ETH               ((EcuM_WakeupSourceType)0x00000200U)
#define ECUM_WKSOURCE_LIN               ((EcuM_WakeupSourceType)0x00000400U)
#define ECUM_WKSOURCE_FLEXRAY           ((EcuM_WakeupSourceType)0x00000800U)
#define ECUM_WKSOURCE_CHNL_CAN0         ((EcuM_WakeupSourceType)0x00001000U)
#define ECUM_WKSOURCE_CHNL_CAN1         ((EcuM_WakeupSourceType)0x00002000U)
#define ECUM_WKSOURCE_CHNL_LIN0         ((EcuM_WakeupSourceType)0x00004000U)
#define ECUM_WKSOURCE_CHNL_FLEXRAY0     ((EcuM_WakeupSourceType)0x00008000U)

/*==================================================================================================
*                                    CONFIG TYPE
==================================================================================================*/
typedef struct
{
    const void* McuConfigPtr;
    const void* PortConfigPtr;
    const void* BswMConfigPtr;
    uint8 DefaultAppMode;
    uint8 DefaultShutdownTarget;
    uint8 DefaultSleepMode;
} EcuM_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define ECUM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const EcuM_ConfigType EcuM_Config;

#define ECUM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define ECUM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the EcuM module
 */
void EcuM_Init(void);

/**
 * @brief Startup the ECU
 */
void EcuM_Startup(void);

/**
 * @brief Shutdown the ECU
 */
void EcuM_Shutdown(void);

/**
 * @brief Request ECU shutdown
 */
void EcuM_RequestShutdown(void);

/**
 * @brief Request ECU sleep
 * @param sleepMode Sleep mode to enter
 */
void EcuM_RequestSleep(uint8 sleepMode);

/**
 * @brief Sets a wakeup event
 * @param wakeupSource Wakeup source that occurred
 */
void EcuM_SetWakeupEvent(EcuM_WakeupSourceType wakeupSource);

/**
 * @brief Validates a wakeup event
 * @param wakeupSource Wakeup source to validate
 */
void EcuM_ValidateWakeupEvent(EcuM_WakeupSourceType wakeupSource);

/**
 * @brief Gets the current ECU state
 * @return Current ECU state
 */
EcuM_StateType EcuM_GetState(void);

/**
 * @brief Gets pending wakeup events
 * @return Pending wakeup events
 */
EcuM_WakeupSourceType EcuM_GetPendingWakeupEvents(void);

/**
 * @brief Gets validated wakeup events
 * @return Validated wakeup events
 */
EcuM_WakeupSourceType EcuM_GetValidatedWakeupEvents(void);

/**
 * @brief Clears wakeup events
 * @param wakeupSource Wakeup events to clear
 */
void EcuM_ClearWakeupEvent(EcuM_WakeupSourceType wakeupSource);

/**
 * @brief Main function for periodic processing
 */
void EcuM_MainFunction(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
#if (ECUM_VERSION_INFO_API == STD_ON)
void EcuM_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#define ECUM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* ECUM_H */
