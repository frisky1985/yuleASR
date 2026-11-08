/**********************************************************************************************************************
 * @file       test_crypto_mbedtls.c
 * @brief      Host unit tests for the mbedTLS software crypto backend (Crypto_MbedTLS.c)
 * @author     YuleTech AutoSAR Team
 * @date       2026-11-08
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      White-box host tests covering:
 *      - Init / DeInit (idempotency, uninitialised guards)
 *      - Random number generation incl. NULL/zero-length guards
 *      - AES-128-GCM encrypt/decrypt round trip + tag mismatch
 *      - SHA-256 / HMAC-SHA256 / HKDF correctness (compared against mbedTLS)
 *      - ECDSA (SECP256R1) sign/verify incl. malformed public key branches
 *      - ECDH shared-secret calculation incl. malformed public key / RNG failure
 *      - KeyGenerate / KeyDerive
 *      - Crypto_MbedTLS_ProcessJob dispatch for every service + argument guards
 *
 *      Crypto_RandomGenerate() (normally provided by Crypto.c, which is not
 *      linked here) is replaced by a deterministic test stub so that the
 *      backend's "key store" reads are reproducible.
 *********************************************************************************************************************/

#include <string.h>

#include "unity.h"
#include "Crypto.h"

#include "mbedtls/bignum.h"
#include "mbedtls/ecp.h"
#include "mbedtls/md.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/sha256.h"

/**********************************************************************************************************************
 * TEST RNG STUB (replaces Crypto.c's Crypto_RandomGenerate)
 *********************************************************************************************************************/

#define RAND_MODE_FILL          (0U)   /* all bytes 0x01                                  */
#define RAND_MODE_UNCOMPRESSED  (1U)   /* 0x04 || 0x01...  (invalid point, "uncompressed") */
#define RAND_MODE_KEYPAIR       (2U)   /* real P-256 key pair produced by make_keypair()   */
#define RAND_MODE_FAIL          (3U)   /* simulate key-store read failure                  */

static uint32 g_rand_mode = RAND_MODE_FILL;
static uint8  g_keypair_priv[32];
static uint8  g_keypair_pub[65];

Std_ReturnType Crypto_RandomGenerate(Crypto_KeyIdType cryptoKeyId, uint8* resultPtr, uint32 resultLength)
{
    uint32 i;

    (void)cryptoKeyId;

    if ((resultPtr == NULL_PTR) || (resultLength == 0U)) {
        return E_NOT_OK;
    }
    if (g_rand_mode == RAND_MODE_FAIL) {
        return E_NOT_OK;
    }
    if (g_rand_mode == RAND_MODE_KEYPAIR) {
        if (resultLength == 32U) {
            (void)memcpy(resultPtr, g_keypair_priv, 32U);
            return E_OK;
        }
        if (resultLength == 65U) {
            (void)memcpy(resultPtr, g_keypair_pub, 65U);
            return E_OK;
        }
        return E_NOT_OK;
    }

    for (i = 0U; i < resultLength; i++) {
        resultPtr[i] = 0x01U;
    }
    if (g_rand_mode == RAND_MODE_UNCOMPRESSED) {
        resultPtr[0] = 0x04U;
    }
    return E_OK;
}

/**********************************************************************************************************************
 * HELPERS
 *********************************************************************************************************************/

/* Deterministic RNG used only to create a real P-256 key pair for ECDSA tests. */
static uint32 g_rng_state = 1U;

static int test_fixed_rng(void* ctx, unsigned char* output, size_t len)
{
    size_t i;

    (void)ctx;

    for (i = 0U; i < len; i++) {
        g_rng_state = (g_rng_state * 1103515245U) + 12345U;
        output[i] = (unsigned char)((g_rng_state >> 16) & 0xFFU);
    }
    return 0;
}

static int make_keypair(uint8* priv, uint8* pub)
{
    mbedtls_ecp_group grp;
    mbedtls_mpi       d;
    mbedtls_ecp_point q;
    size_t            olen = 0U;
    int               ret;

    mbedtls_ecp_group_init(&grp);
    mbedtls_mpi_init(&d);
    mbedtls_ecp_point_init(&q);

    ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret == 0) {
        ret = mbedtls_ecp_gen_keypair(&grp, &d, &q, test_fixed_rng, NULL);
    }
    if (ret == 0) {
        ret = mbedtls_mpi_write_binary(&d, priv, 32U);
    }
    if (ret == 0) {
        ret = mbedtls_ecp_point_write_binary(&grp, &q, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, pub, 65U);
    }

    mbedtls_ecp_point_free(&q);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_group_free(&grp);
    return ret;
}

