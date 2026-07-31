/**********************************************************************************************************************
 * @file       Crypto_Aes.c
 * @brief      Crypto Driver AES模式处理模块
 *
 * 功能: 集成AES模式库到Crypto Driver
 *       支持所有AES模式: ECB, CBC, CFB, OFB, CTR, GCM, CCM
 *       支持硬件加速 (S32K312 HSM) 和软件回退
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#include "Crypto.h"
#include "aes_modes.h"
#include <string.h>

/**********************************************************************************************************************
 * 内部类型定义
 *********************************************************************************************************************/
Std_ReturnType Crypto_AesStreamFinish(Crypto_JobType* job);
Std_ReturnType Crypto_AesStreamUpdate(Crypto_JobType* job);
Std_ReturnType Crypto_AesStreamStart(Crypto_JobType* job);
Std_ReturnType Crypto_AesProcessDecrypt(Crypto_JobType* job);
Std_ReturnType Crypto_AesProcessEncrypt(Crypto_JobType* job);
boolean Crypto_AesIsModeSupported(Crypto_AlgorithmModeType mode);
void Crypto_AesDeInit(void);
void Crypto_AesInit(void);
STATIC Std_ReturnType Crypto_AesGetIvElement(Crypto_KeyIdType keyId,                                              uint8* ivBuffer,                                              uint32* ivLength);
STATIC Std_ReturnType Crypto_AesGetKeyElement(Crypto_KeyIdType keyId,                                               uint8* keyBuffer,                                               uint32* keyLength);
typedef struct {
    Aes_ContextType     aesCtx;
    Aes_GcmContextType  gcmCtx;
    Aes_ModeType        mode;
    boolean             inUse;
    uint32              jobId;
} Crypto_AesJobContextType;

/**********************************************************************************************************************
 * 全局变量
 *********************************************************************************************************************/
#define CRYPTO_MAX_AES_JOBS     8U

STATIC Crypto_AesJobContextType gCryptoAesContexts[CRYPTO_MAX_AES_JOBS];
STATIC boolean gCryptoAesInitialized = FALSE;

/**********************************************************************************************************************
 * 内部辅助函数
 *********************************************************************************************************************/

/**
 * @brief 获取空闲的AES作业上下文
 */
STATIC Crypto_AesJobContextType* Crypto_AesGetFreeContext(void)
{
    uint32 i;
    for (i = 0U; i < CRYPTO_MAX_AES_JOBS; i++) {
        if (!gCryptoAesContexts[i].inUse) {
            return &gCryptoAesContexts[i];
        }
    }
    return NULL_PTR;
}

/**
 * @brief 通过jobId查找AES作业上下文
 */
STATIC Crypto_AesJobContextType* Crypto_AesFindContext(uint32 jobId)
{
    uint32 i;
    for (i = 0U; i < CRYPTO_MAX_AES_JOBS; i++) {
        if (gCryptoAesContexts[i].inUse && gCryptoAesContexts[i].jobId == jobId) {
            return &gCryptoAesContexts[i];
        }
    }
    return NULL_PTR;
}

/**
 * @brief 将AUTOSAR算法模式转换为AES模式
 */
STATIC Aes_ModeType Crypto_AesConvertMode(Crypto_AlgorithmModeType mode)
{
    switch (mode) {
        case CRYPTO_ALGOMODE_ECB:
            return AES_MODE_ECB;
        case CRYPTO_ALGOMODE_CBC:
            return AES_MODE_CBC;
        case CRYPTO_ALGOMODE_CFB:
            return AES_MODE_CFB;
        case CRYPTO_ALGOMODE_OFB:
            return AES_MODE_OFB;
        case CRYPTO_ALGOMODE_CTR:
            return AES_MODE_CTR;
        case CRYPTO_ALGOMODE_GCM:
            return AES_MODE_GCM;
        case CRYPTO_ALGOMODE_CCM:
            return AES_MODE_CCM;
        default:
            return AES_MODE_CBC;
    }
}

/**
 * @brief 获取密钥元素
 */
STATIC Std_ReturnType Crypto_AesGetKeyElement(Crypto_KeyIdType keyId,
                                               uint8* keyBuffer,
                                               uint32* keyLength)
{
    return Crypto_KeyElementGet(keyId, CRYPTO_KEY_ELEMENT_KEY, keyBuffer, keyLength);
}

/**
 * @brief 获取IV元素
 */
