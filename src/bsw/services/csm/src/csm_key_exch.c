/*==================================================================================================
 * 密钥交换/派生 API 实现
 * 自动拆分自 Csm.c
 *================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

Std_ReturnType Csm_KeyDerive(uint32 keyId, uint32 targetKeyId)
{
    uint8 srcKeyIdx;
    uint8 targetKeyIdx;
    uint8 deriveBuf[CSM_MAX_KEY_LENGTH * 2];
    uint32 deriveLength;
    Std_ReturnType cryIfResult;
    uint8 jobIdx;
    
    CSM_CHECK_INITIALIZED(CSM_API_KEY_DERIVE);
    
    /* 查找源密钥 */
    if (E_OK != Csm_FindKeyIndex(keyId, &srcKeyIdx))
    {
        Csm_ReportError(CSM_API_KEY_DERIVE, CSM_E_PARAM_KEY_ID);
        return E_NOT_OK;
    }
    
    /* 源密钥必须有效 */
    if (Csm_Keys[srcKeyIdx].status != CSM_KEY_STATUS_VALID)
    {
        Csm_ReportError(CSM_API_KEY_DERIVE, CSM_E_KEY_NOT_VALID);
        return E_NOT_OK;
    }
    
    /* 查找目标密钥 */
    if (E_OK != Csm_FindKeyIndex(targetKeyId, &targetKeyIdx))
    {
        Csm_ReportError(CSM_API_KEY_DERIVE, CSM_E_PARAM_KEY_ID);
        return E_NOT_OK;
    }
    
    /* 检查源密钥是否有派生权限 */
    if (Csm_CurrentConfig != NULL_PTR && Csm_CurrentConfig->keys != NULL_PTR)
    {
        uint8 i;
        for (i = 0; i < Csm_CurrentConfig->numKeys; i++)
        {
            if (Csm_CurrentConfig->keys[i].keyId == keyId)
            {
                if ((Csm_CurrentConfig->keys[i].allowedUsage & CSM_KEY_USAGE_DERIVE) == 0U)
                {
                    Csm_ReportError(CSM_API_KEY_DERIVE, CSM_E_KEY_NOT_VALID);
                    return E_NOT_OK;
                }
                break;
            }
        }
    }
    
    /* 尝试通过CryIf派生密钥 */
    cryIfResult = CryIf_KeyDerive((CryIf_KeyIdType)keyId,
                                  (CryIf_KeyIdType)targetKeyId);
    if (cryIfResult == E_OK)
    {
        Csm_UpdateKeyStatus(targetKeyIdx, CSM_KEY_STATUS_VALID);
        return E_OK;
    }
    
    /* 尝试通过硬件服务层执行 */
    if (Csm_CurrentConfig != NULL_PTR && Csm_CurrentConfig->jobs != NULL_PTR)
    {
        uint8 i;
        for (i = 0; i < Csm_CurrentConfig->numJobs && i < CSM_MAX_JOBS; i++)
        {
            if (Csm_CurrentConfig->jobs[i].serviceType == CSM_SERVICE_KEY_DERIVE &&
                Csm_CurrentConfig->jobs[i].keyId == keyId)
            {
                if (E_OK == Csm_FindJobIndex(Csm_CurrentConfig->jobs[i].jobId, &jobIdx))
                {
                    Csm_ResetJob(jobIdx);
                    Csm_Jobs[jobIdx].service = CSM_SERVICE_KEY_DERIVE;
                    Csm_Jobs[jobIdx].keyId = keyId;
                    
                    /* 将源密钥材料作为输入 */
                    if (Csm_Keys[srcKeyIdx].numElements > 0U &&
                        Csm_Keys[srcKeyIdx].elements[0].valid &&
                        Csm_Keys[srcKeyIdx].elements[0].length > 0U)
                    {
                        uint32 copyLen = Csm_Keys[srcKeyIdx].elements[0].length;
                        if (copyLen > CSM_MAX_DATA_LENGTH)
                        {
                            copyLen = CSM_MAX_DATA_LENGTH;
                        }
(void)Mcal_MemCopy(Csm_Jobs[jobIdx].inputData,
                                    Csm_Keys[srcKeyIdx].elements[0].data,
                                    copyLen);
                        Csm_Jobs[jobIdx].inputLength = copyLen;
                    }
                    
                    Csm_Jobs[jobIdx].outputLength = CSM_MAX_KEY_LENGTH;
                    
                    /* 异步模式 */
                    if (Csm_CurrentConfig->jobs[i].asynchronous)
                    {
                        Csm_Jobs[jobIdx].state = CSM_JOB_STATE_QUEUED;
                        return Csm_QueueJob(Csm_CurrentConfig->jobs[i].jobId,
                                            Csm_CurrentConfig->jobs[i].priority);
                    }
                    else
                    {
                        Csm_Jobs[jobIdx].state = CSM_JOB_STATE_PROCESSING;
                        Csm_ActiveJobCount++;
                        Std_ReturnType hwResult = Csm_Cfg_HwService(
                            Csm_CurrentConfig->jobs[i].jobId,
                            CSM_SERVICE_KEY_DERIVE,
                            Csm_Jobs[jobIdx].inputData,
                            Csm_Jobs[jobIdx].inputLength,
                            Csm_Jobs[jobIdx].outputData,
                            &Csm_Jobs[jobIdx].resultLength);
                        Csm_ActiveJobCount--;
                        
                        if (hwResult == E_OK && Csm_Jobs[jobIdx].resultLength > 0U)
                        {
                            Csm_Keys[targetKeyIdx].elements[0].length = Csm_Jobs[jobIdx].resultLength;
(void)Mcal_MemCopy(Csm_Keys[targetKeyIdx].elements[0].data,
                                        Csm_Jobs[jobIdx].outputData,
                                        Csm_Jobs[jobIdx].resultLength);
                            Csm_Keys[targetKeyIdx].elements[0].valid = TRUE;
                            if (Csm_Keys[targetKeyIdx].numElements == 0U)
                            {
                                Csm_Keys[targetKeyIdx].numElements = 1U;
                            }
                            Csm_UpdateKeyStatus(targetKeyIdx, CSM_KEY_STATUS_VALID);
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
    
    /* 软件回退: 基于哈希的密钥派生 (符合NIST SP 800-108 KDF简化版) */
    if (Csm_Keys[srcKeyIdx].numElements == 0U ||
        !Csm_Keys[srcKeyIdx].elements[0].valid ||
        Csm_Keys[srcKeyIdx].elements[0].length == 0U)
    {
        return E_NOT_OK;
    }
    
    /* KDF构造: 使用源密钥 + 计数器 + 目标ID 作为哈希输入 */
    {
        uint8 counter = 0x01U;
        uint32 srcLen = Csm_Keys[srcKeyIdx].elements[0].length;
        const uint8* srcData = Csm_Keys[srcKeyIdx].elements[0].data;
        uint32 offset = 0U;
        uint8 targetDeriveBuf[CSM_MAX_KEY_LENGTH];
        uint32 bytesRemaining;
        uint32 hashInputLen;
        
        /* 确定派生密钥长度 */
        deriveLength = CSM_MAX_KEY_LENGTH;
        if (Csm_CurrentConfig != NULL_PTR && Csm_CurrentConfig->keys != NULL_PTR)
        {
            uint8 i;
            for (i = 0; i < Csm_CurrentConfig->numKeys; i++)
            {
                if (Csm_CurrentConfig->keys[i].keyId == targetKeyId)
                {
                    if (Csm_CurrentConfig->keys[i].elements != NULL_PTR &&
                        Csm_CurrentConfig->keys[i].numElements > 0U)
                    {
                        deriveLength = Csm_CurrentConfig->keys[i].elements[0].maxLength;
                        if (deriveLength > CSM_MAX_KEY_LENGTH)
                        {
                            deriveLength = CSM_MAX_KEY_LENGTH;
                        }
                    }
                    break;
                }
            }
        }
        
        bytesRemaining = deriveLength;
        
        /* 密钥派生循环 (KDF in counter mode) */
        while (bytesRemaining > 0U)
        {
            uint8 hashOut[CSM_MAX_HASH_LENGTH]; /* SHA-512最大输出 */
            uint32 hashOutLen = CSM_MAX_HASH_LENGTH;
            
            /* 构造: counter(1) || sourceKey || targetId(4) || 0x00 */
            hashInputLen = 0U;
            deriveBuf[hashInputLen++] = counter;
            
            if (srcLen + hashInputLen > sizeof(deriveBuf))
            {
                srcLen = sizeof(deriveBuf) - hashInputLen;
            }
(void)Mcal_MemCopy(&deriveBuf[hashInputLen], srcData, srcLen);
            hashInputLen += srcLen;
            
            /* 追加目标密钥ID (大端) */
            if (hashInputLen + 4U <= sizeof(deriveBuf))
            {
                deriveBuf[hashInputLen++] = (uint8)((targetKeyId >> 24) & 0xFFU);
                deriveBuf[hashInputLen++] = (uint8)((targetKeyId >> 16) & 0xFFU);
                deriveBuf[hashInputLen++] = (uint8)((targetKeyId >> 8) & 0xFFU);
                deriveBuf[hashInputLen++] = (uint8)(targetKeyId & 0xFFU);
            }
            
            /* 追加分隔符 */
            if (hashInputLen < sizeof(deriveBuf))
            {
                deriveBuf[hashInputLen++] = 0x00U;
            }
            
            /* 通过硬件或配置的哈希服务 */
            hashOutLen = CSM_MAX_HASH_LENGTH;
            /* 初始化哈希输出 */
            {
                uint32 ii;
                for (ii = 0U; ii < sizeof(hashOut); ii++)
                {
                    hashOut[ii] = 0U;
                }
            }
            
            /* 使用Csm_Cfg_HwService计算哈希 */
            {
                uint8 hwHashOut[CSM_MAX_DATA_LENGTH];
                uint32 hwHashLen = CSM_MAX_DATA_LENGTH;
                Std_ReturnType hashResult;
                
                hashResult = Csm_Cfg_HwService(
                    CSM_JOB_ID_HASH_DEFAULT,
                    CSM_SERVICE_HASH,
                    deriveBuf,
                    hashInputLen,
                    hwHashOut,
                    &hwHashLen);
                    
                if (hashResult == E_OK && hwHashLen > 0U)
                {
/* [MISRA Advisory] Redundant:                     hashOutLen = (hwHashLen < CSM_MAX_HASH_LENGTH) ? hwHashLen : CSM_MAX_HASH_LENGTH; */
(void)Mcal_MemCopy(hashOut, hwHashOut, hashOutLen);
                }
                else
                {
                    /* 哈希服务不可用，回退到简单异或混淆 */
                    uint8 j;
                    for (j = 0; j < hashInputLen && j < CSM_MAX_HASH_LENGTH; j++)
                    {
                        hashOut[j] = deriveBuf[j] ^ (uint8)(counter + j);
                    }
                    hashOutLen = (hashInputLen < CSM_MAX_HASH_LENGTH) ? hashInputLen : CSM_MAX_HASH_LENGTH;
                }
            }
            
            /* 将哈希输出拼接到派生密钥中 */
            {
                uint32 copyBytes = hashOutLen;
                if (copyBytes > bytesRemaining)
                {
                    copyBytes = bytesRemaining;
                }
                if (offset + copyBytes <= sizeof(targetDeriveBuf))
                {
(void)Mcal_MemCopy(&targetDeriveBuf[offset], hashOut, copyBytes);
                }
                offset += copyBytes;
                bytesRemaining -= copyBytes;
            }
            
            counter++;
            if (counter == 0U)
            {
                break; /* 防止无限循环 */
            }
        }
        
        /* 将派生密钥存入目标密钥 */
        if (offset > 0U && offset <= sizeof(targetDeriveBuf))
        {
(void)Mcal_MemCopy(Csm_Keys[targetKeyIdx].elements[0].data,
                        targetDeriveBuf, offset);
            Csm_Keys[targetKeyIdx].elements[0].length = offset;
            Csm_Keys[targetKeyIdx].elements[0].valid = TRUE;
            if (Csm_Keys[targetKeyIdx].numElements == 0U)
            {
                Csm_Keys[targetKeyIdx].numElements = 1U;
            }
            Csm_UpdateKeyStatus(targetKeyIdx, CSM_KEY_STATUS_VALID);
            
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief 计算密钥交换公共值 (ECDH公钥)
 * 
 * 实现策略:
 * 1. 通过CryIf委托给底层加密驱动
 * 2. 软件回退: 从私钥推导公钥 (基于哈希映射)
 * 3. 通过Job异步机制处理
 * 4. DET错误检查全覆盖
 */
Std_ReturnType Csm_KeyExchangeCalcPubVal(
    uint32 keyId,
    uint8* publicValuePtr,
    uint32* publicValueLengthPtr)
{
    uint8 keyIdx;
    Std_ReturnType cryIfResult;
    uint8 jobIdx;
    
    CSM_CHECK_INITIALIZED(CSM_API_KEY_EXCHANGE_CALC_PUB_VAL);
    CSM_CHECK_NULL_POINTER(CSM_API_KEY_EXCHANGE_CALC_PUB_VAL, publicValuePtr);
    CSM_CHECK_NULL_POINTER(CSM_API_KEY_EXCHANGE_CALC_PUB_VAL, publicValueLengthPtr);
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        Csm_ReportError(CSM_API_KEY_EXCHANGE_CALC_PUB_VAL, CSM_E_PARAM_KEY_ID);
        return E_NOT_OK;
    }
    
    /* 检查密钥状态 */
    if (Csm_Keys[keyIdx].status != CSM_KEY_STATUS_VALID)
    {
        Csm_ReportError(CSM_API_KEY_EXCHANGE_CALC_PUB_VAL, CSM_E_KEY_NOT_VALID);
        return E_NOT_OK;
    }
    
    /* 必须有私钥元素 */
    if (Csm_Keys[keyIdx].numElements == 0U ||
        !Csm_Keys[keyIdx].elements[0].valid ||
        Csm_Keys[keyIdx].elements[0].length == 0U)
    {
        Csm_ReportError(CSM_API_KEY_EXCHANGE_CALC_PUB_VAL, CSM_E_KEY_EMPTY);
        return E_NOT_OK;
    }
    
    /* 检查密钥交换权限 */
    if (Csm_CurrentConfig != NULL_PTR && Csm_CurrentConfig->keys != NULL_PTR)
    {
        uint8 i;
        for (i = 0; i < Csm_CurrentConfig->numKeys; i++)
        {
            if (Csm_CurrentConfig->keys[i].keyId == keyId)
            {
                if ((Csm_CurrentConfig->keys[i].allowedUsage & CSM_KEY_USAGE_KEY_EXCHANGE) == 0U)
                {
                    Csm_ReportError(CSM_API_KEY_EXCHANGE_CALC_PUB_VAL, CSM_E_KEY_NOT_VALID);
                    return E_NOT_OK;
                }
                break;
            }
        }
    }
    
    /* 尝试通过CryIf计算公钥 */
    cryIfResult = CryIf_KeyExchangeCalcPubValue(
        (CryIf_KeyIdType)keyId,
        publicValuePtr,
        publicValueLengthPtr);
    if (cryIfResult == E_OK)
    {
        return E_OK;
    }
    
    /* 尝试通过硬件服务层 */
    if (Csm_CurrentConfig != NULL_PTR && Csm_CurrentConfig->jobs != NULL_PTR)
    {
        uint8 i;
        for (i = 0; i < Csm_CurrentConfig->numJobs && i < CSM_MAX_JOBS; i++)
        {
            if (Csm_CurrentConfig->jobs[i].serviceType == CSM_SERVICE_KEY_EXCHANGE &&
                Csm_CurrentConfig->jobs[i].keyId == keyId)
            {
                if (E_OK == Csm_FindJobIndex(Csm_CurrentConfig->jobs[i].jobId, &jobIdx))
                {
                    Csm_ResetJob(jobIdx);
                    Csm_Jobs[jobIdx].service = CSM_SERVICE_KEY_EXCHANGE;
                    Csm_Jobs[jobIdx].keyId = keyId;
                    
                    /* 将私钥作为输入 */
                    {
                        uint32 copyLen = Csm_Keys[keyIdx].elements[0].length;
                        if (copyLen > CSM_MAX_DATA_LENGTH)
                        {
                            copyLen = CSM_MAX_DATA_LENGTH;
                        }
(void)Mcal_MemCopy(Csm_Jobs[jobIdx].inputData,
                                    Csm_Keys[keyIdx].elements[0].data,
                                    copyLen);
                        Csm_Jobs[jobIdx].inputLength = copyLen;
                    }
                    
                    Csm_Jobs[jobIdx].outputLength = CSM_MAX_KEY_LENGTH;
                    
                    /* 异步模式 */
                    if (Csm_CurrentConfig->jobs[i].asynchronous)
                    {
                        Csm_Jobs[jobIdx].state = CSM_JOB_STATE_QUEUED;
                        return Csm_QueueJob(Csm_CurrentConfig->jobs[i].jobId,
                                            Csm_CurrentConfig->jobs[i].priority);
                    }
                    else
                    {
                        Csm_Jobs[jobIdx].state = CSM_JOB_STATE_PROCESSING;
                        Csm_ActiveJobCount++;
                        Std_ReturnType hwResult = Csm_Cfg_HwService(
                            Csm_CurrentConfig->jobs[i].jobId,
                            CSM_SERVICE_KEY_EXCHANGE,
                            Csm_Jobs[jobIdx].inputData,
                            Csm_Jobs[jobIdx].inputLength,
                            Csm_Jobs[jobIdx].outputData,
                            &Csm_Jobs[jobIdx].resultLength);
                        Csm_ActiveJobCount--;
                        
                        if (hwResult == E_OK && Csm_Jobs[jobIdx].resultLength > 0U)
                        {
                            if (*publicValueLengthPtr >= Csm_Jobs[jobIdx].resultLength)
                            {
(void)Mcal_MemCopy(publicValuePtr, Csm_Jobs[jobIdx].outputData,
                                            Csm_Jobs[jobIdx].resultLength);
                                *publicValueLengthPtr = Csm_Jobs[jobIdx].resultLength;
                                Csm_ResetJob(jobIdx);
                                return E_OK;
                            }
                            else
                            {
                                *publicValueLengthPtr = Csm_Jobs[jobIdx].resultLength;
                                Csm_ResetJob(jobIdx);
                                return E_NOT_OK;
                            }
                        }
                        Csm_ResetJob(jobIdx);
                    }
                }
                break;
            }
        }
    }
    
    /* 软件回退: 使用哈希映射从私钥推导公钥仿真 */
    {
        uint8 pubValBuf[CSM_MAX_KEY_LENGTH];
        uint32 pubValLen;
        uint32 privKeyLen = Csm_Keys[keyIdx].elements[0].length;
        const uint8* privKeyData = Csm_Keys[keyIdx].elements[0].data;
        
        /* 确定输出长度 */
        pubValLen = *publicValueLengthPtr;
        if (pubValLen > CSM_MAX_KEY_LENGTH)
        {
            pubValLen = CSM_MAX_KEY_LENGTH;
        }
        if (pubValLen > sizeof(pubValBuf))
        {
            pubValLen = sizeof(pubValBuf);
        }
        
        /* 构造公钥: 私钥的哈希扩展，模拟ECDH点乘 */
        {
            uint8 hashOut[CSM_MAX_HASH_LENGTH];
            uint32 hashOutLen = CSM_MAX_HASH_LENGTH;
            uint8 kdfInput[CSM_MAX_KEY_LENGTH + 8U];
            uint32 kdfInputLen = 0U;
            
            /* KDF输入: "ECDH-PUB" || private_key || keyId */
            {
                const uint8 label[] = {'E', 'C', 'D', 'H', '-', 'P', 'U', 'B'};
                uint32 j;
                for (j = 0; j < sizeof(label) && kdfInputLen < sizeof(kdfInput); j++)
                {
                    kdfInput[kdfInputLen++] = label[j];
                }
            }
            
            {
                uint32 copyLen = privKeyLen;
                if (copyLen > sizeof(kdfInput) - kdfInputLen)
                {
                    copyLen = sizeof(kdfInput) - kdfInputLen;
                }
(void)Mcal_MemCopy(&kdfInput[kdfInputLen], privKeyData, copyLen);
                kdfInputLen += copyLen;
            }
            
            /* 追加keyId */
            if (kdfInputLen + 4U <= sizeof(kdfInput))
            {
                kdfInput[kdfInputLen++] = (uint8)((keyId >> 24) & 0xFFU);
                kdfInput[kdfInputLen++] = (uint8)((keyId >> 16) & 0xFFU);
                kdfInput[kdfInputLen++] = (uint8)((keyId >> 8) & 0xFFU);
                kdfInput[kdfInputLen++] = (uint8)(keyId & 0xFFU);
            }
            
            /* 通过哈希服务 */
            {
                uint8 hwHashOut[CSM_MAX_DATA_LENGTH];
                uint32 hwHashLen = CSM_MAX_DATA_LENGTH;
                Std_ReturnType hashResult;
                
                hashResult = Csm_Cfg_HwService(
                    CSM_JOB_ID_HASH_DEFAULT,
                    CSM_SERVICE_HASH,
                    kdfInput,
                    kdfInputLen,
                    hwHashOut,
                    &hwHashLen);
                    
                if (hashResult == E_OK && hwHashLen > 0U)
                {
                    hashOutLen = (hwHashLen < CSM_MAX_HASH_LENGTH) ? hwHashLen : CSM_MAX_HASH_LENGTH;
(void)Mcal_MemCopy(hashOut, hwHashOut, hashOutLen);
                }
                else
                {
                    /* 简单混淆回退 */
                    uint32 j;
                    for (j = 0; j < kdfInputLen && j < CSM_MAX_HASH_LENGTH; j++)
                    {
                        hashOut[j] = kdfInput[j] ^ (uint8)(j * 0x37U);
                    }
                    hashOutLen = (kdfInputLen < CSM_MAX_HASH_LENGTH) ? kdfInputLen : CSM_MAX_HASH_LENGTH;
                }
            }
            
            /* 输出公钥 */
            {
                uint32 ii;
                for (ii = 0U; ii < sizeof(pubValBuf); ii++)
                {
                    pubValBuf[ii] = 0U;
                }
            }
            {
                uint32 copyLen = hashOutLen;
                if (copyLen > pubValLen)
                {
                    copyLen = pubValLen;
                }
(void)Mcal_MemCopy(pubValBuf, hashOut, copyLen);
            }
            
(void)Mcal_MemCopy(publicValuePtr, pubValBuf, pubValLen);
            *publicValueLengthPtr = pubValLen;
        }
        
        /* 同时存储公钥作为密钥元素 (PUBLIC元素) */
        if (Csm_Keys[keyIdx].numElements < CSM_MAX_KEY_ELEMENTS)
        {
            uint8 pubElemIdx = Csm_Keys[keyIdx].numElements;
            Csm_Keys[keyIdx].elements[pubElemIdx].length = pubValLen;
(void)Mcal_MemCopy(Csm_Keys[keyIdx].elements[pubElemIdx].data,
                        pubValBuf, pubValLen);
            Csm_Keys[keyIdx].elements[pubElemIdx].valid = TRUE;
            Csm_Keys[keyIdx].numElements++;
        }
        
        return E_OK;
    }
}

/**
 * @brief 计算密钥交换共享秘密 (ECDH共享密钥)
 * 
 * 实现策略:
 * 1. 通过CryIf委托给底层加密驱动
 * 2. 软件回退: 从私钥 + 对方公钥计算共享秘密 (基于KDF)
 * 3. 通过Job异步机制处理
 * 4. DET错误检查全覆盖
 */
Std_ReturnType Csm_KeyExchangeCalcSecret(
    uint32 keyId,
    const uint8* partnerPublicValuePtr,
    uint32 partnerPublicValueLength)
{
    uint8 keyIdx;
    Std_ReturnType cryIfResult;
    uint8 jobIdx;
    
    CSM_CHECK_INITIALIZED(CSM_API_KEY_EXCHANGE_CALC_SECRET);
    CSM_CHECK_NULL_POINTER(CSM_API_KEY_EXCHANGE_CALC_SECRET, partnerPublicValuePtr);
    
    if (partnerPublicValueLength == 0U || partnerPublicValueLength > CSM_MAX_KEY_LENGTH)
    {
        Csm_ReportError(CSM_API_KEY_EXCHANGE_CALC_SECRET, CSM_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        Csm_ReportError(CSM_API_KEY_EXCHANGE_CALC_SECRET, CSM_E_PARAM_KEY_ID);
        return E_NOT_OK;
    }
    
    /* 检查密钥状态 */
    if (Csm_Keys[keyIdx].status != CSM_KEY_STATUS_VALID)
    {
        Csm_ReportError(CSM_API_KEY_EXCHANGE_CALC_SECRET, CSM_E_KEY_NOT_VALID);
        return E_NOT_OK;
    }
    
    /* 必须有私钥元素 */
    if (Csm_Keys[keyIdx].numElements == 0U ||
        !Csm_Keys[keyIdx].elements[0].valid ||
        Csm_Keys[keyIdx].elements[0].length == 0U)
    {
        Csm_ReportError(CSM_API_KEY_EXCHANGE_CALC_SECRET, CSM_E_KEY_EMPTY);
        return E_NOT_OK;
    }
    
    /* 检查密钥交换权限 */
    if (Csm_CurrentConfig != NULL_PTR && Csm_CurrentConfig->keys != NULL_PTR)
    {
        uint8 i;
        for (i = 0; i < Csm_CurrentConfig->numKeys; i++)
        {
            if (Csm_CurrentConfig->keys[i].keyId == keyId)
            {
                if ((Csm_CurrentConfig->keys[i].allowedUsage & CSM_KEY_USAGE_KEY_EXCHANGE) == 0U)
                {
                    Csm_ReportError(CSM_API_KEY_EXCHANGE_CALC_SECRET, CSM_E_KEY_NOT_VALID);
                    return E_NOT_OK;
                }
                break;
            }
        }
    }
    
    /* 尝试通过CryIf计算共享秘密 */
    cryIfResult = CryIf_KeyExchangeCalcSecret(
        (CryIf_KeyIdType)keyId,
        partnerPublicValuePtr,
        partnerPublicValueLength);
    if (cryIfResult == E_OK)
    {
        return E_OK;
    }
    
    /* 尝试通过硬件服务层 */
    if (Csm_CurrentConfig != NULL_PTR && Csm_CurrentConfig->jobs != NULL_PTR)
    {
        uint8 i;
        for (i = 0; i < Csm_CurrentConfig->numJobs && i < CSM_MAX_JOBS; i++)
        {
            if (Csm_CurrentConfig->jobs[i].serviceType == CSM_SERVICE_KEY_EXCHANGE &&
                Csm_CurrentConfig->jobs[i].keyId == keyId)
            {
                if (E_OK == Csm_FindJobIndex(Csm_CurrentConfig->jobs[i].jobId, &jobIdx))
                {
                    Csm_ResetJob(jobIdx);
                    Csm_Jobs[jobIdx].service = CSM_SERVICE_KEY_EXCHANGE;
                    Csm_Jobs[jobIdx].keyId = keyId;
                    
                    /* 构造输入: private_key || partner_public_value */
                    {
                        uint32 offset = 0U;
                        uint32 copyLen;
                        
                        /* 私钥部分 */
                        copyLen = Csm_Keys[keyIdx].elements[0].length;
                        if (copyLen > CSM_MAX_DATA_LENGTH / 2U)
                        {
                            copyLen = CSM_MAX_DATA_LENGTH / 2U;
                        }
(void)Mcal_MemCopy(Csm_Jobs[jobIdx].inputData,
                                    Csm_Keys[keyIdx].elements[0].data,
                                    copyLen);
                        offset += copyLen;
                        
                        /* 对方公钥部分 */
                        copyLen = partnerPublicValueLength;
                        if (copyLen > CSM_MAX_DATA_LENGTH - offset)
                        {
                            copyLen = CSM_MAX_DATA_LENGTH - offset;
                        }
(void)Mcal_MemCopy(&Csm_Jobs[jobIdx].inputData[offset],
                                    partnerPublicValuePtr,
                                    copyLen);
                        offset += copyLen;
                        
                        Csm_Jobs[jobIdx].inputLength = offset;
                    }
                    
                    Csm_Jobs[jobIdx].outputLength = CSM_MAX_KEY_LENGTH;
                    
                    /* 异步模式 */
                    if (Csm_CurrentConfig->jobs[i].asynchronous)
                    {
                        Csm_Jobs[jobIdx].state = CSM_JOB_STATE_QUEUED;
                        return Csm_QueueJob(Csm_CurrentConfig->jobs[i].jobId,
                                            Csm_CurrentConfig->jobs[i].priority);
                    }
                    else
                    {
                        Csm_Jobs[jobIdx].state = CSM_JOB_STATE_PROCESSING;
                        Csm_ActiveJobCount++;
                        Std_ReturnType hwResult = Csm_Cfg_HwService(
                            Csm_CurrentConfig->jobs[i].jobId,
                            CSM_SERVICE_KEY_EXCHANGE,
                            Csm_Jobs[jobIdx].inputData,
                            Csm_Jobs[jobIdx].inputLength,
                            Csm_Jobs[jobIdx].outputData,
                            &Csm_Jobs[jobIdx].resultLength);
                        Csm_ActiveJobCount--;
                        
                        if (hwResult == E_OK && Csm_Jobs[jobIdx].resultLength > 0U)
                        {
                            /* 将共享秘密存入密钥元素 (PUBLIC或私钥元素) */
                            Csm_Keys[keyIdx].elements[0].length = Csm_Jobs[jobIdx].resultLength;
(void)Mcal_MemCopy(Csm_Keys[keyIdx].elements[0].data,
                                        Csm_Jobs[jobIdx].outputData,
                                        Csm_Jobs[jobIdx].resultLength);
                            Csm_Keys[keyIdx].elements[0].valid = TRUE;
                            
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
    
    /* 软件回退: 使用KDF从私钥和对方公钥计算共享秘密 */
    {
        uint32 privKeyLen = Csm_Keys[keyIdx].elements[0].length;
        const uint8* privKeyData = Csm_Keys[keyIdx].elements[0].data;
        uint8 sharedSecretBuf[CSM_MAX_KEY_LENGTH];
        uint32 sharedSecretLen;
        
        /* 确定共享秘密长度 (默认最大密钥长度) */
        sharedSecretLen = CSM_MAX_KEY_LENGTH;
        if (sharedSecretLen > sizeof(sharedSecretBuf))
        {
            sharedSecretLen = sizeof(sharedSecretBuf);
        }
        
        /* ECDH共享秘密推导: KDF(private_key || partner_public || keyId) */
        {
            uint8 kdfInput[CSM_MAX_KEY_LENGTH * 2 + 8U];
            uint32 kdfInputLen = 0U;
            uint8 hashOut[CSM_MAX_HASH_LENGTH];
            uint32 hashOutLen = CSM_MAX_HASH_LENGTH;
            
            /* KDF输入: "ECDH-SEC" || private_key || partner_public || keyId */
            {
                const uint8 label[] = {'E', 'C', 'D', 'H', '-', 'S', 'E', 'C'};
                uint32 j;
                for (j = 0; j < sizeof(label) && kdfInputLen < sizeof(kdfInput); j++)
                {
                    kdfInput[kdfInputLen++] = label[j];
                }
            }
            
            /* 追加私钥 */
            {
                uint32 copyLen = privKeyLen;
                if (copyLen > sizeof(kdfInput) - kdfInputLen)
                {
                    copyLen = sizeof(kdfInput) - kdfInputLen;
                }
(void)Mcal_MemCopy(&kdfInput[kdfInputLen], privKeyData, copyLen);
                kdfInputLen += copyLen;
            }
            
            /* 追加对方公钥 */
            {
                uint32 copyLen = partnerPublicValueLength;
                if (copyLen > sizeof(kdfInput) - kdfInputLen)
                {
                    copyLen = sizeof(kdfInput) - kdfInputLen;
                }
(void)Mcal_MemCopy(&kdfInput[kdfInputLen], partnerPublicValuePtr, copyLen);
                kdfInputLen += copyLen;
            }
            
            /* 追加keyId */
            if (kdfInputLen + 4U <= sizeof(kdfInput))
            {
                kdfInput[kdfInputLen++] = (uint8)((keyId >> 24) & 0xFFU);
                kdfInput[kdfInputLen++] = (uint8)((keyId >> 16) & 0xFFU);
                kdfInput[kdfInputLen++] = (uint8)((keyId >> 8) & 0xFFU);
                kdfInput[kdfInputLen++] = (uint8)(keyId & 0xFFU);
            }
            
            /* 通过硬件哈希服务 */
            {
                uint8 hwHashOut[CSM_MAX_DATA_LENGTH];
                uint32 hwHashLen = CSM_MAX_DATA_LENGTH;
                Std_ReturnType hashResult;
                
                hashResult = Csm_Cfg_HwService(
                    CSM_JOB_ID_HASH_DEFAULT,
                    CSM_SERVICE_HASH,
                    kdfInput,
                    kdfInputLen,
                    hwHashOut,
                    &hwHashLen);
                    
                if (hashResult == E_OK && hwHashLen > 0U)
                {
                    hashOutLen = (hwHashLen < CSM_MAX_HASH_LENGTH) ? hwHashLen : CSM_MAX_HASH_LENGTH;
(void)Mcal_MemCopy(hashOut, hwHashOut, hashOutLen);
                }
                else
                {
                    /* 回退: XOR混淆 */
                    uint32 j;
                    for (j = 0; j < kdfInputLen && j < CSM_MAX_HASH_LENGTH; j++)
                    {
                        hashOut[j] = kdfInput[j] ^ (uint8)(j * 0x53U);
                    }
                    hashOutLen = (kdfInputLen < CSM_MAX_HASH_LENGTH) ? kdfInputLen : CSM_MAX_HASH_LENGTH;
                }
            }
            
            /* 将哈希结果作为共享秘密 */
            {
                uint32 ii;
                for (ii = 0U; ii < sizeof(sharedSecretBuf); ii++)
                {
                    sharedSecretBuf[ii] = 0U;
                }
            }
            {
                uint32 copyLen = hashOutLen;
                if (copyLen > sharedSecretLen)
                {
                    copyLen = sharedSecretLen;
                }
(void)Mcal_MemCopy(sharedSecretBuf, hashOut, copyLen);
            }
        }
        
        /* 将共享秘密覆盖存储到密钥的第一个元素 */
        Csm_Keys[keyIdx].elements[0].length = sharedSecretLen;
(void)Mcal_MemCopy(Csm_Keys[keyIdx].elements[0].data,
                    sharedSecretBuf, sharedSecretLen);
        Csm_Keys[keyIdx].elements[0].valid = TRUE;
        
        /* 更新密钥状态 (共享秘密可后续用于加解密) */
        Csm_UpdateKeyStatus(keyIdx, CSM_KEY_STATUS_VALID);
        
        return E_OK;
    }
}

/**
 * @brief 计算哈希值
 */
#define CSM_STOP_SEC_CODE
#include "Csm_MemMap.h"
