/***********************************************************************************************************************
 * File:        dem_det.h
 * Description: Dem Development Error Tracer integration
 **********************************************************************************************************************/

#ifndef DEM_DET_H
#define DEM_DET_H

#include "Std_Types.h"

/* Development error codes */
#define DEM_E_NO_ERROR                      (0x00U)
#define DEM_E_PARAM_CONFIG                  (0x01U)
#define DEM_E_PARAM_POINTER                 (0x02U)
#define DEM_E_PARAM_DATA                    (0x03U)
#define DEM_E_PARAM_LENGTH                  (0x04U)
#define DEM_E_UNINIT                        (0x05U)
#define DEM_E_NVRAM_NOT_SUPPORTED           (0x06U)
#define DEM_E_WRONG_CONFIGURATION           (0x07U)
#define DEM_E_WRONG_CONDITION               (0x08U)
#define DEM_E_OUT_OF_RANGE                  (0x09U)

/* API service IDs for Det reporting */
#define DEM_SID_Init                            (0x01U)
#define DEM_SID_Shutdown                        (0x02U)
#define DEM_SID_SetEventStatus                  (0x03U)
#define DEM_SID_ResetEventStatus                (0x04U)
#define DEM_SID_PreTempActive                   (0x05U)
#define DEM_SID_GetEventStatus                  (0x06U)
#define DEM_SID_GetEventUdsStatus               (0x07U)
#define DEM_SID_GetDTCOfEvent                   (0x08U)
#define DEM_SID_SetDTCFilter                    (0x09U)
#define DEM_SID_GetNumberOfFilteredDTC          (0x0AU)
#define DEM_SID_GetNextFilteredDTC              (0x0BU)
#define DEM_SID_DisableDTCRecordUpdate          (0x0CU)
#define DEM_SID_EnableDTCRecordUpdate           (0x0DU)
#define DEM_SID_GetDTCStatus                    (0x0EU)
#define DEM_SID_ClearDTC                        (0x0FU)
#define DEM_SID_SetOperationCycleState          (0x10U)
#define DEM_SID_GetOperationCycleState          (0x11U)
#define DEM_SID_RestartOperationCycle           (0x12U)
#define DEM_SID_SetEnableCondition              (0x13U)
#define DEM_SID_SetStorageCondition             (0x14U)
#define DEM_SID_GetDebouncingOfEvent            (0x15U)
#define DEM_SID_MainFunction                    (0x16U)
#define DEM_SID_SetWIRStatus                    (0x17U)

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    #include "Det.h"
    #define DEM_REPORT_ERROR(ApiId, ErrorId)         Det_ReportError(DEM_MODULE_ID, 0, (ApiId), (ErrorId))
#else
    #define DEM_REPORT_ERROR(ApiId, ErrorId)
#endif

#endif /* DEM_DET_H */
