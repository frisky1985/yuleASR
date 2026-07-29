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

/*
 * @file Uart.c
 * @brief UART驱动核心实现
 * 
 * 硬件: i.MX8M Mini UART
 * 特性: 支持轮询/中断/DMA模式，FIFO，流控制
 */

#include "Uart.h"
#include "SchM_Uart.h"

#if (UART_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*============================================================================
 * i.MX8M Mini UART寄存器定义
 *===========================================================================*/
#ifdef S32K312
#include "S32K312.h"
/* S32K312 uses LPUART (register layout differs from i.MX RT UART) */
#define UART1_BASE_ADDR             S32K312_LPUART0_BASE
#define UART2_BASE_ADDR             S32K312_LPUART1_BASE
#define UART3_BASE_ADDR             S32K312_LPUART2_BASE
#define UART4_BASE_ADDR             S32K312_LPUART3_BASE
#else
#define UART1_BASE_ADDR             0x30860000u
#define UART2_BASE_ADDR             0x30890000u
#define UART3_BASE_ADDR             0x30880000u
#define UART4_BASE_ADDR             0x30A60000u
#endif

/* UART寄存器偏移 */
#define UART_URXD_OFFSET            0x00u   /* 接收数据寄存器 */
#define UART_UTXD_OFFSET            0x40u   /* 发送数据寄存器 */
#define UART_UCR1_OFFSET            0x80u   /* 控制寄存器1 */
#define UART_UCR2_OFFSET            0x84u   /* 控制寄存器2 */
#define UART_UCR3_OFFSET            0x88u   /* 控制寄存器3 */
#define UART_UCR4_OFFSET            0x8Cu   /* 控制寄存器4 */
#define UART_UFCR_OFFSET            0x90u   /* FIFO控制寄存器 */
#define UART_USR1_OFFSET            0x94u   /* 状态寄存器1 */
#define UART_USR2_OFFSET            0x98u   /* 状态寄存器2 */
#define UART_UESC_OFFSET            0x9Cu   /* 逃逸字符寄存器 */
#define UART_UTIM_OFFSET            0xA0u   /* 超时寄存器 */
#define UART_UBIR_OFFSET            0xA4u   /* 波特率增量寄存器 */
#define UART_UBMR_OFFSET            0xA8u   /* 波特率模数寄存器 */
#define UART_UBRC_OFFSET            0xACu   /* 波特率检测寄存器 */
#define UART_ONEMS_OFFSET           0xB0u   /* 1ms计数器寄存器 */
#define UART_UTS_OFFSET             0xB4u   /* 测试寄存器 */

/* UCR1寄存器位 */
#define UCR1_UARTEN                 (1u << 0)   /* UART使能 */
#define UCR1_DOZE                   (1u << 1)   /* DOZE模式 */
#define UCR1_TXDMAEN                (1u << 3)   /* TX DMA使能 */
#define UCR1_RXDMAEN                (1u << 2)   /* RX DMA使能 */
#define UCR1_TXMPTYEN               (1u << 6)   /* TX FIFO空中断使能 */
#define UCR1_RRDYEN                 (1u << 9)   /* RX FIFO就绪中断使能 */
#define UCR1_RDMAEN                 (1u << 8)   /* RX DMA请求使能 */
#define UCR1_TRDYEN                 (1u << 7)   /* TX DMA请求使能 */

/* UCR2寄存器位 */
#define UCR2_SRST                   (1u << 0)   /* 软件复位 */
#define UCR2_RXEN                   (1u << 1)   /* 接收使能 */
#define UCR2_TXEN                   (1u << 2)   /* 发送使能 */
#define UCR2_PREN                   (1u << 3)   /* 奇偶校验使能 */
#define UCR2_PROE                   (1u << 4)   /* 偶校验模式 */
#define UCR2_STPB                   (1u << 5)   /* 2位停止位 */
#define UCR2_WS                     (1u << 6)   /* 字符长度 (0=8位) */
#define UCR2_RTSEN                  (1u << 7)   /* RTS使能 */
#define UCR2_ATENEN                 (1u << 8)   /* 老化使能 */
#define UCR2_CTS                    (1u << 9)   /* CTS使能 */
#define UCR2_IRTS                   (1u << 10)  /* 忽略RTS */
#define UCR2_ESCI                   (1u << 11)  /* 逃逸序列中断 */

/* UCR3寄存器位 */
#define UCR3_ACIEN                  (1u << 0)   /* 自动波特率中断 */
#define UCR3_AIRINTEN               (1u << 1)   /* 空闲中断使能 */
#define UCR3_RXDMUXSEL              (1u << 2)   /* RXD多路复用 */
#define UCR3_DTRDEN                 (1u << 3)   /* DTR/DSR使能 */
#define UCR3_ADNIMP                 (1u << 7)   /* 自动检测禁用 */

/* UCR4寄存器位 */
#define UCR4_DREN                   (1u << 0)   /* 数据就绪中断 */
#define UCR4_OREN                   (1u << 1)   /* 溢出中断 */
#define UCR4_BKEN                   (1u << 2)   /* 中断条件中断 */
#define UCR4_TCEN                   (1u << 3)   /* 传输完成中断 */
#define UCR4_LPBYP                  (1u << 4)   /* 循环旁路 */
#define UCR4_CTSTL_SHIFT            10u         /* CTS触发水平位移 */

