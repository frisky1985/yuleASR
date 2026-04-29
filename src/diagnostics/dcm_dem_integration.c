/**
 * @file dcm_dem_integration.c
 * @brief DCM-DEM Integration Layer Implementation
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

#include "dcm_dem_integration.h"
#include <string.h>
#include "dcm/dcm_security.h"

/******************************************************************************
 * Internal Macros
 ******************************************************************************/
#define DCM_DEM_MAGIC_INIT                      (0x44454D30U)  /* "DEM0" */
#define DCM_DTC_HIGH_BYTE_MASK                  0xFF0000U
#define DCM_DTC_MID_BYTE_MASK                   0x00FF00U
#define DCM_DTC_LOW_BYTE_MASK                   0x0000FFU

/* Required security level for ControlDTCSetting (configurable) */
#define DCM_DTC_SETTING_REQUIRED_SECURITY_LEVEL 0x01U

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;
    const Dcm_DemIntegrationConfigType *config;
    Dcm_DemDtcStatusChangedCallbackType dtcStatusCallback;
    Dcm_DemEventStatusCallbackType eventStatusCallback;
    boolean initialized;
} Dcm_DemIntegrationStateType;

static Dcm_DemIntegrationStateType s_integrationState;

/******************************************************************************
 * Static Helper Functions
 ******************************************************************************/

/**
 * @brief Build negative response
 */
static Dcm_ReturnType buildNegativeResponse(
    uint8_t sid,
    uint8_t nrc,
    Dcm_ResponseType *response)
{
    if ((response != NULL) && (response->data != NULL) &&
        (response->maxLength >= 3U)) {
        response->data[0U] = DCM_SID_NEGATIVE_RESPONSE;
        response->data[1U] = sid;
        response->data[2U] = nrc;
        response->length = 3U;
        response->isNegativeResponse = true;
        response->negativeResponseCode = nrc;
        return DCM_E_OK;
    }
    return DCM_E_NOT_OK;
}

/**
 * @brief Validate DTC format and range
 */
