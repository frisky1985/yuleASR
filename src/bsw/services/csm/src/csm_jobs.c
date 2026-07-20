/*==================================================================================================
 * 作业/密码服务 API 实现
 * 自动拆分自 Csm.c
 *================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

Std_ReturnType Csm_Hash(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr)
{
    uint8 jobIdx;
    Std_ReturnType result;
    
    CSM_CHECK_INITIALIZED(CSM_API_HASH);
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return E_NOT_OK;
    }
    
    if ((mode & CSM_OPERATION_MODE_START) != 0U )
    {
        Csm_ResetJob(jobIdx);
        Csm_Jobs[jobIdx].service = CSM_SERVICE_HASH;
        Csm_Jobs[jobIdx].state = CSM_JOB_STATE_PROCESSING;
    }
    
    /* 复制输入数据 */
    if (dataLength > CSM_MAX_DATA_LENGTH)
    {
        return E_NOT_OK;
    }
    
    if (dataPtr != NULL_PTR && dataLength > 0U )
    {
(void)Mcal_MemCopy(Csm_Jobs[jobIdx].inputData, dataPtr, dataLength);
        Csm_Jobs[jobIdx].inputLength = dataLength;
    }
    
    if ((mode & CSM_OPERATION_MODE_FINISH) != 0U )
    {
        result = Csm_Cfg_HwService(
            jobId,
            CSM_SERVICE_HASH,
            Csm_Jobs[jobIdx].inputData,
            Csm_Jobs[jobIdx].inputLength,
            Csm_Jobs[jobIdx].outputData,
            &Csm_Jobs[jobIdx].resultLength
        );
        
        if (result == E_OK)
        {
            if (resultPtr != NULL_PTR && resultLengthPtr != NULL_PTR)
            {
                if (*resultLengthPtr >= Csm_Jobs[jobIdx].resultLength)
                {
(void)Mcal_MemCopy(resultPtr, Csm_Jobs[jobIdx].outputData, 
                                Csm_Jobs[jobIdx].resultLength);
                    *resultLengthPtr = Csm_Jobs[jobIdx].resultLength;
                }
                else
                {
                    *resultLengthPtr = Csm_Jobs[jobIdx].resultLength;
                    return E_NOT_OK;
                }
            }
            Csm_Jobs[jobIdx].state = CSM_JOB_STATE_IDLE;
        }
        
        return result;
    }
    
    return E_OK;
}

/**
 * @brief 生成MAC
 */
Std_ReturnType Csm_MacGenerate(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* macPtr,
    uint32* macLengthPtr)
{
    uint8 jobIdx;
    Std_ReturnType result;
    
    CSM_CHECK_INITIALIZED(CSM_API_MAC_GENERATE);
    CSM_CHECK_NULL_POINTER(CSM_API_MAC_GENERATE, dataPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_MAC_GENERATE, macPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_MAC_GENERATE, macLengthPtr);
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return E_NOT_OK;
    }
    
    /* 检查密钥权限 */
    if (E_OK != Csm_ValidateKeyUsage(Csm_Jobs[jobIdx].keyId, CSM_KEY_USAGE_MAC_GENERATE))
    {
        return E_NOT_OK;
    }
    
    if ((mode & CSM_OPERATION_MODE_START) != 0U )
    {
        Csm_ResetJob(jobIdx);
        Csm_Jobs[jobIdx].service = CSM_SERVICE_MAC_GENERATE;
    }
    
    if (dataLength > CSM_MAX_DATA_LENGTH)
    {
        return E_NOT_OK;
    }
    
