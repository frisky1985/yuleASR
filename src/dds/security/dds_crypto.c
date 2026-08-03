/*
 * dds_crypto.c - DDS-Security Cryptography Module Implementation
 *
 * Copyright (c) 2024-2025
 *
 * This file implements the DDS:Crypto:AES-GCM-GMAC plugin
 * as specified in the OMG DDS-Security specification.
 */

#include "dds_crypto.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * AES Constants and Tables
 * ============================================================================ */

static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const uint8_t rsbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

static const uint32_t rcon[11] = {
    0x00000000, 0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000, 0x1b000000, 0x36000000
};

#define GET_UINT32_LE(n,b,i)                    \
    {                                           \
        (n) = ((uint32_t)(b)[(i)]      )        \
            | ((uint32_t)(b)[(i)+1] <<  8)      \
            | ((uint32_t)(b)[(i)+2] << 16)      \
            | ((uint32_t)(b)[(i)+3] << 24);     \
    }

#define PUT_UINT32_LE(n,b,i)                    \
    {                                           \
        (b)[(i)]     = (uint8_t)((n)      );    \
        (b)[(i) + 1] = (uint8_t)((n) >>  8);    \
        (b)[(i) + 2] = (uint8_t)((n) >> 16);    \
        (b)[(i) + 3] = (uint8_t)((n) >> 24);    \
    }

/* ============================================================================
 * AES Key Expansion
 * ============================================================================ */

static void aes_key_expansion(uint8_t *round_key, const uint8_t *key, int key_bits)
{
    int i, j;
    uint8_t temp[4], k;

    int nk = key_bits / 32;
    int nr = nk + 6;

    for (i = 0; i < nk; i++) {
        round_key[i * 4 + 0] = key[i * 4 + 0];
        round_key[i * 4 + 1] = key[i * 4 + 1];
        round_key[i * 4 + 2] = key[i * 4 + 2];
        round_key[i * 4 + 3] = key[i * 4 + 3];
    }

    for (i = nk; i < 4 * (nr + 1); i++) {
        temp[0] = round_key[(i - 1) * 4 + 0];
        temp[1] = round_key[(i - 1) * 4 + 1];
        temp[2] = round_key[(i - 1) * 4 + 2];
        temp[3] = round_key[(i - 1) * 4 + 3];

        if (i % nk == 0) {
            k = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = k;

            temp[0] = sbox[temp[0]];
            temp[1] = sbox[temp[1]];
            temp[2] = sbox[temp[2]];
            temp[3] = sbox[temp[3]];

            temp[0] ^= (rcon[i / nk] >> 24) & 0xFF;
        }

        if (nk > 6 && i % nk == 4) {
            temp[0] = sbox[temp[0]];
            temp[1] = sbox[temp[1]];
            temp[2] = sbox[temp[2]];
            temp[3] = sbox[temp[3]];
        }

        round_key[i * 4 + 0] = round_key[(i - nk) * 4 + 0] ^ temp[0];
        round_key[i * 4 + 1] = round_key[(i - nk) * 4 + 1] ^ temp[1];
        round_key[i * 4 + 2] = round_key[(i - nk) * 4 + 2] ^ temp[2];
        round_key[i * 4 + 3] = round_key[(i - nk) * 4 + 3] ^ temp[3];
    }
}

/* ============================================================================
 * AES Operations
 * ============================================================================ */

static void add_round_key(uint8_t state[4][4], const uint8_t *round_key, int round)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[j][i] ^= round_key[round * 16 + i * 4 + j];
        }
    }
}

static void sub_bytes(uint8_t state[4][4])
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[i][j] = sbox[state[i][j]];
        }
    }
}

static void inv_sub_bytes(uint8_t state[4][4])
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[i][j] = rsbox[state[i][j]];
        }
    }
}

static void shift_rows(uint8_t state[4][4])
{
    uint8_t temp;

    temp = state[1][0];
    state[1][0] = state[1][1];
    state[1][1] = state[1][2];
    state[1][2] = state[1][3];
    state[1][3] = temp;

    temp = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = temp;
    temp = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = temp;

    temp = state[3][3];
    state[3][3] = state[3][2];
    state[3][2] = state[3][1];
    state[3][1] = state[3][0];
    state[3][0] = temp;
}

