/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Swc_DiagnosticManager.c
 * @brief Diagnostic Manager Software Component Implementation
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Diagnostic services and DTC management at application layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Swc_DiagnosticManager.h"
#include "Rte.h"
#include "Det.h"
#include <string.h>

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
/* Service IDs (UDS) */
#define SID_DIAGNOSTIC_SESSION_CONTROL      0x10
#define SID_ECU_RESET                       0x11
#define SID_SECURITY_ACCESS                 0x27
#define SID_COMMUNICATION_CONTROL           0x28
#define SID_TESTER_PRESENT                  0x3E
#define SID_CONTROL_DTC_SETTING             0x85
#define SID_READ_DTC_INFORMATION            0x19
#define SID_CLEAR_DIAGNOSTIC_INFORMATION    0x14
#define SID_READ_DATA_BY_IDENTIFIER         0x22
#define SID_WRITE_DATA_BY_IDENTIFIER        0x2E

/* UDS protocol constants */
#define UDS_POSITIVE_RESPONSE_OFFSET        (0x40U)  /* SID + 0x40 = positive response */
#define UDS_NEGATIVE_RESPONSE_SID           (0x7FU)  /* Negative response service ID */
#define UDS_NRC_INCORRECT_MESSAGE_LENGTH    (0x13U)  /* Incorrect message length */
#define UDS_NRC_SUBFUNCTION_NOT_SUPPORTED   (0x12U)  /* Sub-function not supported */
#define UDS_NRC_REQUEST_OUT_OF_RANGE        (0x31U)  /* Request out of range */
#define UDS_NRC_SECURITY_ACCESS_DENIED      (0x33U)  /* Security access denied */
#define UDS_NRC_CONDITIONS_NOT_CORRECT      (0x22U)  /* Conditions not correct */
#define UDS_NRC_INVALID_KEY                 (0x35U)  /* Invalid key */
#define UDS_NRC_SERVICE_NOT_SUPPORTED       (0x11U)  /* Service not supported */

/* Security access sub-function masks */
#define UDS_SECURITY_SUBFUNC_SEED_MASK      (0x01U)  /* LSB indicates request seed (1) or send key (0) */
#define UDS_SECURITY_SUBFUNC_SEED_FLAG      (0x01U)  /* Request seed flag */

/* Bit masks */
#define UDS_MASK_LOW_BYTE                   (0xFFU)  /* Low byte mask */
#define UDS_MASK_HIGH_BYTE                  (0xFF00U) /* High byte mask */

/* Clear all DTCs sentinel */
#define UDS_CLEAR_ALL_DTCS                  (0xFFFFFFU)

/* DTC sub-function constants */
#define UDS_DTC_SUBFUNC_REPORT_BY_STATUS    (0x02U)  /* Report DTC by status mask */
#define UDS_DTC_SUBFUNC_EXTENDED_DATA       (0x06U)  /* Report DTC extended data record */
#define UDS_AVAILABILITY_MASK_VAL           (0x02U)  /* Availability mask byte */
#define UDS_EXTENDED_DATA_RECORD_VAL        (0x06U)  /* Extended data record marker */

/* P2/P2* timing defaults (in ms) */
#define UDS_P2_DEFAULT_HIGH                 (0x00U)  /* P2 high byte */
#define UDS_P2_DEFAULT_LOW                  (0x32U)  /* P2 low byte (50ms) */
#define UDS_P2STAR_DEFAULT_HIGH             (0x01U)  /* P2* high byte */
#define UDS_P2STAR_DEFAULT_LOW              (0xF4U)  /* P2* low byte (500ms) */

/* Security key size */
#define DIAG_SECURITY_KEY_SIZE              (4U)

/* Default session */
#define DIAG_SESSION_DEFAULT_VALUE          ((Swc_DiagnosticSessionType)0x01)
#define DIAG_SESSION_EXTENDED_VALUE         ((Swc_DiagnosticSessionType)0x03)
#define DIAG_SESSION_PROGRAMMING_VALUE      ((Swc_DiagnosticSessionType)0x02)
#define DIAG_SESSION_SAFETY_VALUE           ((Swc_DiagnosticSessionType)0x04)



