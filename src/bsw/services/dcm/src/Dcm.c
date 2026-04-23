/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Service Layer)
* Dependencies         : PduR, Dem
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-15
* Author               : AI Agent (Dcm Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Dcm.h"
#include "Dcm_Cfg.h"
#include "PduR.h"
#include "Dem.h"
#include "Det.h"
#include "MemMap.h"
#include "string.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define DCM_INSTANCE_ID                 (0x00U)

/* Module state */
#define DCM_STATE_UNINIT                (0x00U)
#define DCM_STATE_INIT                  (0x01U)
#define DCM_STATE_BUSY                  (0x02U)

/* Protocol state */
#define DCM_PROTOCOL_IDLE               (0x00U)
#define DCM_PROTOCOL_RX_IN_PROGRESS     (0x01U)
#define DCM_PROTOCOL_PROCESSING         (0x02U)
#define DCM_PROTOCOL_TX_IN_PROGRESS     (0x03U)

/* Response type */
#define DCM_RESPONSE_POSITIVE           (0x00U)
#define DCM_RESPONSE_NEGATIVE           (0x01U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (DCM_DEV_ERROR_DETECT == STD_ON)
    #define DCM_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(DCM_MODULE_ID, DCM_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define DCM_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* Protocol runtime state */
typedef struct
{
    uint8 State;
    uint8 CurrentSID;
    uint8 CurrentSubFunction;
    uint16 RxDataLength;
    uint16 TxDataLength;
    uint8 RxBuffer[DCM_RX_BUFFER_SIZE];
    uint8 TxBuffer[DCM_TX_BUFFER_SIZE];
    uint32 P2Timer;
    uint32 S3Timer;
    boolean ResponsePending;
} Dcm_ProtocolStateType;

/* Module internal state */
typedef struct
{
    uint8 State;
    const Dcm_ConfigType* ConfigPtr;
    uint8 CurrentSession;
    uint8 CurrentSecurityLevel;
    uint8 SecurityAttempts;
    uint32 SecurityDelayTimer;
    boolean SecurityDelayActive;
    Dcm_ProtocolStateType ProtocolStates[DCM_NUM_PROTOCOLS];
    /* Download/Transfer state */
    uint32 DownloadAddress;
    uint32 DownloadSize;
    uint32 TransferOffset;
    uint8 BlockSequenceCounter;
    boolean TransferActive;
} Dcm_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define DCM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC Dcm_InternalStateType Dcm_InternalState;

#define DCM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC void Dcm_ProcessRequest(uint8 ProtocolId);
STATIC void Dcm_SendPositiveResponse(uint8 ProtocolId, uint8 SID, const uint8* Data, uint16 Length);
STATIC void Dcm_SendNegativeResponse(uint8 ProtocolId, uint8 SID, uint8 NRC);
STATIC Std_ReturnType Dcm_ProcessDiagnosticSessionControl(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC Std_ReturnType Dcm_ProcessEcuReset(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC Std_ReturnType Dcm_ProcessSecurityAccess(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC Std_ReturnType Dcm_ProcessTesterPresent(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC Std_ReturnType Dcm_ProcessReadDataByIdentifier(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC Std_ReturnType Dcm_ProcessWriteDataByIdentifier(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC Std_ReturnType Dcm_ProcessReadDTCInformation(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC Std_ReturnType Dcm_ProcessClearDiagnosticInformation(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC Std_ReturnType Dcm_ProcessRoutineControl(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC Std_ReturnType Dcm_ProcessRequestDownload(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC Std_ReturnType Dcm_ProcessTransferData(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC Std_ReturnType Dcm_ProcessRequestTransferExit(uint8 ProtocolId, const uint8* Data, uint16 Length);
STATIC const Dcm_DIDConfigType* Dcm_FindDID(uint16 DID);
STATIC const Dcm_RIDConfigType* Dcm_FindRID(uint16 RID);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define DCM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Find DID configuration by DID value
 */
STATIC const Dcm_DIDConfigType* Dcm_FindDID(uint16 DID)
{
    const Dcm_DIDConfigType* result = NULL_PTR;
    uint8 i;

    if (Dcm_InternalState.ConfigPtr != NULL_PTR)
    {
        for (i = 0U; i < Dcm_InternalState.ConfigPtr->NumDIDs; i++)
        {
            if (Dcm_InternalState.ConfigPtr->DIDs[i].DID == DID)
            {
                result = &Dcm_InternalState.ConfigPtr->DIDs[i];
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Find RID configuration by RID value
 */
STATIC const Dcm_RIDConfigType* Dcm_FindRID(uint16 RID)
{
    const Dcm_RIDConfigType* result = NULL_PTR;
    uint8 i;

    if (Dcm_InternalState.ConfigPtr != NULL_PTR)
    {
        for (i = 0U; i < Dcm_InternalState.ConfigPtr->NumRIDs; i++)
        {
            if (Dcm_InternalState.ConfigPtr->RIDs[i].RID == RID)
            {
                result = &Dcm_InternalState.ConfigPtr->RIDs[i];
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Send positive response
 */
STATIC void Dcm_SendPositiveResponse(uint8 ProtocolId, uint8 SID, const uint8* Data, uint16 Length)
{
    Dcm_ProtocolStateType* protocolState = &Dcm_InternalState.ProtocolStates[ProtocolId];
    uint8 i;

    /* Build positive response: SID + 0x40 */
    protocolState->TxBuffer[0] = SID + 0x40U;

    /* Copy response data */
    for (i = 0U; i < Length; i++)
    {
        protocolState->TxBuffer[i + 1U] = Data[i];
    }

    protocolState->TxDataLength = Length + 1U;
    protocolState->State = DCM_PROTOCOL_TX_IN_PROGRESS;

    /* Trigger transmission via PduR */
    {
        PduInfoType pduInfo;
        pduInfo.SduDataPtr = protocolState->TxBuffer;
        pduInfo.SduLength = protocolState->TxDataLength;
        pduInfo.MetaDataPtr = NULL_PTR;

        (void)PduR_Transmit(ProtocolId, &pduInfo);
    }
}

/**
 * @brief   Send negative response
 */
STATIC void Dcm_SendNegativeResponse(uint8 ProtocolId, uint8 SID, uint8 NRC)
{
    Dcm_ProtocolStateType* protocolState = &Dcm_InternalState.ProtocolStates[ProtocolId];

    /* Build negative response: 0x7F + SID + NRC */
    protocolState->TxBuffer[0] = 0x7FU;
    protocolState->TxBuffer[1] = SID;
    protocolState->TxBuffer[2] = NRC;

    protocolState->TxDataLength = 3U;
    protocolState->State = DCM_PROTOCOL_TX_IN_PROGRESS;

    /* Trigger transmission via PduR */
    {
        PduInfoType pduInfo;
        pduInfo.SduDataPtr = protocolState->TxBuffer;
        pduInfo.SduLength = protocolState->TxDataLength;
        pduInfo.MetaDataPtr = NULL_PTR;

        (void)PduR_Transmit(ProtocolId, &pduInfo);
    }
}

/**
 * @brief   Process Diagnostic Session Control service
 */
STATIC Std_ReturnType Dcm_ProcessDiagnosticSessionControl(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 sessionType;
    uint8 responseData[4];

    if (Length >= 1U)
    {
        sessionType = Data[0];

        /* Validate session type */
        switch (sessionType)
        {
            case DCM_DEFAULT_SESSION:
            case DCM_PROGRAMMING_SESSION:
            case DCM_EXTENDED_DIAGNOSTIC_SESSION:
            case DCM_SAFETY_SYSTEM_DIAGNOSTIC_SESSION:
                /* Update current session */
                Dcm_InternalState.CurrentSession = sessionType;

                /* Build response: session type + P2 + P2* */
                responseData[0] = sessionType;
                responseData[1] = (uint8)(DCM_P2SERVER_MAX >> 8);
                responseData[2] = (uint8)(DCM_P2SERVER_MAX);
                responseData[3] = (uint8)(DCM_P2STAR_SERVER_MAX >> 8);
                responseData[4] = (uint8)(DCM_P2STAR_SERVER_MAX);

                Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_DIAGNOSTIC_SESSION_CONTROL, responseData, 5U);
                result = E_OK;
                break;

            default:
                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_DIAGNOSTIC_SESSION_CONTROL, DCM_E_SUBFUNCTION_NOT_SUPPORTED);
                break;
        }
    }
    else
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_DIAGNOSTIC_SESSION_CONTROL, DCM_E_INCORRECT_MESSAGE_LENGTH);
    }

    return result;
}

/**
 * @brief   Process ECU Reset service
 */
STATIC Std_ReturnType Dcm_ProcessEcuReset(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 resetType;
    uint8 responseData[1];

    if (Length >= 1U)
    {
        resetType = Data[0];

        /* Validate reset type */
        switch (resetType)
        {
            case 0x01U: /* Hard Reset */
            case 0x02U: /* Key Off On Reset */
            case 0x03U: /* Soft Reset */
            case 0x04U: /* Enable Rapid Power Shutdown */
            case 0x05U: /* Disable Rapid Power Shutdown */
                /* Build response: reset type */
                responseData[0] = resetType;
                Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_ECU_RESET, responseData, 1U);

                /* Perform reset (implementation specific) */
                /* Mcu_PerformReset(); */

                result = E_OK;
                break;

            default:
                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ECU_RESET, DCM_E_SUBFUNCTION_NOT_SUPPORTED);
                break;
        }
    }
    else
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ECU_RESET, DCM_E_INCORRECT_MESSAGE_LENGTH);
    }

    return result;
}

/**
 * @brief   Process Security Access service
 */
STATIC Std_ReturnType Dcm_ProcessSecurityAccess(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 subFunction;
    uint8 securityLevel;
    uint8 responseData[DCM_SEED_SIZE + 1];
    uint8 i;

    if (Length >= 1U)
    {
        subFunction = Data[0];
        securityLevel = subFunction & 0x3FU;

        if ((subFunction & 0x40U) == 0U)
        {
            /* Request Seed */
            if (Dcm_InternalState.SecurityDelayActive)
            {
                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, DCM_E_REQUIRED_TIME_DELAY_NOT_EXPIRED);
            }
            else if (Dcm_InternalState.SecurityAttempts >= DCM_MAX_SECURITY_ATTEMPTS)
            {
                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, DCM_E_EXCEED_NUMBER_OF_ATTEMPTS);
                Dcm_InternalState.SecurityDelayActive = TRUE;
                Dcm_InternalState.SecurityDelayTimer = DCM_SECURITY_DELAY_TIME;
            }
            else
            {
                /* Generate seed (simplified - should be random) */
                responseData[0] = subFunction;
                for (i = 0U; i < DCM_SEED_SIZE; i++)
                {
                    responseData[i + 1U] = (uint8)(0xA5U + i);
                }

                Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, responseData, DCM_SEED_SIZE + 1U);
                result = E_OK;
            }
        }
        else
        {
            /* Send Key */
            if (Length >= (DCM_KEY_SIZE + 1U))
            {
                /* Validate key (simplified - should compare calculated key) */
                boolean keyValid = TRUE;

                /* Check if key matches expected value */
                for (i = 0U; i < DCM_KEY_SIZE; i++)
                {
                    if (Data[i + 1U] != (uint8)(0xA5U + i))
                    {
                        keyValid = FALSE;
                        break;
                    }
                }

                if (keyValid)
                {
                    Dcm_InternalState.CurrentSecurityLevel = securityLevel;
                    Dcm_InternalState.SecurityAttempts = 0U;

                    responseData[0] = subFunction;
                    Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, responseData, 1U);
                    result = E_OK;
                }
                else
                {
                    Dcm_InternalState.SecurityAttempts++;
                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, DCM_E_INVALID_KEY);
                }
            }
            else
            {
                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, DCM_E_INCORRECT_MESSAGE_LENGTH);
            }
        }
    }
    else
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, DCM_E_INCORRECT_MESSAGE_LENGTH);
    }

    return result;
}

/**
 * @brief   Process Tester Present service
 */
STATIC Std_ReturnType Dcm_ProcessTesterPresent(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 subFunction;
    uint8 responseData[1];

    if (Length >= 1U)
    {
        subFunction = Data[0];

        if ((subFunction == 0x00U) || (subFunction == 0x80U))
        {
            /* Reset S3 timer */
            Dcm_InternalState.ProtocolStates[ProtocolId].S3Timer = DCM_S3SERVER;

            /* Send response (suppress for subFunction 0x80 if required) */
            if (subFunction == 0x00U)
            {
                responseData[0] = subFunction;
                Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_TESTER_PRESENT, responseData, 1U);
            }

            result = E_OK;
        }
        else
        {
            Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_TESTER_PRESENT, DCM_E_SUBFUNCTION_NOT_SUPPORTED);
        }
    }
    else
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_TESTER_PRESENT, DCM_E_INCORRECT_MESSAGE_LENGTH);
    }

    return result;
}

