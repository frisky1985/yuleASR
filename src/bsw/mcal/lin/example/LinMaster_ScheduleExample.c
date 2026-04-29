/**
 * @file LinMaster_ScheduleExample.c
 * @brief LinMaster调度表配置示例
 * @note 展示如何配置和使用调度表
 */

#include "LinMaster_Schedule.h"
#include <stdio.h>

/* ==========================================
 * 调度表配置示例
 * ========================================== */

/* 示例1: 基本无条件帧调度表 */
static const LinMaster_ScheduleEntryType ExampleNormalTableEntries[] = {
    /* { PID, DataLength, DelayMs, EntryType, Direction, ChecksumType } */
    { 0x10, 8, 10, LINMASTER_ENTRY_UNCONDITIONAL, LINMASTER_FRAME_DIR_RX, LINMASTER_CHECKSUM_ENHANCED },  /* 从机1数据 */
    { 0x11, 8, 10, LINMASTER_ENTRY_UNCONDITIONAL, LINMASTER_FRAME_DIR_TX, LINMASTER_CHECKSUM_ENHANCED },  /* 主机发送 */
    { 0x12, 8, 10, LINMASTER_ENTRY_UNCONDITIONAL, LINMASTER_FRAME_DIR_RX, LINMASTER_CHECKSUM_ENHANCED },  /* 从机2数据 */
    { 0x13, 8, 50, LINMASTER_ENTRY_UNCONDITIONAL, LINMASTER_FRAME_DIR_RX, LINMASTER_CHECKSUM_ENHANCED },  /* 从机3数据，更长延迟 */
};

static LinMaster_ScheduleTableType ExampleNormalTable = {
    .Entries = ExampleNormalTableEntries,
    .EntryCount = sizeof(ExampleNormalTableEntries) / sizeof(ExampleNormalTableEntries[0]),
    .EntryIndex = 0,
    .IsRunning = FALSE,
    .IsCyclic = TRUE,  /* 循环模式 */
    .LastEntryTime = 0,
    .Name = "NormalTable"
};

/* 示例2: 诊断调度表 */
static const LinMaster_ScheduleEntryType ExampleDiagnosticTableEntries[] = {
    { 0x3C, 8, 20, LINMASTER_ENTRY_DIAGNOSTIC_MASTER_REQ, LINMASTER_FRAME_DIR_TX, LINMASTER_CHECKSUM_CLASSIC },  /* 诊断请求 */
    { 0x3D, 8, 100, LINMASTER_ENTRY_DIAGNOSTIC_SLAVE_RESP, LINMASTER_FRAME_DIR_RX, LINMASTER_CHECKSUM_CLASSIC }, /* 等待响应 */
};

static LinMaster_ScheduleTableType ExampleDiagnosticTable = {
    .Entries = ExampleDiagnosticTableEntries,
    .EntryCount = sizeof(ExampleDiagnosticTableEntries) / sizeof(ExampleDiagnosticTableEntries[0]),
    .EntryIndex = 0,
    .IsRunning = FALSE,
    .IsCyclic = FALSE,  /* 单次执行 */
    .LastEntryTime = 0,
    .Name = "DiagnosticTable"
};

/* 示例3: 混合调度表(含事件触发帧) */
static const LinMaster_ScheduleEntryType ExampleEventTableEntries[] = {
    { 0x10, 8, 10, LINMASTER_ENTRY_UNCONDITIONAL, LINMASTER_FRAME_DIR_RX, LINMASTER_CHECKSUM_ENHANCED },
    { 0x14, 8, 15, LINMASTER_ENTRY_EVENT_TRIGGERED, LINMASTER_FRAME_DIR_RX, LINMASTER_CHECKSUM_ENHANCED },  /* 事件触发帧 */
    { 0x15, 8, 20, LINMASTER_ENTRY_SPORADIC, LINMASTER_FRAME_DIR_TX, LINMASTER_CHECKSUM_ENHANCED },        /* 偶发帧 */
};

static LinMaster_ScheduleTableType ExampleEventTable = {
    .Entries = ExampleEventTableEntries,
    .EntryCount = sizeof(ExampleEventTableEntries) / sizeof(ExampleEventTableEntries[0]),
    .EntryIndex = 0,
    .IsRunning = FALSE,
    .IsCyclic = TRUE,
    .LastEntryTime = 0,
    .Name = "EventTable"
};

/* ==========================================
 * 回调函数示例
 * ========================================== */

/**
 * @brief 条目执行回调
 */