/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
typedef struct {
    Swc_DiagnosticManagerStatusType status;
    Swc_DtcStatusType dtcList[SWC_DIAG_MAX_DTCS];
    uint8 numDtcs;
    Swc_DiagnosticRequestType pendingRequest;
    boolean hasPendingRequest;
    uint32 lastTesterPresentTime;
    boolean dtcSettingEnabled;
} Swc_DiagnosticManagerInternalType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define RTE_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC Swc_DiagnosticManagerInternalType swcDiagManager = {
    .status = {
        .currentSession = DIAG_SESSION_DEFAULT,
        .securityLevel = SECURITY_LOCKED,
        .activeProtocol = 0,
        .communicationEnabled = TRUE,
        .sessionTimeout = SWC_DIAG_SESSION_TIMEOUT_DEFAULT,
        .securityTimeout = 0
    },
    .dtcList = {{0}},
    .numDtcs = 0,
    .pendingRequest = {0},
    .hasPendingRequest = FALSE,
    .lastTesterPresentTime = 0,
    .dtcSettingEnabled = TRUE
};

/* Security keys (simplified - in production use proper crypto) */
STATIC const uint8 securityKeyLevel1[4] = {0x01, 0x02, 0x03, 0x04};
STATIC const uint8 securityKeyLevel2[4] = {0x05, 0x06, 0x07, 0x08};

#define RTE_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC void Swc_DiagnosticManager_UpdateTimeouts(void);
STATIC void Swc_DiagnosticManager_ProcessSessionControl(const Swc_DiagnosticRequestType* request,
                                                         Swc_DiagnosticResponseType* response);
STATIC void Swc_DiagnosticManager_ProcessSecurityAccess(const Swc_DiagnosticRequestType* request,
                                                         Swc_DiagnosticResponseType* response);
STATIC void Swc_DiagnosticManager_ProcessReadDtc(const Swc_DiagnosticRequestType* request,
                                                  Swc_DiagnosticResponseType* response);
STATIC void Swc_DiagnosticManager_ProcessClearDtc(const Swc_DiagnosticRequestType* request,
                                                   Swc_DiagnosticResponseType* response);
STATIC void Swc_DiagnosticManager_ProcessTesterPresent(const Swc_DiagnosticRequestType* request,
                                                        Swc_DiagnosticResponseType* response);
STATIC boolean Swc_DiagnosticManager_ValidateKey(Swc_SecurityLevelType level, const uint8* key);
STATIC boolean Swc_DiagnosticManager_CheckSecurityAccess(uint8 serviceId);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Updates session and security timeouts
 */
STATIC void Swc_DiagnosticManager_UpdateTimeouts(void)
{
    uint32 currentTime = Rte_GetTime();

    /* Check session timeout */
    if (swcDiagManager.status.currentSession != DIAG_SESSION_DEFAULT) {
        if ((currentTime - swcDiagManager.lastTesterPresentTime) >
            swcDiagManager.status.sessionTimeout) {
            /* Return to default session */
            swcDiagManager.status.currentSession = DIAG_SESSION_DEFAULT;
            swcDiagManager.status.sessionTimeout = SWC_DIAG_SESSION_TIMEOUT_DEFAULT;
        }
    }

    /* Check security timeout */
    if (swcDiagManager.status.securityLevel != SECURITY_LOCKED) {
        if ((currentTime - swcDiagManager.lastTesterPresentTime) > SWC_DIAG_SECURITY_TIMEOUT) {
            /* Lock security */
            swcDiagManager.status.securityLevel = SECURITY_LOCKED;
            swcDiagManager.status.securityTimeout = 0;
        }
    }
}

/**
 * @brief Processes diagnostic session control service
 */
