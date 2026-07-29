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
 * @file Uart_Cfg.h
 * @brief UART驱动配置头文件
 * 
 * 硬件: i.MX8M Mini
 * 驱动类型: AUTOSAR MCAL 4.x
 * 传输方式: 轮询/中断/DMA
 */

#ifndef UART_CFG_H
#define UART_CFG_H

#include "Std_Types.h"

/*============================================================================
 * 版本信息
 *===========================================================================*/
#define UART_VENDOR_ID_CFG          0x01u
#define UART_MODULE_ID_CFG          0x11u
#define UART_SW_MAJOR_VERSION_CFG   1u
#define UART_SW_MINOR_VERSION_CFG   0u
#define UART_SW_PATCH_VERSION_CFG   0u

/*============================================================================
 * 预编译配置
 *===========================================================================*/
/* 开发错误检测 */
#define UART_DEV_ERROR_DETECT       STD_ON

/* 版本信息API */
#define UART_VERSION_INFO_API       STD_ON

/* DMA支持 */
#define UART_DMA_SUPPORT            STD_ON

/* FIFO支持 */
#define UART_FIFO_SUPPORT           STD_ON

/* 流控制支持 (RTS/CTS) */
#define UART_HW_HANDSHAKE_SUPPORT   STD_ON

/* 多处理器支持 */
#define UART_MULTI_PROCESSOR_MODE   STD_OFF

/*============================================================================
 * 通道配置
 *===========================================================================*/
/* UART通道数量 */
#define UART_CHANNEL_COUNT          4u

/* 通道ID定义 */
#define UART_CHANNEL_0              0u  /* UART1 */
#define UART_CHANNEL_1              1u  /* UART2 */
#define UART_CHANNEL_2              2u  /* UART3 */
#define UART_CHANNEL_3              3u  /* UART4 */

/*============================================================================
 * 时钟配置
 *===========================================================================*/
/* 参考时钟频率 (80MHz) */
#define UART_REF_CLOCK_HZ           80000000u

/*============================================================================
 * DMA配置
 *===========================================================================*/
#if (UART_DMA_SUPPORT == STD_ON)
/* DMA通道映射 */
#define UART0_DMA_TX_CHANNEL        0u
#define UART0_DMA_RX_CHANNEL        1u
#define UART1_DMA_TX_CHANNEL        2u
#define UART1_DMA_RX_CHANNEL        3u
#define UART2_DMA_TX_CHANNEL        4u
#define UART2_DMA_RX_CHANNEL        5u
#define UART3_DMA_TX_CHANNEL        6u
#define UART3_DMA_RX_CHANNEL        7u

/* DMA FIFO阈值 */
#define UART_DMA_FIFO_THRESHOLD     4u
#endif

/*============================================================================
 * 中断配置
 *===========================================================================*/
/* 中断优先级 (0-15, 0最高) */
#define UART_IRQ_PRIORITY_LEVEL     5u

/* 中断类型 */
#define UART_TX_IRQ_TYPE            0u
#define UART_RX_IRQ_TYPE            1u
#define UART_ERROR_IRQ_TYPE         2u

/*============================================================================
 * FIFO配置
 *===========================================================================*/
#if (UART_FIFO_SUPPORT == STD_ON)
/* TX FIFO阈值 */
#define UART_TX_FIFO_THRESHOLD      8u   /* 小于8字节触发 */
#define UART_RX_FIFO_THRESHOLD      8u   /* 大于8字节触发 */

/* FIFO深度 */
#define UART_FIFO_DEPTH             32u
#endif

/*============================================================================
 * 超时配置
 *===========================================================================*/
/* 传输超时 (毫秒) */
#define UART_TX_TIMEOUT_MS          1000u
#define UART_RX_TIMEOUT_MS          1000u

/*============================================================================
 * 接口ID
 *===========================================================================*/
#define UART_SERVICE_ID_INIT                    0x00u
#define UART_SERVICE_ID_DEINIT                  0x01u
#define UART_SERVICE_ID_SEND                    0x02u
#define UART_SERVICE_ID_RECEIVE                 0x03u
#define UART_SERVICE_ID_GETSTATUS               0x04u
#define UART_SERVICE_ID_GETVERSIONINFO          0x05u
#define UART_SERVICE_ID_SENDDMA                 0x06u
#define UART_SERVICE_ID_RECEIVEDMA              0x07u
#define UART_SERVICE_ID_ABORT                   0x08u
#define UART_SERVICE_ID_ENABLEINTERRUPT         0x09u
#define UART_SERVICE_ID_DISABLEINTERRUPT        0x0Au
#define UART_SERVICE_ID_ENABLEDMA               0x0Bu
#define UART_SERVICE_ID_DISABLEDMA              0x0Cu
#define UART_SERVICE_ID_CLEARFIFO               0x0Du
#define UART_SERVICE_ID_SETBAUDRATE             0x0Eu

#endif /* UART_CFG_H */
