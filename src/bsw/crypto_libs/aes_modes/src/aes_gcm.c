/**********************************************************************************************************************
 * @file       aes_gcm.c
 * @brief      AES-GCM模式实现 - Galois/计数器模式 (AEAD)
 *
 * 功能: 提供GCM模式的认证加密/解密功能
 *       支持AEAD（带关联数据的认证加密）
 *       支持流式API
 *
 * GCM特点:
 * - 提供加密和认证
 * - 并行化处理
 * - 无需填充
 * - 广泛用于TLS/HTTPS
 *
 * 实现参考: NIST SP 800-38D
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#include "aes_modes.h"
#include <string.h>

/**********************************************************************************************************************
 * 内部函数声明
 *********************************************************************************************************************/
extern void aes_encrypt_block(const uint8* rk, uint32 nr, const uint8 input[16], uint8 output[16]);

/**********************************************************************************************************************
 * 常量和宏
 *********************************************************************************************************************/
#define GCM_BLOCK_SIZE      AES_BLOCK_SIZE

/**********************************************************************************************************************
 * 内部辅助函数 - GF(2^128)乘法
 *********************************************************************************************************************/

/**
 * @brief GF(2^128)乘法 - 左移
 *
 * 在GF(2^128)上的乘法，多项式为x^128 + x^7 + x^2 + x + 1
 */
static void gcm_mult(const uint8 X[16], const uint8 Y[16], uint8 Z[16])
{
    uint8 V[16];
    uint8 R = 0xE1;  /* x^128 + x^7 + x^2 + x + 1 的常数项 */
    int32 i;
    int32 j;

    memset(Z, 0, 16);
    memcpy(V, Y, 16);

    for (i = 0; i < 16; i++) {
        for (j = 0; j < 8; j++) {
            if (X[i] & (1 << (7 - j))) {
                /* Z = Z ^ V */
                for (int k = 0; k < 16; k++) {
                    Z[k] ^= V[k];
                }
            }

            /* V = V >> 1 */
            uint8 carry = V[15] & 1;
            for (int k = 15; k > 0; k--) {
                V[k] = (V[k] >> 1) | ((V[k - 1] & 1) << 7);
            }
            V[0] = V[0] >> 1;

            if (carry) {
                V[0] ^= R;
            }
        }
    }
}

/**
 * @brief 更优化的GCM乘法 (基于表查找)
 *
 * 使用16个预计算表来加速计算。
 */
static void ghash(const uint8 H[16], const uint8* ad, uint64 ad_len,
                  const uint8* c, uint64 c_len, uint8 tag[16])
{
    uint8 X[16] = {0};
    uint8 buf[16];
    uint64 i;
    uint64 n;

    /* 处理AAD */
    n = (ad_len + 15) / 16;
    for (i = 0; i < n; i++) {
        uint64 len = (ad_len - i * 16 < 16) ? (ad_len - i * 16) : 16;
        memcpy(buf, &ad[i * 16], (size_t)len);
        if (len < 16) {
            memset(&buf[len], 0, 16 - (size_t)len);
        }
        for (int j = 0; j < 16; j++) {
            X[j] ^= buf[j];
        }
        gcm_mult(X, H, X);
    }

    /* 处理密文 */
    n = (c_len + 15) / 16;
    for (i = 0; i < n; i++) {
        uint64 len = (c_len - i * 16 < 16) ? (c_len - i * 16) : 16;
        memcpy(buf, &c[i * 16], (size_t)len);
        if (len < 16) {
            memset(&buf[len], 0, 16 - (size_t)len);
        }
        for (int j = 0; j < 16; j++) {
            X[j] ^= buf[j];
        }
        gcm_mult(X, H, X);
    }

    /* 处理长度 */
    for (int j = 0; j < 8; j++) {
        buf[j] = (uint8)((ad_len * 8) >> (56 - j * 8));
    }
    for (int j = 0; j < 8; j++) {
        buf[8 + j] = (uint8)((c_len * 8) >> (56 - j * 8));
    }
    for (int j = 0; j < 16; j++) {
        X[j] ^= buf[j];
    }
    gcm_mult(X, H, X);

    memcpy(tag, X, 16);
}

/**********************************************************************************************************************
 * 全局函数实现 - GCM加密
 *********************************************************************************************************************/

