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
 * @file Spi.h
 * @brief SPI驱动标准AUTOSAR接口
 * 
 * 支持主/从模式、DMA、中断、多从机
 */

#ifndef SPI_H
#define SPI_H

#include "Spi_Cfg.h"
#include "Std_Types.h"

/* 版本 */
#define SPI_VENDOR_ID               0x01u
#define SPI_MODULE_ID               0x21u
#define SPI_SW_MAJOR_VERSION        1u
#define SPI_SW_MINOR_VERSION        0u
#define SPI_SW_PATCH_VERSION        0u

/* 错误码 */
#define SPI_E_PARAM_CHANNEL         0x0Au
#define SPI_E_PARAM_JOB             0x0Bu
#define SPI_E_PARAM_SEQ             0x0Cu
#define SPI_E_PARAM_POINTER         0x10u
#define SPI_E_PARAM_LENGTH          0x11u
#define SPI_E_PARAM_UNIT            0x12u
#define SPI_E_UNINIT                0x1Au
#define SPI_E_ALREADY_INITIALIZED   0x1Bu
#define SPI_E_SEQ_PENDING           0x20u
#define SPI_E_SEQ_INPROCESS         0x21u
#define SPI_E_JOB_PENDING           0x22u

/* 状态 */
typedef enum {
    SPI_UNINIT = 0,
    SPI_IDLE,
    SPI_BUSY
} Spi_StatusType;

typedef enum {
    SPI_JOB_OK = 0,
    SPI_JOB_PENDING,
    SPI_JOB_FAILED,
    SPI_JOB_QUEUED
} Spi_JobResultType;

typedef enum {
    SPI_SEQ_OK = 0,
    SPI_SEQ_PENDING,
    SPI_SEQ_FAILED,
    SPI_SEQ_CANCELED
} Spi_SeqResultType;

/* 数据模式 */
typedef enum {
    SPI_DATA_MODE_8BIT = 0,
    SPI_DATA_MODE_16BIT,
    SPI_DATA_MODE_32BIT
} Spi_DataModeType;

/* 时序模式 */
typedef enum {
    SPI_CLOCK_MODE_0 = 0,   /* CPOL=0, CPHA=0 */
    SPI_CLOCK_MODE_1,       /* CPOL=0, CPHA=1 */
    SPI_CLOCK_MODE_2,       /* CPOL=1, CPHA=0 */
    SPI_CLOCK_MODE_3        /* CPOL=1, CPHA=1 */
} Spi_ClockModeType;

/* 传输类型 */
typedef enum {
    SPI_FULL_DUPLEX = 0,
    SPI_HALF_DUPLEX_TX,
    SPI_HALF_DUPLEX_RX
} Spi_TransferType;

/* 传输结果 */
typedef enum {
    SPI_TRANSFER_OK = 0,
    SPI_TRANSFER_PENDING,
    SPI_TRANSFER_TIMEOUT,
    SPI_TRANSFER_ERROR
} Spi_TransferResultType;

/* 通道配置 */
typedef struct {
    uint8                   ChannelId;
    Spi_DataModeType        DataMode;
    Spi_ClockModeType       ClockMode;
    uint32                  BaudRate;
    boolean                 LsbFirst;
    boolean                 DmaEnabled;
    uint8                   DmaTxChannel;
    uint8                   DmaRxChannel;
    boolean                 InterruptEnabled;
    uint8                   InterruptPriority;
} Spi_ChannelConfigType;

/* 外部设备配置 */
typedef struct {
    uint8                   DeviceId;
    uint8                   ChannelId;
    uint32                  ChipSelectPin;
    boolean                 ChipSelectActiveLow;
    uint32                  ChipSelectDelay;
    uint32                  BaudRate;
} Spi_ExternalDeviceType;

/* 缓冲区描述符 */
typedef struct {
    uint8*                  Buffer;
    uint32                  Length;
} Spi_BufferType;

/* 全局配置 */
typedef struct {
    uint8                   ChannelCount;
    const Spi_ChannelConfigType* ChannelConfig;
    const Spi_ExternalDeviceType* DeviceConfig;
    uint8                   DeviceCount;
} Spi_ConfigType;

/* API函数 */
extern void Spi_Init(const Spi_ConfigType* Config);
extern void Spi_DeInit(void);
extern Std_ReturnType Spi_SyncTransmit(uint8 DeviceId, const uint8* TxData, uint8* RxData, uint32 Length);
extern Std_ReturnType Spi_AsyncTransmit(uint8 DeviceId, const uint8* TxData, uint8* RxData, uint32 Length);
extern Spi_StatusType Spi_GetStatus(void);
extern Spi_JobResultType Spi_GetJobResult(void);
extern void Spi_MainFunction(void);
extern void Spi_IsrHandler(uint8 Channel);

#endif /* SPI_H */