STATIC void Swc_DiagnosticManager_ProcessSessionControl(const Swc_DiagnosticRequestType* request,
                                                         Swc_DiagnosticResponseType* response)
{
    Swc_DiagnosticSessionType newSession;

    if (request->dataLength < 1U) {
        response->negativeResponseCode = UDS_NRC_INCORRECT_MESSAGE_LENGTH;
        return;
    }

    newSession = (Swc_DiagnosticSessionType)request->data[0];

    switch (newSession) {
        case DIAG_SESSION_DEFAULT:
            swcDiagManager.status.currentSession = DIAG_SESSION_DEFAULT;
            swcDiagManager.status.sessionTimeout = SWC_DIAG_SESSION_TIMEOUT_DEFAULT;
            break;

        case DIAG_SESSION_EXTENDED:
            swcDiagManager.status.currentSession = DIAG_SESSION_EXTENDED;
            swcDiagManager.status.sessionTimeout = SWC_DIAG_SESSION_TIMEOUT_EXTENDED;
            break;

        case DIAG_SESSION_PROGRAMMING:
            /* Requires security level 2 or 3 */
            if (swcDiagManager.status.securityLevel < SECURITY_LEVEL_2) {
                response->negativeResponseCode = UDS_NRC_REQUEST_OUT_OF_RANGE;
                return;
            }
            swcDiagManager.status.currentSession = DIAG_SESSION_PROGRAMMING;
            swcDiagManager.status.sessionTimeout = SWC_DIAG_SESSION_TIMEOUT_PROGRAMMING;
            break;

        case DIAG_SESSION_SAFETY_SYSTEM:
            /* Requires security level 3 */
            if (swcDiagManager.status.securityLevel < SECURITY_LEVEL_3) {
                response->negativeResponseCode = UDS_NRC_REQUEST_OUT_OF_RANGE;
                return;
            }
            swcDiagManager.status.currentSession = DIAG_SESSION_SAFETY_SYSTEM;
            break;

        default:
            response->negativeResponseCode = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
            return;
    }

    /* Build positive response */
    response->responseId = request->serviceId + UDS_POSITIVE_RESPONSE_OFFSET;
    response->dataLength = 5;
    response->data[0] = (uint8)newSession;
    /* Session timeout (P2) */
    response->data[1] = UDS_P2_DEFAULT_HIGH;
    response->data[2] = UDS_P2_DEFAULT_LOW;  /* 50ms */
    /* Enhanced session timeout (P2*) */
    response->data[3] = UDS_P2STAR_DEFAULT_HIGH;
    response->data[4] = UDS_P2STAR_DEFAULT_LOW;  /* 500ms */
}

/**
 * @brief Processes security access service
 */
STATIC void Swc_DiagnosticManager_ProcessSecurityAccess(const Swc_DiagnosticRequestType* request,
                                                         Swc_DiagnosticResponseType* response)
{
    uint8 subFunction;
    Swc_SecurityLevelType targetLevel;

    if (request->dataLength < 1U) {
        response->negativeResponseCode = UDS_NRC_INCORRECT_MESSAGE_LENGTH;
        return;
    }

    subFunction = request->data[0];

    if ((subFunction & UDS_SECURITY_SUBFUNC_SEED_MASK) == UDS_SECURITY_SUBFUNC_SEED_FLAG) {
        /* Request seed */
        targetLevel = (Swc_SecurityLevelType)((subFunction + 1U) / 2);

        if (targetLevel > SECURITY_LEVEL_3) {
            response->negativeResponseCode = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
            return;
        }

        /* Build positive response with seed */
        response->responseId = request->serviceId + UDS_POSITIVE_RESPONSE_OFFSET;
        response->dataLength = DIAG_SECURITY_KEY_SIZE + 1U;
        response->data[0] = subFunction;
        /* Generate seed (simplified) */
        response->data[1] = (uint8)(Rte_GetTime() & UDS_MASK_LOW_BYTE);
        response->data[2] = (uint8)((Rte_GetTime() >> 8) & UDS_MASK_LOW_BYTE);
        response->data[3] = (uint8)((Rte_GetTime() >> 16) & UDS_MASK_LOW_BYTE);
        response->data[4] = (uint8)((Rte_GetTime() >> 24) & UDS_MASK_LOW_BYTE);

    } else {
        /* Send key */
        targetLevel = (Swc_SecurityLevelType)(subFunction / 2U);

        if (request->dataLength < (DIAG_SECURITY_KEY_SIZE + 1U)) {
            response->negativeResponseCode = UDS_NRC_INCORRECT_MESSAGE_LENGTH;
            return;
        }

        /* Validate key */
        if (Swc_DiagnosticManager_ValidateKey(targetLevel, &request->data[1])) {
            swcDiagManager.status.securityLevel = targetLevel;
            swcDiagManager.status.securityTimeout = SWC_DIAG_SECURITY_TIMEOUT;

            response->responseId = request->serviceId + UDS_POSITIVE_RESPONSE_OFFSET;
            response->dataLength = 1;
            response->data[0] = subFunction;
        } else {
            response->negativeResponseCode = UDS_NRC_SECURITY_ACCESS_DENIED;
        }
    }
}