static void fill_pattern(uint8* buf, uint32 len, uint8 seed)
{
    uint32 i;

    for (i = 0U; i < len; i++) {
        buf[i] = (uint8)(seed + i);
    }
}

/* SECP256R1 base point G (uncompressed) — valid peer public key for ECDH. */
static const uint8 kSecp256r1G[65] = {
    0x04U,
    0x6BU, 0x17U, 0xD1U, 0xF2U, 0xE1U, 0x2CU, 0x42U, 0x47U,
    0xF8U, 0xBCU, 0xE6U, 0xE5U, 0x63U, 0xA4U, 0x40U, 0xF2U,
    0x77U, 0x03U, 0x7DU, 0x81U, 0x2DU, 0xEBU, 0x33U, 0xA0U,
    0xF4U, 0xA1U, 0x39U, 0x45U, 0xD8U, 0x98U, 0xC2U, 0x96U,
    0x4FU, 0xE3U, 0x42U, 0xE2U, 0xFEU, 0x1AU, 0x7FU, 0x9BU,
    0x8EU, 0xE7U, 0xEBU, 0x4AU, 0x7CU, 0x0FU, 0x9EU, 0x16U,
    0x2BU, 0xCEU, 0x33U, 0x57U, 0x6BU, 0x31U, 0x5EU, 0xCEU,
    0xCBU, 0xB6U, 0x40U, 0x68U, 0x37U, 0xBFU, 0x51U, 0xF5U
};

/* SHA-256("abc") */
static const uint8 kSha256Abc[32] = {
    0xBAU, 0x78U, 0x16U, 0xBFU, 0x8FU, 0x01U, 0xCFU, 0xEAU,
    0x41U, 0x41U, 0x40U, 0xDEU, 0x5DU, 0xAEU, 0x22U, 0x23U,
    0xB0U, 0x03U, 0x61U, 0xA3U, 0x96U, 0x17U, 0x7AU, 0x9CU,
    0xB4U, 0x10U, 0xFFU, 0x61U, 0xF2U, 0x00U, 0x15U, 0xADU
};

/**********************************************************************************************************************
 * UNITY HOOKS
 *********************************************************************************************************************/

void setUp(void)
{
    g_rand_mode = RAND_MODE_FILL;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_Init());
}

void tearDown(void)
{
    Crypto_MbedTLS_DeInit();
}

/**********************************************************************************************************************
 * INITIALISATION
 *********************************************************************************************************************/

static void test_mbedtls_init_deinit(void)
{
    /* DeInit when not initialised must be a no-op */
    Crypto_MbedTLS_DeInit();
    Crypto_MbedTLS_DeInit();

    /* Re-init after DeInit */
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_Init());
    /* Second Init returns E_OK immediately (already initialised) */
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_Init());

    Crypto_MbedTLS_DeInit();
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_Init());
}

/**********************************************************************************************************************
 * RANDOM
 *********************************************************************************************************************/

static void test_mbedtls_random_generate(void)
{
    uint8  buf[32];
    uint32 i;
    boolean nonZero = FALSE;

    /* Uninitialised backend must refuse */
    Crypto_MbedTLS_DeInit();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_RandomGenerate(buf, 16U));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_Init());

    /* Argument guards */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_RandomGenerate(NULL_PTR, 16U));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_RandomGenerate(buf, 0U));

    (void)memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_RandomGenerate(buf, (uint32)sizeof(buf)));

    for (i = 0U; i < (uint32)sizeof(buf); i++) {
        if (buf[i] != 0U) {
            nonZero = TRUE;
        }
    }
    TEST_ASSERT_TRUE(nonZero == TRUE);
}

/**********************************************************************************************************************
 * AES-GCM
 *********************************************************************************************************************/

