/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file TcpIp_Cfg.h
 * @brief TCP/IP Stack Pre-Compile Configuration
 * @version 1.0.0
 */

#ifndef TCPIP_CFG_H
#define TCPIP_CFG_H

/*==================================================================================================
 *                                    DEVELOPMENT ERROR DETECT
 *==================================================================================================*/
#define TCPIP_DEV_ERROR_DETECT                  (STD_ON)
#define TCPIP_VERSION_INFO_API                  (STD_ON)

/*==================================================================================================
 *                                    SOCKET CONFIGURATION
 *==================================================================================================*/
#define TCPIP_MAX_SOCKETS                       (8U)
#define TCPIP_MAX_TCP_PBUFS                     (16U)
#define TCPIP_MAX_UDP_PBUFS                     (16U)
#define TCPIP_TCP_RCV_BUF_SIZE                  (4096U)
#define TCPIP_TCP_SND_BUF_SIZE                  (4096U)
#define TCPIP_UDP_RCV_BUF_SIZE                  (2048U)

/*==================================================================================================
 *                                    IP CONFIGURATION
 *==================================================================================================*/
#define TCPIP_DEFAULT_IPV4_ADDR                 ((uint32)0xC0A80002UL)  /* 192.168.0.2 */
#define TCPIP_DEFAULT_IPV4_MASK                 ((uint32)0xFFFFFF00UL)  /* 255.255.255.0 */
#define TCPIP_DEFAULT_IPV4_GW                   ((uint32)0xC0A80001UL)  /* 192.168.0.1 */

/* IPv6 link-local prefix (fe80::) */
#define TCPIP_IPV6_LINKLOCAL                    { { 0xFE800000UL, 0x00000000UL, 0x00000000UL, 0x00000001UL } }

/*==================================================================================================
 *                                    TIMING CONFIGURATION
 *==================================================================================================*/
#define TCPIP_ETH_LINK_CHECK_INTERVAL_MS        (100U)
#define TCPIP_MAIN_FUNCTION_PERIOD_MS           (10U)
#define TCPIP_ARP_TIMEOUT_MS                    (300000U)   /* 5 min */

/*==================================================================================================
 *                                    FEATURE ENABLES
 *==================================================================================================*/
#define TCPIP_ENABLE_IPV4                       (STD_ON)
#define TCPIP_ENABLE_IPV6                       (STD_OFF)
#define TCPIP_ENABLE_TCP                        (STD_ON)
#define TCPIP_ENABLE_UDP                        (STD_ON)
#define TCPIP_ENABLE_ARP                        (STD_ON)
#define TCPIP_ENABLE_DHCP                       (STD_OFF)
#define TCPIP_ENABLE_DNS                        (STD_OFF)

/*==================================================================================================
 *                                    BUFFER SIZES
 *==================================================================================================*/
#define TCPIP_PBUF_POOL_SIZE                    (32U)
#define TCPIP_PBUF_POOL_BUF_SIZE                (1518U)
#define TCPIP_MEMP_NUM_TCP_PCB                  (8U)
#define TCPIP_MEMP_NUM_TCP_SEG                  (32U)
#define TCPIP_MEMP_NUM_UDP_PCB                  (8U)

#endif /* TCPIP_CFG_H */
