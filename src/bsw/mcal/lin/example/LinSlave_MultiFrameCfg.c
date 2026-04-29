/**
 * @file LinSlave_MultiFrameCfg.c
 * @brief LinSlave 多Unconditional Frame配置示例
 * @version 2.1.0
 * 
 * 示例配置:
 * - 8个Unconditional Frames (信号报文)
 * - 1个Event Triggered Frame
 * - 1个Sporadic Frame  
 * - 诊断报文支持
 */

#include "LinSlave.h"

/* ============================================
 * 回调函数前向声明
 * ============================================ */

/* Unconditional Frame 接收回调 */
static void UnconditionalRxCallback_0(uint8 FrameIndex, const uint8* DataPtr, uint8 Length, void* UserData);
static void UnconditionalRxCallback_1(uint8 FrameIndex, const uint8* DataPtr, uint8 Length, void* UserData);
static void UnconditionalRxCallback_2(uint8 FrameIndex, const uint8* DataPtr, uint8 Length, void* UserData);
static void UnconditionalRxCallback_3(uint8 FrameIndex, const uint8* DataPtr, uint8 Length, void* UserData);

/* Unconditional Frame 发送回调 */
static void UnconditionalTxCallback_4(uint8 FrameIndex, uint8* DataPtr, uint8* LengthPtr, void* UserData);
static void UnconditionalTxCallback_5(uint8 FrameIndex, uint8* DataPtr, uint8* LengthPtr, void* UserData);
static void UnconditionalTxCallback_6(uint8 FrameIndex, uint8* DataPtr, uint8* LengthPtr, void* UserData);
static void UnconditionalTxCallback_7(uint8 FrameIndex, uint8* DataPtr, uint8* LengthPtr, void* UserData);

/* 错误回调 */
static void FrameErrorCallback(uint8 Pid, LinSlave_ErrorType ErrorCode);

/* 诊断回调 */
static void DiagnosticCallback(uint8 Pid, const uint8* DataPtr, uint8 Length, 
                                uint8* ResponsePtr, uint8* ResponseLengthPtr);

/* ============================================
 * Unconditional Frame配置 (8个帧)
 * ============================================ */

/* 帧0: 车速信号 - 从机接收 */
static LinSlave_UnconditionalFrameConfigType UnconditionalFrame_0 = {
    0x10,                           /* PID */
    2,                              /* 长度: 2字节 (车速值) */
    LINSLAVE_DIR_RX,                /* 方向: 接收 */
    LINSLAVE_CSUM_CLASSIC,          /* 经典校验和 */
    UnconditionalRxCallback_0,      /* 接收回调 */
    NULL,                           /* 无发送回调 */
    FrameErrorCallback,             /* 错误回调 */
    (void*)0x1000,                  /* 用户数据: 车速信号ID */
    LINSLAVE_FRAME_STATUS_IDLE,     /* 状态 */
    {0},                            /* 数据缓存 */
    0                               /* 更新标志 */
};

/* 帧1: 转速信号 - 从机接收 */
static LinSlave_UnconditionalFrameConfigType UnconditionalFrame_1 = {
    0x11,
    2,
    LINSLAVE_DIR_RX,
    LINSLAVE_CSUM_CLASSIC,
    UnconditionalRxCallback_1,
    NULL,
    FrameErrorCallback,
    (void*)0x1001,                  /* 转速信号ID */
    LINSLAVE_FRAME_STATUS_IDLE,
    {0},
    0
};

/* 帧2: 油门位置 - 从机接收 */
static LinSlave_UnconditionalFrameConfigType UnconditionalFrame_2 = {
    0x12,
    1,                              /* 1字节 */
    LINSLAVE_DIR_RX,
    LINSLAVE_CSUM_CLASSIC,
    UnconditionalRxCallback_2,
    NULL,
    FrameErrorCallback,
    (void*)0x1002,
    LINSLAVE_FRAME_STATUS_IDLE,
    {0},
    0
};

/* 帧3: 刹车压力 - 从机接收 */
static LinSlave_UnconditionalFrameConfigType UnconditionalFrame_3 = {
    0x13,
    2,
    LINSLAVE_DIR_RX,
    LINSLAVE_CSUM_CLASSIC,
    UnconditionalRxCallback_3,
    NULL,
    FrameErrorCallback,
    (void*)0x1003,
    LINSLAVE_FRAME_STATUS_IDLE,
    {0},
    0
};

/* 帧4: 发动机温度 - 从机发送 (响应主机请求) */
static LinSlave_UnconditionalFrameConfigType UnconditionalFrame_4 = {
    0x20,
    1,
    LINSLAVE_DIR_TX,                /* 方向: 发送 */
    LINSLAVE_CSUM_CLASSIC,
    NULL,                           /* 无接收回调 */
    UnconditionalTxCallback_4,      /* 发送回调 */
    FrameErrorCallback,
    (void*)0x2000,                  /* 温度信号ID */
    LINSLAVE_FRAME_STATUS_IDLE,
    {0},
    0
};

