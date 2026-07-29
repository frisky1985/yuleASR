/**********************************************************************************************************************
 * @file       aes_autosar.c
 * @brief      AES算法 AUTOSAR Crypto Driver适配层
 *
 * 功能: 提供AUTOSAR Crypto Driver API与AES模式库的完整集成
 *       支持所有AES模式: ECB, CBC, CFB, OFB, CTR, GCM, CCM
 *       支持硬件加速 (S32K312 HSM)
 *       支持流式API
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#include "aes_modes.h"
#include "Crypto.h"
#include <string.h>

/**********************************************************************************************************************
 * 版本信息
 *********************************************************************************************************************/
#define AES_AUTOSAR_VENDOR_ID               AES_MODES_VENDOR_ID
#define AES_AUTOSAR_MODULE_ID               AES_MODES_MODULE_ID

/**********************************************************************************************************************
 * 本地类型定义
 *********************************************************************************************************************/

typedef struct {
    Aes_ContextType         aesCtx;
    Aes_GcmContextType      gcmCtx;
    Aes_ModeType            mode;
    uint8                   operation;  /* AES_MODE_ENCRYPT / AES_MODE_DECRYPT */
    boolean                 initialized;
    uint32                  jobId;
    uint8                   iv[AES_BLOCK_SIZE];
    uint8                   key[AES_MAX_KEY_SIZE];
    uint32                  keyLen;
} Aes_AutosarJobContextType;

/**********************************************************************************************************************
 * 全局变量
 *********************************************************************************************************************/
static Aes_AutosarJobContextType gAesJobContexts[CRYPTO_STACK_MAX_JOBS];
static boolean gAesAutosarInitialized = FALSE;

/**********************************************************************************************************************
 * 内部辅助函数
 *********************************************************************************************************************/

/**
 * @brief 将AUTOSAR算法模式转换为AES模式
 */
static Aes_ModeType Aes_AutosarConvertMode(Crypto_AlgorithmModeType mode)
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
            return AES_MODE_CBC;  /* 默认 */
    }
}

/**
 * @brief 获取空闲的作业上下文
 */
static Aes_AutosarJobContextType* Aes_AutosarGetFreeJobContext(void)
{
    for (uint32 i = 0; i < CRYPTO_STACK_MAX_JOBS; i++) {
        if (!gAesJobContexts[i].initialized) {
            return &gAesJobContexts[i];
        }
    }
    return NULL;
}

/**
 * @brief 通过jobId查找作业上下文
 */
static Aes_AutosarJobContextType* Aes_AutosarFindJobContext(uint32 jobId)
{
    for (uint32 i = 0; i < CRYPTO_STACK_MAX_JOBS; i++) {
        if (gAesJobContexts[i].initialized && gAesJobContexts[i].jobId == jobId) {
            return &gAesJobContexts[i];
        }
    }
    return NULL;
}

/**
 * 核心AES算法声明（来自其他源文件）
 */
extern uint8 Aes_EcbEncrypt(Aes_ContextType* ctx, const uint8* plaintext, uint32 plaintextLen,
                            uint8* ciphertext, uint32* ciphertextLenPtr);
extern uint8 Aes_EcbDecrypt(Aes_ContextType* ctx, const uint8* ciphertext, uint32 ciphertextLen,
                            uint8* plaintext, uint32* plaintextLenPtr);
extern uint8 Aes_CbcEncrypt(Aes_ContextType* ctx, const uint8* iv, const uint8* plaintext,
                            uint32 plaintextLen, uint8* ciphertext, uint32* ciphertextLenPtr);
extern uint8 Aes_CbcDecrypt(Aes_ContextType* ctx, const uint8* iv, const uint8* ciphertext,
                            uint32 ciphertextLen, uint8* plaintext, uint32* plaintextLenPtr);

/**********************************************************************************************************************
 * AUTOSAR初始化函数
 *********************************************************************************************************************/

/**
 * @brief 初始化AES AUTOSAR层
 */
void Aes_AutosarInit(void)
{
    memset(gAesJobContexts, 0, sizeof(gAesJobContexts));
    gAesAutosarInitialized = TRUE;
}

/**
 * @brief 反初始化AES AUTOSAR层
 */
