/**
 * @file secoc_core.c
 * @brief SecOC (Secure Onboard Communication) Core Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * 实现AUTOSAR SecOC 4.4规范的核心功能
 */

#include "secoc_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * AES-CMAC常量和辅助函数
 * ============================================================================ */

#define AES_BLOCK_SIZE 16

/* AES S-box */
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

/* 基本AES加密单块 */
static void aes_encrypt_block(const uint8_t *in, uint8_t *out, const uint8_t *key, int key_bits) {
    /* 简化的AES加密实现 - 生产环境应使用硬件加密或优化库 */
    /* 此处仅作为演示，实际项目中应使用OpenSSL或硬件加密 */
    (void)key_bits;
    
    /* 简化处理: XOR with key (insecure, for demo only) */
    for (int i = 0; i < AES_BLOCK_SIZE; i++) {
        out[i] = in[i] ^ key[i % 16];
        out[i] = sbox[out[i]];
    }
}

/* 左移一位 */
static void left_shift_one(const uint8_t *in, uint8_t *out) {
    uint8_t overflow = 0;
    for (int i = AES_BLOCK_SIZE - 1; i >= 0; i--) {
        out[i] = (in[i] << 1) | overflow;
        overflow = (in[i] & 0x80) ? 1 : 0;
    }
}

/* 生成AES-CMAC子密钥 */
static void generate_subkeys(const uint8_t *key, int key_bits, uint8_t *k1, uint8_t *k2) {
    uint8_t l[AES_BLOCK_SIZE] = {0};
    uint8_t tmp[AES_BLOCK_SIZE] = {0};
    const uint8_t rb[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87};
    
    /* L = AES-CMAC(K, 0^128) */
    aes_encrypt_block(tmp, l, key, key_bits);
    
    /* K1 = L << 1 */
    left_shift_one(l, k1);
    if (l[0] & 0x80) {
        for (int i = 0; i < AES_BLOCK_SIZE; i++) {
            k1[i] ^= rb[i];
        }
    }
    
    /* K2 = K1 << 1 */
    left_shift_one(k1, k2);
    if (k1[0] & 0x80) {
        for (int i = 0; i < AES_BLOCK_SIZE; i++) {
            k2[i] ^= rb[i];
        }
    }
}

/* ============================================================================
 * HMAC-SHA256实现
 * ============================================================================ */

#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

