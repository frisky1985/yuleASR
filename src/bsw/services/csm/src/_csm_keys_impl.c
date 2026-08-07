/*==================================================================================================
 * 密钥管理 API 实现
 * 自动拆分自 Csm.c
 *================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

Std_ReturnType Csm_KeyElementSet(
    uint32 keyId,
    uint32 keyElementId,
    const uint8* keyPtr,
    uint32 keyLength)
{
    uint8 keyIdx;
    uint8 elemIdx;
    
    CSM_CHECK_INITIALIZED(CSM_API_KEY_ELEMENT_SET);
    CSM_CHECK_NULL_POINTER(CSM_API_KEY_ELEMENT_SET, keyPtr);
    
    if (keyLength > CSM_MAX_KEY_LENGTH)
    {
        Csm_ReportError(CSM_API_KEY_ELEMENT_SET, CSM_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        Csm_ReportError(CSM_API_KEY_ELEMENT_SET, CSM_E_PARAM_KEY_ID);
        return E_NOT_OK;
    }
    
    /* 查找或分配元素索引 */
    if (E_OK != Csm_FindKeyElementIndex(keyIdx, keyElementId, &elemIdx))
    {
        /* 新增元素 */
        if (Csm_Keys[keyIdx].numElements >= CSM_MAX_KEY_ELEMENTS)
        {
            return E_NOT_OK;
        }
        elemIdx = Csm_Keys[keyIdx].numElements;
        Csm_Keys[keyIdx].numElements++;
    }
    
    /* 复制数据 */
(void)Mcal_MemCopy(Csm_Keys[keyIdx].elements[elemIdx].data, keyPtr, keyLength);
    Csm_Keys[keyIdx].elements[elemIdx].length = keyLength;
    Csm_Keys[keyIdx].elements[elemIdx].valid = TRUE;
    
    /* 更新状态为更新中 */
    Csm_UpdateKeyStatus(keyIdx, CSM_KEY_STATUS_UPDATE_IN_PROGRESS);
    
    return E_OK;
}

/**
 * @brief 设置密钥为有效状态
 */
Std_ReturnType Csm_KeySetValid(uint32 keyId)
{
    uint8 keyIdx;
    
    CSM_CHECK_INITIALIZED(CSM_API_KEY_SET_VALID);
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        Csm_ReportError(CSM_API_KEY_SET_VALID, CSM_E_PARAM_KEY_ID);
        return E_NOT_OK;
    }
    
    /* 检查必需的元素是否已设置 */
    if (Csm_Keys[keyIdx].numElements == 0U )
    {
        return E_NOT_OK;
    }
    
    /* 持久化密钥 */
#if (CSM_CFG_KEY_PERSISTENCE_SUPPORT == STD_ON)
    {
        uint8 i;
        for (i = 0; i < Csm_Keys[keyIdx].numElements; i++)
        {
            if (E_OK != Csm_PersistKeyElement(keyId, CSM_KEY_ELEMENT_ID_SECRET + i))
            {
                return E_NOT_OK;
            }
        }
    }
#endif
    
    Csm_UpdateKeyStatus(keyIdx, CSM_KEY_STATUS_VALID);
    
    return E_OK;
}

/**
 * @brief 获取密钥元素数据
 */
Std_ReturnType Csm_KeyElementGet(
    uint32 keyId,
    uint32 keyElementId,
    uint8* keyPtr,
    uint32* keyLengthPtr)
{
    uint8 keyIdx;
    uint8 elemIdx;
    
    CSM_CHECK_INITIALIZED(CSM_API_KEY_ELEMENT_GET);
    CSM_CHECK_NULL_POINTER(CSM_API_KEY_ELEMENT_GET, keyPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_KEY_ELEMENT_GET, keyLengthPtr);
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        Csm_ReportError(CSM_API_KEY_ELEMENT_GET, CSM_E_PARAM_KEY_ID);
        return E_NOT_OK;
    }
    
    if (Csm_Keys[keyIdx].status != CSM_KEY_STATUS_VALID)
    {
        return E_NOT_OK;
    }
    
    if (E_OK != Csm_FindKeyElementIndex(keyIdx, keyElementId, &elemIdx))
    {
        return E_NOT_OK;
    }
    
    if (*keyLengthPtr < Csm_Keys[keyIdx].elements[elemIdx].length)
    {
        *keyLengthPtr = Csm_Keys[keyIdx].elements[elemIdx].length;
        return E_NOT_OK;
    }
    
(void)Mcal_MemCopy(keyPtr, Csm_Keys[keyIdx].elements[elemIdx].data, 
                 Csm_Keys[keyIdx].elements[elemIdx].length);
    *keyLengthPtr = Csm_Keys[keyIdx].elements[elemIdx].length;
    
    return E_OK;
}

/**
 * @brief 复制密钥元素
 */