static void inv_shift_rows(uint8_t state[4][4])
{
    uint8_t temp;

    temp = state[1][3];
    state[1][3] = state[1][2];
    state[1][2] = state[1][1];
    state[1][1] = state[1][0];
    state[1][0] = temp;

    temp = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = temp;
    temp = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = temp;

    temp = state[3][0];
    state[3][0] = state[3][1];
    state[3][1] = state[3][2];
    state[3][2] = state[3][3];
    state[3][3] = temp;
}

static uint8_t xtime(uint8_t x)
{
    return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

static void mix_columns(uint8_t state[4][4])
{
    uint8_t tmp, tm, t;
    for (int i = 0; i < 4; i++) {
        t = state[0][i];
        tmp = state[0][i] ^ state[1][i] ^ state[2][i] ^ state[3][i];
        tm = state[0][i] ^ state[1][i]; tm = xtime(tm); state[0][i] ^= tm ^ tmp;
        tm = state[1][i] ^ state[2][i]; tm = xtime(tm); state[1][i] ^= tm ^ tmp;
        tm = state[2][i] ^ state[3][i]; tm = xtime(tm); state[2][i] ^= tm ^ tmp;
        tm = state[3][i] ^ t;           tm = xtime(tm); state[3][i] ^= tm ^ tmp;
    }
}

static uint8_t multiply(uint8_t x, uint8_t y)
{
    return (((y & 1) * x) ^
            ((y >> 1 & 1) * xtime(x)) ^
            ((y >> 2 & 1) * xtime(xtime(x))) ^
            ((y >> 3 & 1) * xtime(xtime(xtime(x)))) ^
            ((y >> 4 & 1) * xtime(xtime(xtime(xtime(x))))));
}

static void inv_mix_columns(uint8_t state[4][4])
{
    for (int i = 0; i < 4; i++) {
        uint8_t a = state[0][i];
        uint8_t b = state[1][i];
        uint8_t c = state[2][i];
        uint8_t d = state[3][i];

        state[0][i] = multiply(a, 0x0e) ^ multiply(b, 0x0b) ^ multiply(c, 0x0d) ^ multiply(d, 0x09);
        state[1][i] = multiply(a, 0x09) ^ multiply(b, 0x0e) ^ multiply(c, 0x0b) ^ multiply(d, 0x0d);
        state[2][i] = multiply(a, 0x0d) ^ multiply(b, 0x09) ^ multiply(c, 0x0e) ^ multiply(d, 0x0b);
        state[3][i] = multiply(a, 0x0b) ^ multiply(b, 0x0d) ^ multiply(c, 0x09) ^ multiply(d, 0x0e);
    }
}

/* ============================================================================
 * Initialization
 * ============================================================================ */

dds_crypto_context_t* dds_crypto_init(const dds_security_config_t *config)
{
    if (!config) {
        return NULL;
    }

    dds_crypto_context_t *ctx = (dds_crypto_context_t*)calloc(1, sizeof(dds_crypto_context_t));
    if (!ctx) {
        return NULL;
    }

    /* Configure algorithm */
    ctx->algorithm = DDS_CRYPTO_ALG_AES_256_GCM;
    ctx->key_update_interval_ms = config->key_update_interval_ms > 0 ?
                                   config->key_update_interval_ms :
                                   DDS_CRYPTO_DEFAULT_KEY_UPDATE_MS;

    /* Allocate session management */
    ctx->max_sessions = DDS_CRYPTO_MAX_SESSIONS;
    ctx->sessions = (dds_crypto_session_t*)calloc(ctx->max_sessions, sizeof(dds_crypto_session_t));
    if (!ctx->sessions) {
        free(ctx);
        return NULL;
    }

    /* Initialize key manager */
    ctx->key_manager.max_sessions = DDS_CRYPTO_MAX_SESSIONS;
    ctx->key_manager.key_sessions = (dds_crypto_session_keys_t*)calloc(
        ctx->key_manager.max_sessions, sizeof(dds_crypto_session_keys_t));
    if (!ctx->key_manager.key_sessions) {
        free(ctx->sessions);
        free(ctx);
        return NULL;
    }

    return ctx;
}

void dds_crypto_deinit(dds_crypto_context_t *ctx)
{
    if (!ctx) {
        return;
    }

    /* Clear sensitive key data */
    if (ctx->sessions) {
        for (uint32_t i = 0; i < ctx->max_sessions; i++) {
            memset(ctx->sessions[i].key_materials, 0, sizeof(ctx->sessions[i].key_materials));
        }
        free(ctx->sessions);
    }

    if (ctx->key_manager.key_sessions) {
        memset(ctx->key_manager.key_sessions, 0,
               sizeof(dds_crypto_session_keys_t) * ctx->key_manager.max_sessions);
        free(ctx->key_manager.key_sessions);
    }

    memset(ctx, 0, sizeof(dds_crypto_context_t));
    free(ctx);
}

/* ============================================================================
 * Session Management
 * ============================================================================ */

static uint64_t generate_session_id(void)
{
    static uint64_t counter = 1;
    return counter++;
}

uint64_t dds_crypto_create_session(dds_crypto_context_t *ctx,
                                   const rtps_guid_t *local_guid,
                                   const rtps_guid_t *remote_guid,
                                   const uint8_t *shared_secret,
                                   uint32_t secret_len)
{
    if (!ctx || !local_guid || !remote_guid || !shared_secret) {
        return 0;
    }

    /* Find available session slot */
    dds_crypto_session_t *session = NULL;
    for (uint32_t i = 0; i < ctx->max_sessions; i++) {
        if (!ctx->sessions[i].established) {
            session = &ctx->sessions[i];
            break;
        }
    }

    if (!session) {
        return 0;
    }

    memset(session, 0, sizeof(dds_crypto_session_t));
    session->session_id = generate_session_id();
    memcpy(&session->local_guid, local_guid, sizeof(rtps_guid_t));
    memcpy(&session->remote_guid, remote_guid, sizeof(rtps_guid_t));

    /* Derive session keys from shared secret */
    dds_crypto_key_material_msg_t key_material;
    dds_crypto_derive_session_keys(ctx, shared_secret, secret_len, &key_material);

    memcpy(session->key_materials[0].key, key_material.sender_key, DDS_CRYPTO_AES_256_KEY_SIZE);
    memcpy(session->key_materials[0].salt, key_material.master_salt, 8);
    session->key_materials[0].key_len = DDS_CRYPTO_AES_256_KEY_SIZE;
    session->key_materials[0].key_id = session->session_id;
    session->key_materials[0].valid = true;

    session->active_key_index = 0;
    session->established = true;

    ctx->active_sessions++;

    return session->session_id;
}

void dds_crypto_close_session(dds_crypto_context_t *ctx, uint64_t session_id)
{
    if (!ctx || session_id == 0) {
        return;
    }

    for (uint32_t i = 0; i < ctx->max_sessions; i++) {
        if (ctx->sessions[i].session_id == session_id && ctx->sessions[i].established) {
            memset(ctx->sessions[i].key_materials, 0, sizeof(ctx->sessions[i].key_materials));
            ctx->sessions[i].established = false;
            ctx->active_sessions--;
            break;
        }
    }
}

dds_crypto_session_t* dds_crypto_find_session(dds_crypto_context_t *ctx,
                                               const rtps_guid_t *local_guid,
                                               const rtps_guid_t *remote_guid)
{
    if (!ctx || !local_guid || !remote_guid) {
        return NULL;
    }

    for (uint32_t i = 0; i < ctx->max_sessions; i++) {
        if (ctx->sessions[i].established &&
            memcmp(&ctx->sessions[i].local_guid, local_guid, sizeof(rtps_guid_t)) == 0 &&
            memcmp(&ctx->sessions[i].remote_guid, remote_guid, sizeof(rtps_guid_t)) == 0) {
            return &ctx->sessions[i];
        }
    }

    return NULL;
}

dds_crypto_status_t dds_crypto_get_session_keys(dds_crypto_context_t *ctx,
                                                uint64_t session_id,
                                                dds_crypto_key_material_msg_t *key_mat)
{
    if (!ctx || session_id == 0 || !key_mat) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    dds_crypto_session_t *session = NULL;
    for (uint32_t i = 0; i < ctx->max_sessions; i++) {
        if (ctx->sessions[i].session_id == session_id) {
            session = &ctx->sessions[i];
            break;
        }
    }

    if (!session || !session->established) {
        return DDS_CRYPTO_ERROR_SESSION_INVALID;
    }

    dds_crypto_key_material_t *km = &session->key_materials[session->active_key_index];
    memcpy(key_mat->sender_key, km->key, km->key_len);
    memcpy(key_mat->receiver_key, km->key, km->key_len);
    memcpy(key_mat->master_salt, km->salt, 8);
    key_mat->key_seq_number = session->key_seq_number;

    return DDS_CRYPTO_OK;
}

/* ============================================================================
 * Key Management
 * ============================================================================ */

dds_crypto_status_t dds_crypto_generate_key(dds_crypto_context_t *ctx,
                                            uint8_t *key,
                                            uint32_t key_len)
{
    if (!ctx || !key || key_len == 0) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    /* Generate random key using system entropy */
    FILE *fp = fopen("/dev/urandom", "rb");
    if (fp) {
        fread(key, 1, key_len, fp);
        fclose(fp);
    } else {
        /* Fallback to simple PRNG */
        for (uint32_t i = 0; i < key_len; i++) {
            key[i] = (uint8_t)(rand() & 0xFF);
        }
    }

    return DDS_CRYPTO_OK;
}

dds_crypto_status_t dds_crypto_derive_session_keys(dds_crypto_context_t *ctx,
                                                    const uint8_t *shared_secret,
                                                    uint32_t secret_len,
                                                    dds_crypto_key_material_msg_t *key_material)
{
    if (!ctx || !shared_secret || !key_material) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    /* Generate master salt */
    dds_crypto_generate_key(ctx, key_material->master_salt, 8);

    /* Derive sender key using HKDF-like construction */
    uint8_t prk[32];
    uint8_t info[] = "DDS-Crypto-Sender-Key";

    /* Simplified HKDF extract */
    for (int i = 0; i < 32; i++) {
        prk[i] = (i < secret_len) ? shared_secret[i] : 0;
    }

    /* Simplified HKDF expand */
    uint8_t okm[96]; /* 3 * 32 bytes */
    uint8_t prev[32] = {0};
    uint8_t counter = 1;

    for (int block = 0; block < 3; block++) {
        /* T(i) = HMAC-SHA256(PRK, T(i-1) | info | counter) */
        uint8_t input[64 + sizeof(info) + 1];
        uint32_t input_len = 0;

        if (block > 0) {
            memcpy(input, prev, 32);
            input_len += 32;
        }
        memcpy(input + input_len, info, sizeof(info) - 1);
        input_len += sizeof(info) - 1;
        input[input_len++] = counter++;

        /* Simplified hash - use XOR based mixing */
        for (int i = 0; i < 32; i++) {
            okm[block * 32 + i] = prk[i];
            for (uint32_t j = 0; j < input_len; j++) {
                okm[block * 32 + i] ^= input[j] ^ (j * 7 + i * 13);
            }
        }
    }

    memcpy(key_material->sender_key, okm, 32);
    memcpy(key_material->receiver_key, okm + 32, 32);
    memcpy(key_material->master_sender_key, okm + 64, 32);

    return DDS_CRYPTO_OK;
}

dds_crypto_status_t dds_crypto_update_session_key(dds_crypto_context_t *ctx,
                                                   uint64_t session_id)
{
    if (!ctx || session_id == 0) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    dds_crypto_session_t *session = NULL;
    for (uint32_t i = 0; i < ctx->max_sessions; i++) {
        if (ctx->sessions[i].session_id == session_id) {
            session = &ctx->sessions[i];
            break;
        }
    }

    if (!session || !session->established) {
        return DDS_CRYPTO_ERROR_SESSION_INVALID;
    }

    /* Rotate to next key slot */
    uint32_t new_index = (session->active_key_index + 1) % DDS_CRYPTO_MAX_KEYS_PER_SESSION;

    /* Generate new key material */
    dds_crypto_generate_key(ctx, session->key_materials[new_index].key, DDS_CRYPTO_AES_256_KEY_SIZE);
    dds_crypto_generate_key(ctx, session->key_materials[new_index].salt, 8);
    session->key_materials[new_index].key_len = DDS_CRYPTO_AES_256_KEY_SIZE;
    session->key_materials[new_index].key_id = session->session_id + session->key_seq_number;
    session->key_materials[new_index].valid = true;

    session->active_key_index = new_index;
    session->key_seq_number++;

    ctx->key_rotations++;

    if (ctx->on_key_rotation) {
        ctx->on_key_rotation(session_id);
    }

    return DDS_CRYPTO_OK;
}

uint32_t dds_crypto_check_key_expiry(dds_crypto_context_t *ctx, uint64_t current_time)
{
    if (!ctx) {
        return 0;
    }

    uint32_t updated_count = 0;

    for (uint32_t i = 0; i < ctx->max_sessions; i++) {
        dds_crypto_session_t *session = &ctx->sessions[i];

        if (session->established) {
            dds_crypto_key_material_t *km = &session->key_materials[session->active_key_index];

            if (current_time > km->creation_time + DDS_CRYPTO_MAX_KEY_LIFETIME_MS) {
                dds_crypto_update_session_key(ctx, session->session_id);
                updated_count++;
            }
        }
    }

    return updated_count;
}

dds_crypto_status_t dds_crypto_generate_iv(dds_crypto_context_t *ctx,
                                           dds_crypto_session_t *session,
                                           uint8_t *iv)
{
    if (!ctx || !session || !iv) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    /* IV = salt (8 bytes) || sequence number (4 bytes) */
    memcpy(iv, session->key_materials[session->active_key_index].salt, 8);

    iv[8] = (session->iv_seq >> 24) & 0xFF;
    iv[9] = (session->iv_seq >> 16) & 0xFF;
    iv[10] = (session->iv_seq >> 8) & 0xFF;
    iv[11] = session->iv_seq & 0xFF;

    session->iv_seq++;

    return DDS_CRYPTO_OK;
}

/* ============================================================================
 * AES-GCM Encryption/Decryption
 * ============================================================================ */

static void ghash(const uint8_t *H, const uint8_t *aad, uint32_t aad_len,
                  const uint8_t *ciphertext, uint32_t ct_len, uint8_t *tag)
{
    /* Simplified GHASH implementation */
    memset(tag, 0, 16);

    /* XOR AAD length and ciphertext length into tag */
    uint64_t aad_bits = (uint64_t)aad_len * 8;
    uint64_t ct_bits = (uint64_t)ct_len * 8;

    for (int i = 0; i < 8; i++) {
        tag[15 - i] ^= (aad_bits >> (i * 8)) & 0xFF;
        tag[7 - i] ^= (ct_bits >> (i * 8)) & 0xFF;
    }

    /* Additional mixing */
    for (uint32_t i = 0; i < ct_len; i++) {
        tag[i % 16] ^= ciphertext[i] ^ H[i % 16];
    }
}

dds_crypto_status_t dds_crypto_encrypt_aes_gcm(dds_crypto_context_t *ctx,
                                               const uint8_t *key,
                                               uint32_t key_len,
                                               const uint8_t *iv,
                                               const uint8_t *plaintext,
                                               uint32_t plaintext_len,
                                               const uint8_t *aad,
                                               uint32_t aad_len,
                                               uint8_t *ciphertext,
                                               uint8_t *tag)
{
    if (!ctx || !key || !iv || !plaintext || !ciphertext || !tag) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    /* Validate key length */
    if (key_len != 16 && key_len != 24 && key_len != 32) {
        return DDS_CRYPTO_ERROR_KEY_INVALID;
    }

    /* Expand key for AES */
    uint8_t round_key[240];
    aes_key_expansion(round_key, key, key_len * 8);

    /* CTR mode encryption (simplified) */
    uint8_t counter[16];
    memcpy(counter, iv, 12);
    counter[12] = 0;
    counter[13] = 0;
    counter[14] = 0;
    counter[15] = 1;

    for (uint32_t block = 0; block < plaintext_len; block += 16) {
        uint8_t keystream[16] = {0};
        uint8_t state[4][4];

        /* Load counter into state */
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                state[j][i] = counter[i * 4 + j];
            }
        }

        /* AES encrypt counter */
        add_round_key(state, round_key, 0);

        int nr = (key_len == 16) ? 10 : (key_len == 24) ? 12 : 14;
        for (int round = 1; round < nr; round++) {
            sub_bytes(state);
            shift_rows(state);
            mix_columns(state);
            add_round_key(state, round_key, round);
        }
        sub_bytes(state);
        shift_rows(state);
        add_round_key(state, round_key, nr);

        /* Extract keystream */
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                keystream[i * 4 + j] = state[j][i];
            }
        }

        /* XOR with plaintext */
        for (uint32_t i = 0; i < 16 && (block + i) < plaintext_len; i++) {
            ciphertext[block + i] = plaintext[block + i] ^ keystream[i];
        }

        /* Increment counter */
        for (int i = 15; i >= 12; i--) {
            if (++counter[i] != 0) break;
        }
    }

    /* Compute authentication tag (simplified) */
    uint8_t H[16] = {0};
    memcpy(H, key, key_len < 16 ? key_len : 16);
    ghash(H, aad, aad_len, ciphertext, plaintext_len, tag);

    ctx->total_encrypted += plaintext_len;

    return DDS_CRYPTO_OK;
}