/* SHA-256初始化向量 */
static const uint32_t sha256_h[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

/* SHA-256常量 */
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

static uint32_t rotr32(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

static uint32_t sha256_ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

static uint32_t sha256_maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t sha256_ep0(uint32_t x) {
    return rotr32(x, 2) ^ rotr32(x, 13) ^ rotr32(x, 22);
}

static uint32_t sha256_ep1(uint32_t x) {
    return rotr32(x, 6) ^ rotr32(x, 11) ^ rotr32(x, 25);
}

static uint32_t sha256_sig0(uint32_t x) {
    return rotr32(x, 7) ^ rotr32(x, 18) ^ (x >> 3);
}

static uint32_t sha256_sig1(uint32_t x) {
    return rotr32(x, 17) ^ rotr32(x, 19) ^ (x >> 10);
}

static void sha256_transform(uint32_t state[8], const uint8_t data[64]) {
    uint32_t w[64];
    uint32_t t1, t2;
    uint32_t a, b, c, d, e, f, g, h;
    
    /* 准备消息日志 */
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)data[i * 4] << 24) | ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) | ((uint32_t)data[i * 4 + 3]);
    }
    for (int i = 16; i < 64; i++) {
        w[i] = sha256_sig1(w[i - 2]) + w[i - 7] + sha256_sig0(w[i - 15]) + w[i - 16];
    }
    
    /* 初始化工作变量 */
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    
    /* 主循环 */
    for (int i = 0; i < 64; i++) {
        t1 = h + sha256_ep1(e) + sha256_ch(e, f, g) + sha256_k[i] + w[i];
        t2 = sha256_ep0(a) + sha256_maj(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    /* 更新状态 */
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

static void sha256_init(uint32_t state[8]) {
    memcpy(state, sha256_h, sizeof(sha256_h));
}

static void sha256_update(uint32_t state[8], const uint8_t *data, uint32_t len, uint8_t *buffer, uint32_t *buf_len, uint64_t *bit_len) {
    for (uint32_t i = 0; i < len; i++) {
        buffer[(*buf_len)++] = data[i];
        if (*buf_len == 64) {
            sha256_transform(state, buffer);
            *buf_len = 0;
            *bit_len += 512;
        }
    }
}

static void sha256_final(uint32_t state[8], uint8_t *buffer, uint32_t *buf_len, uint64_t *bit_len, uint8_t hash[32]) {
    uint32_t i = *buf_len;
    
    /* 填充 */
    buffer[i++] = 0x80;
    if (i > 56) {
        while (i < 64) buffer[i++] = 0;
        sha256_transform(state, buffer);
        i = 0;
    }
    while (i < 56) buffer[i++] = 0;
    
    /* 添加长度 */
    *bit_len += *buf_len * 8;
    buffer[63] = (uint8_t)(*bit_len);
    buffer[62] = (uint8_t)(*bit_len >> 8);
    buffer[61] = (uint8_t)(*bit_len >> 16);
    buffer[60] = (uint8_t)(*bit_len >> 24);
    buffer[59] = (uint8_t)(*bit_len >> 32);
    buffer[58] = (uint8_t)(*bit_len >> 40);
    buffer[57] = (uint8_t)(*bit_len >> 48);
    buffer[56] = (uint8_t)(*bit_len >> 56);
    sha256_transform(state, buffer);
    
    /* 输出 */
    for (i = 0; i < 8; i++) {
        hash[i * 4] = (uint8_t)(state[i] >> 24);
        hash[i * 4 + 1] = (uint8_t)(state[i] >> 16);
        hash[i * 4 + 2] = (uint8_t)(state[i] >> 8);
        hash[i * 4 + 3] = (uint8_t)(state[i]);
    }
}

static void hmac_sha256(const uint8_t *key, uint32_t key_len,
                        const uint8_t *msg, uint32_t msg_len,
                        uint8_t *mac, uint32_t mac_len) {
    uint8_t k_ipad[SHA256_BLOCK_SIZE] = {0};
    uint8_t k_opad[SHA256_BLOCK_SIZE] = {0};
    uint8_t tk[SHA256_DIGEST_SIZE];
    uint32_t state[8];
    uint8_t buffer[64];
    uint32_t buf_len = 0;
    uint64_t bit_len = 0;
    
    /* 如果密钥太长，先计算hash */
    if (key_len > SHA256_BLOCK_SIZE) {
        sha256_init(state);
        sha256_update(state, key, key_len, buffer, &buf_len, &bit_len);
        sha256_final(state, buffer, &buf_len, &bit_len, tk);
        key = tk;
        key_len = SHA256_DIGEST_SIZE;
    }
    
    /* 创建ipad和opad */
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);
    for (int i = 0; i < SHA256_BLOCK_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }
    
    /* 内层hash */
    sha256_init(state);
    buf_len = 0; bit_len = 0;
    sha256_update(state, k_ipad, SHA256_BLOCK_SIZE, buffer, &buf_len, &bit_len);
    sha256_update(state, msg, msg_len, buffer, &buf_len, &bit_len);
    sha256_final(state, buffer, &buf_len, &bit_len, tk);
    
    /* 外层hash */
    sha256_init(state);
    buf_len = 0; bit_len = 0;
    sha256_update(state, k_opad, SHA256_BLOCK_SIZE, buffer, &buf_len, &bit_len);
    sha256_update(state, tk, SHA256_DIGEST_SIZE, buffer, &buf_len, &bit_len);
    sha256_final(state, buffer, &buf_len, &bit_len, tk);
    
    /* 输出 */
    memcpy(mac, tk, (mac_len < SHA256_DIGEST_SIZE) ? mac_len : SHA256_DIGEST_SIZE);
}

/* ============================================================================
 * AES-CMAC实现
 * ============================================================================ */

static void aes_cmac(const uint8_t *key, uint32_t key_len,
                     const uint8_t *msg, uint32_t msg_len,
                     uint8_t *mac, uint32_t mac_len) {
    uint8_t k1[AES_BLOCK_SIZE] = {0};
    uint8_t k2[AES_BLOCK_SIZE] = {0};
    uint8_t x[AES_BLOCK_SIZE] = {0};
    uint8_t y[AES_BLOCK_SIZE] = {0};
    uint8_t m_last[AES_BLOCK_SIZE] = {0};
    int n = (msg_len + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE;
    bool flag = (msg_len == 0) || (msg_len % AES_BLOCK_SIZE != 0);
    
    generate_subkeys(key, key_len * 8, k1, k2);
    
    if (n == 0) {
        n = 1;
        flag = true;
    }
    
    if (!flag) {
        /* 完整块 */
        for (int j = 0; j < AES_BLOCK_SIZE; j++) {
            m_last[j] = msg[(n - 1) * AES_BLOCK_SIZE + j] ^ k1[j];
        }
    } else {
        /* 不完整块 */
        for (int j = 0; j < AES_BLOCK_SIZE; j++) {
            if (j < (int)(msg_len % AES_BLOCK_SIZE)) {
                m_last[j] = msg[(n - 1) * AES_BLOCK_SIZE + j];
            } else if (j == (int)(msg_len % AES_BLOCK_SIZE)) {
                m_last[j] = 0x80;
            } else {
                m_last[j] = 0;
            }
            m_last[j] ^= k2[j];
        }
    }
    
    /* 计算CMAC */
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < AES_BLOCK_SIZE; j++) {
            y[j] = x[j] ^ msg[i * AES_BLOCK_SIZE + j];
        }
        aes_encrypt_block(y, x, key, key_len * 8);
    }
    
    for (int j = 0; j < AES_BLOCK_SIZE; j++) {
        y[j] = x[j] ^ m_last[j];
    }
    aes_encrypt_block(y, x, key, key_len * 8);
    
    memcpy(mac, x, (mac_len < AES_BLOCK_SIZE) ? mac_len : AES_BLOCK_SIZE);
}

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

