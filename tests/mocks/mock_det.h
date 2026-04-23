/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : DET Mock Header
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef MOCK_DET_H
#define MOCK_DET_H

#include "Std_Types.h"

/*==================================================================================================
*                                      MOCK DATA STRUCTURES
==================================================================================================*/
typedef struct {
    uint16 ModuleId;
    uint8 InstanceId;
    uint8 ApiId;
    uint8 ErrorId;
    uint32 CallCount;
    boolean LastCallValid;
} Det_MockDataType;

/*==================================================================================================
*                                      EXTERNAL VARIABLES
==================================================================================================*/
extern Det_MockDataType Det_MockData;

/*==================================================================================================
*                                      MOCK FUNCTIONS
==================================================================================================*/
void Det_Init(void);
void Det_Start(void);
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
Std_ReturnType Det_ReportTransientFault(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);

/*==================================================================================================
*                                      MOCK CONTROL FUNCTIONS
==================================================================================================*/
void Det_Mock_Reset(void);
uint32 Det_Mock_GetCallCount(void);
void Det_Mock_GetLastError(uint16* ModuleId, uint8* InstanceId, uint8* ApiId, uint8* ErrorId);

#endif /* MOCK_DET_H */
