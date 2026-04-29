/**
 * @file dcm_dem_integration.h
 * @brief DCM-DEM Integration Layer Interface
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * This module provides integration between DCM (Diagnostic Communication Manager)
 * and DEM (Diagnostic Event Manager) for DTC services:
 * - 0x14 ClearDiagnosticInformation
 * - 0x19 ReadDTCInformation
 * - 0x85 ControlDTCSetting
 *
 * @copyright Copyright (c) 2024
 */

#ifndef DCM_DEM_INTEGRATION_H
#define DCM_DEM_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dcm_types.h"
#include "dem.h"
#include "dem_types.h"

/******************************************************************************
 * Module Version Information
 ******************************************************************************/
#define DCM_DEM_INTEGRATION_MAJOR_VERSION       1U
#define DCM_DEM_INTEGRATION_MINOR_VERSION       0U
#define DCM_DEM_INTEGRATION_PATCH_VERSION       0U

/******************************************************************************
 * UDS Sub-function IDs for 0x85 ControlDTCSetting
 ******************************************************************************/
#define DCM_SUBFUNC_DTC_SETTING_ON              0x01U   /* Enable DTC setting */
#define DCM_SUBFUNC_DTC_SETTING_OFF             0x02U   /* Disable DTC setting */

/******************************************************************************
 * DTC Setting Control Option Record - DTC Group Definitions
 ******************************************************************************/
#define DCM_DTC_GROUP_ALL                       0xFFFFFFU   /* All DTCs */
#define DCM_DTC_GROUP_EMISSION                  0x000000U   /* Emission-related DTCs */
#define DCM_DTC_GROUP_POWERTRAIN                0x010000U   /* Powertrain DTCs */
#define DCM_DTC_GROUP_CHASSIS                   0x020000U   /* Chassis DTCs */
#define DCM_DTC_GROUP_BODY                      0x030000U   /* Body DTCs */
#define DCM_DTC_GROUP_NETWORK                   0x040000U   /* Network/Communication DTCs */

/******************************************************************************
 * UDS Sub-function IDs for 0x19 ReadDTCInformation
 ******************************************************************************/
