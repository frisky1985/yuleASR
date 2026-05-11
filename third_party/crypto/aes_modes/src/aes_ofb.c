/**********************************************************************************************************************
 * @file       aes_ofb.c
 * @brief      AES-OFB模式实现 - 输出反馈模式
 *
 * 功能: 提供OFB模式的AES加密/解密功能
 *       支持流式API
 *
 * OFB特点:
 * - 密码流模式
 * - 加密和解密完全相同 (都是密钥流与数据异或)
 * - 可并行化生成密钥流
 * - 无需填充
 * - 错误传播比CFB更小
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
 * 全局函数实现 - OFB加密/解密
 *********************************************************************************************************************/

/**
 * @brief OFB模式加密/解密
 *
 * OFB模式生成密钥流与数据异或。加密和解密使用相同的逻辑。
 *
 * @param ctx AES上下文指针
 * @param iv 初始化向量 (16字节)
 * @param input 输入数据 (明文或密文)
 * @param inputLen 输入长度
 * @param output 输出缓冲区
 * @param outputLenPtr 输出: 实际输出长度
 * @return AES_ERR_NONE成功
 */
uint8 Aes_OfbEncrypt(Aes_ContextType* ctx,
                      const uint8* iv,
                      const uint8* input,
                      uint32 inputLen,
                      uint8* output,
                      uint32* outputLenPtr)
{
    uint8 feedback[AES_BLOCK_SIZE];
    uint8 keystream[AES_BLOCK_SIZE];
    uint32 i;

    if (ctx == NULL || iv == NULL || input == NULL || output == NULL || outputLenPtr == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (inputLen == 0) {
        *outputLenPtr = 0;
        return AES_ERR_NONE;
    }

    /* 初始化反馈寄存器 */
    memcpy(feedback, iv, AES_BLOCK_SIZE);

    for (i = 0; i < inputLen; i += AES_BLOCK_SIZE) {
        uint32 chunkLen = (inputLen - i > AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : (inputLen - i);

        /* 加密反馈寄存器得到密钥流 */
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, feedback, keystream);

        /* 密钥流异或输入 */
        for (uint32 j = 0; j < chunkLen; j++) {
            output[i + j] = input[i + j] ^ keystream[j];
        }

        /* 更新反馈寄存器 (密钥流本身) */
        memcpy(feedback, keystream, AES_BLOCK_SIZE);
    }

    *outputLenPtr = inputLen;

    /* 清除敏感数据 */
    memset(feedback, 0, AES_BLOCK_SIZE);
    memset(keystream, 0, AES_BLOCK_SIZE);

    return AES_ERR_NONE;
}

/**********************************************************************************************************************
 * 流式API实现
 *********************************************************************************************************************/

/**
 * @brief OFB流式 - 开始
 */
uint8 Aes_OfbEncryptStart(Aes_ContextType* ctx, const uint8* iv)
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
 * @brief OFB流式 - 更新
 */
uint8 Aes_OfbEncryptUpdate(Aes_ContextType* ctx,
                            const uint8* input,
                            uint32 inputLen,
                            uint8* output)
{
    uint32 i;

    if (ctx == NULL || input == NULL || output == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    for (i = 0; i < inputLen; i++) {
        /* 检查是否需要生成新的密钥流块 */
        if (ctx->tempLen == 0) {
            aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, ctx->iv, ctx->tempBlock);
            memcpy(ctx->iv, ctx->tempBlock, AES_BLOCK_SIZE);  /* 更新反馈 */
        }

        /* 异或 */
        output[i] = input[i] ^ ctx->tempBlock[ctx->tempLen];

        /* 更新密钥流索引 */
        ctx->tempLen = (ctx->tempLen + 1) % AES_BLOCK_SIZE;
    }

    return AES_ERR_NONE;
}

/**
 * @brief OFB流式 - 完成
 */
uint8 Aes_OfbEncryptFinish(Aes_ContextType* ctx)
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

/**********************************************************************************************************************
 * 预计算密钥流 (高性能场景)
 *********************************************************************************************************************/

/**
 * @brief 预计算OFB密钥流
 *
 * 用于需要高速连续加解密的场景，可以提前生成密钥流。
 *
 * @param ctx AES上下文指针
 * @param iv 初始化向量
 * @param keystream 密钥流输出缓冲区
 * @param numBlocks 需要的块数
 * @return AES_ERR_NONE成功
 */
uint8 Aes_OfbGenerateKeystream(Aes_ContextType* ctx,
                                const uint8* iv,
                                uint8* keystream,
                                uint32 numBlocks)
{
    uint8 feedback[AES_BLOCK_SIZE];
    uint32 i;

    if (ctx == NULL || iv == NULL || keystream == NULL) {
        return AES_ERR_INVALID_INPUT;
    }

    if (!ctx->initialized) {
        return AES_ERR_INVALID_INPUT;
    }

    if (numBlocks == 0) {
        return AES_ERR_NONE;
    }

    memcpy(feedback, iv, AES_BLOCK_SIZE);

    for (i = 0; i < numBlocks; i++) {
        aes_encrypt_block(ctx->roundKey[0], ctx->numRounds, feedback, &keystream[i * AES_BLOCK_SIZE]);
        memcpy(feedback, &keystream[i * AES_BLOCK_SIZE], AES_BLOCK_SIZE);
    }

    /* 清除敏感数据 */
    memset(feedback, 0, AES_BLOCK_SIZE);

    return AES_ERR_NONE;
}
