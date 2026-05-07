/**********************************************************************************************************************
 * @file       aes_ctr.c
 * @brief      AES-CTR模式实现 - 计数器模式
 *
 * 功能: 提供CTR模式的AES加密/解密功能
 *       支持流式API
 *       支持随机访问加解密
 *       并行加密支持
 *
 * CTR特点:
 * - 加密和解密使用相同的操作
 * - 可并行化处理
 * - 无需填充 (流密码)
 * - 随机访问安全
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
 * 内部辅助函数 - 计数器递增
 *********************************************************************************************************************/

/**
 * @brief 递增CTR计数器 (大端序 - 高字节在前)
 */
static void ctr_increment_counter(uint8 counter[16], uint32 length)
{
    uint32 i;

    /* 从length位置开始递增 (通常12-15字节) */
    for (i = 15; i >= length; i--) {
        if (++counter[i] != 0) {
            break;  /* 没有进位 */
        }
    }
}

/**
 * @brief 递增CTR计数器 (特定跨度)
 */
static void ctr_increment_by(uint8 counter[16], uint32 length, uint64 increment)
{
    uint64 current = 0;
    uint32 i;

    /* 读取当前计数器值 */
    for (i = length; i < 16; i++) {
        current = (current << 8) | counter[i];
    }

    /* 递增 */
    current += increment;

    /* 写回 */
    for (i = 15; i >= length; i--) {
        counter[i] = (uint8)(current & 0xFF);
        current >>= 8;
    }
}

/**********************************************************************************************************************
 * 全局函数实现 - CTR加密/解密
 *********************************************************************************************************************/

/**
 * @brief CTR模式加密
 *
 * CTR模式将计数器加密后与明文异或得到密文。
 * 由于加密和解密使用相同操作，可以完全并行化。
 *
 * @param ctx AES上下文指针
 * @param nonce 随机数 + 计数器初始值 (16字节)
 * @param plaintext 明文数据
 * @param plaintextLen 明文长度
 * @param ciphertext 密文输出缓冲区
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CtrEncrypt(Aes_ContextType* ctx,
                      const uint8* nonce,
                      const uint8* plaintext,
                      uint32 plaintextLen,
                      uint8* ciphertext)
{
    uint8 counter[AES_BLOCK_SIZE];
    uint8 keystream[AES_BLOCK_SIZE];
    uint32 i;
    uint32 fullBlocks;
    uint32 remainingBytes;

    if (ctx == NULL || nonce == NULL || (plaintext == NULL && plaintextLen > 0) || ciphertext == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (plaintextLen == 0) {
        return AES_ERR_NONE;  /* 空数据 */
    }

    /* 初始化计数器 (Nonce复制) */
    memcpy(counter, nonce, AES_BLOCK_SIZE);

    fullBlocks = plaintextLen / AES_BLOCK_SIZE;
    remainingBytes = plaintextLen % AES_BLOCK_SIZE;

    /* 处理完整块 */
    for (i = 0; i < fullBlocks; i++) {
        /* 加密计数器 */
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, counter, keystream);

        /* 异或得到密文 */
        for (uint32 j = 0; j < AES_BLOCK_SIZE; j++) {
            ciphertext[i * AES_BLOCK_SIZE + j] = plaintext[i * AES_BLOCK_SIZE + j] ^ keystream[j];
        }

        /* 递增计数器 (偏移12，即8字节随机数 + 4字节计数器) */
        ctr_increment_counter(counter, 12);
    }

    /* 处理剩余字节 */
    if (remainingBytes > 0) {
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, counter, keystream);

        for (uint32 j = 0; j < remainingBytes; j++) {
            ciphertext[fullBlocks * AES_BLOCK_SIZE + j] = plaintext[fullBlocks * AES_BLOCK_SIZE + j] ^ keystream[j];
        }
    }

    /* 清除敏感数据 */
    memset(counter, 0, AES_BLOCK_SIZE);
    memset(keystream, 0, AES_BLOCK_SIZE);

    return AES_ERR_NONE;
}

/**********************************************************************************************************************
 * 流式API实现
 *********************************************************************************************************************/

/**
 * @brief CTR流式加密 - 开始
 */