/* 帧5: 电池电压 - 从机发送 */
static LinSlave_UnconditionalFrameConfigType UnconditionalFrame_5 = {
    0x21,
    2,
    LINSLAVE_DIR_TX,
    LINSLAVE_CSUM_CLASSIC,
    NULL,
    UnconditionalTxCallback_5,
    FrameErrorCallback,
    (void*)0x2001,
    LINSLAVE_FRAME_STATUS_IDLE,
    {0},
    0
};

/* 帧6: 里程数 - 从机发送 */
static LinSlave_UnconditionalFrameConfigType UnconditionalFrame_6 = {
    0x22,
    4,                              /* 4字节里程 */
    LINSLAVE_DIR_TX,
    LINSLAVE_CSUM_CLASSIC,
    NULL,
    UnconditionalTxCallback_6,
    FrameErrorCallback,
    (void*)0x2002,
    LINSLAVE_FRAME_STATUS_IDLE,
    {0},
    0
};

/* 帧7: 诊断码 - 从机发送 */
static LinSlave_UnconditionalFrameConfigType UnconditionalFrame_7 = {
    0x23,
    2,
    LINSLAVE_DIR_TX,
    LINSLAVE_CSUM_CLASSIC,
    NULL,
    UnconditionalTxCallback_7,
    FrameErrorCallback,
    (void*)0x2003,
    LINSLAVE_FRAME_STATUS_IDLE,
    {0},
    0
};

/* Unconditional Frame数组 */
static const LinSlave_UnconditionalFrameConfigType* UnconditionalFrames[] = {
    &UnconditionalFrame_0,
    &UnconditionalFrame_1,
    &UnconditionalFrame_2,
    &UnconditionalFrame_3,
    &UnconditionalFrame_4,
    &UnconditionalFrame_5,
    &UnconditionalFrame_6,
    &UnconditionalFrame_7
};

/* ============================================
 * Event Triggered Frame配置
 * ============================================ */

/* 与Event Frame关联的PID列表 */
static const uint8 EventFrame_AssociatedPids[] = {0x10, 0x11, 0x12};

static const LinSlave_EventFrameConfigType EventFrame_0 = {
    0x30,                           /* Event Frame PID */
    3,                              /* 关联的Unconditional Frame数量 */
    EventFrame_AssociatedPids       /* 关联的PID列表 */
};

static const LinSlave_EventFrameConfigType* EventFrames[] = {
    &EventFrame_0
};

/* ============================================
 * Sporadic Frame配置
 * ============================================ */

static const uint8 SporadicFrame_AssociatedPids[] = {0x20, 0x21};

static const LinSlave_SporadicFrameConfigType SporadicFrame_0 = {
    0x40,                           /* Sporadic Frame PID */
    2,                              /* 关联的Unconditional Frame数量 */
    SporadicFrame_AssociatedPids
};

static const LinSlave_SporadicFrameConfigType* SporadicFrames[] = {
    &SporadicFrame_0
};

/* ============================================
 * 诊断报文配置
 * ============================================ */

static const LinSlave_DiagnosticFrameConfigType DiagnosticConfig = {
    0x3C,                           /* 诊断请求PID */
    0x3D,                           /* 诊断响应PID */
    DiagnosticCallback,             /* 诊断回调 */
    FrameErrorCallback,             /* 错误回调 */
    NULL                            /* 用户数据 */
};

/* ============================================
 * 统一配置表 (v2.1)
 * ============================================ */

const LinSlave_ConfigTableType LinSlave_ConfigTable = {
    2,                              /* 主版本 */
    1,                              /* 次版本 */
    0,                              /* 修订版本 */
    
    0x05,                           /* 节点ID: 5 */
    19200,                          /* 波特率: 19200 bps */
    
    /* Unconditional Frames */
    sizeof(UnconditionalFrames) / sizeof(UnconditionalFrames[0]), /* 8个 */
    (const LinSlave_UnconditionalFrameConfigType*)UnconditionalFrames,
    
    /* Event Triggered Frames */
    sizeof(EventFrames) / sizeof(EventFrames[0]), /* 1个 */
    (const LinSlave_EventFrameConfigType*)EventFrames,
    
    /* Sporadic Frames */
    sizeof(SporadicFrames) / sizeof(SporadicFrames[0]), /* 1个 */
    (const LinSlave_SporadicFrameConfigType*)SporadicFrames,
    
    /* Diagnostic Frames */
    &DiagnosticConfig,
    TRUE,                           /* 使用诊断 */
    
    /* 全局错误回调 */
    FrameErrorCallback
};

/* ============================================
 * 回调函数实现
 * ============================================ */

