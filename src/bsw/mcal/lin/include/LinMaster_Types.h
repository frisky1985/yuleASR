/**
 * @file LinMaster_Types.h
 * @brief LinMaster 模块类型定义
 * @version 1.0.0
 */

#ifndef LINMASTER_TYPES_H
#define LINMASTER_TYPES_H

#include "Std_Types.h"

/* 版本信息 */
#define LINMASTER_SW_MAJOR_VERSION       1
#define LINMASTER_SW_MINOR_VERSION       0
#define LINMASTER_SW_PATCH_VERSION       0

/* 模块ID */
#define LINMASTER_MODULE_ID              0x51
#define LINMASTER_INSTANCE_ID            0x00

/* 服务ID */
#define LINMASTER_INIT_SID               0x00
#define LINMASTER_DEINIT_SID             0x01
#define LINMASTER_SENDHEADER_SID         0x02
#define LINMASTER_SENDFRAME_SID          0x03
#define LINMASTER_RECVFRAME_SID          0x04
#define LINMASTER_GETSTATE_SID           0x05
#define LINMASTER_WAKEUP_SID             0x06
#define LINMASTER_GOTOSLEEP_SID          0x07

/* 错误码 */
#define LINMASTER_E_NOT_INITIALIZED      0x10
#define LINMASTER_E_INVALID_PARAMETER    0x11
#define LINMASTER_E_NULL_POINTER         0x12
#define LINMASTER_E_INVALID_LENGTH       0x13
#define LINMASTER_E_INVALID_STATE        0x14
#define LINMASTER_E_TIMEOUT              0x15
#define LINMASTER_E_BUS_ERROR            0x16
#define LINMASTER_E_NO_RESPONSE          0x17

/* LIN帧最大长度定义 */
#define LINMASTER_MAX_DATA_LENGTH        8u
#define LINMASTER_MAX_FRAME_LENGTH       9u  /* 8字节数据 + 校验和 */
#define LINMASTER_MAX_RESPONSE_TIME      100u /* 最大响应等待时间(ms) */

/* Break字段定义 - 13位显性电平 (0) */
#define LINMASTER_BREAK_BYTE             0x00u
#define LINMASTER_SYNC_BYTE              0x55u
#define LINMASTER_PID_MASK               0x3Fu /* PID低6位为实际ID */

/* 状态机状态 */
typedef enum {
    LINMASTER_STATE_UNINIT = 0,     /* 未初始化 */
    LINMASTER_STATE_IDLE,           /* 空闲状态 */
    LINMASTER_STATE_SEND_BREAK,     /* 发送 Break */
    LINMASTER_STATE_SEND_SYNC,      /* 发送 Sync */
    LINMASTER_STATE_SEND_PID,       /* 发送 PID */
    LINMASTER_STATE_WAIT_RX_DATA,   /* 等待接收数据 (从机响应) */
    LINMASTER_STATE_SEND_TX_DATA,   /* 发送数据 (主机发送数据) */
    LINMASTER_STATE_WAIT_CHECKSUM,  /* 等待/发送校验和 */
    LINMASTER_STATE_DELAY,          /* 帧间延迟 */
    LINMASTER_STATE_NEXT_ENTRY      /* 进入下一帧 */
} LinMaster_StateType;

/* 错误类型 */
typedef enum {
    LINMASTER_ERROR_NONE = 0,       /* 无错误 */
    LINMASTER_ERROR_BREAK,          /* Break 发送错误 */
    LINMASTER_ERROR_SYNC,           /* 同步字节错误 */
    LINMASTER_ERROR_PID,            /* PID 错误 */
    LINMASTER_ERROR_CHECKSUM,       /* 校验和错误 */
    LINMASTER_ERROR_TIMEOUT,        /* 响应超时 */
    LINMASTER_ERROR_FRAMING,        /* 帧格式错误 */
    LINMASTER_ERROR_BUS,            /* 总线错误 */
    LINMASTER_ERROR_NO_RESPONSE     /* 从机无响应 */
} LinMaster_ErrorType;