secoc_context_t* secoc_init(dds_crypto_context_t *crypto_ctx) {
    secoc_context_t *ctx = (secoc_context_t*)calloc(1, sizeof(secoc_context_t));
    if (!ctx) {
        return NULL;
    }
    
    ctx->crypto_ctx = crypto_ctx;
    ctx->initialized = true;
    
    /* 初始化密钥槽 */
    for (int i = 0; i < SECOC_MAX_KEY_SLOTS; i++) {
        ctx->key_slots[i].slot_id = i;
        ctx->key_slots[i].is_valid = false;
    }
    
    return ctx;
}

void secoc_deinit(secoc_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    /* 清除所有密钥 */
    for (int i = 0; i < SECOC_MAX_KEY_SLOTS; i++) {
        memset(ctx->key_slots[i].key, 0, sizeof(ctx->key_slots[i].key));
        ctx->key_slots[i].is_valid = false;
    }
    
    ctx->initialized = false;
    free(ctx);
}

/* ============================================================================
 * PDU配置管理
 * ============================================================================ */

secoc_status_t secoc_add_pdu_config(secoc_context_t *ctx, const secoc_pdu_config_t *config) {
    if (!ctx || !config) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->initialized) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (ctx->num_pdu_configs >= SECOC_MAX_PDU_CONFIGS) {
        return SECOC_ERROR_NO_MEMORY;
    }
    
    /* 检查是否已存在 */
    for (uint32_t i = 0; i < ctx->num_pdu_configs; i++) {
        if (ctx->pdu_configs[i].pdu_id == config->pdu_id) {
            return SECOC_ERROR_INVALID_PARAM;
        }
    }
    
    /* 添加配置 */
    memcpy(&ctx->pdu_configs[ctx->num_pdu_configs], config, sizeof(secoc_pdu_config_t));
    ctx->num_pdu_configs++;
    
    return SECOC_OK;
}

const secoc_pdu_config_t* secoc_get_pdu_config(secoc_context_t *ctx, uint32_t pdu_id) {
    if (!ctx || !ctx->initialized) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < ctx->num_pdu_configs; i++) {
        if (ctx->pdu_configs[i].pdu_id == pdu_id) {
            return &ctx->pdu_configs[i];
        }
    }
    
    return NULL;
}

secoc_status_t secoc_remove_pdu_config(secoc_context_t *ctx, uint32_t pdu_id) {
    if (!ctx || !ctx->initialized) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    for (uint32_t i = 0; i < ctx->num_pdu_configs; i++) {
        if (ctx->pdu_configs[i].pdu_id == pdu_id) {
            /* 移除配置 */
            memmove(&ctx->pdu_configs[i], &ctx->pdu_configs[i + 1],
                    (ctx->num_pdu_configs - i - 1) * sizeof(secoc_pdu_config_t));
            ctx->num_pdu_configs--;
            return SECOC_OK;
        }
    }
    
    return SECOC_ERROR_INVALID_PARAM;
}

/* ============================================================================
 * 密钥管理
 * ============================================================================ */