/**
 * @brief Processes read DTC information service
 */
STATIC void Swc_DiagnosticManager_ProcessReadDtc(const Swc_DiagnosticRequestType* request,
                                                  Swc_DiagnosticResponseType* response)
{
    uint8 subFunction;
    uint8 i;
    uint8 dataIndex;

    if (request->dataLength < 1U) {
        response->negativeResponseCode = UDS_NRC_INCORRECT_MESSAGE_LENGTH;
        return;
    }

    subFunction = request->data[0];

    switch (subFunction) {
        case UDS_DTC_SUBFUNC_REPORT_BY_STATUS:  /* Report DTC by status mask */
            response->responseId = request->serviceId + UDS_POSITIVE_RESPONSE_OFFSET;
            response->dataLength = 2 + (swcDiagManager.numDtcs * 4);
            response->data[0] = UDS_AVAILABILITY_MASK_VAL;
            response->data[1] = swcDiagManager.numDtcs;

            dataIndex = 2;
            for (i = 0; i < swcDiagManager.numDtcs; i++) {
                response->data[dataIndex] = (uint8)(swcDiagManager.dtcList[i].dtcCode >> 16);
                dataIndex++;
                response->data[dataIndex] = (uint8)(swcDiagManager.dtcList[i].dtcCode >> 8);
                dataIndex++;
                response->data[dataIndex] = (uint8)(swcDiagManager.dtcList[i].dtcCode);
                dataIndex++;
                response->data[dataIndex] = swcDiagManager.dtcList[i].statusByte;
                dataIndex++;
            }
            break;

        case UDS_DTC_SUBFUNC_EXTENDED_DATA:  /* Report DTC extended data record */
            /* Simplified - return first DTC extended data */
            if (swcDiagManager.numDtcs > 0U ) {
                response->responseId = request->serviceId + UDS_POSITIVE_RESPONSE_OFFSET;
                response->dataLength = 8;
                response->data[0] = UDS_EXTENDED_DATA_RECORD_VAL;
                response->data[1] = (uint8)(swcDiagManager.dtcList[0].dtcCode >> 16);
                response->data[2] = (uint8)(swcDiagManager.dtcList[0].dtcCode >> 8);
                response->data[3] = (uint8)(swcDiagManager.dtcList[0].dtcCode);
                response->data[4] = swcDiagManager.dtcList[0].statusByte;
                response->data[5] = swcDiagManager.dtcList[0].faultDetectionCounter;
                response->data[6] = swcDiagManager.dtcList[0].occurrenceCounter;
                response->data[7] = (uint8)swcDiagManager.dtcList[0].agingCounter;
            } else {
                response->negativeResponseCode = UDS_NRC_REQUEST_OUT_OF_RANGE;
            }
            break;

        default:
            response->negativeResponseCode = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
            break;
    }
}

/**
 * @brief Processes clear DTC service
 */