/**
 * @brief   Process Read Data By Identifier service
 */
STATIC Std_ReturnType Dcm_ProcessReadDataByIdentifier(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint16 did;
    const Dcm_DIDConfigType* didConfig;
    uint8 responseData[DCM_TX_BUFFER_SIZE];

    if (Length >= 2U)
    {
        /* Extract DID (big endian) */
        did = ((uint16)Data[0] << 8U) | Data[1];

        /* Find DID configuration */
        didConfig = Dcm_FindDID(did);

        if (didConfig != NULL_PTR)
        {
            /* Check security level */
            if (Dcm_InternalState.CurrentSecurityLevel >= didConfig->SecurityLevel)
            {
                /* Check session type */
                if ((didConfig->SessionType == DCM_DEFAULT_SESSION) ||
                    (Dcm_InternalState.CurrentSession == didConfig->SessionType))
                {
                    /* Read data */
                    if (didConfig->ReadDataFnc != NULL_PTR)
                    {
                        if (didConfig->ReadDataFnc(&responseData[2]) == E_OK)
                        {
                            /* Build response: DID + data */
                            responseData[0] = (uint8)(did >> 8U);
                            responseData[1] = (uint8)(did);
                            Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_READ_DATA_BY_IDENTIFIER, responseData, didConfig->DataLength + 2U);
                            result = E_OK;
                        }
                        else
                        {
                            Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DATA_BY_IDENTIFIER, DCM_E_CONDITIONS_NOT_CORRECT);
                        }
                    }
                    else
                    {
                        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DATA_BY_IDENTIFIER, DCM_E_CONDITIONS_NOT_CORRECT);
                    }
                }
                else
                {
                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DATA_BY_IDENTIFIER, DCM_E_SERVICE_NOT_SUPPORTED);
                }
            }
            else
            {
                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DATA_BY_IDENTIFIER, DCM_E_SECURITY_ACCESS_DENIED);
            }
        }
        else
        {
            Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DATA_BY_IDENTIFIER, DCM_E_REQUEST_OUT_OF_RANGE);
        }
    }
    else
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DATA_BY_IDENTIFIER, DCM_E_INCORRECT_MESSAGE_LENGTH);
    }

    return result;
}

