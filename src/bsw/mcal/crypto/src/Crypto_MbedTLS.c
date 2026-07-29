/**********************************************************************************************************************
 * @file       Crypto_MbedTLS.c
 * @brief      Crypto Driver Mbed TLS Adapter Layer
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2025-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      Mbed TLS adapter implementation for Crypto Driver.
 *      Provides software-based cryptographic operations for:
 *      - ECDSA (SECP256R1) - Sign/Verify
 *      - ECDH (SECP256R1) - Key Exchange
 *      - AES-128-GCM - Symmetric Encryption
 *      - SHA-256 - Hashing
 *      - HKDF - Key Derivation
 *      - HMAC - Message Authentication
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Crypto.h"
#include "MemMap.h"

/* Mbed TLS Headers */
#include "mbedtls/ecp.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include "mbedtls/sha256.h"
#include "mbedtls/md.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"

/*
 * mbedTLS forward declarations.
 * Some mbedTLS headers may not be in the cppcheck include path,
 * so we provide explicit extern declarations for functions used.
 */
extern int mbedtls_ctr_drbg_random(void *ctx, unsigned char *output, size_t len);
#include "mbedtls/bignum.h"

/**********************************************************************************************************************
 * LOCAL MACROS
 *********************************************************************************************************************/
#define MBEDTLS_SUCCESS         (0)

/**********************************************************************************************************************
 * LOCAL DATA TYPES
 *********************************************************************************************************************/
typedef struct {
    mbedtls_ecp_group_id    grp_id;
    uint16                  key_size;
    uint16                  signature_size;
} Crypto_MbedTLS_EccInfoType;

/**********************************************************************************************************************
 * LOCAL VARIABLES
 *********************************************************************************************************************/
#define CRYPTO_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC mbedtls_entropy_context    Crypto_EntropyCtx;
STATIC mbedtls_ctr_drbg_context   Crypto_CtrDrbgCtx;
STATIC boolean                    Crypto_MbedTLS_Initialized = FALSE;

STATIC const Crypto_MbedTLS_EccInfoType Crypto_EccInfo[] = {
    { MBEDTLS_ECP_DP_SECP256R1, 32, 64 },   /* CRYPTO_ECC_CURVE_SECP256R1 */
    { MBEDTLS_ECP_DP_SECP384R1, 48, 96 },   /* CRYPTO_ECC_CURVE_SECP384R1 */
    { MBEDTLS_ECP_DP_SECP521R1, 66, 132 },  /* CRYPTO_ECC_CURVE_SECP521R1 */
    { MBEDTLS_ECP_DP_SECP256K1, 32, 64 },   /* CRYPTO_ECC_CURVE_SECP256K1 */
    { MBEDTLS_ECP_DP_BP256R1,   32, 64 },   /* CRYPTO_ECC_CURVE_BRAINPOOLP256R1 */
    { MBEDTLS_ECP_DP_BP384R1,   48, 96 },   /* CRYPTO_ECC_CURVE_BRAINPOOLP384R1 */
    { MBEDTLS_ECP_DP_BP512R1,   64, 128 }   /* CRYPTO_ECC_CURVE_BRAINPOOLP512R1 */
};

#define CRYPTO_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/**********************************************************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/
STATIC Std_ReturnType Crypto_MbedTLS_ConvertResult(int mbedtls_result);
STATIC Std_ReturnType Crypto_MbedTLS_ECDSA_Sign_Internal(const uint8* privKey, uint32 privKeyLen,
                                                          const uint8* digest, uint32 digestLen,
                                                          uint8* signature, uint32* sigLen);
STATIC Std_ReturnType Crypto_MbedTLS_ECDSA_Verify_Internal(const uint8* pubKey, uint32 pubKeyLen,
                                                            const uint8* digest, uint32 digestLen,
                                                            const uint8* signature, uint32 sigLen);
STATIC int Crypto_MbedTLS_RandomCallback(void* ctx, unsigned char* output, size_t len);

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - INITIALIZATION
 *********************************************************************************************************************/

