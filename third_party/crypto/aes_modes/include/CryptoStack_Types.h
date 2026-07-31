/*==================================================================================================
 * CryptoStack_Types.h - Crypto Stack common types (yuleASR)
 *
 * Placeholder for the crypto stack type header referenced by the AES modes
 * library. The full type set lives in the Crypto driver's Crypto_Types.h;
 * this header only re-exports the base AUTOSAR types needed by the
 * third-party AES library.
 *================================================================================================*/
#ifndef CRYPTOSTACK_TYPES_H
#define CRYPTOSTACK_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

/* Crypto primitive ID types (matching Crypto_Types.h) */
typedef uint32 Crypto_PrimitiveIdType;
typedef uint32 Crypto_KeyIdType;
typedef uint32 Crypto_JobIdType;

#ifdef __cplusplus
}
#endif

#endif /* CRYPTOSTACK_TYPES_H */
