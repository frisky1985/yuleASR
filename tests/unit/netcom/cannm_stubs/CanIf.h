/**
 * @file CanIf.h
 * @brief Minimal CanIf stub for CanNm unit tests (host only).
 *
 * CanNm only calls CanIf_Transmit; the real CanIf.h drags in Can.h and the
 * MCAL register layer. The stub definition lives in test_cannm.c.
 */

#ifndef CANIF_STUB_H
#define CANIF_STUB_H

#include "Std_Types.h"
#include "ComStack_Types.h"

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

#endif /* CANIF_STUB_H */
