/**
 * @file mbedtls_wrapper.c
 * @brief Mbed TLS Wrapper Implementation for YuleTech AutoSAR
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @author YuleTech AutoSAR Team
 * @version 1.0.0
 */

/* ============================================================================
 * Includes
 * ============================================================================ */

#include "mbedtls_wrapper.h"
#include "mbedtls_config.h"

/* Mbed TLS Headers */
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/sha256.h"
#include "mbedtls/platform.h"
#include "mbedtls/error.h"

/* Standard headers */
#include <string.h>

/* ============================================================================
 * Module Variables
 * ============================================================================ */

/* Entropy context for random number generation */
static mbedtls_entropy_context g_entropy_ctx;
static mbedtls_ctr_drbg_context g_ctr_drbg_ctx;

/* Module state */
static bool g_initialized = false;

/* Static buffer for memory allocation */
static uint8_t g_memory_buffer[16384];

/* ============================================================================
 * Private Function Declarations
 * ============================================================================ */

static mbedtls_wrapper_status_t convert_mbedtls_error(int mbedtls_err);
static void secure_zero(void *ptr, size_t len);

/* ============================================================================
 * Initialization Functions
 * ============================================================================ */

mbedtls_wrapper_status_t mbedtls_wrapper_init(void)
{
    int ret;
    const char *pers = "mbedtls_autosar_init";

    if (g_initialized) {
        return MBEDTLS_WRAPPER_OK;
    }

    /* Initialize memory allocator */
    mbedtls_memory_buffer_alloc_init(g_memory_buffer, sizeof(g_memory_buffer));

    /* Initialize entropy context */
    mbedtls_entropy_init(&g_entropy_ctx);

    /* Initialize CTR-DRBG context */
    mbedtls_ctr_drbg_init(&g_ctr_drbg_ctx);

    /* Seed the DRBG with entropy */
    ret = mbedtls_ctr_drbg_seed(&g_ctr_drbg_ctx,
                                 mbedtls_entropy_func,
                                 &g_entropy_ctx,
                                 (const unsigned char *)pers,
                                 strlen(pers));
    if (ret != 0) {
        mbedtls_ctr_drbg_free(&g_ctr_drbg_ctx);
        mbedtls_entropy_free(&g_entropy_ctx);
        return MBEDTLS_WRAPPER_CRYPTO_ERROR;
    }

    g_initialized = true;
    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_deinit(void)
{
    if (!g_initialized) {
        return MBEDTLS_WRAPPER_OK;
    }

    /* Clear and free contexts */
    mbedtls_ctr_drbg_free(&g_ctr_drbg_ctx);
    mbedtls_entropy_free(&g_entropy_ctx);

    /* Zero memory buffer */
    secure_zero(g_memory_buffer, sizeof(g_memory_buffer));

    g_initialized = false;
    return MBEDTLS_WRAPPER_OK;
}

void mbedtls_wrapper_get_version(uint8_t *major, uint8_t *minor, uint8_t *patch)
{
    if (major != NULL) {
        *major = MBEDTLS_WRAPPER_VERSION_MAJOR;
    }
    if (minor != NULL) {
        *minor = MBEDTLS_WRAPPER_VERSION_MINOR;
    }
    if (patch != NULL) {
        *patch = MBEDTLS_WRAPPER_VERSION_PATCH;
    }
}

/* ============================================================================
 * Random Number Generation
 * ============================================================================ */

mbedtls_wrapper_status_t mbedtls_wrapper_random(uint8_t *output, uint32_t len)
{
    int ret;

    if (!g_initialized || output == NULL || len == 0) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    ret = mbedtls_ctr_drbg_random(&g_ctr_drbg_ctx, output, len);
    if (ret != 0) {
        return convert_mbedtls_error(ret);
    }

    return MBEDTLS_WRAPPER_OK;
}

/* ============================================================================
 * ECC P-256 Key Operations
 * ============================================================================ */

mbedtls_wrapper_status_t mbedtls_wrapper_ecc_p256_gen_keypair(mbedtls_wrapper_ecc_key_t *key)
{
    int ret;
    mbedtls_ecp_group grp;
    mbedtls_mpi d;
    mbedtls_ecp_point Q;
    size_t olen;

    if (!g_initialized || key == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    /* Initialize structures */
    mbedtls_ecp_group_init(&grp);
    mbedtls_mpi_init(&d);
    mbedtls_ecp_point_init(&Q);

    /* Load secp256r1 group */
    ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0) {
        goto cleanup;
    }

    /* Generate key pair */
    ret = mbedtls_ecp_gen_keypair(&grp, &d, &Q,
                                   mbedtls_ctr_drbg_random,
                                   &g_ctr_drbg_ctx);
    if (ret != 0) {
        goto cleanup;
    }

    /* Export private key */
    ret = mbedtls_mpi_write_binary(&d, key->private_key,
                                    MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    /* Export public key (uncompressed format: 0x04 || X || Y) */
    key->public_key[0] = 0x04;
    ret = mbedtls_ecp_point_write_binary(&grp, &Q,
                                          MBEDTLS_ECP_PF_UNCOMPRESSED,
                                          &olen,
                                          key->public_key,
                                          sizeof(key->public_key));
    if (ret != 0) {
        goto cleanup;
    }

    key->has_private = true;
    key->has_public = true;

    ret = MBEDTLS_WRAPPER_OK;

cleanup:
    mbedtls_ecp_group_free(&grp);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_point_free(&Q);

    if (ret != MBEDTLS_WRAPPER_OK) {
        secure_zero(key, sizeof(mbedtls_wrapper_ecc_key_t));
        return convert_mbedtls_error(ret);
    }

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_ecc_p256_load_private_key(
    mbedtls_wrapper_ecc_key_t *key,
    const uint8_t *private_key)
{
    if (key == NULL || private_key == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    memcpy(key->private_key, private_key, MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    key->has_private = true;

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_ecc_p256_load_public_key(
    mbedtls_wrapper_ecc_key_t *key,
    const uint8_t *public_key)
{
    if (key == NULL || public_key == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    memcpy(key->public_key, public_key, MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE * 2 + 1);
    key->has_public = true;

    return MBEDTLS_WRAPPER_OK;
}

void mbedtls_wrapper_ecc_p256_clear_key(mbedtls_wrapper_ecc_key_t *key)
{
    if (key != NULL) {
        secure_zero(key, sizeof(mbedtls_wrapper_ecc_key_t));
        key->has_private = false;
        key->has_public = false;
    }
}

/* ============================================================================
 * ECDH P-256 Key Agreement
 * ============================================================================ */

mbedtls_wrapper_status_t mbedtls_wrapper_ecdh_p256_compute_shared_secret(
    const mbedtls_wrapper_ecc_key_t *private_key,
    const uint8_t *peer_public_key,
    uint8_t *shared_secret)
{
    int ret;
    mbedtls_ecp_group grp;
    mbedtls_mpi d;
    mbedtls_ecp_point Q_peer, shared_point;

    if (!g_initialized || private_key == NULL || peer_public_key == NULL ||
        shared_secret == NULL || !private_key->has_private) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    /* Initialize structures */
    mbedtls_ecp_group_init(&grp);
    mbedtls_mpi_init(&d);
    mbedtls_ecp_point_init(&Q_peer);
    mbedtls_ecp_point_init(&shared_point);

    /* Load secp256r1 group */
    ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0) {
        goto cleanup;
    }

    /* Load private key */
    ret = mbedtls_mpi_read_binary(&d, private_key->private_key,
                                   MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    /* Load peer public key */
    ret = mbedtls_ecp_point_read_binary(&grp, &Q_peer, peer_public_key,
                                         MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE * 2 + 1);
    if (ret != 0) {
        goto cleanup;
    }

    /* Validate peer public key */
    ret = mbedtls_ecp_check_pubkey(&grp, &Q_peer);
    if (ret != 0) {
        goto cleanup;
    }

    /* Compute shared secret: shared_point = d * Q_peer */
    ret = mbedtls_ecp_mul(&grp, &shared_point, &d, &Q_peer,
                          mbedtls_ctr_drbg_random, &g_ctr_drbg_ctx);
    if (ret != 0) {
        goto cleanup;
    }

    /* Export shared secret (X coordinate) */
    ret = mbedtls_mpi_write_binary(&shared_point.X, shared_secret,
                                    MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    ret = MBEDTLS_WRAPPER_OK;

cleanup:
    mbedtls_ecp_group_free(&grp);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_point_free(&Q_peer);
    mbedtls_ecp_point_free(&shared_point);

    if (ret != MBEDTLS_WRAPPER_OK) {
        secure_zero(shared_secret, MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
        return convert_mbedtls_error(ret);
    }

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_ecdh_p256_compute_shared_secret_raw(
    const uint8_t *private_key,
    const uint8_t *peer_public_key,
    uint8_t *shared_secret)
{
    mbedtls_wrapper_ecc_key_t key;
    mbedtls_wrapper_status_t status;

    status = mbedtls_wrapper_ecc_p256_load_private_key(&key, private_key);
    if (status != MBEDTLS_WRAPPER_OK) {
        return status;
    }

    status = mbedtls_wrapper_ecdh_p256_compute_shared_secret(&key, peer_public_key,
                                                              shared_secret);

    mbedtls_wrapper_ecc_p256_clear_key(&key);
    return status;
}

/* ============================================================================
 * ECDSA P-256 Digital Signature
 * ============================================================================ */

mbedtls_wrapper_status_t mbedtls_wrapper_ecdsa_p256_sign(
    const mbedtls_wrapper_ecc_key_t *private_key,
    const uint8_t *message,
    uint32_t message_len,
    uint8_t *signature)
{
    int ret;
    mbedtls_ecp_group grp;
    mbedtls_mpi d, r, s;
    unsigned char hash[MBEDTLS_WRAPPER_SHA256_SIZE];

    if (!g_initialized || private_key == NULL || message == NULL ||
        signature == NULL || !private_key->has_private) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    /* Hash the message */
    ret = mbedtls_sha256_ret(message, message_len, hash, 0);
    if (ret != 0) {
        return MBEDTLS_WRAPPER_CRYPTO_ERROR;
    }

    /* Initialize structures */
    mbedtls_ecp_group_init(&grp);
    mbedtls_mpi_init(&d);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    /* Load secp256r1 group */
    ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0) {
        goto cleanup;
    }

    /* Load private key */
    ret = mbedtls_mpi_read_binary(&d, private_key->private_key,
                                   MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    /* Sign the hash */
    ret = mbedtls_ecdsa_sign(&grp, &r, &s, &d, hash, sizeof(hash),
                             mbedtls_ctr_drbg_random, &g_ctr_drbg_ctx);
    if (ret != 0) {
        goto cleanup;
    }

    /* Export signature (R || S) */
    ret = mbedtls_mpi_write_binary(&r, signature,
                                    MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_mpi_write_binary(&s, signature + MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE,
                                    MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    ret = MBEDTLS_WRAPPER_OK;

cleanup:
    mbedtls_ecp_group_free(&grp);
    mbedtls_mpi_free(&d);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    secure_zero(hash, sizeof(hash));

    if (ret != MBEDTLS_WRAPPER_OK) {
        secure_zero(signature, MBEDTLS_WRAPPER_ECC_P256_SIG_SIZE);
        return convert_mbedtls_error(ret);
    }

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_ecdsa_p256_verify(
    const uint8_t *public_key,
    const uint8_t *message,
    uint32_t message_len,
    const uint8_t *signature)
{
    int ret;
    mbedtls_ecp_group grp;
    mbedtls_ecp_point Q;
    mbedtls_mpi r, s;
    unsigned char hash[MBEDTLS_WRAPPER_SHA256_SIZE];

    if (!g_initialized || public_key == NULL || message == NULL || signature == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    /* Hash the message */
    ret = mbedtls_sha256_ret(message, message_len, hash, 0);
    if (ret != 0) {
        return MBEDTLS_WRAPPER_CRYPTO_ERROR;
    }

    /* Initialize structures */
    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&Q);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    /* Load secp256r1 group */
    ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0) {
        goto cleanup;
    }

    /* Load public key */
    ret = mbedtls_ecp_point_read_binary(&grp, &Q, public_key,
                                         MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE * 2 + 1);
    if (ret != 0) {
        goto cleanup;
    }

    /* Load signature components */
    ret = mbedtls_mpi_read_binary(&r, signature, MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_mpi_read_binary(&s, signature + MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE,
                                   MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    /* Verify signature */
    ret = mbedtls_ecdsa_verify(&grp, hash, sizeof(hash), &Q, &r, &s);
    if (ret != 0) {
        goto cleanup;
    }

    ret = MBEDTLS_WRAPPER_OK;

cleanup:
    mbedtls_ecp_group_free(&grp);
    mbedtls_ecp_point_free(&Q);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    secure_zero(hash, sizeof(hash));

    if (ret != MBEDTLS_WRAPPER_OK) {
        return MBEDTLS_WRAPPER_ERROR;
    }

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_ecdsa_p256_sign_hash(
    const mbedtls_wrapper_ecc_key_t *private_key,
    const uint8_t *hash,
    uint8_t *signature)
{
    int ret;
    mbedtls_ecp_group grp;
    mbedtls_mpi d, r, s;

    if (!g_initialized || private_key == NULL || hash == NULL ||
        signature == NULL || !private_key->has_private) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    /* Initialize structures */
    mbedtls_ecp_group_init(&grp);
    mbedtls_mpi_init(&d);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    /* Load secp256r1 group */
    ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0) {
        goto cleanup;
    }

    /* Load private key */
    ret = mbedtls_mpi_read_binary(&d, private_key->private_key,
                                   MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    /* Sign the hash */
    ret = mbedtls_ecdsa_sign(&grp, &r, &s, &d, hash, MBEDTLS_WRAPPER_SHA256_SIZE,
                             mbedtls_ctr_drbg_random, &g_ctr_drbg_ctx);
    if (ret != 0) {
        goto cleanup;
    }

    /* Export signature (R || S) */
    ret = mbedtls_mpi_write_binary(&r, signature,
                                    MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_mpi_write_binary(&s, signature + MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE,
                                    MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    ret = MBEDTLS_WRAPPER_OK;

cleanup:
    mbedtls_ecp_group_free(&grp);
    mbedtls_mpi_free(&d);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);

    if (ret != MBEDTLS_WRAPPER_OK) {
        secure_zero(signature, MBEDTLS_WRAPPER_ECC_P256_SIG_SIZE);
        return convert_mbedtls_error(ret);
    }

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_ecdsa_p256_verify_hash(
    const uint8_t *public_key,
    const uint8_t *hash,
    const uint8_t *signature)
{
    int ret;
    mbedtls_ecp_group grp;
    mbedtls_ecp_point Q;
    mbedtls_mpi r, s;

    if (!g_initialized || public_key == NULL || hash == NULL || signature == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    /* Initialize structures */
    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&Q);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    /* Load secp256r1 group */
    ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0) {
        goto cleanup;
    }

    /* Load public key */
    ret = mbedtls_ecp_point_read_binary(&grp, &Q, public_key,
                                         MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE * 2 + 1);
    if (ret != 0) {
        goto cleanup;
    }

    /* Load signature components */
    ret = mbedtls_mpi_read_binary(&r, signature, MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_mpi_read_binary(&s, signature + MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE,
                                   MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE);
    if (ret != 0) {
        goto cleanup;
    }

    /* Verify signature */
    ret = mbedtls_ecdsa_verify(&grp, hash, MBEDTLS_WRAPPER_SHA256_SIZE, &Q, &r, &s);
    if (ret != 0) {
        goto cleanup;
    }

    ret = MBEDTLS_WRAPPER_OK;

cleanup:
    mbedtls_ecp_group_free(&grp);
    mbedtls_ecp_point_free(&Q);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);

    if (ret != MBEDTLS_WRAPPER_OK) {
        return MBEDTLS_WRAPPER_ERROR;
    }

    return MBEDTLS_WRAPPER_OK;
}

/* ============================================================================
 * AES-128-GCM Encryption/Decryption
 * ============================================================================ */

mbedtls_wrapper_status_t mbedtls_wrapper_aes_gcm_init(
    mbedtls_wrapper_aes_gcm_ctx_t *ctx,
    const uint8_t *key,
    const uint8_t *iv,
    uint32_t iv_len,
    mbedtls_wrapper_aes_gcm_mode_t mode)
{
    if (ctx == NULL || key == NULL || iv == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    if (iv_len != MBEDTLS_WRAPPER_AES_GCM_IV_SIZE) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    memcpy(ctx->key, key, MBEDTLS_WRAPPER_AES_128_KEY_SIZE);
    memcpy(ctx->iv, iv, iv_len);
    ctx->key_len = MBEDTLS_WRAPPER_AES_128_KEY_SIZE;
    ctx->iv_len = iv_len;
    ctx->mode = mode;

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_aes_gcm_encrypt(
    const mbedtls_wrapper_aes_gcm_ctx_t *ctx,
    const uint8_t *aad,
    uint32_t aad_len,
    const uint8_t *plaintext,
    uint32_t plaintext_len,
    uint8_t *ciphertext,
    uint8_t *tag)
{
    int ret;
    mbedtls_gcm_context gcm;

    if (!g_initialized || ctx == NULL || plaintext == NULL ||
        ciphertext == NULL || tag == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    mbedtls_gcm_init(&gcm);

    /* Set key */
    ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES,
                             ctx->key, ctx->key_len * 8);
    if (ret != 0) {
        goto cleanup;
    }

    /* Encrypt and authenticate */
    ret = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT,
                                    plaintext_len,
                                    ctx->iv, ctx->iv_len,
                                    aad, aad_len,
                                    plaintext, ciphertext,
                                    MBEDTLS_WRAPPER_AES_GCM_TAG_SIZE, tag);
    if (ret != 0) {
        goto cleanup;
    }

    ret = MBEDTLS_WRAPPER_OK;

cleanup:
    mbedtls_gcm_free(&gcm);

    if (ret != MBEDTLS_WRAPPER_OK) {
        secure_zero(ciphertext, plaintext_len);
        secure_zero(tag, MBEDTLS_WRAPPER_AES_GCM_TAG_SIZE);
        return convert_mbedtls_error(ret);
    }

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_aes_gcm_decrypt(
    const mbedtls_wrapper_aes_gcm_ctx_t *ctx,
    const uint8_t *aad,
    uint32_t aad_len,
    const uint8_t *ciphertext,
    uint32_t ciphertext_len,
    const uint8_t *tag,
    uint8_t *plaintext)
{
    int ret;
    mbedtls_gcm_context gcm;

    if (!g_initialized || ctx == NULL || ciphertext == NULL ||
        plaintext == NULL || tag == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    mbedtls_gcm_init(&gcm);

    /* Set key */
    ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES,
                             ctx->key, ctx->key_len * 8);
    if (ret != 0) {
        goto cleanup;
    }

    /* Decrypt and verify */
    ret = mbedtls_gcm_auth_decrypt(&gcm, ciphertext_len,
                                   ctx->iv, ctx->iv_len,
                                   aad, aad_len,
                                   tag, MBEDTLS_WRAPPER_AES_GCM_TAG_SIZE,
                                   ciphertext, plaintext);
    if (ret != 0) {
        goto cleanup;
    }

    ret = MBEDTLS_WRAPPER_OK;

cleanup:
    mbedtls_gcm_free(&gcm);

    if (ret != MBEDTLS_WRAPPER_OK) {
        secure_zero(plaintext, ciphertext_len);
        return MBEDTLS_WRAPPER_ERROR;
    }

    return MBEDTLS_WRAPPER_OK;
}

void mbedtls_wrapper_aes_gcm_clear(mbedtls_wrapper_aes_gcm_ctx_t *ctx)
{
    if (ctx != NULL) {
        secure_zero(ctx, sizeof(mbedtls_wrapper_aes_gcm_ctx_t));
    }
}

/* ============================================================================
 * HKDF Key Derivation
 * ============================================================================ */

mbedtls_wrapper_status_t mbedtls_wrapper_hkdf_derive(
    const uint8_t *ikm,
    uint32_t ikm_len,
    const uint8_t *salt,
    uint32_t salt_len,
    const uint8_t *info,
    uint32_t info_len,
    uint8_t *okm,
    uint32_t okm_len)
{
    int ret;
    const mbedtls_md_info_t *md_info;

    if (!g_initialized || ikm == NULL || okm == NULL ||
        okm_len > MBEDTLS_WRAPPER_HKDF_MAX_SIZE) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) {
        return MBEDTLS_WRAPPER_CRYPTO_ERROR;
    }

    ret = mbedtls_hkdf(md_info,
                       salt, salt_len,
                       ikm, ikm_len,
                       info, info_len,
                       okm, okm_len);
    if (ret != 0) {
        return convert_mbedtls_error(ret);
    }

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_hkdf_extract(
    const uint8_t *salt,
    uint32_t salt_len,
    const uint8_t *ikm,
    uint32_t ikm_len,
    uint8_t *prk)
{
    int ret;
    const mbedtls_md_info_t *md_info;

    if (!g_initialized || ikm == NULL || prk == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) {
        return MBEDTLS_WRAPPER_CRYPTO_ERROR;
    }

    ret = mbedtls_hkdf_extract(md_info,
                               salt, salt_len,
                               ikm, ikm_len,
                               prk);
    if (ret != 0) {
        return convert_mbedtls_error(ret);
    }

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_hkdf_expand(
    const uint8_t *prk,
    uint32_t prk_len,
    const uint8_t *info,
    uint32_t info_len,
    uint8_t *okm,
    uint32_t okm_len)
{
    int ret;
    const mbedtls_md_info_t *md_info;

    if (!g_initialized || prk == NULL || okm == NULL ||
        okm_len > MBEDTLS_WRAPPER_HKDF_MAX_SIZE) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) {
        return MBEDTLS_WRAPPER_CRYPTO_ERROR;
    }

    ret = mbedtls_hkdf_expand(md_info, prk, prk_len,
                              info, info_len,
                              okm, okm_len);
    if (ret != 0) {
        return convert_mbedtls_error(ret);
    }

    return MBEDTLS_WRAPPER_OK;
}

/* ============================================================================
 * SHA-256 Hashing
 * ============================================================================ */

mbedtls_wrapper_status_t mbedtls_wrapper_sha256(
    const uint8_t *input,
    uint32_t input_len,
    uint8_t *hash)
{
    int ret;

    if (input == NULL || hash == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    ret = mbedtls_sha256_ret(input, input_len, hash, 0);
    if (ret != 0) {
        return MBEDTLS_WRAPPER_CRYPTO_ERROR;
    }

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_sha256_init(void **ctx)
{
    mbedtls_sha256_context *sha_ctx;

    if (ctx == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    sha_ctx = (mbedtls_sha256_context *)mbedtls_calloc(1, sizeof(mbedtls_sha256_context));
    if (sha_ctx == NULL) {
        return MBEDTLS_WRAPPER_NO_MEMORY;
    }

    mbedtls_sha256_init(sha_ctx);
    *ctx = sha_ctx;

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_sha256_update(
    void *ctx,
    const uint8_t *input,
    uint32_t input_len)
{
    int ret;

    if (ctx == NULL || input == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    ret = mbedtls_sha256_update_ret((mbedtls_sha256_context *)ctx, input, input_len);
    if (ret != 0) {
        return MBEDTLS_WRAPPER_CRYPTO_ERROR;
    }

    return MBEDTLS_WRAPPER_OK;
}

mbedtls_wrapper_status_t mbedtls_wrapper_sha256_final(void *ctx, uint8_t *hash)
{
    int ret;

    if (ctx == NULL || hash == NULL) {
        return MBEDTLS_WRAPPER_INVALID_PARAM;
    }

    ret = mbedtls_sha256_finish_ret((mbedtls_sha256_context *)ctx, hash);
    if (ret != 0) {
        return MBEDTLS_WRAPPER_CRYPTO_ERROR;
    }

    mbedtls_sha256_free((mbedtls_sha256_context *)ctx);
    mbedtls_free(ctx);

    return MBEDTLS_WRAPPER_OK;
}

/* ============================================================================
 * Private Helper Functions
 * ============================================================================ */

static mbedtls_wrapper_status_t convert_mbedtls_error(int mbedtls_err)
{
    switch (mbedtls_err) {
        case 0:
            return MBEDTLS_WRAPPER_OK;
        case MBEDTLS_ERR_MD_FEATURE_UNAVAILABLE:
        case MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE:
        case MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE:
            return MBEDTLS_WRAPPER_INVALID_PARAM;
        case MBEDTLS_ERR_MD_ALLOC_FAILED:
        case MBEDTLS_ERR_CIPHER_ALLOC_FAILED:
        case MBEDTLS_ERR_ECP_ALLOC_FAILED:
            return MBEDTLS_WRAPPER_NO_MEMORY;
        default:
            return MBEDTLS_WRAPPER_CRYPTO_ERROR;
    }
}

static void secure_zero(void *ptr, size_t len)
{
    volatile unsigned char *p = ptr;
    while (len--) {
        *p++ = 0;
    }
}

/* ============================================================================
 * End of File
 * ============================================================================ */
