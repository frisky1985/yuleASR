/**
 * @file SoAd.h
 * @brief Minimal SoAd stub for DoIP unit tests (host only).
 *
 * DoIP only calls SoAd_IfTransmit / SoAd_CloseConnection; the real SoAd.h
 * drags in TcpIp.h and the socket configuration chain. Stub definitions
 * live in test_doip.c. Signatures match the real
 * src/bsw/services/soad/include/SoAd.h.
 */

#ifndef SOAD_STUB_H
#define SOAD_STUB_H

#include "Std_Types.h"
#include "ComStack_Types.h"

typedef uint16 SoAd_SoConIdType;

Std_ReturnType SoAd_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
Std_ReturnType SoAd_CloseConnection(SoAd_SoConIdType SoConId);

#endif /* SOAD_STUB_H */
