/**********************************************************************************************************************
 * @file       aes_cbc.c
 * @brief      AES-CBC模式实现 - 密码块链接模式
 *
 * 功能: 提供CBC模式的AES加密/解密功能
 *       支持PKCS#7填充
 *       支持流式API用于大数据加解密
 *
 * 实现参考: FIPS-197, NIST SP 800-38A
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
extern void aes_decrypt_block(const uint8* rk, uint32 nr, const uint8 input[16], uint8 output[16]);

/**********************************************************************************************************************
 * 全局函数实现 - CBC加密
 *********************************************************************************************************************/

/**
 * @brief CBC模式加密
 *
 * CBC模式将前一块的密文与当前块的明文异或后再加密，提供了更好的安全性。
 */
uint8 Aes_CbcEncrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext,
                      uint32* ciphertextLenPtr)
{
    uint32 i;
    uint8  tempBlock[AES_BLOCK_SIZE];
    uint8  ivLocal[AES_BLOCK_SIZE];
    uint32 numBlocks;
    uint32 outputLen;

    if (ctx == NULL || iv == NULL || plaintext == NULL || ciphertext == NULL || ciphertextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (plaintextLen == 0) {
        return AES_ERR_INVALID_LENGTH;
    }

    /* 复制IV到本地 */
    memcpy(ivLocal, iv, AES_BLOCK_SIZE);

    /* 计算需要的块数 */
    numBlocks = (plaintextLen / AES_BLOCK_SIZE) + 1;
    outputLen = numBlocks * AES_BLOCK_SIZE;

    /* 检查输出缓冲区大小 */
    if (*ciphertextLenPtr < outputLen) {
        return AES_ERR_BUFFER_TOO_SMALL;
    }

    /* 加密完整块 */
    for (i = 0; i < (plaintextLen / AES_BLOCK_SIZE); i++) {
        /* 异或IV前块密文 */
        for (uint32 j = 0; j < AES_BLOCK_SIZE; j++) {
            tempBlock[j] = plaintext[i * AES_BLOCK_SIZE + j] ^ ivLocal[j];
        }

        /* 加密 */
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, tempBlock, &ciphertext[i * AES_BLOCK_SIZE]);

        /* 更新IV为当前密文块 */
        memcpy(ivLocal, &ciphertext[i * AES_BLOCK_SIZE], AES_BLOCK_SIZE);
    }

    /* 处理最后一块 (包含填充) */
    uint32 remaining = plaintextLen % AES_BLOCK_SIZE;
    memcpy(tempBlock, &plaintext[i * AES_BLOCK_SIZE], remaining);

    /* PKCS#7填充 */
    uint8 padLen = AES_BLOCK_SIZE - remaining;
    for (uint32 j = remaining; j < AES_BLOCK_SIZE; j++) {
        tempBlock[j] = padLen;
    }

    /* 异或并加密 */
    for (uint32 j = 0; j < AES_BLOCK_SIZE; j++) {
        tempBlock[j] ^= ivLocal[j];
    }
    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, tempBlock, &ciphertext[i * AES_BLOCK_SIZE]);

    /* 清除敏感数据 */
    memset(tempBlock, 0, AES_BLOCK_SIZE);
    memset(ivLocal, 0, AES_BLOCK_SIZE);

    *ciphertextLenPtr = outputLen;
    return AES_ERR_NONE;
}

/**
 * @brief CBC模式解密
 */
