/**********************************************************************************************************************
 * @file       Crypto_Backend.c
 * @brief      Crypto Driver backend dispatch implementation
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-11-28
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      Holds the backend vtables for the software (mbedTLS) and HSE/HSM
 *      providers and dispatches the primitive calls used by Crypto.c.
 *      The software provider is the default so that enabling the abstraction
 *      does not change runtime behaviour.
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Crypto_Backend.h"

#if defined(CRYPTO_BACKEND_HSM_AVAILABLE)
#include "Crypto_Cfg.h"
#endif

/**********************************************************************************************************************
 * EXTERNAL DECLARATIONS - mbedTLS software backend
 *********************************************************************************************************************/
extern Std_ReturnType Crypto_MbedTLS_Init(void);
extern void           Crypto_MbedTLS_DeInit(void);
extern Std_ReturnType Crypto_MbedTLS_ProcessJob(Crypto_JobType* job);
extern Std_ReturnType Crypto_MbedTLS_RandomGenerate(uint8* resultPtr, uint32 resultLength);
extern Std_ReturnType Crypto_MbedTLS_KeyGenerate(Crypto_KeyIdType keyId);
extern Std_ReturnType Crypto_MbedTLS_KeyDerive(Crypto_KeyIdType srcKeyId, Crypto_KeyIdType dstKeyId);
extern Std_ReturnType Crypto_MbedTLS_ECDH_CalcSecret(Crypto_KeyIdType privKeyId, const uint8* pubKeyPtr,
                                                     uint32 pubKeyLen);
extern Std_ReturnType Crypto_MbedTLS_ECDSA_Sign(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen,
                                                uint8* sig, uint32* sigLen);
extern Std_ReturnType Crypto_MbedTLS_ECDSA_Verify(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen,
                                                  const uint8* sig, uint32 sigLen);
extern Std_ReturnType Crypto_MbedTLS_AES_GCM_Encrypt(Crypto_KeyIdType keyId, const uint8* pt, uint32 ptLen,
                                                     const uint8* aad, uint32 aadLen, const uint8* iv, uint8* ct,
                                                     uint8* tag);
extern Std_ReturnType Crypto_MbedTLS_AES_GCM_Decrypt(Crypto_KeyIdType keyId, const uint8* ct, uint32 ctLen,
                                                     const uint8* aad, uint32 aadLen, const uint8* iv,
                                                     const uint8* tag, uint8* pt);
extern Std_ReturnType Crypto_MbedTLS_SHA256(const uint8* data, uint32 dataLen, uint8* digest);
extern Std_ReturnType Crypto_MbedTLS_HMAC(Crypto_KeyIdType keyId, const uint8* data, uint32 dataLen, uint8* mac);
extern Std_ReturnType Crypto_MbedTLS_HKDF(Crypto_KeyIdType ikmKeyId, const uint8* salt, uint32 saltLen,
                                          const uint8* info, uint32 infoLen, uint8* okm, uint32 okmLen);

#if defined(CRYPTO_BACKEND_HSM_AVAILABLE)
extern Std_ReturnType Crypto_Hsm_Init(const Crypto_HsmConfigType* config);
extern void           Crypto_Hsm_DeInit(void);
extern Std_ReturnType Crypto_Hsm_ProcessJob(const Crypto_JobType* job);
#endif

/**********************************************************************************************************************
 * LOCAL DATA - mbedTLS software provider
 *********************************************************************************************************************/
static const Crypto_BackendOpsType Crypto_BackendSoftwareOps = {
    Crypto_MbedTLS_Init,
    Crypto_MbedTLS_DeInit,
    Crypto_MbedTLS_ProcessJob,
    Crypto_MbedTLS_RandomGenerate,
    Crypto_MbedTLS_KeyGenerate,
    Crypto_MbedTLS_KeyDerive,
    Crypto_MbedTLS_ECDH_CalcSecret,
    Crypto_MbedTLS_ECDSA_Sign,
    Crypto_MbedTLS_ECDSA_Verify,
    Crypto_MbedTLS_AES_GCM_Encrypt,
    Crypto_MbedTLS_AES_GCM_Decrypt,
    Crypto_MbedTLS_SHA256,
    Crypto_MbedTLS_HMAC,
    Crypto_MbedTLS_HKDF
};

static const Crypto_BackendProviderType Crypto_BackendSoftware = {
    "mbedtls-sw",
    FALSE,
    &Crypto_BackendSoftwareOps
};

/**********************************************************************************************************************
 * LOCAL FUNCTIONS - HSE/HSM provider
 *********************************************************************************************************************/
static Std_ReturnType Crypto_BackendHse_Init(void)
{
#if defined(CRYPTO_BACKEND_HSM_AVAILABLE)
    return Crypto_Hsm_Init(&Crypto_Config.hsmConfig);
#else
    return E_NOT_OK;
#endif
}

static void Crypto_BackendHse_DeInit(void)
{
#if defined(CRYPTO_BACKEND_HSM_AVAILABLE)
    Crypto_Hsm_DeInit();
#endif
}

