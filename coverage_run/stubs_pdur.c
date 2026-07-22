/**
 * @file stubs_pdur.c
 * @brief Minimal stubs for PduR external dependencies (coverage test)
 */
#include "ComStack_Types.h"
#include "PduR.h"
#include "Det.h"

/* CanIf stubs */
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    (void)TxPduId; (void)PduInfoPtr;
    return E_OK;
}
Std_ReturnType CanIf_CancelTransmit(PduIdType TxPduId) {
    (void)TxPduId;
    return E_OK;
}

/* Com stubs */
void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)RxPduId; (void)PduInfoPtr;
}
void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    (void)TxPduId; (void)result;
}
Std_ReturnType Com_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr) {
    (void)TxPduId; (void)PduInfoPtr;
    return E_OK;
}

/* Dcm stubs */
void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)RxPduId; (void)PduInfoPtr;
}
void Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    (void)TxPduId; (void)result;
}
Std_ReturnType Dcm_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr) {
    (void)TxPduId; (void)PduInfoPtr;
    return E_OK;
}