static void test_mbedtls_aes_gcm_roundtrip(void)
{
    uint8  plaintext[32];
    uint8  ciphertext[32];
    uint8  decrypted[32];
    uint8  tag[16];
    uint8  badTag[16];
    uint8  aad[8];
    uint8  iv[12];

    fill_pattern(plaintext, 32U, 0x10U);
    fill_pattern(aad, 8U, 0x40U);
    fill_pattern(iv, 12U, 0x60U);

    TEST_ASSERT_EQUAL_UINT8(E_OK,
        Crypto_MbedTLS_AES_GCM_Encrypt(0U, plaintext, 32U, aad, 8U, iv, ciphertext, tag));

    TEST_ASSERT_EQUAL_UINT8(E_OK,
        Crypto_MbedTLS_AES_GCM_Decrypt(0U, ciphertext, 32U, aad, 8U, iv, tag, decrypted));
    TEST_ASSERT_EQUAL_MEMORY(plaintext, decrypted, 32U);

    /* Corrupted tag must fail authentication */
    (void)memcpy(badTag, tag, sizeof(badTag));
    badTag[0] = (uint8)(badTag[0] ^ 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK,
        Crypto_MbedTLS_AES_GCM_Decrypt(0U, ciphertext, 32U, aad, 8U, iv, badTag, decrypted));

    /* Key-store read failure propagates as E_NOT_OK */
    g_rand_mode = RAND_MODE_FAIL;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK,
        Crypto_MbedTLS_AES_GCM_Encrypt(0U, plaintext, 32U, aad, 8U, iv, ciphertext, tag));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK,
        Crypto_MbedTLS_AES_GCM_Decrypt(0U, ciphertext, 32U, aad, 8U, iv, tag, decrypted));
    g_rand_mode = RAND_MODE_FILL;
}

/**********************************************************************************************************************
 * SHA-256
 *********************************************************************************************************************/

static void test_mbedtls_sha256(void)
{
    uint8 digest[32];

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_SHA256(NULL_PTR, 3U, digest));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_SHA256((const uint8*)"abc", 3U, NULL_PTR));

    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_SHA256((const uint8*)"abc", 3U, digest));
    TEST_ASSERT_EQUAL_MEMORY(kSha256Abc, digest, 32U);
}

/**********************************************************************************************************************
 * HMAC
 *********************************************************************************************************************/

static void test_mbedtls_hmac(void)
{
    const mbedtls_md_info_t* mdInfo;
    uint8  data[16];
    uint8  mac[32];
    uint8  expected[32];
    uint8  key[32];

    fill_pattern(data, 16U, 0xA0U);
    (void)memset(key, 0x01, sizeof(key));   /* matches the RAND_MODE_FILL stub */

    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_HMAC(0U, data, 16U, mac));

    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    TEST_ASSERT_TRUE(mdInfo != NULL);
    TEST_ASSERT_EQUAL_INT(0, mbedtls_md_hmac(mdInfo, key, 32U, data, 16U, expected));
    TEST_ASSERT_EQUAL_MEMORY(expected, mac, 32U);

    g_rand_mode = RAND_MODE_FAIL;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_HMAC(0U, data, 16U, mac));
    g_rand_mode = RAND_MODE_FILL;
}

/**********************************************************************************************************************
 * HKDF
 *********************************************************************************************************************/

static void test_mbedtls_hkdf(void)
{
    const mbedtls_md_info_t* mdInfo;
    uint8  salt[16];
    uint8  info[8];
    uint8  ikm[32];
    uint8  okm[42];
    uint8  expected[42];

    fill_pattern(salt, 16U, 0x20U);
    fill_pattern(info, 8U, 0x80U);
    (void)memset(ikm, 0x01, sizeof(ikm));

    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_HKDF(0U, salt, 16U, info, 8U, okm, 42U));

    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    TEST_ASSERT_TRUE(mdInfo != NULL);
    TEST_ASSERT_EQUAL_INT(0, mbedtls_hkdf(mdInfo, salt, 16U, ikm, 32U, info, 8U, expected, 42U));
    TEST_ASSERT_EQUAL_MEMORY(expected, okm, 42U);

    /* Default (empty) salt/info are legal */
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_HKDF(0U, NULL_PTR, 0U, NULL_PTR, 0U, okm, 32U));

    g_rand_mode = RAND_MODE_FAIL;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_HKDF(0U, salt, 16U, info, 8U, okm, 42U));
    g_rand_mode = RAND_MODE_FILL;
}

/**********************************************************************************************************************
 * ECDSA
 *********************************************************************************************************************/

