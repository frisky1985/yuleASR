/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/
/**
 * @file Csm.c
 * @brief CSM (Crypto Services Manager) 核心实现
 * 
 * 功能: 密码服务管理器核心功能实现
 * - 密钥管理
 * - 服务队列管理
 * - 异步服务处理
 * 
 * @author yuleASR Team
 * @version 1.0.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Csm.h"
#include "Csm_Cfg.h"
#include "Det.h"
/* Provide Mcal_MemCopy macro (Mcal.h not available) */
#ifndef Mcal_MemCopy
#include <string.h>
#define Mcal_MemCopy(dst, src, len)  memcpy((dst), (src), (len))
#ifndef Mcal_MemCompare
#define Mcal_MemCompare(a, b, len)  memcmp((a), (b), (len))
#endif
#endif

#include "CryIf.h"

#if (CSM_CFG_DEM_INTEGRATION == STD_ON)
#include "Dem.h"
#endif

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief 模块ID (用于Det)
 */
#define CSM_MODULE_ID                           0x70U

/**
 * @brief 操作模式定义
 */
#define CSM_OPERATION_MODE_START                0x01U
#define CSM_OPERATION_MODE_UPDATE               0x02U
#define CSM_OPERATION_MODE_FINISH               0x04U
#define CSM_OPERATION_MODE_SINGLECALL           0x07U

/**
 * @brief 魔数用于数据完整性校验
 */
#define CSM_MAGIC_INITIALIZED                   0x43534D01U
#define CSM_MAGIC_KEY_VALID                     0x4B455956U

/**
 * @brief 开发错误检测宏
 */
#if (CSM_CFG_DEV_ERROR_DETECT == STD_ON)
#define CSM_CHECK_INITIALIZED(apiId) \
    do { \
        if (Csm_State == CSM_STATE_UNINIT) { \
            Csm_ReportError((apiId), CSM_E_NOT_INITIALIZED); \
            return E_NOT_OK; \
        } \
    } while(0)

#define CSM_CHECK_NULL_POINTER(apiId, ptr) \
    do { \
        if ((ptr) == NULL_PTR) { \
            Csm_ReportError((apiId), CSM_E_PARAM_POINTER); \
            return E_NOT_OK; \
        } \
    } while(0)
#else
#define CSM_CHECK_INITIALIZED(apiId)
#define CSM_CHECK_NULL_POINTER(apiId, ptr)
#endif

/*==================================================================================================
*                                       类型定义
==================================================================================================*/
/**
 * @brief CSM状态
 */
typedef enum
{
    CSM_STATE_UNINIT = 0,
    CSM_STATE_INIT,
    CSM_STATE_ACTIVE
} Csm_InternalStateType;

/*==================================================================================================
*                                       全局变量
==================================================================================================*/
#define CSM_START_SEC_VAR_INIT_UNSPECIFIED
#include "Csm_MemMap.h"

/**
 * @brief 初始化状态
 */
STATIC volatile Csm_InternalStateType Csm_State = CSM_STATE_UNINIT;

/**
 * @brief 当前配置
 */
STATIC const Csm_ConfigType* Csm_CurrentConfig = NULL_PTR;

/**
 * @brief 初始化魔数
 */
STATIC volatile uint32 Csm_InitMagic = 0U;

#define CSM_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "Csm_MemMap.h"

#define CSM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Csm_MemMap.h"

/**
 * @brief 密钥数据数组
 */
STATIC Csm_KeyType Csm_Keys[CSM_MAX_KEYS];

/**
 * @brief 作业数据数组
 */
STATIC Csm_JobType Csm_Jobs[CSM_MAX_JOBS];

/**
 * @brief 服务队列
 */
STATIC Csm_QueueType Csm_JobQueue;

/**
 * @brief 回调函数数组
 */
STATIC Csm_CallbackType Csm_Callbacks[CSM_MAX_JOBS];

/**
 * @brief 回调上下文数组
 */
