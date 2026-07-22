/**
 * @file stubs_com.c
 * @brief Minimal stubs for Com external dependencies (not provided by Com.c)
 */
#include "ComStack_Types.h"
#include "PduR.h"
#include "Det.h"

/* PduR stubs for Com module (Com.c calls PduR_Transmit) */
Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    (void)TxPduId; (void)PduInfoPtr;
    return E_OK;
}

/* Dcm stubs (Com.c may reference Dcm via PduR layer) */
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
