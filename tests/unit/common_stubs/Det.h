#ifndef DET_STUB_H
#define DET_STUB_H

#include "Std_Types.h"

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/* Det version info (matching what Crc.h/EcuC.h might check) */
#define DET_VENDOR_ID                   (100u)
#define DET_MODULE_ID                   (15u)
#define DET_INSTANCE_ID                 (0u)
#define DET_AR_RELEASE_MAJOR_VERSION    (4u)
#define DET_AR_RELEASE_MINOR_VERSION    (7u)
#define DET_AR_RELEASE_REVISION_VERSION (0u)
#define DET_SW_MAJOR_VERSION            (1u)
#define DET_SW_MINOR_VERSION            (0u)
#define DET_SW_PATCH_VERSION            (0u)

/* Include Det_Cfg.h for DET_DEV_ERROR_DETECT etc. */
#include "Det_Cfg.h"

/* DET error reporting function */
extern Std_ReturnType Det_ReportError(uint16 ModuleId,
                                       uint8 InstanceId,
                                       uint8 ApiId,
                                       uint8 ErrorId);

#endif /* DET_STUB_H */
