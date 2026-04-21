/**
 * @file Det.h
 * @brief Development Error Tracer module header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef DET_H
#define DET_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DET_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define DET_MODULE_ID                   (0x0FU) /* DET Module ID */
#define DET_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define DET_AR_RELEASE_MINOR_VERSION    (0x04U)
#define DET_AR_RELEASE_REVISION_VERSION (0x00U)
#define DET_SW_MAJOR_VERSION            (0x01U)
#define DET_SW_MINOR_VERSION            (0x00U)
#define DET_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define DET_SID_REPORTERROR             (0x01U)
#define DET_SID_START                   (0x02U)
#define DET_SID_GETVERSIONINFO          (0x03U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define DET_E_NO_ERROR                  (0x00U)
#define DET_E_PARAM_POINTER             (0x01U)
#define DET_E_UNAVAILABLE               (0x02U)

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define DET_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Reports a development error
 * @param ModuleId Module ID of the calling module
 * @param InstanceId Instance ID of the calling module
 * @param ApiId API ID of the calling function
 * @param ErrorId Error code to report
 * @return E_OK if error was reported, E_NOT_OK otherwise
 */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);

/**
 * @brief Starts the DET module
 */
void Det_Start(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Det_GetVersionInfo(Std_VersionInfoType* versioninfo);

#define DET_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DET_H */
