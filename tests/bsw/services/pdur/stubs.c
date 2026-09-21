/**
 * @file stubs.c
 * @brief Lower/upper layer stubs for the PduR unit test.
 *
 * PduR.c routes PDUs by calling CanIf_Transmit/CanIf_CancelTransmit (downward),
 * Com_RxIndication/Com_TxConfirmation/Com_TriggerTransmit and the Dcm
 * counterparts (upward). These stubs record every call so the tests can
 * assert on the routing chain (destination PDU ids, SDU lengths, results).
 */

#include "PduR.h"

uint32 mock_CanIf_Transmit_Count = 0U;
PduIdType mock_CanIf_Transmit_LastPduId = 0;
PduLengthType mock_CanIf_Transmit_LastSduLength = 0;

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    mock_CanIf_Transmit_Count++;
    mock_CanIf_Transmit_LastPduId = TxPduId;
    mock_CanIf_Transmit_LastSduLength =
        (PduInfoPtr != NULL_PTR) ? PduInfoPtr->SduLength : 0U;
    return E_OK;
}

uint32 mock_CanIf_CancelTransmit_Count = 0U;
PduIdType mock_CanIf_CancelTransmit_LastPduId = 0;

Std_ReturnType CanIf_CancelTransmit(PduIdType TxPduId) {
    mock_CanIf_CancelTransmit_Count++;
    mock_CanIf_CancelTransmit_LastPduId = TxPduId;
    return E_OK;
}

uint32 mock_Com_RxIndication_Count = 0U;
PduIdType mock_Com_RxIndication_LastPduId = 0;

void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)PduInfoPtr;
    mock_Com_RxIndication_Count++;
    mock_Com_RxIndication_LastPduId = RxPduId;
}

uint32 mock_Com_TxConfirmation_Count = 0U;
PduIdType mock_Com_TxConfirmation_LastPduId = 0;
Std_ReturnType mock_Com_TxConfirmation_LastResult = E_NOT_OK;

void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    mock_Com_TxConfirmation_Count++;
    mock_Com_TxConfirmation_LastPduId = TxPduId;
    mock_Com_TxConfirmation_LastResult = result;
}

uint32 mock_Com_TriggerTransmit_Count = 0U;
PduIdType mock_Com_TriggerTransmit_LastPduId = 0;

Std_ReturnType Com_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr) {
    (void)PduInfoPtr;
    mock_Com_TriggerTransmit_Count++;
    mock_Com_TriggerTransmit_LastPduId = TxPduId;
    return E_OK;
}

uint32 mock_Dcm_RxIndication_Count = 0U;

void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)RxPduId;
    (void)PduInfoPtr;
    mock_Dcm_RxIndication_Count++;
}

uint32 mock_Dcm_TxConfirmation_Count = 0U;

void Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    (void)TxPduId;
    (void)result;
    mock_Dcm_TxConfirmation_Count++;
}

uint32 mock_Dcm_TriggerTransmit_Count = 0U;

Std_ReturnType Dcm_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr) {
    (void)TxPduId;
    (void)PduInfoPtr;
    mock_Dcm_TriggerTransmit_Count++;
    return E_OK;
}