/* 操作状态 */
typedef enum {
    LINMASTER_OK = 0,               /* 成功 */
    LINMASTER_NOT_OK,               /* 失败 */
    LINMASTER_BUSY,                 /* 忙碌 */
    LINMASTER_TIMEOUT               /* 超时 */
} LinMaster_StatusType;

/* 校验和类型 */
typedef enum {
    LINMASTER_CHECKSUM_CLASSIC = 0, /* 经典校验和 - 仅数据字节 */
    LINMASTER_CHECKSUM_ENHANCED     /* 增强校验和 - PID + 数据字节 */
} LinMaster_ChecksumType;

/* 帧方向 */
typedef enum {
    LINMASTER_FRAME_DIR_RX = 0,     /* 主机接收 (从机发送数据) */
    LINMASTER_FRAME_DIR_TX,         /* 主机发送 (主机发送数据) */
    LINMASTER_FRAME_DIR_TX_RX       /* 双向传输 */
} LinMaster_FrameDirectionType;

/* 帧类型 */
typedef enum {
    LINMASTER_FRAME_TYPE_UNCONDITIONAL = 0,  /* 无条件帧 */
    LINMASTER_FRAME_TYPE_EVENT,              /* 事件触发帧 */
    LINMASTER_FRAME_TYPE_SPORADIC,           /* 偶发帧 */
    LINMASTER_FRAME_TYPE_DIAGNOSTIC,         /* 诊断帧 */
    LINMASTER_FRAME_TYPE_USER_DEFINED        /* 用户定义帧 */
} LinMaster_FrameType;

/* 配置结构体 */
typedef struct {
    uint8 BaudRate;                 /* 波特率: 0=9600, 1=19200, 2=custom */
    uint16 CustomBaudRate;          /* 自定义波特率 (当BaudRate=2时使用) */
    uint16 BreakDurationUs;         /* Break 持续时间 (微秒) 默认 13位 */
    uint8 InterFrameDelayMs;        /* 帧间延迟 (毫秒) */
    uint16 ResponseTimeoutMs;       /* 响应超时时间 (毫秒) */
} LinMaster_ConfigType;

/* 帧配置结构体 */
typedef struct {
    uint8 Pid;                      /* Protected ID (包含校验位) */
    uint8 Id;                       /* 实际ID (0-59) */
    uint8 DataLength;               /* 数据长度 (1-8) */
    LinMaster_FrameDirectionType Direction;  /* 帧方向 */
    LinMaster_ChecksumType ChecksumType;     /* 校验和类型 */
    LinMaster_FrameType FrameType;  /* 帧类型 */
    uint16 DelayAfterFrameMs;       /* 帧后延迟 (ms) */
} LinMaster_FrameConfigType;

/* 接收回调函数类型 */
typedef void (*LinMaster_RxCallbackFuncType)(
    uint8 Pid,                      /* 接收到的PID */
    const uint8* DataPtr,           /* 接收数据指针 */
    uint8 Length,                   /* 数据长度 */
    boolean IsChecksumValid         /* 校验和是否有效 */
);

/* 发送完成回调函数类型 */
typedef void (*LinMaster_TxCallbackFuncType)(
    uint8 Pid,                      /* 发送的PID */
    uint8* DataPtr,                 /* 发送数据指针 */
    uint8 Length,                   /* 数据长度 */
    boolean IsSuccessful            /* 是否发送成功 */
);

/* 错误回调函数类型 */
typedef void (*LinMaster_ErrorCallbackFuncType)(
    LinMaster_ErrorType ErrorCode,  /* 错误码 */
    uint8 Pid,                      /* 相关PID (如果有) */
    uint8 BytePosition              /* 错误发生的位置 */
);

/* 状态变化回调函数类型 */
typedef void (*LinMaster_StateCallbackFuncType)(
    LinMaster_StateType OldState,   /* 旧状态 */
    LinMaster_StateType NewState    /* 新状态 */
);

#endif /* LINMASTER_TYPES_H */