static Std_ReturnType Crypto_BackendHse_ProcessJob(Crypto_JobType* job)
{
#if defined(CRYPTO_BACKEND_HSM_AVAILABLE)
    return Crypto_Hsm_ProcessJob(job);
#else
    (void)job;
    return E_NOT_OK;
#endif
}

static Std_ReturnType Crypto_BackendHse_NotSupported(void)
{
    /* The S32K3 HSE primitive mapping is not part of this adaptation skeleton.
     * Only init/deinit/job processing are wired; everything else falls back to
     * the software provider through the driver's HSM fallback policy. */
    return E_NOT_OK;
}

static Std_ReturnType Crypto_BackendHse_RandomGenerate(uint8* resultPtr, uint32 resultLength)
{
    (void)resultPtr;
    (void)resultLength;
    return Crypto_BackendHse_NotSupported();
}

static Std_ReturnType Crypto_BackendHse_KeyGenerate(Crypto_KeyIdType keyId)
{
    (void)keyId;
    return Crypto_BackendHse_NotSupported();
}

static Std_ReturnType Crypto_BackendHse_KeyDerive(Crypto_KeyIdType srcKeyId, Crypto_KeyIdType dstKeyId)
{
    (void)srcKeyId;
    (void)dstKeyId;
    return Crypto_BackendHse_NotSupported();
}

static Std_ReturnType Crypto_BackendHse_EcdhCalcSecret(Crypto_KeyIdType privKeyId, const uint8* pubKeyPtr,
                                                       uint32 pubKeyLen)
{
    (void)privKeyId;
    (void)pubKeyPtr;
    (void)pubKeyLen;
    return Crypto_BackendHse_NotSupported();
}

static Std_ReturnType Crypto_BackendHse_EcdsaSign(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen,
                                                  uint8* sig, uint32* sigLen)
{
    (void)keyId;
    (void)digest;
    (void)digestLen;
    (void)sig;
    (void)sigLen;
    return Crypto_BackendHse_NotSupported();
}

static Std_ReturnType Crypto_BackendHse_EcdsaVerify(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen,
                                                    const uint8* sig, uint32 sigLen)
{
    (void)keyId;
    (void)digest;
    (void)digestLen;
    (void)sig;
    (void)sigLen;
    return Crypto_BackendHse_NotSupported();
}

static Std_ReturnType Crypto_BackendHse_AesGcmEncrypt(Crypto_KeyIdType keyId, const uint8* pt, uint32 ptLen,
                                                      const uint8* aad, uint32 aadLen, const uint8* iv, uint8* ct,
                                                      uint8* tag)
{
    (void)keyId;
    (void)pt;
    (void)ptLen;
    (void)aad;
    (void)aadLen;
    (void)iv;
    (void)ct;
    (void)tag;
    return Crypto_BackendHse_NotSupported();
}

static Std_ReturnType Crypto_BackendHse_AesGcmDecrypt(Crypto_KeyIdType keyId, const uint8* ct, uint32 ctLen,
                                                      const uint8* aad, uint32 aadLen, const uint8* iv,
                                                      const uint8* tag, uint8* pt)
{
    (void)keyId;
    (void)ct;
    (void)ctLen;
    (void)aad;
    (void)aadLen;
    (void)iv;
    (void)tag;
    (void)pt;
    return Crypto_BackendHse_NotSupported();
}

static Std_ReturnType Crypto_BackendHse_Sha256(const uint8* data, uint32 dataLen, uint8* digest)
{
    (void)data;
    (void)dataLen;
    (void)digest;
    return Crypto_BackendHse_NotSupported();
}

static Std_ReturnType Crypto_BackendHse_Hmac(Crypto_KeyIdType keyId, const uint8* data, uint32 dataLen, uint8* mac)
{
    (void)keyId;
    (void)data;
    (void)dataLen;
    (void)mac;
    return Crypto_BackendHse_NotSupported();
}

static Std_ReturnType Crypto_BackendHse_Hkdf(Crypto_KeyIdType ikmKeyId, const uint8* salt, uint32 saltLen,
                                             const uint8* info, uint32 infoLen, uint8* okm, uint32 okmLen)
{
    (void)ikmKeyId;
    (void)salt;
    (void)saltLen;
    (void)info;
    (void)infoLen;
    (void)okm;
    (void)okmLen;
    return Crypto_BackendHse_NotSupported();
}

static const Crypto_BackendOpsType Crypto_BackendHseOps = {
    Crypto_BackendHse_Init,
    Crypto_BackendHse_DeInit,
    Crypto_BackendHse_ProcessJob,
    Crypto_BackendHse_RandomGenerate,
    Crypto_BackendHse_KeyGenerate,
    Crypto_BackendHse_KeyDerive,
    Crypto_BackendHse_EcdhCalcSecret,
    Crypto_BackendHse_EcdsaSign,
    Crypto_BackendHse_EcdsaVerify,
    Crypto_BackendHse_AesGcmEncrypt,
    Crypto_BackendHse_AesGcmDecrypt,
    Crypto_BackendHse_Sha256,
    Crypto_BackendHse_Hmac,
    Crypto_BackendHse_Hkdf
};

