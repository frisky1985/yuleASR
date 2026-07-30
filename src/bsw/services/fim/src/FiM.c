/**
 * @file FiM.c
 * @brief Function Inhibition Manager
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Service Layer)
* Dependencies         : Dem, Det
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-30
* Author               : AI Agent (FiM Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "FiM.h"
#include "FiM_Cfg.h"
#include "Dem.h"
#include "Det.h"
#include "MemMap.h"
#include "string.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define FIM_INSTANCE_ID                 (0x00U)

/* Module state */
#define FIM_STATE_UNINIT                (0x00U)
#define FIM_STATE_INIT                  (0x01U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (FIM_DEV_ERROR_DETECT == STD_ON)
    #define FIM_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(FIM_MODULE_ID, FIM_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define FIM_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* Function state type */
typedef struct {
    FiM_PermissionStateType Permission;
    boolean Available;
    FiM_InhibitionStatusType InhibitionStatus;
    uint8 LastCalculatedInhibitionMask;
} FiM_FunctionStateType;

/* Summary event state type */
typedef struct {
    boolean IsFailed;
    uint8 InhibitionMask;
} FiM_SummaryEventStateType;

/* Module internal state */
typedef struct {
    uint8 State;
    const FiM_ConfigType* ConfigPtr;
    FiM_FunctionStateType FunctionStates[FIM_NUM_FUNCTIONS];
    FiM_SummaryEventStateType SummaryEventStates[FIM_NUM_SUMMARY_EVENTS];
} FiM_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define FIM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC FiM_InternalStateType FiM_InternalState;

#define FIM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC const FiM_FunctionConfigType* FiM_FindFunctionConfig(FiM_FunctionIdType FID);
STATIC Std_ReturnType FiM_CalculateInhibitionMask(FiM_FunctionIdType FID, uint8* InhibitionMask);
STATIC boolean FiM_CheckEventInhibition(const FiM_EventInhibitionType* EventInhibition, uint8 CurrentDtcStatus);
STATIC void FiM_UpdateFunctionPermission(FiM_FunctionIdType FID);
STATIC void FiM_UpdateSummaryEventState(FiM_SummaryEventIdType SummaryEventId);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define FIM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Find function configuration by Function ID
 */