static boolean isValidDTC(uint32_t dtc)
{
    /* DTC must not be 0x000000 or 0xFFFFFF (reserved) */
    if ((dtc == 0x000000U) || (dtc == 0xFFFFFFU)) {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Extract 3-byte DTC from request data
 */
static uint32_t extractDTC(const uint8_t *data, uint32_t offset)
{
    return (((uint32_t)data[offset]) << 16) |
           (((uint32_t)data[offset + 1U]) << 8) |
           ((uint32_t)data[offset + 2U]);
}

/**
 * @brief Process reportNumberOfDTCByStatusMask subfunction
 */
static Dcm_ReturnType processReportNumberOfDTCByStatusMask(
    uint8_t dtcStatusMask,
    Dcm_ResponseType *response)
{
    Std_ReturnType demResult;
    uint16_t numberOfFilteredDTC = 0U;
    uint8_t availabilityMask;

    /* Set DTC filter with provided status mask */
    demResult = Dem_SetDTCFilter(
        dtcStatusMask,
        DEM_DTC_KIND_ALL_DTCS,
        DEM_DTC_FORMAT_UDS,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY
    );

    if (demResult != E_OK) {
        return buildNegativeResponse(
            UDS_SVC_READ_DTC_INFORMATION,
            UDS_NRC_CONDITIONS_NOT_CORRECT,
            response
        );
    }

    /* Get number of filtered DTCs */
    demResult = Dem_GetNumberOfFilteredDTC(&numberOfFilteredDTC);

    if (demResult != E_OK) {
        return buildNegativeResponse(
            UDS_SVC_READ_DTC_INFORMATION,
            UDS_NRC_CONDITIONS_NOT_CORRECT,
            response
        );
    }

    /* Get availability mask */
    availabilityMask = Dem_GetDTCStatusAvailabilityMask();

    /* Build positive response:
     * Byte 0: serviceId + 0x40
     * Byte 1: sub-function = 0x01
     * Byte 2: DTCFormatIdentifier
     * Byte 3: DTCAvailabilityMask
     * Byte 4-5: DTCCount (high byte, low byte)
     */
    if (response->maxLength < 6U) {
        return buildNegativeResponse(
            UDS_SVC_READ_DTC_INFORMATION,
            UDS_NRC_RESPONSE_TOO_LONG,
            response
        );
    }

    response->data[0U] = UDS_SVC_READ_DTC_INFORMATION + DCM_SID_POSITIVE_RESPONSE_OFFSET;
    response->data[1U] = DCM_SUBFUNC_REPORT_NUMBER_OF_DTC_BY_STATUS_MASK;
    response->data[2U] = DCM_DTC_FORMAT_ISO_14229_1;
    response->data[3U] = availabilityMask;
    response->data[4U] = (uint8_t)((numberOfFilteredDTC >> 8) & 0xFFU);
    response->data[5U] = (uint8_t)(numberOfFilteredDTC & 0xFFU);
    response->length = 6U;
    response->isNegativeResponse = false;

    return DCM_E_OK;
}

/**
 * @brief Process reportDTCByStatusMask subfunction
 */
static Dcm_ReturnType processReportDTCByStatusMask(
    uint8_t dtcStatusMask,
    Dcm_ResponseType *response)
{
    Std_ReturnType demResult;
    uint32_t dtc;
    Dem_UdsStatusByteType dtcStatus;
    uint8_t availabilityMask;
    uint32_t responseIndex = 4U;  /* Start after header */
    uint16_t dtcCount = 0U;

    /* Set DTC filter with provided status mask */
    demResult = Dem_SetDTCFilter(
        dtcStatusMask,
        DEM_DTC_KIND_ALL_DTCS,
        DEM_DTC_FORMAT_UDS,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY
    );

    if (demResult != E_OK) {
        return buildNegativeResponse(
            UDS_SVC_READ_DTC_INFORMATION,
            UDS_NRC_CONDITIONS_NOT_CORRECT,
            response
        );
    }

    /* Get availability mask */
    availabilityMask = Dem_GetDTCStatusAvailabilityMask();

    /* Build response header:
     * Byte 0: serviceId + 0x40
     * Byte 1: sub-function = 0x02
     * Byte 2: DTCFormatIdentifier
     * Byte 3: DTCAvailabilityMask
     */
    if (response->maxLength < 4U) {
        return buildNegativeResponse(
            UDS_SVC_READ_DTC_INFORMATION,
            UDS_NRC_RESPONSE_TOO_LONG,
            response
        );
    }

    response->data[0U] = UDS_SVC_READ_DTC_INFORMATION + DCM_SID_POSITIVE_RESPONSE_OFFSET;
    response->data[1U] = DCM_SUBFUNC_REPORT_DTC_BY_STATUS_MASK;
    response->data[2U] = DCM_DTC_FORMAT_ISO_14229_1;
    response->data[3U] = availabilityMask;

    /* Iterate through filtered DTCs */
    while (responseIndex + 4U <= response->maxLength) {
        demResult = Dem_GetNextFilteredDTC(&dtc, &dtcStatus);

        if (demResult != E_OK) {
            /* No more DTCs */
            break;
        }

        /* Add DTC to response: DTC (3 bytes) + status (1 byte) */
        response->data[responseIndex++] = (uint8_t)((dtc >> 16) & 0xFFU);
        response->data[responseIndex++] = (uint8_t)((dtc >> 8) & 0xFFU);
        response->data[responseIndex++] = (uint8_t)(dtc & 0xFFU);
        response->data[responseIndex++] = (uint8_t)dtcStatus;
        dtcCount++;
    }

    response->length = responseIndex;
    response->isNegativeResponse = false;

    return DCM_E_OK;
}

/**
 * @brief Process reportSupportedDTC subfunction
 */
static Dcm_ReturnType processReportSupportedDTC(
    Dcm_ResponseType *response)
{
    Std_ReturnType demResult;
    uint32_t dtc;
    Dem_UdsStatusByteType dtcStatus;
    uint8_t availabilityMask;
    uint32_t responseIndex = 4U;  /* Start after header */

    /* Set DTC filter to get all supported DTCs (status mask = 0x00) */
    demResult = Dem_SetDTCFilter(
        0x00U,  /* All DTCs regardless of status */
        DEM_DTC_KIND_ALL_DTCS,
        DEM_DTC_FORMAT_UDS,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY
    );

    if (demResult != E_OK) {
        return buildNegativeResponse(
            UDS_SVC_READ_DTC_INFORMATION,
            UDS_NRC_CONDITIONS_NOT_CORRECT,
            response
        );
    }

    /* Get availability mask */
    availabilityMask = Dem_GetDTCStatusAvailabilityMask();

    /* Build response header:
     * Byte 0: serviceId + 0x40
     * Byte 1: sub-function = 0x0A
     * Byte 2: DTCFormatIdentifier
     * Byte 3: DTCAvailabilityMask
     */
    if (response->maxLength < 4U) {
        return buildNegativeResponse(
            UDS_SVC_READ_DTC_INFORMATION,
            UDS_NRC_RESPONSE_TOO_LONG,
            response
        );
    }

    response->data[0U] = UDS_SVC_READ_DTC_INFORMATION + DCM_SID_POSITIVE_RESPONSE_OFFSET;
    response->data[1U] = DCM_SUBFUNC_REPORT_SUPPORTED_DTC;
    response->data[2U] = DCM_DTC_FORMAT_ISO_14229_1;
    response->data[3U] = availabilityMask;

    /* Iterate through all supported DTCs */
    while (responseIndex + 4U <= response->maxLength) {
        demResult = Dem_GetNextFilteredDTC(&dtc, &dtcStatus);

        if (demResult != E_OK) {
            /* No more DTCs */
            break;
        }

        /* Add DTC to response: DTC (3 bytes) + status (1 byte) */
        response->data[responseIndex++] = (uint8_t)((dtc >> 16) & 0xFFU);
        response->data[responseIndex++] = (uint8_t)((dtc >> 8) & 0xFFU);
        response->data[responseIndex++] = (uint8_t)(dtc & 0xFFU);
        response->data[responseIndex++] = (uint8_t)dtcStatus;
    }

    response->length = responseIndex;
    response->isNegativeResponse = false;

    return DCM_E_OK;
}

/**
 * @brief Process reportDTCFaultDetectionCounter subfunction
 */
static Dcm_ReturnType processReportDTCFaultDetectionCounter(
    Dcm_ResponseType *response)
{
    /* Simplified implementation - would iterate through events
     * and return fault detection counters
     */

    /* Build response header:
     * Byte 0: serviceId + 0x40
     * Byte 1: sub-function = 0x14
     */
    if (response->maxLength < 2U) {
        return buildNegativeResponse(
            UDS_SVC_READ_DTC_INFORMATION,
            UDS_NRC_RESPONSE_TOO_LONG,
            response
        );
    }

    response->data[0U] = UDS_SVC_READ_DTC_INFORMATION + DCM_SID_POSITIVE_RESPONSE_OFFSET;
    response->data[1U] = DCM_SUBFUNC_REPORT_DTC_FAULT_DETECTION_COUNTER;
    response->length = 2U;
    response->isNegativeResponse = false;

    return DCM_E_OK;
}

/**
 * @brief Build positive response for ControlDTCSetting
 * @param dtcSettingType The DTC setting type (on/off)
 * @param response Response buffer
 * @return Dcm_ReturnType Result
 */
static Dcm_ReturnType buildControlDTCSettingPositiveResponse(
    uint8_t dtcSettingType,
    Dcm_ResponseType *response)
{
    if (response->maxLength < 2U) {
        return buildNegativeResponse(
            UDS_SVC_CONTROL_DTC_SETTING,
            UDS_NRC_RESPONSE_TOO_LONG,
            response
        );
    }

    /* Build positive response:
     * Byte 0: serviceId + 0x40 (0xC5)
     * Byte 1: DTCSettingType (sub-function without SPRMIB)
     */
    response->data[0U] = UDS_SVC_CONTROL_DTC_SETTING + DCM_SID_POSITIVE_RESPONSE_OFFSET;
    response->data[1U] = dtcSettingType & DCM_SUBFUNCTION_MASK;
    response->length = 2U;
    response->isNegativeResponse = false;

    return DCM_E_OK;
}

/**
 * @brief Build positive response for ClearDiagnosticInformation
 */
static Dcm_ReturnType buildClearDTCPositiveResponse(
    Dcm_ResponseType *response)
{
    if (response->maxLength < 1U) {
        return buildNegativeResponse(
            UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION,
            UDS_NRC_RESPONSE_TOO_LONG,
            response
        );
    }

    response->data[0U] = UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION + DCM_SID_POSITIVE_RESPONSE_OFFSET;
    response->length = 1U;
    response->isNegativeResponse = false;

    return DCM_E_OK;
}

/******************************************************************************
 * Public Functions - Initialization
 ******************************************************************************/

Dcm_ReturnType Dcm_DemIntegration_Init(
    const Dcm_DemIntegrationConfigType *config)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;

    if (config != NULL) {
        /* Clear state */
        (void)memset(&s_integrationState, 0, sizeof(s_integrationState));

        s_integrationState.magic = DCM_DEM_MAGIC_INIT;
        s_integrationState.config = config;
        s_integrationState.dtcStatusCallback = NULL;
        s_integrationState.eventStatusCallback = NULL;
        s_integrationState.initialized = TRUE;

        result = DCM_E_OK;
    }

    return result;
}

Dcm_ReturnType Dcm_DemIntegration_DeInit(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;

    if (s_integrationState.initialized) {
        s_integrationState.initialized = FALSE;
        s_integrationState.magic = 0U;
        s_integrationState.config = NULL;
        s_integrationState.dtcStatusCallback = NULL;
        s_integrationState.eventStatusCallback = NULL;

        result = DCM_E_OK;
    }

    return result;
}

/******************************************************************************
 * Public Functions - Service Handlers
 ******************************************************************************/

Dcm_ReturnType Dcm_DemIntegration_ControlDTCSetting(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t dtcSettingType;
    uint8_t suppressPosResponseBit;
    uint32_t dtcGroup = DCM_DTC_GROUP_ALL;  /* Default: all DTCs */
    uint8_t clientId = 0U;                   /* Default client ID */
    Std_ReturnType demResult;
    boolean hasDTCGroup = FALSE;

    /* Check initialization */
    if (!s_integrationState.initialized) {
        return buildNegativeResponse(
            UDS_SVC_CONTROL_DTC_SETTING,
            UDS_NRC_CONDITIONS_NOT_CORRECT,
            response
        );
    }

    /* Validate parameters */
    if ((request == NULL) || (response == NULL) ||
        (request->data == NULL) || (response->data == NULL)) {
        return DCM_E_NOT_OK;
    }

    /* Check minimum request length: SID (1) + sub-function (1) = 2 bytes */
    if (request->length < 2U) {
        return buildNegativeResponse(
            UDS_SVC_CONTROL_DTC_SETTING,
            UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT,
            response
        );
    }

    /* Extract sub-function (DTCSettingType) and SPRMIB */
    dtcSettingType = request->data[1U] & DCM_SUBFUNCTION_MASK;
    suppressPosResponseBit = request->data[1U] & DCM_SUPPRESS_POS_RESPONSE_MASK;

    /* Set SPRMIB flag in response if requested */
    if (suppressPosResponseBit != 0U) {
        response->suppressPositiveResponse = true;
    }

    /* Validate DTCSettingType */
    if ((dtcSettingType != DCM_SUBFUNC_DTC_SETTING_ON) &&
        (dtcSettingType != DCM_SUBFUNC_DTC_SETTING_OFF)) {
        return buildNegativeResponse(
            UDS_SVC_CONTROL_DTC_SETTING,
            UDS_NRC_SUBFUNCTION_NOT_SUPPORTED,
            response
        );
    }

    /* Check security access - required security level */
    uint8_t currentSecurityLevel = Dcm_GetSecurityLevel();
    if (currentSecurityLevel < DCM_DTC_SETTING_REQUIRED_SECURITY_LEVEL) {
        return buildNegativeResponse(
            UDS_SVC_CONTROL_DTC_SETTING,
            UDS_NRC_SECURITY_ACCESS_DENIED,
            response
        );
    }

    /* Check for DTCSettingControlOptionRecord (optional DTC group filtering) */
    /* ISO 14229-1: Request = SID + SF + [DTCGroup (3 bytes)] */
    if (request->length >= 5U) {
        /* DTCGroup is present (3 bytes: high, mid, low) */
        dtcGroup = (((uint32_t)request->data[2U]) << 16) |
                   (((uint32_t)request->data[3U]) << 8) |
                   ((uint32_t)request->data[4U]);
        hasDTCGroup = TRUE;
    }

    /* Validate DTCGroup if provided */
    if (hasDTCGroup) {
        /* Check for valid DTC group values */
        /* 0x000000 - Emission related */
        /* 0x010000 - Powertrain */
        /* 0x020000 - Chassis */
        /* 0x030000 - Body */
        /* 0x040000 - Network */
        /* 0xFFFFFF - All DTCs */
        boolean validGroup = FALSE;
        
        if ((dtcGroup == DCM_DTC_GROUP_ALL) ||
            (dtcGroup == DCM_DTC_GROUP_EMISSION) ||
            (dtcGroup == DCM_DTC_GROUP_POWERTRAIN) ||
            (dtcGroup == DCM_DTC_GROUP_CHASSIS) ||
            (dtcGroup == DCM_DTC_GROUP_BODY) ||
            (dtcGroup == DCM_DTC_GROUP_NETWORK)) {
            validGroup = TRUE;
        }

        if (!validGroup) {
            return buildNegativeResponse(
                UDS_SVC_CONTROL_DTC_SETTING,
                UDS_NRC_REQUEST_OUT_OF_RANGE,
                response
            );
        }
    }

    /* Execute the DTC setting control */
    if (dtcSettingType == DCM_SUBFUNC_DTC_SETTING_ON) {
        /* Enable DTC setting */
        demResult = Dem_EnableDTCSetting(dtcGroup, clientId);
    } else {
        /* Disable DTC setting */
        demResult = Dem_DisableDTCSetting(dtcGroup, clientId);
    }

    /* Convert DEM result to appropriate response */
    switch (demResult) {
        case E_OK:
            /* DTC setting controlled successfully */
            result = buildControlDTCSettingPositiveResponse(dtcSettingType, response);
            break;

        case E_NOT_OK:
        default:
            /* Control operation failed - conditions not correct */
            result = buildNegativeResponse(
                UDS_SVC_CONTROL_DTC_SETTING,
                UDS_NRC_CONDITIONS_NOT_CORRECT,
                response
            );
            break;
    }

    return result;
}

Dcm_ReturnType Dcm_DemIntegration_ClearDiagnosticInformation(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint32_t dtc;
    Std_ReturnType demResult;

    /* Check initialization */
    if (!s_integrationState.initialized) {
        return buildNegativeResponse(
            UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION,
            UDS_NRC_CONDITIONS_NOT_CORRECT,
            response
        );
    }

    /* Validate parameters */
    if ((request == NULL) || (response == NULL) ||
        (request->data == NULL) || (response->data == NULL)) {
        return DCM_E_NOT_OK;
    }

    /* Check minimum request length: SID (1) + DTC (3) = 4 bytes */
    if (request->length < 4U) {
        return buildNegativeResponse(
            UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION,
            UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT,
            response
        );
    }

    /* Extract DTC from request (3 bytes: high, mid, low) */
    dtc = extractDTC(request->data, 1U);

    /* Validate DTC */
    if (!isValidDTC(dtc)) {
        return buildNegativeResponse(
            UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION,
            UDS_NRC_REQUEST_OUT_OF_RANGE,
            response
        );
    }

    /* Call DEM to clear DTC */
    demResult = Dem_ClearDTC(
        dtc,
        DEM_DTC_FORMAT_UDS,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY
    );

    /* Convert DEM result to appropriate response */
    switch (demResult) {
        case E_OK:
            /* DTC cleared successfully or no matching DTC found */
            result = buildClearDTCPositiveResponse(response);
            break;

        case E_NOT_OK:
        default:
            /* Clear operation failed */
            result = buildNegativeResponse(
                UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION,
                UDS_NRC_CONDITIONS_NOT_CORRECT,
                response
            );
            break;
    }

    return result;
}

Dcm_ReturnType Dcm_DemIntegration_ReadDTCInformation(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t subFunction;
    uint8_t suppressPosResponseBit;

    /* Check initialization */
    if (!s_integrationState.initialized) {
        return buildNegativeResponse(
            UDS_SVC_READ_DTC_INFORMATION,
            UDS_NRC_CONDITIONS_NOT_CORRECT,
            response
        );
    }

    /* Validate parameters */
    if ((request == NULL) || (response == NULL) ||
        (request->data == NULL) || (response->data == NULL)) {
        return DCM_E_NOT_OK;
    }

    /* Check minimum request length: SID (1) + sub-function (1) = 2 bytes */
    if (request->length < 2U) {
        return buildNegativeResponse(
            UDS_SVC_READ_DTC_INFORMATION,
            UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT,
            response
        );
    }

    /* Extract sub-function and SPRMB */
    subFunction = request->data[1U] & DCM_SUBFUNCTION_MASK;
    suppressPosResponseBit = request->data[1U] & DCM_SUPPRESS_POS_RESPONSE_MASK;

    /* Set SPRMIB flag in response if requested */
    if (suppressPosResponseBit != 0U) {
        response->suppressPositiveResponse = true;
    }

    /* Route to appropriate subfunction handler */
    switch (subFunction) {
        case DCM_SUBFUNC_REPORT_NUMBER_OF_DTC_BY_STATUS_MASK:
            /* Request: SID + SF + DTCStatusMask */
            if (request->length < 3U) {
                return buildNegativeResponse(
                    UDS_SVC_READ_DTC_INFORMATION,
                    UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT,
                    response
                );
            }
            result = processReportNumberOfDTCByStatusMask(
                request->data[2U],
                response
            );
            break;

        case DCM_SUBFUNC_REPORT_DTC_BY_STATUS_MASK:
            /* Request: SID + SF + DTCStatusMask */
            if (request->length < 3U) {
                return buildNegativeResponse(
                    UDS_SVC_READ_DTC_INFORMATION,
                    UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT,
                    response
                );
            }
            result = processReportDTCByStatusMask(
                request->data[2U],
                response
            );
            break;

        case DCM_SUBFUNC_REPORT_SUPPORTED_DTC:
            /* Request: SID + SF */
            result = processReportSupportedDTC(response);
            break;

        case DCM_SUBFUNC_REPORT_DTC_FAULT_DETECTION_COUNTER:
            /* Request: SID + SF */
            result = processReportDTCFaultDetectionCounter(response);
            break;

        case DCM_SUBFUNC_REPORT_FIRST_TEST_FAILED_DTC:
        case DCM_SUBFUNC_REPORT_FIRST_CONFIRMED_DTC:
        case DCM_SUBFUNC_REPORT_MOST_RECENT_TEST_FAILED_DTC:
        case DCM_SUBFUNC_REPORT_MOST_RECENT_CONFIRMED_DTC:
            /* These subfunctions are not fully implemented yet */
            return buildNegativeResponse(
                UDS_SVC_READ_DTC_INFORMATION,
                UDS_NRC_SUBFUNCTION_NOT_SUPPORTED,
                response
            );

        case DCM_SUBFUNC_REPORT_DTC_SNAPSHOT_RECORD_BY_DTC:
        case DCM_SUBFUNC_REPORT_DTC_EXT_DATA_RECORD_BY_DTC:
            /* These subfunctions require DTC parameter */
            return buildNegativeResponse(
                UDS_SVC_READ_DTC_INFORMATION,
                UDS_NRC_SUBFUNCTION_NOT_SUPPORTED,
                response
            );

        default:
            /* Subfunction not supported */
            result = buildNegativeResponse(
                UDS_SVC_READ_DTC_INFORMATION,
                UDS_NRC_SUBFUNCTION_NOT_SUPPORTED,
                response
            );
            break;
    }

    return result;
}

/******************************************************************************
 * Public Functions - Callback Registration
 ******************************************************************************/

Dcm_ReturnType Dcm_DemIntegration_RegisterDtcStatusCallback(
    Dcm_DemDtcStatusChangedCallbackType callback)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;

    if (s_integrationState.initialized && (callback != NULL)) {
        s_integrationState.dtcStatusCallback = callback;
        result = DCM_E_OK;
    }

    return result;
}

