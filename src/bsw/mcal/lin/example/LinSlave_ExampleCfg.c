/**
 * @file LinSlave_ExampleCfg.c
 * @brief LinSlave v2.0 配置表示例
 * @version 2.0.0
 * 
 * 示例配置表:
 * - PID 0x10: 信号报文 (接收)
 * - PID 0x20: 信号报文 (发送)
 * - PID 0x30: 诊断报文 (通过UDS)
 */

#include "LinSlave.h"

/* 前向声明 */
static void SignalRxCallback(const LinSlave_PduInfoType* PduInfo);
static void SignalTxCallback(const LinSlave_PduInfoType* PduInfo);
static void DiagCallback(const LinSlave_PduInfoType* PduInfo);
static void FrameErrorCallback(uint8 Pid, LinSlave_FrameErrorType ErrorType);

/* PID配置条目 */
static const LinSlave_PidConfigEntryType PidEntry_0x10 = {
    0x10,                           /* PID */
    LINSLAVE_DIR_RX,                /* 方向: 接收 */
    LINSLAVE_CSUM_CLASSIC,          /* 校验和: 经典 */
    LINSLAVE_MSG_TYPE_SIGNAL,       /* 消息类型: 信号 */
    LINSLAVE_TP_NONE,               /* TP类型: 无 */
    8,                              /* 数据长度: 8字节 */
    {
        .DataCallback = SignalRxCallback  /* 数据回调 */
    },
    FrameErrorCallback              /* 错误回调 */
};

static const LinSlave_PidConfigEntryType PidEntry_0x11 = {
    0x11,
    LINSLAVE_DIR_TX,
    LINSLAVE_CSUM_CLASSIC,
    LINSLAVE_MSG_TYPE_SIGNAL,
    LINSLAVE_TP_NONE,
    8,
    {
        .DataCallback = SignalTxCallback
    },
    FrameErrorCallback
};

static const LinSlave_PidConfigEntryType PidEntry_0x12 = {
    0x12,
    LINSLAVE_DIR_RX_TX,
    LINSLAVE_CSUM_ENHANCED,
    LINSLAVE_MSG_TYPE_SIGNAL,
    LINSLAVE_TP_NONE,
    8,
    {
        .DataCallback = SignalRxCallback
    },
    FrameErrorCallback
};

/* 诊断报文配置 */
static const LinSlave_PidConfigEntryType PidEntry_DiagRequest = {
    0x3C,                           /* 诊断请求PID */
    LINSLAVE_DIR_RX,                /* 方向: 接收 */
    LINSLAVE_CSUM_CLASSIC,
    LINSLAVE_MSG_TYPE_DIAGNOSTIC,   /* 消息类型: 诊断 */
    LINSLAVE_TP_LIN_TP,             /* TP类型: LIN_TP */
    8,
    {
        .DiagCallback = DiagCallback
    },
    FrameErrorCallback
};

static const LinSlave_PidConfigEntryType PidEntry_DiagResponse = {
    0x3D,                           /* 诊断响应PID */
    LINSLAVE_DIR_TX,
    LINSLAVE_CSUM_CLASSIC,
    LINSLAVE_MSG_TYPE_DIAGNOSTIC,
    LINSLAVE_TP_LIN_TP,
    8,
    {
        .DiagCallback = DiagCallback
    },
    FrameErrorCallback
};

/* 配置表条目数组 */
static const LinSlave_PidConfigType* ConfigEntries[] = {
    (const LinSlave_PidConfigType*)&PidEntry_0x10,
    (const LinSlave_PidConfigType*)&PidEntry_0x11,
    (const LinSlave_PidConfigType*)&PidEntry_0x12,
    (const LinSlave_PidConfigType*)&PidEntry_DiagRequest,
    (const LinSlave_PidConfigType*)&PidEntry_DiagResponse
};

/* 配置表实例 */
const LinSlave_ConfigTableType LinSlave_ConfigTable = {
    LINSLAVE_CFGTABLE_VERSION_MAJOR,
    LINSLAVE_CFGTABLE_VERSION_MINOR,
    LINSLAVE_CFGTABLE_VERSION_PATCH,
    0x01,                           /* 节点ID */
    19200,                          /* 波特率 */
    sizeof(ConfigEntries) / sizeof(ConfigEntries[0]),  /* 条目数 */
    ConfigEntries                   /* 条目数组 */
};

/* 回调函数实现 */
static void SignalRxCallback(const LinSlave_PduInfoType* PduInfo)
{
    /* 处理接收到的信号数据 */
    (void)PduInfo;
    /* 用户实现: 应用逻辑处理 */
}

static void SignalTxCallback(const LinSlave_PduInfoType* PduInfo)
{
    /* 准备发送的信号数据 */
    (void)PduInfo;
    /* 用户实现: 填充发送数据 */
}

static void DiagCallback(const LinSlave_PduInfoType* PduInfo)
{
    /* UDS诊断回调 - 由UDS框架处理 */
    (void)PduInfo;
}

static void FrameErrorCallback(uint8 Pid, LinSlave_FrameErrorType ErrorType)
{
    /* 帧错误处理 */
    (void)Pid;
    (void)ErrorType;
}

/* 使用示例
void LinSlave_InitExample(void)
{
    LinSlave_StatusType status;
    
    // 使用配置表方式初始化
    status = LinSlave_InitWithConfigTable(&LinSlave_ConfigTable);
    
    if (status != LINSLAVE_OK) {
        // 初始化失败处理
    }
}

void LinSlave_MainTask(void)
{
    // 周期调用主函数 (10ms周期)
    LinSlave_MainFunction();
}
*/