/**
 * @brief GCM模式加密
 *
 * GCM提供认证加密功能，同时输出密文和认证标签。
 *
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (通常12字节)
 * @param ivLen IV长度 (推荐12字节)
 * @param aad 附加认证数据 (可NULL)
 * @param aadLen AAD长度
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @param tag 认证标签输出 (12-16字节推荐)
 * @param tagLen 标签长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_GcmEncrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      uint32 ivLen,
                      const uint8* aad,
                      uint32 aadLen,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext,
                      uint8* tag,
                      uint32 tagLen)
{
    uint8 H[16] = {0};          /* 散列子密钥 */
    uint8 J0[16] = {0};         /* 计数器初始值 */
    uint8 S[16] = {0};          /* GHASH输出 */
    uint8 len_block[16];
    uint32 i;
    uint8 counter[16];
    uint8 keystream[16];

    if (ctx == NULL || iv == NULL || ciphertext == NULL || tag == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 验证标签长度 */
    if (tagLen < AES_GCM_TAG_MIN_SIZE || tagLen > AES_GCM_TAG_MAX_SIZE) {
        return AES_ERR_INVALID_TAG;
    }

    /* 计算散列子密钥 H = E(K, 0^128) */
    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, H, H);

    /* 计算J0 */
    if (ivLen == 12) {
        /* J0 = IV || 0^31 || 1 */
        memcpy(J0, iv, 12);
        J0[15] = 1;
    } else {
        /* J0 = GHASH(H, {}, IV) */
        ghash(H, NULL, 0, iv, ivLen, J0);
    }

    /* 加密明文 */
    memcpy(counter, J0, 16);
    counter[15]++;  /* 递增计数器 */

    for (i = 0; i < plaintextLen; i += 16) {
        uint32 chunkLen = (plaintextLen - i > 16) ? 16 : (plaintextLen - i);

        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, counter, keystream);

        for (uint32 j = 0; j < chunkLen; j++) {
            ciphertext[i + j] = plaintext[i + j] ^ keystream[j];
        }

        /* 递增计数器 */
        for (int32 j = 15; j >= 0; j--) {
            if (++counter[j] != 0) break;
        }
    }

    /* 计算GHASH */
    ghash(H, aad, aadLen, ciphertext, plaintextLen, S);

    /* 计算标签 T = GCTR(K, J0, S) */
    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, J0, keystream);
    for (i = 0; i < tagLen; i++) {
        tag[i] = S[i] ^ keystream[i];
    }

    /* 清除敏感数据 */
    memset(H, 0, 16);
    memset(J0, 0, 16);
    memset(S, 0, 16);
    memset(counter, 0, 16);
    memset(keystream, 0, 16);

    return AES_ERR_NONE;
}

/**
 * @brief GCM模式解密
 */
uint8 Aes_GcmDecrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      uint32 ivLen,
                      const uint8* aad,
                      uint32 aadLen,
                      const uint8* ciphertext,
                      uint32 ciphertextLen,
                      const uint8* tag,
                      uint32 tagLen,
                      uint8* plaintext,
                      uint32* plaintextLenPtr)
{
    uint8 H[16] = {0};
    uint8 J0[16] = {0};
    uint8 S[16] = {0};
    uint8 computed_tag[16];
    uint32 i;
    uint8 counter[16];
    uint8 keystream[16];

    if (ctx == NULL || iv == NULL || ciphertext == NULL || tag == NULL || plaintext == NULL || plaintextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (tagLen < AES_GCM_TAG_MIN_SIZE || tagLen > AES_GCM_TAG_MAX_SIZE) {
        return AES_ERR_INVALID_TAG;
    }

    /* 计算散列子密钥 */
    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, H, H);

    /* 计算J0 */
    if (ivLen == 12) {
        memcpy(J0, iv, 12);
        J0[15] = 1;
    } else {
        ghash(H, NULL, 0, iv, ivLen, J0);
    }

    /* 计算认证标签 */
    ghash(H, aad, aadLen, ciphertext, ciphertextLen, S);
    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, J0, keystream);
    for (i = 0; i < tagLen; i++) {
        computed_tag[i] = S[i] ^ keystream[i];
    }

    /* 验证标签 */
    for (i = 0; i < tagLen; i++) {
        if (computed_tag[i] != tag[i]) {
            memset(computed_tag, 0, 16);
            return AES_ERR_AUTHENTICATION_FAILED;
        }
    }

    /* 标签验证通过，解密数据 */
    memcpy(counter, J0, 16);
    counter[15]++;

    for (i = 0; i < ciphertextLen; i += 16) {
        uint32 chunkLen = (ciphertextLen - i > 16) ? 16 : (ciphertextLen - i);

        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, counter, keystream);

        for (uint32 j = 0; j < chunkLen; j++) {
            plaintext[i + j] = ciphertext[i + j] ^ keystream[j];
        }

        for (int32 j = 15; j >= 0; j--) {
            if (++counter[j] != 0) break;
        }
    }

    *plaintextLenPtr = ciphertextLen;

    /* 清除敏感数据 */
    memset(H, 0, 16);
    memset(J0, 0, 16);
    memset(S, 0, 16);
    memset(counter, 0, 16);
    memset(keystream, 0, 16);
    memset(computed_tag, 0, 16);

    return AES_ERR_NONE;
}

/**********************************************************************************************************************
 * 流式API实现
 *********************************************************************************************************************/

/**
 * @brief GCM流式加密 - 初始化
 */