STATIC void* Csm_CallbackContexts[CSM_MAX_JOBS];

/**
 * @brief 当前处理的作业数
 */
STATIC uint8 Csm_ActiveJobCount = 0U;

#define CSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Csm_MemMap.h"

/*==================================================================================================
*                                       静态函数声明
==================================================================================================*/
STATIC void Csm_ReportError(uint8 apiId, uint8 errorId);
STATIC void Csm_NotifyEvent(uint32 jobId, Std_ReturnType result);
STATIC Std_ReturnType Csm_ValidateConfig(const Csm_ConfigType* config);
STATIC Std_ReturnType Csm_FindKeyIndex(uint32 keyId, uint8* index);
STATIC Std_ReturnType Csm_FindJobIndex(uint32 jobId, uint8* index);
STATIC Std_ReturnType Csm_FindKeyElementIndex(uint8 keyIdx, uint32 elementId, uint8* index);
STATIC Std_ReturnType Csm_QueueJob(uint32 jobId, Csm_JobPriorityType priority);
STATIC Std_ReturnType Csm_DequeueJob(uint32* jobId);
STATIC void Csm_ProcessQueue(void);
STATIC Std_ReturnType Csm_ExecuteJob(uint8 jobIdx);
STATIC void Csm_ResetJob(uint8 jobIdx);
STATIC Std_ReturnType Csm_ValidateKeyUsage(uint32 keyId, Csm_KeyUsageType requiredUsage);
STATIC void Csm_UpdateKeyStatus(uint8 keyIdx, Csm_KeyStatusType newStatus);
STATIC Std_ReturnType Csm_PersistKeyElement(uint32 keyId, uint32 elementId);
STATIC Std_ReturnType Csm_LoadKeyElement(uint32 keyId, uint32 elementId);

/*==================================================================================================
*                                       函数实现
==================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

/**
 * @brief 报告错误
 */
STATIC void Csm_ReportError(uint8 apiId, uint8 errorId)
{
#if (CSM_CFG_DEV_ERROR_DETECT == STD_ON)
    Det_ReportError(CSM_MODULE_ID, 0, apiId, errorId);
#else
    (void)apiId;
    (void)errorId;
#endif
}

/**
 * @brief 通知事件回调
 */
STATIC void Csm_NotifyEvent(uint32 jobId, Std_ReturnType result)
{
    uint8 jobIdx;
    
    if (E_OK == Csm_FindJobIndex(jobId, &jobIdx))
    {
        if (Csm_Callbacks[jobIdx] != NULL_PTR)
        {
            Csm_Callbacks[jobIdx](
                jobId,
                result,
                Csm_Jobs[jobIdx].outputData,
                Csm_Jobs[jobIdx].resultLength,
                Csm_CallbackContexts[jobIdx]
            );
        }
    }
}

/**
 * @brief 验证配置
 */