STATIC const FiM_FunctionConfigType* FiM_FindFunctionConfig(FiM_FunctionIdType FID)
{
    const FiM_FunctionConfigType* result = NULL_PTR;
    uint16 i;

    if ((FiM_InternalState.ConfigPtr != NULL_PTR) && 
        (FID >= FIM_FID_MIN) && (FID <= FIM_FID_MAX))
    {
        for (i = 0U; i < FiM_InternalState.ConfigPtr->NumFunctions; i++)
        {
            if (FiM_InternalState.ConfigPtr->FunctionConfigs[i].FunctionId == FID)
            {
                result = &FiM_InternalState.ConfigPtr->FunctionConfigs[i];
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Calculate inhibition mask for a function based on all related events
 */
STATIC Std_ReturnType FiM_CalculateInhibitionMask(FiM_FunctionIdType FID, uint8* InhibitionMask)
{
    Std_ReturnType result = E_NOT_OK;
    const FiM_FunctionConfigType* functionConfig;
    boolean eventFailed;
    Dem_UdsStatusByteType dtcStatus;
    uint8 calculatedMask = FIM_INHIBITION_MASK_NONE;
    uint8 i;

    functionConfig = FiM_FindFunctionConfig(FID);

    if ((functionConfig != NULL_PTR) && (InhibitionMask != NULL_PTR))
    {
        /* Iterate through all event inhibitions for this function */
        for (i = 0U; i < functionConfig->NumEventInhibitions; i++)
        {
            const FiM_EventInhibitionType* eventInhibition = &functionConfig->EventInhibitions[i];
            
            /* Check if using summary event */
            if (eventInhibition->UseSummaryEvent)
            {
                /* Use summary event state */
                if ((eventInhibition->SummaryEventId < FIM_NUM_SUMMARY_EVENTS) &&
                    (FiM_InternalState.SummaryEventStates[eventInhibition->SummaryEventId].IsFailed))
                {
                    calculatedMask |= eventInhibition->InhibitionMask;
                }
            }
            else
            {
                /* Check event failed status using DEM API */
                if (Dem_GetEventFailed(eventInhibition->EventId, &eventFailed) == E_OK)
                {
                    /* Build a UDS-like status byte from event status */
                    dtcStatus = 0U;
                    
                    if (eventFailed)
                    {
                        dtcStatus |= DEM_UDS_STATUS_TF;  /* Test Failed */
                        dtcStatus |= DEM_UDS_STATUS_TFTOC; /* Test Failed This Operation Cycle */
                        dtcStatus |= DEM_UDS_STATUS_PDTC;  /* Pending DTC */
                    }
                    
                    /* Check if event inhibits this function */
                    if (FiM_CheckEventInhibition(eventInhibition, dtcStatus))
                    {
                        calculatedMask |= eventInhibition->InhibitionMask;
                    }
                }
            }
        }

        *InhibitionMask = calculatedMask;
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Check if event should inhibit function based on current DTC status
 */
STATIC boolean FiM_CheckEventInhibition(const FiM_EventInhibitionType* EventInhibition, uint8 CurrentDtcStatus)
{
    boolean inhibit = FALSE;

    if (EventInhibition != NULL_PTR)
    {
        /* Check if any of the configured inhibition conditions match */
        if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_TEST_FAILED) &&
            (CurrentDtcStatus & DEM_UDS_STATUS_TF))
        {
            inhibit = TRUE;
        }
        else if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_TEST_FAILED_TOC) &&
                 (CurrentDtcStatus & DEM_UDS_STATUS_TFTOC))
        {
            inhibit = TRUE;
        }
        else if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_PENDING) &&
                 (CurrentDtcStatus & DEM_UDS_STATUS_PDTC))
        {
            inhibit = TRUE;
        }
        else if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_CONFIRMED) &&
                 (CurrentDtcStatus & DEM_UDS_STATUS_CDTC))
        {
            inhibit = TRUE;
        }
        else if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_TEST_NOT_COMPLETED) &&
                 (CurrentDtcStatus & DEM_UDS_STATUS_TNCTOC))
        {
            inhibit = TRUE;
        }
        else if ((EventInhibition->InhibitionMask & FIM_INHIBITION_MASK_WARNING_INDICATOR) &&
                 (CurrentDtcStatus & DEM_UDS_STATUS_WIR))
        {
            inhibit = TRUE;
        }
    }

    return inhibit;
}

/**
 * @brief   Update function permission based on inhibition mask
 */
STATIC void FiM_UpdateFunctionPermission(FiM_FunctionIdType FID)
{
    uint8 inhibitionMask = FIM_INHIBITION_MASK_NONE;
    uint16 functionIndex;

    if ((FID >= FIM_FID_MIN) && (FID <= FIM_FID_MAX))
    {
        functionIndex = FID - FIM_FID_MIN;

        /* Calculate current inhibition mask */
        if (FiM_CalculateInhibitionMask(FID, &inhibitionMask) == E_OK)
        {
            FiM_InternalState.FunctionStates[functionIndex].LastCalculatedInhibitionMask = inhibitionMask;

            /* Update inhibition status */
            if (inhibitionMask != FIM_INHIBITION_MASK_NONE)
            {
                FiM_InternalState.FunctionStates[functionIndex].InhibitionStatus = FIM_INHIBITED_YES;
                FiM_InternalState.FunctionStates[functionIndex].Permission = FIM_PERMISSION_DENIED;
            }
            else
            {
                FiM_InternalState.FunctionStates[functionIndex].InhibitionStatus = FIM_INHIBITED_NO;
                FiM_InternalState.FunctionStates[functionIndex].Permission = FIM_PERMISSION_ALLOWED;
            }
        }
    }
}

/**
 * @brief   Update summary event state based on its component events
 */
