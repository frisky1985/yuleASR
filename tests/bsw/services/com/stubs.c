/**
 * @file stubs.c
 * @brief Minimal dependency stubs for the Com unit test.
 *
 * Com_TransmitIPdu() calls PduR_Transmit(); this stub records every call so
 * the tests can assert on the transmission chain (triggered signals,
 * TxConfirmation handling).
 */

#include "PduR.h"

uint32 mock_PduR_Transmit_Count = 0U;
PduIdType mock_PduR_Transmit_LastPduId = 0;
uint16 mock_PduR_Transmit_LastSduLength = 0;

Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    mock_PduR_Transmit_Count++;
    mock_PduR_Transmit_LastPduId = TxPduId;
    mock_PduR_Transmit_LastSduLength =
        (PduInfoPtr != NULL_PTR) ? PduInfoPtr->SduLength : 0U;
    return E_OK;
}
