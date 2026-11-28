/**********************************************************************************************************************
 * @file       Crypto_Backend.h
 * @brief      Crypto Driver backend dispatch abstraction
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-11-28
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      Backend vtable used by Crypto.c to decouple the AUTOSAR Crypto Driver
 *      API from the concrete primitive provider (software mbedTLS, or the
 *      NXP S32K3 HSE / HSM hardware engine).
 *
 *      The default provider is the mbedTLS software backend, so behaviour is
 *      identical to the previous direct Crypto_MbedTLS_* calls.  The HSE
 *      provider is compiled in as a stub that reports "not available" unless
 *      CRYPTO_BACKEND_HSM_AVAILABLE is defined by the target build.
 *********************************************************************************************************************/

#ifndef CRYPTO_BACKEND_H
#define CRYPTO_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Std_Types.h"
#include "Crypto_Types.h"

/**********************************************************************************************************************
 * GLOBAL DATA TYPES AND STRUCTURES
 *********************************************************************************************************************/

/** @brief Primitive operations implemented by a crypto backend */
typedef struct
{
    Std_ReturnType (*Init)(void);
    void           (*DeInit)(void);
    Std_ReturnType (*ProcessJob)(Crypto_JobType* job);
    Std_ReturnType (*RandomGenerate)(uint8* resultPtr, uint32 resultLength);
    Std_ReturnType (*KeyGenerate)(Crypto_KeyIdType keyId);
    Std_ReturnType (*KeyDerive)(Crypto_KeyIdType srcKeyId, Crypto_KeyIdType dstKeyId);
    Std_ReturnType (*EcdhCalcSecret)(Crypto_KeyIdType privKeyId, const uint8* pubKeyPtr, uint32 pubKeyLen);
    Std_ReturnType (*EcdsaSign)(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen, uint8* sig,
                                uint32* sigLen);
    Std_ReturnType (*EcdsaVerify)(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen, const uint8* sig,
                                  uint32 sigLen);
    Std_ReturnType (*AesGcmEncrypt)(Crypto_KeyIdType keyId, const uint8* pt, uint32 ptLen, const uint8* aad,
                                    uint32 aadLen, const uint8* iv, uint8* ct, uint8* tag);
    Std_ReturnType (*AesGcmDecrypt)(Crypto_KeyIdType keyId, const uint8* ct, uint32 ctLen, const uint8* aad,
                                    uint32 aadLen, const uint8* iv, const uint8* tag, uint8* pt);
    Std_ReturnType (*Sha256)(const uint8* data, uint32 dataLen, uint8* digest);
    Std_ReturnType (*Hmac)(Crypto_KeyIdType keyId, const uint8* data, uint32 dataLen, uint8* mac);
    Std_ReturnType (*Hkdf)(Crypto_KeyIdType ikmKeyId, const uint8* salt, uint32 saltLen, const uint8* info,
                           uint32 infoLen, uint8* okm, uint32 okmLen);
} Crypto_BackendOpsType;

/** @brief A selectable crypto backend */
typedef struct
{
    const char*                 name;       /**< Human readable backend name */
    boolean                     hsmBacked;  /**< TRUE if primitives run on an HSM/HSE */
    const Crypto_BackendOpsType* ops;       /**< Primitive operation table */
} Crypto_BackendProviderType;

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/

/** @brief Returns the software (mbedTLS) provider - always available */
const Crypto_BackendProviderType* Crypto_Backend_GetSoftware(void);

/** @brief Returns the HSE/HSM provider - operations return E_NOT_OK when unavailable */
const Crypto_BackendProviderType* Crypto_Backend_GetHse(void);

/** @brief TRUE when the HSE provider is compiled in and usable */
boolean Crypto_Backend_IsHseAvailable(void);

/**
 * @brief Selects the active backend.
 * @param provider Provider to activate; NULL_PTR restores the software default.
 * @return E_OK when selected, E_NOT_OK for an unavailable provider.
 */
Std_ReturnType Crypto_Backend_Select(const Crypto_BackendProviderType* provider);

/** @brief Returns the currently active provider (never NULL_PTR) */
const Crypto_BackendProviderType* Crypto_Backend_GetActive(void);

/* Dispatch helpers - route to the active provider's operation table */
Std_ReturnType Crypto_Backend_Init(void);
void           Crypto_Backend_DeInit(void);
Std_ReturnType Crypto_Backend_ProcessJob(Crypto_JobType* job);
Std_ReturnType Crypto_Backend_RandomGenerate(uint8* resultPtr, uint32 resultLength);
Std_ReturnType Crypto_Backend_KeyGenerate(Crypto_KeyIdType keyId);
Std_ReturnType Crypto_Backend_KeyDerive(Crypto_KeyIdType srcKeyId, Crypto_KeyIdType dstKeyId);
Std_ReturnType Crypto_Backend_EcdhCalcSecret(Crypto_KeyIdType privKeyId, const uint8* pubKeyPtr, uint32 pubKeyLen);
Std_ReturnType Crypto_Backend_EcdsaSign(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen, uint8* sig,
                                        uint32* sigLen);
Std_ReturnType Crypto_Backend_EcdsaVerify(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen,
                                          const uint8* sig, uint32 sigLen);
Std_ReturnType Crypto_Backend_AesGcmEncrypt(Crypto_KeyIdType keyId, const uint8* pt, uint32 ptLen, const uint8* aad,
                                            uint32 aadLen, const uint8* iv, uint8* ct, uint8* tag);
Std_ReturnType Crypto_Backend_AesGcmDecrypt(Crypto_KeyIdType keyId, const uint8* ct, uint32 ctLen, const uint8* aad,
                                            uint32 aadLen, const uint8* iv, const uint8* tag, uint8* pt);
Std_ReturnType Crypto_Backend_Sha256(const uint8* data, uint32 dataLen, uint8* digest);
Std_ReturnType Crypto_Backend_Hmac(Crypto_KeyIdType keyId, const uint8* data, uint32 dataLen, uint8* mac);
Std_ReturnType Crypto_Backend_Hkdf(Crypto_KeyIdType ikmKeyId, const uint8* salt, uint32 saltLen, const uint8* info,
                                   uint32 infoLen, uint8* okm, uint32 okmLen);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_BACKEND_H */