secoc_status_t secoc_import_key(secoc_context_t *ctx, uint8_t slot_id,
                                const uint8_t *key, uint8_t key_len) {
    if (!ctx || !ctx->initialized || !key) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (slot_id >= SECOC_MAX_KEY_SLOTS) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (key_len > 32) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_key_slot_t *slot = &ctx->key_slots[slot_id];
    memcpy(slot->key, key, key_len);
    slot->key_len = key_len;
    slot->is_valid = true;
    slot->key_version++;
    
    return SECOC_OK;
}

secoc_status_t secoc_get_key(secoc_context_t *ctx, uint8_t slot_id,
                             uint8_t *key, uint8_t *key_len) {
    if (!ctx || !ctx->initialized || !key || !key_len) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (slot_id >= SECOC_MAX_KEY_SLOTS) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_key_slot_t *slot = &ctx->key_slots[slot_id];
    if (!slot->is_valid) {
        return SECOC_ERROR_KEY_NOT_FOUND;
    }
    
    memcpy(key, slot->key, slot->key_len);
    *key_len = slot->key_len;
    
    return SECOC_OK;
}

secoc_status_t secoc_clear_key(secoc_context_t *ctx, uint8_t slot_id) {
    if (!ctx || !ctx->initialized) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (slot_id >= SECOC_MAX_KEY_SLOTS) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_key_slot_t *slot = &ctx->key_slots[slot_id];
    memset(slot->key, 0, sizeof(slot->key));
    slot->key_len = 0;
    slot->is_valid = false;
    
    return SECOC_OK;
}

/* ============================================================================
 * MAC计算
 * ============================================================================ */

secoc_status_t secoc_compute_mac(secoc_context_t *ctx, const secoc_pdu_config_t *config,
                                 const secoc_authenticated_pdu_t *auth_pdu,
                                 uint8_t *computed_mac) {
    if (!ctx || !config || !auth_pdu || !computed_mac) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 获取密钥 */
    secoc_key_slot_t *key_slot = &ctx->key_slots[config->auth_key_slot];
    if (!key_slot->is_valid) {
        return SECOC_ERROR_KEY_NOT_FOUND;
    }
    
    /* 构建认证数据: [Data | Freshness Value] */
    uint8_t auth_data[SECOC_MAX_PDU_LENGTH + SECOC_MAX_FRESHNESS_LENGTH];
    uint32_t auth_len = 0;
    
    /* 添加数据 */
    if (auth_pdu->data && auth_pdu->data_len > 0) {
        memcpy(auth_data, auth_pdu->data, auth_pdu->data_len);
        auth_len += auth_pdu->data_len;
    }
    
    /* 添加Freshness值 */
    uint8_t fv_len = config->freshness_value_len / 8;
    if (fv_len > 0 && fv_len <= SECOC_MAX_FRESHNESS_LENGTH) {
        memcpy(&auth_data[auth_len], auth_pdu->freshness_value, fv_len);
        auth_len += fv_len;
    }
    
    /* 计算MAC */
    uint32_t mac_len_bytes = config->mac_len / 8;
    if (mac_len_bytes == 0) mac_len_bytes = AES_BLOCK_SIZE;
    
    switch (config->auth_algo) {
        case SECOC_ALGO_AES_CMAC_128:
        case SECOC_ALGO_AES_CMAC_256:
            aes_cmac(key_slot->key, key_slot->key_len, auth_data, auth_len, computed_mac, mac_len_bytes);
            break;
            
        case SECOC_ALGO_HMAC_SHA256:
            hmac_sha256(key_slot->key, key_slot->key_len, auth_data, auth_len, computed_mac, mac_len_bytes);
            break;
            
        case SECOC_ALGO_AES_GMAC:
            /* 使用DDS Crypto的GMAC功能 */
            if (ctx->crypto_ctx) {
                /* 复用dds_crypto_compute_gmac - 需要IV */
                uint8_t iv[DDS_CRYPTO_GCM_IV_SIZE] = {0};
                dds_crypto_compute_gmac(ctx->crypto_ctx, key_slot->key, key_slot->key_len,
                                       iv, auth_data, auth_len, NULL, 0, computed_mac);
            } else {
                /* 降级到AES-CMAC */
                aes_cmac(key_slot->key, key_slot->key_len, auth_data, auth_len, computed_mac, mac_len_bytes);
            }
            break;
            
        default:
            return SECOC_ERROR_INVALID_PARAM;
    }
    
    return SECOC_OK;
}

