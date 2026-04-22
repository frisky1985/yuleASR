/**
 * @file BswM.h
 * @brief Basic Software Mode Manager module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-22
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Basic Software Mode Manager (BSWM)
 * Layer: Integration Layer
 */

#ifndef BSWM_H
#define BSWM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define BSWM_VENDOR_ID                  (0x01U) /* YuleTech Vendor ID */
#define BSWM_MODULE_ID                  (0x2AU) /* BSWM Module ID */
#define BSWM_AR_RELEASE_MAJOR_VERSION   (0x04U)
#define BSWM_AR_RELEASE_MINOR_VERSION   (0x04U)
#define BSWM_AR_RELEASE_REVISION_VERSION (0x00U)
#define BSWM_SW_MAJOR_VERSION           (0x01U)
#define BSWM_SW_MINOR_VERSION           (0x00U)
#define BSWM_SW_PATCH_VERSION           (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define BSWM_SERVICE_ID_INIT            (0x00U)
#define BSWM_SERVICE_ID_DEINIT          (0x04U)
#define BSWM_SERVICE_ID_GETVERSIONINFO  (0x01U)
#define BSWM_SERVICE_ID_REQUESTMODE     (0x02U)
#define BSWM_SERVICE_ID_GETMODE         (0x0BU)
#define BSWM_SERVICE_ID_MAINFUNCTION    (0x03U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define BSWM_E_NO_ERROR                 (0x00U)
#define BSWM_E_PARAM_CONFIG             (0x01U)
#define BSWM_E_PARAM_POINTER            (0x02U)
#define BSWM_E_REQ_USER_OUT_OF_RANGE    (0x03U)
#define BSWM_E_REQ_MODE_OUT_OF_RANGE    (0x04U)
#define BSWM_E_NULL_POINTER             (0x05U)
#define BSWM_E_UNINIT                   (0x06U)
#define BSWM_E_PARAM_INVALID            (0x07U)

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define BSWM_DEV_ERROR_DETECT           (STD_ON)
#define BSWM_VERSION_INFO_API           (STD_ON)
#define BSWM_MODE_REQUEST_QUEUE_SIZE    (16U)
#define BSWM_NUM_REQUESTING_USERS       (8U)
#define BSWM_NUM_RULES                  (16U)

/*==================================================================================================
*                                    MODE DEFINITIONS
==================================================================================================*/
typedef uint8 BswM_ModeType;

#define BSWM_MODE_STARTUP               (0x00U)
#define BSWM_MODE_RUN                   (0x01U)
#define BSWM_MODE_POST_RUN              (0x02U)
#define BSWM_MODE_SHUTDOWN              (0x03U)
#define BSWM_MODE_SLEEP                 (0x04U)
#define BSWM_MODE_MAX                   (0x05U)

/*==================================================================================================
*                                    USER TYPE DEFINITIONS
==================================================================================================*/
typedef uint8 BswM_UserType;

#define BSWM_USER_ECU_STATE             (0x00U)
#define BSWM_USER_COMMUNICATION         (0x01U)
#define BSWM_USER_DIAGNOSTIC            (0x02U)
#define BSWM_USER_STORAGE               (0x03U)
#define BSWM_USER_IO_CONTROL            (0x04U)
#define BSWM_USER_MODE_MANAGER          (0x05U)
#define BSWM_USER_WATCHDOG              (0x06U)
#define BSWM_USER_APPLICATION           (0x07U)

/*==================================================================================================
*                                    RULE CONFIGURATION TYPE
==================================================================================================*/
typedef struct
{
    boolean (*ConditionFnc)(void);
    void (*TrueActionList)(void);
    void (*FalseActionList)(void);
} BswM_RuleConfigType;

/*==================================================================================================
*                                    CONFIG TYPE
==================================================================================================*/
typedef struct
{
    const BswM_RuleConfigType* Rules;
    uint8 NumRules;
} BswM_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define BSWM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const BswM_ConfigType BswM_Config;

#define BSWM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define BSWM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the BswM module
 * @param ConfigPtr Pointer to configuration structure
 */
void BswM_Init(const BswM_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the BswM module
 */
void BswM_Deinit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
#if (BSWM_VERSION_INFO_API == STD_ON)
void BswM_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Requests a mode change
 * @param requestingUser User requesting the mode change
 * @param requestedMode Requested mode
 */
void BswM_RequestMode(BswM_UserType requestingUser, BswM_ModeType requestedMode);

/**
 * @brief Gets the current mode for a requesting user
 * @param requestingUser User to get mode for
 * @return Current mode
 */
BswM_ModeType BswM_GetCurrentMode(BswM_UserType requestingUser);

/**
 * @brief Main function for periodic processing
 */
void BswM_MainFunction(void);

#define BSWM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* BSWM_H */