/* reportNumberOfDTCByStatusMask */
#define DCM_SUBFUNC_REPORT_NUMBER_OF_DTC_BY_STATUS_MASK         0x01U
/* reportDTCByStatusMask */
#define DCM_SUBFUNC_REPORT_DTC_BY_STATUS_MASK                   0x02U
/* reportDTCSnapshotIdentification */
#define DCM_SUBFUNC_REPORT_DTC_SNAPSHOT_IDENTIFICATION          0x03U
/* reportDTCSnapshotRecordByDTCNumber */
#define DCM_SUBFUNC_REPORT_DTC_SNAPSHOT_RECORD_BY_DTC           0x04U
/* reportDTCStoredDataByRecordNumber */
#define DCM_SUBFUNC_REPORT_DTC_STORED_DATA_BY_RECORD            0x05U
/* reportDTCExtDataRecordByDTCNumber */
#define DCM_SUBFUNC_REPORT_DTC_EXT_DATA_RECORD_BY_DTC           0x06U
/* reportNumberOfDTCBySeverityMaskRecord */
#define DCM_SUBFUNC_REPORT_NUMBER_OF_DTC_BY_SEVERITY_MASK       0x07U
/* reportDTCBySeverityMaskRecord */
#define DCM_SUBFUNC_REPORT_DTC_BY_SEVERITY_MASK                 0x08U
/* reportSeverityInformationOfDTC */
#define DCM_SUBFUNC_REPORT_SEVERITY_INFORMATION_OF_DTC          0x09U
/* reportSupportedDTC */
#define DCM_SUBFUNC_REPORT_SUPPORTED_DTC                        0x0AU
/* reportFirstTestFailedDTC */
#define DCM_SUBFUNC_REPORT_FIRST_TEST_FAILED_DTC                0x0BU
/* reportFirstConfirmedDTC */
#define DCM_SUBFUNC_REPORT_FIRST_CONFIRMED_DTC                  0x0CU
/* reportMostRecentTestFailedDTC */
#define DCM_SUBFUNC_REPORT_MOST_RECENT_TEST_FAILED_DTC          0x0DU
/* reportMostRecentConfirmedDTC */
#define DCM_SUBFUNC_REPORT_MOST_RECENT_CONFIRMED_DTC            0x0EU
/* reportMirrorMemoryDTCByStatusMask */
#define DCM_SUBFUNC_REPORT_MIRROR_MEMORY_DTC_BY_STATUS_MASK     0x0FU
/* reportMirrorMemoryDTCExtDataRecordByDTCNumber */
#define DCM_SUBFUNC_REPORT_MIRROR_MEMORY_DTC_EXT_DATA_BY_DTC    0x10U
/* reportNumberOfMirrorMemoryDTCByStatusMask */
#define DCM_SUBFUNC_REPORT_NUMBER_OF_MIRROR_MEMORY_DTC          0x11U
/* reportNumberOfEmissionsRelatedOBDDTCByStatusMask */
#define DCM_SUBFUNC_REPORT_NUMBER_OF_EMISSION_RELATED_DTC       0x12U
/* reportEmissionsRelatedOBDDTCByStatusMask */
#define DCM_SUBFUNC_REPORT_EMISSION_RELATED_DTC_BY_STATUS_MASK  0x13U
/* reportDTCFaultDetectionCounter */
#define DCM_SUBFUNC_REPORT_DTC_FAULT_DETECTION_COUNTER          0x14U
/* reportDTCWithPermanentStatus */
#define DCM_SUBFUNC_REPORT_DTC_WITH_PERMANENT_STATUS            0x15U
/* reportDTCExtDataRecordByRecordNumber */
#define DCM_SUBFUNC_REPORT_DTC_EXT_DATA_RECORD_BY_NUMBER        0x16U
/* reportUserDefMemoryDTCByStatusMask */
#define DCM_SUBFUNC_REPORT_USER_DEF_MEMORY_DTC_BY_STATUS_MASK   0x17U
/* reportUserDefMemoryDTCSnapshotRecordByDTCNumber */
#define DCM_SUBFUNC_REPORT_USER_DEF_MEMORY_DTC_SNAPSHOT_BY_DTC  0x18U
/* reportUserDefMemoryDTCExtDataRecordByDTCNumber */
#define DCM_SUBFUNC_REPORT_USER_DEF_MEMORY_DTC_EXT_DATA_BY_DTC  0x19U
/* reportSupportedDTCExtDataRecord */
#define DCM_SUBFUNC_REPORT_SUPPORTED_DTC_EXT_DATA_RECORD        0x1AU
/* reportDTCExtDataRecordByRecordNumber */
#define DCM_SUBFUNC_REPORT_WWH_OBD_DTC_BY_STATUS_MASK           0x42U

/******************************************************************************
 * DTC Format Identifiers
 ******************************************************************************/
#define DCM_DTC_FORMAT_SAE_J2012_DA_2013    0x00U   /* OBD-II DTC format */
#define DCM_DTC_FORMAT_ISO_11992_4          0x01U   /* UDS DTC format */
#define DCM_DTC_FORMAT_SAE_J1939_73         0x02U   /* J1939 DTC format */
#define DCM_DTC_FORMAT_ISO_14229_1          0x03U   /* ISO 14229-1 DTC format */
#define DCM_DTC_FORMAT_SAE_J2012_DB_2010    0x04U   /* OBD-II DTC format (DB2010) */

/******************************************************************************
 * DTC Status Availability Mask
 ******************************************************************************/
#define DCM_DTC_STATUS_AVAILABILITY_MASK    0xFFU   /* All status bits available */

/******************************************************************************
 * Configuration Types
 ******************************************************************************/
/**
 * @brief DCM-DEM Integration Configuration Type
 */
