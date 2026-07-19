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
 * @file Spi.c
 * @brief SPI驱动实现 - i.MX8M Mini ECSPI
 * @req SHALL_SPI - SPI驱动实现 - i.MX8M Mini ECSPI
 * 
 * 支持DMA、中断、主机模式
 */

#include "Spi.h"
#include "Mcu.h"

#if (SPI_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"

/* Version check */
#if defined(SPI_AR_RELEASE_MAJOR_VERSION) && (SPI_AR_RELEASE_MAJOR_VERSION != 4u)
#error "Spi: AR major mismatch"
#endif
#if defined(SPI_AR_RELEASE_MINOR_VERSION) && (SPI_AR_RELEASE_MINOR_VERSION != 4u)
#error "Spi: AR minor mismatch"
#endif
#endif

/* i.MX8M Mini ECSPI寄存器 */
#define ECSPI1_BASE         0x30820000u
#define ECSPI2_BASE         0x30830000u
#define ECSPI3_BASE         0x30840000u

/* 寄存器偏移 */
#define ECSPI_RXDATA        0x00u
#define ECSPI_TXDATA        0x04u
#define ECSPI_CONREG        0x08u   /* 控制寄存器 */
#define ECSPI_CONFIGREG     0x0Cu   /* 配置寄存器 */
#define ECSPI_INTREG        0x10u   /* 中断寄存器 */
#define ECSPI_DMAREG        0x14u   /* DMA寄存器 */
#define ECSPI_STATREG       0x18u   /* 状态寄存器 */
#define ECSPI_PERIODREG     0x1Cu   /* 周期寄存器 */
#define ECSPI_TESTREG       0x20u   /* 测试寄存器 */
#define ECSPI_MSGDATA       0x40u   /* 消息数据寄存器 */

/* CONREG位 */
#define CONREG_EN           (1u << 0)
#define CONREG_MODE         (1u << 1)
#define CONREG_XCH          (1u << 2)
#define CONREG_SMC          (1u << 3)
#define CONREG_BURST_EN     (1u << 4)
#define CONREG_CHANNEL_SHIFT 18
#define CONREG_DRCTL_SHIFT  16

/* STATREG位 */
#define STATREG_RR          (1u << 3)   /* RX FIFO就绪 */
#define STATREG_RF          (1u << 4)   /* RX FIFO满 */
#define STATREG_TE          (1u << 5)   /* TX FIFO空 */
#define STATREG_TF          (1u << 6)   /* TX FIFO满 */
#define STATREG_RO          (1u << 7)   /* RX FIFO溢出 */
#define STATREG_TC          (1u << 7)   /* 传输完成 */

/* INTREG位 */
#define INTREG_TEEN         (1u << 5)   /* TX空中断使能 */
#define INTREG_TDEN         (1u << 6)   /* TX数据中断使能 */
#define INTREG_TFEN         (1u << 7)   /* TX FIFO满中断使能 */
#define INTREG_RREN         (1u << 8)   /* RX就绪中断使能 */
#define INTREG_RDEN         (1u << 9)   /* RX数据中断使能 */
#define INTREG_RFEN         (1u << 10)  /* RX FIFO满中断使能 */
#define INTREG_ROEN         (1u << 11)  /* RX溢出中断使能 */
#define INTREG_TCEN         (1u << 7)   /* 传输完成中断使能 */

/* DMAREG位 */
#define DMAREG_RXDEN        (1u << 0)   /* RX DMA使能 */
#define DMAREG_TXDEN        (1u << 1)   /* TX DMA使能 */
#define DMAREG_RX_THRESHOLD_SHIFT 16
#define DMAREG_TX_THRESHOLD_SHIFT 24

/* 全局变量 */
static boolean Spi_Initialized = FALSE;
static const Spi_ConfigType* Spi_ConfigPtr = NULL_PTR;
static Spi_StatusType Spi_Status = SPI_UNINIT;

static volatile uint32* const Spi_BaseAddr[SPI_CHANNEL_COUNT] = {
    (volatile uint32*)ECSPI1_BASE,
    (volatile uint32*)ECSPI2_BASE,
    (volatile uint32*)ECSPI3_BASE,
    (volatile uint32*)ECSPI3_BASE
};