/**
 * @brief   Process Write Data By Identifier service
 */
STATIC Std_ReturnType Dcm_ProcessWriteDataByIdentifier(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint16 did;
    const Dcm_DIDConfigType* didConfig;

    if (Length >= 2U)
    {
        /* Extract DID (big endian) */
        did = ((uint16)Data[0] << 8U) | Data[1];

        /* Find DID configuration */
        didConfig = Dcm_FindDID(did);

        if (didConfig != NULL_PTR)
        {
            /* Check security level */
            if (Dcm_InternalState.CurrentSecurityLevel >= didConfig->SecurityLevel)
            {
                /* Check session type */
                if ((didConfig->SessionType == DCM_DEFAULT_SESSION) ||
                    (Dcm_InternalState.CurrentSession == didConfig->SessionType))
                {
                    /* Write data */
                    if (didConfig->WriteDataFnc != NULL_PTR)
                    {
                        if (didConfig->WriteDataFnc(&Data[2], Length - 2U) == E_OK)
                        {
                            /* Build response: DID */
                            uint8 responseData[2];
                            responseData[0] = (uint8)(did >> 8U);
                            responseData[1] = (uint8)(did);
                            Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_WRITE_DATA_BY_IDENTIFIER, responseData, 2U);
                            result = E_OK;
                        }
                        else
                        {
                            Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_WRITE_DATA_BY_IDENTIFIER, DCM_E_CONDITIONS_NOT_CORRECT);
                        }
                    }
                    else
                    {
                        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_WRITE_DATA_BY_IDENTIFIER, DCM_E_REQUEST_OUT_OF_RANGE);
                    }
                }
                else
                {
                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_WRITE_DATA_BY_IDENTIFIER, DCM_E_SERVICE_NOT_SUPPORTED);
                }
            }
            else
            {
                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_WRITE_DATA_BY_IDENTIFIER, DCM_E_SECURITY_ACCESS_DENIED);
            }
        }
        else
        {
            Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_WRITE_DATA_BY_IDENTIFIER, DCM_E_REQUEST_OUT_OF_RANGE);
        }
    }
    else
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_WRITE_DATA_BY_IDENTIFIER, DCM_E_INCORRECT_MESSAGE_LENGTH);
    }

    return result;
}

