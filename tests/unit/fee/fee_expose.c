/**
 * @file fee_expose.c
 * @brief 将 Fee.c 中的 static 内部状态暴露给测试代码
 *
 * 通过 #define static 为空使内部变量可见,
 * 并提供 setter/getter 函数让测试安全地操作内部状态.
 */

#define static
#include "../src/Fee.c"
#undef static

/* ---- 测试辅助 API ---- */

/** 强制设置 Fee_CurrentJob.JobState, 用于加速状态机 */
void FeeTest_SetJobState(uint8 state)
{
    Fee_CurrentJob.JobState = state;
}

/** 强制设置 Fee_FlsJob.JobState */
void FeeTest_SetFlsJobState(uint8 state)
{
    Fee_FlsJob.JobState = state;
}

/** 获取 Fee_CurrentJob.JobType (验证当前 job) */
uint8 FeeTest_GetJobType(void)
{
    return (uint8)Fee_CurrentJob.JobType;
}

/** 获取 Fee_CurrentJob.JobState (验证状态机走到哪) */
uint8 FeeTest_GetJobState(void)
{
    return (uint8)Fee_CurrentJob.JobState;
}

/** 获取 Fee_Initialized (验证 Init 是否成功) */
boolean FeeTest_IsInitialized(void)
{
    return Fee_Initialized;
}

/** 直接注入当前 job 类型 / 模块状态, 用于驱动 ProcessJob 的 default 分支 */
void FeeTest_SetJobType(uint8 type)
{
    Fee_CurrentJob.JobType = type;
}

void FeeTest_SetModuleStatus(uint8 status)
{
    Fee_ModuleStatus = (Fee_StatusType)status;
}

/** 注入/恢复 ConfigPtr, 用于驱 Fee_Write 的 "无配置" 分支 */
void FeeTest_SetConfigPtr(const Fee_ConfigType* cfg)
{
    Fee_ConfigPtr = cfg;
}

/** 获取 Fee_CurrentJob.JobResult (验证 FinishJob 后结果) */
uint8 FeeTest_GetJobResult(void)
{
    return (uint8)Fee_CurrentJob.JobResult;
}

/* ---- GC 路径辅助 API ---- */

/** 构造一个指定 sector 的状态, 用于伪造 IsGcNeeded=TRUE 条件 */
void FeeTest_SetSectorInfo(uint8 idx,
                           boolean isValid,
                           boolean isActive,
                           uint32 freeSpace,
                           uint32 sectorSize,
                           uint8 sectorState)
{
    Fee_SectorInfo[idx].IsValid    = isValid;
    Fee_SectorInfo[idx].IsActive   = isActive;
    Fee_SectorInfo[idx].FreeSpace  = freeSpace;
    Fee_SectorInfo[idx].SectorSize = sectorSize;
    Fee_SectorInfo[idx].SectorState = sectorState;
    Fee_SectorInfo[idx].SectorStartAddress = (0x10000u * (uint32)(idx + 1u));
}

/** 清所有 sector (使 IsGcNeeded=FALSE) */
void FeeTest_ResetAllSectors(void)
{
    for (uint8 i = 0u; i < FEE_NUM_SECTORS; i++) {
        Fee_SectorInfo[i].IsValid    = FALSE;
        Fee_SectorInfo[i].IsActive   = FALSE;
        Fee_SectorInfo[i].FreeSpace  = 0u;
        Fee_SectorInfo[i].SectorSize = 0u;
        Fee_SectorInfo[i].SectorState = FEE_BLOCK_STATE_EMPTY;
        Fee_SectorInfo[i].SectorStartAddress = 0u;
        Fee_SectorInfo[i].NextWriteAddress = 0u;
        Fee_SectorInfo[i].EraseCycleCount   = 0u;
    }
}

/** 获取/设置 GcControl 字段 */
uint8 FeeTest_GetGcState(void)           { return Fee_GcControl.GcState; }
void  FeeTest_SetGcState(uint8 s)        { Fee_GcControl.GcState = s; }
uint8 FeeTest_GetGcSourceSector(void)    { return Fee_GcControl.SourceSectorIndex; }
uint8 FeeTest_GetGcTargetSector(void)    { return Fee_GcControl.TargetSectorIndex; }

/** 计数器 */
uint32 FeeTest_GetGcCycleCounter(void)    { return Fee_GcCycleCounter; }
uint32 FeeTest_GetEraseCycleCounter(void) { return Fee_EraseCycleCounter; }

/** ModuleStatus 直接读写 (验证 GC 与正常 job 的互斥) */
uint8 FeeTest_GetModuleStatus(void)
{
    return (uint8)Fee_ModuleStatus;
}

/** IsGcNeeded 返回值验证 — 直接调内部函数 */
boolean FeeTest_CallIsGcNeeded(void)
{
    return Fee_IsGcNeeded();
}

/** FLS job state — 区别于 CurrentJob (正常 job) */
uint8 FeeTest_GetFlsJobState(void)
{
    return Fee_FlsJob.JobState;
}