static void test_mbedtls_ecdsa_sign_verify(void)
{
    uint8  digest[32];
    uint8  signature[64];
    uint8  priv[32];
    uint8  pub[65];
    uint32 sigLen = 0U;

    fill_pattern(digest, 32U, 0x30U);

    TEST_ASSERT_EQUAL_INT(0, make_keypair(priv, pub));
    (void)memcpy(g_keypair_priv, priv, 32U);
    (void)memcpy(g_keypair_pub, pub, 65U);
    g_rand_mode = RAND_MODE_KEYPAIR;

    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ECDSA_Sign(0U, digest, 32U, signature, &sigLen));
    TEST_ASSERT_EQUAL_UINT32(64U, sigLen);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ECDSA_Verify(0U, digest, 32U, signature, 64U));

    /* Altering the digest must break verification */
    digest[0] = (uint8)(digest[0] ^ 0xFFU);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ECDSA_Verify(0U, digest, 32U, signature, 64U));
}

static void test_mbedtls_ecdsa_error_paths(void)
{
    uint8  digest[32];
    uint8  signature[64];
    uint32 sigLen = 64U;

    fill_pattern(digest, 32U, 0x70U);

    /* Key-store failure */
    g_rand_mode = RAND_MODE_FAIL;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ECDSA_Sign(0U, digest, 32U, signature, &sigLen));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ECDSA_Verify(0U, digest, 32U, signature, 64U));

    /* Public key does not start with 0x04 (invalid key format) */
    g_rand_mode = RAND_MODE_FILL;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ECDSA_Verify(0U, digest, 32U, signature, 64U));

    /* Public key starts with 0x04 but is not on the curve -> verification fails */
    g_rand_mode = RAND_MODE_UNCOMPRESSED;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ECDSA_Verify(0U, digest, 32U, signature, 64U));

    g_rand_mode = RAND_MODE_FILL;
}

/**********************************************************************************************************************
 * ECDH
 *********************************************************************************************************************/

static void test_mbedtls_ecdh_calc_secret(void)
{
    uint8 peer[65];

    (void)memcpy(peer, kSecp256r1G, sizeof(peer));

    /* Valid peer key + valid private scalar (0x01... repeated) */
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ECDH_CalcSecret(0U, peer, 65U));

    /* Invalid key format */
    peer[0] = 0x02U;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ECDH_CalcSecret(0U, peer, 65U));

    /* Key-store failure */
    (void)memcpy(peer, kSecp256r1G, sizeof(peer));
    g_rand_mode = RAND_MODE_FAIL;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ECDH_CalcSecret(0U, peer, 65U));
    g_rand_mode = RAND_MODE_FILL;
}

/**********************************************************************************************************************
 * KEY MANAGEMENT
 *********************************************************************************************************************/

static void test_mbedtls_key_management(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_KeyGenerate(0U));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_KeyDerive(0U, 0U));
}

/**********************************************************************************************************************
 * PROCESS JOB
 *********************************************************************************************************************/

static Crypto_JobType                      g_job;
static Crypto_JobPrimitiveInputOutputType  g_io;
static Crypto_JobPrimitiveInfoType         g_info;
static Crypto_AlgorithmInfoType            g_alg;

static void job_init(uint32 service)
{
    (void)memset(&g_job, 0, sizeof(g_job));
    (void)memset(&g_io, 0, sizeof(g_io));
    (void)memset(&g_info, 0, sizeof(g_info));
    (void)memset(&g_alg, 0, sizeof(g_alg));

    g_alg.family    = CRYPTO_ALGOFAM_AES;
    g_alg.mode      = CRYPTO_ALGOMODE_GCM;
    g_alg.keyLength = 128U;
    g_alg.curve     = CRYPTO_ECC_CURVE_SECP256R1;

    g_info.algorithm      = &g_alg;
    g_info.service        = service;
    g_info.processingType = CRYPTO_PROCESSING_SYNC;

    g_job.jobId                     = 1U;
    g_job.jobPrimitiveInfo          = &g_info;
    g_job.jobPrimitiveInputOutput   = &g_io;
    g_job.cryptoKeyId               = 0U;
}