/* UFCR寄存器位 */
#define UFCR_RXTL_SHIFT             0u          /* RX FIFO阈值位移 */
#define UFCR_RFDIV_SHIFT            7u          /* 参考时钟分额位移 */
#define UFCR_TXTL_SHIFT             10u         /* TX FIFO阈值位移 */

/* USR1寄存器位 */
#define USR1_SAD                    (1u << 3)   /* 多点模式地址检测 */
#define USR1_AWAKE                  (1u << 4)   /* 唤醒 */
#define USR1_AIRINT                 (1u << 5)   /* 空闲中断 */
#define USR1_RXDS                   (1u << 6)   /* RX FIFO满 */
#define USR1_RRDY                   (1u << 9)   /* RX FIFO就绪 */
#define USR1_FRAMERR                (1u << 10)  /* 帧错误 */
#define USR1_ESCF                   (1u << 11)  /* 逃逸序列标志 */
#define USR1_RTSD                   (1u << 12)  /* RTS状态 */
#define USR1_AGTIM                  (1u << 13)  /* 自动波特率中断 */
#define USR1_DTRD                   (1u << 15)  /* DTR/DSR检测 */

/* USR2寄存器位 */
#define USR2_RDR                    (1u << 0)   /* 数据就绪 */
#define USR2_ORE                    (1u << 1)   /* 溢出错误 */
#define USR2_BRCD                   (1u << 2)   /* 中断条件检测 */
#define USR2_TXDC                   (1u << 3)   /* 传输完成 */
#define USR2_RTSF                   (1u << 4)   /* RTS边沿触发 */
#define USR2_DCDDELT                (1u << 6)   /* DCD变化 */
#define USR2_DCDIN                  (1u << 7)   /* DCD输入 */
#define USR2_TXFE                   (1u << 14)  /* TX FIFO空 */
#define USR2_RXFE                   (1u << 15)  /* RX FIFO空 */

/* 参考时钟分额 */
#define UART_RFDIV_1                0x5u
#define UART_RFDIV_2                0x0u
#define UART_RFDIV_3                0x1u
#define UART_RFDIV_4                0x2u
#define UART_RFDIV_5                0x3u
#define UART_RFDIV_6                0x4u
#define UART_RFDIV_7                0x6u

/*============================================================================
 * 全局变量
 *===========================================================================*/
/* 驱动初始化状态 */
static boolean Uart_Initialized = FALSE;

/* 驱动配置指针 */
static const Uart_ConfigType* Uart_ConfigPtr = NULL_PTR;

/* 通道基地地址表 */
static volatile uint32* const Uart_BaseAddr[UART_CHANNEL_COUNT] = {
    (volatile uint32*)UART1_BASE_ADDR,
    (volatile uint32*)UART2_BASE_ADDR,
    (volatile uint32*)UART3_BASE_ADDR,
    (volatile uint32*)UART4_BASE_ADDR
};

/* 通道状态跟踪 */
typedef struct {
    Uart_StatusType         Status;         /* 通道状态 */
    Uart_TxStatusType       TxStatus;       /* 发送状态 */
    Uart_RxStatusType       RxStatus;       /* 接收状态 */
    Uart_BufferType         TxBuffer;       /* 发送缓冲区 */
    Uart_BufferType         RxBuffer;       /* 接收缓冲区 */
    uint32                  TxStartTime;    /* 发送开始时间 */
    uint32                  RxStartTime;    /* 接收开始时间 */
    uint8                   ErrorCode;      /* 错误码 */
    boolean                 DmaActive;      /* DMA活动状态 */
} Uart_ChannelStateType;

static Uart_ChannelStateType Uart_ChannelState[UART_CHANNEL_COUNT];

/* 回调函数指针 */
static Uart_TxNotificationType   Uart_TxNotification[UART_CHANNEL_COUNT];
static Uart_RxNotificationType   Uart_RxNotification[UART_CHANNEL_COUNT];
static Uart_ErrorNotificationType Uart_ErrorNotification[UART_CHANNEL_COUNT];

/*============================================================================
 * 验证宏
 *===========================================================================*/
#if (UART_DEV_ERROR_DETECT == STD_ON)
    #define UART_VALIDATE_CHANNEL(Channel, ApiId)         do {             if ((Channel) >= UART_CHANNEL_COUNT) {                 Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, (ApiId), UART_E_PARAM_CHANNEL);                 return E_NOT_OK;             }         } while(0)

    #define UART_VALIDATE_POINTER(Ptr, ApiId)         do {             if ((Ptr) == NULL_PTR) {                 Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, (ApiId), UART_E_PARAM_POINTER);                 return E_NOT_OK;             }         } while(0)

    #define UART_VALIDATE_INITIALIZED(ApiId)         do {             if (Uart_Initialized == FALSE) {                 Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, (ApiId), UART_E_UNINIT);                 return E_NOT_OK;             }         } while(0)
#else
    #define UART_VALIDATE_CHANNEL(Channel, ApiId)
    #define UART_VALIDATE_POINTER(Ptr, ApiId)
    #define UART_VALIDATE_INITIALIZED(ApiId)
#endif

