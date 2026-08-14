/** @file Det_mock.h — Det external declarations for coverage tests */
#ifndef COV_DET_MOCK_H
#define COV_DET_MOCK_H
#include "Std_Types.h"
#define DET_MODULE_ID        15U
#define DET_INSTANCE_ID      0U
#define DET_VENDOR_ID        100U
#define DET_AR_RELEASE_MAJOR_VERSION 4U
#define DET_AR_RELEASE_MINOR_VERSION 4U
#define DET_SW_MAJOR_VERSION 1U
#define DET_SW_MINOR_VERSION 0U
#define DET_SW_PATCH_VERSION 0U
/* Det_ConfigType for modules that need it */
typedef struct { uint8 dummy; } Det_ConfigType;
extern void Det_Init(void);
extern void Det_Start(void);
extern Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
extern Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
extern Std_ReturnType Det_ReportTransientFault(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
typedef struct {
    uint16 ModuleId; uint8 InstanceId; uint8 ApiId; uint8 ErrorId;
    uint32 CallCount; boolean LastCallValid;
} Det_MockDataType;
extern Det_MockDataType Det_MockData;
extern void Det_Mock_Reset(void);
#endif