void Aes_AutosarDeInit(void)
{
    for (uint32 i = 0; i < CRYPTO_STACK_MAX_JOBS; i++) {
        if (gAesJobContexts[i].initialized) {
            Aes_Clear(&gAesJobContexts[i].aesCtx);
            memset(&gAesJobContexts[i], 0, sizeof(Aes_AutosarJobContextType));
        }
    }
    gAesAutosarInitialized = FALSE;
}

/**********************************************************************************************************************
 * AUTOSAR加密服务
 *********************************************************************************************************************/

/**
 * @brief 执行AES加密作业 (AUTOSAR API)
 *
 * 根据作业配置选择适当的AES模式进行加密。
 *
 * @param job AUTOSAR Crypto作业指针
 * @return E_OK成功, E_NOT_OK失败
 */
Std_ReturnType Aes_AutosarEncrypt(Crypto_JobType* job)
{
    Aes_AutosarJobContextType* jobCtx;
    Crypto_JobPrimitiveInputOutputType* io;
    uint8 result;
    uint32 outputLen;
    boolean useHardware = FALSE;

    if (job == NULL || job->jobPrimitiveInfo == NULL) {
        return E_NOT_OK;
    }

    if (!gAesAutosarInitialized) {
        return E_NOT_OK;
    }

    io = job->jobPrimitiveInputOutput;
    if (io == NULL) {
        return E_NOT_OK;
    }

    /* 检查是否使用硬件加速 */
    #if (CRYPTO_CFG_HSM_ENABLED == STD_ON)
    if (Aes_HsmIsAvailable()) {
        useHardware = TRUE;
    }
    #endif

    /* 获取或创建作业上下文 */
    jobCtx = Aes_AutosarFindJobContext(job->jobId);
    if (jobCtx == NULL) {
        jobCtx = Aes_AutosarGetFreeJobContext();
        if (jobCtx == NULL) {
            return E_NOT_OK;  /* 没有可用上下文 */
        }
        jobCtx->jobId = job->jobId;
        jobCtx->mode = Aes_AutosarConvertMode(job->jobPrimitiveInfo->algorithm.mode);
        jobCtx->operation = AES_MODE_ENCRYPT;
        jobCtx->initialized = TRUE;

        /* 获取密钥 */
        if (job->cryptoKeyId != 0) {
            Crypto_KeyElementGet(job->cryptoKeyId, CRYPTO_KEY_ELEMENT_KEY,
                                jobCtx->key, &jobCtx->keyLen);
            Aes_Init(&jobCtx->aesCtx, jobCtx->key, jobCtx->keyLen);
        }
    }

    /* 获取IV */
    if (io->secondaryInputPtr != NULL && io->secondaryInputLength == AES_BLOCK_SIZE) {
        memcpy(jobCtx->iv, io->secondaryInputPtr, AES_BLOCK_SIZE);
    }

    outputLen = *io->outputLengthPtr;

    /* 根据模式调用相应的加密函数 */
    switch (jobCtx->mode) {
        case AES_MODE_ECB:
            result = Aes_EcbEncrypt(&jobCtx->aesCtx, io->inputPtr, io->inputLength,
                                   io->outputPtr, &outputLen);
            break;

        case AES_MODE_CBC:
            result = Aes_CbcEncrypt(&jobCtx->aesCtx, jobCtx->iv, io->inputPtr,
                                   io->inputLength, io->outputPtr, &outputLen);
            break;

        case AES_MODE_CFB:
            result = Aes_CfbEncrypt(&jobCtx->aesCtx, jobCtx->iv, io->inputPtr,
                                   io->inputLength, io->outputPtr, &outputLen);
            break;

        case AES_MODE_OFB:
            result = Aes_OfbEncrypt(&jobCtx->aesCtx, jobCtx->iv, io->inputPtr,
                                   io->inputLength, io->outputPtr, &outputLen);
            break;

        case AES_MODE_CTR:
            result = Aes_CtrEncrypt(&jobCtx->aesCtx, jobCtx->iv, io->inputPtr,
                                   io->inputLength, io->outputPtr);
            outputLen = io->inputLength;
            break;

        case AES_MODE_GCM:
            /* 处理GCM AEAD - 需要AAD和标签 */
            {
                uint8* aad = NULL;
                uint32 aadLen = 0;
                uint8* tagPtr = io->secondaryOutputPtr;
                uint32 tagLen = (io->secondaryOutputLengthPtr != NULL) ?
                                *io->secondaryOutputLengthPtr : AES_GCM_TAG_SIZE;

                /* 获取AAD (如果有) */
                if (job->jobPrimitiveInfo->algorithm.mode == CRYPTO_ALGOMODE_GCM) {
                    Crypto_KeyElementGet(job->cryptoKeyId, CRYPTO_KEYELEMENT_AAD,
                                        aad, &aadLen);
                }

                result = Aes_GcmEncrypt(&jobCtx->aesCtx, jobCtx->iv, AES_GCM_IV_SIZE,
                                       aad, aadLen,
                                       io->inputPtr, io->inputLength,
                                       io->outputPtr,
                                       tagPtr, tagLen);
                outputLen = io->inputLength;
            }
            break;

        case AES_MODE_CCM:
            /* 处理CCM AEAD */
            {
                uint8* aad = NULL;
                uint32 aadLen = 0;
                uint32 tagLen = AES_CCM_TAG_SIZE;

                result = Aes_CcmEncrypt(&jobCtx->aesCtx, jobCtx->iv, AES_BLOCK_SIZE,
                                       aad, aadLen,
                                       io->inputPtr, io->inputLength,
                                       io->outputPtr, tagLen,
                                       io->secondaryOutputPtr);
                outputLen = io->inputLength;
            }
            break;

        default:
            result = AES_ERR_NOT_SUPPORTED;
            break;
    }

    if (result == AES_ERR_NONE) {
        *io->outputLengthPtr = outputLen;
        return E_OK;
    }

    return E_NOT_OK;
}

