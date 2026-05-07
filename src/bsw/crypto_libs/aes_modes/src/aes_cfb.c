/**********************************************************************************************************************
 * @file       aes_cfb.c
 * @brief      AES-CFB模式实现 - 密码反馈模式
 *
 * 功能: 提供CFB模式的AES加密/解密功能
 *       支持CFB全块模式 (CFB128)
 *       支持CFB8模式 (8-bit反馈)
 *
 * CFB特点:
 * - 密码流模式
 * - 加密使用AES加密，解密使用AES加密 (与明文异或)
 * - 无需填充 (流密码)
 *
 * 实现参考: NIST SP 800-38A
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
 * 全局函数实现 - CFB加密/解密
 *********************************************************************************************************************/

/**
 * @brief CFB模式加密 (CFB128 - 全块反馈)
 *
 * CFB模式将输出反馈到输入，形成密码流。
 * 加密和解密都使用AES加密，因此需要的硬件资源更少。
 *
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @param ciphertextLenPtr 输出: 实际密文长度 (等于输入长度)
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CfbEncrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext,
                      uint32* ciphertextLenPtr)
{
    uint8 feedback[AES_BLOCK_SIZE];
    uint8 keystream[AES_BLOCK_SIZE];
    uint32 i;

    if (ctx == NULL || iv == NULL || plaintext == NULL || ciphertext == NULL || ciphertextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (plaintextLen == 0) {
        *ciphertextLenPtr = 0;
        return AES_ERR_NONE;
    }

    /* 初始化反馈寄存器 */
    memcpy(feedback, iv, AES_BLOCK_SIZE);

    for (i = 0; i < plaintextLen; i += AES_BLOCK_SIZE) {
        uint32 chunkLen = (plaintextLen - i > AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : (plaintextLen - i);

        /* 加密反馈寄存器 */
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, feedback, keystream);

        /* 异或得到密文 */
        for (uint32 j = 0; j < chunkLen; j++) {
            ciphertext[i + j] = plaintext[i + j] ^ keystream[j];
        }

        /* 更新反馈寄存器: 密文块 (或部分块) */
        if (chunkLen == AES_BLOCK_SIZE) {
            memcpy(feedback, &ciphertext[i], AES_BLOCK_SIZE);
        } else {
            /* 移动剩余并放入新密文 */
            memmove(feedback, &feedback[chunkLen], AES_BLOCK_SIZE - chunkLen);
            memcpy(&feedback[AES_BLOCK_SIZE - chunkLen], &ciphertext[i], chunkLen);
        }
    }

    *ciphertextLenPtr = plaintextLen;

    /* 清除敏感数据 */
    memset(feedback, 0, AES_BLOCK_SIZE);
    memset(keystream, 0, AES_BLOCK_SIZE);

    return AES_ERR_NONE;
}

/**
 * @brief CFB模式解密 (CFB128)
 */
