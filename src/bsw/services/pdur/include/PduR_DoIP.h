/*==================================================================================================
 * PduR_DoIP.h - PduR DoIP interface header (AUTOSAR PduR)
 *
 * Declares the PduR <-> DoIP module interface functions.
 *================================================================================================*/
#ifndef PDUR_DOIP_H
#define PDUR_DOIP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"
#include "ComStack_Types.h"

/* PduR -> DoIP (upper layer) */
extern void PduR_DoIPRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/* DoIP -> PduR (lower layer) */
extern Std_ReturnType PduR_DoIPTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
extern void PduR_DoIPTxConfirmation(PduIdType TxPduId, Std_ReturnType result);

#ifdef __cplusplus
}
#endif

#endif /* PDUR_DOIP_H */
