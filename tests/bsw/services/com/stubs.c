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
uint8 mock_PduR_Transmit_LastSdu[8] = {0U};

Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    uint16 i;
    mock_PduR_Transmit_Count++;
    mock_PduR_Transmit_LastPduId = TxPduId;
    mock_PduR_Transmit_LastSduLength =
        (PduInfoPtr != NULL_PTR) ? PduInfoPtr->SduLength : 0U;
    for (i = 0U; i < 8U; i++) { mock_PduR_Transmit_LastSdu[i] = 0U; }
    if ((PduInfoPtr != NULL_PTR) && (PduInfoPtr->SduDataPtr != NULL_PTR)) {
        uint16 copy = (PduInfoPtr->SduLength < 8U) ? PduInfoPtr->SduLength : 8U;
        for (i = 0U; i < copy; i++) {
            mock_PduR_Transmit_LastSdu[i] = PduInfoPtr->SduDataPtr[i];
        }
    }
    return E_OK;
}
