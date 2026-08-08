/*==================================================================================================
 * 作业/密码服务 API 实现
 * 自动拆分自 Csm.c
 *================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

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