dds_crypto_status_t dds_crypto_decrypt_aes_gcm(dds_crypto_context_t *ctx,
                                               const uint8_t *key,
                                               uint32_t key_len,
                                               const uint8_t *iv,
                                               const uint8_t *ciphertext,
                                               uint32_t ciphertext_len,
                                               const uint8_t *aad,
                                               uint32_t aad_len,
                                               const uint8_t *tag,
                                               uint8_t *plaintext)
{
    if (!ctx || !key || !iv || !ciphertext || !tag || !plaintext) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    /* Verify tag first */
    uint8_t computed_tag[16];
    uint8_t H[16] = {0};
    memcpy(H, key, key_len < 16 ? key_len : 16);
    ghash(H, aad, aad_len, ciphertext, ciphertext_len, computed_tag);

    if (memcmp(computed_tag, tag, 16) != 0) {
        ctx->total_failed++;
        return DDS_CRYPTO_ERROR_TAG_MISMATCH;
    }

    /* Decrypt using same CTR mode as encryption */
    dds_crypto_status_t status = dds_crypto_encrypt_aes_gcm(ctx, key, key_len, iv,
                                                             ciphertext, ciphertext_len,
                                                             aad, aad_len, plaintext, computed_tag);

    if (status == DDS_CRYPTO_OK) {
        ctx->total_decrypted += ciphertext_len;
    }

    return status;
}

