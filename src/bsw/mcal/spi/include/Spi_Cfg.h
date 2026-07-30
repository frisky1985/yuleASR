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
 * @file Spi_Cfg.h
 * @brief SPI驱动配置头文件
 * 
 * 硬件: i.MX8M Mini ECSPI
 * 支持DMA、中断、多从机
 */

#ifndef SPI_CFG_H
#define SPI_CFG_H

#include "Std_Types.h"

/* 版本 */
#define SPI_VENDOR_ID_CFG           0x01u
#define SPI_MODULE_ID_CFG           0x7Au  /* AUTOSAR standard: 122 */
#define SPI_SW_MAJOR_VERSION_CFG    1u
#define SPI_SW_MINOR_VERSION_CFG    0u
#define SPI_SW_PATCH_VERSION_CFG    0u

/* 基础类型定义 */
/** @brief SPI 通道标识类型 */
typedef uint8 Spi_ChannelType;

/** @brief SPI 序列标识类型 */
typedef uint8 Spi_SequenceType;

/* 开关配置 */
#define SPI_DEV_ERROR_DETECT        STD_ON
#define SPI_VERSION_INFO_API        STD_ON
#define SPI_DMA_SUPPORT             STD_ON
#define SPI_INTERRUPT_SUPPORT       STD_ON
#define SPI_MULTI_SLAVE_SUPPORT     STD_ON
#define SPI_FIFO_SUPPORT            STD_ON

/* 通道配置 */
#define SPI_CHANNEL_COUNT           4u
#define SPI_MAX_JOB                 16u
#define SPI_MAX_SEQUENCE            8u
#define SPI_MAX_EXTERNAL_DEV        16u

/* 时钟配置 */
#define SPI_REF_CLOCK_HZ            80000000u   /* 80MHz */

/* DMA配置 */
#if (SPI_DMA_SUPPORT == STD_ON)
#define SPI_DMA_TX_CHANNEL_BASE     8u
#define SPI_DMA_RX_CHANNEL_BASE     12u
#define SPI_DMA_FIFO_THRESHOLD      4u
#endif

/* 中断配置 */
#define SPI_IRQ_PRIORITY            4u

/* FIFO配置 */
#if (SPI_FIFO_SUPPORT == STD_ON)
#define SPI_FIFO_DEPTH              64u
#define SPI_TX_FIFO_THRESHOLD       32u
#define SPI_RX_FIFO_THRESHOLD       32u
#endif

/* 超时配置 */
#define SPI_TRANSFER_TIMEOUT_MS     1000u

/* 服务ID */
#define SPI_SERVICE_ID_INIT                     0x00u
#define SPI_SERVICE_ID_DEINIT                   0x01u
#define SPI_SERVICE_ID_WRITEB_IB                0x02u
#define SPI_SERVICE_ID_ASYNC_TRANSMIT           0x03u
#define SPI_SERVICE_ID_READ_IB                  0x04u
#define SPI_SERVICE_ID_SETUP_EB                 0x05u
#define SPI_SERVICE_ID_GETSTATUS                0x06u
#define SPI_SERVICE_ID_GETJOBRESULT             0x07u
#define SPI_SERVICE_ID_GETSEQUENCERESULT        0x08u
#define SPI_SERVICE_ID_GETVERSIONINFO           0x09u
#define SPI_SERVICE_ID_SYNCTRANSMIT             0x0Au
#define SPI_SERVICE_ID_GETHWUNITSTATUS          0x0Bu
#define SPI_SERVICE_ID_CANCEL                   0x0Cu
#define SPI_SERVICE_ID_SET_CLOCK                0x0Du

#endif /* SPI_CFG_H */
