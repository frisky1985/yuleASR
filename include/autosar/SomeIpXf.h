#ifndef SOMEIPXF_H
#define SOMEIPXF_H
#include "Std_Types.h"
#include "ComStack_Types.h"

#define SOMEIPXF_VENDOR_ID                  (0x01U)
#define SOMEIPXF_MODULE_ID                  (0x7BU)
#define SOMEIPXF_INSTANCE_ID                (0x00U)
#define SOMEIPXF_AR_RELEASE_MAJOR_VERSION   (0x22U)
#define SOMEIPXF_AR_RELEASE_MINOR_VERSION   (0x11U)
#define SOMEIPXF_AR_RELEASE_REVISION_VERSION (0x00U)
#define SOMEIPXF_SW_MAJOR_VERSION           (0x04U)
#define SOMEIPXF_SW_MINOR_VERSION           (0x07U)
#define SOMEIPXF_SW_PATCH_VERSION           (0x00U)
#define SOMEIPXF_SID_INIT                   (0x01U)
#define SOMEIPXF_SID_DEINIT                 (0x02U)
#define SOMEIPXF_SID_TRANSFORM              (0x04U)
#define SOMEIPXF_SID_DETRANSFORM            (0x05U)

#define SOMEIPXF_E_PARAM_POINTER            (0x01U)
#define SOMEIPXF_E_PARAM_CONFIG             (0x02U)
#define SOMEIPXF_E_UNINIT                   (0x03U)
#define SOMEIPXF_E_INVALID_DATA_TYPE        (0x08U)

typedef struct { uint16 dummy; } SomeIpXf_ConfigType;
Std_ReturnType SomeIpXf_Init(const SomeIpXf_ConfigType* cfg);
#endif
