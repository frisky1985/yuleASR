/**
 * @file CanIf.h
 * @brief Minimal CanIf stub for J1939Tp unit tests (host only).
 *
 * J1939Tp only calls CanIf_Transmit; the real CanIf.h drags in Can.h and the
 * MCAL register layer. The stub definition lives in test_j1939tp.c.
 */

#ifndef CANIF_STUB_H
#define CANIF_STUB_H

#include "Std_Types.h"
#include "ComStack_Types.h"

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

#endif /* CANIF_STUB_H */