STATIC Std_ReturnType Crypto_AesGetIvElement(Crypto_KeyIdType keyId,
                                              uint8* ivBuffer,
                                              uint32* ivLength)
{
    return Crypto_KeyElementGet(keyId, CRYPTO_KEY_ELEMENT_IV, ivBuffer, ivLength);
}

/**********************************************************************************************************************
 * 外部API实现
 *********************************************************************************************************************/

/**
 * @brief 初始化AES模块
 */
void Crypto_AesInit(void)
{
    uint32 i;
    for (i = 0U; i < CRYPTO_MAX_AES_JOBS; i++) {
        gCryptoAesContexts[i].inUse = FALSE;
    }
    gCryptoAesInitialized = TRUE;
}

/**
 * @brief 反初始化AES模块
 */
void Crypto_AesDeInit(void)
{
    uint32 i;
    for (i = 0U; i < CRYPTO_MAX_AES_JOBS; i++) {
        if (gCryptoAesContexts[i].inUse) {
            Aes_Clear(&gCryptoAesContexts[i].aesCtx);
            gCryptoAesContexts[i].inUse = FALSE;
        }
    }
    gCryptoAesInitialized = FALSE;
}

/**
 * @brief 检查是否支持指定的AES模式
 */
boolean Crypto_AesIsModeSupported(Crypto_AlgorithmModeType mode)
{
    switch (mode) {
        case CRYPTO_ALGOMODE_ECB:
        case CRYPTO_ALGOMODE_CBC:
        case CRYPTO_ALGOMODE_CFB:
        case CRYPTO_ALGOMODE_OFB:
        case CRYPTO_ALGOMODE_CTR:
        case CRYPTO_ALGOMODE_GCM:
        case CRYPTO_ALGOMODE_CCM:
            return TRUE;
        default:
            return FALSE;
    }
}

/**
 * @brief 处理AES加密作业
 */