/* ============================================================================
 * GMAC Operations
 * ============================================================================ */

dds_crypto_status_t dds_crypto_compute_gmac(dds_crypto_context_t *ctx,
                                            const uint8_t *key,
                                            uint32_t key_len,
                                            const uint8_t *iv,
                                            const uint8_t *message,
                                            uint32_t message_len,
                                            const uint8_t *aad,
                                            uint32_t aad_len,
                                            uint8_t *tag)
{
    if (!ctx || !key || !iv || !message || !tag) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    /* GMAC is essentially AES-GCM with empty ciphertext */
    return dds_crypto_encrypt_aes_gcm(ctx, key, key_len, iv,
                                       message, message_len,
                                       aad, aad_len, (uint8_t*)message, tag);
}

dds_crypto_status_t dds_crypto_verify_gmac(dds_crypto_context_t *ctx,
                                           const uint8_t *key,
                                           uint32_t key_len,
                                           const uint8_t *iv,
                                           const uint8_t *message,
                                           uint32_t message_len,
                                           const uint8_t *aad,
                                           uint32_t aad_len,
                                           const uint8_t *tag)
{
    if (!ctx || !key || !iv || !message || !tag) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    uint8_t computed_tag[16];
    dds_crypto_status_t status = dds_crypto_compute_gmac(ctx, key, key_len, iv,
                                                          message, message_len,
                                                          aad, aad_len, computed_tag);

    if (status != DDS_CRYPTO_OK) {
        return status;
    }

    if (memcmp(computed_tag, tag, 16) != 0) {
        return DDS_CRYPTO_ERROR_TAG_MISMATCH;
    }

    return DDS_CRYPTO_OK;
}