/*============================================================================
 * 内部函数声明
 *===========================================================================*/
static void Uart_HwInit(Uart_ChannelType Channel);
static void Uart_HwDeInit(Uart_ChannelType Channel);
static void Uart_SetBaudRateInternal(Uart_ChannelType Channel, uint32 BaudRate);
static void Uart_ProcessTxInterrupt(Uart_ChannelType Channel);
static void Uart_ProcessRxInterrupt(Uart_ChannelType Channel);
static void Uart_ProcessError(Uart_ChannelType Channel);
static inline void Uart_WriteReg(Uart_ChannelType Channel, uint32 Offset, uint32 Value);
static inline uint32 Uart_ReadReg(Uart_ChannelType Channel, uint32 Offset);

/*============================================================================
 * API实现
 *===========================================================================*/

/**
 * @brief UART驱动初始化
 * @param Config 驱动配置指针
 */
void Uart_Init(const Uart_ConfigType* Config)
{
    uint8 channel;
    
    #if (UART_DEV_ERROR_DETECT == STD_ON)
    if (Config == NULL_PTR) {
        Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, 
                        UART_SERVICE_ID_INIT, UART_E_PARAM_POINTER);
        return;
    }
    if (Uart_Initialized == TRUE) {
        Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, 
                        UART_SERVICE_ID_INIT, UART_E_ALREADY_INITIALIZED);
        return;
    }
    #endif
    
    /* 保存配置指针 */
    Uart_ConfigPtr = Config;
    
    /* 初始化所有通道 */
    for (channel = 0; channel < Config->ChannelCount; channel++) {
        /* 清零状态 */
        Uart_ChannelState[channel].Status = UART_STATE_READY;
        Uart_ChannelState[channel].TxStatus = UART_TX_IDLE;
        Uart_ChannelState[channel].RxStatus = UART_RX_IDLE;
        Uart_ChannelState[channel].ErrorCode = UART_E_NO_ERROR;
        Uart_ChannelState[channel].DmaActive = FALSE;
        
        /* 硬件初始化 */
        Uart_HwInit(channel);
    }
    
    Uart_Initialized = TRUE;
}

/**
 * @brief UART驱动反初始化
 */
void Uart_DeInit(void)
{
    uint8 channel;
    
    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_DEINIT);
    
    /* 反初始化所有通道 */
    for (channel = 0; channel < Uart_ConfigPtr->ChannelCount; channel++) {
        Uart_HwDeInit(channel);
        
        Uart_ChannelState[channel].Status = UART_STATE_UNINIT;
        Uart_ChannelState[channel].TxStatus = UART_TX_IDLE;
        Uart_ChannelState[channel].RxStatus = UART_RX_IDLE;
    }
    
    Uart_ConfigPtr = NULL_PTR;
    Uart_Initialized = FALSE;
}

/**
 * @brief 硬件初始化
 */
static void Uart_HwInit(Uart_ChannelType Channel)
{
    const Uart_ChannelConfigType* ChannelConfig = &Uart_ConfigPtr->ChannelConfig[Channel];
    volatile uint32* base = Uart_BaseAddr[Channel];
    uint32 regVal;
    
    /* 软件复位 */
    *(base + (UART_UCR2_OFFSET / 4)) = 0x0;
    while (*(base + (UART_UCR2_OFFSET / 4)) & UCR2_SRST);
    
    /* 配置UCR1 */
    regVal = UCR1_UARTEN;
    if (ChannelConfig->DmaEnabled) {
        regVal |= (UCR1_TXDMAEN | UCR1_RXDMAEN | UCR1_RDMAEN | UCR1_TRDYEN);
    }
    if (ChannelConfig->OpMode == UART_MODE_INTERRUPT) {
        regVal |= (UCR1_TXMPTYEN | UCR1_RRDYEN);
    }
    *(base + (UART_UCR1_OFFSET / 4)) = regVal;
    
    /* 配置UCR2 */
    regVal = UCR2_SRST | UCR2_RXEN | UCR2_TXEN;
    
    /* 校验配置 */
    if (ChannelConfig->Parity != UART_PARITY_NONE) {
        regVal |= UCR2_PREN;
        if (ChannelConfig->Parity == UART_PARITY_EVEN) {
            regVal |= UCR2_PROE;
        }
    }
    
    /* 停止位 */
    if (ChannelConfig->StopBits == UART_STOP_BITS_2) {
        regVal |= UCR2_STPB;
    }
    
    /* 数据位 */
    if (ChannelConfig->DataBits != UART_DATA_BITS_8) {
        regVal |= UCR2_WS;
    }
    
    /* 流控制 */
    if (ChannelConfig->HwHandshake != UART_HW_HANDSHAKE_NONE) {
        regVal |= UCR2_CTS;
        if (ChannelConfig->HwHandshake == UART_HW_HANDSHAKE_RTS_CTS) {
            regVal |= UCR2_RTSEN;
        }
    }
    
    *(base + (UART_UCR2_OFFSET / 4)) = regVal;
    
    /* 配置UCR3 */
    regVal = UCR3_RXDMUXSEL | UCR3_ADNIMP;
    *(base + (UART_UCR3_OFFSET / 4)) = regVal;
    
    /* 配置UCR4 */
    regVal = 0;
    if (ChannelConfig->OpMode == UART_MODE_INTERRUPT) {
        regVal |= (UCR4_DREN | UCR4_OREN);
    }
    *(base + (UART_UCR4_OFFSET / 4)) = regVal;
    
    /* 配置FIFO */
    if (ChannelConfig->FifoMode == UART_FIFO_ENABLED) {
        regVal = ((ChannelConfig->RxFifoThreshold << UFCR_RXTL_SHIFT) |
                  (UART_RFDIV_1 << UFCR_RFDIV_SHIFT) |
                  (ChannelConfig->TxFifoThreshold << UFCR_TXTL_SHIFT));
        *(base + (UART_UFCR_OFFSET / 4)) = regVal;
    } else {
        *(base + (UART_UFCR_OFFSET / 4)) = (UART_RFDIV_1 << UFCR_RFDIV_SHIFT);
    }
    
    /* 设置波特率 */
    Uart_SetBaudRateInternal(Channel, ChannelConfig->BaudRate);
}