STATIC void FiM_UpdateSummaryEventState(FiM_SummaryEventIdType SummaryEventId)
{
    if ((SummaryEventId >= FIM_SUMMARY_EVENT_ID_MIN) && 
        (SummaryEventId < FIM_SUMMARY_EVENT_ID_MAX + FIM_SUMMARY_EVENT_ID_MIN))
    {
        uint16 index = SummaryEventId - FIM_SUMMARY_EVENT_ID_MIN;
        boolean isFailed = FALSE;
        
        /* Check if the summary event is a valid DEM event */
        if (SummaryEventId > 0U)
        {
            if (Dem_GetEventFailed(SummaryEventId, &isFailed) != E_OK)
            {
                isFailed = FALSE;
            }
        }
        
        FiM_InternalState.SummaryEventStates[index].IsFailed = isFailed;
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the FiM module
 */
void FiM_Init(const FiM_ConfigType* ConfigPtr)
{
    uint16 i;

#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        FIM_DET_REPORT_ERROR(FIM_SID_INIT, FIM_E_PARAM_POINTER);
        return;
    }

    if (ConfigPtr->NumFunctions > FIM_NUM_FUNCTIONS)
    {
        FIM_DET_REPORT_ERROR(FIM_SID_INIT, FIM_E_PARAM_CONFIG);
        return;
    }
#endif

    /* Store configuration pointer */
    FiM_InternalState.ConfigPtr = ConfigPtr;

    /* Initialize function states */
    for (i = 0U; i < FIM_NUM_FUNCTIONS; i++)
    {
        FiM_InternalState.FunctionStates[i].Permission = FIM_DEFAULT_PERMISSION;
        FiM_InternalState.FunctionStates[i].Available = FIM_DEFAULT_AVAILABILITY;
        FiM_InternalState.FunctionStates[i].InhibitionStatus = FIM_INHIBITED_NO;
        FiM_InternalState.FunctionStates[i].LastCalculatedInhibitionMask = FIM_INHIBITION_MASK_NONE;
    }

    /* Initialize summary event states */
    for (i = 0U; i < FIM_NUM_SUMMARY_EVENTS; i++)
    {
        FiM_InternalState.SummaryEventStates[i].IsFailed = FALSE;
        FiM_InternalState.SummaryEventStates[i].InhibitionMask = FIM_INHIBITION_MASK_NONE;
    }

    /* Calculate initial permissions for all configured functions */
    for (i = 0U; i < ConfigPtr->NumFunctions; i++)
    {
        if (ConfigPtr->FunctionConfigs[i].FunctionId != FIM_FID_INVALID)
        {
            FiM_UpdateFunctionPermission(ConfigPtr->FunctionConfigs[i].FunctionId);
        }
    }

    /* Set module state to initialized */
    FiM_InternalState.State = FIM_STATE_INIT;
}

/**
 * @brief   Deinitializes the FiM module
 */
void FiM_DeInit(void)
{
#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (FiM_InternalState.State != FIM_STATE_INIT)
    {
        FIM_DET_REPORT_ERROR(FIM_SID_DEINIT, FIM_E_UNINIT);
        return;
    }
#endif

    /* Clear configuration pointer */
    FiM_InternalState.ConfigPtr = NULL_PTR;

    /* Set module state to uninitialized */
    FiM_InternalState.State = FIM_STATE_UNINIT;
}

/**
 * @brief   Set function availability
 */
Std_ReturnType FiM_SetFunctionAvailable(FiM_FunctionIdType FID, boolean Availability)
{
    Std_ReturnType result = E_NOT_OK;

#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (FiM_InternalState.State != FIM_STATE_INIT)
    {
        FIM_DET_REPORT_ERROR(FIM_SID_SETFUNCTIONAVAILABLE, FIM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((FID < FIM_FID_MIN) || (FID > FIM_FID_MAX))
    {
        FIM_DET_REPORT_ERROR(FIM_SID_SETFUNCTIONAVAILABLE, FIM_E_PARAM_FID);
        return E_NOT_OK;
    }
#endif

    if ((FID >= FIM_FID_MIN) && (FID <= FIM_FID_MAX))
    {
        uint16 functionIndex = FID - FIM_FID_MIN;
        const FiM_FunctionConfigType* functionConfig = FiM_FindFunctionConfig(FID);

        if (functionConfig != NULL_PTR)
        {
            FiM_InternalState.FunctionStates[functionIndex].Available = Availability;
            
            /* Recalculate permission based on availability */
            if (Availability == FALSE)
            {
                FiM_InternalState.FunctionStates[functionIndex].Permission = FIM_PERMISSION_DENIED;
            }
            else
            {
                FiM_UpdateFunctionPermission(FID);
            }
            
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Get function permission
 */
Std_ReturnType FiM_GetFunctionPermission(FiM_FunctionIdType FID, FiM_PermissionStateType* Permission)
{
    Std_ReturnType result = E_NOT_OK;

#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (FiM_InternalState.State != FIM_STATE_INIT)
    {
        FIM_DET_REPORT_ERROR(FIM_SID_GETFUNCTIONPERMISSION, FIM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((FID < FIM_FID_MIN) || (FID > FIM_FID_MAX))
    {
        FIM_DET_REPORT_ERROR(FIM_SID_GETFUNCTIONPERMISSION, FIM_E_PARAM_FID);
        return E_NOT_OK;
    }

    if (Permission == NULL_PTR)
    {
        FIM_DET_REPORT_ERROR(FIM_SID_GETFUNCTIONPERMISSION, FIM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if ((FID >= FIM_FID_MIN) && (FID <= FIM_FID_MAX) && (Permission != NULL_PTR))
    {
        uint16 functionIndex = FID - FIM_FID_MIN;
        const FiM_FunctionConfigType* functionConfig = FiM_FindFunctionConfig(FID);

        if (functionConfig != NULL_PTR)
        {
            /* Return permission - also check if function is available */
            if (FiM_InternalState.FunctionStates[functionIndex].Available == FALSE)
            {
                *Permission = FIM_PERMISSION_DENIED;
            }
            else
            {
                *Permission = FiM_InternalState.FunctionStates[functionIndex].Permission;
            }
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Set function permission (for testing purposes)
 */
Std_ReturnType FiM_SetFunctionPermission(FiM_FunctionIdType FID, FiM_PermissionStateType Permission)
{
    Std_ReturnType result = E_NOT_OK;

#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (FiM_InternalState.State != FIM_STATE_INIT)
    {
        FIM_DET_REPORT_ERROR(FIM_SID_SETFUNCTIONPERMISSION, FIM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((FID < FIM_FID_MIN) || (FID > FIM_FID_MAX))
    {
        FIM_DET_REPORT_ERROR(FIM_SID_SETFUNCTIONPERMISSION, FIM_E_PARAM_FID);
        return E_NOT_OK;
    }
#endif

    if ((FID >= FIM_FID_MIN) && (FID <= FIM_FID_MAX))
    {
        uint16 functionIndex = FID - FIM_FID_MIN;
        const FiM_FunctionConfigType* functionConfig = FiM_FindFunctionConfig(FID);

        if (functionConfig != NULL_PTR)
        {
            FiM_InternalState.FunctionStates[functionIndex].Permission = Permission;
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Get inhibition status
 */
Std_ReturnType FiM_GetInhibitionStatus(FiM_FunctionIdType FID, FiM_InhibitionStatusType* InhibitionStatus)
{
    Std_ReturnType result = E_NOT_OK;

#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == InhibitionStatus)
    {
        Det_ReportError(FIM_MODULE_ID, FIM_INSTANCE_ID, FIM_SID_GETINHIBITIONSTATUS, FIM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (FIM_MAX_FUNCTIONS <= FID)
    {
        Det_ReportError(FIM_MODULE_ID, FIM_INSTANCE_ID, FIM_SID_GETINHIBITIONSTATUS, FIM_E_PARAM_FID);
        return E_NOT_OK;
    }
#endif

    /* Check if FID is within range */
    if (FID < FIM_MAX_FUNCTIONS)
    {
        *InhibitionStatus = FiM_InternalState.FunctionStates[FID].InhibitionStatus;
        result = E_OK;
    }

    return result;
}
