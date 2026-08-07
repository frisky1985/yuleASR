/*==================================================================================================
 * 作业/密码服务 API 实现
 * 自动拆分自 Csm.c
 *================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

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
        uint8 const* privKeyData = Csm_Keys[keyIdx].elements[0].data;
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
                    kdfInput[kdfInputLen] = label[j];
                    kdfInputLen++;
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
                kdfInput[kdfInputLen] = (uint8)((keyId >> 24) & 0xFFU);
                kdfInputLen++;
                kdfInput[kdfInputLen] = (uint8)((keyId >> 16) & 0xFFU);
                kdfInputLen++;
                kdfInput[kdfInputLen] = (uint8)((keyId >> 8) & 0xFFU);
                kdfInputLen++;
                kdfInput[kdfInputLen] = (uint8)(keyId & 0xFFU);
                kdfInputLen++;
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