typedef struct {
    uint8_t maxNumberOfDTCs;                    /* Maximum number of DTCs to return */
    uint16_t maxResponseLength;                 /* Maximum response message length */
    boolean supportDtcSnapshot;                 /* Support DTC snapshot records */
    boolean supportDtcExtendedData;             /* Support DTC extended data records */
    uint8_t defaultDtcStatusMask;               /* Default DTC status mask */
    uint8_t defaultDtcSeverityMask;             /* Default DTC severity mask */
} Dcm_DemIntegrationConfigType;

/******************************************************************************
 * Callback Function Types
 ******************************************************************************/
/**
 * @brief DTC Status Changed Callback
 * @param dtc The DTC code
 * @param statusOld Old DTC status
 * @param statusNew New DTC status
 */
typedef void (*Dcm_DemDtcStatusChangedCallbackType)(
    uint32_t dtc,
    uint8_t statusOld,
    uint8_t statusNew
);

/**
 * @brief DEM Event Status Changed Callback for DCM notification
 * @param EventId The event ID
 * @param EventStatusOld Old event status
 * @param EventStatusNew New event status
 */
typedef void (*Dcm_DemEventStatusCallbackType)(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatusOld,
    Dem_EventStatusType EventStatusNew
);

/******************************************************************************
 * Initialization Functions
 ******************************************************************************/

/**
 * @brief Initialize DCM-DEM integration module
 *
 * @param config Pointer to integration configuration
 * @return Dcm_ReturnType Initialization result
 */
Dcm_ReturnType Dcm_DemIntegration_Init(
    const Dcm_DemIntegrationConfigType *config
);

/**
 * @brief Deinitialize DCM-DEM integration module
 *
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_DemIntegration_DeInit(void);

/******************************************************************************
 * Service Handler Functions (Called by DCM)
 ******************************************************************************/

/**
 * @brief Handle ControlDTCSetting service (0x85)
 *
 * @param request Pointer to request message
 * @param response Pointer to response message buffer
 * @return Dcm_ReturnType Processing result
 */