static void test_mbedtls_processjob_argument_guards(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ProcessJob(NULL_PTR));

    job_init(CRYPTO_SERVICE_HASH);
    g_job.jobPrimitiveInfo = NULL_PTR;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ProcessJob(&g_job));

    job_init(CRYPTO_SERVICE_HASH);
    g_job.jobPrimitiveInputOutput = NULL_PTR;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ProcessJob(&g_job));

    /* Unknown service -> default branch */
    job_init(0xFFU);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ProcessJob(&g_job));
}

static void test_mbedtls_processjob_encrypt_decrypt(void)
{
    static uint32 aadWords[2];
    static uint32 ivWords[3];
    uint8  plaintext[32];
    uint8  ciphertext[32];
    uint8  decrypted[32];
    uint8  tag[16];
    uint8  inputBuf[48];
    uint8* aad = (uint8*)aadWords;
    uint8* iv  = (uint8*)ivWords;

    fill_pattern(plaintext, 32U, 0x10U);
    fill_pattern(aad, 8U, 0x40U);
    fill_pattern(iv, 12U, 0x60U);

    job_init(CRYPTO_SERVICE_ENCRYPT);
    g_io.input8Ptr             = plaintext;
    g_io.inputLength           = 32U;
    g_io.secondaryInputPtr     = aadWords;
    g_io.secondaryInputLength  = 8U;
    g_io.tertiaryInputPtr      = ivWords;
    g_io.output8Ptr            = ciphertext;
    g_io.secondaryOutputPtr    = (uint32*)(void*)tag;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));

    /* ProcessJob DECRYPT expects ciphertext || tag in one buffer */
    (void)memcpy(inputBuf, ciphertext, 32U);
    (void)memcpy(inputBuf + 32U, tag, 16U);

    job_init(CRYPTO_SERVICE_DECRYPT);
    g_io.input8Ptr             = inputBuf;
    g_io.inputLength           = 32U;
    g_io.secondaryInputPtr     = aadWords;
    g_io.secondaryInputLength  = 8U;
    g_io.tertiaryInputPtr      = ivWords;
    g_io.output8Ptr            = decrypted;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));
    TEST_ASSERT_EQUAL_MEMORY(plaintext, decrypted, 32U);

    /* NULL secondary input (empty AAD) */
    job_init(CRYPTO_SERVICE_ENCRYPT);
    g_io.input8Ptr          = plaintext;
    g_io.inputLength        = 32U;
    g_io.tertiaryInputPtr   = ivWords;
    g_io.output8Ptr         = ciphertext;
    g_io.secondaryOutputPtr = (uint32*)(void*)tag;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));
}

static void test_mbedtls_processjob_hash_mac(void)
{
    const mbedtls_md_info_t* mdInfo;
    uint8  data[16];
    uint8  digest[32];
    uint8  mac[32];
    uint8  expected[32];
    uint8  key[32];

    fill_pattern(data, 16U, 0xA0U);

    job_init(CRYPTO_SERVICE_HASH);
    g_io.input8Ptr   = data;
    g_io.inputLength = 16U;
    g_io.output8Ptr  = digest;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_SHA256(data, 16U, expected));
    TEST_ASSERT_EQUAL_MEMORY(expected, digest, 32U);

    (void)memset(key, 0x01, sizeof(key));
    job_init(CRYPTO_SERVICE_MACGENERATE);
    g_io.input8Ptr   = data;
    g_io.inputLength = 16U;
    g_io.output8Ptr  = mac;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));

    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    TEST_ASSERT_TRUE(mdInfo != NULL);
    TEST_ASSERT_EQUAL_INT(0, mbedtls_md_hmac(mdInfo, key, 32U, data, 16U, expected));
    TEST_ASSERT_EQUAL_MEMORY(expected, mac, 32U);
}