Dcm_ReturnType Dcm_DemIntegration_RegisterEventStatusCallback(
    Dcm_DemEventStatusCallbackType callback)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;

    if (s_integrationState.initialized && (callback != NULL)) {
        s_integrationState.eventStatusCallback = callback;
        result = DCM_E_OK;
    }

    return result;
}

/******************************************************************************
 * Public Functions - DEM Notifications
 ******************************************************************************/

void Dcm_DemIntegration_NotifyDtcStatusChanged(
    uint32_t dtc,
    Dem_UdsStatusByteType statusOld,
    Dem_UdsStatusByteType statusNew)
{
    /* Notify registered callback if available */
    if ((s_integrationState.initialized) &&
        (s_integrationState.dtcStatusCallback != NULL)) {
        s_integrationState.dtcStatusCallback(
            dtc,
            (uint8_t)statusOld,
            (uint8_t)statusNew
        );
    }
}

void Dcm_DemIntegration_NotifyEventStatusChanged(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatusOld,
    Dem_EventStatusType EventStatusNew)
{
    /* Notify registered callback if available */
    if ((s_integrationState.initialized) &&
        (s_integrationState.eventStatusCallback != NULL)) {
        s_integrationState.eventStatusCallback(
            EventId,
            EventStatusOld,
            EventStatusNew
        );
    }
}

/******************************************************************************
 * Public Functions - Utility
 ******************************************************************************/

bool Dcm_DemIntegration_IsInitialized(void)
{
    return s_integrationState.initialized;
}

Dcm_ReturnType Dcm_DemIntegration_ConvertReturnType(
    Std_ReturnType demReturn)
{
    Dcm_ReturnType dcmReturn;

    switch (demReturn) {
        case E_OK:
            dcmReturn = DCM_E_OK;
            break;
        case E_NOT_OK:
        default:
            dcmReturn = DCM_E_NOT_OK;
            break;
    }

    return dcmReturn;
}

uint8_t Dcm_DemIntegration_GetDTCStatusAvailabilityMask(void)
{
    if (s_integrationState.initialized) {
        return Dem_GetDTCStatusAvailabilityMask();
    }
    return 0x00U;
}

uint8_t Dcm_DemIntegration_GetDTCFormatIdentifier(void)
{
    return DCM_DTC_FORMAT_ISO_14229_1;
}