uint8 Aes_CbcDecrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      const uint8* ciphertext,
                      uint32 ciphertextLen,
                      uint8* plaintext,
                      uint32* plaintextLenPtr)
{
    uint32 i;
    uint8  tempBlock[AES_BLOCK_SIZE];
    uint8  ivLocal[AES_BLOCK_SIZE];
    uint8  prevCipher[AES_BLOCK_SIZE];
    uint32 numBlocks;

    if (ctx == NULL || iv == NULL || ciphertext == NULL || plaintext == NULL || plaintextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (ciphertextLen == 0 || (ciphertextLen % AES_BLOCK_SIZE) != 0) {
        return AES_ERR_INVALID_LENGTH;
    }

    numBlocks = ciphertextLen / AES_BLOCK_SIZE;

    /* 检查输出缓冲区大小 (至少需要ciphertextLen字节) */
    if (*plaintextLenPtr < (ciphertextLen - AES_BLOCK_SIZE)) {
        return AES_ERR_BUFFER_TOO_SMALL;
    }

    /* 复制IV到本地 */
    memcpy(ivLocal, iv, AES_BLOCK_SIZE);

    /* 解密所有块 */
    for (i = 0; i < numBlocks; i++) {
        /* 保存当前密文块用于下一轮异或 */
        memcpy(prevCipher, &ciphertext[i * AES_BLOCK_SIZE], AES_BLOCK_SIZE);

        /* 解密 */
        aes_decrypt_block(ctx->roundKey[0], ctx->numRounds,
                          &ciphertext[i * AES_BLOCK_SIZE], tempBlock);

        /* 异或IV前块密文 */
        for (uint32 j = 0; j < AES_BLOCK_SIZE; j++) {
            plaintext[i * AES_BLOCK_SIZE + j] = tempBlock[j] ^ ivLocal[j];
        }

        /* 更新IV为当前密文块 */
        memcpy(ivLocal, prevCipher, AES_BLOCK_SIZE);
    }

    /* 移除PKCS#7填充 */
    uint8 padLen = plaintext[ciphertextLen - 1];

    /* 验证填充 */
    if (padLen == 0 || padLen > AES_BLOCK_SIZE) {
        return AES_ERR_INVALID_LENGTH;
    }

    /* 检查填充一致性 */
    for (i = 0; i < padLen; i++) {
        if (plaintext[ciphertextLen - 1 - i] != padLen) {
            return AES_ERR_INVALID_LENGTH;
        }
    }

    *plaintextLenPtr = ciphertextLen - padLen;

    /* 清除敏感数据 */
    memset(tempBlock, 0, AES_BLOCK_SIZE);
    memset(ivLocal, 0, AES_BLOCK_SIZE);
    memset(prevCipher, 0, AES_BLOCK_SIZE);

    return AES_ERR_NONE;
}

/**********************************************************************************************************************
 * 流式API实现 - CBC加密
 *********************************************************************************************************************/

/**
 * @brief CBC流式加密 - 开始
 */
uint8 Aes_CbcEncryptStart(Aes_ContextType* ctx, const uint8* iv)
{
    if (ctx == NULL || iv == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 保存IV到上下文 */
    memcpy(ctx->iv, iv, AES_BLOCK_SIZE);
    ctx->blockCount = 0;
    ctx->tempLen = 0;

    return AES_ERR_NONE;
}

/**
 * @brief CBC流式加密 - 更新
 *
 * 输入数据必须是16字节的倍数。如果需要处理非对齐数据，请在最后使用Aes_CbcEncryptFinish。
 */
uint8 Aes_CbcEncryptUpdate(Aes_ContextType* ctx,
                            const uint8* plaintext,
                            uint32 plaintextLen,
                            uint8* ciphertext,
                            uint32* ciphertextLenPtr)
{
    uint32 i;
    uint32 numBlocks;

    if (ctx == NULL || plaintext == NULL || ciphertext == NULL || ciphertextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (plaintextLen == 0 || (plaintextLen % AES_BLOCK_SIZE) != 0) {
        return AES_ERR_INVALID_LENGTH;
    }

    numBlocks = plaintextLen / AES_BLOCK_SIZE;

    for (i = 0; i < numBlocks; i++) {
        /* 异或IV前块密文 */
        for (uint32 j = 0; j < AES_BLOCK_SIZE; j++) {
            ctx->tempBlock[j] = plaintext[i * AES_BLOCK_SIZE + j] ^ ctx->iv[j];
        }

        /* 加密 */
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, ctx->tempBlock, &ciphertext[i * AES_BLOCK_SIZE]);

        /* 更新IV */
        memcpy(ctx->iv, &ciphertext[i * AES_BLOCK_SIZE], AES_BLOCK_SIZE);
    }

    ctx->blockCount += numBlocks;
    *ciphertextLenPtr = plaintextLen;

    return AES_ERR_NONE;
}

/**
 * @brief CBC流式加密 - 完成
 *
 * 处理剩余数据并添加PKCS#7填充。
 */
uint8 Aes_CbcEncryptFinish(Aes_ContextType* ctx,
                            const uint8* plaintext,
                            uint32 plaintextLen,
                            uint8* ciphertext,
                            uint32* ciphertextLenPtr)
{
    uint32 i;
    uint8  padLen;

    if (ctx == NULL || ciphertext == NULL || ciphertextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* plaintext可以为NULL表示没有剩余数据 */
    if (plaintext == NULL) {
        plaintextLen = 0;
    }

    if (plaintextLen >= AES_BLOCK_SIZE) {
        return AES_ERR_INVALID_LENGTH;
    }

    /* 检查输出缓冲区大小 */
    if (*ciphertextLenPtr < AES_BLOCK_SIZE) {
        return AES_ERR_BUFFER_TOO_SMALL;
    }

    /* 复制剩余数据 */
    if (plaintextLen > 0) {
        memcpy(ctx->tempBlock, plaintext, plaintextLen);
    }

    /* PKCS#7填充 */
    padLen = AES_BLOCK_SIZE - plaintextLen;
    for (i = plaintextLen; i < AES_BLOCK_SIZE; i++) {
        ctx->tempBlock[i] = padLen;
    }

    /* 异或并加密 */
    for (i = 0; i < AES_BLOCK_SIZE; i++) {
        ctx->tempBlock[i] ^= ctx->iv[i];
    }

    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, ctx->tempBlock, ciphertext);

    /* 清除敏感数据 */
    memset(ctx->tempBlock, 0, AES_BLOCK_SIZE);
    memset(ctx->iv, 0, AES_BLOCK_SIZE);

    *ciphertextLenPtr = AES_BLOCK_SIZE;

    return AES_ERR_NONE;
}

/**********************************************************************************************************************
 * 流式API实现 - CBC解密
 *********************************************************************************************************************/

/**
 * @brief CBC流式解密 - 开始
 */
uint8 Aes_CbcDecryptStart(Aes_ContextType* ctx, const uint8* iv)
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
 * @brief CBC流式解密 - 更新
 */
uint8 Aes_CbcDecryptUpdate(Aes_ContextType* ctx,
                            const uint8* ciphertext,
                            uint32 ciphertextLen,
                            uint8* plaintext,
                            uint32* plaintextLenPtr)
{
    uint32 i;
    uint32 numBlocks;
    uint8  prevCipher[AES_BLOCK_SIZE];

    if (ctx == NULL || ciphertext == NULL || plaintext == NULL || ciphertextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (ciphertextLen == 0 || (ciphertextLen % AES_BLOCK_SIZE) != 0) {
        return AES_ERR_INVALID_LENGTH;
    }

    numBlocks = ciphertextLen / AES_BLOCK_SIZE;

    for (i = 0; i < numBlocks; i++) {
        memcpy(prevCipher, &ciphertext[i * AES_BLOCK_SIZE], AES_BLOCK_SIZE);

        aes_decrypt_block(ctx->roundKey[0], ctx->numRounds,
                          &ciphertext[i * AES_BLOCK_SIZE], ctx->tempBlock);

        for (uint32 j = 0; j < AES_BLOCK_SIZE; j++) {
            plaintext[i * AES_BLOCK_SIZE + j] = ctx->tempBlock[j] ^ ctx->iv[j];
        }

        memcpy(ctx->iv, prevCipher, AES_BLOCK_SIZE);
    }

    *ciphertextLenPtr = ciphertextLen;

    /* 清除敏感数据 */
    memset(prevCipher, 0, AES_BLOCK_SIZE);

    return AES_ERR_NONE;
}

/**
 * @brief CBC流式解密 - 完成
 *
 * 移除PKCS#7填充。最后一块应该作为输入传入，返回去除填充后的真实长度。
 */
uint8 Aes_CbcDecryptFinish(Aes_ContextType* ctx,
                            const uint8* ciphertext,
                            uint8* plaintext,
                            uint32* plaintextLenPtr)
{
    uint8  padLen;
    uint32 i;

    if (ctx == NULL || ciphertext == NULL || plaintext == NULL || plaintextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 解密最后一块 */
    aes_decrypt_block(ctx->roundKey[0], ctx->numRounds, ciphertext, ctx->tempBlock);

    /* 异或IV */
    for (i = 0; i < AES_BLOCK_SIZE; i++) {
        ctx->tempBlock[i] ^= ctx->iv[i];
    }

    /* 移除PKCS#7填充 */
    padLen = ctx->tempBlock[AES_BLOCK_SIZE - 1];

    if (padLen == 0 || padLen > AES_BLOCK_SIZE) {
        return AES_ERR_INVALID_LENGTH;
    }

    /* 验证填充 */
    for (i = AES_BLOCK_SIZE - padLen; i < AES_BLOCK_SIZE; i++) {
        if (ctx->tempBlock[i] != padLen) {
            return AES_ERR_INVALID_LENGTH;
        }
    }

    /* 复制有效数据 */
    *plaintextLenPtr = AES_BLOCK_SIZE - padLen;
    memcpy(plaintext, ctx->tempBlock, *plaintextLenPtr);

    /* 清除敏感数据 */
    memset(ctx->tempBlock, 0, AES_BLOCK_SIZE);
    memset(ctx->iv, 0, AES_BLOCK_SIZE);

    return AES_ERR_NONE;
}