Std_ReturnType Crypto_AesProcessEncrypt(Crypto_JobType* job)
{
    Crypto_AesJobContextType* ctx;
    Crypto_JobPrimitiveInputOutputType* io;
    uint8 key[AES_MAX_KEY_SIZE];
    uint32 keyLen = AES_MAX_KEY_SIZE;
    uint8 iv[AES_BLOCK_SIZE];
    uint32 ivLen = AES_BLOCK_SIZE;
    uint8 result;
    uint32 outputLen;
    boolean isNewJob = FALSE;

    if ((job == NULL_PTR) || (job->jobPrimitiveInputOutput == NULL_PTR)) {
        return E_NOT_OK;
    }

    if (gCryptoAesInitialized == 0U) {
        return E_NOT_OK;
    }

    io = job->jobPrimitiveInputOutput;

    /* 检查输入输出参数 */
    if (((const uint8*)io->inputPtr == NULL_PTR) || ((uint8*)io->outputPtr == NULL_PTR) ||
        (io->outputLengthPtr == NULL_PTR)) {
        return E_NOT_OK;
    }

    /* 获取或创建上下文 */
    ctx = Crypto_AesFindContext(job->jobId);
    if (ctx == NULL_PTR) {
        ctx = Crypto_AesGetFreeContext();
        if (ctx == NULL_PTR) {
            return E_NOT_OK;
        }
        ctx->jobId = job->jobId;
        ctx->mode = Crypto_AesConvertMode(job->jobPrimitiveInfo->algorithm->mode);
        ctx->inUse = TRUE;
        isNewJob = TRUE;

        /* 获取并初始化密钥 */
        if (Crypto_AesGetKeyElement(job->cryptoKeyId, key, &keyLen) == E_OK) {
            result = Aes_Init(&ctx->aesCtx, key, keyLen);
            if (result != AES_ERR_NONE) {
                ctx->inUse = FALSE;
                return E_NOT_OK;
            }
        } else {
            ctx->inUse = FALSE;
            return E_NOT_OK;
        }
    }

    /* 获取IV */
    if (isNewJob) {
        if (io->secondaryInputPtr != NULL_PTR && io->secondaryInputLength == AES_BLOCK_SIZE) {
            (void)memcpy(iv, io->secondaryInputPtr, AES_BLOCK_SIZE);
        } else if (Crypto_AesGetIvElement(job->cryptoKeyId, iv, &ivLen) == E_OK) {
            /* IV从密钥元素获取 */
        } else {
            (void)memset(iv, 0, AES_BLOCK_SIZE);  /* 零IV作为后备 */
        }
    }

    outputLen = *io->outputLengthPtr;

    /* 根据操作模式处理 */
    switch (io->mode) {
        case CRYPTO_OPERATIONMODE_START:
            /* 已在上面初始化 */
            *io->outputLengthPtr = 0U;
            return E_OK;

        case CRYPTO_OPERATIONMODE_UPDATE:
        case CRYPTO_OPERATIONMODE_STREAMSTART:
            /* 流式更新 - 处理完整块 */
            switch (ctx->mode) {
                case AES_MODE_CBC:
                    if (isNewJob) {
                        result = Aes_CbcEncryptStart(&ctx->aesCtx, iv);
                    }
                    if ((io->inputLength % AES_BLOCK_SIZE) == 0U) {
                        result = Aes_CbcEncryptUpdate(&ctx->aesCtx, (const uint8*)io->inputPtr,
                                                      io->inputLength,
                                                      (uint8*)io->outputPtr, &outputLen);
                    }
                    break;

                case AES_MODE_CTR:
                    if (isNewJob) {
                        result = Aes_CtrEncryptStart(&ctx->aesCtx, iv);
                    }
result = Aes_CtrEncryptUpdate(&ctx->aesCtx, (const uint8*)io->inputPtr,
                                                  io->inputLength, (uint8*)io->outputPtr);
                    outputLen = io->inputLength;
                    break;

                default:
                    /* 其他模式不支持流式UPDATE模式 */
                    return E_NOT_OK;
            }
            break;

        case CRYPTO_OPERATIONMODE_FINISH:
        case CRYPTO_OPERATIONMODE_SINGLECALL:
            /* 单次调用或完成 */
            switch (ctx->mode) {
                case AES_MODE_ECB:
                    result = Aes_EcbEncrypt(&ctx->aesCtx, (const uint8*)io->inputPtr, io->inputLength,
                                           (uint8*)io->outputPtr, &outputLen);
                    break;

                case AES_MODE_CBC:
                    result = Aes_CbcEncrypt(&ctx->aesCtx, iv, (const uint8*)io->inputPtr,
                                           io->inputLength, (uint8*)io->outputPtr, &outputLen);
                    break;

                case AES_MODE_CFB:
                    result = Aes_CfbEncrypt(&ctx->aesCtx, iv, (const uint8*)io->inputPtr,
                                           io->inputLength, (uint8*)io->outputPtr, &outputLen);
                    break;

                case AES_MODE_OFB:
                    result = Aes_OfbEncrypt(&ctx->aesCtx, iv, (const uint8*)io->inputPtr,
                                           io->inputLength, (uint8*)io->outputPtr, &outputLen);
                    break;

                case AES_MODE_CTR:
                    result = Aes_CtrEncrypt(&ctx->aesCtx, iv, (const uint8*)io->inputPtr,
                                           io->inputLength, (uint8*)io->outputPtr);
                    outputLen = io->inputLength;
                    break;

                case AES_MODE_GCM:
                    /* AEAD加密 */
                    {
                        uint32 tagLen = AES_GCM_TAG_SIZE;
                        if (io->secondaryOutputLengthPtr != NULL_PTR) {
                            tagLen = *io->secondaryOutputLengthPtr;
                        }
                        result = Aes_GcmEncrypt(&ctx->aesCtx, iv, AES_GCM_IV_SIZE,
                                               (const uint8*)io->tertiaryInputPtr, io->tertiaryInputLength,
                                               (const uint8*)io->inputPtr, io->inputLength,
                                               (uint8*)io->outputPtr,
                                               (uint8*)io->secondaryOutputPtr, tagLen);
                        outputLen = io->inputLength;
                        if (io->secondaryOutputLengthPtr != NULL_PTR) {
                            *io->secondaryOutputLengthPtr = tagLen;
                        }
                    }
                    break;

                case AES_MODE_CCM:
                    /* AEAD加密 */
                    {
                        uint32 tagLen = AES_CCM_TAG_SIZE;
                        result = Aes_CcmEncrypt(&ctx->aesCtx, iv, AES_BLOCK_SIZE,
                                               (const uint8*)io->tertiaryInputPtr, io->tertiaryInputLength,
                                               (const uint8*)io->inputPtr, io->inputLength,
                                               (uint8*)io->outputPtr, tagLen,
                                               (uint8*)io->secondaryOutputPtr);
                        outputLen = io->inputLength;
                        if (io->secondaryOutputLengthPtr != NULL_PTR) {
                            *io->secondaryOutputLengthPtr = tagLen;
                        }
                    }
                    break;

                default:
                    return E_NOT_OK;
            }

            /* 清理上下文 */
            Aes_Clear(&ctx->aesCtx);
            ctx->inUse = FALSE;
            break;

        default:
            return E_NOT_OK;
    }

    if (result == AES_ERR_NONE) {
        *io->outputLengthPtr = outputLen;
        return E_OK;
    }

    return E_NOT_OK;
}