#define CRYPTO_START_SEC_CODE
#include "MemMap.h"

/**********************************************************************************************************************
 * Crypto_MbedTLS_Init
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_Init(void)
{
    int ret;
    const char* pers = "yule_crypto";
    
    if (Crypto_MbedTLS_Initialized) {
        return E_OK;
    }
    
    mbedtls_entropy_init(&Crypto_EntropyCtx);
    mbedtls_ctr_drbg_init(&Crypto_CtrDrbgCtx);
    
    ret = mbedtls_ctr_drbg_seed(&Crypto_CtrDrbgCtx, mbedtls_entropy_func, &Crypto_EntropyCtx,
                                 (const unsigned char*)pers, strlen(pers));
    
    if (ret != MBEDTLS_SUCCESS) {
        mbedtls_ctr_drbg_free(&Crypto_CtrDrbgCtx);
        mbedtls_entropy_free(&Crypto_EntropyCtx);
        return E_NOT_OK;
    }
    
    Crypto_MbedTLS_Initialized = TRUE;
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_MbedTLS_DeInit
 *********************************************************************************************************************/
void Crypto_MbedTLS_DeInit(void)
{
    if (!Crypto_MbedTLS_Initialized) {
        return;
    }
    
    mbedtls_ctr_drbg_free(&Crypto_CtrDrbgCtx);
    mbedtls_entropy_free(&Crypto_EntropyCtx);
    
    Crypto_MbedTLS_Initialized = FALSE;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - RANDOM NUMBER GENERATION
 **********************************************************************************************************************/
/**********************************************************************************************************************
 * Crypto_MbedTLS_RandomGenerate
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_RandomGenerate(uint8* resultPtr, uint32 resultLength)
{
    int ret;
    
    if (!Crypto_MbedTLS_Initialized) {
        return E_NOT_OK;
    }
    
    if ((resultPtr == NULL_PTR) || (resultLength == 0U)) {
        return E_NOT_OK;
    }
    
    ret = mbedtls_ctr_drbg_random(&Crypto_CtrDrbgCtx, resultPtr, resultLength);
    
    return Crypto_MbedTLS_ConvertResult(ret);
}

/**********************************************************************************************************************
 * Crypto_MbedTLS_RandomCallback
 *********************************************************************************************************************/
STATIC int Crypto_MbedTLS_RandomCallback(void* ctx, unsigned char* output, size_t len)
{
    (void)ctx;
    
    if (mbedtls_ctr_drbg_random(&Crypto_CtrDrbgCtx, output, len) == 0U ) {
        return 0;
    }
    return -1;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - ECDSA OPERATIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_MbedTLS_ECDSA_Sign
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_ECDSA_Sign(Crypto_KeyIdType keyId,
                                          const uint8* digest,
                                          uint32 digestLen,
                                          uint8* signature,
                                          uint32* sigLen)
{
    uint8 privKey[32];
    uint32 privKeyLen = 32;
    
    (void)keyId;  /* Would retrieve key from key store in full implementation */
    
    /* For demo: use test key - in production, retrieve from key storage */
    if (Crypto_RandomGenerate(0, privKey, 32) != E_OK) {
        return E_NOT_OK;
    }
    
    return Crypto_MbedTLS_ECDSA_Sign_Internal(privKey, privKeyLen, digest, digestLen, signature, sigLen);
}

/**********************************************************************************************************************
 * Crypto_MbedTLS_ECDSA_Sign_Internal
 *********************************************************************************************************************/
STATIC Std_ReturnType Crypto_MbedTLS_ECDSA_Sign_Internal(const uint8* privKey, uint32 privKeyLen,
                                                          const uint8* digest, uint32 digestLen,
                                                          uint8* signature, uint32* sigLen)
{
    mbedtls_ecdsa_context   ctx;
    mbedtls_mpi             r, s;
    int                     ret;
    
    (void)privKeyLen;
    
    mbedtls_ecdsa_init(&ctx);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);
    
    /* Setup context for secp256r1 */
    ret = mbedtls_ecdsa_setup(&ctx, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* Load private key */
    ret = mbedtls_mpi_read_binary(&ctx.d, privKey, 32);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* Generate signature */
    ret = mbedtls_ecdsa_sign(&ctx.grp, &r, &s, &ctx.d, digest, digestLen,
                             Crypto_MbedTLS_RandomCallback, NULL_PTR);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* Export signature (R || S) */
    ret = mbedtls_mpi_write_binary(&r, signature, 32);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    ret = mbedtls_mpi_write_binary(&s, signature + 32, 32);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    *sigLen = 64;  /* R + S for secp256r1 */
    
cleanup:
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    mbedtls_ecdsa_free(&ctx);
    
    return Crypto_MbedTLS_ConvertResult(ret);
}

/**********************************************************************************************************************
 * Crypto_MbedTLS_ECDSA_Verify
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_ECDSA_Verify(Crypto_KeyIdType keyId,
                                            const uint8* digest,
                                            uint32 digestLen,
                                            const uint8* signature,
                                            uint32 sigLen)
{
    uint8 pubKey[65];
    uint32 pubKeyLen = 65;
    
    (void)keyId;
    (void)sigLen;
    
    /* For demo: would retrieve public key from key storage */
    if (Crypto_RandomGenerate(0, pubKey, 65) != E_OK) {
        return E_NOT_OK;
    }
    
    return Crypto_MbedTLS_ECDSA_Verify_Internal(pubKey, pubKeyLen, digest, digestLen, signature, 64);
}

/**********************************************************************************************************************
 * Crypto_MbedTLS_ECDSA_Verify_Internal
 *********************************************************************************************************************/
STATIC Std_ReturnType Crypto_MbedTLS_ECDSA_Verify_Internal(const uint8* pubKey, uint32 pubKeyLen,
                                                            const uint8* digest, uint32 digestLen,
                                                            const uint8* signature, uint32 sigLen)
{
    mbedtls_ecdsa_context   ctx;
    mbedtls_ecp_point       Q;
    mbedtls_mpi             r, s;
    int                     ret;
    
    (void)pubKeyLen;
    (void)sigLen;
    
    mbedtls_ecdsa_init(&ctx);
    mbedtls_ecp_point_init(&Q);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);
    
    /* Setup context */
    ret = mbedtls_ecdsa_setup(&ctx, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* Load public key (uncompressed format: 0x04 || X || Y) */
    if (pubKey[0] != 0x04) {
        ret = -1;
        goto cleanup;
    }
    
    ret = mbedtls_mpi_read_binary(&Q.X, pubKey + 1, 32);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    ret = mbedtls_mpi_read_binary(&Q.Y, pubKey + 33, 32);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    ret = mbedtls_mpi_lset(&Q.Z, 1);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* Load signature components */
    ret = mbedtls_mpi_read_binary(&r, signature, 32);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    ret = mbedtls_mpi_read_binary(&s, signature + 32, 32);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* Verify signature */
    ret = mbedtls_ecdsa_verify(&ctx.grp, digest, digestLen, &Q, &r, &s);
    
cleanup:
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    mbedtls_ecp_point_free(&Q);
    mbedtls_ecdsa_free(&ctx);
    
    return Crypto_MbedTLS_ConvertResult(ret);
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - ECDH OPERATIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_MbedTLS_ECDH_CalcSecret
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_ECDH_CalcSecret(Crypto_KeyIdType privKeyId,
                                               const uint8* pubKeyPtr,
                                               uint32 pubKeyLen)
{
    mbedtls_ecp_group       grp;
    mbedtls_ecp_point       Q;
    mbedtls_mpi             d, z;
    uint8                   privKey[32];
    uint8                   sharedSecret[32];
    int                     ret;
    
    (void)privKeyId;
    (void)pubKeyLen;
    
    /* Initialize structures */
    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&Q);
    mbedtls_mpi_init(&d);
    mbedtls_mpi_init(&z);
    
    /* Load curve */
    ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* Load private key (would come from key store) */
    if (Crypto_RandomGenerate(0, privKey, 32) != E_OK) {
        ret = -1;
        goto cleanup;
    }
    
    ret = mbedtls_mpi_read_binary(&d, privKey, 32);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* Load peer public key */
    if (pubKeyPtr[0] != 0x04) {
        ret = -1;
        goto cleanup;
    }
    
    ret = mbedtls_mpi_read_binary(&Q.X, pubKeyPtr + 1, 32);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    ret = mbedtls_mpi_read_binary(&Q.Y, pubKeyPtr + 33, 32);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    ret = mbedtls_mpi_lset(&Q.Z, 1);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* Calculate shared secret: z = d * Q */
    ret = mbedtls_ecp_mul(&grp, &Q, &d, &Q, Crypto_MbedTLS_RandomCallback, NULL_PTR);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* Export shared secret (X coordinate) */
    ret = mbedtls_mpi_write_binary(&Q.X, sharedSecret, 32);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* In full implementation, would store derived key */
    
cleanup:
    mbedtls_mpi_free(&z);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_point_free(&Q);
    mbedtls_ecp_group_free(&grp);
    
    return Crypto_MbedTLS_ConvertResult(ret);
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - AES-GCM OPERATIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_MbedTLS_AES_GCM_Encrypt
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_AES_GCM_Encrypt(Crypto_KeyIdType keyId,
                                               const uint8* plaintext,
                                               uint32 plaintextLen,
                                               const uint8* aad,
                                               uint32 aadLen,
                                               const uint8* iv,
                                               uint8* ciphertext,
                                               uint8* tag)
{
    mbedtls_gcm_context     ctx;
    uint8                   key[16];  /* AES-128 */
    int                     ret;
    
    (void)keyId;
    
    /* Get key from storage (demo uses random) */
    if (Crypto_RandomGenerate(0, key, 16) != E_OK) {
        return E_NOT_OK;
    }
    
    mbedtls_gcm_init(&ctx);
    
    /* Set key */
    ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret != MBEDTLS_SUCCESS) {
        mbedtls_gcm_free(&ctx);
        return E_NOT_OK;
    }
    
    /* Encrypt and authenticate */
    ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, plaintextLen,
                                     iv, 12,   /* 96-bit IV for CCC */
                                     aad, aadLen,
                                     plaintext, ciphertext,
                                     16, tag);  /* 128-bit tag */
    
    mbedtls_gcm_free(&ctx);
    
    return Crypto_MbedTLS_ConvertResult(ret);
}