Std_ReturnType Csm_KeyElementCopy(
    uint32 keyId,
    uint32 keyElementId,
    uint32 targetKeyId,
    uint32 targetKeyElementId)
{
    uint8 keyData[CSM_MAX_KEY_LENGTH];
    uint32 length = CSM_MAX_KEY_LENGTH;
    Std_ReturnType result;
    
    result = Csm_KeyElementGet(keyId, keyElementId, keyData, &length);
    if (result != E_OK)
    {
        return result;
    }
    
    return Csm_KeyElementSet(targetKeyId, targetKeyElementId, keyData, length);
}

/**
 * @brief 复制完整密钥
 */
Std_ReturnType Csm_KeyCopy(uint32 keyId, uint32 targetKeyId)
{
    uint8 keyIdx;
    uint8 i;
    Std_ReturnType result;
    
    CSM_CHECK_INITIALIZED(CSM_API_KEY_COPY);
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        return E_NOT_OK;
    }
    
    for (i = 0; i < Csm_Keys[keyIdx].numElements; i++)
    {
        result = Csm_KeyElementCopy(keyId, CSM_KEY_ELEMENT_ID_SECRET + i,
                                     targetKeyId, CSM_KEY_ELEMENT_ID_SECRET + i);
        if (result != E_OK)
        {
            return result;
        }
    }
    
    return Csm_KeySetValid(targetKeyId);
}

/**
 * @brief 获取密钥的元素ID列表
 */
Std_ReturnType Csm_KeyElementIdsGet(
    uint32 keyId,
    uint32* keyElementIdsPtr,
    uint32* keyElementIdsLengthPtr)
{
    uint8 keyIdx;
    uint8 i;
    
    CSM_CHECK_INITIALIZED(CSM_API_KEY_ELEMENT_IDS_GET);
    CSM_CHECK_NULL_POINTER(CSM_API_KEY_ELEMENT_IDS_GET, keyElementIdsPtr);
    CSM_CHECK_NULL_POINTER(CSM_API_KEY_ELEMENT_IDS_GET, keyElementIdsLengthPtr);
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        return E_NOT_OK;
    }
    
    if (*keyElementIdsLengthPtr < Csm_Keys[keyIdx].numElements)
    {
        *keyElementIdsLengthPtr = Csm_Keys[keyIdx].numElements;
        return E_NOT_OK;
    }
    
    for (i = 0; i < Csm_Keys[keyIdx].numElements; i++)
    {
        keyElementIdsPtr[i] = CSM_KEY_ELEMENT_ID_SECRET + i;
    }
    *keyElementIdsLengthPtr = Csm_Keys[keyIdx].numElements;
    
    return E_OK;
}

/**
 * @brief 生成密钥
 * 
 * 实现策略:
 * 1. 通过CryIf委托给底层加密驱动
 * 2. 软件回退: 使用Csm_Cfg_RandomGenerate生成随机密钥材料
 * 3. 支持Job异步处理模式
 * 4. DET错误检查全覆盖
 */