/**
 * @brief 硬件反初始化
 */
static void Uart_HwDeInit(Uart_ChannelType Channel)
{
    volatile uint32* base = Uart_BaseAddr[Channel];
    
    /* 禁用UART */
    *(base + (UART_UCR1_OFFSET / 4)) = 0;
    *(base + (UART_UCR2_OFFSET / 4)) = 0;
}

/**
 * @brief 内部波特率设置
 */
static void Uart_SetBaudRateInternal(Uart_ChannelType Channel, uint32 BaudRate)
{
    volatile uint32* base = Uart_BaseAddr[Channel];
    uint32 refClock = UART_REF_CLOCK_HZ;
    uint32 div;
    uint32 bfDiv;
    uint32 bmDiv;
    
    /* 计算分额 */
    div = refClock / (BaudRate * 16);
    
    /* UBIR = 波特率 - 1 */
    bfDiv = BaudRate - 1;
    
    /* UBMR = 参考时钟 / (16 * 分额) - 1 */
    bmDiv = (refClock / div / 16) - 1;
    
    *(base + (UART_UBIR_OFFSET / 4)) = bfDiv;
    *(base + (UART_UBMR_OFFSET / 4)) = bmDiv;
}


/**
 * @brief 轮询方式发送数据
 * @param Channel 通道ID
 * @param Data 数据指针
 * @param Length 数据长度
 * @return E_OK成功，E_NOT_OK失败
 */
Std_ReturnType Uart_Send(
    Uart_ChannelType Channel,
    const uint8* Data,
    uint32 Length
)
{
    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_SEND);
    UART_VALIDATE_POINTER(Data, UART_SERVICE_ID_SEND);
    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_SEND);
    
    const Uart_ChannelConfigType* ChannelConfig = &Uart_ConfigPtr->ChannelConfig[Channel];
    volatile uint32* base = Uart_BaseAddr[Channel];
    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
    uint32 i;
    uint32 startTime;
    
    /* 检查通道状态 */
    if (state->Status == UART_STATE_TX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
        #if (UART_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, 
                        UART_SERVICE_ID_SEND, UART_E_TX_BUSY);
        #endif
        return E_NOT_OK;
    }
    
    /* 设置通道状态 */
    if (state->Status == UART_STATE_RX_BUSY) {
        state->Status = UART_STATE_TX_RX_BUSY;
    } else {
        state->Status = UART_STATE_TX_BUSY;
    }
    state->TxStatus = UART_TX_ACTIVE;
    state->TxBuffer.Buffer = (uint8*)Data;
    state->TxBuffer.Length = Length;
    state->TxBuffer.Transferred = 0;
    state->TxBuffer.Result = UART_RESULT_PENDING;
    
    /* 轮询方式发送 */
    startTime = Uart_GetCurrentTime();
    
    for (i = 0; i < Length; i++) {
        /* 等待TX FIFO可用 */
        while ((*(base + (UART_USR1_OFFSET / 4)) & USR1_TRDY) == 0U ) {
            if (Uart_GetElapsedTime(startTime) > ChannelConfig->TxTimeout) {
                state->TxBuffer.Result = UART_RESULT_TIMEOUT;
                state->TxStatus = UART_TX_ERROR;
                state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
                                UART_STATE_RX_BUSY : UART_STATE_READY;
                return E_NOT_OK;
            }
        }
        
        /* 写入数据 */
        *(base + (UART_UTXD_OFFSET / 4)) = Data[i];
        state->TxBuffer.Transferred++;
    }
    
    /* 等待传输完成 */
    while ((*(base + (UART_USR2_OFFSET / 4)) & USR2_TXDC) == 0U ) {
        if (Uart_GetElapsedTime(startTime) > ChannelConfig->TxTimeout) {
            state->TxBuffer.Result = UART_RESULT_TIMEOUT;
            state->TxStatus = UART_TX_ERROR;
            state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
                            UART_STATE_RX_BUSY : UART_STATE_READY;
            return E_NOT_OK;
        }
    }
    
    /* 发送完成 */
    state->TxBuffer.Result = UART_RESULT_OK;
    state->TxStatus = UART_TX_COMPLETE;
    state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
                    UART_STATE_RX_BUSY : UART_STATE_READY;
    
    /* 调用回调 */
    if (Uart_TxNotification[Channel] != NULL_PTR) {
        Uart_TxNotification[Channel]();
    }
    
    return E_OK;
}