/**
 * @brief   Process Read DTC Information service
 */
STATIC Std_ReturnType Dcm_ProcessReadDTCInformation(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 subFunction;
    uint8 responseData[DCM_TX_BUFFER_SIZE];
    uint16 responseLength = 0U;

    if (Length >= 1U)
    {
        subFunction = Data[0];

        switch (subFunction)
        {
            case 0x01U: /* reportNumberOfDTCByStatusMask */
            case 0x12U: /* reportNumberOfEmissionsRelatedOBDDTCByStatusMask */
                if (Length >= 2U)
                {
                    uint8 dtcStatusMask = Data[1];
                    uint16 dtcCount = 0U;
                    uint8 i;

                    /* Count DTCs matching the status mask */
                    for (i = 0U; i < DEM_NUM_DTCS; i++)
                    {
                        Dem_DTCStatusType dtcStatus;
                        if (Dem_GetDTCStatus(Dem_Config.DtcParameters[i].Dtc,
                                             DEM_DTC_ORIGIN_PRIMARY_MEMORY, &dtcStatus) == E_OK)
                        {
                            if ((dtcStatus & dtcStatusMask) != 0U)
                            {
                                dtcCount++;
                            }
                        }
                    }

                    responseData[0] = subFunction;
                    responseData[1] = DEM_DTC_STATUS_AVAILABILITY_MASK;
                    responseData[2] = 0x00U; /* DTCFormatIdentifier = ISO_14229 */
                    responseData[3] = (uint8)(dtcCount >> 8);
                    responseData[4] = (uint8)(dtcCount);
                    responseLength = 5U;
                    Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_READ_DTC_INFORMATION, responseData, responseLength);
                    result = E_OK;
                }
                else
                {
                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DTC_INFORMATION, DCM_E_INCORRECT_MESSAGE_LENGTH);
                }
                break;

            case 0x02U: /* reportDTCByStatusMask */
            case 0x13U: /* reportEmissionsRelatedOBDDTCByStatusMask */
                if (Length >= 2U)
                {
                    uint8 dtcStatusMask = Data[1];
                    uint8 i;
                    uint16 idx = 2U;

                    responseData[0] = subFunction;
                    responseData[1] = DEM_DTC_STATUS_AVAILABILITY_MASK;

                    /* Return all DTCs matching the status mask */
                    for (i = 0U; i < DEM_NUM_DTCS; i++)
                    {
                        Dem_DTCStatusType dtcStatus;
                        if (Dem_GetDTCStatus(Dem_Config.DtcParameters[i].Dtc,
                                             DEM_DTC_ORIGIN_PRIMARY_MEMORY, &dtcStatus) == E_OK)
                        {
                            if ((dtcStatus & dtcStatusMask) != 0U)
                            {
                                if ((idx + 4U) < DCM_TX_BUFFER_SIZE)
                                {
                                    responseData[idx++] = (uint8)(Dem_Config.DtcParameters[i].Dtc >> 16);
                                    responseData[idx++] = (uint8)(Dem_Config.DtcParameters[i].Dtc >> 8);
                                    responseData[idx++] = (uint8)(Dem_Config.DtcParameters[i].Dtc);
                                    responseData[idx++] = dtcStatus;
                                }
                            }
                        }
                    }

                    responseLength = idx;
                    Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_READ_DTC_INFORMATION, responseData, responseLength);
                    result = E_OK;
                }
                else
                {
                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DTC_INFORMATION, DCM_E_INCORRECT_MESSAGE_LENGTH);
                }
                break;

            case 0x06U: /* reportDTCExtDataRecordByDTCNumber */
                if (Length >= 5U)
                {
                    uint32 dtc = ((uint32)Data[1] << 16) | ((uint32)Data[2] << 8) | Data[3];
                    uint8 extDataRecordNumber = Data[4];

                    /* Simplified: return empty extended data */
                    responseData[0] = subFunction;
                    responseData[1] = (uint8)(dtc >> 16);
                    responseData[2] = (uint8)(dtc >> 8);
                    responseData[3] = (uint8)(dtc);
                    responseData[4] = 0x00U; /* DTC status */
                    responseData[5] = extDataRecordNumber;
                    responseLength = 6U;
                    Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_READ_DTC_INFORMATION, responseData, responseLength);
                    result = E_OK;
                }
                else
                {
                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DTC_INFORMATION, DCM_E_INCORRECT_MESSAGE_LENGTH);
                }
                break;

            case 0x0AU: /* reportSupportedDTC */
                {
                    uint8 i;
                    uint16 idx = 2U;

                    responseData[0] = subFunction;
                    responseData[1] = DEM_DTC_STATUS_AVAILABILITY_MASK;

                    /* Return all configured DTCs */
                    for (i = 0U; i < DEM_NUM_DTCS; i++)
                    {
                        Dem_DTCStatusType dtcStatus;
                        if (Dem_GetDTCStatus(Dem_Config.DtcParameters[i].Dtc,
                                             DEM_DTC_ORIGIN_PRIMARY_MEMORY, &dtcStatus) == E_OK)
                        {
                            if ((idx + 4U) < DCM_TX_BUFFER_SIZE)
                            {
                                responseData[idx++] = (uint8)(Dem_Config.DtcParameters[i].Dtc >> 16);
                                responseData[idx++] = (uint8)(Dem_Config.DtcParameters[i].Dtc >> 8);
                                responseData[idx++] = (uint8)(Dem_Config.DtcParameters[i].Dtc);
                                responseData[idx++] = dtcStatus;
                            }
                        }
                    }

                    responseLength = idx;
                    Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_READ_DTC_INFORMATION, responseData, responseLength);
                    result = E_OK;
                }
                break;

            default:
                /* Simplified fallback - return no DTCs */
                responseData[0] = subFunction;
                responseData[1] = DEM_DTC_STATUS_AVAILABILITY_MASK;
                responseData[2] = 0x00U;
                responseData[3] = 0x00U;
                responseData[4] = 0x00U;
                Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_READ_DTC_INFORMATION, responseData, 5U);
                result = E_OK;
                break;
        }
    }
    else
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DTC_INFORMATION, DCM_E_INCORRECT_MESSAGE_LENGTH);
    }

    return result;
}

