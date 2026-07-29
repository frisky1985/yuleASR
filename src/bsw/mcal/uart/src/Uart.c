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

     1|/*
     2| * @file Uart.c
     3| * @brief UART驱动核心实现
     4| * 
     5| * 硬件: i.MX8M Mini UART
     6| * 特性: 支持轮询/中断/DMA模式，FIFO，流控制
     7| */
     8|
     9|#include "Uart.h"
    10|#include "SchM_Uart.h"
    11|
    12|#if (UART_DEV_ERROR_DETECT == STD_ON)
    13|#include "Det.h"
    14|#endif
    15|
    16|/*============================================================================
    17| * i.MX8M Mini UART寄存器定义
    18| *===========================================================================*/
    19|#define UART1_BASE_ADDR             0x30860000u
    20|#define UART2_BASE_ADDR             0x30890000u
    21|#define UART3_BASE_ADDR             0x30880000u
    22|#define UART4_BASE_ADDR             0x30A60000u
    23|
    24|/* UART寄存器偏移 */
    25|#define UART_URXD_OFFSET            0x00u   /* 接收数据寄存器 */
    26|#define UART_UTXD_OFFSET            0x40u   /* 发送数据寄存器 */
    27|#define UART_UCR1_OFFSET            0x80u   /* 控制寄存器1 */
    28|#define UART_UCR2_OFFSET            0x84u   /* 控制寄存器2 */
    29|#define UART_UCR3_OFFSET            0x88u   /* 控制寄存器3 */
    30|#define UART_UCR4_OFFSET            0x8Cu   /* 控制寄存器4 */
    31|#define UART_UFCR_OFFSET            0x90u   /* FIFO控制寄存器 */
    32|#define UART_USR1_OFFSET            0x94u   /* 状态寄存器1 */
    33|#define UART_USR2_OFFSET            0x98u   /* 状态寄存器2 */
    34|#define UART_UESC_OFFSET            0x9Cu   /* 逃逸字符寄存器 */
    35|#define UART_UTIM_OFFSET            0xA0u   /* 超时寄存器 */
    36|#define UART_UBIR_OFFSET            0xA4u   /* 波特率增量寄存器 */
    37|#define UART_UBMR_OFFSET            0xA8u   /* 波特率模数寄存器 */
    38|#define UART_UBRC_OFFSET            0xACu   /* 波特率检测寄存器 */
    39|#define UART_ONEMS_OFFSET           0xB0u   /* 1ms计数器寄存器 */
    40|#define UART_UTS_OFFSET             0xB4u   /* 测试寄存器 */
    41|
    42|/* UCR1寄存器位 */
    43|#define UCR1_UARTEN                 (1u << 0)   /* UART使能 */
    44|#define UCR1_DOZE                   (1u << 1)   /* DOZE模式 */
    45|#define UCR1_TXDMAEN                (1u << 3)   /* TX DMA使能 */
    46|#define UCR1_RXDMAEN                (1u << 2)   /* RX DMA使能 */
    47|#define UCR1_TXMPTYEN               (1u << 6)   /* TX FIFO空中断使能 */
    48|#define UCR1_RRDYEN                 (1u << 9)   /* RX FIFO就绪中断使能 */
    49|#define UCR1_RDMAEN                 (1u << 8)   /* RX DMA请求使能 */
    50|#define UCR1_TRDYEN                 (1u << 7)   /* TX DMA请求使能 */
    51|
    52|/* UCR2寄存器位 */
    53|#define UCR2_SRST                   (1u << 0)   /* 软件复位 */
    54|#define UCR2_RXEN                   (1u << 1)   /* 接收使能 */
    55|#define UCR2_TXEN                   (1u << 2)   /* 发送使能 */
    56|#define UCR2_PREN                   (1u << 3)   /* 奇偶校验使能 */
    57|#define UCR2_PROE                   (1u << 4)   /* 偶校验模式 */
    58|#define UCR2_STPB                   (1u << 5)   /* 2位停止位 */
    59|#define UCR2_WS                     (1u << 6)   /* 字符长度 (0=8位) */
    60|#define UCR2_RTSEN                  (1u << 7)   /* RTS使能 */
    61|#define UCR2_ATENEN                 (1u << 8)   /* 老化使能 */
    62|#define UCR2_CTS                    (1u << 9)   /* CTS使能 */
    63|#define UCR2_IRTS                   (1u << 10)  /* 忽略RTS */
    64|#define UCR2_ESCI                   (1u << 11)  /* 逃逸序列中断 */
    65|
    66|/* UCR3寄存器位 */
    67|#define UCR3_ACIEN                  (1u << 0)   /* 自动波特率中断 */
    68|#define UCR3_AIRINTEN               (1u << 1)   /* 空闲中断使能 */
    69|#define UCR3_RXDMUXSEL              (1u << 2)   /* RXD多路复用 */
    70|#define UCR3_DTRDEN                 (1u << 3)   /* DTR/DSR使能 */
    71|#define UCR3_ADNIMP                 (1u << 7)   /* 自动检测禁用 */
    72|
    73|/* UCR4寄存器位 */
    74|#define UCR4_DREN                   (1u << 0)   /* 数据就绪中断 */
    75|#define UCR4_OREN                   (1u << 1)   /* 溢出中断 */
    76|#define UCR4_BKEN                   (1u << 2)   /* 中断条件中断 */
    77|#define UCR4_TCEN                   (1u << 3)   /* 传输完成中断 */
    78|#define UCR4_LPBYP                  (1u << 4)   /* 循环旁路 */
    79|#define UCR4_CTSTL_SHIFT            10u         /* CTS触发水平位移 */
    80|
    81|/* UFCR寄存器位 */
    82|#define UFCR_RXTL_SHIFT             0u          /* RX FIFO阈值位移 */
    83|#define UFCR_RFDIV_SHIFT            7u          /* 参考时钟分额位移 */
    84|#define UFCR_TXTL_SHIFT             10u         /* TX FIFO阈值位移 */
    85|
    86|/* USR1寄存器位 */
    87|#define USR1_SAD                    (1u << 3)   /* 多点模式地址检测 */
    88|#define USR1_AWAKE                  (1u << 4)   /* 唤醒 */
    89|#define USR1_AIRINT                 (1u << 5)   /* 空闲中断 */
    90|#define USR1_RXDS                   (1u << 6)   /* RX FIFO满 */
    91|#define USR1_RRDY                   (1u << 9)   /* RX FIFO就绪 */
    92|#define USR1_FRAMERR                (1u << 10)  /* 帧错误 */
    93|#define USR1_ESCF                   (1u << 11)  /* 逃逸序列标志 */
    94|#define USR1_RTSD                   (1u << 12)  /* RTS状态 */
    95|#define USR1_AGTIM                  (1u << 13)  /* 自动波特率中断 */
    96|#define USR1_DTRD                   (1u << 15)  /* DTR/DSR检测 */
    97|
    98|/* USR2寄存器位 */
    99|#define USR2_RDR                    (1u << 0)   /* 数据就绪 */
   100|#define USR2_ORE                    (1u << 1)   /* 溢出错误 */
   101|#define USR2_BRCD                   (1u << 2)   /* 中断条件检测 */
   102|#define USR2_TXDC                   (1u << 3)   /* 传输完成 */
   103|#define USR2_RTSF                   (1u << 4)   /* RTS边沿触发 */
   104|#define USR2_DCDDELT                (1u << 6)   /* DCD变化 */
   105|#define USR2_DCDIN                  (1u << 7)   /* DCD输入 */
   106|#define USR2_TXFE                   (1u << 14)  /* TX FIFO空 */
   107|#define USR2_RXFE                   (1u << 15)  /* RX FIFO空 */
   108|
   109|/* 参考时钟分额 */
   110|#define UART_RFDIV_1                0x5u
   111|#define UART_RFDIV_2                0x0u
   112|#define UART_RFDIV_3                0x1u
   113|#define UART_RFDIV_4                0x2u
   114|#define UART_RFDIV_5                0x3u
   115|#define UART_RFDIV_6                0x4u
   116|#define UART_RFDIV_7                0x6u
   117|
   118|/*============================================================================
   119| * 全局变量
   120| *===========================================================================*/
   121|/* 驱动初始化状态 */
   122|static boolean Uart_Initialized = FALSE;
   123|
   124|/* 驱动配置指针 */
   125|static const Uart_ConfigType* Uart_ConfigPtr = NULL_PTR;
   126|
   127|/* 通道基地地址表 */
   128|static volatile uint32* const Uart_BaseAddr[UART_CHANNEL_COUNT] = {
   129|    (volatile uint32*)UART1_BASE_ADDR,
   130|    (volatile uint32*)UART2_BASE_ADDR,
   131|    (volatile uint32*)UART3_BASE_ADDR,
   132|    (volatile uint32*)UART4_BASE_ADDR
   133|};
   134|
   135|/* 通道状态跟踪 */
   136|typedef struct {
   137|    Uart_StatusType         Status;         /* 通道状态 */
   138|    Uart_TxStatusType       TxStatus;       /* 发送状态 */
   139|    Uart_RxStatusType       RxStatus;       /* 接收状态 */
   140|    Uart_BufferType         TxBuffer;       /* 发送缓冲区 */
   141|    Uart_BufferType         RxBuffer;       /* 接收缓冲区 */
   142|    uint32                  TxStartTime;    /* 发送开始时间 */
   143|    uint32                  RxStartTime;    /* 接收开始时间 */
   144|    uint8                   ErrorCode;      /* 错误码 */
   145|    boolean                 DmaActive;      /* DMA活动状态 */
   146|} Uart_ChannelStateType;
   147|
   148|static Uart_ChannelStateType Uart_ChannelState[UART_CHANNEL_COUNT];
   149|
   150|/* 回调函数指针 */
   151|static Uart_TxNotificationType   Uart_TxNotification[UART_CHANNEL_COUNT];
   152|static Uart_RxNotificationType   Uart_RxNotification[UART_CHANNEL_COUNT];
   153|static Uart_ErrorNotificationType Uart_ErrorNotification[UART_CHANNEL_COUNT];
   154|
   155|/*============================================================================
   156| * 验证宏
   157| *===========================================================================*/
   158|#if (UART_DEV_ERROR_DETECT == STD_ON)
   159|    #define UART_VALIDATE_CHANNEL(Channel, ApiId)         do {             if ((Channel) >= UART_CHANNEL_COUNT) {                 Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, (ApiId), UART_E_PARAM_CHANNEL);                 return E_NOT_OK;             }         } while(0)
   160|
   161|    #define UART_VALIDATE_POINTER(Ptr, ApiId)         do {             if ((Ptr) == NULL_PTR) {                 Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, (ApiId), UART_E_PARAM_POINTER);                 return E_NOT_OK;             }         } while(0)
   162|
   163|    #define UART_VALIDATE_INITIALIZED(ApiId)         do {             if (Uart_Initialized == FALSE) {                 Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, (ApiId), UART_E_UNINIT);                 return E_NOT_OK;             }         } while(0)
   164|#else
   165|    #define UART_VALIDATE_CHANNEL(Channel, ApiId)
   166|    #define UART_VALIDATE_POINTER(Ptr, ApiId)
   167|    #define UART_VALIDATE_INITIALIZED(ApiId)
   168|#endif
   169|
   170|/*============================================================================
   171| * 内部函数声明
   172| *===========================================================================*/
   173|static void Uart_HwInit(Uart_ChannelType Channel);
   174|static void Uart_HwDeInit(Uart_ChannelType Channel);
   175|static void Uart_SetBaudRateInternal(Uart_ChannelType Channel, uint32 BaudRate);
   176|static void Uart_ProcessTxInterrupt(Uart_ChannelType Channel);
   177|static void Uart_ProcessRxInterrupt(Uart_ChannelType Channel);
   178|static void Uart_ProcessError(Uart_ChannelType Channel);
   179|static inline void Uart_WriteReg(Uart_ChannelType Channel, uint32 Offset, uint32 Value);
   180|static inline uint32 Uart_ReadReg(Uart_ChannelType Channel, uint32 Offset);
   181|
   182|/*============================================================================
   183| * API实现
   184| *===========================================================================*/
   185|
   186|/**
   187| * @brief UART驱动初始化
   188| * @param Config 驱动配置指针
   189| */
   190|void Uart_Init(const Uart_ConfigType* Config)
   191|{
   192|    uint8 channel;
   193|    
   194|    #if (UART_DEV_ERROR_DETECT == STD_ON)
   195|    if (Config == NULL_PTR) {
   196|        Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, 
   197|                        UART_SERVICE_ID_INIT, UART_E_PARAM_POINTER);
   198|        return;
   199|    }
   200|    if (Uart_Initialized == TRUE) {
   201|        Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, 
   202|                        UART_SERVICE_ID_INIT, UART_E_ALREADY_INITIALIZED);
   203|        return;
   204|    }
   205|    #endif
   206|    
   207|    /* 保存配置指针 */
   208|    Uart_ConfigPtr = Config;
   209|    
   210|    /* 初始化所有通道 */
   211|    for (channel = 0; channel < Config->ChannelCount; channel++) {
   212|        /* 清零状态 */
   213|        Uart_ChannelState[channel].Status = UART_STATE_READY;
   214|        Uart_ChannelState[channel].TxStatus = UART_TX_IDLE;
   215|        Uart_ChannelState[channel].RxStatus = UART_RX_IDLE;
   216|        Uart_ChannelState[channel].ErrorCode = UART_E_NO_ERROR;
   217|        Uart_ChannelState[channel].DmaActive = FALSE;
   218|        
   219|        /* 硬件初始化 */
   220|        Uart_HwInit(channel);
   221|    }
   222|    
   223|    Uart_Initialized = TRUE;
   224|}
   225|
   226|/**
   227| * @brief UART驱动反初始化
   228| */
   229|void Uart_DeInit(void)
   230|{
   231|    uint8 channel;
   232|    
   233|    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_DEINIT);
   234|    
   235|    /* 反初始化所有通道 */
   236|    for (channel = 0; channel < Uart_ConfigPtr->ChannelCount; channel++) {
   237|        Uart_HwDeInit(channel);
   238|        
   239|        Uart_ChannelState[channel].Status = UART_STATE_UNINIT;
   240|        Uart_ChannelState[channel].TxStatus = UART_TX_IDLE;
   241|        Uart_ChannelState[channel].RxStatus = UART_RX_IDLE;
   242|    }
   243|    
   244|    Uart_ConfigPtr = NULL_PTR;
   245|    Uart_Initialized = FALSE;
   246|}
   247|
   248|/**
   249| * @brief 硬件初始化
   250| */
   251|static void Uart_HwInit(Uart_ChannelType Channel)
   252|{
   253|    const Uart_ChannelConfigType* ChannelConfig = &Uart_ConfigPtr->ChannelConfig[Channel];
   254|    volatile uint32* base = Uart_BaseAddr[Channel];
   255|    uint32 regVal;
   256|    
   257|    /* 软件复位 */
   258|    *(base + (UART_UCR2_OFFSET / 4)) = 0x0;
   259|    while (*(base + (UART_UCR2_OFFSET / 4)) & UCR2_SRST);
   260|    
   261|    /* 配置UCR1 */
   262|    regVal = UCR1_UARTEN;
   263|    if (ChannelConfig->DmaEnabled) {
   264|        regVal |= (UCR1_TXDMAEN | UCR1_RXDMAEN | UCR1_RDMAEN | UCR1_TRDYEN);
   265|    }
   266|    if (ChannelConfig->OpMode == UART_MODE_INTERRUPT) {
   267|        regVal |= (UCR1_TXMPTYEN | UCR1_RRDYEN);
   268|    }
   269|    *(base + (UART_UCR1_OFFSET / 4)) = regVal;
   270|    
   271|    /* 配置UCR2 */
   272|    regVal = UCR2_SRST | UCR2_RXEN | UCR2_TXEN;
   273|    
   274|    /* 校验配置 */
   275|    if (ChannelConfig->Parity != UART_PARITY_NONE) {
   276|        regVal |= UCR2_PREN;
   277|        if (ChannelConfig->Parity == UART_PARITY_EVEN) {
   278|            regVal |= UCR2_PROE;
   279|        }
   280|    }
   281|    
   282|    /* 停止位 */
   283|    if (ChannelConfig->StopBits == UART_STOP_BITS_2) {
   284|        regVal |= UCR2_STPB;
   285|    }
   286|    
   287|    /* 数据位 */
   288|    if (ChannelConfig->DataBits != UART_DATA_BITS_8) {
   289|        regVal |= UCR2_WS;
   290|    }
   291|    
   292|    /* 流控制 */
   293|    if (ChannelConfig->HwHandshake != UART_HW_HANDSHAKE_NONE) {
   294|        regVal |= UCR2_CTS;
   295|        if (ChannelConfig->HwHandshake == UART_HW_HANDSHAKE_RTS_CTS) {
   296|            regVal |= UCR2_RTSEN;
   297|        }
   298|    }
   299|    
   300|    *(base + (UART_UCR2_OFFSET / 4)) = regVal;
   301|    
   302|    /* 配置UCR3 */
   303|    regVal = UCR3_RXDMUXSEL | UCR3_ADNIMP;
   304|    *(base + (UART_UCR3_OFFSET / 4)) = regVal;
   305|    
   306|    /* 配置UCR4 */
   307|    regVal = 0;
   308|    if (ChannelConfig->OpMode == UART_MODE_INTERRUPT) {
   309|        regVal |= (UCR4_DREN | UCR4_OREN);
   310|    }
   311|    *(base + (UART_UCR4_OFFSET / 4)) = regVal;
   312|    
   313|    /* 配置FIFO */
   314|    if (ChannelConfig->FifoMode == UART_FIFO_ENABLED) {
   315|        regVal = ((ChannelConfig->RxFifoThreshold << UFCR_RXTL_SHIFT) |
   316|                  (UART_RFDIV_1 << UFCR_RFDIV_SHIFT) |
   317|                  (ChannelConfig->TxFifoThreshold << UFCR_TXTL_SHIFT));
   318|        *(base + (UART_UFCR_OFFSET / 4)) = regVal;
   319|    } else {
   320|        *(base + (UART_UFCR_OFFSET / 4)) = (UART_RFDIV_1 << UFCR_RFDIV_SHIFT);
   321|    }
   322|    
   323|    /* 设置波特率 */
   324|    Uart_SetBaudRateInternal(Channel, ChannelConfig->BaudRate);
   325|}
   326|
   327|/**
   328| * @brief 硬件反初始化
   329| */
   330|static void Uart_HwDeInit(Uart_ChannelType Channel)
   331|{
   332|    volatile uint32* base = Uart_BaseAddr[Channel];
   333|    
   334|    /* 禁用UART */
   335|    *(base + (UART_UCR1_OFFSET / 4)) = 0;
   336|    *(base + (UART_UCR2_OFFSET / 4)) = 0;
   337|}
   338|
   339|/**
   340| * @brief 内部波特率设置
   341| */
   342|static void Uart_SetBaudRateInternal(Uart_ChannelType Channel, uint32 BaudRate)
   343|{
   344|    volatile uint32* base = Uart_BaseAddr[Channel];
   345|    uint32 refClock = UART_REF_CLOCK_HZ;
   346|    uint32 div;
   347|    uint32 bfDiv;
   348|    uint32 bmDiv;
   349|    
   350|    /* 计算分额 */
   351|    div = refClock / (BaudRate * 16);
   352|    
   353|    /* UBIR = 波特率 - 1 */
   354|    bfDiv = BaudRate - 1;
   355|    
   356|    /* UBMR = 参考时钟 / (16 * 分额) - 1 */
   357|    bmDiv = (refClock / div / 16) - 1;
   358|    
   359|    *(base + (UART_UBIR_OFFSET / 4)) = bfDiv;
   360|    *(base + (UART_UBMR_OFFSET / 4)) = bmDiv;
   361|}
   362|
   363|
   364|/**
   365| * @brief 轮询方式发送数据
   366| * @param Channel 通道ID
   367| * @param Data 数据指针
   368| * @param Length 数据长度
   369| * @return E_OK成功，E_NOT_OK失败
   370| */
   371|Std_ReturnType Uart_Send(
   372|    Uart_ChannelType Channel,
   373|    const uint8* Data,
   374|    uint32 Length
   375|)
   376|{
   377|    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_SEND);
   378|    UART_VALIDATE_POINTER(Data, UART_SERVICE_ID_SEND);
   379|    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_SEND);
   380|    
   381|    const Uart_ChannelConfigType* ChannelConfig = &Uart_ConfigPtr->ChannelConfig[Channel];
   382|    volatile uint32* base = Uart_BaseAddr[Channel];
   383|    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
   384|    uint32 i;
   385|    uint32 startTime;
   386|    
   387|    /* 检查通道状态 */
   388|    if (state->Status == UART_STATE_TX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
   389|        #if (UART_DEV_ERROR_DETECT == STD_ON)
   390|        Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, 
   391|                        UART_SERVICE_ID_SEND, UART_E_TX_BUSY);
   392|        #endif
   393|        return E_NOT_OK;
   394|    }
   395|    
   396|    /* 设置通道状态 */
   397|    if (state->Status == UART_STATE_RX_BUSY) {
   398|        state->Status = UART_STATE_TX_RX_BUSY;
   399|    } else {
   400|        state->Status = UART_STATE_TX_BUSY;
   401|    }
   402|    state->TxStatus = UART_TX_ACTIVE;
   403|    state->TxBuffer.Buffer = (uint8*)Data;
   404|    state->TxBuffer.Length = Length;
   405|    state->TxBuffer.Transferred = 0;
   406|    state->TxBuffer.Result = UART_RESULT_PENDING;
   407|    
   408|    /* 轮询方式发送 */
   409|    startTime = Uart_GetCurrentTime();
   410|    
   411|    for (i = 0; i < Length; i++) {
   412|        /* 等待TX FIFO可用 */
   413|        while ((*(base + (UART_USR1_OFFSET / 4)) & USR1_TRDY) == 0) {
   414|            if (Uart_GetElapsedTime(startTime) > ChannelConfig->TxTimeout) {
   415|                state->TxBuffer.Result = UART_RESULT_TIMEOUT;
   416|                state->TxStatus = UART_TX_ERROR;
   417|                state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
   418|                                UART_STATE_RX_BUSY : UART_STATE_READY;
   419|                return E_NOT_OK;
   420|            }
   421|        }
   422|        
   423|        /* 写入数据 */
   424|        *(base + (UART_UTXD_OFFSET / 4)) = Data[i];
   425|        state->TxBuffer.Transferred++;
   426|    }
   427|    
   428|    /* 等待传输完成 */
   429|    while ((*(base + (UART_USR2_OFFSET / 4)) & USR2_TXDC) == 0) {
   430|        if (Uart_GetElapsedTime(startTime) > ChannelConfig->TxTimeout) {
   431|            state->TxBuffer.Result = UART_RESULT_TIMEOUT;
   432|            state->TxStatus = UART_TX_ERROR;
   433|            state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
   434|                            UART_STATE_RX_BUSY : UART_STATE_READY;
   435|            return E_NOT_OK;
   436|        }
   437|    }
   438|    
   439|    /* 发送完成 */
   440|    state->TxBuffer.Result = UART_RESULT_OK;
   441|    state->TxStatus = UART_TX_COMPLETE;
   442|    state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
   443|                    UART_STATE_RX_BUSY : UART_STATE_READY;
   444|    
   445|    /* 调用回调 */
   446|    if (Uart_TxNotification[Channel] != NULL_PTR) {
   447|        Uart_TxNotification[Channel]();
   448|    }
   449|    
   450|    return E_OK;
   451|}
   452|
   453|/**
   454| * @brief DMA方式发送数据
   455| */
   456|Std_ReturnType Uart_SendDMA(
   457|    Uart_ChannelType Channel,
   458|    const uint8* Data,
   459|    uint32 Length
   460|)
   461|{
   462|    #if (UART_DMA_SUPPORT == STD_ON)
   463|    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_SENDDMA);
   464|    UART_VALIDATE_POINTER(Data, UART_SERVICE_ID_SENDDMA);
   465|    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_SENDDMA);
   466|    
   467|    const Uart_ChannelConfigType* ChannelConfig = &Uart_ConfigPtr->ChannelConfig[Channel];
   468|    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
   469|    
   470|    if (ChannelConfig->DmaEnabled == FALSE) {
   471|        return E_NOT_OK;
   472|    }
   473|    
   474|    /* 检查通道状态 */
   475|    if (state->Status == UART_STATE_TX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
   476|        return E_NOT_OK;
   477|    }
   478|    
   479|    /* 设置通道状态 */
   480|    if (state->Status == UART_STATE_RX_BUSY) {
   481|        state->Status = UART_STATE_TX_RX_BUSY;
   482|    } else {
   483|        state->Status = UART_STATE_TX_BUSY;
   484|    }
   485|    state->TxStatus = UART_TX_ACTIVE;
   486|    state->TxBuffer.Buffer = (uint8*)Data;
   487|    state->TxBuffer.Length = Length;
   488|    state->TxBuffer.Transferred = 0;
   489|    state->TxBuffer.Result = UART_RESULT_PENDING;
   490|    state->DmaActive = TRUE;
   491|    
   492|    /* 配置DMA */
   493|    Dma_ConfigType dmaConfig;
   494|    dmaConfig.Channel = ChannelConfig->DmaTxChannel;
   495|    dmaConfig.SourceAddr = (uint32)Data;
   496|    dmaConfig.DestAddr = UART1_BASE_ADDR + UART_UTXD_OFFSET + (Channel * 0x40000);
   497|    dmaConfig.TransferSize = Length;
   498|    dmaConfig.SourceInc = TRUE;
   499|    dmaConfig.DestInc = FALSE;
   500|    dmaConfig.TransferWidth = DMA_WIDTH_8BIT;
   501|    dmaConfig.Mode = DMA_MODE_NORMAL;
   502|    
   503|    /* 启动DMA传输 */
   504|    Dma_InitChannel(&dmaConfig);
   505|    Dma_EnableChannel(ChannelConfig->DmaTxChannel);
   506|    
   507|    return E_OK;
   508|    #else
   509|    (void)Channel;
   510|    (void)Data;
   511|    (void)Length;
   512|    return E_NOT_OK;
   513|    #endif
   514|}
   515|
   516|/**
   517| * @brief 中断方式发送数据
   518| */
   519|Std_ReturnType Uart_SendInterrupt(
   520|    Uart_ChannelType Channel,
   521|    const uint8* Data,
   522|    uint32 Length
   523|)
   524|{
   525|    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_SEND);
   526|    UART_VALIDATE_POINTER(Data, UART_SERVICE_ID_SEND);
   527|    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_SEND);
   528|    
   529|    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
   530|    volatile uint32* base = Uart_BaseAddr[Channel];
   531|    
   532|    /* 检查通道状态 */
   533|    if (state->Status == UART_STATE_TX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
   534|        return E_NOT_OK;
   535|    }
   536|    
   537|    /* 设置通道状态 */
   538|    if (state->Status == UART_STATE_RX_BUSY) {
   539|        state->Status = UART_STATE_TX_RX_BUSY;
   540|    } else {
   541|        state->Status = UART_STATE_TX_BUSY;
   542|    }
   543|    state->TxStatus = UART_TX_ACTIVE;
   544|    state->TxBuffer.Buffer = (uint8*)Data;
   545|    state->TxBuffer.Length = Length;
   546|    state->TxBuffer.Transferred = 0;
   547|    state->TxBuffer.Result = UART_RESULT_PENDING;
   548|    state->TxStartTime = Uart_GetCurrentTime();
   549|    
   550|    /* 清除之前的中断标志 */
   551|    *(base + (UART_USR1_OFFSET / 4)) |= USR1_TRDY;
   552|    
   553|    /* 使能发送中断 */
   554|    *(base + (UART_UCR1_OFFSET / 4)) |= UCR1_TXMPTYEN;
   555|    
   556|    return E_OK;
   557|}
   558|
   559|/**
   560| * @brief 轮询方式接收数据
   561| */
   562|Std_ReturnType Uart_Receive(
   563|    Uart_ChannelType Channel,
   564|    uint8* Buffer,
   565|    uint32 Length
   566|)
   567|{
   568|    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_RECEIVE);
   569|    UART_VALIDATE_POINTER(Buffer, UART_SERVICE_ID_RECEIVE);
   570|    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_RECEIVE);
   571|    
   572|    const Uart_ChannelConfigType* ChannelConfig = &Uart_ConfigPtr->ChannelConfig[Channel];
   573|    volatile uint32* base = Uart_BaseAddr[Channel];
   574|    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
   575|    uint32 i;
   576|    uint32 startTime;
   577|    uint32 regVal;
   578|    
   579|    /* 检查通道状态 */
   580|    if (state->Status == UART_STATE_RX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
   581|        #if (UART_DEV_ERROR_DETECT == STD_ON)
   582|        Det_ReportError(UART_MODULE_ID, UART_INSTANCE_ID, 
   583|                        UART_SERVICE_ID_RECEIVE, UART_E_RX_BUSY);
   584|        #endif
   585|        return E_NOT_OK;
   586|    }
   587|    
   588|    /* 设置通道状态 */
   589|    if (state->Status == UART_STATE_TX_BUSY) {
   590|        state->Status = UART_STATE_TX_RX_BUSY;
   591|    } else {
   592|        state->Status = UART_STATE_RX_BUSY;
   593|    }
   594|    state->RxStatus = UART_RX_ACTIVE;
   595|    state->RxBuffer.Buffer = Buffer;
   596|    state->RxBuffer.Length = Length;
   597|    state->RxBuffer.Transferred = 0;
   598|    state->RxBuffer.Result = UART_RESULT_PENDING;
   599|    
   600|    startTime = Uart_GetCurrentTime();
   601|    
   602|    for (i = 0; i < Length; i++) {
   603|        /* 等待RX FIFO就绪 */
   604|        while ((*(base + (UART_USR2_OFFSET / 4)) & USR2_RDR) == 0) {
   605|            if (Uart_GetElapsedTime(startTime) > ChannelConfig->RxTimeout) {
   606|                state->RxBuffer.Result = UART_RESULT_TIMEOUT;
   607|                state->RxStatus = UART_RX_ERROR;
   608|                state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
   609|                                UART_STATE_TX_BUSY : UART_STATE_READY;
   610|                return E_NOT_OK;
   611|            }
   612|            
   613|            /* 检查错误 */
   614|            regVal = *(base + (UART_USR2_OFFSET / 4));
   615|            if (regVal & USR2_ORE) {
   616|                state->ErrorCode = UART_E_OVERRUN;
   617|                state->RxBuffer.Result = UART_RESULT_ERROR;
   618|                state->RxStatus = UART_RX_ERROR;
   619|                state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
   620|                                UART_STATE_TX_BUSY : UART_STATE_READY;
   621|                *(base + (UART_USR2_OFFSET / 4)) |= USR2_ORE; /* 清除标志 */
   622|                return E_NOT_OK;
   623|            }
   624|        }
   625|        
   626|        /* 读取数据 */
   627|        Buffer[i] = (uint8)(*(base + (UART_URXD_OFFSET / 4)));
   628|        state->RxBuffer.Transferred++;
   629|    }
   630|    
   631|    /* 接收完成 */
   632|    state->RxBuffer.Result = UART_RESULT_OK;
   633|    state->RxStatus = UART_RX_COMPLETE;
   634|    state->Status = (state->Status == UART_STATE_TX_RX_BUSY) ? 
   635|                    UART_STATE_TX_BUSY : UART_STATE_READY;
   636|    
   637|    /* 调用回调 */
   638|    if (Uart_RxNotification[Channel] != NULL_PTR) {
   639|        Uart_RxNotification[Channel]();
   640|    }
   641|    
   642|    return E_OK;
   643|}
   644|
   645|/**
   646| * @brief DMA方式接收数据
   647| */
   648|Std_ReturnType Uart_ReceiveDMA(
   649|    Uart_ChannelType Channel,
   650|    uint8* Buffer,
   651|    uint32 Length
   652|)
   653|{
   654|    #if (UART_DMA_SUPPORT == STD_ON)
   655|    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_RECEIVEDMA);
   656|    UART_VALIDATE_POINTER(Buffer, UART_SERVICE_ID_RECEIVEDMA);
   657|    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_RECEIVEDMA);
   658|    
   659|    const Uart_ChannelConfigType* ChannelConfig = &Uart_ConfigPtr->ChannelConfig[Channel];
   660|    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
   661|    
   662|    if (ChannelConfig->DmaEnabled == FALSE) {
   663|        return E_NOT_OK;
   664|    }
   665|    
   666|    /* 检查通道状态 */
   667|    if (state->Status == UART_STATE_RX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
   668|        return E_NOT_OK;
   669|    }
   670|    
   671|    /* 设置通道状态 */
   672|    if (state->Status == UART_STATE_TX_BUSY) {
   673|        state->Status = UART_STATE_TX_RX_BUSY;
   674|    } else {
   675|        state->Status = UART_STATE_RX_BUSY;
   676|    }
   677|    state->RxStatus = UART_RX_ACTIVE;
   678|    state->RxBuffer.Buffer = Buffer;
   679|    state->RxBuffer.Length = Length;
   680|    state->RxBuffer.Transferred = 0;
   681|    state->RxBuffer.Result = UART_RESULT_PENDING;
   682|    state->DmaActive = TRUE;
   683|    
   684|    /* 配置DMA */
   685|    Dma_ConfigType dmaConfig;
   686|    dmaConfig.Channel = ChannelConfig->DmaRxChannel;
   687|    dmaConfig.SourceAddr = UART1_BASE_ADDR + UART_URXD_OFFSET + (Channel * 0x40000);
   688|    dmaConfig.DestAddr = (uint32)Buffer;
   689|    dmaConfig.TransferSize = Length;
   690|    dmaConfig.SourceInc = FALSE;
   691|    dmaConfig.DestInc = TRUE;
   692|    dmaConfig.TransferWidth = DMA_WIDTH_8BIT;
   693|    dmaConfig.Mode = DMA_MODE_NORMAL;
   694|    
   695|    /* 启动DMA传输 */
   696|    Dma_InitChannel(&dmaConfig);
   697|    Dma_EnableChannel(ChannelConfig->DmaRxChannel);
   698|    
   699|    return E_OK;
   700|    #else
   701|    (void)Channel;
   702|    (void)Buffer;
   703|    (void)Length;
   704|    return E_NOT_OK;
   705|    #endif
   706|}
   707|
   708|/**
   709| * @brief 中断方式接收数据
   710| */
   711|Std_ReturnType Uart_ReceiveInterrupt(
   712|    Uart_ChannelType Channel,
   713|    uint8* Buffer,
   714|    uint32 Length
   715|)
   716|{
   717|    UART_VALIDATE_CHANNEL(Channel, UART_SERVICE_ID_RECEIVE);
   718|    UART_VALIDATE_POINTER(Buffer, UART_SERVICE_ID_RECEIVE);
   719|    UART_VALIDATE_INITIALIZED(UART_SERVICE_ID_RECEIVE);
   720|    
   721|    Uart_ChannelStateType* state = &Uart_ChannelState[Channel];
   722|    volatile uint32* base = Uart_BaseAddr[Channel];
   723|    
   724|    /* 检查通道状态 */
   725|    if (state->Status == UART_STATE_RX_BUSY || state->Status == UART_STATE_TX_RX_BUSY) {
   726|        return E_NOT_OK;
   727|    }
   728|    
   729|    /* 设置通道状态 */
   730|    if (state->Status == UART_STATE_TX_BUSY) {
   731|        state->Status = UART_STATE_TX_RX_BUSY;
   732|    } else {
   733|        state->Status = UART_STATE_RX_BUSY;
   734|    }
   735|    state->RxStatus = UART_RX_ACTIVE;
   736|    state->RxBuffer.Buffer = Buffer;
   737|    state->RxBuffer.Length = Length;
   738|    state->RxBuffer.Transferred = 0;
   739|    state->RxBuffer.Result = UART_RESULT_PENDING;
   740|    state->RxStartTime = Uart_GetCurrentTime();
   741|    
   742|    /* 清除之前的中断标志 */
   743|    *(base + (UART_USR1_OFFSET / 4)) |= USR1_RRDY;
   744|    
   745|    /* 使能接收中断 */
   746|    *(base + (UART_UCR1_OFFSET / 4)) |= UCR1_RRDYEN;
   747|    
   748|    return E_OK;
   749|}
   750|
   751|

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
    
    if (BaudRate == 0) {
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
