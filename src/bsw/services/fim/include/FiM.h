/**
 * @file FiM.h
 * @brief Function Inhibition Manager module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Function Inhibition Manager (FiM)
 * Layer: Service Layer
 * Purpose: Function permission management based on diagnostic event status
 */

#ifndef FIM_H
#define FIM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "FiM_Cfg.h"
#include "Dem_Types.h"  /* For Dem_EventIdType, Dem_EventStatusType, Dem_UdsStatusByteType */

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define FIM_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define FIM_INSTANCE_ID           0U
#define FIM_MODULE_ID                   (0x55U) /* FiM Module ID */
#define FIM_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define FIM_AR_RELEASE_MINOR_VERSION    (0x04U)
#define FIM_AR_RELEASE_REVISION_VERSION (0x00U)
#define FIM_SW_MAJOR_VERSION            (0x01U)
#define FIM_SW_MINOR_VERSION            (0x00U)
#define FIM_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define FIM_SID_INIT                            (0x01U)
#define FIM_SID_DEINIT                          (0x02U)
#define FIM_SID_GETVERSIONINFO                  (0x03U)
#define FIM_SID_SETFUNCTIONAVAILABLE            (0x04U)
#define FIM_SID_GETFUNCTIONPERMISSION           (0x05U)
#define FIM_SID_SETFUNCTIONPERMISSION           (0x06U)
#define FIM_SID_GETINHIBITIONSTATUS             (0x07U)
#define FIM_SID_DEMTRIGGERONMONITORSTATUS       (0x08U)
#define FIM_SID_DEMTRIGGERONEVENTSTATUS         (0x09U)
#define FIM_SID_MAINFUNCTION                    (0x0AU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define FIM_E_PARAM_CONFIG                      (0x01U)
#define FIM_E_PARAM_POINTER                     (0x02U)
#define FIM_E_PARAM_FID                         (0x03U)
#define FIM_E_PARAM_EVENTID                     (0x04U)
#define FIM_E_UNINIT                            (0x05U)
#define FIM_E_INIT_FAILED                       (0x06U)

/*==================================================================================================
*                                    FiM INHIBITION MASK TYPES
==================================================================================================*/
#define FIM_INHIBITION_MASK_NONE                (0x00U)
#define FIM_INHIBITION_MASK_ALL                 (0xFFU)
#define FIM_INHIBITION_MASK_TEST_FAILED         (0x01U)
#define FIM_INHIBITION_MASK_TEST_FAILED_TOC     (0x02U)
#define FIM_INHIBITION_MASK_PENDING             (0x04U)
#define FIM_INHIBITION_MASK_CONFIRMED           (0x08U)
#define FIM_INHIBITION_MASK_TEST_NOT_COMPLETED  (0x10U)
#define FIM_INHIBITION_MASK_WARNING_INDICATOR   (0x20U)

/*==================================================================================================
*                                    FiM INHIBITION CONFIGURATION TYPE
==================================================================================================*/
typedef enum {
    FIM_INHIBITION_CONFIG_NONE = 0,
    FIM_INHIBITION_CONFIG_TEST_FAILED,
    FIM_INHIBITION_CONFIG_TEST_FAILED_TOC,
    FIM_INHIBITION_CONFIG_PENDING,
    FIM_INHIBITION_CONFIG_CONFIRMED,
    FIM_INHIBITION_CONFIG_TEST_NOT_COMPLETED_TOC,
    FIM_INHIBITION_CONFIG_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR,
    FIM_INHIBITION_CONFIG_TEST_FAILED_SINCE_LAST_CLEAR,
    FIM_INHIBITION_CONFIG_WARNING_INDICATOR
} FiM_InhibitionConfigurationType;

/*==================================================================================================
*                                    FiM FUNCTION ID TYPE
==================================================================================================*/
typedef uint16 FiM_FunctionIdType;

/*==================================================================================================
*                                    FiM PERMISSION STATE TYPE
==================================================================================================*/
typedef enum {
    FIM_PERMISSION_DENIED = 0,
    FIM_PERMISSION_ALLOWED
} FiM_PermissionStateType;

/*==================================================================================================
*                                    FiM INHIBITION STATUS TYPE
==================================================================================================*/
typedef enum {
    FIM_INHIBITED_NO = 0,
    FIM_INHIBITED_YES
} FiM_InhibitionStatusType;

/*==================================================================================================
*                                    FiM SUMMARY EVENTS TYPE
==================================================================================================*/
typedef uint16 FiM_SummaryEventIdType;

/*==================================================================================================
*                                    FiM EVENT INHIBITION TYPE
==================================================================================================*/
typedef struct {
    Dem_EventIdType EventId;
    uint8 InhibitionMask;
    boolean UseSummaryEvent;
    FiM_SummaryEventIdType SummaryEventId;
} FiM_EventInhibitionType;

/*==================================================================================================
*                                    FiM FUNCTION CONFIG TYPE
==================================================================================================*/
typedef struct {
    FiM_FunctionIdType FunctionId;
    const FiM_EventInhibitionType* EventInhibitions;
    uint8 NumEventInhibitions;
    boolean FunctionAvailable;
} FiM_FunctionConfigType;

/*==================================================================================================
*                                    FiM CONFIG TYPE
==================================================================================================*/
typedef struct {
    const FiM_FunctionConfigType* FunctionConfigs;
    uint16 NumFunctions;
    const Dem_EventIdType* SummaryEvents;
    uint16 NumSummaryEvents;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean InhibitionConfigurationSupported;
} FiM_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define FIM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const FiM_ConfigType FiM_Config;

#define FIM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define FIM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Function Inhibition Manager
 * @param ConfigPtr Pointer to configuration structure
 */
void FiM_Init(const FiM_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the FiM module
 */
void FiM_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void FiM_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Sets function availability
 * @param FID Function ID
 * @param Availability TRUE if function is available, FALSE otherwise
 * @return Result of operation
 */
Std_ReturnType FiM_SetFunctionAvailable(FiM_FunctionIdType FID, boolean Availability);

/**
 * @brief Gets function permission
 * @param FID Function ID
 * @param Permission Pointer to store permission state
 * @return Result of operation
 */
Std_ReturnType FiM_GetFunctionPermission(FiM_FunctionIdType FID, FiM_PermissionStateType* Permission);

/**
 * @brief Sets function permission (for testing purposes)
 * @param FID Function ID
 * @param Permission Permission state to set
 * @return Result of operation
 */
Std_ReturnType FiM_SetFunctionPermission(FiM_FunctionIdType FID, FiM_PermissionStateType Permission);

/**
 * @brief Gets inhibition status
 * @param FID Function ID
 * @param InhibitionStatus Pointer to store inhibition status
 * @return Result of operation
 */
Std_ReturnType FiM_GetInhibitionStatus(FiM_FunctionIdType FID, FiM_InhibitionStatusType* InhibitionStatus);

/**
 * @brief DEM trigger on monitor status change
 * @param EventId Event ID
 * @param EventStatus New event status
 */
void FiM_DemTriggerOnMonitorStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);

/**
 * @brief DEM trigger on event status change
 * @param EventId Event ID
 * @param EventStatusOld Previous event status
 * @param EventStatusNew New event status
 */
void FiM_DemTriggerOnEventStatus(Dem_EventIdType EventId, 
                                  Dem_UdsStatusByteType EventStatusOld, 
                                  Dem_UdsStatusByteType EventStatusNew);

/**
 * @brief Main function for FiM processing
 */
void FiM_MainFunction(void);

#define FIM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* FIM_H */