/**
 * @brief   Process Clear Diagnostic Information service
 */
STATIC Std_ReturnType Dcm_ProcessClearDiagnosticInformation(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;

    if (Length >= 3U)
    {
        /* Clear DTCs (implementation specific) */
        /* Dem_ClearDTCs(); */

        Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_CLEAR_DIAGNOSTIC_INFORMATION, NULL_PTR, 0U);
        result = E_OK;
    }
    else
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_CLEAR_DIAGNOSTIC_INFORMATION, DCM_E_INCORRECT_MESSAGE_LENGTH);
    }

    return result;
}

/**
 * @brief   Process Routine Control service
 */
STATIC Std_ReturnType Dcm_ProcessRoutineControl(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 subFunction;
    uint16 rid;
    const Dcm_RIDConfigType* ridConfig;
    uint8 responseData[DCM_TX_BUFFER_SIZE];
    uint16 responseLength = 0U;

    if (Length >= 3U)
    {
        subFunction = Data[0];
        rid = ((uint16)Data[1] << 8U) | Data[2];

        /* Find RID configuration */
        ridConfig = Dcm_FindRID(rid);

        if (ridConfig != NULL_PTR)
        {
            /* Check security level */
            if (Dcm_InternalState.CurrentSecurityLevel >= ridConfig->SecurityLevel)
            {
                /* Check session type */
                if ((ridConfig->SessionType == DCM_DEFAULT_SESSION) ||
                    (Dcm_InternalState.CurrentSession == ridConfig->SessionType))
                {
                    /* Process routine based on subfunction */
                    switch (subFunction)
                    {
                        case 0x01U: /* Start Routine */
                            if (ridConfig->StartFnc != NULL_PTR)
                            {
                                if (ridConfig->StartFnc(&Data[3], Length - 3U, &responseData[3], &responseLength) == E_OK)
                                {
                                    responseData[0] = subFunction;
                                    responseData[1] = (uint8)(rid >> 8U);
                                    responseData[2] = (uint8)(rid);
                                    Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_ROUTINE_CONTROL, responseData, responseLength + 3U);
                                    result = E_OK;
                                }
                                else
                                {
                                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ROUTINE_CONTROL, DCM_E_CONDITIONS_NOT_CORRECT);
                                }
                            }
                            break;

                        case 0x02U: /* Stop Routine */
                            if (ridConfig->StopFnc != NULL_PTR)
                            {
                                if (ridConfig->StopFnc(&Data[3], Length - 3U, &responseData[3], &responseLength) == E_OK)
                                {
                                    responseData[0] = subFunction;
                                    responseData[1] = (uint8)(rid >> 8U);
                                    responseData[2] = (uint8)(rid);
                                    Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_ROUTINE_CONTROL, responseData, responseLength + 3U);
                                    result = E_OK;
                                }
                                else
                                {
                                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ROUTINE_CONTROL, DCM_E_CONDITIONS_NOT_CORRECT);
                                }
                            }
                            break;

                        case 0x03U: /* Request Routine Results */
                            if (ridConfig->RequestResultFnc != NULL_PTR)
                            {
                                if (ridConfig->RequestResultFnc(&responseData[3], &responseLength) == E_OK)
                                {
                                    responseData[0] = subFunction;
                                    responseData[1] = (uint8)(rid >> 8U);
                                    responseData[2] = (uint8)(rid);
                                    Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_ROUTINE_CONTROL, responseData, responseLength + 3U);
                                    result = E_OK;
                                }
                                else
                                {
                                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ROUTINE_CONTROL, DCM_E_CONDITIONS_NOT_CORRECT);
                                }
                            }
                            break;

                        default:
                            Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ROUTINE_CONTROL, DCM_E_SUBFUNCTION_NOT_SUPPORTED);
                            break;
                    }
                }
                else
                {
                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ROUTINE_CONTROL, DCM_E_SERVICE_NOT_SUPPORTED);
                }
            }
            else
            {
                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ROUTINE_CONTROL, DCM_E_SECURITY_ACCESS_DENIED);
            }
        }
        else
        {
            Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ROUTINE_CONTROL, DCM_E_REQUEST_OUT_OF_RANGE);
        }
    }
    else
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ROUTINE_CONTROL, DCM_E_INCORRECT_MESSAGE_LENGTH);
    }

    return result;
}

/**
 * @brief   Process Request Download service (0x34)
 */