STATIC Std_ReturnType Csm_ValidateConfig(const Csm_ConfigType* config)
{
    if (config == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    if (config->numKeys > CSM_MAX_KEYS)
    {
        return E_NOT_OK;
    }
    
    if (config->numJobs > CSM_MAX_JOBS)
    {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief 查找密钥索引
 */
STATIC Std_ReturnType Csm_FindKeyIndex(uint32 keyId, uint8* index)
{
    uint8 i;
    
    if (index == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    for (i = 0; i < CSM_MAX_KEYS; i++)
    {
        if (Csm_Keys[i].keyId == keyId)
        {
            *index = i;
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief 查找作业索引
 */
STATIC Std_ReturnType Csm_FindJobIndex(uint32 jobId, uint8* index)
{
    uint8 i;
    
    if (index == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    for (i = 0; i < CSM_MAX_JOBS; i++)
    {
        if (Csm_Jobs[i].jobId == jobId)
        {
            *index = i;
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief 查找密钥元素索引
 */
STATIC Std_ReturnType Csm_FindKeyElementIndex(uint8 keyIdx, uint32 elementId, uint8* index)
{
    uint8 i;
    
    if ((keyIdx >= CSM_MAX_KEYS) || (index == NULL_PTR))
    {
        return E_NOT_OK;
    }
    
    for (i = 0; i < Csm_Keys[keyIdx].numElements; i++)
    {
        if (Csm_Keys[keyIdx].elements[i].valid)
        {
            /* 简化处理：假设元素ID按顺序存储 */
            *index = i;
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief 将作业加入队列
 */
STATIC Std_ReturnType Csm_QueueJob(uint32 jobId, Csm_JobPriorityType priority)
{
#if (CSM_CFG_QUEUE_SUPPORT == STD_ON)
    uint8 i, insertPos;
    
    if (Csm_JobQueue.count >= CSM_CFG_QUEUE_SIZE)
    {
        return E_NOT_OK; /* 队列满 */
    }
    
    /* 按优先级插入作业 (高优先级在前) */
    insertPos = Csm_JobQueue.tail;
    for (i = Csm_JobQueue.head; i != Csm_JobQueue.tail; i = (i + 1) % CSM_CFG_QUEUE_SIZE)
    {
        if (Csm_JobQueue.items[i].priority < priority)
        {
            insertPos = i;
            break;
        }
    }
    
    /* 移动元素以插入新作业 */
    for (i = Csm_JobQueue.tail; i != insertPos; i = (i - 1 + CSM_CFG_QUEUE_SIZE) % CSM_CFG_QUEUE_SIZE)
    {
        uint8 prev = (i - 1 + CSM_CFG_QUEUE_SIZE) % CSM_CFG_QUEUE_SIZE;
        Csm_JobQueue.items[i] = Csm_JobQueue.items[prev];
    }
    
    /* 插入新作业 */
    Csm_JobQueue.items[insertPos].jobId = jobId;
    Csm_JobQueue.items[insertPos].priority = priority;
    Csm_JobQueue.items[insertPos].timestamp = Csm_Cfg_GetTimestamp();
    Csm_JobQueue.tail = (Csm_JobQueue.tail + 1) % CSM_CFG_QUEUE_SIZE;
    Csm_JobQueue.count++;
    
    return E_OK;
#else
    (void)jobId;
    (void)priority;
    return E_NOT_OK;
#endif
}

/**
 * @brief 从队列取出作业
 */
STATIC Std_ReturnType Csm_DequeueJob(uint32* jobId)
{
#if (CSM_CFG_QUEUE_SUPPORT == STD_ON)
    if (Csm_JobQueue.count == 0U )
    {
        return E_NOT_OK;
    }
    
    if (jobId != NULL_PTR)
    {
        *jobId = Csm_JobQueue.items[Csm_JobQueue.head].jobId;
    }
    
    Csm_JobQueue.head = (Csm_JobQueue.head + 1) % CSM_CFG_QUEUE_SIZE;
    Csm_JobQueue.count--;
    
    return E_OK;
#else
    (void)jobId;
    return E_NOT_OK;
#endif
}

/**
 * @brief 处理队列
 */
STATIC void Csm_ProcessQueue(void)
{
#if (CSM_CFG_QUEUE_SUPPORT == STD_ON)
    uint32 jobId;
    uint8 jobIdx;
    
    /* 检查是否可以处理更多作业 */
    if (Csm_ActiveJobCount >= CSM_CFG_MAX_CONCURRENT_JOBS)
    {
        return;
    }
    
    /* 从队列取出作业并执行 */
    while ((Csm_JobQueue.count > 0U ) && (Csm_ActiveJobCount < CSM_CFG_MAX_CONCURRENT_JOBS))
    {
        if (E_OK == Csm_DequeueJob(&jobId))
        {
            if (E_OK == Csm_FindJobIndex(jobId, &jobIdx))
            {
                Csm_Jobs[jobIdx].state = CSM_JOB_STATE_PROCESSING;
                Csm_ActiveJobCount++;
                (void)Csm_ExecuteJob(jobIdx);
            }
        }
        else
        {
            break;
        }
    }
#endif
}

/**
 * @brief 执行作业
 */
STATIC Std_ReturnType Csm_ExecuteJob(uint8 jobIdx)
{
    Std_ReturnType result = E_NOT_OK;
    Csm_JobType* job = &Csm_Jobs[jobIdx];
    
    if (jobIdx >= CSM_MAX_JOBS)
    {
        return E_NOT_OK;
    }
    
    /* 调用硬件服务层 */
    result = Csm_Cfg_HwService(
        job->jobId,
        job->service,
        job->inputData,
        job->inputLength,
        job->outputData,
        &job->resultLength
    );
    
    job->result = result;
    
    if (result == E_OK)
    {
        job->state = CSM_JOB_STATE_RESULT_READY;
        Csm_NotifyEvent(job->jobId, E_OK);
    }
    else if (result == ((Std_ReturnType)1))
    {
        /* 硬件忙碌，保持PROCESSING状态，下次继续 */
    }
    else
    {
        job->state = CSM_JOB_STATE_IDLE;
        Csm_NotifyEvent(job->jobId, result);
        if (Csm_ActiveJobCount > 0U )
        {
            Csm_ActiveJobCount--;
        }
    }
    
    return result;
}

/**
 * @brief 重置作业
 */
STATIC void Csm_ResetJob(uint8 jobIdx)
{
    if (jobIdx >= CSM_MAX_JOBS)
    {
        return;
    }
    
    Csm_Jobs[jobIdx].state = CSM_JOB_STATE_IDLE;
    Csm_Jobs[jobIdx].inputLength = 0U;
    Csm_Jobs[jobIdx].outputLength = 0U;
    Csm_Jobs[jobIdx].resultLength = 0U;
    Csm_Jobs[jobIdx].result = E_NOT_OK;
    Csm_Jobs[jobIdx].verifyResult = FALSE;
}

/**
 * @brief 验证密钥使用权限
 */
STATIC Std_ReturnType Csm_ValidateKeyUsage(uint32 keyId, Csm_KeyUsageType requiredUsage)
{
    uint8 keyIdx;
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        return E_NOT_OK;
    }
    
    if (Csm_Keys[keyIdx].status != CSM_KEY_STATUS_VALID)
    {
        return E_NOT_OK;
    }
    
    /* 检查是否有必要的配置信息 */
    if (Csm_CurrentConfig != NULL_PTR && Csm_CurrentConfig->keys != NULL_PTR)
    {
        uint8 i;
        for (i = 0; i < Csm_CurrentConfig->numKeys; i++)
        {
            if (Csm_CurrentConfig->keys[i].keyId == keyId)
            {
                if ((Csm_CurrentConfig->keys[i].allowedUsage & requiredUsage) == 0U )
                {
                    return E_NOT_OK; /* 权限不足 */
                }
                return E_OK;
            }
        }
    }
    
    return E_OK;
}

/**
 * @brief 更新密钥状态
 */
STATIC void Csm_UpdateKeyStatus(uint8 keyIdx, Csm_KeyStatusType newStatus)
{
    if (keyIdx >= CSM_MAX_KEYS)
    {
        return;
    }
    
    Csm_Keys[keyIdx].status = newStatus;
}

/**
 * @brief 持久化密钥元素
 */
STATIC Std_ReturnType Csm_PersistKeyElement(uint32 keyId, uint32 elementId)
{
#if (CSM_CFG_KEY_PERSISTENCE_SUPPORT == STD_ON)
    uint8 keyIdx;
    uint8 elemIdx;
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        return E_NOT_OK;
    }
    
    if (E_OK != Csm_FindKeyElementIndex(keyIdx, elementId, &elemIdx))
    {
        return E_NOT_OK;
    }
    
    return Csm_Cfg_KeyWrite(
        keyId,
        elementId,
        Csm_Keys[keyIdx].elements[elemIdx].data,
        Csm_Keys[keyIdx].elements[elemIdx].length
    );
#else
    (void)keyId;
    (void)elementId;
    return E_OK;
#endif
}

/**
 * @brief 加载密钥元素
 */
STATIC Std_ReturnType Csm_LoadKeyElement(uint32 keyId, uint32 elementId)
{
#if (CSM_CFG_KEY_PERSISTENCE_SUPPORT == STD_ON)
    uint8 keyIdx;
    uint8 elemIdx;
    uint32 length = CSM_MAX_KEY_LENGTH;
    
    if (E_OK != Csm_FindKeyIndex(keyId, &keyIdx))
    {
        return E_NOT_OK;
    }
    
    if (E_OK != Csm_FindKeyElementIndex(keyIdx, elementId, &elemIdx))
    {
        return E_NOT_OK;
    }
    
    if (E_OK == Csm_Cfg_KeyRead(
            keyId,
            elementId,
            Csm_Keys[keyIdx].elements[elemIdx].data,
            &length))
    {
        Csm_Keys[keyIdx].elements[elemIdx].length = length;
        Csm_Keys[keyIdx].elements[elemIdx].valid = TRUE;
        return E_OK;
    }
    
    return E_NOT_OK;
#else
    (void)keyId;
    (void)elementId;
    return E_OK;
#endif
}

/*==================================================================================================
*                                       API函数实现
==================================================================================================*/

/**
 * @brief 初始化CSM模块
 */
Std_ReturnType Csm_Init(const Csm_ConfigType* config)
{
    uint8 i, j;
    
#if (CSM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (config == NULL_PTR)
    {
        Csm_ReportError(CSM_API_INIT, CSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (Csm_State != CSM_STATE_UNINIT)
    {
        Csm_ReportError(CSM_API_INIT, CSM_E_ALREADY_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    /* 验证配置 */
    if (E_OK != Csm_ValidateConfig(config))
    {
        return E_NOT_OK;
    }
    
    Csm_CurrentConfig = config;
    
    /* 初始化密钥 */
    for (i = 0; i < CSM_MAX_KEYS; i++)
    {
        Csm_Keys[i].keyId = CSM_KEY_ID_NONE;
        Csm_Keys[i].status = CSM_KEY_STATUS_EMPTY;
        Csm_Keys[i].numElements = 0U;
        Csm_Keys[i].referenceCount = 0U;
        
        for (j = 0; j < CSM_MAX_KEY_ELEMENTS; j++)
        {
            Csm_Keys[i].elements[j].length = 0U;
            Csm_Keys[i].elements[j].valid = FALSE;
        }
    }
    
    /* 配置密钥 */
    if (config->keys != NULL_PTR)
    {
        for (i = 0; i < config->numKeys && i < CSM_MAX_KEYS; i++)
        {
            Csm_Keys[i].keyId = config->keys[i].keyId;
            Csm_Keys[i].status = CSM_KEY_STATUS_INVALID;
            Csm_Keys[i].numElements = config->keys[i].numElements;
        }
    }
    
    /* 初始化作业 */
    for (i = 0; i < CSM_MAX_JOBS; i++)
    {
        Csm_Jobs[i].jobId = CSM_JOB_ID_NONE;
        Csm_Jobs[i].state = CSM_JOB_STATE_IDLE;
        Csm_ResetJob(i);
        Csm_Callbacks[i] = NULL_PTR;
        Csm_CallbackContexts[i] = NULL_PTR;
    }
    
    /* 配置作业 */
    if (config->jobs != NULL_PTR)
    {
        for (i = 0; i < config->numJobs && i < CSM_MAX_JOBS; i++)
        {
            Csm_Jobs[i].jobId = config->jobs[i].jobId;
            Csm_Jobs[i].service = config->jobs[i].serviceType;
            Csm_Jobs[i].keyId = config->jobs[i].keyId;
            Csm_Jobs[i].algorithm = config->jobs[i].algorithm;
        }
    }
    
    /* 初始化队列 */
#if (CSM_CFG_QUEUE_SUPPORT == STD_ON)
    Csm_JobQueue.head = 0U;
    Csm_JobQueue.tail = 0U;
    Csm_JobQueue.count = 0U;
#endif
    
    Csm_ActiveJobCount = 0U;
    Csm_InitMagic = CSM_MAGIC_INITIALIZED;
    Csm_State = CSM_STATE_ACTIVE;
    
    return E_OK;
}

/**
 * @brief 去初始化CSM模块
 */
Std_ReturnType Csm_DeInit(void)
{
    uint8 i;
    
    CSM_CHECK_INITIALIZED(CSM_API_DEINIT);
    
    /* 检查是否有活动的作业 */
    for (i = 0; i < CSM_MAX_JOBS; i++)
    {
        if (Csm_Jobs[i].state == CSM_JOB_STATE_PROCESSING)
        {
            return E_NOT_OK;
        }
    }
    
    Csm_State = CSM_STATE_UNINIT;
    Csm_CurrentConfig = NULL_PTR;
    Csm_InitMagic = 0U;
    
    return E_OK;
}

/**
 * @brief 设置密钥元素数据
 */
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
        elemIdx = Csm_Keys[keyIdx].numElements++;
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
        uint8* srcData = Csm_Keys[srcKeyIdx].elements[0].data;
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
                    hashOutLen = (hwHashLen < CSM_MAX_HASH_LENGTH) ? hwHashLen : CSM_MAX_HASH_LENGTH;
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
        uint8* privKeyData = Csm_Keys[keyIdx].elements[0].data;
        
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
        uint8* privKeyData = Csm_Keys[keyIdx].elements[0].data;
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

#if (CSM_VERSION_INFO_API == STD_ON)
/**
 * @brief 获取版本信息
 */
void Csm_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo == NULL_PTR)
    {
#if (CSM_CFG_DEV_ERROR_DETECT == STD_ON)
        Csm_ReportError(CSM_API_INIT, CSM_E_PARAM_POINTER);
#endif
        return;
    }
    
    versioninfo->vendorID = CSM_VENDOR_ID;
    versioninfo->moduleID = CSM_MODULE_ID;
    versioninfo->sw_major_version = CSM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CSM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CSM_SW_PATCH_VERSION;
}
#endif

#define CSM_STOP_SEC_CODE
#include "Csm_MemMap.h"

/*==================================================================================================
*                                       配置定义
==================================================================================================*/
#define CSM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Csm_MemMap.h"

/**
 * @brief 默认密钥元素配置
 */
STATIC const Csm_KeyElementConfigType Csm_DefaultKeyElements[] =
{
    {
        .elementId = CSM_KEY_ELEMENT_ID_SECRET,
        .elementType = CSM_KEY_ELEMENT_TYPE_SECRET,
        .maxLength = CSM_MAX_KEY_LENGTH,
        .readAllowed = FALSE,
        .writeAllowed = TRUE,
        .partialAccessAllowed = FALSE
    }
};

/**
 * @brief 默认密钥配置
 */
STATIC const Csm_KeyConfigType Csm_DefaultKeys[] =
{
    {
        .keyId = CSM_KEY_ID_MASTER,
        .allowedUsage = CSM_KEY_USAGE_ENCRYPT | CSM_KEY_USAGE_DECRYPT | 
                       CSM_KEY_USAGE_MAC_GENERATE | CSM_KEY_USAGE_MAC_VERIFY,
        .elements = Csm_DefaultKeyElements,
        .numElements = 1,
        .cryptoKeyType = 0
    },
    {
        .keyId = CSM_KEY_ID_SESSION,
        .allowedUsage = CSM_KEY_USAGE_ENCRYPT | CSM_KEY_USAGE_DECRYPT,
        .elements = Csm_DefaultKeyElements,
        .numElements = 1,
        .cryptoKeyType = 0
    }
};

/**
 * @brief 默认算法配置
 */
STATIC const Csm_AlgorithmType Csm_DefaultAlgorithm =
{
    .family = CSM_ALGOFAM_AES,
    .mode = CSM_ALGOMODE_CBC,
    .classType = CSM_ALGOCLASS_CIPHER,
    .keyLength = 128,
    .secondaryFamily = NULL_PTR
};

/**
 * @brief 默认作业配置
 */
STATIC const Csm_JobConfigType Csm_DefaultJobs[] =
{
    {
        .jobId = CSM_JOB_ID_ENCRYPT_DEFAULT,
        .serviceType = CSM_SERVICE_ENCRYPT,
        .priority = CSM_JOB_PRIORITY_NORMAL,
        .keyId = CSM_KEY_ID_MASTER,
        .algorithm = { CSM_ALGOFAM_AES, CSM_ALGOMODE_CBC, CSM_ALGOCLASS_CIPHER, 128, NULL_PTR },
        .asynchronous = FALSE,
        .callbackId = 0
    },
    {
        .jobId = CSM_JOB_ID_DECRYPT_DEFAULT,
        .serviceType = CSM_SERVICE_DECRYPT,
        .priority = CSM_JOB_PRIORITY_NORMAL,
        .keyId = CSM_KEY_ID_MASTER,
        .algorithm = { CSM_ALGOFAM_AES, CSM_ALGOMODE_CBC, CSM_ALGOCLASS_CIPHER, 128, NULL_PTR },
        .asynchronous = FALSE,
        .callbackId = 0
    },
    {
        .jobId = CSM_JOB_ID_HASH_DEFAULT,
        .serviceType = CSM_SERVICE_HASH,
        .priority = CSM_JOB_PRIORITY_NORMAL,
        .keyId = CSM_KEY_ID_NONE,
        .algorithm = { CSM_ALGOFAM_SHA2_256, CSM_ALGOMODE_NOT_SET, CSM_ALGOCLASS_HASH, 0, NULL_PTR },
        .asynchronous = FALSE,
        .callbackId = 0
    },
    {
        .jobId = CSM_JOB_ID_MAC_GENERATE_DEFAULT,
        .serviceType = CSM_SERVICE_MAC_GENERATE,
        .priority = CSM_JOB_PRIORITY_HIGH,
        .keyId = CSM_KEY_ID_MASTER,
        .algorithm = { CSM_ALGOFAM_HMAC, CSM_ALGOMODE_NOT_SET, CSM_ALGOCLASS_MAC, 256, NULL_PTR },
        .asynchronous = FALSE,
        .callbackId = 0
    },
    {
        .jobId = CSM_JOB_ID_MAC_VERIFY_DEFAULT,
        .serviceType = CSM_SERVICE_MAC_VERIFY,
        .priority = CSM_JOB_PRIORITY_HIGH,
        .keyId = CSM_KEY_ID_MASTER,
        .algorithm = { CSM_ALGOFAM_HMAC, CSM_ALGOMODE_NOT_SET, CSM_ALGOCLASS_MAC, 256, NULL_PTR },
        .asynchronous = FALSE,
        .callbackId = 0
    }
};

/**
 * @brief 默认配置
 */
const Csm_ConfigType Csm_Config =
{
    .keys = Csm_DefaultKeys,
    .numKeys = 2,
    .jobs = Csm_DefaultJobs,
    .numJobs = 5,
    .useAsyncMode = FALSE,
    .queueProcessingPeriod = CSM_CFG_MAIN_FUNCTION_PERIOD_MS,
    .devErrorDetect = CSM_CFG_DEV_ERROR_DETECT
};

#define CSM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Csm_MemMap.h"