static const Crypto_BackendProviderType Crypto_BackendHse = {
    "s32k3-hse",
    TRUE,
    &Crypto_BackendHseOps
};

/**********************************************************************************************************************
 * LOCAL DATA - active provider
 *********************************************************************************************************************/
static const Crypto_BackendProviderType* Crypto_BackendActive = &Crypto_BackendSoftware;

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS
 *********************************************************************************************************************/
const Crypto_BackendProviderType* Crypto_Backend_GetSoftware(void)
{
    return &Crypto_BackendSoftware;
}

const Crypto_BackendProviderType* Crypto_Backend_GetHse(void)
{
    return &Crypto_BackendHse;
}

boolean Crypto_Backend_IsHseAvailable(void)
{
#if defined(CRYPTO_BACKEND_HSM_AVAILABLE)
    return TRUE;
#else
    return FALSE;
#endif
}

Std_ReturnType Crypto_Backend_Select(const Crypto_BackendProviderType* provider)
{
    if (provider == NULL_PTR)
    {
        Crypto_BackendActive = &Crypto_BackendSoftware;
        return E_OK;
    }
    if ((provider == &Crypto_BackendHse) && (Crypto_Backend_IsHseAvailable() == FALSE))
    {
        return E_NOT_OK;
    }
    Crypto_BackendActive = provider;
    return E_OK;
}

const Crypto_BackendProviderType* Crypto_Backend_GetActive(void)
{
    return Crypto_BackendActive;
}

Std_ReturnType Crypto_Backend_Init(void)
{
    return Crypto_BackendActive->ops->Init();
}

void Crypto_Backend_DeInit(void)
{
    Crypto_BackendActive->ops->DeInit();
}

Std_ReturnType Crypto_Backend_ProcessJob(Crypto_JobType* job)
{
    return Crypto_BackendActive->ops->ProcessJob(job);
}

Std_ReturnType Crypto_Backend_RandomGenerate(uint8* resultPtr, uint32 resultLength)
{
    return Crypto_BackendActive->ops->RandomGenerate(resultPtr, resultLength);
}

Std_ReturnType Crypto_Backend_KeyGenerate(Crypto_KeyIdType keyId)
{
    return Crypto_BackendActive->ops->KeyGenerate(keyId);
}

Std_ReturnType Crypto_Backend_KeyDerive(Crypto_KeyIdType srcKeyId, Crypto_KeyIdType dstKeyId)
{
    return Crypto_BackendActive->ops->KeyDerive(srcKeyId, dstKeyId);
}

Std_ReturnType Crypto_Backend_EcdhCalcSecret(Crypto_KeyIdType privKeyId, const uint8* pubKeyPtr, uint32 pubKeyLen)
{
    return Crypto_BackendActive->ops->EcdhCalcSecret(privKeyId, pubKeyPtr, pubKeyLen);
}

Std_ReturnType Crypto_Backend_EcdsaSign(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen, uint8* sig,
                                        uint32* sigLen)
{
    return Crypto_BackendActive->ops->EcdsaSign(keyId, digest, digestLen, sig, sigLen);
}

Std_ReturnType Crypto_Backend_EcdsaVerify(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen,
                                          const uint8* sig, uint32 sigLen)
{
    return Crypto_BackendActive->ops->EcdsaVerify(keyId, digest, digestLen, sig, sigLen);
}

Std_ReturnType Crypto_Backend_AesGcmEncrypt(Crypto_KeyIdType keyId, const uint8* pt, uint32 ptLen, const uint8* aad,
                                            uint32 aadLen, const uint8* iv, uint8* ct, uint8* tag)
{
    return Crypto_BackendActive->ops->AesGcmEncrypt(keyId, pt, ptLen, aad, aadLen, iv, ct, tag);
}

Std_ReturnType Crypto_Backend_AesGcmDecrypt(Crypto_KeyIdType keyId, const uint8* ct, uint32 ctLen, const uint8* aad,
                                            uint32 aadLen, const uint8* iv, const uint8* tag, uint8* pt)
{
    return Crypto_BackendActive->ops->AesGcmDecrypt(keyId, ct, ctLen, aad, aadLen, iv, tag, pt);
}

Std_ReturnType Crypto_Backend_Sha256(const uint8* data, uint32 dataLen, uint8* digest)
{
    return Crypto_BackendActive->ops->Sha256(data, dataLen, digest);
}

Std_ReturnType Crypto_Backend_Hmac(Crypto_KeyIdType keyId, const uint8* data, uint32 dataLen, uint8* mac)
{
    return Crypto_BackendActive->ops->Hmac(keyId, data, dataLen, mac);
}

Std_ReturnType Crypto_Backend_Hkdf(Crypto_KeyIdType ikmKeyId, const uint8* salt, uint32 saltLen, const uint8* info,
                                   uint32 infoLen, uint8* okm, uint32 okmLen)
{
    return Crypto_BackendActive->ops->Hkdf(ikmKeyId, salt, saltLen, info, infoLen, okm, okmLen);
}

/**********************************************************************************************************************
 * END OF FILE
 *********************************************************************************************************************/
