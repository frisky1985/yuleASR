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

/* ---- PduR_Transmit recorder ---- */
uint32 stub_PduR_Transmit_calls = 0U;
const uint8 *stub_PduR_lastPdu = NULL_PTR;
PduIdType stub_PduR_lastTxPduId = 0U;
PduLengthType stub_PduR_lastLength = 0U;

Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    stub_PduR_Transmit_calls++;
    stub_PduR_lastTxPduId = TxPduId;
    if (PduInfoPtr != NULL_PTR)
    {
        stub_PduR_lastPdu = PduInfoPtr->SduDataPtr;
        stub_PduR_lastLength = PduInfoPtr->SduLength;
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

const Dem_ConfigType Dem_Config = { 0U };
