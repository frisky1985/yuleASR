/** @file Det.h
 * @brief Default Error Tracer (Det) interface header
 * @details AUTOSAR R22-11 compliant Det module for development error reporting
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#ifndef DET_H
#define DET_H

#include "common/autosar_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Error IDs for PduR module
 *=============================================================================*/
/* PduR Module ID: 0x37 */
#define PDUR_MODULE_ID                          0x37U

/* PduR Development Error IDs */
#define PDUR_E_NO_INIT                          0x11U
#define PDUR_E_ALREADY_INITIALIZED              0x12U
#define PDUR_E_PARAM_CONFIG                     0x13U
#define PDUR_E_PARAM_POINTER                    0x14U
#define PDUR_E_PARAM_PDUID                      0x15U
#define PDUR_E_ROUTING_ERROR                    0x16U
#define PDUR_E_BUFFER_ERROR                     0x17U

/*=============================================================================
 * Error IDs for SoAd module
 *=============================================================================*/
/* SoAd Module ID: 0x36 */
#define SOAD_MODULE_ID                          0x36U

/* SoAd Development Error IDs */
#define SOAD_E_NO_INIT                          0x11U
#define SOAD_E_ALREADY_INITIALIZED              0x12U
#define SOAD_E_PARAM_CONFIG                     0x13U
#define SOAD_E_PARAM_POINTER                    0x14U
#define SOAD_E_PARAM_PDUID                      0x15U
#define SOAD_E_PARAM_SOCKET                     0x16U
#define SOAD_E_PARAM_MODE                       0x17U
#define SOAD_E_SOCKET_NOT_CONNECTED             0x18U
#define SOAD_E_SOCKET_ERROR                     0x19U
#define SOAD_E_ROUTING_ERROR                    0x1AU

/*=============================================================================*/
/* Error IDs for WdgM module
 *=============================================================================*/
/* WdgM Module ID: 0x12 */
#define WDGM_MODULE_ID                          0x12U

/* WdgM Development Error IDs */
#define WDGM_E_NO_INIT                          0x10U
#define WDGM_E_ALREADY_INITIALIZED              0x11U
#define WDGM_E_PARAM_CONFIG                     0x12U
#define WDGM_E_PARAM_MODE                       0x13U
#define WDGM_E_INV_POINTER                      0x14U
#define WDGM_E_CPID_OUT_OF_RANGE                0x15U
#define WDGM_E_SupervisedEntityId               0x16U
#define WDGM_E_ClearSEStatusFailed              0x17U
#define WDGM_E_DeinitNotAllowed                 0x18U

/* WdgIf Module ID: 0x11 */
#define WDGIF_MODULE_ID                         0x11U

/* WdgIf Development Error IDs */
#define WDGIF_E_PARAM_DEVICE                    0x01U
#define WDGIF_E_INV_POINTER                     0x02U
#define WDGIF_E_INIT_FAILED                     0x03U
#define WDGIF_E_MODE_FAILED                     0x04U

/*=============================================================================
 * Det API
 *=============================================================================*/
/**
 * @brief Reports a development error
 * @param ModuleId The module ID of the caller
 * @param InstanceId The instance ID of the caller
 * @param ApiId The API ID where the error occurred
 * @param ErrorId The error ID
 * @return Always returns E_OK
 */
Std_ReturnType Det_ReportError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId
);

/**
 * @brief Reports a runtime error
 * @param ModuleId The module ID of the caller
 * @param InstanceId The instance ID of the caller
 * @param ApiId The API ID where the error occurred
 * @param ErrorId The error ID
 * @return Always returns E_OK
 */
Std_ReturnType Det_ReportRuntimeError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId
);

/**
 * @brief Reports a transient fault
 * @param ModuleId The module ID of the caller
 * @param InstanceId The instance ID of the caller
 * @param ApiId The API ID where the fault occurred
 * @param FaultId The fault ID
 * @return Always returns E_OK
 */
Std_ReturnType Det_ReportTransientFault(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 FaultId
);

#ifdef __cplusplus
}
#endif

#endif /* DET_H */