(void)Mcal_MemCopy(Csm_Jobs[jobIdx].inputData, dataPtr, dataLength);
    Csm_Jobs[jobIdx].inputLength = dataLength;
    
    if ((mode & CSM_OPERATION_MODE_FINISH) != 0U )
    {
        result = Csm_Cfg_HwService(
            jobId,
            CSM_SERVICE_MAC_GENERATE,
            Csm_Jobs[jobIdx].inputData,
            Csm_Jobs[jobIdx].inputLength,
            Csm_Jobs[jobIdx].outputData,
            &Csm_Jobs[jobIdx].resultLength
        );
        
        if (result == E_OK)
        {
            if (*macLengthPtr >= Csm_Jobs[jobIdx].resultLength)
            {
(void)Mcal_MemCopy(macPtr, Csm_Jobs[jobIdx].outputData, 
                            Csm_Jobs[jobIdx].resultLength);
                *macLengthPtr = Csm_Jobs[jobIdx].resultLength;
            }
            else
            {
                *macLengthPtr = Csm_Jobs[jobIdx].resultLength;
                return E_NOT_OK;
            }
        }
        
        return result;
    }
    
    return E_OK;
}

/**
 * @brief 验证MAC
 */
Std_ReturnType Csm_MacVerify(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* macPtr,
    uint32 macLength,
    boolean* verifyPtr)
{
    uint8 jobIdx;
    uint8 calculatedMac[CSM_MAX_MAC_LENGTH];
    uint32 calculatedMacLength = CSM_MAX_MAC_LENGTH;
    Std_ReturnType result;
    
    CSM_CHECK_INITIALIZED(CSM_API_MAC_VERIFY);
    CSM_CHECK_NULL_POINTER(CSM_API_MAC_VERIFY, dataPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_MAC_VERIFY, macPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_MAC_VERIFY, verifyPtr);
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return E_NOT_OK;
    }
    
    /* 检查密钥权限 */
    if (E_OK != Csm_ValidateKeyUsage(Csm_Jobs[jobIdx].keyId, CSM_KEY_USAGE_MAC_VERIFY))
    {
        return E_NOT_OK;
    }
    
    /* 生成MAC并比较 */
    result = Csm_MacGenerate(jobId, mode, dataPtr, dataLength, 
                              calculatedMac, &calculatedMacLength);
    if (result != E_OK)
    {
        return result;
    }
    
    if ((mode & CSM_OPERATION_MODE_FINISH) != 0U )
    {
        if (calculatedMacLength == macLength)
        {
            *verifyPtr = (Mcal_MemCompare(calculatedMac, macPtr, macLength) == 0U );
        }
        else
        {
            *verifyPtr = FALSE;
        }
    }
    
    return E_OK;
}

/**
 * @brief 加密数据
 */
Std_ReturnType Csm_Encrypt(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr)
{
    uint8 jobIdx;
    Std_ReturnType result;
    
    CSM_CHECK_INITIALIZED(CSM_API_ENCRYPT);
    CSM_CHECK_NULL_POINTER(CSM_API_ENCRYPT, dataPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_ENCRYPT, resultPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_ENCRYPT, resultLengthPtr);
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return E_NOT_OK;
    }
    
    /* 检查密钥权限 */
    if (E_OK != Csm_ValidateKeyUsage(Csm_Jobs[jobIdx].keyId, CSM_KEY_USAGE_ENCRYPT))
    {
        return E_NOT_OK;
    }
    
    if ((mode & CSM_OPERATION_MODE_START) != 0U )
    {
        Csm_ResetJob(jobIdx);
        Csm_Jobs[jobIdx].service = CSM_SERVICE_ENCRYPT;
    }
    
    if (dataLength > CSM_MAX_DATA_LENGTH)
    {
        return E_NOT_OK;
    }
    
(void)Mcal_MemCopy(Csm_Jobs[jobIdx].inputData, dataPtr, dataLength);
    Csm_Jobs[jobIdx].inputLength = dataLength;
    
    if ((mode & CSM_OPERATION_MODE_FINISH) != 0U )
    {
        result = Csm_Cfg_HwService(
            jobId,
            CSM_SERVICE_ENCRYPT,
            Csm_Jobs[jobIdx].inputData,
            Csm_Jobs[jobIdx].inputLength,
            Csm_Jobs[jobIdx].outputData,
            &Csm_Jobs[jobIdx].resultLength
        );
        
        if (result == E_OK)
        {
            if (*resultLengthPtr >= Csm_Jobs[jobIdx].resultLength)
            {
(void)Mcal_MemCopy(resultPtr, Csm_Jobs[jobIdx].outputData,
                            Csm_Jobs[jobIdx].resultLength);
                *resultLengthPtr = Csm_Jobs[jobIdx].resultLength;
            }
            else
            {
                *resultLengthPtr = Csm_Jobs[jobIdx].resultLength;
                return E_NOT_OK;
            }
        }
        
        return result;
    }
    
    return E_OK;
}