typedef struct {
    Spi_JobResultType       JobResult;
    uint8                   ActiveDevice;
    const uint8*            TxBuffer;
    uint8*                  RxBuffer;
    uint32                  Length;
    uint32                  Transferred;
    boolean                 DmaActive;
    uint32                  StartTime;
} Spi_ChannelStateType;

static Spi_ChannelStateType Spi_ChannelState[SPI_CHANNEL_COUNT];

/* 验证宏 */
#if (SPI_DEV_ERROR_DETECT == STD_ON)
    #define SPI_VALIDATE_INITIALIZED(ApiId)         do { if (Spi_Initialized == FALSE) {             Det_ReportError(SPI_MODULE_ID, 0, (ApiId), SPI_E_UNINIT);             return E_NOT_OK; } } while(0)
#else
    #define SPI_VALIDATE_INITIALIZED(ApiId)
#endif

/**
 * @brief SPI初始化
 * @req SHALL_SPI - SPI初始化
 */
void Spi_Init(const Spi_ConfigType* Config)
{
    uint8 i;
    
    if (Config == NULL_PTR) {
        #if (SPI_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(SPI_MODULE_ID, 0, SPI_SERVICE_ID_INIT, SPI_E_PARAM_POINTER);
        #endif
        return;
    }
    
    Spi_ConfigPtr = Config;
    
    for (i = 0; i < Config->ChannelCount && i < SPI_CHANNEL_COUNT; i++) {
        volatile uint32* base = Spi_BaseAddr[i];
        const Spi_ChannelConfigType* chCfg = &Config->ChannelConfig[i];
        uint32 conreg = 0;
        uint32 cfgreg = 0;
        
        /* 禁用SPI */
        *(base + (ECSPI_CONREG / 4)) = 0;
        
        /* 配置控制寄存器 */
        conreg = CONREG_EN | CONREG_MODE;  /* 主机模式 */
        conreg |= (i << CONREG_CHANNEL_SHIFT);
        *(base + (ECSPI_CONREG / 4)) = conreg;
        
        /* 配置时序 */
        cfgreg = (chCfg->ClockMode << 0);
        if (chCfg->DataMode == SPI_DATA_MODE_16BIT) {
            cfgreg |= (1u << 4);
        } else if (chCfg->DataMode == SPI_DATA_MODE_32BIT) {
            cfgreg |= (3u << 4);
        }
        *(base + (ECSPI_CONFIGREG / 4)) = cfgreg;
        
        /* 配置DMA */
        if (chCfg->DmaEnabled) {
            uint32 dmareg = DMAREG_RXDEN | DMAREG_TXDEN;
            dmareg |= (SPI_DMA_FIFO_THRESHOLD << DMAREG_RX_THRESHOLD_SHIFT);
            dmareg |= (SPI_DMA_FIFO_THRESHOLD << DMAREG_TX_THRESHOLD_SHIFT);
            *(base + (ECSPI_DMAREG / 4)) = dmareg;
        }
        
        /* 配置中断 */
        if (chCfg->InterruptEnabled) {
            *(base + (ECSPI_INTREG / 4)) = (INTREG_TEEN | INTREG_RREN);
        }
        
        /* 设置波特率 */
        Spi_SetBaudRateInternal(i, chCfg->BaudRate);
        
        /* 初始化状态 */
        Spi_ChannelState[i].JobResult = SPI_JOB_OK;
        Spi_ChannelState[i].ActiveDevice = 0xFF;
        Spi_ChannelState[i].DmaActive = FALSE;
    }
    
    Spi_Initialized = TRUE;
    Spi_Status = SPI_IDLE;
}

/**
 * @brief 内部波特率设置
 * @req SHALL_SPI - 内部波特率设置
 */
