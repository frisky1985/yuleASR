/**********************************************************************************************************************
 * @file       aes_ccm.c
 * @brief      AES-CCM模式实现 - 计数器密码块链消息认证码模式 (AEAD)
 *
 * 功能: 提供CCM模式的认证加密/解密功能
 *       支持AEAD
 *       适用于无线通信等场景
 *
 * CCM特点:
 * - 提供加密和认证
 * - 使用CBC-MAC进行认证
 * - 通常用于无线通信 (802.11i, ZigBee等)
 *
 * 实现参考: NIST SP 800-38C, RFC 3610
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
 * 内部辅助函数
 *********************************************************************************************************************/

/**
 * @brief 构建CCM B0块
 *
 * B0 = Flags || Nonce || l(m)
 * Flags: Reserved(1bit) || Adata(1bit) || M(3bits) || L(3bits)
 */
static void ccm_build_b0(uint8 b0[16], const uint8* nonce, uint32 nonceLen,
                         uint32 tagLen, uint32 L, uint64 msgLen)
{
    uint8 flags = 0;
    uint32 i;

    /* Adata标志 - 总是设置为1表示有AAD */
    flags |= 0x40;

    /* M (标签长度) - (M-2)/2 在3-4位 */
    flags |= (((tagLen - 2) / 2) << 3);

    /* L (L字节数) - L-1 在0-2位 */
    flags |= (L - 1);

    b0[0] = flags;

    /* Nonce */
    memcpy(&b0[1], nonce, nonceLen);

    /* l(m) - 消息长度以大端序存储在最后L字节 */
    for (i = 0; i < L; i++) {
        b0[15 - i] = (uint8)((msgLen >> (i * 8)) & 0xFF);
    }
}

/**
 * @brief 构建认证数据长度编码
 */
static uint32 ccm_encode_aad_len(uint8* out, uint64 aadLen)
{
    if (aadLen == 0) {
        return 0;
    }

    if (aadLen < 0xFF00) {
        /* 2字节长度 */
        out[0] = (uint8)((aadLen >> 8) & 0xFF);
        out[1] = (uint8)(aadLen & 0xFF);
        return 2;
    } else {
        /* 6字节长度 (0xFF || 0xFE || 长度) */
        out[0] = 0xFF;
        out[1] = 0xFE;
        out[2] = (uint8)((aadLen >> 32) & 0xFF);
        out[3] = (uint8)((aadLen >> 24) & 0xFF);
        out[4] = (uint8)((aadLen >> 16) & 0xFF);
        out[5] = (uint8)((aadLen >> 8) & 0xFF);
        out[6] = (uint8)(aadLen & 0xFF);
        return 7;  /* 2 + 5字节长度 */
    }
}

/**
 * @brief 生成CCM计数器块
 *
 * A_i = Flags || Nonce || Counter
 */
static void ccm_gen_counter(uint8 counter[16], const uint8* nonce, uint32 nonceLen,
                            uint32 L, uint32 ctr)
{
    uint32 i;

    /* Flags: 保留0, M=0, L=L-1 */
    counter[0] = L - 1;

    /* Nonce */
    memcpy(&counter[1], nonce, nonceLen);

    /* 计数器以大端序存储 */
    for (i = 0; i < L; i++) {
        counter[15 - i] = (uint8)((ctr >> (i * 8)) & 0xFF);
    }
}

/**********************************************************************************************************************
 * 全局函数实现 - CCM加密
 *********************************************************************************************************************/