/**
 * @brief 解密数据
 */
Std_ReturnType Csm_Decrypt(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr)
{
    uint8 jobIdx;
    Std_ReturnType result;
    
    CSM_CHECK_INITIALIZED(CSM_API_DECRYPT);
    CSM_CHECK_NULL_POINTER(CSM_API_DECRYPT, dataPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_DECRYPT, resultPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_DECRYPT, resultLengthPtr);
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return E_NOT_OK;
    }
    
    /* 检查密钥权限 */
    if (E_OK != Csm_ValidateKeyUsage(Csm_Jobs[jobIdx].keyId, CSM_KEY_USAGE_DECRYPT))
    {
        return E_NOT_OK;
    }
    
    if ((mode & CSM_OPERATION_MODE_START) != 0U )
    {
        Csm_ResetJob(jobIdx);
        Csm_Jobs[jobIdx].service = CSM_SERVICE_DECRYPT;
    }
    
    if (dataLength > CSM_MAX_DATA_LENGTH)
    {
        return E_NOT_OK;
    }
    
(void)Mcal_MemCopy(Csm_Jobs[jobIdx].inputData, dataPtr, dataLength);
    Csm_Jobs[jobIdx].inputLength = dataLength;
    
    if ((mode & CSM_OPERATION_MODE_FINISH) != 0U )
    {
        result = Csm_Cfg_HwService(
            jobId,
            CSM_SERVICE_DECRYPT,
            Csm_Jobs[jobIdx].inputData,
            Csm_Jobs[jobIdx].inputLength,
            Csm_Jobs[jobIdx].outputData,
            &Csm_Jobs[jobIdx].resultLength
        );
        
        if (result == E_OK)
        {
            if (*resultLengthPtr >= Csm_Jobs[jobIdx].resultLength)
            {
(void)Mcal_MemCopy(resultPtr, Csm_Jobs[jobIdx].outputData,
                            Csm_Jobs[jobIdx].resultLength);
                *resultLengthPtr = Csm_Jobs[jobIdx].resultLength;
            }
            else
            {
                *resultLengthPtr = Csm_Jobs[jobIdx].resultLength;
                return E_NOT_OK;
            }
        }
        
        return result;
    }
    
    return E_OK;
}

/**
 * @brief 生成数字签名
 */
Std_ReturnType Csm_SignatureGenerate(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* resultPtr,
    uint32* resultLengthPtr)
{
    uint8 jobIdx;
    Std_ReturnType result;
    
    CSM_CHECK_INITIALIZED(CSM_API_SIGNATURE_GENERATE);
    CSM_CHECK_NULL_POINTER(CSM_API_SIGNATURE_GENERATE, dataPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_SIGNATURE_GENERATE, resultPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_SIGNATURE_GENERATE, resultLengthPtr);
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return E_NOT_OK;
    }
    
    /* 检查密钥权限 */
    if (E_OK != Csm_ValidateKeyUsage(Csm_Jobs[jobIdx].keyId, CSM_KEY_USAGE_SIGN))
    {
        return E_NOT_OK;
    }
    
    if ((mode & CSM_OPERATION_MODE_START) != 0U )
    {
        Csm_ResetJob(jobIdx);
        Csm_Jobs[jobIdx].service = CSM_SERVICE_SIGNATURE_GENERATE;
    }
    
    if (dataLength > CSM_MAX_DATA_LENGTH)
    {
        return E_NOT_OK;
    }
    