uint8 Aes_CfbDecrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      const uint8* ciphertext,
                      uint32 ciphertextLen,
                      uint8* plaintext,
                      uint32* plaintextLenPtr)
{
    uint8 feedback[AES_BLOCK_SIZE];
    uint8 keystream[AES_BLOCK_SIZE];
    uint32 i;

    if (ctx == NULL || iv == NULL || ciphertext == NULL || plaintext == NULL || plaintextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (ciphertextLen == 0) {
        *plaintextLenPtr = 0;
        return AES_ERR_NONE;
    }

    /* 初始化反馈寄存器 */
    memcpy(feedback, iv, AES_BLOCK_SIZE);

    for (i = 0; i < ciphertextLen; i += AES_BLOCK_SIZE) {
        uint32 chunkLen = (ciphertextLen - i > AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : (ciphertextLen - i);

        /* 加密反馈寄存器 */
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, feedback, keystream);

        /* 异或得到明文 */
        for (uint32 j = 0; j < chunkLen; j++) {
            plaintext[i + j] = ciphertext[i + j] ^ keystream[j];
        }

        /* 更新反馈寄存器: 密文块 (或部分块) */
        if (chunkLen == AES_BLOCK_SIZE) {
            memcpy(feedback, &ciphertext[i], AES_BLOCK_SIZE);
        } else {
            memmove(feedback, &feedback[chunkLen], AES_BLOCK_SIZE - chunkLen);
            memcpy(&feedback[AES_BLOCK_SIZE - chunkLen], &ciphertext[i], chunkLen);
        }
    }

    *plaintextLenPtr = ciphertextLen;

    /* 清除敏感数据 */
    memset(feedback, 0, AES_BLOCK_SIZE);
    memset(keystream, 0, AES_BLOCK_SIZE);

    return AES_ERR_NONE;
}

/**********************************************************************************************************************
 * CFB8实现 (8-bit反馈)
 *********************************************************************************************************************/

/**
 * @brief CFB8模式加密 (8-bit反馈)
 *
 * 逐字节加密，适用于需要逐字节处理的场景。
 *
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @return AES_ERR_NONE成功
 */
uint8 Aes_Cfb8Encrypt(Aes_ContextType* ctx,
                       const uint8* iv,
                       const uint8* plaintext,
                       uint32 plaintextLen,
                       uint8* ciphertext)
{
    uint8 feedback[AES_BLOCK_SIZE];
    uint8 keystream[AES_BLOCK_SIZE];
    uint32 i;

    if (ctx == NULL || iv == NULL || plaintext == NULL || ciphertext == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 初始化反馈寄存器 */
    memcpy(feedback, iv, AES_BLOCK_SIZE);

    for (i = 0; i < plaintextLen; i++) {
        /* 加密反馈寄存器 */
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, feedback, keystream);

        /* 取第一字节异或 */
        ciphertext[i] = plaintext[i] ^ keystream[0];

        /* 移动反馈寄存器并放入密文 */
        memmove(feedback, &feedback[1], AES_BLOCK_SIZE - 1);
        feedback[AES_BLOCK_SIZE - 1] = ciphertext[i];
    }

    /* 清除敏感数据 */
    memset(feedback, 0, AES_BLOCK_SIZE);
    memset(keystream, 0, AES_BLOCK_SIZE);

    return AES_ERR_NONE;
}

/**
 * @brief CFB8模式解密
 */
uint8 Aes_Cfb8Decrypt(Aes_ContextType* ctx,
                       const uint8* iv,
                       const uint8* ciphertext,
                       uint32 ciphertextLen,
                       uint8* plaintext)
{
    uint8 feedback[AES_BLOCK_SIZE];
    uint8 keystream[AES_BLOCK_SIZE];
    uint32 i;

    if (ctx == NULL || iv == NULL || ciphertext == NULL || plaintext == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 初始化反馈寄存器 */
    memcpy(feedback, iv, AES_BLOCK_SIZE);

    for (i = 0; i < ciphertextLen; i++) {
        /* 加密反馈寄存器 */
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, feedback, keystream);

        /* 取第一字节异或 */
        plaintext[i] = ciphertext[i] ^ keystream[0];

        /* 移动反馈寄存器并放入密文 (注意：是密文，不是明文) */
        memmove(feedback, &feedback[1], AES_BLOCK_SIZE - 1);
        feedback[AES_BLOCK_SIZE - 1] = ciphertext[i];
    }

    /* 清除敏感数据 */
    memset(feedback, 0, AES_BLOCK_SIZE);
    memset(keystream, 0, AES_BLOCK_SIZE);

    return AES_ERR_NONE;
}

/**********************************************************************************************************************
 * CFB流式API
 *********************************************************************************************************************/

/**
 * @brief CFB流式加密 - 开始
 */
uint8 Aes_CfbEncryptStart(Aes_ContextType* ctx, const uint8* iv)
{
    if (ctx == NULL || iv == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    memcpy(ctx->iv, iv, AES_BLOCK_SIZE);
    ctx->blockCount = 0;
    ctx->tempLen = 0;

    return AES_ERR_NONE;
}

/**
 * @brief CFB流式加密 - 更新
 */
uint8 Aes_CfbEncryptUpdate(Aes_ContextType* ctx,
                            const uint8* plaintext,
                            uint32 plaintextLen,
                            uint8* ciphertext)
{
    if (ctx == NULL || plaintext == NULL || ciphertext == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    for (uint32 i = 0; i < plaintextLen; i++) {
        /* 加密IV寄存器 */
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, ctx->iv, ctx->tempBlock);

        /* 取第一字节异或 */
        ciphertext[i] = plaintext[i] ^ ctx->tempBlock[0];

        /* 移动IV并放入密文 */
        memmove(ctx->iv, &ctx->iv[1], AES_BLOCK_SIZE - 1);
        ctx->iv[AES_BLOCK_SIZE - 1] = ciphertext[i];
    }

    return AES_ERR_NONE;
}

/**
 * @brief CFB流式加密 - 完成
 */
uint8 Aes_CfbEncryptFinish(Aes_ContextType* ctx)
{
    if (ctx == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 清除敏感状态 */
    memset(ctx->iv, 0, AES_BLOCK_SIZE);
    memset(ctx->tempBlock, 0, AES_BLOCK_SIZE);
    ctx->tempLen = 0;

    return AES_ERR_NONE;
}

/**
 * @brief CFB流式解密 - 开始
 */
uint8 Aes_CfbDecryptStart(Aes_ContextType* ctx, const uint8* iv)
{
    if (ctx == NULL || iv == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    memcpy(ctx->iv, iv, AES_BLOCK_SIZE);
    ctx->blockCount = 0;

    return AES_ERR_NONE;
}

/**
 * @brief CFB流式解密 - 更新
 */
uint8 Aes_CfbDecryptUpdate(Aes_ContextType* ctx,
                            const uint8* ciphertext,
                            uint32 ciphertextLen,
                            uint8* plaintext)
{
    if (ctx == NULL || ciphertext == NULL || plaintext == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    for (uint32 i = 0; i < ciphertextLen; i++) {
        /* 加密IV寄存器 */
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, ctx->iv, ctx->tempBlock);

        /* 取第一字节异或 */
        plaintext[i] = ciphertext[i] ^ ctx->tempBlock[0];

        /* 移动IV并放入密文 */
        memmove(ctx->iv, &ctx->iv[1], AES_BLOCK_SIZE - 1);
        ctx->iv[AES_BLOCK_SIZE - 1] = ciphertext[i];
    }

    return AES_ERR_NONE;
}

/**
 * @brief CFB流式解密 - 完成
 */
uint8 Aes_CfbDecryptFinish(Aes_ContextType* ctx)
{
    return Aes_CfbEncryptFinish(ctx);  /* 相同的清理逻辑 */
}