/**
 * @brief CCM模式加密
 *
 * CCM提供认证加密功能。
 *
 * @param ctx AES上下文指针
 * @param nonce Nonce (7-13字节)
 * @param nonceLen Nonce长度
 * @param aad 附加认证数据 (可NULL)
 * @param aadLen AAD长度
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区 (与明文同长度)
 * @param tagLen 认证标签长度 (4, 6, 8, 10, 12, 14, 16)
 * @param tag 认证标签输出
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CcmEncrypt(Aes_ContextType* ctx,
                      const uint8* nonce,
                      uint32 nonceLen,
                      const uint8* aad,
                      uint32 aadLen,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext,
                      uint32 tagLen,
                      uint8* tag)
{
    uint8 b0[16];
    uint8 x[16] = {0};  /* CBC状态 */
    uint8 tmp[16];
    uint32 L;
    uint32 i;
    uint8 counter[16];
    uint8 keystream[16];
    uint8 aadLenBuf[8];
    uint32 aadLenEncodedLen;

    if (ctx == NULL || nonce == NULL || ciphertext == NULL || tag == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 验证参数 */
    if (nonceLen < AES_CCM_NONCE_MIN_SIZE || nonceLen > AES_CCM_NONCE_MAX_SIZE) {
        return AES_ERR_INVALID_IV;
    }

    if (tagLen < AES_CCM_TAG_MIN_SIZE || tagLen > AES_CCM_TAG_MAX_SIZE || (tagLen % 2) != 0) {
        return AES_ERR_INVALID_TAG;
    }

    /* 计算L = 15 - nonceLen */
    L = 15 - nonceLen;

    /* 构建B0 */
    ccm_build_b0(b0, nonce, nonceLen, tagLen, L, plaintextLen);

    /* 计算CBC-MAC */
    /* X_1 = E(K, B0) */
    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, b0, x);

    /* 处理AAD */
    if (aadLen > 0 && aad != NULL) {
        aadLenEncodedLen = ccm_encode_aad_len(aadLenBuf, aadLen);

        /* 处理AAD长度 */
        for (i = 0; i < aadLenEncodedLen; i++) {
            tmp[i] = aadLenBuf[i];
        }
        for (i = 0; i < aadLenEncodedLen; i++) {
            x[i] ^= tmp[i];
        }
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, x, x);

        /* 处理AAD数据 */
        for (i = 0; i < aadLen; i += 16) {
            uint32 chunkLen = (aadLen - i > 16) ? 16 : (aadLen - i);
            memset(tmp, 0, 16);
            memcpy(tmp, &aad[i], chunkLen);
            for (uint32 j = 0; j < 16; j++) {
                x[j] ^= tmp[j];
            }
            aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, x, x);
        }
    }

    /* 处理明文 */
    for (i = 0; i < plaintextLen; i += 16) {
        uint32 chunkLen = (plaintextLen - i > 16) ? 16 : (plaintextLen - i);
        memset(tmp, 0, 16);
        memcpy(tmp, &plaintext[i], chunkLen);
        for (uint32 j = 0; j < 16; j++) {
            x[j] ^= tmp[j];
        }
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, x, x);
    }

    /* 加密明文 - CTR模式 */
    for (i = 0; i < plaintextLen; i += 16) {
        uint32 chunkLen = (plaintextLen - i > 16) ? 16 : (plaintextLen - i);
        uint32 ctr = (i / 16) + 1;

        ccm_gen_counter(counter, nonce, nonceLen, L, ctr);
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, counter, keystream);

        for (uint32 j = 0; j < chunkLen; j++) {
            ciphertext[i + j] = plaintext[i + j] ^ keystream[j];
        }
    }

    /* 计算标签 */
    ccm_gen_counter(counter, nonce, nonceLen, L, 0);
    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, counter, keystream);

    for (i = 0; i < tagLen; i++) {
        tag[i] = x[i] ^ keystream[i];
    }

    /* 清除敏感数据 */
    memset(b0, 0, 16);
    memset(x, 0, 16);
    memset(counter, 0, 16);
    memset(keystream, 0, 16);

    return AES_ERR_NONE;
}

/**
 * @brief CCM模式解密
 */