(void)Mcal_MemCopy(Csm_Jobs[jobIdx].inputData, dataPtr, dataLength);
    Csm_Jobs[jobIdx].inputLength = dataLength;
    
    if ((mode & CSM_OPERATION_MODE_FINISH) != 0U )
    {
        result = Csm_Cfg_HwService(
            jobId,
            CSM_SERVICE_SIGNATURE_GENERATE,
            Csm_Jobs[jobIdx].inputData,
            Csm_Jobs[jobIdx].inputLength,
            Csm_Jobs[jobIdx].outputData,
            &Csm_Jobs[jobIdx].resultLength
        );
        
        if (result == E_OK)
        {
            if (*resultLengthPtr >= Csm_Jobs[jobIdx].resultLength)
            {
(void)Mcal_MemCopy(resultPtr, Csm_Jobs[jobIdx].outputData,
                            Csm_Jobs[jobIdx].resultLength);
                *resultLengthPtr = Csm_Jobs[jobIdx].resultLength;
            }
            else
            {
                *resultLengthPtr = Csm_Jobs[jobIdx].resultLength;
                return E_NOT_OK;
            }
        }
        
        return result;
    }
    
    return E_OK;
}

/**
 * @brief 验证数字签名
 */
Std_ReturnType Csm_SignatureVerify(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* signaturePtr,
    uint32 signatureLength,
    boolean* verifyPtr)
{
    uint8 jobIdx;
    Std_ReturnType result;
    
    CSM_CHECK_INITIALIZED(CSM_API_SIGNATURE_VERIFY);
    CSM_CHECK_NULL_POINTER(CSM_API_SIGNATURE_VERIFY, dataPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_SIGNATURE_VERIFY, signaturePtr);
    CSM_CHECK_NULL_POINTER(CSM_API_SIGNATURE_VERIFY, verifyPtr);
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return E_NOT_OK;
    }
    
    /* 检查密钥权限 */
    if (E_OK != Csm_ValidateKeyUsage(Csm_Jobs[jobIdx].keyId, CSM_KEY_USAGE_VERIFY))
    {
        return E_NOT_OK;
    }
    
    if ((mode & CSM_OPERATION_MODE_START) != 0U )
    {
        Csm_ResetJob(jobIdx);
        Csm_Jobs[jobIdx].service = CSM_SERVICE_SIGNATURE_VERIFY;
    }
    
    if ((mode & CSM_OPERATION_MODE_UPDATE) != 0U )
    {
        if (dataLength > CSM_MAX_DATA_LENGTH)
        {
            return E_NOT_OK;
        }
(void)Mcal_MemCopy(Csm_Jobs[jobIdx].inputData, dataPtr, dataLength);
        Csm_Jobs[jobIdx].inputLength = dataLength;
    }
    
    if ((mode & CSM_OPERATION_MODE_FINISH) != 0U )
    {
        /* 存储签名 */
        if (signatureLength > CSM_MAX_SIGNATURE_LENGTH)
        {
            return E_NOT_OK;
        }
(void)Mcal_MemCopy(Csm_Jobs[jobIdx].outputData, signaturePtr, signatureLength);
        Csm_Jobs[jobIdx].outputLength = signatureLength;
        
        result = Csm_Cfg_HwService(
            jobId,
            CSM_SERVICE_SIGNATURE_VERIFY,
            Csm_Jobs[jobIdx].inputData,
            Csm_Jobs[jobIdx].inputLength,
            Csm_Jobs[jobIdx].outputData,
            &Csm_Jobs[jobIdx].outputLength
        );
        
        if (result == E_OK)
        {
            /* 硬件层返回验证结果 */
            *verifyPtr = Csm_Jobs[jobIdx].verifyResult;
        }
        
        return result;
    }
    
    return E_OK;
}

/**
 * @brief 生成随机数
 */
Std_ReturnType Csm_RandomGenerate(
    uint32 jobId,
    uint8* resultPtr,
    uint32 resultLength)
{
    uint8 jobIdx;
    
    CSM_CHECK_INITIALIZED(CSM_API_RANDOM_GENERATE);
    CSM_CHECK_NULL_POINTER(CSM_API_RANDOM_GENERATE, resultPtr);
    
    if (resultLength > CSM_MAX_DATA_LENGTH)
    {
        return E_NOT_OK;
    }
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return Csm_Cfg_RandomGenerate(resultPtr, resultLength);
    }
    
    Csm_Jobs[jobIdx].service = CSM_SERVICE_RANDOM_GENERATE;
    Csm_Jobs[jobIdx].resultLength = resultLength;
    
    return Csm_Cfg_RandomGenerate(resultPtr, resultLength);
}