Std_ReturnType Csm_KeyGenerate(uint32 keyId)
{
    uint8 keyIdx;
    uint8 keyBuf[CSM_MAX_KEY_LENGTH];
    uint32 keyLength;
    Std_ReturnType cryIfResult;
    uint8 jobIdx;
    
    CSM_CHECK_INITIALIZED(CSM_API_KEY_GENERATE);
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        Csm_ReportError(CSM_API_KEY_GENERATE, CSM_E_PARAM_KEY_ID);
        return E_NOT_OK;
    }
    
    /* 检查密钥是否已存在且有效 */
    if (Csm_Keys[keyIdx].status == CSM_KEY_STATUS_VALID)
    {
        return E_OK;
    }
    
    /* 尝试通过CryIf生成密钥 */
    cryIfResult = CryIf_KeyGenerate((CryIf_KeyIdType)keyId);
    if (cryIfResult == E_OK)
    {
        /* CryIf成功，直接从密钥存储加载 */
        Csm_UpdateKeyStatus(keyIdx, CSM_KEY_STATUS_VALID);
        return E_OK;
    }
    
    /* 尝试通过硬件服务层生成 */
    if (Csm_CurrentConfig != NULL_PTR && Csm_CurrentConfig->jobs != NULL_PTR)
    {
        /* 查找关联的作业配置 */
        uint8 i;
        for (i = 0; i < Csm_CurrentConfig->numJobs && i < CSM_MAX_JOBS; i++)
        {
            if (Csm_CurrentConfig->jobs[i].keyId == keyId &&
                Csm_CurrentConfig->jobs[i].serviceType == CSM_SERVICE_KEY_GENERATE)
            {
                if (E_OK == Csm_FindJobIndex(Csm_CurrentConfig->jobs[i].jobId, &jobIdx))
                {
                    Csm_ResetJob(jobIdx);
                    Csm_Jobs[jobIdx].service = CSM_SERVICE_KEY_GENERATE;
                    Csm_Jobs[jobIdx].keyId = keyId;
                    Csm_Jobs[jobIdx].inputLength = 0U;
                    Csm_Jobs[jobIdx].outputLength = CSM_MAX_KEY_LENGTH;
                    
                    /* 异步: 入队处理 */
                    if (Csm_CurrentConfig->jobs[i].asynchronous)
                    {
                        Csm_Jobs[jobIdx].state = CSM_JOB_STATE_QUEUED;
                        return Csm_QueueJob(Csm_CurrentConfig->jobs[i].jobId,
                                            Csm_CurrentConfig->jobs[i].priority);
                    }
                    else
                    {
                        /* 同步: 通过Csm_Cfg_HwService执行 */
                        Csm_Jobs[jobIdx].state = CSM_JOB_STATE_PROCESSING;
                        Csm_ActiveJobCount++;
                        Std_ReturnType hwResult = Csm_Cfg_HwService(
                            Csm_CurrentConfig->jobs[i].jobId,
                            CSM_SERVICE_KEY_GENERATE,
                            Csm_Jobs[jobIdx].inputData,
                            Csm_Jobs[jobIdx].inputLength,
                            Csm_Jobs[jobIdx].outputData,
                            &Csm_Jobs[jobIdx].resultLength);
                        Csm_ActiveJobCount--;
                        
                        if (hwResult == E_OK && Csm_Jobs[jobIdx].resultLength > 0U)
                        {
                            /* 将生成的密钥材料存入密钥元素 */
                            Csm_Keys[keyIdx].elements[0].length = Csm_Jobs[jobIdx].resultLength;
(void)Mcal_MemCopy(Csm_Keys[keyIdx].elements[0].data,
                                        Csm_Jobs[jobIdx].outputData,
                                        Csm_Jobs[jobIdx].resultLength);
                            Csm_Keys[keyIdx].elements[0].valid = TRUE;
                            if (Csm_Keys[keyIdx].numElements == 0U)
                            {
                                Csm_Keys[keyIdx].numElements = 1U;
                            }
                            Csm_UpdateKeyStatus(keyIdx, CSM_KEY_STATUS_VALID);
                            Csm_ResetJob(jobIdx);
                            return E_OK;
                        }
                        Csm_ResetJob(jobIdx);
                    }
                }
                break;
            }
        }
    }
    
    /* 软件回退: 使用随机数生成器生成密钥材料 */
    /* 确定密钥长度 (从配置或默认值) */
    keyLength = CSM_MAX_KEY_LENGTH;
    if (Csm_CurrentConfig != NULL_PTR && Csm_CurrentConfig->keys != NULL_PTR)
    {
        uint8 i;
        for (i = 0; i < Csm_CurrentConfig->numKeys; i++)
        {
            if (Csm_CurrentConfig->keys[i].keyId == keyId)
            {
                /* 从元素配置中获取密钥长度 */
                if (Csm_CurrentConfig->keys[i].elements != NULL_PTR &&
                    Csm_CurrentConfig->keys[i].numElements > 0U)
                {
                    keyLength = Csm_CurrentConfig->keys[i].elements[0].maxLength;
                    if (keyLength > CSM_MAX_KEY_LENGTH)
                    {
                        keyLength = CSM_MAX_KEY_LENGTH;
                    }
                }
                break;
            }
        }
    }
    
    /* 生成随机密钥数据 */
    if (E_OK != Csm_Cfg_RandomGenerate(keyBuf, keyLength))
    {
        Csm_ReportError(CSM_API_KEY_GENERATE, CSM_E_ENTROPY_EXHAUSTION);
        return E_NOT_OK;
    }
    
    /* 存入密钥元素 */
(void)Mcal_MemCopy(Csm_Keys[keyIdx].elements[0].data, keyBuf, keyLength);
    Csm_Keys[keyIdx].elements[0].length = keyLength;
    Csm_Keys[keyIdx].elements[0].valid = TRUE;
    if (Csm_Keys[keyIdx].numElements == 0U)
    {
        Csm_Keys[keyIdx].numElements = 1U;
    }
    
    /* 标记密钥为有效 */
    Csm_UpdateKeyStatus(keyIdx, CSM_KEY_STATUS_VALID);
    
    return E_OK;
}

/**
 * @brief 派生密钥 (KDF)
 * 
 * 实现策略:
 * 1. 通过CryIf委托给底层加密驱动
 * 2. 软件回退: 基于HMAC-SHA256的KDF (或简单哈希迭代)
 * 3. 支持Job异步处理
 * 4. DET检查: 源密钥有效, 目标密钥存在, 使用权限
 */

#define CSM_STOP_SEC_CODE
#include "Csm_MemMap.h"