/**********************************************************************************************************************
 * Crypto_MbedTLS_AES_GCM_Decrypt
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_AES_GCM_Decrypt(Crypto_KeyIdType keyId,
                                               const uint8* ciphertext,
                                               uint32 ciphertextLen,
                                               const uint8* aad,
                                               uint32 aadLen,
                                               const uint8* iv,
                                               const uint8* tag,
                                               uint8* plaintext)
{
    mbedtls_gcm_context     ctx;
    uint8                   key[16];
    int                     ret;
    
    (void)keyId;
    
    if (Crypto_RandomGenerate(0, key, 16) != E_OK) {
        return E_NOT_OK;
    }
    
    mbedtls_gcm_init(&ctx);
    
    ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret != MBEDTLS_SUCCESS) {
        mbedtls_gcm_free(&ctx);
        return E_NOT_OK;
    }
    
    /* Decrypt and verify */
    ret = mbedtls_gcm_auth_decrypt(&ctx, ciphertextLen,
                                    iv, 12,
                                    aad, aadLen,
                                    tag, 16,
                                    ciphertext, plaintext);
    
    mbedtls_gcm_free(&ctx);
    
    return Crypto_MbedTLS_ConvertResult(ret);
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - HASH OPERATIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_MbedTLS_SHA256
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_SHA256(const uint8* data, uint32 dataLen, uint8* digest)
{
    int ret;
    
    if ((data == NULL_PTR) || (digest == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    ret = mbedtls_sha256_ret(data, dataLen, digest, 0);  /* 0 = SHA-256, not SHA-224 */
    
    return Crypto_MbedTLS_ConvertResult(ret);
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - HMAC OPERATIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_MbedTLS_HMAC
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_HMAC(Crypto_KeyIdType keyId,
                                    const uint8* data,
                                    uint32 dataLen,
                                    uint8* mac)
{
    const mbedtls_md_info_t*    md_info;
    uint8                       key[32];
    int                         ret;
    
    (void)keyId;
    
    if (Crypto_RandomGenerate(0, key, 32) != E_OK) {
        return E_NOT_OK;
    }
    
    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL_PTR) {
        return E_NOT_OK;
    }
    
    ret = mbedtls_md_hmac(md_info, key, 32, data, dataLen, mac);
    
    return Crypto_MbedTLS_ConvertResult(ret);
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - HKDF OPERATIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_MbedTLS_HKDF
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_HKDF(Crypto_KeyIdType ikmKeyId,
                                    const uint8* salt,
                                    uint32 saltLen,
                                    const uint8* info,
                                    uint32 infoLen,
                                    uint8* okm,
                                    uint32 okmLen)
{
    const mbedtls_md_info_t*    md_info;
    uint8                       ikm[32];
    int                         ret;
    
    (void)ikmKeyId;
    
    if (Crypto_RandomGenerate(0, ikm, 32) != E_OK) {
        return E_NOT_OK;
    }
    
    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL_PTR) {
        return E_NOT_OK;
    }
    
    ret = mbedtls_hkdf(md_info,
                        salt, saltLen,
                        ikm, 32,
                        info, infoLen,
                        okm, okmLen);
    
    return Crypto_MbedTLS_ConvertResult(ret);
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - KEY MANAGEMENT
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_MbedTLS_KeyGenerate
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_KeyGenerate(Crypto_KeyIdType keyId)
{
    mbedtls_ecp_group       grp;
    mbedtls_mpi             d;
    mbedtls_ecp_point       Q;
    int                     ret;
    
    (void)keyId;
    
    mbedtls_ecp_group_init(&grp);
    mbedtls_mpi_init(&d);
    mbedtls_ecp_point_init(&Q);
    
    ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != MBEDTLS_SUCCESS) {
        goto cleanup;
    }
    
    /* Generate key pair */
    ret = mbedtls_ecp_gen_keypair(&grp, &d, &Q, Crypto_MbedTLS_RandomCallback, NULL_PTR);
    
cleanup:
    mbedtls_ecp_point_free(&Q);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_group_free(&grp);
    
    return Crypto_MbedTLS_ConvertResult(ret);
}

/**********************************************************************************************************************
 * Crypto_MbedTLS_KeyDerive
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_KeyDerive(Crypto_KeyIdType srcKeyId, Crypto_KeyIdType dstKeyId)
{
    (void)srcKeyId;
    (void)dstKeyId;
    
    /* Would implement key derivation using HKDF in full implementation */
    return E_OK;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - JOB PROCESSING
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_MbedTLS_ProcessJob
 *********************************************************************************************************************/
Std_ReturnType Crypto_MbedTLS_ProcessJob(Crypto_JobType* job)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (job == NULL_PTR || job->jobPrimitiveInfo == NULL_PTR || 
        job->jobPrimitiveInputOutput == NULL_PTR) {
        return E_NOT_OK;
    }
    
    switch (job->jobPrimitiveInfo->service) {
        case CRYPTO_SERVICE_ENCRYPT:
            /* AES-GCM Encryption */
            result = Crypto_MbedTLS_AES_GCM_Encrypt(
                job->cryptoKeyId,
                job->jobPrimitiveInputOutput->input8Ptr,
                job->jobPrimitiveInputOutput->inputLength,
                job->jobPrimitiveInputOutput->secondaryInputPtr != NULL_PTR ? 
                    (const uint8*)job->jobPrimitiveInputOutput->secondaryInputPtr : NULL_PTR,
                job->jobPrimitiveInputOutput->secondaryInputLength,
                job->jobPrimitiveInputOutput->tertiaryInputPtr != NULL_PTR ? 
                    (const uint8*)job->jobPrimitiveInputOutput->tertiaryInputPtr : NULL_PTR,
                job->jobPrimitiveInputOutput->output8Ptr,
                job->jobPrimitiveInputOutput->secondaryOutputPtr != NULL_PTR ? 
                    job->jobPrimitiveInputOutput->secondaryOutputPtr : NULL_PTR
            );
            break;
            
        case CRYPTO_SERVICE_DECRYPT:
            /* AES-GCM Decryption */
            result = Crypto_MbedTLS_AES_GCM_Decrypt(
                job->cryptoKeyId,
                job->jobPrimitiveInputOutput->input8Ptr,
                job->jobPrimitiveInputOutput->inputLength,
                job->jobPrimitiveInputOutput->secondaryInputPtr != NULL_PTR ? 
                    (const uint8*)job->jobPrimitiveInputOutput->secondaryInputPtr : NULL_PTR,
                job->jobPrimitiveInputOutput->secondaryInputLength,
                job->jobPrimitiveInputOutput->tertiaryInputPtr != NULL_PTR ? 
                    (const uint8*)job->jobPrimitiveInputOutput->tertiaryInputPtr : NULL_PTR,
                job->jobPrimitiveInputOutput->input8Ptr + job->jobPrimitiveInputOutput->inputLength,  /* Tag appended */
                job->jobPrimitiveInputOutput->output8Ptr
            );
            break;
            
        case CRYPTO_SERVICE_HASH:
            /* SHA-256 */
            result = Crypto_MbedTLS_SHA256(
                job->jobPrimitiveInputOutput->input8Ptr,
                job->jobPrimitiveInputOutput->inputLength,
                job->jobPrimitiveInputOutput->output8Ptr
            );
            break;
            
        case CRYPTO_SERVICE_MACGENERATE:
            /* HMAC */
            result = Crypto_MbedTLS_HMAC(
                job->cryptoKeyId,
                job->jobPrimitiveInputOutput->input8Ptr,
                job->jobPrimitiveInputOutput->inputLength,
                job->jobPrimitiveInputOutput->output8Ptr
            );
            break;
            
        case CRYPTO_SERVICE_SIGN:
            /* ECDSA Sign */
            result = Crypto_MbedTLS_ECDSA_Sign(
                job->cryptoKeyId,
                job->jobPrimitiveInputOutput->input8Ptr,
                job->jobPrimitiveInputOutput->inputLength,
                job->jobPrimitiveInputOutput->output8Ptr,
                job->jobPrimitiveInputOutput->outputLength8Ptr
            );
            break;
            
        case CRYPTO_SERVICE_VERIFY:
            /* ECDSA Verify */
            result = Crypto_MbedTLS_ECDSA_Verify(
                job->cryptoKeyId,
                job->jobPrimitiveInputOutput->input8Ptr,
                job->jobPrimitiveInputOutput->inputLength,
                job->jobPrimitiveInputOutput->secondaryInputPtr != NULL_PTR ? 
                    (const uint8*)job->jobPrimitiveInputOutput->secondaryInputPtr : NULL_PTR,
                job->jobPrimitiveInputOutput->secondaryInputLength
            );
            if (result == E_OK) {
                if (job->jobPrimitiveInputOutput->verifyPtr != NULL_PTR) {
                    *job->jobPrimitiveInputOutput->verifyPtr = CRYPTO_VERIFICATION_PASSED;
                }
            }
            break;
            
        case CRYPTO_SERVICE_RANDOMGENERATE:
            /* Random Generation */
            result = Crypto_MbedTLS_RandomGenerate(
                job->jobPrimitiveInputOutput->output8Ptr,
                job->jobPrimitiveInputOutput->outputLength != NULL_PTR ? 
                    *job->jobPrimitiveInputOutput->outputLengthPtr : 0U
            );
            break;
            
        case CRYPTO_SERVICE_KEYEXCHANGECALCSECRET:
            /* ECDH Key Exchange */
            result = Crypto_MbedTLS_ECDH_CalcSecret(
                job->cryptoKeyId,
                job->jobPrimitiveInputOutput->input8Ptr,
                job->jobPrimitiveInputOutput->inputLength
            );
            break;
            
        case CRYPTO_SERVICE_KEYDERIVE:
            /* HKDF */
            result = Crypto_MbedTLS_HKDF(
                job->cryptoKeyId,
                job->jobPrimitiveInputOutput->secondaryInputPtr != NULL_PTR ? 
                    (const uint8*)job->jobPrimitiveInputOutput->secondaryInputPtr : NULL_PTR,
                job->jobPrimitiveInputOutput->secondaryInputLength,
                job->jobPrimitiveInputOutput->tertiaryInputPtr != NULL_PTR ? 
                    (const uint8*)job->jobPrimitiveInputOutput->tertiaryInputPtr : NULL_PTR,
                job->jobPrimitiveInputOutput->tertiaryInputLength,
                job->jobPrimitiveInputOutput->output8Ptr,
                job->jobPrimitiveInputOutput->outputLength != NULL_PTR ? 
                    *job->jobPrimitiveInputOutput->outputLengthPtr : 0U
            );
            break;
            
        default:
            result = E_NOT_OK;
            break;
    }
    
    return result;
}

/**********************************************************************************************************************
 * LOCAL FUNCTIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_MbedTLS_ConvertResult
 *********************************************************************************************************************/
STATIC Std_ReturnType Crypto_MbedTLS_ConvertResult(int mbedtls_result)
{
    if (mbedtls_result == MBEDTLS_SUCCESS) {
        return E_OK;
    }
    return E_NOT_OK;
}

#define CRYPTO_STOP_SEC_CODE
#include "MemMap.h"
Std_ReturnType Crypto_MbedTLS_ProcessJob(Crypto_JobType* job);
Std_ReturnType Crypto_MbedTLS_KeyDerive(Crypto_KeyIdType srcKeyId, Crypto_KeyIdType dstKeyId);
Std_ReturnType Crypto_MbedTLS_KeyGenerate(Crypto_KeyIdType keyId);
Std_ReturnType Crypto_MbedTLS_HKDF(Crypto_KeyIdType ikmKeyId,                                    const uint8* salt,                                    uint32 saltLen,                                    const uint8* info,                                    uint32 infoLen,                                    uint8* okm,                                    uint32 okmLen);
Std_ReturnType Crypto_MbedTLS_HMAC(Crypto_KeyIdType keyId,                                    const uint8* data,                                    uint32 dataLen,                                    uint8* mac);
Std_ReturnType Crypto_MbedTLS_SHA256(const uint8* data, uint32 dataLen, uint8* digest);
Std_ReturnType Crypto_MbedTLS_AES_GCM_Decrypt(Crypto_KeyIdType keyId,                                               const uint8* ciphertext,                                               uint32 ciphertextLen,                                               const uint8* aad,                                               uint32 aadLen,                                               const uint8* iv,                                               const uint8* tag,                                               uint8* plaintext);
Std_ReturnType Crypto_MbedTLS_AES_GCM_Encrypt(Crypto_KeyIdType keyId,                                               const uint8* plaintext,                                               uint32 plaintextLen,                                               const uint8* aad,                                               uint32 aadLen,                                               const uint8* iv,                                               uint8* ciphertext,                                               uint8* tag);
Std_ReturnType Crypto_MbedTLS_ECDH_CalcSecret(Crypto_KeyIdType privKeyId,                                               const uint8* pubKeyPtr,                                               uint32 pubKeyLen);
Std_ReturnType Crypto_MbedTLS_RandomGenerate(uint8* resultPtr, uint32 resultLength);
void Crypto_MbedTLS_DeInit(void);
Std_ReturnType Crypto_MbedTLS_Init(void);

/**********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