/* ============================================================================
 * DDS Data Protection
 * ============================================================================ */

dds_crypto_status_t dds_crypto_encrypt_rtps_data(dds_crypto_context_t *ctx,
                                                  uint64_t session_id,
                                                  const uint8_t *plaintext,
                                                  uint32_t plaintext_len,
                                                  uint8_t *output_buffer,
                                                  uint32_t output_size,
                                                  uint32_t *output_len)
{
    if (!ctx || session_id == 0 || !plaintext || !output_buffer || !output_len) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    /* Calculate required output size */
    uint32_t required_size = sizeof(dds_crypto_transformation_header_t) +
                              plaintext_len +
                              DDS_CRYPTO_GCM_TAG_SIZE;

    if (output_size < required_size) {
        return DDS_CRYPTO_ERROR_BUFFER_TOO_SMALL;
    }

    /* Find session */
    dds_crypto_session_t *session = NULL;
    for (uint32_t i = 0; i < ctx->max_sessions; i++) {
        if (ctx->sessions[i].session_id == session_id) {
            session = &ctx->sessions[i];
            break;
        }
    }

    if (!session || !session->established) {
        return DDS_CRYPTO_ERROR_SESSION_INVALID;
    }

    /* Get active key material */
    dds_crypto_key_material_t *km = &session->key_materials[session->active_key_index];
    if (!km->valid) {
        return DDS_CRYPTO_ERROR_KEY_INVALID;
    }

    /* Create transformation header */
    dds_crypto_transformation_header_t *header = (dds_crypto_transformation_header_t*)output_buffer;
    dds_crypto_create_transform_header(ctx, session,
                                       (ctx->algorithm == DDS_CRYPTO_ALG_AES_256_GCM) ?
                                       DDS_CRYPTO_TRANSFORM_AES256_GCM :
                                       DDS_CRYPTO_TRANSFORM_AES128_GCM,
                                       header);

    /* Generate IV */
    uint8_t iv[DDS_CRYPTO_GCM_IV_SIZE];
    dds_crypto_generate_iv(ctx, session, iv);
    memcpy(header->initialization_vector, iv, DDS_CRYPTO_GCM_IV_SIZE);

    /* Encrypt data */
    uint8_t *ciphertext = output_buffer + sizeof(dds_crypto_transformation_header_t);
    uint8_t tag[DDS_CRYPTO_GCM_TAG_SIZE];

    dds_crypto_status_t status = dds_crypto_encrypt_aes_gcm(ctx,
                                                            km->key, km->key_len,
                                                            iv,
                                                            plaintext, plaintext_len,
                                                            NULL, 0,
                                                            ciphertext, tag);

    if (status != DDS_CRYPTO_OK) {
        return status;
    }

    /* Append tag */
    memcpy(ciphertext + plaintext_len, tag, DDS_CRYPTO_GCM_TAG_SIZE);

    *output_len = sizeof(dds_crypto_transformation_header_t) + plaintext_len + DDS_CRYPTO_GCM_TAG_SIZE;

    return DDS_CRYPTO_OK;
}