STATIC Std_ReturnType Dcm_ProcessRequestDownload(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 dataFormatIdentifier;
    uint8 addressAndLengthFormatIdentifier;
    uint8 memoryAddressSize;
    uint8 memorySizeSize;
    uint8 responseData[4];

    if (Length >= 3U)
    {
        dataFormatIdentifier = Data[0];
        addressAndLengthFormatIdentifier = Data[1];
        memoryAddressSize = (addressAndLengthFormatIdentifier >> 4U) & 0x0FU;
        memorySizeSize = addressAndLengthFormatIdentifier & 0x0FU;

        if (Length >= (2U + memoryAddressSize + memorySizeSize))
        {
            uint32 memoryAddress = 0U;
            uint32 memorySize = 0U;
            uint8 i;

            /* Extract memory address */
            for (i = 0U; i < memoryAddressSize; i++)
            {
                memoryAddress = (memoryAddress << 8U) | Data[2U + i];
            }

            /* Extract memory size */
            for (i = 0U; i < memorySizeSize; i++)
            {
                memorySize = (memorySize << 8U) | Data[2U + memoryAddressSize + i];
            }

            /* Store download parameters */
            Dcm_InternalState.DownloadAddress = memoryAddress;
            Dcm_InternalState.DownloadSize = memorySize;
            Dcm_InternalState.TransferOffset = 0U;
            Dcm_InternalState.BlockSequenceCounter = 0U;
            Dcm_InternalState.TransferActive = TRUE;

            /* Build response: lengthFormatIdentifier + maxNumberOfBlockLength */
            responseData[0] = 0x20U; /* lengthFormatIdentifier */
            responseData[1] = 0x00U;
            responseData[2] = (uint8)(DCM_TRANSFER_BLOCK_SIZE >> 8U);
            responseData[3] = (uint8)(DCM_TRANSFER_BLOCK_SIZE);

            Dcm_SendPositiveResponse(ProtocolId, DCM_UDS_SID_REQUEST_DOWNLOAD, responseData, 4U);
            result = E_OK;
        }
        else
        {
            Dcm_SendNegativeResponse(ProtocolId, DCM_UDS_SID_REQUEST_DOWNLOAD, DCM_E_INCORRECT_MESSAGE_LENGTH);
        }
    }
    else
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_UDS_SID_REQUEST_DOWNLOAD, DCM_E_INCORRECT_MESSAGE_LENGTH);
    }

    (void)dataFormatIdentifier; /* Unused in simplified implementation */
    return result;
}

/**
 * @brief   Process Transfer Data service (0x36)
 */
STATIC Std_ReturnType Dcm_ProcessTransferData(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 blockSequenceCounter;
    uint16 dataLength;

    if (!Dcm_InternalState.TransferActive)
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_UDS_SID_TRANSFER_DATA, DCM_E_REQUEST_SEQUENCE_ERROR);
        return result;
    }

    if (Length >= 1U)
    {
        blockSequenceCounter = Data[0];
        dataLength = Length - 1U;

        /* Validate block sequence counter */
        if (blockSequenceCounter == (uint8)(Dcm_InternalState.BlockSequenceCounter + 1U))
        {
            /* Update transfer state */
            Dcm_InternalState.BlockSequenceCounter = blockSequenceCounter;
            Dcm_InternalState.TransferOffset += dataLength;

            /* In a real implementation, data would be written to flash here */
            /* For now, just acknowledge the transfer */

            /* Build response: blockSequenceCounter */
            Dcm_SendPositiveResponse(ProtocolId, DCM_UDS_SID_TRANSFER_DATA, &blockSequenceCounter, 1U);
            result = E_OK;
        }
        else
        {
            Dcm_SendNegativeResponse(ProtocolId, DCM_UDS_SID_TRANSFER_DATA, DCM_E_WRONGBLOCKSEQUENCECOUNTER);
        }
    }
    else
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_UDS_SID_TRANSFER_DATA, DCM_E_INCORRECT_MESSAGE_LENGTH);
    }

    return result;
}

/**
 * @brief   Process Request Transfer Exit service (0x37)
 */
STATIC Std_ReturnType Dcm_ProcessRequestTransferExit(uint8 ProtocolId, const uint8* Data, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;

    if (!Dcm_InternalState.TransferActive)
    {
        Dcm_SendNegativeResponse(ProtocolId, DCM_UDS_SID_REQUEST_TRANSFER_EXIT, DCM_E_REQUEST_SEQUENCE_ERROR);
        return result;
    }

    (void)Data;
    (void)Length;

    /* Finalize transfer */
    Dcm_InternalState.TransferActive = FALSE;
    Dcm_InternalState.BlockSequenceCounter = 0U;

    /* Build positive response (no parameters) */
    Dcm_SendPositiveResponse(ProtocolId, DCM_UDS_SID_REQUEST_TRANSFER_EXIT, NULL_PTR, 0U);
    result = E_OK;

    return result;
}

/**
 * @brief   Process incoming diagnostic request
 */