static void Spi_SetBaudRateInternal(uint8 Channel, uint32 BaudRate)
{
    volatile uint32* base = Spi_BaseAddr[Channel];
    uint32 refClock = SPI_REF_CLOCK_HZ;
    uint32 preDiv = 0;
    uint32 postDiv = 0;
    uint32 tempDiv;
    
    tempDiv = refClock / BaudRate;
    
    /* 计算预分额和后分额 - 使用标志变量替代goto */
    boolean found = FALSE;
    for (preDiv = 0; preDiv < 16 && !found; preDiv++) {
        for (postDiv = 0; postDiv < 16; postDiv++) {
            if ((1u << preDiv) * (postDiv + 1)) >= tempDiv) {
                found = TRUE;
                break;
            }
        }
    }
    
    uint32 periodreg = (preDiv << 0) | (postDiv << 4);
    *(base + (ECSPI_PERIODREG / 4)) = periodreg;
}

/**
 * @brief SPI反初始化
 * @req SHALL_SPI - SPI反初始化
 */
void Spi_DeInit(void)
{
    uint8 i;
    
    SPI_VALIDATE_INITIALIZED(SPI_SERVICE_ID_DEINIT);
    
    for (i = 0; i < SPI_CHANNEL_COUNT; i++) {
        volatile uint32* base = Spi_BaseAddr[i];
        *(base + (ECSPI_CONREG / 4)) = 0;  /* 禁用SPI */
    }
    
    Spi_ConfigPtr = NULL_PTR;
    Spi_Initialized = FALSE;
    Spi_Status = SPI_UNINIT;
}

/**
 * @brief 同步传输
 * @req SHALL_SPI - 同步传输
 */
Std_ReturnType Spi_SyncTransmit(uint8 DeviceId, const uint8* TxData, uint8* RxData, uint32 Length)
{
    SPI_VALIDATE_INITIALIZED(SPI_SERVICE_ID_SYNCTRANSMIT);
    
    if (DeviceId >= Spi_ConfigPtr->DeviceCount) {
        return E_NOT_OK;
    }
    
    const Spi_ExternalDeviceType* dev = &Spi_ConfigPtr->DeviceConfig[DeviceId];
    uint8 channel = dev->ChannelId;
    volatile uint32* base = Spi_BaseAddr[channel];
    Spi_ChannelStateType* state = &Spi_ChannelState[channel];
    uint32 i;
    uint32 startTime;
    
    /* 检查是否忙 */
    if (Spi_Status == SPI_BUSY) {
        return E_NOT_OK;
    }
    
    Spi_Status = SPI_BUSY;
    state->ActiveDevice = DeviceId;
    state->TxBuffer = TxData;
    state->RxBuffer = RxData;
    state->Length = Length;
    state->Transferred = 0;
    state->JobResult = SPI_JOB_PENDING;
    
    /* 选择从机 */
    uint32 conreg = *(base + (ECSPI_CONREG / 4));
    conreg &= ~(3u << CONREG_CHANNEL_SHIFT);
    conreg |= (dev->ChannelId << CONREG_CHANNEL_SHIFT);
    *(base + (ECSPI_CONREG / 4)) = conreg;
    
    /* 设置波特率 */
    Spi_SetBaudRateInternal(channel, dev->BaudRate);
    
    /* 传输 */
    startTime = Spi_GetCurrentTime();
    
    for (i = 0; i < Length; i++) {
        /* 等待TX FIFO空 */
        while (*(base + (ECSPI_STATREG / 4)) & STATREG_TF) {
            if (Spi_GetElapsedTime(startTime) > SPI_TRANSFER_TIMEOUT_MS) {
                state->JobResult = SPI_JOB_FAILED;
                Spi_Status = SPI_IDLE;
                return E_NOT_OK;
            }
        }
        
        /* 发送 */
        *(base + (ECSPI_TXDATA / 4)) = TxData ? TxData[i] : 0xFF;
        
        /* 等待RX数据 */
        while ((*(base + (ECSPI_STATREG / 4)) & STATREG_RR) == 0) {
            if (Spi_GetElapsedTime(startTime) > SPI_TRANSFER_TIMEOUT_MS) {
                state->JobResult = SPI_JOB_FAILED;
                Spi_Status = SPI_IDLE;
                return E_NOT_OK;
            }
        }
        
        /* 接收 */
        if (RxData) {
            RxData[i] = (uint8)(*(base + (ECSPI_RXDATA / 4)));
        } else {
            (void)*(base + (ECSPI_RXDATA / 4));
        }
        
        state->Transferred++;
    }
    
    state->JobResult = SPI_JOB_OK;
    Spi_Status = SPI_IDLE;
    
    return E_OK;
}