/**
 * @brief 处理AES解密作业
 */
Std_ReturnType Crypto_AesProcessDecrypt(Crypto_JobType* job)
{
    Crypto_AesJobContextType* ctx;
    Crypto_JobPrimitiveInputOutputType* io;
    uint8 key[AES_MAX_KEY_SIZE];
    uint32 keyLen = AES_MAX_KEY_SIZE;
    uint8 iv[AES_BLOCK_SIZE];
    uint32 ivLen = AES_BLOCK_SIZE;
    uint8 result;
    uint32 outputLen;
    boolean isNewJob = FALSE;

    if ((job == NULL_PTR) || (job->jobPrimitiveInputOutput == NULL_PTR)) {
        return E_NOT_OK;
    }

    if (gCryptoAesInitialized == 0U) {
        return E_NOT_OK;
    }

    io = job->jobPrimitiveInputOutput;

    /* 检查输入输出参数 */
    if (((const uint8*)io->inputPtr == NULL_PTR) || ((uint8*)io->outputPtr == NULL_PTR) ||
        (io->outputLengthPtr == NULL_PTR)) {
        return E_NOT_OK;
    }

    /* 获取或创建上下文 */
    ctx = Crypto_AesFindContext(job->jobId);
    if (ctx == NULL_PTR) {
        ctx = Crypto_AesGetFreeContext();
        if (ctx == NULL_PTR) {
            return E_NOT_OK;
        }
        ctx->jobId = job->jobId;
        ctx->mode = Crypto_AesConvertMode(job->jobPrimitiveInfo->algorithm->mode);
        ctx->inUse = TRUE;
        isNewJob = TRUE;

        /* 获取并初始化密钥 */
        if (Crypto_AesGetKeyElement(job->cryptoKeyId, key, &keyLen) == E_OK) {
            result = Aes_Init(&ctx->aesCtx, key, keyLen);
            if (result != AES_ERR_NONE) {
                ctx->inUse = FALSE;
                return E_NOT_OK;
            }
        } else {
            ctx->inUse = FALSE;
            return E_NOT_OK;
        }
    }

    /* 获取IV */
    if (isNewJob) {
        if (io->secondaryInputPtr != NULL_PTR && io->secondaryInputLength == AES_BLOCK_SIZE) {
            (void)memcpy(iv, io->secondaryInputPtr, AES_BLOCK_SIZE);
        } else if (Crypto_AesGetIvElement(job->cryptoKeyId, iv, &ivLen) == E_OK) {
            /* IV从密钥元素获取 */
        } else {
            (void)memset(iv, 0, AES_BLOCK_SIZE);
        }
    }

    outputLen = *io->outputLengthPtr;

    /* 根据操作模式处理 */
    switch (io->mode) {
        case CRYPTO_OPERATIONMODE_START:
            *io->outputLengthPtr = 0U;
            return E_OK;

        case CRYPTO_OPERATIONMODE_UPDATE:
        case CRYPTO_OPERATIONMODE_STREAMSTART:
            /* 流式更新 */
            switch (ctx->mode) {
                case AES_MODE_CBC:
                    if (isNewJob) {
                        result = Aes_CbcDecryptStart(&ctx->aesCtx, iv);
                    }
                    if ((io->inputLength % AES_BLOCK_SIZE) == 0U) {
                        result = Aes_CbcDecryptUpdate(&ctx->aesCtx, (const uint8*)io->inputPtr,
                                                      io->inputLength,
                                                      (uint8*)io->outputPtr, &outputLen);
                    }
                    break;

                case AES_MODE_CTR:
                    if (isNewJob) {
                        result = Aes_CtrEncryptStart(&ctx->aesCtx, iv);
                    }
result = Aes_CtrEncryptUpdate(&ctx->aesCtx, (const uint8*)io->inputPtr,
                                                  io->inputLength, (uint8*)io->outputPtr);
                    outputLen = io->inputLength;
                    break;

                default:
                    return E_NOT_OK;
            }
            break;

        case CRYPTO_OPERATIONMODE_FINISH:
        case CRYPTO_OPERATIONMODE_SINGLECALL:
            /* 单次调用或完成 */
            switch (ctx->mode) {
                case AES_MODE_ECB:
                    result = Aes_EcbDecrypt(&ctx->aesCtx, (const uint8*)io->inputPtr, io->inputLength,
                                           (uint8*)io->outputPtr, &outputLen);
                    break;

                case AES_MODE_CBC:
                    result = Aes_CbcDecrypt(&ctx->aesCtx, iv, (const uint8*)io->inputPtr,
                                           io->inputLength, (uint8*)io->outputPtr, &outputLen);
                    break;

                case AES_MODE_CFB:
                    result = Aes_CfbDecrypt(&ctx->aesCtx, iv, (const uint8*)io->inputPtr,
                                           io->inputLength, (uint8*)io->outputPtr, &outputLen);
                    break;

                case AES_MODE_OFB:
                    result = Aes_OfbEncrypt(&ctx->aesCtx, iv, (const uint8*)io->inputPtr,
                                           io->inputLength, (uint8*)io->outputPtr, &outputLen);
                    break;

                case AES_MODE_CTR:
                    result = Aes_CtrEncrypt(&ctx->aesCtx, iv, (const uint8*)io->inputPtr,
                                           io->inputLength, (uint8*)io->outputPtr);
                    outputLen = io->inputLength;
                    break;

                case AES_MODE_GCM:
                    /* AEAD解密 */
                    {
                        uint32 tagLen = AES_GCM_TAG_SIZE;
                        if (io->tertiaryInputLength > 0U) {
                            tagLen = io->tertiaryInputLength;
                        }
                        result = Aes_GcmDecrypt(&ctx->aesCtx, iv, AES_GCM_IV_SIZE,
                                               (const uint8*)io->tertiaryInputPtr, io->tertiaryInputLength,
                                               (const uint8*)io->inputPtr, io->inputLength,
                                               (const uint8*)io->secondaryInputPtr, tagLen,
                                               (uint8*)io->outputPtr, &outputLen);

                        /* 验证结果 */
                        if (io->verifyPtr != NULL_PTR) {
                            *io->verifyPtr = (result == AES_ERR_NONE) ?
                                            CRYPTO_VERIFY_PASSED : CRYPTO_VERIFY_FAILED;
                        }
                    }
                    break;

                case AES_MODE_CCM:
                    /* AEAD解密 */
                    {
                        uint32 tagLen = AES_CCM_TAG_SIZE;
                        if (io->tertiaryInputLength > 0U) {
                            tagLen = io->tertiaryInputLength;
                        }
                        result = Aes_CcmDecrypt(&ctx->aesCtx, iv, AES_BLOCK_SIZE,
                                               (const uint8*)io->tertiaryInputPtr, io->tertiaryInputLength,
                                               (const uint8*)io->inputPtr, io->inputLength,
                                               (const uint8*)io->secondaryInputPtr, tagLen,
                                               (uint8*)io->outputPtr, &outputLen);

                        if (io->verifyPtr != NULL_PTR) {
                            *io->verifyPtr = (result == AES_ERR_NONE) ?
                                            CRYPTO_VERIFY_PASSED : CRYPTO_VERIFY_FAILED;
                        }
                    }
                    break;

                default:
                    return E_NOT_OK;
            }

            /* 清理上下文 */
            Aes_Clear(&ctx->aesCtx);
            ctx->inUse = FALSE;
            break;

        default:
            return E_NOT_OK;
    }

    if (result == AES_ERR_NONE) {
        *io->outputLengthPtr = outputLen;
        return E_OK;
    }

    return E_NOT_OK;
}