/**
 * @brief DMA方式发送数据
 */
Std_ReturnType Uart_SendDMA(
    Uart_ChannelType Channel,
    const uint8* Data,
    uint32 Length
)
{
    #if (UART_DMA_SUPPORT == STD_ON)
    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_SENDDMA);
    UART_VALIDATE_POINTER(Data, UART_SERVICE_ID_SENDDMA);
    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_SENDDMA);
    
    const Uart_ChannelConfigType* ChannelConfig = &Uart_ConfigPtr->ChannelConfig[Channel];
    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
    
    if (ChannelConfig->DmaEnabled == FALSE) {
        return E_NOT_OK;
    }
    
    /* 检查通道状态 */
    if (state->Status == UART_STATE_TX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
        return E_NOT_OK;
    }
    
    /* 设置通道状态 */
    if (state->Status == UART_STATE_RX_BUSY) {
        state->Status = UART_STATE_TX_RX_BUSY;
    } else {
        state->Status = UART_STATE_TX_BUSY;
    }
    state->TxStatus = UART_TX_ACTIVE;
    state->TxBuffer.Buffer = (uint8*)Data;
    state->TxBuffer.Length = Length;
    state->TxBuffer.Transferred = 0;
    state->TxBuffer.Result = UART_RESULT_PENDING;
    state->DmaActive = TRUE;
    
    /* 配置DMA */
    Dma_ConfigType dmaConfig;
    dmaConfig.Channel = ChannelConfig->DmaTxChannel;
    dmaConfig.SourceAddr = (uint32)Data;
    dmaConfig.DestAddr = UART1_BASE_ADDR + UART_UTXD_OFFSET + (Channel * 0x40000);
    dmaConfig.TransferSize = Length;
    dmaConfig.SourceInc = TRUE;
    dmaConfig.DestInc = FALSE;
    dmaConfig.TransferWidth = DMA_WIDTH_8BIT;
    dmaConfig.Mode = DMA_MODE_NORMAL;
    
    /* 启动DMA传输 */
    Dma_InitChannel(&dmaConfig);
    Dma_EnableChannel(ChannelConfig->DmaTxChannel);
    
    return E_OK;
    #else
    (void)Channel;
    (void)Data;
    (void)Length;
    return E_NOT_OK;
    #endif
}

/**
 * @brief 中断方式发送数据
 */
Std_ReturnType Uart_SendInterrupt(
    Uart_ChannelType Channel,
    const uint8* Data,
    uint32 Length
)
{
    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_SEND);
    UART_VALIDATE_POINTER(Data, UART_SERVICE_ID_SEND);
    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_SEND);
    
    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
    volatile uint32* base = Uart_BaseAddr[Channel];
    
    /* 检查通道状态 */
    if (state->Status == UART_STATE_TX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
        return E_NOT_OK;
    }
    
    /* 设置通道状态 */
    if (state->Status == UART_STATE_RX_BUSY) {
        state->Status = UART_STATE_TX_RX_BUSY;
    } else {
        state->Status = UART_STATE_TX_BUSY;
    }
    state->TxStatus = UART_TX_ACTIVE;
    state->TxBuffer.Buffer = (uint8*)Data;
    state->TxBuffer.Length = Length;
    state->TxBuffer.Transferred = 0;
    state->TxBuffer.Result = UART_RESULT_PENDING;
    state->TxStartTime = Uart_GetCurrentTime();
    
    /* 清除之前的中断标志 */
    *(base + (UART_USR1_OFFSET / 4)) |= USR1_TRDY;
    
    /* 使能发送中断 */
    *(base + (UART_UCR1_OFFSET / 4)) |= UCR1_TXMPTYEN;
    
    return E_OK;
}

/**
 * @brief 轮询方式接收数据
 */