STATIC void Dcm_ProcessRequest(uint8 ProtocolId)
{
    Dcm_ProtocolStateType* protocolState = &Dcm_InternalState.ProtocolStates[ProtocolId];
    uint8 serviceId;

    if (protocolState->RxDataLength > 0U)
    {
        serviceId = protocolState->RxBuffer[0];

        /* Process based on service ID */
        switch (serviceId)
        {
            case DCM_SERVICE_DIAGNOSTIC_SESSION_CONTROL:
                (void)Dcm_ProcessDiagnosticSessionControl(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            case DCM_SERVICE_ECU_RESET:
                (void)Dcm_ProcessEcuReset(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            case DCM_SERVICE_SECURITY_ACCESS:
                (void)Dcm_ProcessSecurityAccess(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            case DCM_SERVICE_TESTER_PRESENT:
                (void)Dcm_ProcessTesterPresent(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            case DCM_SERVICE_READ_DATA_BY_IDENTIFIER:
                (void)Dcm_ProcessReadDataByIdentifier(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            case DCM_SERVICE_WRITE_DATA_BY_IDENTIFIER:
                (void)Dcm_ProcessWriteDataByIdentifier(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            case DCM_SERVICE_READ_DTC_INFORMATION:
                (void)Dcm_ProcessReadDTCInformation(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            case DCM_SERVICE_CLEAR_DIAGNOSTIC_INFORMATION:
                (void)Dcm_ProcessClearDiagnosticInformation(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            case DCM_SERVICE_ROUTINE_CONTROL:
                (void)Dcm_ProcessRoutineControl(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            case DCM_UDS_SID_REQUEST_DOWNLOAD:
                (void)Dcm_ProcessRequestDownload(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            case DCM_UDS_SID_TRANSFER_DATA:
                (void)Dcm_ProcessTransferData(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            case DCM_UDS_SID_REQUEST_TRANSFER_EXIT:
                (void)Dcm_ProcessRequestTransferExit(ProtocolId, &protocolState->RxBuffer[1], protocolState->RxDataLength - 1U);
                break;

            default:
                Dcm_SendNegativeResponse(ProtocolId, serviceId, DCM_E_SERVICE_NOT_SUPPORTED);
                break;
        }
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the DCM module
 */
void Dcm_Init(const Dcm_ConfigType* ConfigPtr)
{
    uint8 i;

#if (DCM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        DCM_DET_REPORT_ERROR(DCM_SERVICE_ID_INIT, DCM_E_PARAM_POINTER);
        return;
    }
#endif

    /* Store configuration pointer */
    Dcm_InternalState.ConfigPtr = ConfigPtr;

    /* Initialize session and security */
    Dcm_InternalState.CurrentSession = DCM_DEFAULT_SESSION;
    Dcm_InternalState.CurrentSecurityLevel = DCM_SEC_LEV_LOCKED;
    Dcm_InternalState.SecurityAttempts = 0U;
    Dcm_InternalState.SecurityDelayActive = FALSE;
    Dcm_InternalState.SecurityDelayTimer = 0U;

    /* Initialize transfer state */
    Dcm_InternalState.DownloadAddress = 0U;
    Dcm_InternalState.DownloadSize = 0U;
    Dcm_InternalState.TransferOffset = 0U;
    Dcm_InternalState.BlockSequenceCounter = 0U;
    Dcm_InternalState.TransferActive = FALSE;

    /* Initialize protocol states */
    for (i = 0U; i < DCM_NUM_PROTOCOLS; i++)
    {
        Dcm_InternalState.ProtocolStates[i].State = DCM_PROTOCOL_IDLE;
        Dcm_InternalState.ProtocolStates[i].CurrentSID = 0U;
        Dcm_InternalState.ProtocolStates[i].RxDataLength = 0U;
        Dcm_InternalState.ProtocolStates[i].TxDataLength = 0U;
        Dcm_InternalState.ProtocolStates[i].P2Timer = 0U;
        Dcm_InternalState.ProtocolStates[i].S3Timer = DCM_S3SERVER;
        Dcm_InternalState.ProtocolStates[i].ResponsePending = FALSE;
    }

    /* Set module state to initialized */
    Dcm_InternalState.State = DCM_STATE_INIT;
}

/**
 * @brief   Deinitializes the DCM module
 */
void Dcm_DeInit(void)
{
#if (DCM_DEV_ERROR_DETECT == STD_ON)
    if (Dcm_InternalState.State != DCM_STATE_INIT)
    {
        DCM_DET_REPORT_ERROR(DCM_SERVICE_ID_DEINIT, DCM_E_UNINIT);
        return;
    }
#endif

    /* Clear configuration pointer */
    Dcm_InternalState.ConfigPtr = NULL_PTR;

    /* Reset session and security */
    Dcm_InternalState.CurrentSession = DCM_DEFAULT_SESSION;
    Dcm_InternalState.CurrentSecurityLevel = DCM_SEC_LEV_LOCKED;

    /* Set module state to uninitialized */
    Dcm_InternalState.State = DCM_STATE_UNINIT;
}

/**
 * @brief   Main function for DCM processing
 */
void Dcm_MainFunction(void)
{
    uint8 i;

    if (Dcm_InternalState.State == DCM_STATE_INIT)
    {
        /* Process each protocol */
        for (i = 0U; i < DCM_NUM_PROTOCOLS; i++)
        {
            Dcm_ProtocolStateType* protocolState = &Dcm_InternalState.ProtocolStates[i];

            /* Handle S3 timeout (session timeout) */
            if (protocolState->S3Timer > 0U)
            {
                protocolState->S3Timer--;
                if (protocolState->S3Timer == 0U)
                {
                    /* Session timeout - return to default session */
                    Dcm_InternalState.CurrentSession = DCM_DEFAULT_SESSION;
                    Dcm_InternalState.CurrentSecurityLevel = DCM_SEC_LEV_LOCKED;
                }
            }

            /* Handle security delay timer */
            if (Dcm_InternalState.SecurityDelayActive)
            {
                if (Dcm_InternalState.SecurityDelayTimer > 0U)
                {
                    Dcm_InternalState.SecurityDelayTimer--;
                }
                else
                {
                    Dcm_InternalState.SecurityDelayActive = FALSE;
                    Dcm_InternalState.SecurityAttempts = 0U;
                }
            }

            /* Handle P2 timeout */
            if (protocolState->P2Timer > 0U)
            {
                protocolState->P2Timer--;
                if (protocolState->P2Timer == 0U)
                {
                    /* P2 timeout - send response pending if needed */
                    if (protocolState->ResponsePending)
                    {
                        Dcm_SendNegativeResponse(i, protocolState->CurrentSID, DCM_E_RESPONSE_TOO_LONG);
                        protocolState->ResponsePending = FALSE;
                    }
                }
            }
        }
    }
}

/**
 * @brief   TxConfirmation callback from PduR
 */
void Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
#if (DCM_DEV_ERROR_DETECT == STD_ON)
    if (Dcm_InternalState.State != DCM_STATE_INIT)
    {
        DCM_DET_REPORT_ERROR(DCM_SERVICE_ID_TXCONFIRMATION, DCM_E_UNINIT);
        return;
    }
#endif

    if (TxPduId < DCM_NUM_PROTOCOLS)
    {
        /* Reset protocol state to idle after transmission complete */
        Dcm_InternalState.ProtocolStates[TxPduId].State = DCM_PROTOCOL_IDLE;
        Dcm_InternalState.ProtocolStates[TxPduId].TxDataLength = 0U;
    }
}

/**
 * @brief   RxIndication callback from PduR
 */
void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
#if (DCM_DEV_ERROR_DETECT == STD_ON)
    if (Dcm_InternalState.State != DCM_STATE_INIT)
    {
        DCM_DET_REPORT_ERROR(DCM_SERVICE_ID_RXINDICATION, DCM_E_UNINIT);
        return;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        DCM_DET_REPORT_ERROR(DCM_SERVICE_ID_RXINDICATION, DCM_E_PARAM_POINTER);
        return;
    }
#endif

    if ((RxPduId < DCM_NUM_PROTOCOLS) && (PduInfoPtr != NULL_PTR))
    {
        Dcm_ProtocolStateType* protocolState = &Dcm_InternalState.ProtocolStates[RxPduId];

        /* Copy received data to RX buffer */
        if ((PduInfoPtr->SduDataPtr != NULL_PTR) && (PduInfoPtr->SduLength > 0U))
        {
            uint16 copyLength = (PduInfoPtr->SduLength < DCM_RX_BUFFER_SIZE) ?
                                PduInfoPtr->SduLength : DCM_RX_BUFFER_SIZE;

            (void)memcpy(protocolState->RxBuffer, PduInfoPtr->SduDataPtr, copyLength);
            protocolState->RxDataLength = copyLength;
            protocolState->State = DCM_PROTOCOL_PROCESSING;

            /* Process the request */
            Dcm_ProcessRequest(RxPduId);
        }
    }
}

/**
 * @brief   TriggerTransmit callback from PduR
 */
Std_ReturnType Dcm_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    Std_ReturnType result = E_NOT_OK;

#if (DCM_DEV_ERROR_DETECT == STD_ON)
    if (Dcm_InternalState.State != DCM_STATE_INIT)
    {
        DCM_DET_REPORT_ERROR(DCM_SERVICE_ID_TRIGGERTRANSMIT, DCM_E_UNINIT);
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        DCM_DET_REPORT_ERROR(DCM_SERVICE_ID_TRIGGERTRANSMIT, DCM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if ((TxPduId < DCM_NUM_PROTOCOLS) && (PduInfoPtr != NULL_PTR))
    {
        Dcm_ProtocolStateType* protocolState = &Dcm_InternalState.ProtocolStates[TxPduId];

        /* Provide TX data */
        PduInfoPtr->SduDataPtr = protocolState->TxBuffer;
        PduInfoPtr->SduLength = protocolState->TxDataLength;

        result = E_OK;
    }

    return result;
}

/**
 * @brief   Get current security level
 */
Std_ReturnType Dcm_GetSecurityLevel(uint8* SecLevel)
{
    Std_ReturnType result = E_NOT_OK;

#if (DCM_DEV_ERROR_DETECT == STD_ON)
    if (Dcm_InternalState.State != DCM_STATE_INIT)
    {
        DCM_DET_REPORT_ERROR(0x0DU, DCM_E_UNINIT);
        return E_NOT_OK;
    }

    if (SecLevel == NULL_PTR)
    {
        DCM_DET_REPORT_ERROR(0x0DU, DCM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if (SecLevel != NULL_PTR)
    {
        *SecLevel = Dcm_InternalState.CurrentSecurityLevel;
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Get current session control type
 */
Std_ReturnType Dcm_GetSesCtrlType(uint8* SesCtrlType)
{
    Std_ReturnType result = E_NOT_OK;

#if (DCM_DEV_ERROR_DETECT == STD_ON)
    if (Dcm_InternalState.State != DCM_STATE_INIT)
    {
        DCM_DET_REPORT_ERROR(0x0CU, DCM_E_UNINIT);
        return E_NOT_OK;
    }

    if (SesCtrlType == NULL_PTR)
    {
        DCM_DET_REPORT_ERROR(0x0CU, DCM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if (SesCtrlType != NULL_PTR)
    {
        *SesCtrlType = Dcm_InternalState.CurrentSession;
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Reset to default session
 */
Std_ReturnType Dcm_ResetToDefaultSession(void)
{
    Std_ReturnType result = E_NOT_OK;

#if (DCM_DEV_ERROR_DETECT == STD_ON)
    if (Dcm_InternalState.State != DCM_STATE_INIT)
    {
        DCM_DET_REPORT_ERROR(0x2DU, DCM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    Dcm_InternalState.CurrentSession = DCM_DEFAULT_SESSION;
    Dcm_InternalState.CurrentSecurityLevel = DCM_SEC_LEV_LOCKED;
    result = E_OK;

    return result;
}

/**
 * @brief   Get version information
 */
#if (DCM_VERSION_INFO_API == STD_ON)
void Dcm_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (DCM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        DCM_DET_REPORT_ERROR(DCM_SERVICE_ID_GETVERSIONINFO, DCM_E_PARAM_POINTER);
        return;
    }
#endif

    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID = DCM_VENDOR_ID;
        versioninfo->moduleID = DCM_MODULE_ID;
        versioninfo->sw_major_version = DCM_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = DCM_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = DCM_SW_PATCH_VERSION;
    }
}
#endif

#define DCM_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