/**********************************************************************************************************************
 * 流式API实现
 *********************************************************************************************************************/

/**
 * @brief 启动AES流式操作
 */
Std_ReturnType Crypto_AesStreamStart(Crypto_JobType* job)
{
    Crypto_AesJobContextType* ctx;
    uint8 key[AES_MAX_KEY_SIZE];
    uint32 keyLen = AES_MAX_KEY_SIZE;
    uint8 iv[AES_BLOCK_SIZE];
    uint32 ivLen = AES_BLOCK_SIZE;
    uint8 result;
    Crypto_JobPrimitiveInputOutputType* io;

    if ((job == NULL_PTR) || (job->jobPrimitiveInputOutput == NULL_PTR)) {
        return E_NOT_OK;
    }

    io = job->jobPrimitiveInputOutput;

    ctx = Crypto_AesGetFreeContext();
    if (ctx == NULL_PTR) {
        return E_NOT_OK;
    }

    ctx->jobId = job->jobId;
    ctx->mode = Crypto_AesConvertMode(job->jobPrimitiveInfo->algorithm->mode);
    ctx->inUse = TRUE;

    /* 获取密钥 */
    if (Crypto_AesGetKeyElement(job->cryptoKeyId, key, &keyLen) != E_OK) {
        ctx->inUse = FALSE;
        return E_NOT_OK;
    }

    result = Aes_Init(&ctx->aesCtx, key, keyLen);
    if (result != AES_ERR_NONE) {
        ctx->inUse = FALSE;
        return E_NOT_OK;
    }

    /* 获取IV */
    if (io->secondaryInputPtr != NULL_PTR && io->secondaryInputLength == AES_BLOCK_SIZE) {
        (void)memcpy(iv, io->secondaryInputPtr, AES_BLOCK_SIZE);
    } else if (Crypto_AesGetIvElement(job->cryptoKeyId, iv, &ivLen) != E_OK) {
        (void)memset(iv, 0, AES_BLOCK_SIZE);
    }

    /* 根据模式启动流式操作 */
    switch (ctx->mode) {
        case AES_MODE_CBC:
            result = Aes_CbcEncryptStart(&ctx->aesCtx, iv);
            break;

        case AES_MODE_CTR:
            result = Aes_CtrEncryptStart(&ctx->aesCtx, iv);
            break;

        default:
            ctx->inUse = FALSE;
            return E_NOT_OK;
    }

    return (result == AES_ERR_NONE) ? E_OK : E_NOT_OK;
}