uint8 Aes_CtrEncryptStart(Aes_ContextType* ctx, const uint8* nonce)
{
    if (ctx == NULL || nonce == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 保存nonce到IV字段 (实际是计数器) */
    memcpy(ctx->iv, nonce, AES_BLOCK_SIZE);
    ctx->blockCount = 0;
    ctx->tempLen = 0;

    return AES_ERR_NONE;
}

/**
 * @brief CTR流式加密 - 更新
 */
uint8 Aes_CtrEncryptUpdate(Aes_ContextType* ctx,
                            const uint8* plaintext,
                            uint32 plaintextLen,
                            uint8* ciphertext)
{
    uint32 i;
    uint32 fullBlocks;
    uint32 remainingBytes;
    uint32 offset = 0;

    if (ctx == NULL || plaintext == NULL || ciphertext == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (plaintextLen == 0) {
        return AES_ERR_NONE;
    }

    /* 处理临时缓冲区中的剩余数据 */
    if (ctx->tempLen > 0) {
        uint32 needed = AES_BLOCK_SIZE - ctx->tempLen;
        uint32 toCopy = (plaintextLen < needed) ? plaintextLen : needed;

        memcpy(&ctx->tempBlock[ctx->tempLen], plaintext, toCopy);
        ctx->tempLen += toCopy;
        plaintext += toCopy;
        plaintextLen -= toCopy;

        if (ctx->tempLen == AES_BLOCK_SIZE) {
            /* 加密计数器并异或 */
            aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, ctx->iv, ctx->tempBlock);
            ctr_increment_counter(ctx->iv, 12);
            ctx->blockCount++;

            /* 异或数据 */
            for (uint32 j = 0; j < AES_BLOCK_SIZE; j++) {
                ciphertext[offset++] = ctx->tempBlock[j] ^ plaintext[j - toCopy];
            }
            ctx->tempLen = 0;
        }
    }

    /* 处理完整块 */
    fullBlocks = plaintextLen / AES_BLOCK_SIZE;
    for (i = 0; i < fullBlocks; i++) {
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, ctx->iv, keystream);
        ctr_increment_counter(ctx->iv, 12);
        ctx->blockCount++;

        for (uint32 j = 0; j < AES_BLOCK_SIZE; j++) {
            ciphertext[offset++] = plaintext[i * AES_BLOCK_SIZE + j] ^ keystream[j];
        }
    }

    /* 保存剩余数据到临时缓冲区 */
    remainingBytes = plaintextLen % AES_BLOCK_SIZE;
    if (remainingBytes > 0) {
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, ctx->iv, keystream);
        ctr_increment_counter(ctx->iv, 12);
        ctx->blockCount++;

        for (uint32 j = 0; j < remainingBytes; j++) {
            ciphertext[offset++] = plaintext[fullBlocks * AES_BLOCK_SIZE + j] ^ keystream[j];
        }
    }

    return AES_ERR_NONE;
}

/**
 * @brief CTR流式加密 - 完成
 *
 * CTR模式无需填充，完成函数只是清理状态。
 */
uint8 Aes_CtrEncryptFinish(Aes_ContextType* ctx)
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
    ctx->blockCount = 0;

    return AES_ERR_NONE;
}

/**********************************************************************************************************************
 * 进阶功能 - 随机访问
 *********************************************************************************************************************/

/**
 * @brief CTR模式加密特定块
 *
 * 支持随机访问加密，用于仅需加密/解密特定块的场景。
 *
 * @param ctx AES上下文指针
 * @param nonce 随机数 (16字节)
 * @param blockIndex 块索引 (0开始)
 * @param plaintext 明文块 (16字节)
 * @param ciphertext 密文输出 (16字节)
 * @return AES_ERR_NONE成功
 */
uint8 Aes_CtrEncryptBlock(Aes_ContextType* ctx,
                           const uint8* nonce,
                           uint64 blockIndex,
                           const uint8* plaintext,
                           uint8* ciphertext)
{
    uint8 counter[AES_BLOCK_SIZE];
    uint8 keystream[AES_BLOCK_SIZE];

    if (ctx == NULL || nonce == NULL || plaintext == NULL || ciphertext == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    /* 初始化计数器 */
    memcpy(counter, nonce, AES_BLOCK_SIZE);

    /* 设置块索引到计数器 */
    ctr_increment_by(counter, 12, blockIndex);

    /* 加密计数器 */
    aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, counter, keystream);

    /* 异或得到密文 */
    for (uint32 j = 0; j < AES_BLOCK_SIZE; j++) {
        ciphertext[j] = plaintext[j] ^ keystream[j];
    }

    /* 清除敏感数据 */
    memset(counter, 0, AES_BLOCK_SIZE);
    memset(keystream, 0, AES_BLOCK_SIZE);

    return AES_ERR_NONE;
}