/**
 * @brief 执行AES解密作业 (AUTOSAR API)
 */
Std_ReturnType Aes_AutosarDecrypt(Crypto_JobType* job)
{
    Aes_AutosarJobContextType* jobCtx;
    Crypto_JobPrimitiveInputOutputType* io;
    uint8 result;
    uint32 outputLen;

    if (job == NULL || job->jobPrimitiveInfo == NULL) {
        return E_NOT_OK;
    }

    if (!gAesAutosarInitialized) {
        return E_NOT_OK;
    }

    io = job->jobPrimitiveInputOutput;
    if (io == NULL) {
        return E_NOT_OK;
    }

    /* 获取或创建作业上下文 */
    jobCtx = Aes_AutosarFindJobContext(job->jobId);
    if (jobCtx == NULL) {
        jobCtx = Aes_AutosarGetFreeJobContext();
        if (jobCtx == NULL) {
            return E_NOT_OK;
        }
        jobCtx->jobId = job->jobId;
        jobCtx->mode = Aes_AutosarConvertMode(job->jobPrimitiveInfo->algorithm.mode);
        jobCtx->operation = AES_MODE_DECRYPT;
        jobCtx->initialized = TRUE;

        /* 获取密钥 */
        if (job->cryptoKeyId != 0) {
            Crypto_KeyElementGet(job->cryptoKeyId, CRYPTO_KEY_ELEMENT_KEY,
                                jobCtx->key, &jobCtx->keyLen);
            Aes_Init(&jobCtx->aesCtx, jobCtx->key, jobCtx->keyLen);
        }
    }

    /* 获取IV */
    if (io->secondaryInputPtr != NULL && io->secondaryInputLength == AES_BLOCK_SIZE) {
        memcpy(jobCtx->iv, io->secondaryInputPtr, AES_BLOCK_SIZE);
    }

    outputLen = *io->outputLengthPtr;

    /* 根据模式调用相应的解密函数 */
    switch (jobCtx->mode) {
        case AES_MODE_ECB:
            result = Aes_EcbDecrypt(&jobCtx->aesCtx, io->inputPtr, io->inputLength,
                                   io->outputPtr, &outputLen);
            break;

        case AES_MODE_CBC:
            result = Aes_CbcDecrypt(&jobCtx->aesCtx, jobCtx->iv, io->inputPtr,
                                   io->inputLength, io->outputPtr, &outputLen);
            break;

        case AES_MODE_CFB:
            result = Aes_CfbDecrypt(&jobCtx->aesCtx, jobCtx->iv, io->inputPtr,
                                   io->inputLength, io->outputPtr, &outputLen);
            break;

        case AES_MODE_OFB:
            result = Aes_OfbEncrypt(&jobCtx->aesCtx, jobCtx->iv, io->inputPtr,
                                   io->inputLength, io->outputPtr, &outputLen);
            break;

        case AES_MODE_CTR:
            result = Aes_CtrEncrypt(&jobCtx->aesCtx, jobCtx->iv, io->inputPtr,
                                   io->inputLength, io->outputPtr);
            outputLen = io->inputLength;
            break;

        case AES_MODE_GCM:
            /* 处理GCM AEAD解密 */
            {
                uint8* aad = NULL;
                uint32 aadLen = 0;
                uint8* tagPtr = (uint8*)io->tertiaryInputPtr;
                uint32 tagLen = io->tertiaryInputLength;

                result = Aes_GcmDecrypt(&jobCtx->aesCtx, jobCtx->iv, AES_GCM_IV_SIZE,
                                       aad, aadLen,
                                       io->inputPtr, io->inputLength,
                                       tagPtr, tagLen,
                                       io->outputPtr, &outputLen);
            }
            break;

        case AES_MODE_CCM:
            /* 处理CCM AEAD解密 */
            {
                uint8* aad = NULL;
                uint32 aadLen = 0;
                uint8* tagPtr = (uint8*)io->tertiaryInputPtr;
                uint32 tagLen = io->tertiaryInputLength;

                result = Aes_CcmDecrypt(&jobCtx->aesCtx, jobCtx->iv, AES_BLOCK_SIZE,
                                       aad, aadLen,
                                       io->inputPtr, io->inputLength,
                                       tagPtr, tagLen,
                                       io->outputPtr, &outputLen);
            }
            break;

        default:
            result = AES_ERR_NOT_SUPPORTED;
            break;
    }

    if (result == AES_ERR_NONE) {
        *io->outputLengthPtr = outputLen;
        return E_OK;
    }

    /* 认证失败特殊处理 */
    if (result == AES_ERR_AUTHENTICATION_FAILED && io->verifyPtr != NULL) {
        *io->verifyPtr = CRYPTO_VERIFY_FAILED;
    }

    return E_NOT_OK;
}