/**
 * @brief 设置作业密钥
 */
Std_ReturnType Csm_JobKeySetUp(uint32 jobId, uint32 keyId)
{
    uint8 jobIdx;
    
    CSM_CHECK_INITIALIZED(CSM_API_JOB_KEY_SETUP);
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return E_NOT_OK;
    }
    
    Csm_Jobs[jobIdx].keyId = keyId;
    
    return E_OK;
}

/**
 * @brief 异步设置作业密钥
 */
Std_ReturnType Csm_JobKeySetUpAsync(uint32 jobId, uint32 keyId)
{
    /* 目前与同步版本相同 */
    return Csm_JobKeySetUp(jobId, keyId);
}

/**
 * @brief 取消作业
 */
Std_ReturnType Csm_CancelJob(uint32 jobId)
{
    uint8 jobIdx;
    
    CSM_CHECK_INITIALIZED(CSM_API_CANCEL_JOB);
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return E_NOT_OK;
    }
    
    if (Csm_Jobs[jobIdx].state == CSM_JOB_STATE_PROCESSING)
    {
        Csm_ResetJob(jobIdx);
        if (Csm_ActiveJobCount > 0U )
        {
            Csm_ActiveJobCount--;
        }
    }
    else if (Csm_Jobs[jobIdx].state == CSM_JOB_STATE_QUEUED)
    {
        Csm_ResetJob(jobIdx);
    }
    
    return E_OK;
}

/**
 * @brief 主函数处理
 */
void Csm_MainFunction(void)
{
    uint8 i;
    
    if (Csm_State != CSM_STATE_ACTIVE)
    {
        return;
    }
    
    /* 处理正在进行的作业 */
    for (i = 0; i < CSM_MAX_JOBS; i++)
    {
        if (Csm_Jobs[i].state == CSM_JOB_STATE_PROCESSING)
        {
            (void)Csm_ExecuteJob(i);
        }
    }
    
    /* 处理队列 */
#if (CSM_CFG_QUEUE_SUPPORT == STD_ON)
    Csm_ProcessQueue();
#endif
}

/**
 * @brief 注册作业完成回调
 */
Std_ReturnType Csm_RegisterCallback(
    uint32 jobId,
    Csm_CallbackType callback,
    void* userContext)
{
    uint8 jobIdx;
    
    CSM_CHECK_INITIALIZED(0xF0U);
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return E_NOT_OK;
    }
    
    Csm_Callbacks[jobIdx] = callback;
    Csm_CallbackContexts[jobIdx] = userContext;
    
    return E_OK;
}

/**
 * @brief 获取密钥状态
 */
Std_ReturnType Csm_GetKeyStatus(
    uint32 keyId, Csm_KeyStatusType* keyStatusPtr)
{
    uint8 keyIdx;
    
    CSM_CHECK_INITIALIZED(0xF1U);
    CSM_CHECK_NULL_POINTER(0xF1U, keyStatusPtr);
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        return E_NOT_OK;
    }
    
    *keyStatusPtr = Csm_Keys[keyIdx].status;
    
    return E_OK;
}

/**
 * @brief 获取作业状态
 */
Std_ReturnType Csm_GetJobState(
    uint32 jobId, Csm_JobStateType* jobStatePtr)
{
    uint8 jobIdx;
    
    CSM_CHECK_INITIALIZED(0xF2U);
    CSM_CHECK_NULL_POINTER(0xF2U, jobStatePtr);
    
    if (E_OK != Csm_FindJobIndex(jobId, &jobIdx))
    {
        return E_NOT_OK;
    }
    
    *jobStatePtr = Csm_Jobs[jobIdx].state;
    
    return E_OK;
}
#define CSM_STOP_SEC_CODE
#include "Csm_MemMap.h"