Std_ReturnType Uart_Receive(
    Uart_ChannelType Channel,
    uint8* Buffer,
    uint32 Length
)
{
    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_RECEIVE);
    UART_VALIDATE_POINTER(Buffer, UART_SERVICE_ID_RECEIVE);
    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_RECEIVE);
    
    const Uart_ChannelConfigType* ChannelConfig = &Uart_ConfigPtr->ChannelConfig[Channel];
    volatile uint32* base = Uart_BaseAddr[Channel];
    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
    uint32 i;
    uint32 startTime;
    uint32 regVal;
    
    /* 检查通道状态 */
    if (state->Status == UART_STATE_RX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
        #if (UART_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, 
                        UART_SERVICE_ID_RECEIVE, UART_E_RX_BUSY);
        #endif
        return E_NOT_OK;
    }
    
    /* 设置通道状态 */
    if (state->Status == UART_STATE_TX_BUSY) {
        state->Status = UART_STATE_TX_RX_BUSY;
    } else {
        state->Status = UART_STATE_RX_BUSY;
    }
    state->RxStatus = UART_RX_ACTIVE;
    state->RxBuffer.Buffer = Buffer;
    state->RxBuffer.Length = Length;
    state->RxBuffer.Transferred = 0;
    state->RxBuffer.Result = UART_RESULT_PENDING;
    
    startTime = Uart_GetCurrentTime();
    
    for (i = 0; i < Length; i++) {
        /* 等待RX FIFO就绪 */
        while ((*(base + (UART_USR2_OFFSET / 4)) & USR2_RDR) == 0U ) {
            if (Uart_GetElapsedTime(startTime) > ChannelConfig->RxTimeout) {
                state->RxBuffer.Result = UART_RESULT_TIMEOUT;
                state->RxStatus = UART_RX_ERROR;
                state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
                                UART_STATE_TX_BUSY : UART_STATE_READY;
                return E_NOT_OK;
            }
            
            /* 检查错误 */
            regVal = *(base + (UART_USR2_OFFSET / 4));
            if (regVal & USR2_ORE) {
                state->ErrorCode = UART_E_OVERRUN;
                state->RxBuffer.Result = UART_RESULT_ERROR;
                state->RxStatus = UART_RX_ERROR;
                state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
                                UART_STATE_TX_BUSY : UART_STATE_READY;
                *(base + (UART_USR2_OFFSET / 4)) |= USR2_ORE; /* 清除标志 */
                return E_NOT_OK;
            }
        }
        
        /* 读取数据 */
        Buffer[i] = (uint8)(*(base + (UART_URXD_OFFSET / 4)));
        state->RxBuffer.Transferred++;
    }
    
    /* 接收完成 */
    state->RxBuffer.Result = UART_RESULT_OK;
    state->RxStatus = UART_RX_COMPLETE;
    state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
                    UART_STATE_TX_BUSY : UART_STATE_READY;
    
    /* 调用回调 */
    if (Uart_RxNotification[Channel] != NULL_PTR) {
        Uart_RxNotification[Channel]();
    }
    
    return E_OK;
}

/**
 * @brief DMA方式接收数据
 */
Std_ReturnType Uart_ReceiveDMA(
    Uart_ChannelType Channel,
    uint8* Buffer,
    uint32 Length
)
{
    #if (UART_DMA_SUPPORT == STD_ON)
    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_RECEIVEDMA);
    UART_VALIDATE_POINTER(Buffer, UART_SERVICE_ID_RECEIVEDMA);
    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_RECEIVEDMA);
    
    const Uart_ChannelConfigType* ChannelConfig = &Uart_ConfigPtr->ChannelConfig[Channel];
    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
    
    if (ChannelConfig->DmaEnabled == FALSE) {
        return E_NOT_OK;
    }
    
    /* 检查通道状态 */
    if (state->Status == UART_STATE_RX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
        return E_NOT_OK;
    }
    
    /* 设置通道状态 */
    if (state->Status == UART_STATE_TX_BUSY) {
        state->Status = UART_STATE_TX_RX_BUSY;
    } else {
        state->Status = UART_STATE_RX_BUSY;
    }
    state->RxStatus = UART_RX_ACTIVE;
    state->RxBuffer.Buffer = Buffer;
    state->RxBuffer.Length = Length;
    state->RxBuffer.Transferred = 0;
    state->RxBuffer.Result = UART_RESULT_PENDING;
    state->DmaActive = TRUE;
    
    /* 配置DMA */
    Dma_ConfigType dmaConfig;
    dmaConfig.Channel = ChannelConfig->DmaRxChannel;
    dmaConfig.SourceAddr = UART1_BASE_ADDR + UART_URXD_OFFSET + (Channel * 0x40000);
    dmaConfig.DestAddr = (uint32)Buffer;
    dmaConfig.TransferSize = Length;
    dmaConfig.SourceInc = FALSE;
    dmaConfig.DestInc = TRUE;
    dmaConfig.TransferWidth = DMA_WIDTH_8BIT;
    dmaConfig.Mode = DMA_MODE_NORMAL;
    
    /* 启动DMA传输 */
    Dma_InitChannel(&dmaConfig);
    Dma_EnableChannel(ChannelConfig->DmaRxChannel);
    
    return E_OK;
    #else
    (void)Channel;
    (void)Buffer;
    (void)Length;
    return E_NOT_OK;
    #endif
}

/**
 * @brief 中断方式接收数据
 */
Std_ReturnType Uart_ReceiveInterrupt(
    Uart_ChannelType Channel,
    uint8* Buffer,
    uint32 Length
)
{
    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_RECEIVE);
    UART_VALIDATE_POINTER(Buffer, UART_SERVICE_ID_RECEIVE);
    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_RECEIVE);
    
    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
    volatile uint32* base = Uart_BaseAddr[Channel];
    
    /* 检查通道状态 */
    if (state->Status == UART_STATE_RX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
        return E_NOT_OK;
    }
    
    /* 设置通道状态 */
    if (state->Status == UART_STATE_TX_BUSY) {
        state->Status = UART_STATE_TX_RX_BUSY;
    } else {
        state->Status = UART_STATE_RX_BUSY;
    }
    state->RxStatus = UART_RX_ACTIVE;
    state->RxBuffer.Buffer = Buffer;
    state->RxBuffer.Length = Length;
    state->RxBuffer.Transferred = 0;
    state->RxBuffer.Result = UART_RESULT_PENDING;
    state->RxStartTime = Uart_GetCurrentTime();
    
    /* 清除之前的中断标志 */
    *(base + (UART_USR1_OFFSET / 4)) |= USR1_RRDY;
    
    /* 使能接收中断 */
    *(base + (UART_UCR1_OFFSET / 4)) |= UCR1_RRDYEN;
    
    return E_OK;
}