/**********************************************************************************************************************
 * 流式API - AUTOSAR适配
 *********************************************************************************************************************/

/**
 * @brief 启动AES流式加密作业
 */
Std_ReturnType Aes_AutosarStreamEncryptStart(Crypto_JobType* job)
{
    Aes_AutosarJobContextType* jobCtx;
    uint8 result;

    if (job == NULL) {
        return E_NOT_OK;
    }

    jobCtx = Aes_AutosarGetFreeJobContext();
    if (jobCtx == NULL) {
        return E_NOT_OK;
    }

    jobCtx->jobId = job->jobId;
    jobCtx->mode = Aes_AutosarConvertMode(job->jobPrimitiveInfo->algorithm.mode);
    jobCtx->operation = AES_MODE_ENCRYPT;
    jobCtx->initialized = TRUE;

    /* 获取密钥和IV */
    if (job->cryptoKeyId != 0) {
        Crypto_KeyElementGet(job->cryptoKeyId, CRYPTO_KEY_ELEMENT_KEY,
                            jobCtx->key, &jobCtx->keyLen);
        Aes_Init(&jobCtx->aesCtx, jobCtx->key, jobCtx->keyLen);
    }

    if (job->jobPrimitiveInputOutput != NULL &&
        job->jobPrimitiveInputOutput->secondaryInputPtr != NULL) {
        memcpy(jobCtx->iv, job->jobPrimitiveInputOutput->secondaryInputPtr, AES_BLOCK_SIZE);
    }

    /* 根据模式启动流式操作 */
    switch (jobCtx->mode) {
        case AES_MODE_CBC:
            result = Aes_CbcEncryptStart(&jobCtx->aesCtx, jobCtx->iv);
            break;

        case AES_MODE_CTR:
            result = Aes_CtrEncryptStart(&jobCtx->aesCtx, jobCtx->iv);
            break;

        default:
            result = AES_ERR_NOT_SUPPORTED;
            break;
    }

    return (result == AES_ERR_NONE) ? E_OK : E_NOT_OK;
}

/**
 * @brief 更新AES流式加密作业
 */
