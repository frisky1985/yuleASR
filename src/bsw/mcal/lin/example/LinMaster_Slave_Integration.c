/**
 * @file LinMaster_Slave_Integration.c
 * @brief LinMaster与LinSlave集成示例
 * @version 1.0.0
 * 
 * 演示Master和Slave的协同工作:
 * - Master发送调度表
 * - Slave响应请求
 * - 诊断通信示例
 */

#include "LinMaster.h"
#include "LinMaster_Schedule.h"
#include "LinMaster_Diagnostic.h"
#include "LinSlave.h"
#include "LinSlave_CfgTable.h"
#include <stdio.h>
#include <string.h>

/* ============================================
 * Master 配置
 * ============================================ */

/* Master调度表条目 */
static const LinMaster_ScheduleEntryType MasterScheduleEntries[] = {
    /* {Pid, DataLen, DelayMs, EntryType} */
    {0x11, 8, 20, LINMASTER_ENTRY_UNCONDITIONAL},      /* 从机1信号报文 */
    {0x12, 8, 20, LINMASTER_ENTRY_UNCONDITIONAL},      /* 从机2信号报文 */
    {0x3C, 8, 50, LINMASTER_ENTRY_DIAGNOSTIC_MASTER_REQ}, /* 诊断请求 */
    {0x3D, 8, 50, LINMASTER_ENTRY_DIAGNOSTIC_SLAVE_RESP}, /* 诊断响应 */
};

static const LinMaster_ScheduleTableType MasterScheduleTable = {
    MasterScheduleEntries,
    sizeof(MasterScheduleEntries) / sizeof(MasterScheduleEntries[0]),
    0,      /* 条目索引 */
    FALSE,  /* 未运行 */
    TRUE    /* 循环模式 */
};

/* Master配置 */
static const LinMaster_ConfigType MasterConfig = {
    19200,  /* 波特率 */
    8,      /* 数据长度 */
    50,     /* 超时ms */
    NULL,   /* 错误回调 */
    NULL    /* 接收回调 */
};

/* ============================================
 * Slave 配置
 * ============================================ */

/* Slave Unconditional Frame配置 */
static const LinSlave_UnconditionalFrameConfigType SlaveUnconditionalFrames[] = {
    {
        0x11,                           /* PID */
        8,                              /* 长度 */
        LINSLAVE_DIR_TX,                /* 方向: Slave发送 */
        LINSLAVE_CHECKSUM_CLASSIC,      /* 校验类型 */
        NULL,                           /* RX回调 */
        NULL,                           /* TX回调 */
        NULL                            /* 用户数据 */
    },
    {
        0x12,                           /* PID */
        8,                              /* 长度 */
        LINSLAVE_DIR_TX,                /* 方向: Slave发送 */
        LINSLAVE_CHECKSUM_CLASSIC,      /* 校验类型 */
        NULL,
        NULL,
        NULL
    }
};

/* Slave配置表 */
static const LinSlave_ConfigTableType SlaveConfigTable = {
    2, 1, 0,                        /* 版本 */
    0x05,                           /* 节点ID */
    19200,                          /* 波特率 */
    2,                              /* Unconditional数量 */
    SlaveUnconditionalFrames,
    0, NULL,                        /* Event Frame */
    0, NULL,                        /* Sporadic Frame */
    NULL,                           /* 诊断配置 */
    TRUE,                           /* 使用诊断 */
    NULL                            /* 错误回调 */
};

/* ============================================
 * 仿真HAL层 - 用于测试
 * ============================================ */

static uint8 SimulationBuffer[256];
static uint8 SimulationIndex = 0;
static boolean IsMasterTx = FALSE;

void Hal_SimulateBusTransfer(void)
{
    /* 模拟总线传输: Master发送的数据被Slave接收 */
    uint8 i;
    for (i = 0; i < SimulationIndex; i++) {
        if (IsMasterTx) {
            /* Master发送，Slave接收 */
            LinSlave_RxInterruptHandler(SimulationBuffer[i]);
        } else {
            /* Slave发送，Master接收 */
            LinMaster_RxInterruptHandler(SimulationBuffer[i]);
        }
    }
    SimulationIndex = 0;
}

void Hal_MasterSendByte(uint8 Data)
{
    SimulationBuffer[SimulationIndex++] = Data;
    IsMasterTx = TRUE;
}

/* ============================================
 * 回调函数
 * ============================================ */

static uint8 ReceivedData[8];
static boolean DataReceived = FALSE;

void Master_RxCallback(uint8 Pid, const uint8* DataPtr, uint8 Length)
{
    memcpy(ReceivedData, DataPtr, Length);
    DataReceived = TRUE;
    printf("Master收到响应 - PID: 0x%02X, 数据: ", Pid);
    uint8 i;
    for (i = 0; i < Length; i++) {
        printf("%02X ", DataPtr[i]);
    }
    printf("\n");
}