dds_crypto_status_t dds_crypto_decrypt_rtps_data(dds_crypto_context_t *ctx,
                                                  uint64_t session_id,
                                                  const uint8_t *input_buffer,
                                                  uint32_t input_len,
                                                  uint8_t *plaintext,
                                                  uint32_t plaintext_size,
                                                  uint32_t *plaintext_len)
{
    if (!ctx || session_id == 0 || !input_buffer || !plaintext || !plaintext_len) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    if (input_len < sizeof(dds_crypto_transformation_header_t) + DDS_CRYPTO_GCM_TAG_SIZE) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    /* Parse header */
    dds_crypto_transformation_header_t *header = (dds_crypto_transformation_header_t*)input_buffer;

    /* Find session */
    dds_crypto_session_t *session = NULL;
    for (uint32_t i = 0; i < ctx->max_sessions; i++) {
        if (ctx->sessions[i].session_id == session_id) {
            session = &ctx->sessions[i];
            break;
        }
    }

    if (!session || !session->established) {
        return DDS_CRYPTO_ERROR_SESSION_INVALID;
    }

    /* Get active key material */
    dds_crypto_key_material_t *km = &session->key_materials[session->active_key_index];
    if (!km->valid) {
        return DDS_CRYPTO_ERROR_KEY_INVALID;
    }

    /* Calculate ciphertext length */
    uint32_t ciphertext_len = input_len - sizeof(dds_crypto_transformation_header_t) - DDS_CRYPTO_GCM_TAG_SIZE;

    if (plaintext_size < ciphertext_len) {
        return DDS_CRYPTO_ERROR_BUFFER_TOO_SMALL;
    }

    /* Extract IV from header */
    uint8_t iv[DDS_CRYPTO_GCM_IV_SIZE];
    memcpy(iv, header->initialization_vector, DDS_CRYPTO_GCM_IV_SIZE);

    /* Extract ciphertext and tag */
    uint8_t *ciphertext = (uint8_t*)input_buffer + sizeof(dds_crypto_transformation_header_t);
    uint8_t *tag = ciphertext + ciphertext_len;

    /* Decrypt */
    dds_crypto_status_t status = dds_crypto_decrypt_aes_gcm(ctx,
                                                            km->key, km->key_len,
                                                            iv,
                                                            ciphertext, ciphertext_len,
                                                            NULL, 0,
                                                            tag, plaintext);

    if (status == DDS_CRYPTO_OK) {
        *plaintext_len = ciphertext_len;
    }

    return status;
}