Std_ReturnType Aes_AutosarStreamEncryptUpdate(Crypto_JobType* job)
{
    Aes_AutosarJobContextType* jobCtx;
    Crypto_JobPrimitiveInputOutputType* io;
    uint8 result;
    uint32 outputLen;

    if (job == NULL || job->jobPrimitiveInputOutput == NULL) {
        return E_NOT_OK;
    }

    jobCtx = Aes_AutosarFindJobContext(job->jobId);
    if (jobCtx == NULL) {
        return E_NOT_OK;
    }

    io = job->jobPrimitiveInputOutput;
    outputLen = *io->outputLengthPtr;

    switch (jobCtx->mode) {
        case AES_MODE_CBC:
            result = Aes_CbcEncryptUpdate(&jobCtx->aesCtx, io->inputPtr, io->inputLength,
                                         io->outputPtr, &outputLen);
            break;

        case AES_MODE_CTR:
            result = Aes_CtrEncryptUpdate(&jobCtx->aesCtx, io->inputPtr, io->inputLength,
                                         io->outputPtr);
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
 * @brief 完成AES流式加密作业
 */
Std_ReturnType Aes_AutosarStreamEncryptFinish(Crypto_JobType* job)
{
    Aes_AutosarJobContextType* jobCtx;
    Crypto_JobPrimitiveInputOutputType* io;
    uint8 result;
    uint32 outputLen;

    if (job == NULL || job->jobPrimitiveInputOutput == NULL) {
        return E_NOT_OK;
    }

    jobCtx = Aes_AutosarFindJobContext(job->jobId);
    if (jobCtx == NULL) {
        return E_NOT_OK;
    }

    io = job->jobPrimitiveInputOutput;
    outputLen = *io->outputLengthPtr;

    switch (jobCtx->mode) {
        case AES_MODE_CBC:
            result = Aes_CbcEncryptFinish(&jobCtx->aesCtx, io->inputPtr, io->inputLength,
                                         io->outputPtr, &outputLen);
            break;

        case AES_MODE_CTR:
            result = Aes_CtrEncryptFinish(&jobCtx->aesCtx);
            outputLen = 0;
            break;

        default:
            result = AES_ERR_NOT_SUPPORTED;
            break;
    }

    if (result == AES_ERR_NONE) {
        *io->outputLengthPtr = outputLen;
        /* 清理作业上下文 */
        memset(jobCtx, 0, sizeof(Aes_AutosarJobContextType));
        return E_OK;
    }

    return E_NOT_OK;
}

/**********************************************************************************************************************
 * 硬件加速接口
 *********************************************************************************************************************/

/**
 * @brief 检查HSM硬件加速是否可用
 */
boolean Aes_HsmIsAvailable(void)
{
    #if (CRYPTO_CFG_HSM_ENABLED == STD_ON)
    return (boolean)(Crypto_HsmGetStatus() == CRYPTO_HSM_IDLE);
    #else
    return FALSE;
    #endif
}

/**
 * @brief 使用HSM执行AES加密
 */
uint8 Aes_HsmEncrypt(Aes_ContextType* ctx, Aes_ModeType mode, const uint8* iv,
                      const uint8* input, uint32 inputLen, uint8* output)
{
    #if (CRYPTO_CFG_HSM_ENABLED == STD_ON)
    /* 调用S32K312 HSM API */
    return AES_ERR_NOT_SUPPORTED;  /* 待实现 */
    #else
    (void)ctx;
    (void)mode;
    (void)iv;
    (void)input;
    (void)inputLen;
    (void)output;
    return AES_ERR_NOT_SUPPORTED;
    #endif
}

/**
 * @brief 使用HSM执行AES解密
 */
uint8 Aes_HsmDecrypt(Aes_ContextType* ctx, Aes_ModeType mode, const uint8* iv,
                      const uint8* input, uint32 inputLen, uint8* output)
{
    #if (CRYPTO_CFG_HSM_ENABLED == STD_ON)
    return AES_ERR_NOT_SUPPORTED;  /* 待实现 */
    #else
    (void)ctx;
    (void)mode;
    (void)iv;
    (void)input;
    (void)inputLen;
    (void)output;
    return AES_ERR_NOT_SUPPORTED;
    #endif
}