static void test_mbedtls_processjob_sign_verify(void)
{
    uint8  digest[32];
    uint8  signature[64];
    uint8  priv[32];
    uint8  pub[65];
    uint32 sigLen = 0U;
    Crypto_VerifyResultType verifyResult = CRYPTO_VERIFICATION_FAILED;

    fill_pattern(digest, 32U, 0x30U);

    TEST_ASSERT_EQUAL_INT(0, make_keypair(priv, pub));
    (void)memcpy(g_keypair_priv, priv, 32U);
    (void)memcpy(g_keypair_pub, pub, 65U);
    g_rand_mode = RAND_MODE_KEYPAIR;

    job_init(CRYPTO_SERVICE_SIGN);
    g_io.input8Ptr        = digest;
    g_io.inputLength      = 32U;
    g_io.output8Ptr       = signature;
    g_io.outputLength8Ptr = &sigLen;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));
    TEST_ASSERT_EQUAL_UINT32(64U, sigLen);

    job_init(CRYPTO_SERVICE_VERIFY);
    g_io.input8Ptr            = digest;
    g_io.inputLength          = 32U;
    g_io.secondaryInputPtr    = (uint32*)(void*)signature;
    g_io.secondaryInputLength = 64U;
    g_io.verifyPtr            = &verifyResult;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));
    TEST_ASSERT_EQUAL_INT(CRYPTO_VERIFICATION_PASSED, (int)verifyResult);

    /* verifyPtr == NULL: result is still reported, just not written back */
    job_init(CRYPTO_SERVICE_VERIFY);
    g_io.input8Ptr            = digest;
    g_io.inputLength          = 32U;
    g_io.secondaryInputPtr    = (uint32*)(void*)signature;
    g_io.secondaryInputLength = 64U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));

    g_rand_mode = RAND_MODE_FILL;
}

static void test_mbedtls_processjob_random_keyexchange_keyderive(void)
{
    static uint32 saltWords[4];
    static uint32 infoWords[2];
    uint8  randomBuf[16];
    uint8  okm[42];
    uint8* salt = (uint8*)saltWords;
    uint8* info = (uint8*)infoWords;
    uint32 randomLen = 16U;
    uint32 okmLen    = 42U;

    fill_pattern(salt, 16U, 0x20U);
    fill_pattern(info, 8U, 0x80U);

    job_init(CRYPTO_SERVICE_RANDOMGENERATE);
    g_io.output8Ptr      = randomBuf;
    g_io.outputLengthPtr = &randomLen;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));

    /* NULL outputLengthPtr -> length defaults to 0 -> E_NOT_OK */
    job_init(CRYPTO_SERVICE_RANDOMGENERATE);
    g_io.output8Ptr = randomBuf;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, Crypto_MbedTLS_ProcessJob(&g_job));

    job_init(CRYPTO_SERVICE_KEYEXCHANGECALCSECRET);
    g_io.input8Ptr   = kSecp256r1G;
    g_io.inputLength = 65U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));

    job_init(CRYPTO_SERVICE_KEYDERIVE);
    g_io.secondaryInputPtr    = saltWords;
    g_io.secondaryInputLength = 16U;
    g_io.tertiaryInputPtr     = infoWords;
    g_io.tertiaryInputLength  = 8U;
    g_io.output8Ptr           = okm;
    g_io.outputLengthPtr      = &okmLen;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));

    /* NULL salt/info and NULL outputLengthPtr defaults */
    job_init(CRYPTO_SERVICE_KEYDERIVE);
    g_io.output8Ptr = okm;
    TEST_ASSERT_EQUAL_UINT8(E_OK, Crypto_MbedTLS_ProcessJob(&g_job));
}

/**********************************************************************************************************************
 * MAIN
 *********************************************************************************************************************/

int main(void)
{
    UnityBegin();

    RUN_TEST(test_mbedtls_init_deinit);
    RUN_TEST(test_mbedtls_random_generate);
    RUN_TEST(test_mbedtls_aes_gcm_roundtrip);
    RUN_TEST(test_mbedtls_sha256);
    RUN_TEST(test_mbedtls_hmac);
    RUN_TEST(test_mbedtls_hkdf);
    RUN_TEST(test_mbedtls_ecdsa_sign_verify);
    RUN_TEST(test_mbedtls_ecdsa_error_paths);
    RUN_TEST(test_mbedtls_ecdh_calc_secret);
    RUN_TEST(test_mbedtls_key_management);
    RUN_TEST(test_mbedtls_processjob_argument_guards);
    RUN_TEST(test_mbedtls_processjob_encrypt_decrypt);
    RUN_TEST(test_mbedtls_processjob_hash_mac);
    RUN_TEST(test_mbedtls_processjob_sign_verify);
    RUN_TEST(test_mbedtls_processjob_random_keyexchange_keyderive);

    return UnityEnd();
}

/**********************************************************************************************************************
 * END OF FILE
 *********************************************************************************************************************/