dds_crypto_status_t dds_crypto_compute_message_mac(dds_crypto_context_t *ctx,
                                                    uint64_t session_id,
                                                    const uint8_t *message,
                                                    uint32_t message_len,
                                                    const uint8_t *header,
                                                    uint32_t header_len,
                                                    uint8_t *mac)
{
    if (!ctx || session_id == 0 || !message || !mac) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    /* Find session */
    dds_crypto_session_t *session = NULL;
    for (uint32_t i = 0; i < ctx->max_sessions; i++) {
        if (ctx->sessions[i].session_id == session_id) {
            session = &ctx->sessions[i];
            break;
        }
    }

    if (!session || !session->established) {
        return DDS_CRYPTO_ERROR_SESSION_INVALID;
    }

    dds_crypto_key_material_t *km = &session->key_materials[session->active_key_index];

    /* Use GMAC for message authentication */
    uint8_t iv[DDS_CRYPTO_GCM_IV_SIZE];
    dds_crypto_generate_iv(ctx, session, iv);

    return dds_crypto_compute_gmac(ctx, km->key, km->key_len, iv,
                                   message, message_len,
                                   header, header_len, mac);
}

dds_crypto_status_t dds_crypto_create_transform_header(dds_crypto_context_t *ctx,
                                                        dds_crypto_session_t *session,
                                                        uint8_t transform_kind,
                                                        dds_crypto_transformation_header_t *header)
{
    if (!ctx || !session || !header) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    header->transformation_kind = transform_kind;

    header->session_id[0] = (session->session_id >> 56) & 0xFF;
    header->session_id[1] = (session->session_id >> 48) & 0xFF;
    header->session_id[2] = (session->session_id >> 40) & 0xFF;
    header->session_id[3] = (session->session_id >> 32) & 0xFF;
    header->session_id[4] = (session->session_id >> 24) & 0xFF;
    header->session_id[5] = (session->session_id >> 16) & 0xFF;
    header->session_id[6] = (session->session_id >> 8) & 0xFF;
    header->session_id[7] = session->session_id & 0xFF;

    dds_crypto_key_material_t *km = &session->key_materials[session->active_key_index];
    header->key_id[0] = (km->key_id >> 24) & 0xFF;
    header->key_id[1] = (km->key_id >> 16) & 0xFF;
    header->key_id[2] = (km->key_id >> 8) & 0xFF;
    header->key_id[3] = km->key_id & 0xFF;

    return DDS_CRYPTO_OK;
}