/* ============================================================================
 * 发送方功能
 * ============================================================================ */

secoc_status_t secoc_authenticate_tx_pdu(secoc_context_t *ctx, uint32_t pdu_id,
                                         const uint8_t *data, uint32_t data_len,
                                         secoc_authenticated_pdu_t *auth_pdu) {
    if (!ctx || !ctx->initialized || !data || !auth_pdu) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 获取PDU配置 */
    const secoc_pdu_config_t *config = secoc_get_pdu_config(ctx, pdu_id);
    if (!config) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 初始化认证PDU结构 */
    memset(auth_pdu, 0, sizeof(secoc_authenticated_pdu_t));
    auth_pdu->pdu_id = pdu_id;
    
    /* 复制数据 */
    if (data_len > SECOC_MAX_PDU_LENGTH) {
        return SECOC_ERROR_PDU_TOO_LARGE;
    }
    auth_pdu->data = (uint8_t*)malloc(data_len);
    if (!auth_pdu->data) {
        return SECOC_ERROR_NO_MEMORY;
    }
    memcpy(auth_pdu->data, data, data_len);
    auth_pdu->data_len = data_len;
    
    /* 设置freshness值长度 */
    auth_pdu->freshness_len = config->freshness_value_len / 8;
    auth_pdu->auth_len = config->mac_len / 8;
    if (auth_pdu->auth_len == 0) auth_pdu->auth_len = AES_BLOCK_SIZE;
    
    /* 获取Freshness值 - 这里使用简单的计数器模式 */
    /* 在实际实现中，这里应调用secoc_freshness_get_tx_value() */
    static uint64_t tx_counter = 1;
    uint64_t freshness = tx_counter++;
    for (int i = 0; i < (int)auth_pdu->freshness_len && i < 8; i++) {
        auth_pdu->freshness_value[i] = (freshness >> (i * 8)) & 0xFF;
    }
    
    /* 计算MAC */
    secoc_status_t status = secoc_compute_mac(ctx, config, auth_pdu, auth_pdu->authenticator);
    if (status != SECOC_OK) {
        free(auth_pdu->data);
        auth_pdu->data = NULL;
        return status;
    }
    
    /* 更新统计 */
    ctx->tx_pdu_count++;
    
    return SECOC_OK;
}

secoc_status_t secoc_build_secured_pdu(const secoc_authenticated_pdu_t *auth_pdu,
                                       uint8_t *secured_pdu, uint32_t *secured_len,
                                       uint32_t max_len) {
    if (!auth_pdu || !secured_pdu || !secured_len) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    uint32_t total_len = auth_pdu->data_len + auth_pdu->freshness_len + auth_pdu->auth_len;
    if (total_len > max_len) {
        return SECOC_ERROR_PDU_TOO_LARGE;
    }
    
    uint32_t offset = 0;
    
    /* 序列化数据 */
    if (auth_pdu->data && auth_pdu->data_len > 0) {
        memcpy(&secured_pdu[offset], auth_pdu->data, auth_pdu->data_len);
        offset += auth_pdu->data_len;
    }
    
    /* 序列化Freshness值 */
    if (auth_pdu->freshness_len > 0) {
        memcpy(&secured_pdu[offset], auth_pdu->freshness_value, auth_pdu->freshness_len);
        offset += auth_pdu->freshness_len;
    }
    
    /* 序列化Authenticator */
    if (auth_pdu->auth_len > 0) {
        memcpy(&secured_pdu[offset], auth_pdu->authenticator, auth_pdu->auth_len);
        offset += auth_pdu->auth_len;
    }
    
    *secured_len = offset;
    
    return SECOC_OK;
}

/* ============================================================================
 * 接收方功能
 * ============================================================================ */

