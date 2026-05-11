/*
 * @file Uart.h
 * @brief UART驱动标准AUTOSAR接口头文件
 * 
 * AUTOSAR MCAL 4.x 规范兼容
 * 支持DMA、中断、FIFO、流控制
 */

#ifndef UART_H
#define UART_H

#include "Uart_Cfg.h"
#include "Std_Types.h"
#include "Mcu.h"
#include "Det.h"

/*============================================================================
 * 版本信息
 *===========================================================================*/
#define UART_VENDOR_ID              0x01u
#define UART_MODULE_ID              0x11u
#define UART_INSTANCE_ID            0x00u

#define UART_SW_MAJOR_VERSION       1u
#define UART_SW_MINOR_VERSION       0u
#define UART_SW_PATCH_VERSION       0u

/*============================================================================
 * 错误码定义 (AUTOSAR标准)
 *===========================================================================*/
#define UART_E_NO_ERROR             0x00u   /* 无错误 */
#define UART_E_PARAM_CHANNEL        0x01u   /* 无效通道参数 */
#define UART_E_PARAM_POINTER        0x02u   /* 无效指针参数 */
#define UART_E_PARAM_CONFIG         0x03u   /* 无效配置参数 */
#define UART_E_PARAM_BAUDRATE       0x04u   /* 无效波特率 */
#define UART_E_PARAM_DATABITS       0x05u   /* 无效数据位 */
#define UART_E_PARAM_STOPBITS       0x06u   /* 无效停止位 */
#define UART_E_PARAM_PARITY         0x07u   /* 无效校验 */
#define UART_E_UNINIT               0x10u   /* 驱动未初始化 */
#define UART_E_ALREADY_INITIALIZED  0x11u   /* 驱动已初始化 */
#define UART_E_TX_BUSY              0x20u   /* 发送忙 */
#define UART_E_RX_BUSY              0x21u   /* 接收忙 */
#define UART_E_TX_ERROR             0x30u   /* 发送错误 */
#define UART_E_RX_ERROR             0x31u   /* 接收错误 */
#define UART_E_OVERRUN              0x32u   /* 溢出错误 */
#define UART_E_FRAMING              0x33u   /* 帧错误 */
#define UART_E_PARITY               0x34u   /* 奇偶校验错误 */
#define UART_E_BREAK                0x35u   /* 中断信号 */
#define UART_E_TIMEOUT              0x40u   /* 超时错误 */
#define UART_E_DMA_ERROR            0x50u   /* DMA错误 */

/*============================================================================
 * 类型定义
 *===========================================================================*/

/* UART通道类型 */
typedef uint8 Uart_ChannelType;

/* UART状态类型 */
typedef enum {
    UART_STATE_UNINIT = 0,      /* 未初始化 */
    UART_STATE_READY,           /* 就绪 */
    UART_STATE_TX_BUSY,         /* 发送忙 */
    UART_STATE_RX_BUSY,         /* 接收忙 */
    UART_STATE_TX_RX_BUSY,      /* 发送接收忙 */
    UART_STATE_ERROR            /* 错误状态 */
} Uart_StatusType;

/* UART操作模式 */
typedef enum {
    UART_MODE_POLLING = 0,      /* 轮询模式 */
    UART_MODE_INTERRUPT,        /* 中断模式 */
    UART_MODE_DMA               /* DMA模式 */
} Uart_OpModeType;

/* 数据位数 */
typedef enum {
    UART_DATA_BITS_5 = 0,       /* 5位数据 */
    UART_DATA_BITS_6,           /* 6位数据 */
    UART_DATA_BITS_7,           /* 7位数据 */
    UART_DATA_BITS_8            /* 8位数据 */
} Uart_DataBitsType;

/* 停止位 */
typedef enum {
    UART_STOP_BITS_1 = 0,       /* 1位停止位 */
    UART_STOP_BITS_1_5,         /* 1.5位停止位 */
    UART_STOP_BITS_2            /* 2位停止位 */
} Uart_StopBitsType;

/* 奇偶校验 */
typedef enum {
    UART_PARITY_NONE = 0,       /* 无校验 */
    UART_PARITY_ODD,            /* 奇校验 */
    UART_PARITY_EVEN            /* 偶校验 */
} Uart_ParityType;

/* 流控制 */
typedef enum {
    UART_HW_HANDSHAKE_NONE = 0, /* 无硬件流控制 */
    UART_HW_HANDSHAKE_RTS,      /* 仅RTS */
    UART_HW_HANDSHAKE_CTS,      /* 仅CTS */
    UART_HW_HANDSHAKE_RTS_CTS   /* RTS/CTS */
} Uart_HwHandshakeType;

/* FIFO模式 */
typedef enum {
    UART_FIFO_DISABLED = 0,     /* 禁用FIFO */
    UART_FIFO_ENABLED           /* 启用FIFO */
} Uart_FifoModeType;

/* 传输状态 */
typedef enum {
    UART_TX_IDLE = 0,           /* 发送空闲 */
    UART_TX_ACTIVE,             /* 发送进行中 */
    UART_TX_COMPLETE,           /* 发送完成 */
    UART_TX_ERROR               /* 发送错误 */
} Uart_TxStatusType;