/* 接收回调实现 */
static void UnconditionalRxCallback_0(uint8 FrameIndex, const uint8* DataPtr, uint8 Length, void* UserData)
{
    /* 处理车速信号 */
    uint16 vehicleSpeed = (DataPtr[0] << 8) | DataPtr[1];
    (void)FrameIndex;
    (void)UserData;
    (void)Length;
    (void)vehicleSpeed;
    /* 应用逻辑: 存储车速值 */
}

static void UnconditionalRxCallback_1(uint8 FrameIndex, const uint8* DataPtr, uint8 Length, void* UserData)
{
    /* 处理转速信号 */
    uint16 rpm = (DataPtr[0] << 8) | DataPtr[1];
    (void)FrameIndex;
    (void)UserData;
    (void)Length;
    (void)rpm;
}

static void UnconditionalRxCallback_2(uint8 FrameIndex, const uint8* DataPtr, uint8 Length, void* UserData)
{
    /* 处理油门位置 */
    uint8 throttle = DataPtr[0];
    (void)FrameIndex;
    (void)UserData;
    (void)Length;
    (void)throttle;
}

static void UnconditionalRxCallback_3(uint8 FrameIndex, const uint8* DataPtr, uint8 Length, void* UserData)
{
    /* 处理刹车压力 */
    uint16 brakePressure = (DataPtr[0] << 8) | DataPtr[1];
    (void)FrameIndex;
    (void)UserData;
    (void)Length;
    (void)brakePressure;
}

/* 发送回调实现 */
static void UnconditionalTxCallback_4(uint8 FrameIndex, uint8* DataPtr, uint8* LengthPtr, void* UserData)
{
    /* 发送发动机温度 */
    (void)FrameIndex;
    (void)UserData;
    
    DataPtr[0] = 85;  /* 示例: 85°C */
    *LengthPtr = 1;
}

static void UnconditionalTxCallback_5(uint8 FrameIndex, uint8* DataPtr, uint8* LengthPtr, void* UserData)
{
    /* 发送电池电压 */
    (void)FrameIndex;
    (void)UserData;
    
    DataPtr[0] = 0x0E;  /* 高字节: 14V */
    DataPtr[1] = 0x52;  /* 低字节: 0x52 = 3.3V (14.2V) */
    *LengthPtr = 2;
}

static void UnconditionalTxCallback_6(uint8 FrameIndex, uint8* DataPtr, uint8* LengthPtr, void* UserData)
{
    /* 发送里程数 */
    (void)FrameIndex;
    (void)UserData;
    
    DataPtr[0] = 0x00;
    DataPtr[1] = 0x12;
    DataPtr[2] = 0x34;
    DataPtr[3] = 0x56;  /* 0x00123456 km */
    *LengthPtr = 4;
}

static void UnconditionalTxCallback_7(uint8 FrameIndex, uint8* DataPtr, uint8* LengthPtr, void* UserData)
{
    /* 发送诊断码 */
    (void)FrameIndex;
    (void)UserData;
    
    DataPtr[0] = 0x00;  /* 无诊断 */
    DataPtr[1] = 0x00;
    *LengthPtr = 2;
}

/* 错误回调 */
static void FrameErrorCallback(uint8 Pid, LinSlave_ErrorType ErrorCode)
{
    /* 处理通信错误 */
    (void)Pid;
    (void)ErrorCode;
    /* 可以记录错误日志或触发故障处理 */
}

/* 诊断回调 */
static void DiagnosticCallback(uint8 Pid, const uint8* DataPtr, uint8 Length, 
                                uint8* ResponsePtr, uint8* ResponseLengthPtr)
{
    (void)Pid;
    (void)DataPtr;
    (void)Length;
    
    /* 简单诊断响应示例 */
    ResponsePtr[0] = 0x7F;  /* 负响响应 */
    ResponsePtr[1] = DataPtr[0];  /* 请求的SID */
    ResponsePtr[2] = 0x11;  /* serviceNotSupported */
    *ResponseLengthPtr = 3;
}

/* ============================================
 * 使用示例
 * ============================================ */

/*
void LinSlave_Init_MultiFrameExample(void)
{
    LinSlave_StatusType status;
    
    // 使用配置表初始化
    status = LinSlave_CfgTable_Init(&LinSlave_ConfigTable);
    
    if (status != LINSLAVE_OK) {
        // 初始化失败处理
        return;
    }
    
    // 初始化LIN模块
    status = LinSlave_InitWithConfigTable(&LinSlave_ConfigTable);
    
    // 启动主循环
    while (1) {
        LinSlave_MainFunction();
        
        // 检查帧更新状态
        for (uint8 i = 0; i < 8; i++) {
            if (LinSlave_CfgTable_IsFrameUpdated(i)) {
                const uint8* data = LinSlave_CfgTable_GetFrameData(i);
                // 处理更新的数据
                LinSlave_CfgTable_ClearUpdateFlag(i);
            }
        }
    }
}
*/
