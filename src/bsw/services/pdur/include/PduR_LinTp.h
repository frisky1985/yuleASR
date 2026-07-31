/*==================================================================================================
 * PduR_LinTp.h - PduR LinTp interface header (AUTOSAR PduR)
 *
 * Declares the PduR <-> LinTp module interface functions.
 *================================================================================================*/
#ifndef PDUR_LINTP_H
#define PDUR_LINTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"
#include "ComStack_Types.h"

/* PduR -> LinTp (upper layer) */
extern void PduR_LinTpRxIndication(PduIdType RxPduId, Std_ReturnType result);
extern void PduR_LinTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
extern BufReq_ReturnType PduR_LinTpStartOfReception(PduIdType RxPduId,
                                                    const PduInfoType* PduInfoPtr,
                                                    PduLengthType TpSduLength,
                                                    PduLengthType* BufferSizePtr);
extern BufReq_ReturnType PduR_LinTpCopyRxData(PduIdType RxPduId,
                                              const PduInfoType* PduInfoPtr,
                                              PduLengthType* BufferSizePtr);
extern BufReq_ReturnType PduR_LinTpCopyTxData(PduIdType TxPduId,
                                              const PduInfoType* PduInfoPtr,
                                              const RetryInfoType* RetryInfoPtr,
                                              PduLengthType* AvailableDataPtr);
extern BufReq_ReturnType PduR_LinTpProvideTxBuffer(PduIdType TxPduId,
                                                   PduInfoType** PduInfoPtr,
                                                   PduLengthType* AvailableDataPtr);

#ifdef __cplusplus
}
#endif

#endif /* PDUR_LINTP_H */
