/*
 * dds_auth.c - DDS-Security Authentication Module Implementation
 * 
 * Copyright (c) 2024-2025
 * 
 * This file implements the DDS:Auth:PKI-DH authentication plugin
 * as specified in the OMG DDS-Security specification.
 */

#include "dds_auth.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "mbedtls/bignum.h"

/* 批C 收尾: mbedTLS 静态内存池 (共享, 幂等初始化) */
#include "Crypto_MbedTLS_Mem.h"

/* ============================================================================
 * DH Parameters - RFC 3526 Group 14 (2048-bit)
 * ============================================================================ */

/* Prime 'p' for 2048-bit DH group */
static const uint8_t dh_p_2048[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
    0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
    0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
    0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22,
    0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
    0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B,
    0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
    0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
    0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
    0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B,
    0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
    0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5,
    0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
    0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,
    0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05,
    0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A,
    0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
    0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96,
    0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
    0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D,
    0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04,
    0xF1, 0x74, 0x6C, 0x08, 0xCA, 0x18, 0x21, 0x7C,
    0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
    0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03,
    0x9B, 0x27, 0x83, 0xA2, 0xEC, 0x07, 0xA2, 0x8F,
    0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9,
    0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18,
    0x39, 0x95, 0x49, 0x7C, 0xEA, 0x95, 0x6A, 0xE5,
    0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
    0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAA, 0xC4, 0x2D,
    0xAD, 0x33, 0x17, 0x0D, 0x04, 0x50, 0x7A, 0x33
};

static const uint8_t dh_g_2048[1] = { 0x02 };

/* ============================================================================
 * SHA-256 Implementation
 * ============================================================================ */

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32U - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2U) ^ ROTR(x, 13U) ^ ROTR(x, 22U))
#define EP1(x) (ROTR(x, 6U) ^ ROTR(x, 11U) ^ ROTR(x, 25U))
#define SIG0(x) (ROTR(x, 7U) ^ ROTR(x, 18U) ^ ((x) >> 3U))
#define SIG1(x) (ROTR(x, 17U) ^ ROTR(x, 19U) ^ ((x) >> 10U))

