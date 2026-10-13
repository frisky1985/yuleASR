/**
 * @file stubs.c
 * @brief Minimal dependency stubs for dcm_test.
 *
 * Dcm.c references PduR_Transmit (positive/negative response path) and
 * Dem_GetStatusOfDTC + Dem_Config (ReadDTCInformation path, via the
 * Dem_GetDTCStatus macro in Dem.h). Det_ReportError is provided by the
 * test file itself, so tests/mocks/mock_det.c must NOT be linked.
 */
#include "PduR.h"
#include "Dem.h"
#include "Dcm.h"
#include <string.h>

/* ---- Det_ReportError is provided by test_dcm.c itself ---- */

/* ---- PduR_Transmit recorder ---- */
uint32 stub_PduR_Transmit_calls = 0U;
const uint8 *stub_PduR_lastPdu = NULL_PTR;
PduIdType stub_PduR_lastTxPduId = 0U;
PduLengthType stub_PduR_lastLength = 0U;

/* Static buffer to hold a copy of the PDU data so it survives after the
 * caller's stack frame is destroyed (dcm_transfer.c uses stack-local txBuffer). */
static uint8 stub_PduR_pduCopy[512];

Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    stub_PduR_Transmit_calls++;
    stub_PduR_lastTxPduId = TxPduId;
    if (PduInfoPtr != NULL_PTR && PduInfoPtr->SduDataPtr != NULL_PTR)
    {
        stub_PduR_lastLength = PduInfoPtr->SduLength;
        if (PduInfoPtr->SduLength <= sizeof(stub_PduR_pduCopy))
        {
            memcpy(stub_PduR_pduCopy, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
        }
        stub_PduR_lastPdu = stub_PduR_pduCopy;
    }
    return E_OK;
}

/* ---- Dem stubs (unknown DTC -> E_NOT_OK) ---- */
Std_ReturnType Dem_GetStatusOfDTC(Dem_DtcType DTC, Dem_DTCOriginType DTCOrigin, Dem_UdsStatusByteType* DTCStatus)
{
    (void)DTC;
    (void)DTCOrigin;
    if (DTCStatus != NULL_PTR)
    {
        *DTCStatus = 0U;
    }
    return E_NOT_OK;
}

/* Provide a non-NULL DtcParameters array so Dcm_ProcessReadDTCInformation
 * can safely index Dem_Config.DtcParameters[i].Dtc without NULL deref. */
static const Dem_DtcParameterType stub_DtcParameters[1] = { {0U, 0U, 0U, 0U, FALSE, FALSE, 0U, FALSE} };

const Dem_ConfigType Dem_Config = {
    NULL_PTR,             /* EventParameters */
    0U,                   /* NumEvents */
    stub_DtcParameters,   /* DtcParameters */
    0U,                   /* NumDtcs */
    NULL_PTR, 0U,         /* FreezeFrameRecords, NumFreezeFrameRecords */
    NULL_PTR, 0U,         /* ExtendedDataRecords, NumExtendedDataRecords */
    NULL_PTR, 0U,         /* Indicators, NumIndicators */
    FALSE, FALSE, FALSE,  /* DevErrorDetect, VersionInfoApi, ClearDtcSupported */
    FALSE, FALSE,         /* ClearDtcLimitation, OBDRelevantSupport */
    FALSE, FALSE,         /* J1939Support, TriggerFimReports */
    FALSE, 0U,            /* TriggerMonitorInitBeforeClearOk, DtcStatusAvailabilityMask */
    0, 0, 0              /* notification callbacks */
};

/* ---- Minimal Dcm_Config for test initialization ---- */
const Dcm_ConfigType Dcm_Config = {
    2U,       /* NumProtocols */
    4U,       /* NumConnections */
    4U,       /* NumRxPduIds */
    4U,       /* NumTxPduIds */
    4U,       /* NumSessions */
    3U,       /* NumSecurityLevels */
    16U,      /* NumServices */
    0U,       /* NumDIDs */
    0U,       /* NumRIDs */
    NULL_PTR, /* DIDs */
    NULL_PTR, /* RIDs */
    TRUE,     /* DevErrorDetect */
    TRUE,     /* VersionInfoApi */
    TRUE,     /* RespondAllRequest */
    TRUE      /* DcmTaskTime */
};
