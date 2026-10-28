/**
 * @file PduR.h
 * @brief Minimal PduR stub for FrTp unit tests (host only).
 *
 * FrTp only calls the two TP callback entry points below; the real PduR.h
 * drags in PduR_Cfg.h and the full routing configuration, which the unit
 * tests do not need. The stub definitions live in test_frtp.c.
 */

#ifndef PDUR_STUB_H
#define PDUR_STUB_H

#include "Std_Types.h"
#include "ComStack_Types.h"

void PduR_FrTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
void PduR_FrTpRxIndication(PduIdType RxPduId, Std_ReturnType result);

#endif /* PDUR_STUB_H */