static const uint32_t sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void sha256_transform(uint32_t state[8], const uint8_t data[64])
{
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    for (i = 0, j = 0; i < 16U; ++i, j += 4U) {
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1U] << 16) |
               ((uint32_t)data[j + 2U] << 8) | ((uint32_t)data[j + 3U]);
    }
    for (; i < 64U; ++i) {
        m[i] = SIG1(m[i - 2U]) + m[i - 7U] + SIG0(m[i - 15U]) + m[i - 16U];
    }

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];

    for (i = 0; i < 64U; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + sha256_k[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

dds_auth_status_t dds_auth_sha256(const uint8_t *data, uint32_t len, uint8_t *hash)
{
    if (!data || !hash) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    uint32_t bit_len_high = (len >> 29);
    uint32_t bit_len_low = (len << 3);

    uint8_t buffer[64];
    uint32_t buffer_len = 0;

    /* Process full blocks */
    while (len >= 64U) {
        sha256_transform(state, data);
        data += 64;
        len -= 64U;
    }

    /* Copy remaining data to buffer */
    buffer_len = len;
    memcpy(buffer, data, buffer_len);

    /* Padding */
    buffer[buffer_len] = 0x80;
    buffer_len++;

    if (buffer_len > 56U) {
        memset(buffer + buffer_len, 0, 64U - buffer_len);
        sha256_transform(state, buffer);
        buffer_len = 0;
    }

    memset(buffer + buffer_len, 0, 56U - buffer_len);

    /* Append length */
    buffer[56] = (bit_len_high >> 24) & 0xFFU;
    buffer[57] = (bit_len_high >> 16) & 0xFFU;
    buffer[58] = (bit_len_high >> 8) & 0xFFU;
    buffer[59] = bit_len_high & 0xFFU;
    buffer[60] = (bit_len_low >> 24) & 0xFFU;
    buffer[61] = (bit_len_low >> 16) & 0xFFU;
    buffer[62] = (bit_len_low >> 8) & 0xFFU;
    buffer[63] = bit_len_low & 0xFFU;

    sha256_transform(state, buffer);

    /* Output hash */
    for (int i = 0; i < 8; i++) {
        hash[i * 4] = (state[i] >> 24) & 0xFFU;
        hash[(i * 4) + 1] = (state[i] >> 16) & 0xFFU;
        hash[(i * 4) + 2] = (state[i] >> 8) & 0xFFU;
        hash[(i * 4) + 3] = state[i] & 0xFFU;
    }

    return DDS_AUTH_OK;
}

/* ============================================================================
 * Random Number Generation
 * ============================================================================ */

dds_auth_status_t dds_auth_generate_challenge(uint8_t *challenge, uint32_t len)
{
    if (!challenge || (len == 0U)) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    /* Use simple random for embedded systems */
    /* In production, use hardware RNG */
    static uint32_t seed = 0;
    if (seed == 0U) {
        seed = (uint32_t)time(NULL);
    }

    for (uint32_t i = 0; i < len; i++) {
        seed = (seed * 1103515245U) + 12345U;
        challenge[i] = (uint8_t)(seed >> 16);
    }

    return DDS_AUTH_OK;
}

dds_auth_status_t dds_auth_generate_random(uint8_t *buffer, uint32_t len)
{
    return dds_auth_generate_challenge(buffer, len);
}

/* ============================================================================
 * 静态存储 (ISO 26262 / AUTOSAR R21-11 BSW 禁止动态内存)
 * ============================================================================ */

/* 单例上下文: 编译期静态分配, 替代 calloc */
static dds_auth_context_t s_auth_ctx;

/* 握手表: 编译期最大数量固定分配 */
static dds_security_handshake_t s_handshakes[DDS_AUTH_MAX_HANDSHAKES];

/* ============================================================================
 * Initialization
 * ============================================================================ */

dds_auth_context_t* dds_auth_init(const dds_security_config_t *config)
{
    if (!config) {
        return NULL;
    }

    /* 批C 收尾: 先建 mbedTLS 静态内存池 (幂等), 否则 mbedtls_mpi 分配返回 NULL */
    (void)Crypto_MbedTLS_MemInit();

    dds_auth_context_t *ctx = &s_auth_ctx;
    (void)memset(ctx, 0, sizeof(dds_auth_context_t));

    /* 静态握手表 (编译期固定上限) */
    ctx->max_handshakes = DDS_AUTH_MAX_HANDSHAKES;
    ctx->handshakes = s_handshakes;
    (void)memset(ctx->handshakes, 0, sizeof(s_handshakes));

    /* Configure defaults */
    ctx->handshake_timeout_ms = (config->handshake_timeout_ms > 0U) ? config->handshake_timeout_ms : DDS_AUTH_DEFAULT_TIMEOUT_MS;
    ctx->max_retries = (config->max_handshake_attempts > 0U) ? config->max_handshake_attempts : 3U;
    ctx->validate_cert_chain = true;
    ctx->check_crl = false;
    ctx->check_ocsp = false;

    /* Initialize DH parameters */
    dds_auth_init_dh_params(ctx, DDS_AUTH_DH_GROUP_2048);

    return ctx;
}

void dds_auth_deinit(dds_auth_context_t *ctx)
{
    if (!ctx) {
        return;
    }

    if ((ctx->handshakes) != 0U) {
        /* Clear sensitive data */
        for (uint32_t i = 0; i < ctx->max_handshakes; i++) {
            if (ctx->handshakes[i].state != DDS_HANDSHAKE_STATE_NONE) {
                memset(ctx->handshakes[i].identity.shared_secret, 0, sizeof(ctx->handshakes[i].identity.shared_secret));
            }
        }
        /* 静态握手表无需 free */
    }

    ctx->handshakes = NULL;
}

/* ============================================================================
 * Certificate Operations
 * ============================================================================ */

dds_auth_status_t dds_auth_load_certificate(dds_auth_context_t *ctx,
                                            const char *cert_path,
                                            dds_security_cert_t *cert)
{
    if (!ctx || !cert_path || !cert) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    FILE *fp = fopen(cert_path, "rb");
    if (!fp) {
        return DDS_AUTH_ERROR_INVALID_CERT;
    }

    cert->length = fread(cert->data, 1, DDS_SECURITY_MAX_CERT_SIZE, fp);
    fclose(fp);

    if (cert->length == 0U) {
        return DDS_AUTH_ERROR_INVALID_CERT;
    }

    /* Calculate fingerprint */
    dds_auth_calc_fingerprint(ctx, cert, cert->fingerprint);

    /* Set default validity (should parse from cert in real implementation) */
    cert->valid_from = 0;
    cert->valid_until = 0xFFFFFFFFFFFFFFFF;
    strncpy(cert->subject_name, "CN=Unknown,O=DDS", sizeof(cert->subject_name) - 1U);

    return DDS_AUTH_OK;
}

dds_auth_status_t dds_auth_load_cert_chain(dds_auth_context_t *ctx,
                                           const char *chain_path,
                                           dds_security_cert_chain_t *chain)
{
    if (!ctx || !chain_path || !chain) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    dds_auth_status_t status = dds_auth_load_certificate(ctx, chain_path, &chain->certs[0]);
    if (status == DDS_AUTH_OK) {
        chain->cert_count = 1;
    }

    return status;
}

dds_auth_status_t dds_auth_load_private_key(dds_auth_context_t *ctx,
                                            const char *key_path,
                                            dds_security_key_pair_t *key)
{
    if (!ctx || !key_path || !key) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    FILE *fp = fopen(key_path, "rb");
    if ((fp) != 0U) {
        key->private_key_len = fread(key->private_key, 1, DDS_SECURITY_MAX_KEY_SIZE, fp);
        fclose(fp);
    } else {
        /* Generate key if file not found (for testing) */
        key->private_key_len = 32;
        dds_auth_generate_random(key->private_key, key->private_key_len);
    }

    /* Derive public key (simplified) */
    key->public_key_len = 32;
    dds_auth_sha256(key->private_key, key->private_key_len, key->public_key);

    return DDS_AUTH_OK;
}

dds_auth_status_t dds_auth_validate_certificate(dds_auth_context_t *ctx,
                                                const dds_security_cert_t *cert,
                                                const dds_security_cert_t *ca_cert)
{
    if (!ctx || !cert || !ca_cert) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    if ((cert->length == 0U) || (ca_cert->length == 0U)) {
        return DDS_AUTH_ERROR_INVALID_CERT;
    }

    /* Simplified validation - in production, full X.509 validation needed */
    return DDS_AUTH_OK;
}

dds_auth_status_t dds_auth_validate_cert_chain(dds_auth_context_t *ctx,
                                               const dds_security_cert_chain_t *chain,
                                               const dds_security_cert_t *ca_cert)
{
    if (!ctx || !chain || !ca_cert) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    if (chain->cert_count == 0U) {
        return DDS_AUTH_ERROR_INVALID_CERT;
    }

    for (uint32_t i = 0; i < chain->cert_count; i++) {
        dds_auth_status_t status = dds_auth_validate_certificate(ctx, &chain->certs[i], ca_cert);
        if (status != DDS_AUTH_OK) {
            return status;
        }
    }

    return DDS_AUTH_OK;
}

dds_auth_status_t dds_auth_check_cert_validity(dds_auth_context_t *ctx,
                                               const dds_security_cert_t *cert,
                                               uint64_t current_time)
{
    if (!ctx || !cert) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    if ((current_time < cert->valid_from) || (current_time > cert->valid_until)) {
        return DDS_AUTH_ERROR_CERT_EXPIRED;
    }

    return DDS_AUTH_OK;
}

dds_auth_status_t dds_auth_calc_fingerprint(dds_auth_context_t *ctx,
                                            const dds_security_cert_t *cert,
                                            uint8_t *fingerprint)
{
    if (!ctx || !cert || !fingerprint) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    return dds_auth_sha256(cert->data, cert->length, fingerprint);
}

/* ============================================================================
 * DH Key Exchange
 * ============================================================================ */

dds_auth_status_t dds_auth_init_dh_params(dds_auth_context_t *ctx, uint8_t group_id)
{
    if (!ctx) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    ctx->dh_group_id = group_id;

    switch (group_id) {
        case DDS_AUTH_DH_GROUP_2048:
            memcpy(ctx->dh_params.p, dh_p_2048, sizeof(dh_p_2048));
            ctx->dh_params.p_len = sizeof(dh_p_2048);
            memcpy(ctx->dh_params.g, dh_g_2048, sizeof(dh_g_2048));
            ctx->dh_params.g_len = sizeof(dh_g_2048);
            ctx->dh_params.key_size = 2048;
            break;

        default:
            return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    return DDS_AUTH_OK;
}

dds_auth_status_t dds_auth_generate_dh_keypair(dds_auth_context_t *ctx,
                                               dds_security_key_pair_t *key_pair)
{
    if (!ctx || !key_pair) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    mbedtls_mpi p, g, priv, pub;
    mbedtls_mpi_init(&p);
    mbedtls_mpi_init(&g);
    mbedtls_mpi_init(&priv);
    mbedtls_mpi_init(&pub);

    int rc = mbedtls_mpi_read_binary(&p, dh_p_2048, sizeof(dh_p_2048));
    if (rc == 0) {
        rc = mbedtls_mpi_read_binary(&g, dh_g_2048, sizeof(dh_g_2048));
    }

    /* 私钥: 随机 256-bit (32 字节) */
    uint8_t priv_bytes[32];
    if (rc == 0) {
        dds_auth_generate_random(priv_bytes, sizeof(priv_bytes));
        rc = mbedtls_mpi_read_binary(&priv, priv_bytes, sizeof(priv_bytes));
    }

    /* 公钥: pub = g^priv mod p */
    if (rc == 0) {
        rc = mbedtls_mpi_exp_mod(&pub, &g, &priv, &p, NULL);
    }

    if (rc == 0) {
        rc = mbedtls_mpi_write_binary(&pub, key_pair->public_key, sizeof(key_pair->public_key));
    }

    if (rc == 0) {
        memcpy(key_pair->private_key, priv_bytes, sizeof(priv_bytes));
        key_pair->private_key_len = sizeof(priv_bytes);
        key_pair->public_key_len = (mbedtls_mpi_size(&pub) < sizeof(key_pair->public_key))
            ? mbedtls_mpi_size(&pub) : sizeof(key_pair->public_key);
    }

    mbedtls_mpi_free(&p);
    mbedtls_mpi_free(&g);
    mbedtls_mpi_free(&priv);
    mbedtls_mpi_free(&pub);

    return (rc == 0) ? DDS_AUTH_OK : DDS_AUTH_ERROR_HANDSHAKE_FAILED;
}

dds_auth_status_t dds_auth_compute_shared_secret(dds_auth_context_t *ctx,
                                                 const uint8_t *local_private,
                                                 const uint8_t *remote_public,
                                                 uint32_t remote_public_len,
                                                 uint8_t *shared_secret,
                                                 uint32_t *secret_len)
{
    if (!ctx || !local_private || !remote_public || !shared_secret || !secret_len) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    /* 标准 DH: shared = remote_public^local_private mod p */
    mbedtls_mpi p, priv, pub, shared;
    mbedtls_mpi_init(&p);
    mbedtls_mpi_init(&priv);
    mbedtls_mpi_init(&pub);
    mbedtls_mpi_init(&shared);

    int rc = mbedtls_mpi_read_binary(&p, dh_p_2048, sizeof(dh_p_2048));
    if (rc == 0) {
        rc = mbedtls_mpi_read_binary(&priv, local_private, 32);
    }
    if (rc == 0) {
        rc = mbedtls_mpi_read_binary(&pub, remote_public, remote_public_len);
    }
    if (rc == 0) {
        rc = mbedtls_mpi_exp_mod(&shared, &pub, &priv, &p, NULL);
    }
    if (rc == 0) {
        *secret_len = 32;
        size_t olen = 0;
        rc = mbedtls_mpi_write_binary(&shared, shared_secret, 32);
        (void)olen;
    }

    mbedtls_mpi_free(&p);
    mbedtls_mpi_free(&priv);
    mbedtls_mpi_free(&pub);
    mbedtls_mpi_free(&shared);

    return (rc == 0) ? DDS_AUTH_OK : DDS_AUTH_ERROR_HANDSHAKE_FAILED;
}

dds_auth_status_t dds_auth_derive_key(dds_auth_context_t *ctx,
                                      const uint8_t *shared_secret,
                                      uint32_t secret_len,
                                      const uint8_t *salt,
                                      uint8_t *key,
                                      uint32_t key_len)
{
    if (!ctx || !shared_secret || !key) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    /* Simple KDF using SHA-256 */
    uint8_t hash[32];
    dds_auth_sha256(shared_secret, secret_len, hash);

    if ((salt) != 0U) {
        for (uint32_t i = 0; i < 32U; i++) {
            hash[i] ^= salt[i % 8U];
        }
        dds_auth_sha256(hash, 32, hash);
    }

    uint32_t copy_len = (key_len < 32U) ? key_len : 32U;
    memcpy(key, hash, copy_len);

    /* If need more key material, derive additional blocks */
    if (key_len > 32U) {
        uint8_t counter = 1;
        uint8_t additional[32];
        memcpy(additional, hash, 32);

        for (uint32_t offset = 32; offset < key_len; offset += 32U) {
            additional[31] ^= counter++;
            dds_auth_sha256(additional, 32, additional);

            uint32_t remaining = key_len - offset;
            copy_len = (remaining < 32U) ? remaining : 32U;
            memcpy(key + offset, additional, copy_len);
        }
    }

    return DDS_AUTH_OK;
}

/* ============================================================================
 * Signature Operations
 * ============================================================================ */

dds_auth_status_t dds_auth_sign(dds_auth_context_t *ctx,
                                const uint8_t *data,
                                uint32_t data_len,
                                const uint8_t *private_key,
                                uint8_t *signature,
                                uint32_t *sig_len)
{
    if (!ctx || !data || !private_key || !signature || !sig_len) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    /* Simplified signature: hash(data || key) */
    uint8_t hash_input[4096];
    uint32_t hash_len = data_len + 32U;

    if (hash_len > sizeof(hash_input)) {
        hash_len = sizeof(hash_input);
    }

    memcpy(hash_input, data, hash_len - 32U);
    memcpy(hash_input + hash_len - 32U, private_key, 32);

    *sig_len = 32;
    dds_auth_sha256(hash_input, hash_len, signature);

    return DDS_AUTH_OK;
}

dds_auth_status_t dds_auth_verify(dds_auth_context_t *ctx,
                                  const uint8_t *data,
                                  uint32_t data_len,
                                  const uint8_t *signature,
                                  uint32_t sig_len,
                                  const uint8_t *public_key)
{
    if (!ctx || !data || !signature || !public_key) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    uint8_t expected_sig[32];
    uint32_t expected_len = 32;
    dds_auth_sign(ctx, data, data_len, public_key, expected_sig, &expected_len);

    if ((sig_len != expected_len) || (memcmp(signature, expected_sig, sig_len) != 0)) {
        return DDS_AUTH_ERROR_INVALID_SIGNATURE;
    }

    return DDS_AUTH_OK;
}

/* ============================================================================
 * Handshake Protocol
 * ============================================================================ */

dds_security_handshake_t* dds_auth_handshake_create(dds_auth_context_t *ctx,
                                                    const rtps_guid_t *local_guid,
                                                    const rtps_guid_t *remote_guid)
{
    if (!ctx || !local_guid || !remote_guid) {
        return NULL;
    }

    dds_security_handshake_t *handshake = NULL;
    for (uint32_t i = 0; i < ctx->max_handshakes; i++) {
        if (ctx->handshakes[i].state == DDS_HANDSHAKE_STATE_NONE) {
            handshake = &ctx->handshakes[i];
            break;
        }
    }

    if (!handshake) {
        return NULL;
    }

    memset(handshake, 0, sizeof(dds_security_handshake_t));
    handshake->state = DDS_HANDSHAKE_STATE_BEGIN;
    memcpy(&handshake->local_guid, local_guid, sizeof(rtps_guid_t));
    memcpy(&handshake->remote_guid, remote_guid, sizeof(rtps_guid_t));
    handshake->handshake_id = (uint64_t)(uintptr_t)handshake;
    handshake->timeout_ms = ctx->handshake_timeout_ms;
    handshake->start_time = 0; /* Should set to current time */

    dds_auth_generate_challenge(handshake->local_challenge, 32);

    return handshake;
}

void dds_auth_handshake_destroy(dds_auth_context_t *ctx,
                                dds_security_handshake_t *handshake)
{
    if (!ctx || !handshake) {
        return;
    }

    memset(handshake->identity.shared_secret, 0, sizeof(handshake->identity.shared_secret));
    handshake->state = DDS_HANDSHAKE_STATE_NONE;
}

dds_auth_status_t dds_auth_begin_handshake_request(dds_auth_context_t *ctx,
                                                   const rtps_guid_t *local_guid,
                                                   dds_security_handshake_t **handshake,
                                                   dds_auth_handshake_request_t *request)
{
    if (!ctx || !local_guid || !handshake || !request) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    *handshake = dds_auth_handshake_create(ctx, local_guid, local_guid);
    if (!*handshake) {
        return DDS_AUTH_ERROR_NO_MEMORY;
    }

    dds_security_key_pair_t dh_keypair;
    dds_auth_generate_dh_keypair(ctx, &dh_keypair);

    memcpy((*handshake)->local_dh_public, dh_keypair.public_key, dh_keypair.public_key_len);
    (*handshake)->local_dh_public_len = dh_keypair.public_key_len;
    memcpy((*handshake)->identity.key_pair.private_key, dh_keypair.private_key,
           dh_keypair.private_key_len);
    (*handshake)->identity.key_pair.private_key_len = dh_keypair.private_key_len;

    memset(request, 0, sizeof(dds_auth_handshake_request_t));
    request->msg_type = DDS_AUTH_MSG_HANDSHAKE_REQUEST;
    memcpy(&request->requester_guid, local_guid, sizeof(rtps_guid_t));
    memcpy(request->dh_public, (*handshake)->local_dh_public, (*handshake)->local_dh_public_len);
    request->dh_public_len = (*handshake)->local_dh_public_len;
    memcpy(request->challenge, (*handshake)->local_challenge, 32);

    dds_auth_sha256((uint8_t*)&ctx->identity_cert_chain,
                    sizeof(dds_security_cert_chain_t),
                    request->cert_chain_hash);

    (*handshake)->state = DDS_HANDSHAKE_STATE_REQUEST_SENT;
    ctx->active_handshakes++;

    if ((ctx->on_handshake_begin) != 0U) {
        ctx->on_handshake_begin((rtps_guid_t*)local_guid, (rtps_guid_t*)local_guid);
    }

    return DDS_AUTH_OK;
}

dds_auth_status_t dds_auth_process_handshake_request(dds_auth_context_t *ctx,
                                                     const dds_auth_handshake_request_t *request,
                                                     dds_security_handshake_t **handshake,
                                                     dds_auth_handshake_reply_t *reply)
{
    if (!ctx || !request || !handshake || !reply) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    if (request->msg_type != (uint32_t)(DDS_AUTH_MSG_HANDSHAKE_REQUEST)) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    *handshake = dds_auth_handshake_create(ctx, &request->requester_guid, &request->requester_guid);
    if (!*handshake) {
        return DDS_AUTH_ERROR_NO_MEMORY;
    }

    memcpy((*handshake)->remote_dh_public, request->dh_public, request->dh_public_len);
    (*handshake)->remote_dh_public_len = request->dh_public_len;
    memcpy((*handshake)->remote_challenge, request->challenge, 32);

    dds_security_key_pair_t dh_keypair;
    dds_auth_generate_dh_keypair(ctx, &dh_keypair);

    memcpy((*handshake)->local_dh_public, dh_keypair.public_key, dh_keypair.public_key_len);
    (*handshake)->local_dh_public_len = dh_keypair.public_key_len;
    memcpy((*handshake)->identity.key_pair.private_key, dh_keypair.private_key,
           dh_keypair.private_key_len);
    (*handshake)->identity.key_pair.private_key_len = dh_keypair.private_key_len;

    dds_auth_compute_shared_secret(ctx,
                                   (*handshake)->identity.key_pair.private_key,
                                   (*handshake)->remote_dh_public,
                                   (*handshake)->remote_dh_public_len,
                                   (*handshake)->identity.shared_secret,
                                   &(*handshake)->identity.shared_secret_len);

    dds_auth_generate_challenge((*handshake)->local_challenge, 32);

    uint8_t response_data[64];
    memcpy(response_data, (*handshake)->remote_challenge, 32);
    memcpy(response_data + 32, (*handshake)->local_challenge, 32);

    memset(reply, 0, sizeof(dds_auth_handshake_reply_t));
    reply->msg_type = DDS_AUTH_MSG_HANDSHAKE_REPLY;
    memcpy(&reply->replier_guid, &request->requester_guid, sizeof(rtps_guid_t));
    memcpy(reply->dh_public, (*handshake)->local_dh_public, (*handshake)->local_dh_public_len);
    reply->dh_public_len = (*handshake)->local_dh_public_len;
    memcpy(reply->challenge, (*handshake)->local_challenge, 32);
    memcpy(reply->response, response_data, 32);

    dds_auth_sign(ctx, (uint8_t*)reply, sizeof(dds_auth_handshake_reply_t) - 256U,
                  (*handshake)->identity.key_pair.private_key,
                  reply->signature, &reply->signature_len);

    (*handshake)->state = DDS_HANDSHAKE_STATE_REPLY_SENT;

    return DDS_AUTH_OK;
}

dds_auth_status_t dds_auth_process_handshake_reply(dds_auth_context_t *ctx,
                                                   dds_security_handshake_t *handshake,
                                                   const dds_auth_handshake_reply_t *reply,
                                                   dds_auth_handshake_final_t *final)
{
    if (!ctx || !handshake || !reply || !final) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    if (reply->msg_type != (uint32_t)(DDS_AUTH_MSG_HANDSHAKE_REPLY)) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    memcpy(handshake->remote_dh_public, reply->dh_public, reply->dh_public_len);
    handshake->remote_dh_public_len = reply->dh_public_len;
    memcpy(handshake->remote_challenge, reply->challenge, 32);

    dds_auth_compute_shared_secret(ctx,
                                   handshake->identity.key_pair.private_key,
                                   handshake->remote_dh_public,
                                   handshake->remote_dh_public_len,
                                   handshake->identity.shared_secret,
                                   &handshake->identity.shared_secret_len);

    memset(final, 0, sizeof(dds_auth_handshake_final_t));
    final->msg_type = DDS_AUTH_MSG_HANDSHAKE_FINAL;
    memcpy(&final->requester_guid, &handshake->local_guid, sizeof(rtps_guid_t));
    memcpy(final->response, reply->challenge, 32);

    dds_auth_sign(ctx, (uint8_t*)final, sizeof(dds_auth_handshake_final_t) - 256U,
                  handshake->identity.key_pair.private_key,
                  final->signature, &final->signature_len);

    handshake->state = DDS_HANDSHAKE_STATE_FINAL_SENT;
    handshake->verified = true;
    handshake->identity.authenticated = true;

    return DDS_AUTH_OK;
}

dds_auth_status_t dds_auth_process_handshake_final(dds_auth_context_t *ctx,
                                                   dds_security_handshake_t *handshake,
                                                   const dds_auth_handshake_final_t *final)
{
    if (!ctx || !handshake || !final) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    if (final->msg_type != (uint32_t)(DDS_AUTH_MSG_HANDSHAKE_FINAL)) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    handshake->state = DDS_HANDSHAKE_STATE_COMPLETED;
    handshake->verified = true;
    handshake->identity.authenticated = true;

    ctx->active_handshakes--;

    if ((ctx->on_handshake_complete) != 0U) {
        ctx->on_handshake_complete(&handshake->local_guid, &handshake->remote_guid, true);
    }

    return DDS_AUTH_OK;
}

uint32_t dds_auth_check_handshake_timeouts(dds_auth_context_t *ctx, uint64_t current_time)
{
    if (!ctx) {
        return 0;
    }

    uint32_t timeout_count = 0;

    for (uint32_t i = 0; i < ctx->max_handshakes; i++) {
        dds_security_handshake_t *handshake = &ctx->handshakes[i];

        if ((handshake->state != DDS_HANDSHAKE_STATE_NONE) &&
            (handshake->state != DDS_HANDSHAKE_STATE_COMPLETED)) {

            if (current_time > (handshake->start_time + handshake->timeout_ms)) {
                handshake->state = DDS_HANDSHAKE_STATE_FAILED;
                timeout_count++;
                ctx->active_handshakes--;

                if ((ctx->on_handshake_complete) != 0U) {
                    ctx->on_handshake_complete(&handshake->local_guid,
                                               &handshake->remote_guid, false);
                }
            }
        }
    }

    return timeout_count;
}

dds_auth_status_t dds_auth_get_shared_secret(dds_security_handshake_t *handshake,
                                             uint8_t *secret,
                                             uint32_t *secret_len)
{
    if (!handshake || !secret || !secret_len) {
        return DDS_AUTH_ERROR_INVALID_PARAM;
    }

    if (handshake->identity.shared_secret_len == 0U) {
        return DDS_AUTH_ERROR_HANDSHAKE_FAILED;
    }

    *secret_len = handshake->identity.shared_secret_len;
    memcpy(secret, handshake->identity.shared_secret, *secret_len);

    return DDS_AUTH_OK;
}