/**
 * @brief 更新AES流式操作
 */
Std_ReturnType Crypto_AesStreamUpdate(Crypto_JobType* job)
{
    Crypto_AesJobContextType* ctx;
    Crypto_JobPrimitiveInputOutputType* io;
    uint8 result;
    uint32 outputLen;

    if ((job == NULL_PTR) || (job->jobPrimitiveInputOutput == NULL_PTR)) {
        return E_NOT_OK;
    }

    ctx = Crypto_AesFindContext(job->jobId);
    if (ctx == NULL_PTR) {
        return E_NOT_OK;
    }

    io = job->jobPrimitiveInputOutput;
    outputLen = *io->outputLengthPtr;

    switch (ctx->mode) {
        case AES_MODE_CBC:
            result = Aes_CbcEncryptUpdate(&ctx->aesCtx, (const uint8*)io->inputPtr,
                                         io->inputLength,
                                         (uint8*)io->outputPtr, &outputLen);
            break;

        case AES_MODE_CTR:
            result = Aes_CtrEncryptUpdate(&ctx->aesCtx, (const uint8*)io->inputPtr,
                                         io->inputLength, (uint8*)io->outputPtr);
            outputLen = io->inputLength;
            break;

        default:
            return E_NOT_OK;
    }

    if (result == AES_ERR_NONE) {
        *io->outputLengthPtr = outputLen;
        return E_OK;
    }

    return E_NOT_OK;
}

/**
 * @brief 完成AES流式操作
 */
Std_ReturnType Crypto_AesStreamFinish(Crypto_JobType* job)
{
    Crypto_AesJobContextType* ctx;
    Crypto_JobPrimitiveInputOutputType* io;
    uint8 result;
    uint32 outputLen;

    if ((job == NULL_PTR) || (job->jobPrimitiveInputOutput == NULL_PTR)) {
        return E_NOT_OK;
    }

    ctx = Crypto_AesFindContext(job->jobId);
    if (ctx == NULL_PTR) {
        return E_NOT_OK;
    }

    io = job->jobPrimitiveInputOutput;
    outputLen = *io->outputLengthPtr;

    switch (ctx->mode) {
        case AES_MODE_CBC:
            result = Aes_CbcEncryptFinish(&ctx->aesCtx, (const uint8*)io->inputPtr,
                                         io->inputLength,
                                         (uint8*)io->outputPtr, &outputLen);
            break;

        case AES_MODE_CTR:
            result = Aes_CtrEncryptFinish(&ctx->aesCtx);
            outputLen = 0U;
            break;

        default:
            ctx->inUse = FALSE;
            return E_NOT_OK;
    }

    /* 清理上下文 */
    Aes_Clear(&ctx->aesCtx);
    ctx->inUse = FALSE;

    if (result == AES_ERR_NONE) {
        *io->outputLengthPtr = outputLen;
        return E_OK;
    }

    return E_NOT_OK;
}