typedef enum {
    UART_RX_IDLE = 0,           /* 接收空闲 */
    UART_RX_ACTIVE,             /* 接收进行中 */
    UART_RX_COMPLETE,           /* 接收完成 */
    UART_RX_ERROR               /* 接收错误 */
} Uart_RxStatusType;

/* 传输结果 */
typedef enum {
    UART_RESULT_OK = 0,         /* 成功 */
    UART_RESULT_PENDING,        /* 进行中 */
    UART_RESULT_TIMEOUT,        /* 超时 */
    UART_RESULT_ERROR           /* 错误 */
} Uart_ResultType;

/*============================================================================
 * 配置结构体
 *===========================================================================*/

/* 单个通道配置 */
typedef struct {
    Uart_ChannelType        ChannelId;          /* 通道ID */
    uint32                  BaudRate;           /* 波特率 */
    Uart_DataBitsType       DataBits;           /* 数据位 */
    Uart_StopBitsType       StopBits;           /* 停止位 */
    Uart_ParityType         Parity;             /* 校验 */
    Uart_OpModeType         OpMode;             /* 操作模式 */
    Uart_HwHandshakeType    HwHandshake;        /* 硬件流控制 */
    Uart_FifoModeType       FifoMode;           /* FIFO模式 */
    uint8                   TxFifoThreshold;    /* TX FIFO阈值 */
    uint8                   RxFifoThreshold;    /* RX FIFO阈值 */
    boolean                 DmaEnabled;         /* DMA使能 */
    uint8                   DmaTxChannel;       /* TX DMA通道 */
    uint8                   DmaRxChannel;       /* RX DMA通道 */
    uint8                   IrqPriority;        /* 中断优先级 */
    uint32                  TxTimeout;          /* 发送超时(ms) */
    uint32                  RxTimeout;          /* 接收超时(ms) */
} Uart_ChannelConfigType;

/* 全局配置 */
typedef struct {
    uint8                   ChannelCount;       /* 通道数量 */
    const Uart_ChannelConfigType* ChannelConfig;/* 通道配置数组 */
} Uart_ConfigType;

/* 传输信息 */
typedef struct {
    uint8*                  Buffer;             /* 缓冲区指针 */
    uint32                  Length;             /* 数据长度 */
    uint32                  Transferred;        /* 已传输长度 */
    Uart_ResultType         Result;             /* 传输结果 */
} Uart_BufferType;

/*============================================================================
 * 回调函数类型
 *===========================================================================*/
/* 发送完成回调 */
typedef void (*Uart_TxNotificationType)(void);

/* 接收完成回调 */
typedef void (*Uart_RxNotificationType)(void);

/* 错误通知回调 */
typedef void (*Uart_ErrorNotificationType)(uint8 ErrorCode);

/*============================================================================
 * API函数声明
 *===========================================================================*/

/* 初始化和反初始化 */
extern void Uart_Init(const Uart_ConfigType* Config);
extern void Uart_DeInit(void);

/* 发送函数 */
extern Std_ReturnType Uart_Send(
    Uart_ChannelType Channel,
    const uint8* Data,
    uint32 Length
);

extern Std_ReturnType Uart_SendDMA(
    Uart_ChannelType Channel,
    const uint8* Data,
    uint32 Length
);

extern Std_ReturnType Uart_SendInterrupt(
    Uart_ChannelType Channel,
    const uint8* Data,
    uint32 Length
);

/* 接收函数 */
extern Std_ReturnType Uart_Receive(
    Uart_ChannelType Channel,
    uint8* Buffer,
    uint32 Length
);

extern Std_ReturnType Uart_ReceiveDMA(
    Uart_ChannelType Channel,
    uint8* Buffer,
    uint32 Length
);

extern Std_ReturnType Uart_ReceiveInterrupt(
    Uart_ChannelType Channel,
    uint8* Buffer,
    uint32 Length
);

/* 状态和结果 */
extern Uart_StatusType Uart_GetStatus(Uart_ChannelType Channel);
extern Uart_ResultType Uart_GetTxResult(Uart_ChannelType Channel);
extern Uart_ResultType Uart_GetRxResult(Uart_ChannelType Channel);

/* 波特率设置 */
extern Std_ReturnType Uart_SetBaudRate(
    Uart_ChannelType Channel,
    uint32 BaudRate
);

/* 中断管理 */
extern void Uart_EnableInterrupt(Uart_ChannelType Channel);
extern void Uart_DisableInterrupt(Uart_ChannelType Channel);

/* DMA管理 */
#if (UART_DMA_SUPPORT == STD_ON)
extern void Uart_EnableDMA(Uart_ChannelType Channel);
extern void Uart_DisableDMA(Uart_ChannelType Channel);
#endif

/* FIFO管理 */
#if (UART_FIFO_SUPPORT == STD_ON)
extern void Uart_ClearFIFO(Uart_ChannelType Channel);
extern Std_ReturnType Uart_SetFifoThreshold(
    Uart_ChannelType Channel,
    uint8 TxThreshold,
    uint8 RxThreshold
);
#endif

/* 传输中止 */
extern void Uart_Abort(Uart_ChannelType Channel);

/* 版本信息 */
#if (UART_VERSION_INFO_API == STD_ON)
extern void Uart_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

/* 主函数 (轮询模式需要) */
extern void Uart_MainFunction(void);

/* 中断处理函数 */
extern void Uart_IsrHandler(Uart_ChannelType Channel);

#endif /* UART_H */