void Slave_RxCallback(uint8 FrameIndex, const uint8* DataPtr, uint8 Length, LinSlave_DirectionType Dir)
{
    printf("Slave收到请求 - 帧索引: %d, 长度: %d\n", FrameIndex, Length);
}

void Slave_TxCallback(uint8 FrameIndex, uint8* DataPtr, uint8* LengthPtr, LinSlave_DirectionType Dir)
{
    /* 填充测试数据 */
    *LengthPtr = 8;
    DataPtr[0] = 0x01;
    DataPtr[1] = 0x02;
    DataPtr[2] = 0x03;
    DataPtr[3] = 0x04;
    DataPtr[4] = 0x05;
    DataPtr[5] = 0x06;
    DataPtr[6] = 0x07;
    DataPtr[7] = 0x08;
    printf("Slave发送响应 - 帧索引: %d\n", FrameIndex);
}

/* ============================================
 * 诊断回调
 * ============================================ */

void Diag_ResponseCallback(const LinMaster_Diag_ResponseType* Response)
{
    if (Response->IsNegative) {
        printf("诊断负响应 - NRC: 0x%02X\n", Response->Nrc);
    } else {
        printf("诊断正响应 - SID: 0x%02X, 长度: %d\n", 
               Response->ResponseSid, Response->Length);
    }
}

/* ============================================
 * 主函数
 * ============================================ */

int main(void)
{
    printf("=== LinMaster/Slave 集成测试 ===\n\n");
    
    /* 初始化Master */
    printf("1. 初始化Master...\n");
    LinMaster_Init(&MasterConfig);
    LinMaster_Schedule_Init(&MasterScheduleTable);
    LinMaster_RegisterRxCallback(Master_RxCallback);
    
    /* 初始化Slave */
    printf("2. 初始化Slave...\n");
    LinSlave_CfgTable_Init(&SlaveConfigTable);
    LinSlave_InitWithConfigTable(&SlaveConfigTable);
    
    /* 注册Slave回调 */
    LinSlave_CfgTableType* CfgTable = (LinSlave_CfgTableType*)&SlaveConfigTable;
    CfgTable->UnconditionalFrameConfigs[0].TxCallback = (LinSlave_UnconditionalTxCallbackType)Slave_TxCallback;
    
    /* 启动调度表 */
    printf("3. 启动调度表...\n\n");
    LinMaster_Schedule_Start();
    
    /* 运行几个调度周期 */
    uint8 cycle;
    for (cycle = 0; cycle < 3; cycle++) {
        printf("--- 调度周期 %d ---\n", cycle + 1);
        
        /* 执行调度表 */
        uint8 entryIdx;
        for (entryIdx = 0; entryIdx < 2; entryIdx++) {
            const LinMaster_ScheduleEntryType* entry = 
                LinMaster_Schedule_GetCurrentEntry();
            
            if (entry != NULL) {
                printf("Master发送报文头 - PID: 0x%02X\n", entry->Pid);
                
                /* 模拟发送 */
                LinMaster_SendHeader(entry->Pid);
                
                /* 模拟总线传输 */
                Hal_SimulateBusTransfer();
                
                /* 运行Slave状态机 */
                LinSlave_MainFunction();
                
                /* 模拟Slave响应 */
                if (entry->EntryType == LINMASTER_ENTRY_UNCONDITIONAL) {
                    uint8 i;
                    for (i = 0; i < entry->DataLength; i++) {
                        /* Slave发送数据 */
                    }
                    Hal_SimulateBusTransfer();
                }
                
                /* 进入下一条目 */
                LinMaster_Schedule_Process();
            }
        }
        printf("\n");
    }
    
    /* 诊断测试 */
    printf("4. 诊断通信测试...\n");
    
    LinMaster_Diag_Init();
    LinMaster_Diag_RegisterCallback(Diag_ResponseCallback);
    
    /* 发送诊断会话控制请求 */
    LinMaster_Diag_RequestType request;
    request.Sid = 0x10;  /* 会话控制 */
    request.SubFunction = 0x03;  /* 扩展会话 */
    request.Length = 2;
    request.IsFunctional = FALSE;
    request.Data[0] = 0x10;
    request.Data[1] = 0x03;
    
    printf("发送诊断请求 - SID: 0x%02X\n", request.Sid);
    LinMaster_Diag_SendRequest(&request);
    
    /* 运行诊断状态机 */
    uint8 i;
    for (i = 0; i < 10; i++) {
        LinMaster_Diag_MainFunction();
        LinSlave_MainFunction();
    }
    
    printf("\n=== 测试完成 ===\n");
    
    return 0;
}