STATIC void Swc_DiagnosticManager_ProcessClearDtc(const Swc_DiagnosticRequestType* request,
                                                   Swc_DiagnosticResponseType* response)
{
    uint32 dtcCode;

    if (request->dataLength < 3U) {
        response->negativeResponseCode = UDS_NRC_INCORRECT_MESSAGE_LENGTH;
        return;
    }

    dtcCode = ((uint32)request->data[0] << 16) |
              ((uint32)request->data[1] << 8) |
              (uint32)request->data[2];

    if (Swc_DiagnosticManager_ClearDtc(dtcCode) == RTE_E_OK) {
        response->responseId = request->serviceId + UDS_POSITIVE_RESPONSE_OFFSET;
        response->dataLength = 0;
    } else {
        response->negativeResponseCode = UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
}

/**
 * @brief Processes tester present service
 */
STATIC void Swc_DiagnosticManager_ProcessTesterPresent(const Swc_DiagnosticRequestType* request,
                                                        Swc_DiagnosticResponseType* response)
{
    if (request->dataLength < 1U) {
        response->negativeResponseCode = UDS_NRC_INCORRECT_MESSAGE_LENGTH;
        return;
    }

    /* Update last tester present time */
    swcDiagManager.lastTesterPresentTime = Rte_GetTime();

    /* Build response */
    response->responseId = request->serviceId + UDS_POSITIVE_RESPONSE_OFFSET;
    response->dataLength = 1;
    response->data[0] = request->data[0];  /* Echo sub-function */
}

/**
 * @brief Validates security key
 */
STATIC boolean Swc_DiagnosticManager_ValidateKey(Swc_SecurityLevelType level, const uint8* key)
{
    const uint8* expectedKey;
    uint8 i;

    switch (level) {
        case SECURITY_LEVEL_1:
            expectedKey = securityKeyLevel1;
            break;
        case SECURITY_LEVEL_2:
            expectedKey = securityKeyLevel2;
            break;
        default:
            return FALSE;
    }

    for (i = 0; i < DIAG_SECURITY_KEY_SIZE; i++) {
        if (key[i] != expectedKey[i]) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief Checks if service requires security access
 */
STATIC boolean Swc_DiagnosticManager_CheckSecurityAccess(uint8 serviceId)
{
    /* Services requiring security level 1 or higher */
    switch (serviceId) {
        case SID_WRITE_DATA_BY_IDENTIFIER:
        case SID_CONTROL_DTC_SETTING:
            return (swcDiagManager.status.securityLevel >= SECURITY_LEVEL_1);

        case SID_ECU_RESET:
        case SID_COMMUNICATION_CONTROL:
            return (swcDiagManager.status.securityLevel >= SECURITY_LEVEL_2);

        default:
            return TRUE;
    }
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the Diagnostic Manager component
 */
void Swc_DiagnosticManager_Init(void)
{
    uint8 i;

    if (swcDiagManager.status.currentSession != 0U ) {
        /* Already initialized */
        return;
    }

    /* Initialize status */
    swcDiagManager.status.currentSession = DIAG_SESSION_DEFAULT;
    swcDiagManager.status.securityLevel = SECURITY_LOCKED;
    swcDiagManager.status.activeProtocol = 0;
    swcDiagManager.status.communicationEnabled = TRUE;
    swcDiagManager.status.sessionTimeout = SWC_DIAG_SESSION_TIMEOUT_DEFAULT;
    swcDiagManager.status.securityTimeout = 0;

    /* Initialize DTC list */
    for (i = 0; i < SWC_DIAG_MAX_DTCS; i++) {
        swcDiagManager.dtcList[i].dtcCode = 0;
        swcDiagManager.dtcList[i].statusByte = 0;
        swcDiagManager.dtcList[i].faultDetectionCounter = 0;
        swcDiagManager.dtcList[i].occurrenceCounter = 0;
        swcDiagManager.dtcList[i].agingCounter = 0;
        swcDiagManager.dtcList[i].lastOccurrenceTime = 0;
    }
    swcDiagManager.numDtcs = 0;

    /* Initialize request handling */
    swcDiagManager.hasPendingRequest = FALSE;
    swcDiagManager.lastTesterPresentTime = Rte_GetTime();
    swcDiagManager.dtcSettingEnabled = TRUE;

    Det_ReportError(SWC_DIAGNOSTICMANAGER_MODULE_ID, SWC_DIAGNOSTICMANAGER_INSTANCE_ID,
                    0x01, RTE_E_OK);
}

/**
 * @brief 50ms cyclic runnable
 */
void Swc_DiagnosticManager_50ms(void)
{
    /* Update timeouts */
    Swc_DiagnosticManager_UpdateTimeouts();

    /* Write status via RTE */
    (void)Rte_Write_DiagnosticSession(&swcDiagManager.status.currentSession);
    (void)Rte_Write_SecurityLevel(&swcDiagManager.status.securityLevel);
}

/**
 * @brief Process diagnostic request runnable
 */
void Swc_DiagnosticManager_ProcessRequest(void)
{
    Swc_DiagnosticRequestType request;
    Swc_DiagnosticResponseType response;

    if (!swcDiagManager.hasPendingRequest) {
        /* Check for new request via RTE */
        if ((uint8_t)(Rte_Read_DiagnosticRequest(&request)) == RTE_E_OK) {
            swcDiagManager.pendingRequest = request;
            swcDiagManager.hasPendingRequest = TRUE;
        } else {
            return;
        }
    }

    /* Process the request */
    request = swcDiagManager.pendingRequest;
/* [MISRA Advisory] Redundant:     swcDiagManager.hasPendingRequest = FALSE; */

    /* Initialize response */
    response.responseId = UDS_NEGATIVE_RESPONSE_SID;  /* Negative response service */
    response.dataLength = 0;
    response.negativeResponseCode = 0;

    /* Check security access */
    if (!Swc_DiagnosticManager_CheckSecurityAccess(request.serviceId)) {
        response.negativeResponseCode = UDS_NRC_REQUEST_OUT_OF_RANGE;
        (void)Rte_Write_DiagnosticResponse(&response);
        return;
    }

    /* Process based on service ID */
    switch (request.serviceId) {
        case SID_DIAGNOSTIC_SESSION_CONTROL:
            Swc_DiagnosticManager_ProcessSessionControl(&request, &response);
            break;

        case SID_SECURITY_ACCESS:
            Swc_DiagnosticManager_ProcessSecurityAccess(&request, &response);
            break;

        case SID_READ_DTC_INFORMATION:
            Swc_DiagnosticManager_ProcessReadDtc(&request, &response);
            break;

        case SID_CLEAR_DIAGNOSTIC_INFORMATION:
            Swc_DiagnosticManager_ProcessClearDtc(&request, &response);
            break;

        case SID_TESTER_PRESENT:
            Swc_DiagnosticManager_ProcessTesterPresent(&request, &response);
            break;

        default:
            response.negativeResponseCode = UDS_NRC_SERVICE_NOT_SUPPORTED;
            break;
    }

    /* Send response */
    (void)Rte_Write_DiagnosticResponse(&response);
}

/**
 * @brief Changes diagnostic session
 */
Rte_StatusType Swc_DiagnosticManager_ChangeSession(Swc_DiagnosticSessionType session)
{
    Swc_DiagnosticRequestType request;
    request.serviceId = SID_DIAGNOSTIC_SESSION_CONTROL;
    request.subFunction = (uint8)session;
    request.dataLength = 1;
    request.data[0] = (uint8)session;

    /* Process the request */
    swcDiagManager.pendingRequest = request;
    Swc_DiagnosticManager_ProcessRequest();

    return RTE_E_OK;
}

/**
 * @brief Gets current diagnostic session
 */
Rte_StatusType Swc_DiagnosticManager_GetSession(Swc_DiagnosticSessionType* session)
{
    if (session == NULL_PTR) {
        return RTE_E_INVALID;
    }

    *session = swcDiagManager.status.currentSession;
    return RTE_E_OK;
}

/**
 * @brief Unlocks security level
 */
Rte_StatusType Swc_DiagnosticManager_UnlockSecurity(Swc_SecurityLevelType level,
                                                     const uint8* key)
{
    if ((key == NULL_PTR) || (level > SECURITY_LEVEL_3)) {
        return RTE_E_INVALID;
    }

    if (Swc_DiagnosticManager_ValidateKey(level, key)) {
        swcDiagManager.status.securityLevel = level;
        swcDiagManager.status.securityTimeout = SWC_DIAG_SECURITY_TIMEOUT;
        return RTE_E_OK;
    }

    return RTE_E_INVALID;
}

/**
 * @brief Gets security level
 */
Rte_StatusType Swc_DiagnosticManager_GetSecurityLevel(Swc_SecurityLevelType* level)
{
    if (level == NULL_PTR) {
        return RTE_E_INVALID;
    }

    *level = swcDiagManager.status.securityLevel;
    return RTE_E_OK;
}

/**
 * @brief Processes diagnostic request
 */
Rte_StatusType Swc_DiagnosticManager_ProcessDiagnosticRequest(
    const Swc_DiagnosticRequestType* request,
    Swc_DiagnosticResponseType* response)
{
    if ((request == NULL_PTR) || (response == NULL_PTR)) {
        return RTE_E_INVALID;
    }

    swcDiagManager.pendingRequest = *request;
    swcDiagManager.hasPendingRequest = TRUE;
    Swc_DiagnosticManager_ProcessRequest();

    (void)memset(response, 0, sizeof(*response));
    response->responseId = swcDiagManager.pendingRequest.data[0];  /* Simplified */
    return RTE_E_OK;
}

/**
 * @brief Gets DTC status
 */
Rte_StatusType Swc_DiagnosticManager_GetDtcStatus(uint32 dtcCode, Swc_DtcStatusType* status)
{
    uint8 i;

    if (status == NULL_PTR) {
        return RTE_E_INVALID;
    }

    for (i = 0; i < swcDiagManager.numDtcs; i++) {
        if (swcDiagManager.dtcList[i].dtcCode == dtcCode) {
            *status = swcDiagManager.dtcList[i];
            return RTE_E_OK;
        }
    }

    return RTE_E_INVALID;
}

/**
 * @brief Clears DTC
 */
Rte_StatusType Swc_DiagnosticManager_ClearDtc(uint32 dtcCode)
{
    uint8 i;
    uint8 j;

    if (dtcCode == UDS_CLEAR_ALL_DTCS) {
        /* Clear all DTCs */
        for (i = 0; i < SWC_DIAG_MAX_DTCS; i++) {
            swcDiagManager.dtcList[i].dtcCode = 0;
            swcDiagManager.dtcList[i].statusByte = 0;
            swcDiagManager.dtcList[i].faultDetectionCounter = 0;
            swcDiagManager.dtcList[i].occurrenceCounter = 0;
        }
        swcDiagManager.numDtcs = 0;
        return RTE_E_OK;
    }

    /* Clear specific DTC */
    for (i = 0; i < swcDiagManager.numDtcs; i++) {
        if (swcDiagManager.dtcList[i].dtcCode == dtcCode) {
            /* Shift remaining DTCs */
            for (j = i; j < (swcDiagManager.numDtcs - 1); j++) {
                swcDiagManager.dtcList[j] = swcDiagManager.dtcList[j + 1U];
            }
            swcDiagManager.numDtcs--;
            return RTE_E_OK;
        }
    }

    return RTE_E_INVALID;
}

/**
 * @brief Gets diagnostic manager status
 */
Rte_StatusType Swc_DiagnosticManager_GetStatus(Swc_DiagnosticManagerStatusType* status)
{
    if (status == NULL_PTR) {
        return RTE_E_INVALID;
    }

    *status = swcDiagManager.status;
    return RTE_E_OK;
}

/*==================================================================================================
*                                    MAIN FUNCTION & DEINIT
==================================================================================================*/

/**
 * @brief DiagnosticManager MainFunction - called periodically by RTE Scheduler
 * Dispatches 50ms timeout management and processes diagnostic requests.
 */
void Swc_DiagnosticManager_MainFunction(void)
{
    if (swcDiagManager.status.currentSession == 0U ) {
        return; /* Not initialized */
    }

    /* 50ms timeout management (called every 5th tick at 10ms base) */
    Swc_DiagnosticManager_UpdateTimeouts();
    (void)Rte_Write_DiagnosticSession(&swcDiagManager.status.currentSession);
    (void)Rte_Write_SecurityLevel(&swcDiagManager.status.securityLevel);

    /* Always try to process pending requests */
    Swc_DiagnosticManager_ProcessRequest();
}

/**
 * @brief DiagnosticManager Deinit - deinitializes the component
 */
void Swc_DiagnosticManager_Deinit(void)
{
    uint8 i;

    if (swcDiagManager.status.currentSession == 0U ) {
        return;
    }

    swcDiagManager.status.currentSession = DIAG_SESSION_DEFAULT;
    swcDiagManager.status.securityLevel = SECURITY_LOCKED;
    swcDiagManager.status.communicationEnabled = TRUE;

    for (i = 0; i < SWC_DIAG_MAX_DTCS; i++) {
        swcDiagManager.dtcList[i].dtcCode = 0;
    }
    swcDiagManager.numDtcs = 0;
    swcDiagManager.hasPendingRequest = FALSE;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"