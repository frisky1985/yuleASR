/**********************************************************************************************************************
 * @file       aes_ecb.c
 * @brief      AES-ECB模式实现 - 电子密码本模式
 *
 * 功能: 提供ECB模式的AES加密/解密功能
 *       支持PKCS#7填充
 *       注意: ECB模式不推荐用于安全应用，因为相同明文块会产生相同密文块
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
 * 全局函数实现 - ECB加密
 *********************************************************************************************************************/

/**
 * @brief ECB模式加密
 *
 * ECB模式对每个明文块独立加密，不使用IV。支持PKCS#7填充以处理任意长度的数据。
 *
 * @note 输出缓冲区应至少为: ((inputLen / 16) + 1) * 16 字节
 */
uint8 Aes_EcbEncrypt(Aes_ContextType* ctx,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext,
                      uint32* ciphertextLenPtr)
{
    uint32 i;
    uint8  tempBlock[AES_BLOCK_SIZE];
    uint32 numBlocks;
    uint32 outputLen;

    if (ctx == NULL || plaintext == NULL || ciphertext == NULL || ciphertextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (plaintextLen == 0) {
        return AES_ERR_INVALID_LENGTH;
    }

    /* 计算需要的块数 (包括填充) */
    numBlocks = (plaintextLen / AES_BLOCK_SIZE) + 1;
    outputLen = numBlocks * AES_BLOCK_SIZE;

    /* 检查输出缓冲区大小 */
    if (*ciphertextLenPtr < outputLen) {
        return AES_ERR_BUFFER_TOO_SMALL;
    }

    /* 加密完整块 */
    for (i = 0; i < (plaintextLen / AES_BLOCK_SIZE); i++) {
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds,
                          &plaintext[i * AES_BLOCK_SIZE],
                          &ciphertext[i * AES_BLOCK_SIZE]);
    }

    /* 处理最后一块 (包含填充) */
    uint32 remaining = plaintextLen % AES_BLOCK_SIZE;
    memcpy(tempBlock, &plaintext[i * AES_BLOCK_SIZE], remaining);

    /* PKCS#7填充 */
    uint8 padLen = AES_BLOCK_SIZE - remaining;
    for (uint32 j = remaining; j < AES_BLOCK_SIZE; j++) {
        tempBlock[j] = padLen;
    }

    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, tempBlock, &ciphertext[i * AES_BLOCK_SIZE]);

    /* 清除敏感数据 */
    memset(tempBlock, 0, AES_BLOCK_SIZE);

    *ciphertextLenPtr = outputLen;
    return AES_ERR_NONE;
}

/**
 * @brief ECB模式解密
 *
 * 从密文解密数据，并自动移除PKCS#7填充。
 */
uint8 Aes_EcbDecrypt(Aes_ContextType* ctx,
                      const uint8* ciphertext,
                      uint32 ciphertextLen,
                      uint8* plaintext,
                      uint32* plaintextLenPtr)
{
    uint32 i;
    uint32 numBlocks;
    uint32 outputLen;

    if (ctx == NULL || ciphertext == NULL || plaintext == NULL || plaintextLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 密文长度必须是块大小的倍数 */
    if (ciphertextLen == 0 || (ciphertextLen % AES_BLOCK_SIZE) != 0) {
        return AES_ERR_INVALID_LENGTH;
    }

    numBlocks = ciphertextLen / AES_BLOCK_SIZE;

    /* 解密所有块 (除了可能的最后一块需要特殊处理来检查填充) */
    for (i = 0; i < numBlocks; i++) {
        aes_decrypt_block(ctx->roundKey[0], ctx->numRounds,
                          &ciphertext[i * AES_BLOCK_SIZE],
                          &plaintext[i * AES_BLOCK_SIZE]);
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

    outputLen = ciphertextLen - padLen;

    /* 检查输出缓冲区大小 */
    if (*plaintextLenPtr < outputLen) {
        return AES_ERR_BUFFER_TOO_SMALL;
    }

    *plaintextLenPtr = outputLen;

    return AES_ERR_NONE;
}

/**********************************************************************************************************************
 * 单块ECB操作 (用于内部调用)
 *********************************************************************************************************************/

/**
 * @brief ECB模式多块加密 (无填充)
 *
 * 用于需要严格块对齐的场景。
 */
uint8 Aes_EcbEncryptNoPadding(Aes_ContextType* ctx,
                               const uint8* plaintext,
                               uint32 plaintextLen,
                               uint8* ciphertext)
{
    uint32 numBlocks;

    if (ctx == NULL || plaintext == NULL || ciphertext == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (plaintextLen == 0 || (plaintextLen % AES_BLOCK_SIZE) != 0) {
        return AES_ERR_INVALID_LENGTH;
    }

    numBlocks = plaintextLen / AES_BLOCK_SIZE;

    for (uint32 i = 0; i < numBlocks; i++) {
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds,
                          &plaintext[i * AES_BLOCK_SIZE],
                          &ciphertext[i * AES_BLOCK_SIZE]);
    }

    return AES_ERR_NONE;
}

/**
 * @brief ECB模式多块解密 (无填充)
 */
uint8 Aes_EcbDecryptNoPadding(Aes_ContextType* ctx,
                               const uint8* ciphertext,
                               uint32 ciphertextLen,
                               uint8* plaintext)
{
    uint32 numBlocks;

    if (ctx == NULL || ciphertext == NULL || plaintext == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (ciphertextLen == 0 || (ciphertextLen % AES_BLOCK_SIZE) != 0) {
        return AES_ERR_INVALID_LENGTH;
    }

    numBlocks = ciphertextLen / AES_BLOCK_SIZE;

    for (uint32 i = 0; i < numBlocks; i++) {
        aes_decrypt_block(ctx->roundKey[0], ctx->numRounds,
                          &ciphertext[i * AES_BLOCK_SIZE],
                          &plaintext[i * AES_BLOCK_SIZE]);
    }

    return AES_ERR_NONE;
}
