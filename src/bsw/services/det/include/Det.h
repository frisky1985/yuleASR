/*==================================================================================================
* Project          : AUTOSAR Reference Implementation
* File Name        : Det.h
* Description      : Development Error Tracer header file providing API for error reporting
*                    and BSW development error tracing.
*==================================================================================================
* (C) Copyright 2024, yuleASR
*==================================================================================================*/

#ifndef DET_H
#define DET_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                         INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Det_Cfg.h"

/*==================================================================================================
*                                        VERSION INFO
==================================================================================================*/
#define DET_VENDOR_ID                   (0x01U)
#define DET_MODULE_ID                   (0x0FU)

#define DET_SW_MAJOR_VERSION            (0x04U)
#define DET_SW_MINOR_VERSION            (0x00U)
#define DET_SW_PATCH_VERSION            (0x00U)

#define DET_AR_MAJOR_VERSION            (0x04U)
#define DET_AR_MINOR_VERSION            (0x04U)
#define DET_AR_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                     ERROR CODES
==================================================================================================*/
#define DET_E_NO_ERROR                  (0x00U)
#define DET_E_PARAM_POINTER             (0x01U)
#define DET_E_ALREADY_INITIALIZED       (0x02U)
#define DET_E_NOT_INITIALIZED           (0x03U)
#define DET_E_NVM_NOT_UPDATED           (0x04U)

/*==================================================================================================
*                                 API SERVICE IDs
==================================================================================================*/
#define DET_SID_INIT                    (0x00U)
#define DET_SID_DEINIT                  (0x01U)
#define DET_SID_REPORT_ERROR            (0x02U)
#define DET_SID_START                   (0x03U)
#define DET_SID_GET_VERSION_INFO        (0x04U)
#define DET_SID_REPORT_RUNTIME_ERROR    (0x05U)
#define DET_SID_REPORT_TRANSIENT_FAULT  (0x06U)

/*==================================================================================================
*                                      TYPES AND STRUCTS
==================================================================================================*/

typedef struct
{
    uint16 ModuleId;
    uint8 InstanceId;
    uint8 ApiId;
    uint8 ErrorId;
} Det_ErrorEntryType;

typedef P2FUNC(Std_ReturnType, DET_CODE, Det_ErrorHookPtrType)(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId
);

typedef P2FUNC(Std_ReturnType, DET_CODE, Det_RuntimeErrorHookPtrType)(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId
);

typedef P2FUNC(Std_ReturnType, DET_CODE, Det_TransientFaultHookPtrType)(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 FaultId
);

typedef struct
{
    boolean DetEnableFreezeOnError;
    boolean DetEnableLogging;
    uint8 DetMaxErrorEntries;
} Det_ConfigType;

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/
#define DET_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

extern VAR(Det_ErrorEntryType, DET_VAR) Det_ErrorLog[DET_MAX_ERROR_ENTRIES];
extern VAR(uint8, DET_VAR) Det_ErrorCount;

#define DET_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                      FUNCTION PROTOTYPES
==================================================================================================*/
#define DET_START_SEC_CODE
#include "MemMap.h"

extern FUNC(void, DET_CODE) Det_Init(
    P2CONST(Det_ConfigType, AUTOMATIC, DET_CONST) ConfigPtr
);

extern FUNC(void, DET_CODE) Det_DeInit(void);

extern FUNC(Std_ReturnType, DET_CODE) Det_ReportError(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) ErrorId
);

extern FUNC(void, DET_CODE) Det_Start(void);

#if (DET_VERSION_INFO_API == STD_ON)
extern FUNC(void, DET_CODE) Det_GetVersionInfo(
    P2VAR(Std_VersionInfoType, AUTOMATIC, DET_APPL_DATA) versioninfo
);
#endif

#if (DET_RUNTIME_ERROR_REPORTING == STD_ON)
extern FUNC(Std_ReturnType, DET_CODE) Det_ReportRuntimeError(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) ErrorId
);
#endif

#if (DET_TRANSIENT_FAULT_REPORTING == STD_ON)
extern FUNC(Std_ReturnType, DET_CODE) Det_ReportTransientFault(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) FaultId
);
#endif

#define DET_STOP_SEC_CODE
#include "MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* DET_H */