secoc_status_t secoc_parse_secured_pdu(const uint8_t *secured_pdu, uint32_t secured_len,
                                       const secoc_pdu_config_t *pdu_config,
                                       secoc_authenticated_pdu_t *auth_pdu) {
    if (!secured_pdu || !pdu_config || !auth_pdu) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    uint8_t fv_len = pdu_config->freshness_value_len / 8;
    uint8_t auth_len = pdu_config->mac_len / 8;
    if (auth_len == 0) auth_len = AES_BLOCK_SIZE;
    
    /* 计算数据长度 */
    if (secured_len < (uint32_t)(fv_len + auth_len)) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    uint32_t data_len = secured_len - fv_len - auth_len;
    
    /* 初始化认证PDU结构 */
    memset(auth_pdu, 0, sizeof(secoc_authenticated_pdu_t));
    auth_pdu->pdu_id = pdu_config->pdu_id;
    auth_pdu->freshness_len = fv_len;
    auth_pdu->auth_len = auth_len;
    
    /* 解析数据 */
    if (data_len > 0) {
        auth_pdu->data = (uint8_t*)malloc(data_len);
        if (!auth_pdu->data) {
            return SECOC_ERROR_NO_MEMORY;
        }
        memcpy(auth_pdu->data, secured_pdu, data_len);
        auth_pdu->data_len = data_len;
    }
    
    /* 解析Freshness值 */
    if (fv_len > 0) {
        memcpy(auth_pdu->freshness_value, &secured_pdu[data_len], fv_len);
    }
    
    /* 解析Authenticator */
    if (auth_len > 0) {
        memcpy(auth_pdu->authenticator, &secured_pdu[data_len + fv_len], auth_len);
    }
    
    return SECOC_OK;
}

secoc_status_t secoc_verify_rx_pdu(secoc_context_t *ctx, uint32_t pdu_id,
                                   const uint8_t *secured_pdu, uint32_t secured_len,
                                   uint8_t *original_data, uint32_t *original_len,
                                   secoc_verify_result_t *result) {
    if (!ctx || !ctx->initialized || !secured_pdu || !original_data || !original_len || !result) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 获取PDU配置 */
    const secoc_pdu_config_t *config = secoc_get_pdu_config(ctx, pdu_id);
    if (!config) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 解析Secured PDU */
    secoc_authenticated_pdu_t auth_pdu;
    secoc_status_t status = secoc_parse_secured_pdu(secured_pdu, secured_len, config, &auth_pdu);
    if (status != SECOC_OK) {
        *result = SECOC_VERIFY_FAILURE;
        ctx->verify_failure_count++;
        return status;
    }
    
    /* 检查Freshness值 (简化版本) */
    /* 在实际实现中，这里应调用secoc_freshness_verify() */
    
    /* 计算预期的MAC */
    uint8_t computed_mac[SECOC_MAX_MAC_LENGTH];
    status = secoc_compute_mac(ctx, config, &auth_pdu, computed_mac);
    if (status != SECOC_OK) {
        if (auth_pdu.data) free(auth_pdu.data);
        *result = SECOC_VERIFY_KEY_INVALID;
        ctx->verify_failure_count++;
        return status;
    }
    
    /* 验证MAC */
    uint8_t auth_len = config->mac_len / 8;
    if (auth_len == 0) auth_len = AES_BLOCK_SIZE;
    
    if (memcmp(auth_pdu.authenticator, computed_mac, auth_len) != 0) {
        if (auth_pdu.data) free(auth_pdu.data);
        *result = SECOC_VERIFY_MAC_FAILED;
        ctx->verify_failure_count++;
        if (ctx->on_verify_failure) {
            ctx->on_verify_failure(pdu_id, *result);
        }
        return SECOC_OK;
    }
    
    /* 验证成功 */
    *result = SECOC_VERIFY_SUCCESS;
    ctx->verify_success_count++;
    
    /* 输出原始数据 */
    if (auth_pdu.data_len > 0 && auth_pdu.data) {
        memcpy(original_data, auth_pdu.data, auth_pdu.data_len);
        *original_len = auth_pdu.data_len;
        free(auth_pdu.data);
    } else {
        *original_len = 0;
    }
    
    /* 更新统计 */
    ctx->rx_pdu_count++;
    
    return SECOC_OK;
}

/* ============================================================================
 * 统计信息
 * ============================================================================ */

void secoc_get_stats(secoc_context_t *ctx, uint64_t *tx_count, uint64_t *rx_count,
                     uint64_t *verify_success, uint64_t *verify_failure) {
    if (!ctx) {
        return;
    }
    
    if (tx_count) *tx_count = ctx->tx_pdu_count;
    if (rx_count) *rx_count = ctx->rx_pdu_count;
    if (verify_success) *verify_success = ctx->verify_success_count;
    if (verify_failure) *verify_failure = ctx->verify_failure_count;
}

void secoc_reset_stats(secoc_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    ctx->tx_pdu_count = 0;
    ctx->rx_pdu_count = 0;
    ctx->verify_success_count = 0;
    ctx->verify_failure_count = 0;
    ctx->replay_detected_count = 0;
}