static void Example_EntryCallback(
    const LinMaster_ScheduleEntryType* Entry,
    uint8 EntryIndex,
    LinMaster_EntryStateType State)
{
    const char* stateStr[] = {
        "IDLE", "PENDING", "EXECUTING", "COMPLETED", "ERROR"
    };
    const char* typeStr[] = {
        "UNCONDITIONAL", "EVENT_TRIGGERED", "SPORADIC",
        "DIAG_MASTER_REQ", "DIAG_SLAVE_RESP"
    };
    
    printf("[Schedule] Entry[%d] PID=0x%02X Type=%s State=%s\n",
           EntryIndex, Entry->Pid, typeStr[Entry->EntryType], stateStr[State]);
    
    /* 根据条目类型调用LinMaster API发送帧 */
    switch (Entry->EntryType) {
        case LINMASTER_ENTRY_UNCONDITIONAL:
            if (Entry->Direction == LINMASTER_FRAME_DIR_TX) {
                /* 主机发送数据 */
                /* LinMaster_SendFrame(Entry->Pid, txData, Entry->DataLength, Entry->ChecksumType); */
            } else {
                /* 主机只发送头部，等待从机响应 */
                /* LinMaster_ReceiveFrame(Entry->Pid, Entry->DataLength, Entry->ChecksumType); */
            }
            break;
            
        case LINMASTER_ENTRY_DIAGNOSTIC_MASTER_REQ:
            /* 发送诊断请求 */
            /* LinMaster_SendFrame(LINMASTER_DIAG_MASTER_REQ_PID, diagData, 8, LINMASTER_CHECKSUM_CLASSIC); */
            break;
            
        case LINMASTER_ENTRY_DIAGNOSTIC_SLAVE_RESP:
            /* 等待诊断响应 */
            /* LinMaster_ReceiveFrame(LINMASTER_DIAG_SLAVE_RESP_PID, 8, LINMASTER_CHECKSUM_CLASSIC); */
            break;
            
        case LINMASTER_ENTRY_EVENT_TRIGGERED:
            /* 发送事件触发头部 */
            /* LinMaster_SendHeader(Entry->Pid); */
            break;
            
        case LINMASTER_ENTRY_SPORADIC:
            /* 检查是否有数据需要发送 */
            /* 如果有数据: LinMaster_SendFrame(...) */
            /* 如果无数据: 跳过该条目 */
            break;
            
        default:
            break;
    }
}

/**
 * @brief 调度表完成回调(单次模式)
 */
static void Example_TableCompleteCallback(const LinMaster_ScheduleTableType* Table)
{
    printf("[Schedule] Table '%s' completed!\n", Table->Name);
    
    /* 可在此处切换回正常调度表 */
    /* LinMaster_Schedule_SwitchTable(&ExampleNormalTable, TRUE); */
}

/**
 * @brief 调度表切换回调
 */
static void Example_TableSwitchCallback(
    const LinMaster_ScheduleTableType* OldTable,
    const LinMaster_ScheduleTableType* NewTable)
{
    const char* oldName = (OldTable != NULL) ? OldTable->Name : "NULL";
    printf("[Schedule] Switching table: '%s' -> '%s'\n", oldName, NewTable->Name);
}

/* ==========================================
 * 使用示例
 * ========================================== */

void Example_ScheduleInit(void)
{
    /* 初始化调度模块，使用正常调度表 */
    if (LinMaster_Schedule_Init(&ExampleNormalTable) == E_OK) {
        printf("Schedule module initialized\n");
    }
    
    /* 注册回调函数 */
    LinMaster_Schedule_RegisterEntryCallback(Example_EntryCallback);
    LinMaster_Schedule_RegisterTableCompleteCallback(Example_TableCompleteCallback);
    LinMaster_Schedule_RegisterTableSwitchCallback(Example_TableSwitchCallback);
}

void Example_StartNormalSchedule(void)
{
    /* 启动正常调度表(循环模式) */
    if (LinMaster_Schedule_Start() == E_OK) {
        printf("Normal schedule started\n");
    }
}

void Example_SwitchToDiagnostic(void)
{
    /* 暂停当前调度表 */
    LinMaster_Schedule_Pause();
    
    /* 切换到诊断调度表(单次执行，完成后自动回调) */
    if (LinMaster_Schedule_SwitchTable(&ExampleDiagnosticTable, TRUE) == E_OK) {
        printf("Switched to diagnostic table\n");
    }
}

void Example_AddDynamicEntry(void)
{
    /* 动态添加调度条目(在运行时) */
    LinMaster_ScheduleEntryType newEntry = {
        .Pid = 0x20,
        .DataLength = 8,
        .DelayMs = 30,
        .EntryType = LINMASTER_ENTRY_UNCONDITIONAL,
        .Direction = LINMASTER_FRAME_DIR_RX,
        .ChecksumType = LINMASTER_CHECKSUM_ENHANCED
    };
    
    if (LinMaster_Schedule_AddEntry(&newEntry) == E_OK) {
        printf("Added dynamic entry\n");
    }
}

void Example_ScheduleMainFunction(void)
{
    /* 在主循环中定期调用(1ms) */
    LinMaster_Schedule_Process();
}

void Example_PrintScheduleStatus(void)
{
    const LinMaster_ScheduleTableType* currentTable;
    const LinMaster_ScheduleEntryType* currentEntry;
    
    currentTable = LinMaster_Schedule_GetCurrentTable();
    currentEntry = LinMaster_Schedule_GetCurrentEntry();
    
    if (currentTable != NULL) {
        printf("Current Table: %s\n", currentTable->Name);
        printf("Entry Index: %d/%d\n", 
               LinMaster_Schedule_GetEntryIndex(),
               currentTable->EntryCount);
        printf("Is Running: %s\n", LinMaster_Schedule_IsRunning() ? "Yes" : "No");
        printf("Is Cyclic: %s\n", LinMaster_Schedule_IsCyclic() ? "Yes" : "No");
        
        if (currentEntry != NULL) {
            printf("Current PID: 0x%02X\n", currentEntry->Pid);
        }
        
        printf("Elapsed Time: %lu ms\n", (unsigned long)LinMaster_Schedule_GetElapsedTime());
        printf("Remaining Time: %lu ms\n", (unsigned long)LinMaster_Schedule_GetRemainingTime());
    }
}
