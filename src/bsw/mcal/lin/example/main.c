/**
 * @file main.c
 * @brief LinSlave 使用示例
 * @version 1.0.0
 */

#include <stdio.h>
#include <string.h>
#include "LinSlave.h"
#include "LinSlave_Pid.h"

/* 节点配置 */
#define MY_NODE_ID          5
#define RESPONSE_DATA_LEN   4

/* 响应数据 */
static uint8 ResponseData[RESPONSE_DATA_LEN] = {0x11, 0x22, 0x33, 0x44};

/**
 * 接收回调函数
 * 当收到匹配的LIN报文时调用
 */
void MyRxCallback(uint8 Pid, uint8* ResponseDataPtr, uint8* ResponseLengthPtr, uint8* ChecksumTypePtr)
{
    uint8 id = LinSlave_ExtractId(Pid);
    
    printf("[Callback] Received PID 0x%02X (ID=%d)\n", Pid, id);
    
    /* 设置响应数据 */
    memcpy(ResponseDataPtr, ResponseData, RESPONSE_DATA_LEN);
    *ResponseLengthPtr = RESPONSE_DATA_LEN;
    *ChecksumTypePtr = 1;  /* 增强校验和 */
}

/**
 * 错误回调函数
 */
void MyErrorCallback(LinSlave_ErrorType ErrorCode, uint8 Pid)
{
    const char* errorStr[] = {
        "NONE", "BREAK", "SYNC", "PID", "CHECKSUM",
        "TIMEOUT", "FRAMING", "OVERRUN"
    };
    
    printf("[Error] Error=%s, PID=0x%02X\n", 
           (ErrorCode <= LINSLAVE_ERROR_OVERRUN) ? errorStr[ErrorCode] : "UNKNOWN",
           Pid);
}

/**
 * 模拟主机发送报文
 */
void SimulateMasterFrame(uint8 targetId)
{
    uint8 pid = LinSlave_CalculatePid(targetId);
    
    printf("\n[Master] Sending frame to ID=%d (PID=0x%02X)\n", targetId, pid);
    
    /* 模拟 Break */
    LinSlave_BreakDetected();
    
    /* 模拟 Sync */
    LinSlave_RxInterruptHandler(0x55);
    
    /* 模拟 PID */
    LinSlave_RxInterruptHandler(pid);
}

int main(void)
{
    LinSlave_StatusType status;
    LinSlave_ConfigType config;
    
    printf("========================================\n");
    printf("   LinSlave Example Program\n");
    printf("========================================\n");
    
    /* 配置初始化 */
    config.NodeId = MY_NODE_ID;
    config.BaudRate = 1;
    config.ResponseLength = RESPONSE_DATA_LEN;
    config.ChecksumType = 1;  /* 增强 */
    config.BreakThresholdUs = 1000;
    config.TimeoutMs = 100;
    
    /* 初始化 LinSlave */
    status = LinSlave_Init(&config);
    if (status != LINSLAVE_OK) {
        printf("Failed to initialize LinSlave!\n");
        return 1;
    }
    printf("[Init] LinSlave initialized successfully\n");
    printf("       Node ID: %d\n", MY_NODE_ID);
    printf("       BaudRate: %d\n", config.BaudRate);
    
    /* 注册回调 */
    LinSlave_RegisterRxCallback(MyRxCallback);
    LinSlave_RegisterErrorCallback(MyErrorCallback);
    printf("[Init] Callbacks registered\n");
    
    /* 测试 1: 模拟主机发送报文给本节点 */
    printf("\n--- Test 1: Master sends to this node (ID=%d) ---\n", MY_NODE_ID);
    SimulateMasterFrame(MY_NODE_ID);
    
    /* 测试 2: 模拟主机发送报文给其他节点 */
    printf("\n--- Test 2: Master sends to other node (ID=%d) ---\n", MY_NODE_ID + 1);
    SimulateMasterFrame(MY_NODE_ID + 1);
    
    /* 测试 3: 模拟错误的Sync */
    printf("\n--- Test 3: Invalid Sync byte ---\n");
    LinSlave_BreakDetected();
    LinSlave_RxInterruptHandler(0x56);  /* 错误的Sync */
    
    /* 测试 4: 模拟错误的PID */
    printf("\n--- Test 4: Invalid PID ---\n");
    LinSlave_BreakDetected();
    LinSlave_RxInterruptHandler(0x55);  /* Sync */
    LinSlave_RxInterruptHandler(0xFF);  /* 无效的PID */
    
    /* 反初始化 */
    LinSlave_DeInit();
    printf("\n[Exit] LinSlave deinitialized\n");
    
    printf("\n========================================\n");
    printf("   Example completed successfully\n");
    printf("========================================\n");
    
    return 0;
}