/**
 * @brief 获取通道状态
 */
Uart_StatusType Uart_GetStatus(Uart_ChannelType Channel)
{
    if (Channel >= UART_CHANNEL_COUNT) {
        return UART_STATE_ERROR;
    }
    
    if (Uart_Initialized == FALSE) {
        return UART_STATE_UNINIT;
    }
    
    return Uart_ChannelState[Channel].Status;
}

/**
 * @brief 获取发送结果
 */
Uart_ResultType Uart_GetTxResult(Uart_ChannelType Channel)
{
    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_GETSTATUS);
    
    return Uart_ChannelState[Channel].TxBuffer.Result;
}

/**
 * @brief 获取接收结果
 */
Uart_ResultType Uart_GetRxResult(Uart_ChannelType Channel)
{
    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_GETSTATUS);
    
    return Uart_ChannelState[Channel].RxBuffer.Result;
}

/**
 * @brief 设置波特率
 */
Std_ReturnType Uart_SetBaudRate(Uart_ChannelType Channel, uint32 BaudRate)
{
    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_SETBAUDRATE);
    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_SETBAUDRATE);
    
    if (BaudRate == 0U ) {
        #if (UART_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, 
                        UART_SERVICE_ID_SETBAUDRATE, UART_E_PARAM_BAUDRATE);
        #endif
        return E_NOT_OK;
    }
    
    if (Uart_ChannelState[Channel].Status != UART_STATE_READY) {
        return E_NOT_OK;
    }
    
    Uart_SetBaudRateInternal(Channel, BaudRate);
    
    return E_OK;
}

/**
 * @brief 使能中断
 */
void Uart_EnableInterrupt(Uart_ChannelType Channel)
{
    if (Channel >= UART_CHANNEL_COUNT || Uart_Initialized == FALSE) {
        return;
    }
    
    volatile uint32* base = Uart_BaseAddr[Channel];
    *(base + (UART_UCR1_OFFSET / 4)) |= (UCR1_TXMPTYEN | UCR1_RRDYEN);
}

/**
 * @brief 禁用中断
 */
void Uart_DisableInterrupt(Uart_ChannelType Channel)
{
    if (Channel >= UART_CHANNEL_COUNT) {
        return;
    }
    
    volatile uint32* base = Uart_BaseAddr[Channel];
    *(base + (UART_UCR1_OFFSET / 4)) &= ~(UCR1_TXMPTYEN | UCR1_RRDYEN);
}

/**
 * @brief 清除FIFO
 */
void Uart_ClearFIFO(Uart_ChannelType Channel)
{
    if (Channel >= UART_CHANNEL_COUNT || Uart_Initialized == FALSE) {
        return;
    }
    
    #if (UART_FIFO_SUPPORT == STD_ON)
    volatile uint32* base = Uart_BaseAddr[Channel];
    uint32 ufcr = *(base + (UART_UFCR_OFFSET / 4));
    ufcr |= (1u << 15) | (1u << 14);
    *(base + (UART_UFCR_OFFSET / 4)) = ufcr;
    #endif
}

/**
 * @brief 中断处理函数
 */
void Uart_IsrHandler(Uart_ChannelType Channel)
{
    if (Channel >= UART_CHANNEL_COUNT || Uart_Initialized == FALSE) {
        return;
    }
    
    volatile uint32* base = Uart_BaseAddr[Channel];
    uint32 usr1 = *(base + (UART_USR1_OFFSET / 4));
    uint32 usr2 = *(base + (UART_USR2_OFFSET / 4));
    
    if (usr1 & USR1_RRDY) {
        Uart_ProcessRxInterrupt(Channel);
    }
    
    if (usr1 & USR1_TRDY) {
        Uart_ProcessTxInterrupt(Channel);
    }
    
    if (usr2 & (USR2_ORE | USR2_BRCD)) {
        Uart_ProcessError(Channel);
    }
    
    *(base + (UART_USR1_OFFSET / 4)) = usr1;
    *(base + (UART_USR2_OFFSET / 4)) = usr2;
}

/**
 * @brief 主函数
 */
void Uart_MainFunction(void)
{
    uint8 channel;
    Uart_ChannelStateType* state;
    
    if (Uart_Initialized == FALSE) {
        return;
    }
    
    for (channel = 0; channel < Uart_ConfigPtr->ChannelCount; channel++) {
        state = &Uart_ChannelState[channel];
        
        if ((state->Status == UART_STATE_TX_BUSY || state->Status == UART_STATE_TX_RX_BUSY)
            && state->TxStatus == UART_TX_ACTIVE) {
            if (Uart_GetElapsedTime(state->TxStartTime) > 
                Uart_ConfigPtr->ChannelConfig[channel].TxTimeout) {
                state->TxBuffer.Result = UART_RESULT_TIMEOUT;
                state->TxStatus = UART_TX_ERROR;
                Uart_Abort(channel);
            }
        }
        
        if ((state->Status == UART_STATE_RX_BUSY || state->Status == UART_STATE_TX_RX_BUSY)
            && state->RxStatus == UART_RX_ACTIVE) {
            if (Uart_GetElapsedTime(state->RxStartTime) > 
                Uart_ConfigPtr->ChannelConfig[channel].RxTimeout) {
                state->RxBuffer.Result = UART_RESULT_TIMEOUT;
                state->RxStatus = UART_RX_ERROR;
                Uart_Abort(channel);
            }
        }
    }
}