uint8 Aes_CcmDecrypt(Aes_ContextType* ctx,
                      const uint8* nonce,
                      uint32 nonceLen,
                      const uint8* aad,
                      uint32 aadLen,
                      const uint8* ciphertext,
                      uint32 ciphertextLen,
                      const uint8* tag,
                      uint32 tagLen,
                      uint8* plaintext,
                      uint32* plaintextLenPtr)
{
    uint8 b0[16];
    uint8 x[16] = {0};
    uint8 tmp[16];
    uint32 L;
    uint32 i;
    uint8 counter[16];
    uint8 keystream[16];
    uint8 computed_tag[16];
    uint8 aadLenBuf[8];
    uint32 aadLenEncodedLen;

    if (ctx == NULL || nonce == NULL || ciphertext == NULL || tag == NULL || plaintext == NULL || plaintextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (nonceLen < AES_CCM_NONCE_MIN_SIZE || nonceLen > AES_CCM_NONCE_MAX_SIZE) {
        return AES_ERR_INVALID_IV;
    }

    if (tagLen < AES_CCM_TAG_MIN_SIZE || tagLen > AES_CCM_TAG_MAX_SIZE || (tagLen % 2) != 0) {
        return AES_ERR_INVALID_TAG;
    }

    L = 15 - nonceLen;

    /* 解密密文 - CTR模式 */
    for (i = 0; i < ciphertextLen; i += 16) {
        uint32 chunkLen = (ciphertextLen - i > 16) ? 16 : (ciphertextLen - i);
        uint32 ctr = (i / 16) + 1;

        ccm_gen_counter(counter, nonce, nonceLen, L, ctr);
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, counter, keystream);

        for (uint32 j = 0; j < chunkLen; j++) {
            plaintext[i + j] = ciphertext[i + j] ^ keystream[j];
        }
    }

    /* 计算CBC-MAC验证标签 */
    ccm_build_b0(b0, nonce, nonceLen, tagLen, L, ciphertextLen);
    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, b0, x);

    /* 处理AAD */
    if (aadLen > 0 && aad != NULL) {
        aadLenEncodedLen = ccm_encode_aad_len(aadLenBuf, aadLen);

        for (i = 0; i < aadLenEncodedLen; i++) {
            tmp[i] = aadLenBuf[i];
        }
        for (i = 0; i < aadLenEncodedLen; i++) {
            x[i] ^= tmp[i];
        }
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, x, x);

        for (i = 0; i < aadLen; i += 16) {
            uint32 chunkLen = (aadLen - i > 16) ? 16 : (aadLen - i);
            memset(tmp, 0, 16);
            memcpy(tmp, &aad[i], chunkLen);
            for (uint32 j = 0; j < 16; j++) {
                x[j] ^= tmp[j];
            }
            aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, x, x);
        }
    }

    /* 处理解密后的明文 */
    for (i = 0; i < ciphertextLen; i += 16) {
        uint32 chunkLen = (ciphertextLen - i > 16) ? 16 : (ciphertextLen - i);
        memset(tmp, 0, 16);
        memcpy(tmp, &plaintext[i], chunkLen);
        for (uint32 j = 0; j < 16; j++) {
            x[j] ^= tmp[j];
        }
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, x, x);
    }

    /* 计算标签 */
    ccm_gen_counter(counter, nonce, nonceLen, L, 0);
    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, counter, keystream);

    for (i = 0; i < tagLen; i++) {
        computed_tag[i] = x[i] ^ keystream[i];
    }

    /* 验证标签 */
    for (i = 0; i < tagLen; i++) {
        if (computed_tag[i] != tag[i]) {
            memset(plaintext, 0, ciphertextLen);
            memset(computed_tag, 0, 16);
            return AES_ERR_AUTHENTICATION_FAILED;
        }
    }

    *plaintextLenPtr = ciphertextLen;

    /* 清除敏感数据 */
    memset(b0, 0, 16);
    memset(x, 0, 16);
    memset(counter, 0, 16);
    memset(keystream, 0, 16);
    memset(computed_tag, 0, 16);

    return AES_ERR_NONE;
}