/**
 * @brief 异步传输
 * @req SHALL_SPI - 异步传输
 */
Std_ReturnType Spi_AsyncTransmit(uint8 DeviceId, const uint8* TxData, uint8* RxData, uint32 Length)
{
    SPI_VALIDATE_INITIALIZED(SPI_SERVICE_ID_ASYNC_TRANSMIT);
    
    if (DeviceId >= Spi_ConfigPtr->DeviceCount) {
        return E_NOT_OK;
    }
    
    const Spi_ExternalDeviceType* dev = &Spi_ConfigPtr->DeviceConfig[DeviceId];
    uint8 channel = dev->ChannelId;
    const Spi_ChannelConfigType* chCfg = &Spi_ConfigPtr->ChannelConfig[channel];
    Spi_ChannelStateType* state = &Spi_ChannelState[channel];
    
    if (Spi_Status == SPI_BUSY) {
        return E_NOT_OK;
    }
    
    Spi_Status = SPI_BUSY;
    state->ActiveDevice = DeviceId;
    state->TxBuffer = TxData;
    state->RxBuffer = RxData;
    state->Length = Length;
    state->Transferred = 0;
    state->StartTime = Spi_GetCurrentTime();
    state->JobResult = SPI_JOB_PENDING;
    
    /* 选择从机 */
    uint32 conreg = *(base + (ECSPI_CONREG / 4));
    conreg &= ~(3u << CONREG_CHANNEL_SHIFT);
    conreg |= (dev->ChannelId << CONREG_CHANNEL_SHIFT);
    *(base + (ECSPI_CONREG / 4)) = conreg;
    
    /* 设置波特率 */
    Spi_SetBaudRateInternal(channel, dev->BaudRate);
    
    if (chCfg->DmaEnabled && Length > SPI_DMA_FIFO_THRESHOLD) {
        /* DMA传输 */
        state->DmaActive = TRUE;
        
        /* 配置TX DMA */
        if (TxData) {
            Dma_ConfigTx(chCfg->DmaTxChannel, (uint32)TxData, 
                        (uint32)(base + (ECSPI_TXDATA / 4)), Length);
        }
        
        /* 配置RX DMA */
        if (RxData) {
            Dma_ConfigRx(chCfg->DmaRxChannel, 
                        (uint32)(base + (ECSPI_RXDATA / 4)), (uint32)RxData, Length);
        }
        
        /* 启动DMA */
        Dma_EnableChannel(chCfg->DmaTxChannel);
        Dma_EnableChannel(chCfg->DmaRxChannel);
        
        /* 启动传输 */
        *(base + (ECSPI_CONREG / 4)) |= CONREG_XCH;
    } else {
        /* 中断传输 */
        state->DmaActive = FALSE;
        
        /* 填充TX FIFO */
        uint32 fifoFill = (Length < SPI_FIFO_DEPTH) ? Length : SPI_FIFO_DEPTH;
        for (uint32 i = 0; i < fifoFill; i++) {
            *(base + (ECSPI_TXDATA / 4)) = TxData ? TxData[i] : 0xFF;
        }
        
        /* 使能中断 */
        *(base + (ECSPI_INTREG / 4)) |= (INTREG_TCEN | INTREG_RREN);
        
        /* 启动传输 */
        *(base + (ECSPI_CONREG / 4)) |= CONREG_XCH;
    }
    
    return E_OK;
}

/**
 * @brief 获取状态
 * @req SHALL_SPI - 获取状态
 */
Spi_StatusType Spi_GetStatus(void)
{
    return Spi_Status;
}

/**
 * @brief 获取任务结果
 * @req SHALL_SPI - 获取任务结果
 */