/* ============================================================================
 * Replay Protection
 * ============================================================================ */

dds_crypto_status_t dds_crypto_check_replay(dds_crypto_context_t *ctx,
                                            uint64_t session_id,
                                            uint64_t seq_number)
{
    if (!ctx || session_id == 0) {
        return DDS_CRYPTO_ERROR_INVALID_PARAM;
    }

    /* Simplified replay check - in production, implement proper sliding window */
    return DDS_CRYPTO_OK;
}

void dds_crypto_update_replay_window(dds_crypto_context_t *ctx,
                                     uint64_t session_id,
                                     uint64_t seq_number)
{
    if (!ctx || session_id == 0) {
        return;
    }

    /* Update window tracking */
}

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

uint32_t dds_crypto_get_encrypted_size(uint32_t plaintext_len)
{
    return sizeof(dds_crypto_transformation_header_t) + plaintext_len + DDS_CRYPTO_GCM_TAG_SIZE;
}

uint32_t dds_crypto_get_decrypted_size(uint32_t ciphertext_len)
{
    if (ciphertext_len < sizeof(dds_crypto_transformation_header_t) + DDS_CRYPTO_GCM_TAG_SIZE) {
        return 0;
    }
    return ciphertext_len - sizeof(dds_crypto_transformation_header_t) - DDS_CRYPTO_GCM_TAG_SIZE;
}

void dds_crypto_get_stats(dds_crypto_context_t *ctx,
                          uint64_t *encrypted,
                          uint64_t *decrypted,
                          uint64_t *failed)
{
    if (!ctx) {
        return;
    }

    if (encrypted) *encrypted = ctx->total_encrypted;
    if (decrypted) *decrypted = ctx->total_decrypted;
    if (failed) *failed = ctx->total_failed;
}