Dcm_ReturnType Dcm_DemIntegration_ControlDTCSetting(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Handle ClearDiagnosticInformation service (0x14)
 *
 * @param request Pointer to request message
 * @param response Pointer to response message buffer
 * @return Dcm_ReturnType Processing result
 */
Dcm_ReturnType Dcm_DemIntegration_ClearDiagnosticInformation(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Handle ReadDTCInformation service (0x19)
 *
 * @param request Pointer to request message
 * @param response Pointer to response message buffer
 * @return Dcm_ReturnType Processing result
 */
Dcm_ReturnType Dcm_DemIntegration_ReadDTCInformation(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/******************************************************************************
 * DEM Callback Registration (Called by DEM)
 ******************************************************************************/

/**
 * @brief Register callback for DTC status changes
 *
 * @param callback Callback function pointer
 * @return Dcm_ReturnType Registration result
 */
Dcm_ReturnType Dcm_DemIntegration_RegisterDtcStatusCallback(
    Dcm_DemDtcStatusChangedCallbackType callback
);

/**
 * @brief Register callback for event status changes
 *
 * @param callback Callback function pointer
 * @return Dcm_ReturnType Registration result
 */
Dcm_ReturnType Dcm_DemIntegration_RegisterEventStatusCallback(
    Dcm_DemEventStatusCallbackType callback
);

/******************************************************************************
 * DEM Notification Functions (Called by DEM)
 ******************************************************************************/

/**
 * @brief Notify DCM that DTC status has changed
 *
 * @param dtc The DTC code
 * @param statusOld Old DTC status
 * @param statusNew New DTC status
 */
void Dcm_DemIntegration_NotifyDtcStatusChanged(
    uint32_t dtc,
    Dem_UdsStatusByteType statusOld,
    Dem_UdsStatusByteType statusNew
);

/**
 * @brief Notify DCM that event status has changed
 *
 * @param EventId The event ID
 * @param EventStatusOld Old event status
 * @param EventStatusNew New event status
 */
void Dcm_DemIntegration_NotifyEventStatusChanged(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatusOld,
    Dem_EventStatusType EventStatusNew
);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Check if integration module is initialized
 *
 * @return bool True if initialized
 */
bool Dcm_DemIntegration_IsInitialized(void);

/**
 * @brief Convert DEM return type to DCM return type
 *
 * @param demReturn DEM return value
 * @return Dcm_ReturnType DCM return value
 */
Dcm_ReturnType Dcm_DemIntegration_ConvertReturnType(
    Std_ReturnType demReturn
);

/**
 * @brief Get DTC status availability mask
 *
 * @return uint8_t Status availability mask
 */
uint8_t Dcm_DemIntegration_GetDTCStatusAvailabilityMask(void);

/**
 * @brief Get DTC format identifier
 *
 * @return uint8_t DTC format identifier
 */
uint8_t Dcm_DemIntegration_GetDTCFormatIdentifier(void);

/******************************************************************************
 * Internal Helper Functions (for use by integration layer only)
 ******************************************************************************/

/**
 * @brief Process reportNumberOfDTCByStatusMask subfunction
 */
static Dcm_ReturnType processReportNumberOfDTCByStatusMask(
    uint8_t dtcStatusMask,
    Dcm_ResponseType *response
);

/**
 * @brief Process reportDTCByStatusMask subfunction
 */
static Dcm_ReturnType processReportDTCByStatusMask(
    uint8_t dtcStatusMask,
    Dcm_ResponseType *response
);

/**
 * @brief Process reportDTCSnapshotRecordByDTCNumber subfunction
 */
static Dcm_ReturnType processReportDTCSnapshotRecordByDTC(
    uint32_t dtc,
    uint8_t recordNumber,
    Dcm_ResponseType *response
);

/**
 * @brief Process reportDTCExtDataRecordByDTCNumber subfunction
 */
static Dcm_ReturnType processReportDTCExtDataRecordByDTC(
    uint32_t dtc,
    uint8_t recordNumber,
    Dcm_ResponseType *response
);

/**
 * @brief Process reportSupportedDTC subfunction
 */
static Dcm_ReturnType processReportSupportedDTC(
    Dcm_ResponseType *response
);

/**
 * @brief Process reportFirstTestFailedDTC subfunction
 */
static Dcm_ReturnType processReportFirstTestFailedDTC(
    Dcm_ResponseType *response
);

/**
 * @brief Process reportFirstConfirmedDTC subfunction
 */
static Dcm_ReturnType processReportFirstConfirmedDTC(
    Dcm_ResponseType *response
);

/**
 * @brief Process reportMostRecentTestFailedDTC subfunction
 */
static Dcm_ReturnType processReportMostRecentTestFailedDTC(
    Dcm_ResponseType *response
);

/**
 * @brief Process reportMostRecentConfirmedDTC subfunction
 */
static Dcm_ReturnType processReportMostRecentConfirmedDTC(
    Dcm_ResponseType *response
);

/**
 * @brief Process reportDTCFaultDetectionCounter subfunction
 */
static Dcm_ReturnType processReportDTCFaultDetectionCounter(
    Dcm_ResponseType *response
);

/**
 * @brief Build positive response for ControlDTCSetting
 */
static Dcm_ReturnType buildControlDTCSettingPositiveResponse(
    uint8_t dtcSettingType,
    Dcm_ResponseType *response
);

/**
 * @brief Build positive response for ClearDiagnosticInformation
 */
static Dcm_ReturnType buildClearDTCPositiveResponse(
    Dcm_ResponseType *response
);

/**
 * @brief Build negative response
 */
static Dcm_ReturnType buildNegativeResponse(
    uint8_t sid,
    uint8_t nrc,
    Dcm_ResponseType *response
);

#ifdef __cplusplus
}
#endif

#endif /* DCM_DEM_INTEGRATION_H */