Spi_JobResultType Spi_GetJobResult(void)
{
    if (Spi_Initialized == FALSE) {
        return SPI_JOB_FAILED;
    }
    
    for (uint8 i = 0; i < SPI_CHANNEL_COUNT; i++) {
        if (Spi_ChannelState[i].JobResult == SPI_JOB_PENDING) {
            return SPI_JOB_PENDING;
        }
    }
    
    return SPI_JOB_OK;
}

/**
 * @brief 中断处理
 * @req SHALL_SPI - 中断处理
 */
void Spi_IsrHandler(uint8 Channel)
{
    if (Channel >= SPI_CHANNEL_COUNT || Spi_Initialized == FALSE) {
        return;
    }
    
    volatile uint32* base = Spi_BaseAddr[Channel];
    Spi_ChannelStateType* state = &Spi_ChannelState[Channel];
    uint32 stat = *(base + (ECSPI_STATREG / 4));
    
    /* RX中断 */
    if (stat & STATREG_RR) {
        while ((*(base + (ECSPI_STATREG / 4)) & STATREG_RR) && 
               state->Transferred < state->Length) {
            if (state->RxBuffer) {
                state->RxBuffer[state->Transferred] = 
                    (uint8)(*(base + (ECSPI_RXDATA / 4)));
            } else {
                (void)*(base + (ECSPI_RXDATA / 4));
            }
            state->Transferred++;
        }
    }
    
    /* TX中断 - 继续填充FIFO */
    if ((stat & STATREG_TE) && state->Transferred < state->Length) {
        while ((*(base + (ECSPI_STATREG / 4)) & STATREG_TE) == 0 && 
               (state->Transferred + (state->Length - state->TxSent)) < SPI_FIFO_DEPTH) {
            *(base + (ECSPI_TXDATA / 4)) = 
                state->TxBuffer ? state->TxBuffer[state->TxSent] : 0xFF;
            state->TxSent++;
        }
    }
    
    /* 传输完成 */
    if (state->Transferred >= state->Length) {
        state->JobResult = SPI_JOB_OK;
        Spi_Status = SPI_IDLE;
        *(base + (ECSPI_INTREG / 4)) = 0;  /* 禁用所有中断 */
    }
}

/**
 * @brief 主函数
 * @req SHALL_SPI - 主函数
 */
void Spi_MainFunction(void)
{
    if (Spi_Initialized == FALSE || Spi_Status != SPI_BUSY) {
        return;
    }
    
    for (uint8 i = 0; i < SPI_CHANNEL_COUNT; i++) {
        Spi_ChannelStateType* state = &Spi_ChannelState[i];
        
        if (state->JobResult == SPI_JOB_PENDING) {
            /* 检查超时 */
            if (Spi_GetElapsedTime(state->StartTime) > SPI_TRANSFER_TIMEOUT_MS) {
                state->JobResult = SPI_JOB_FAILED;
                Spi_Status = SPI_IDLE;
                
                /* 禁用DMA */
                if (state->DmaActive) {
                    volatile uint32* base = Spi_BaseAddr[i];
                    *(base + (ECSPI_DMAREG / 4)) = 0;
                    Dma_DisableChannel(Spi_ConfigPtr->ChannelConfig[i].DmaTxChannel);
                    Dma_DisableChannel(Spi_ConfigPtr->ChannelConfig[i].DmaRxChannel);
                }
            }
        }
    }
}

/* 辅助函数 */
static uint32 Spi_GetCurrentTime(void)
{
    return Gpt_GetTimeElapsed(0);
}

static uint32 Spi_GetElapsedTime(uint32 StartTime)
{
    uint32 current = Spi_GetCurrentTime();
    return (current >= StartTime) ? (current - StartTime) : 
           ((0xFFFFFFFF - StartTime) + current);
}

#if (SPI_VERSION_INFO_API == STD_ON)
void Spi_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (SPI_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) {
        Det_ReportError(SPI_MODULE_ID, SPI_INSTANCE_ID, 0x02U, SPI_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID = SPI_VENDOR_ID;
    versioninfo->moduleID = SPI_MODULE_ID;
    versioninfo->sw_major_version = SPI_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SPI_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SPI_SW_PATCH_VERSION;
}
#endif