/**
 * @brief 传输中止
 */
void Uart_Abort(Uart_ChannelType Channel)
{
    if (Channel >= UART_CHANNEL_COUNT || Uart_Initialized == FALSE) {
        return;
    }
    
    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
    volatile uint32* base = Uart_BaseAddr[Channel];
    
    *(base + (UART_UCR1_OFFSET / 4)) &= ~(UCR1_TXMPTYEN | UCR1_RRDYEN);
    
    #if (UART_DMA_SUPPORT == STD_ON)
    if (state->DmaActive) {
        Dma_DisableChannel(Uart_ConfigPtr->ChannelConfig[Channel].DmaTxChannel);
        Dma_DisableChannel(Uart_ConfigPtr->ChannelConfig[Channel].DmaRxChannel);
        state->DmaActive = FALSE;
    }
    #endif
    
    state->Status = UART_STATE_READY;
    state->TxStatus = UART_TX_IDLE;
    state->RxStatus = UART_RX_IDLE;
}

#if (UART_VERSION_INFO_API == STD_ON)
void Uart_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    if (VersionInfo == NULL_PTR) {
        #if (UART_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, 
                        UART_SERVICE_ID_GETVERSIONINFO, UART_E_PARAM_POINTER);
        #endif
        return;
    }
    
    VersionInfo->vendorID = UART_VENDOR_ID;
    VersionInfo->moduleID = UART_MODULE_ID;
    VersionInfo->sw_major_version = UART_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = UART_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = UART_SW_PATCH_VERSION;
}
#endif

/* 辅助函数 */
static inline void Uart_WriteReg(Uart_ChannelType Channel, uint32 Offset, uint32 Value)
{
    *(Uart_BaseAddr[Channel] + (Offset / 4)) = Value;
}

static inline uint32 Uart_ReadReg(Uart_ChannelType Channel, uint32 Offset)
{
    return *(Uart_BaseAddr[Channel] + (Offset / 4));
}

static uint32 Uart_GetCurrentTime(void)
{
    return Gpt_GetTimeElapsed(0);
}

static uint32 Uart_GetElapsedTime(uint32 StartTime)
{
    uint32 current = Uart_GetCurrentTime();
    return (current >= StartTime) ? (current - StartTime) : ((0xFFFFFFFF - StartTime) + current);
}

static void Uart_ProcessTxInterrupt(Uart_ChannelType Channel)
{
    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
    volatile uint32* base = Uart_BaseAddr[Channel];
    
    if (state->TxBuffer.Transferred >= state->TxBuffer.Length) {
        state->TxStatus = UART_TX_COMPLETE;
        state->TxBuffer.Result = UART_RESULT_OK;
        state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? UART_STATE_RX_BUSY : UART_STATE_READY;
        *(base + (UART_UCR1_OFFSET / 4)) &= ~UCR1_TXMPTYEN;
        if (Uart_TxNotification[Channel] != NULL_PTR) {
            Uart_TxNotification[Channel]();
        }
        return;
    }
    
    *(base + (UART_UTXD_OFFSET / 4)) = state->TxBuffer.Buffer[state->TxBuffer.Transferred];
    state->TxBuffer.Transferred++;
}

static void Uart_ProcessRxInterrupt(Uart_ChannelType Channel)
{
    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
    volatile uint32* base = Uart_BaseAddr[Channel];
    
    if (state->RxBuffer.Transferred >= state->RxBuffer.Length) {
        state->RxStatus = UART_RX_COMPLETE;
        state->RxBuffer.Result = UART_RESULT_OK;
        state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? UART_STATE_TX_BUSY : UART_STATE_READY;
        *(base + (UART_UCR1_OFFSET / 4)) &= ~UCR1_RRDYEN;
        if (Uart_RxNotification[Channel] != NULL_PTR) {
            Uart_RxNotification[Channel]();
        }
        return;
    }
    
    state->RxBuffer.Buffer[state->RxBuffer.Transferred] = (uint8)(*(base + (UART_URXD_OFFSET / 4)));
    state->RxBuffer.Transferred++;
}

static void Uart_ProcessError(Uart_ChannelType Channel)
{
    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
    volatile uint32* base = Uart_BaseAddr[Channel];
    uint32 usr2 = *(base + (UART_USR2_OFFSET / 4));
    
    if (usr2 & USR2_ORE) state->ErrorCode = UART_E_OVERRUN;
    else if (usr2 & USR2_BRCD) state->ErrorCode = UART_E_BREAK;
    
    state->Status = UART_STATE_ERROR;
    if (Uart_ErrorNotification[Channel] != NULL_PTR) {
        Uart_ErrorNotification[Channel](state->ErrorCode);
    }
    *(base + (UART_USR2_OFFSET / 4)) |= (USR2_ORE | USR2_BRCD);
}