uint8 Aes_GcmEncryptStart(Aes_ContextType* ctx,
                           Aes_GcmContextType* gcmCtx,
                           const uint8* iv,
                           uint32 ivLen)
{
    if (ctx == NULL || gcmCtx == NULL || iv == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    memset(gcmCtx, 0, sizeof(Aes_GcmContextType));

    /* 复制AES上下文 */
    memcpy(&gcmCtx->aes, ctx, sizeof(Aes_ContextType));

    /* 计算散列子密钥 */
    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, gcmCtx->H, gcmCtx->H);

    /* 计算J0 */
    if (ivLen == 12) {
        memcpy(gcmCtx->J0, iv, 12);
        gcmCtx->J0[15] = 1;
    } else {
        ghash(gcmCtx->H, NULL, 0, iv, ivLen, gcmCtx->J0);
    }

    memcpy(gcmCtx->counter, gcmCtx->J0, 16);
    gcmCtx->counter[15]++;
    gcmCtx->initialized = TRUE;

    return AES_ERR_NONE;
}

/**
 * @brief GCM流式加密 - 处理AAD
 */
uint8 Aes_GcmEncryptUpdateAad(Aes_GcmContextType* gcmCtx,
                               const uint8* aad,
                               uint32 aadLen)
{
    if (gcmCtx == NULL || aad == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!gcmCtx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 累加AAD到GHASH状态 */
    for (uint32 i = 0; i < aadLen; i += 16) {
        uint32 chunkLen = (aadLen - i > 16) ? 16 : (aadLen - i);
        uint8 buf[16] = {0};

        memcpy(buf, &aad[i], chunkLen);

        for (uint32 j = 0; j < 16; j++) {
            gcmCtx->ghash[j] ^= buf[j];
        }
        gcm_mult(gcmCtx->ghash, gcmCtx->H, gcmCtx->ghash);
    }

    gcmCtx->aadLen += aadLen;

    return AES_ERR_NONE;
}

/**
 * @brief GCM流式加密 - 加密数据
 */
uint8 Aes_GcmEncryptUpdate(Aes_GcmContextType* gcmCtx,
                            const uint8* plaintext,
                            uint32 plaintextLen,
                            uint8* ciphertext)
{
    uint8 keystream[16];

    if (gcmCtx == NULL || plaintext == NULL || ciphertext == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!gcmCtx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    for (uint32 i = 0; i < plaintextLen; i += 16) {
        uint32 chunkLen = (plaintextLen - i > 16) ? 16 : (plaintextLen - i);

        aes_encrypt_block(gcmCtx->aes.roundKey[0], gcmCtx->aes.numRounds, gcmCtx->counter, keystream);

        for (uint32 j = 0; j < chunkLen; j++) {
            ciphertext[i + j] = plaintext[i + j] ^ keystream[j];
        }

        /* 更新GHASH状态 */
        uint8 buf[16] = {0};
        memcpy(buf, &ciphertext[i], chunkLen);
        for (uint32 j = 0; j < 16; j++) {
            gcmCtx->ghash[j] ^= buf[j];
        }
        gcm_mult(gcmCtx->ghash, gcmCtx->H, gcmCtx->ghash);

        /* 递增计数器 */
        for (int32 j = 15; j >= 0; j--) {
            if (++gcmCtx->counter[j] != 0) break;
        }
    }

    gcmCtx->cipherLen += plaintextLen;

    return AES_ERR_NONE;
}

/**
 * @brief GCM流式加密 - 完成
 */
uint8 Aes_GcmEncryptFinish(Aes_GcmContextType* gcmCtx,
                            uint8* tag,
                            uint32 tagLen)
{
    uint8 len_block[16];
    uint8 keystream[16];

    if (gcmCtx == NULL || tag == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!gcmCtx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 添加长度块 */
    memset(len_block, 0, 16);
    for (int i = 0; i < 8; i++) {
        len_block[i] = (uint8)((gcmCtx->aadLen * 8) >> (56 - i * 8));
    }
    for (int i = 0; i < 8; i++) {
        len_block[8 + i] = (uint8)((gcmCtx->cipherLen * 8) >> (56 - i * 8));
    }

    for (uint32 j = 0; j < 16; j++) {
        gcmCtx->ghash[j] ^= len_block[j];
    }
    gcm_mult(gcmCtx->ghash, gcmCtx->H, gcmCtx->ghash);

    /* 计算标签 */
    aes_encrypt_block(gcmCtx->aes.roundKey[0], gcmCtx->aes.numRounds, gcmCtx->J0, keystream);

    for (uint32 i = 0; i < tagLen && i < 16; i++) {
        tag[i] = gcmCtx->ghash[i] ^ keystream[i];
    }

    /* 清除敏感数据 */
    memset(gcmCtx->H, 0, 16);
    memset(gcmCtx->J0, 0, 16);
    memset(gcmCtx->counter, 0, 16);
    memset(gcmCtx->ghash, 0, 16);
    gcmCtx->initialized = FALSE;

    return AES_ERR_NONE;
}
