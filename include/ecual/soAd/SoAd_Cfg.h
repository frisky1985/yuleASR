/******************************************************************************
 * @file    SoAd_Cfg.h
 * @brief   Socket Adapter (SoAd) Configuration - AUTOSAR R22-11
 *
 * Configuration parameters for the SoAd module.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x36 (SoAd)
 * ASIL-B Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef SOAD_CFG_H
#define SOAD_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Module Version Information (from configuration)
 ******************************************************************************/
#define SOAD_CFG_VENDOR_ID              0x01U
#define SOAD_CFG_MODULE_ID              0x36U
#define SOAD_CFG_SW_MAJOR_VERSION       1U
#define SOAD_CFG_SW_MINOR_VERSION       0U
#define SOAD_CFG_SW_PATCH_VERSION       0U

/******************************************************************************
 * General Configuration
 ******************************************************************************/

/* Development error detection */
#define SOAD_DEV_ERROR_DETECT           STD_ON

/* Version info API */
#define SOAD_VERSION_INFO_API           STD_ON

/* Main function period in milliseconds */
#define SOAD_MAIN_FUNCTION_PERIOD_MS    10U

/* Enable extended diagnostic information */
#define SOAD_EXTENDED_DIAGNOSTICS       STD_ON

/* Enable statistics collection */
#define SOAD_STATISTICS_ENABLED         STD_ON

/******************************************************************************
 * Socket Connection Configuration
 ******************************************************************************/

/* Maximum number of socket connections */
#define SOAD_CFG_MAX_SOCKET_CONNECTIONS 8U

/* Maximum number of PDU routes */
#define SOAD_CFG_MAX_PDU_ROUTES         16U

/* Maximum number of routing groups */
#define SOAD_CFG_MAX_ROUTING_GROUPS     4U

/* Maximum PDUs per socket */
#define SOAD_CFG_MAX_PDUS_PER_SOCKET    8U

/* Maximum PDU size */
#define SOAD_CFG_MAX_PDU_SIZE           4096U

/* Socket RX buffer size */
#define SOAD_CFG_SOCKET_RX_BUFFER_SIZE  8192U

/* Socket TX buffer size */
#define SOAD_CFG_SOCKET_TX_BUFFER_SIZE  8192U

/* PDU header size (for header options) */
#define SOAD_CFG_PDU_HEADER_SIZE        8U

/******************************************************************************
 * Socket Connection 0 - DDS TCP Server (Port 7410)
 ******************************************************************************/
#define SOAD_SOCON_0_ENABLED            STD_ON
#define SOAD_SOCON_0_ID                 0U
#define SOAD_SOCON_0_TYPE               SOAD_SOCKET_TYPE_STREAM
#define SOAD_SOCON_0_PROTOCOL           SOAD_SOCKET_PROTOCOL_TCP
#define SOAD_SOCON_0_MODE               SOAD_SOCON_MODE_TCPIP
#define SOAD_SOCON_0_LOCAL_PORT         7410U
#define SOAD_SOCON_0_LOCAL_IP           0x00000000U  /* Any IP */
#define SOAD_SOCON_0_REMOTE_PORT        0U
#define SOAD_SOCON_0_REMOTE_IP          0x00000000U
#define SOAD_SOCON_0_AUTO_CONNECT       STD_OFF      /* Server waits for accept */
#define SOAD_SOCON_0_IS_SERVER          STD_ON
#define SOAD_SOCON_0_KEEP_ALIVE         STD_ON
#define SOAD_SOCON_0_KEEP_ALIVE_TIME    60U
#define SOAD_SOCON_0_NAGLE_ENABLE       STD_OFF      /* Disable for low latency DDS */
#define SOAD_SOCON_0_RX_BUFFER_SIZE     8192U
#define SOAD_SOCON_0_TX_BUFFER_SIZE     8192U
#define SOAD_SOCON_0_CONN_TIMEOUT_MS    5000U
#define SOAD_SOCON_0_TX_TIMEOUT_MS      1000U
#define SOAD_SOCON_0_ROUTING_GROUP      0U

/******************************************************************************
 * Socket Connection 1 - DDS TCP Client
 ******************************************************************************/
#define SOAD_SOCON_1_ENABLED            STD_ON
#define SOAD_SOCON_1_ID                 1U
#define SOAD_SOCON_1_TYPE               SOAD_SOCKET_TYPE_STREAM
#define SOAD_SOCON_1_PROTOCOL           SOAD_SOCKET_PROTOCOL_TCP
#define SOAD_SOCON_1_MODE               SOAD_SOCON_MODE_TCPIP
#define SOAD_SOCON_1_LOCAL_PORT         0U           /* Auto-assign */
#define SOAD_SOCON_1_LOCAL_IP           0x00000000U
#define SOAD_SOCON_1_REMOTE_PORT        7410U
#define SOAD_SOCON_1_REMOTE_IP          0xC0A80101U  /* 192.168.1.1 */
#define SOAD_SOCON_1_AUTO_CONNECT       STD_ON
#define SOAD_SOCON_1_IS_SERVER          STD_OFF
#define SOAD_SOCON_1_KEEP_ALIVE         STD_ON
#define SOAD_SOCON_1_KEEP_ALIVE_TIME    60U
#define SOAD_SOCON_1_NAGLE_ENABLE       STD_OFF
#define SOAD_SOCON_1_RX_BUFFER_SIZE     8192U
#define SOAD_SOCON_1_TX_BUFFER_SIZE     8192U
#define SOAD_SOCON_1_CONN_TIMEOUT_MS    5000U
#define SOAD_SOCON_1_TX_TIMEOUT_MS      1000U
#define SOAD_SOCON_1_ROUTING_GROUP      0U

/******************************************************************************
 * Socket Connection 2 - DDS UDP Discovery (Port 7400)
 ******************************************************************************/
#define SOAD_SOCON_2_ENABLED            STD_ON
#define SOAD_SOCON_2_ID                 2U
#define SOAD_SOCON_2_TYPE               SOAD_SOCKET_TYPE_DGRAM
#define SOAD_SOCON_2_PROTOCOL           SOAD_SOCKET_PROTOCOL_UDP
#define SOAD_SOCON_2_MODE               SOAD_SOCON_MODE_UDP_EXT
#define SOAD_SOCON_2_LOCAL_PORT         7400U
#define SOAD_SOCON_2_LOCAL_IP           0x00000000U
#define SOAD_SOCON_2_REMOTE_PORT        7400U
#define SOAD_SOCON_2_REMOTE_IP          0xEFFFFFF5U  /* 239.255.255.245 Multicast */
#define SOAD_SOCON_2_AUTO_CONNECT       STD_ON
#define SOAD_SOCON_2_IS_SERVER          STD_OFF
#define SOAD_SOCON_2_KEEP_ALIVE         STD_OFF
#define SOAD_SOCON_2_KEEP_ALIVE_TIME    0U
#define SOAD_SOCON_2_NAGLE_ENABLE       STD_OFF
#define SOAD_SOCON_2_RX_BUFFER_SIZE     4096U
#define SOAD_SOCON_2_TX_BUFFER_SIZE     4096U
#define SOAD_SOCON_2_CONN_TIMEOUT_MS    1000U
#define SOAD_SOCON_2_TX_TIMEOUT_MS      1000U
#define SOAD_SOCON_2_ROUTING_GROUP      1U

/******************************************************************************
 * Socket Connection 3 - DDS UDP Data (Port 7411)
 ******************************************************************************/
#define SOAD_SOCON_3_ENABLED            STD_ON
#define SOAD_SOCON_3_ID                 3U
#define SOAD_SOCON_3_TYPE               SOAD_SOCKET_TYPE_DGRAM
#define SOAD_SOCON_3_PROTOCOL           SOAD_SOCKET_PROTOCOL_UDP
#define SOAD_SOCON_3_MODE               SOAD_SOCON_MODE_UDP_EXT
#define SOAD_SOCON_3_LOCAL_PORT         7411U
#define SOAD_SOCON_3_LOCAL_IP           0x00000000U
#define SOAD_SOCON_3_REMOTE_PORT        7411U
#define SOAD_SOCON_3_REMOTE_IP          0x00000000U
#define SOAD_SOCON_3_AUTO_CONNECT       STD_ON
#define SOAD_SOCON_3_IS_SERVER          STD_OFF
#define SOAD_SOCON_3_KEEP_ALIVE         STD_OFF
#define SOAD_SOCON_3_NAGLE_ENABLE       STD_OFF
#define SOAD_SOCON_3_RX_BUFFER_SIZE     8192U
#define SOAD_SOCON_3_TX_BUFFER_SIZE     8192U
#define SOAD_SOCON_3_CONN_TIMEOUT_MS    1000U
#define SOAD_SOCON_3_TX_TIMEOUT_MS      1000U
#define SOAD_SOCON_3_ROUTING_GROUP      1U

/******************************************************************************
 * PDU Route Configuration
 ******************************************************************************/

/* PDU Route 0 - DDS Discovery (UDP Multicast) */
#define SOAD_PDU_ROUTE_0_ENABLED        STD_ON
#define SOAD_PDU_ROUTE_0_ID             0U
#define SOAD_PDU_ROUTE_0_SOCON_ID       2U
#define SOAD_PDU_ROUTE_0_TX_PDU_ID      0U
#define SOAD_PDU_ROUTE_0_RX_PDU_ID      100U
#define SOAD_PDU_ROUTE_0_DIRECTION      SOAD_PDU_DIR_TX_RX
#define SOAD_PDU_ROUTE_0_UPPER_LAYER    SOAD_UPPER_DDS
#define SOAD_PDU_ROUTE_0_HEADER_ENABLE  STD_OFF
#define SOAD_PDU_ROUTE_0_HEADER_SIZE    0U
#define SOAD_PDU_ROUTE_0_PDU_SIZE       2048U
#define SOAD_PDU_ROUTE_0_USE_TP         STD_OFF
#define SOAD_PDU_ROUTE_0_ROUTING_GROUP  1U

/* PDU Route 1 - DDS Data (UDP) */
#define SOAD_PDU_ROUTE_1_ENABLED        STD_ON
#define SOAD_PDU_ROUTE_1_ID             1U
#define SOAD_PDU_ROUTE_1_SOCON_ID       3U
#define SOAD_PDU_ROUTE_1_TX_PDU_ID      1U
#define SOAD_PDU_ROUTE_1_RX_PDU_ID      101U
#define SOAD_PDU_ROUTE_1_DIRECTION      SOAD_PDU_DIR_TX_RX
#define SOAD_PDU_ROUTE_1_UPPER_LAYER    SOAD_UPPER_DDS
#define SOAD_PDU_ROUTE_1_HEADER_ENABLE  STD_OFF
#define SOAD_PDU_ROUTE_1_HEADER_SIZE    0U
#define SOAD_PDU_ROUTE_1_PDU_SIZE       4096U
#define SOAD_PDU_ROUTE_1_USE_TP         STD_OFF
#define SOAD_PDU_ROUTE_1_ROUTING_GROUP  1U

/* PDU Route 2 - DDS Control (TCP Server) */
#define SOAD_PDU_ROUTE_2_ENABLED        STD_ON
#define SOAD_PDU_ROUTE_2_ID             2U
#define SOAD_PDU_ROUTE_2_SOCON_ID       0U
#define SOAD_PDU_ROUTE_2_TX_PDU_ID      2U
#define SOAD_PDU_ROUTE_2_RX_PDU_ID      102U
#define SOAD_PDU_ROUTE_2_DIRECTION      SOAD_PDU_DIR_TX_RX
#define SOAD_PDU_ROUTE_2_UPPER_LAYER    SOAD_UPPER_DDS
#define SOAD_PDU_ROUTE_2_HEADER_ENABLE  STD_ON
#define SOAD_PDU_ROUTE_2_HEADER_SIZE    4U
#define SOAD_PDU_ROUTE_2_PDU_SIZE       4096U
#define SOAD_PDU_ROUTE_2_USE_TP         STD_ON
#define SOAD_PDU_ROUTE_2_ROUTING_GROUP  0U

/* PDU Route 3 - DDS Control (TCP Client) */
#define SOAD_PDU_ROUTE_3_ENABLED        STD_ON
#define SOAD_PDU_ROUTE_3_ID             3U
#define SOAD_PDU_ROUTE_3_SOCON_ID       1U
#define SOAD_PDU_ROUTE_3_TX_PDU_ID      3U
#define SOAD_PDU_ROUTE_3_RX_PDU_ID      103U
#define SOAD_PDU_ROUTE_3_DIRECTION      SOAD_PDU_DIR_TX_RX
#define SOAD_PDU_ROUTE_3_UPPER_LAYER    SOAD_UPPER_DDS
#define SOAD_PDU_ROUTE_3_HEADER_ENABLE  STD_ON
#define SOAD_PDU_ROUTE_3_HEADER_SIZE    4U
#define SOAD_PDU_ROUTE_3_PDU_SIZE       4096U
#define SOAD_PDU_ROUTE_3_USE_TP         STD_ON
#define SOAD_PDU_ROUTE_3_ROUTING_GROUP  0U

/******************************************************************************
 * Routing Group Configuration
 ******************************************************************************/

/* Routing Group 0 - DDS TCP Control Channel */
#define SOAD_ROUTING_GROUP_0_ENABLED    STD_ON
#define SOAD_ROUTING_GROUP_0_ID         0U
#define SOAD_ROUTING_GROUP_0_DEFAULT    STD_ON

/* Routing Group 1 - DDS UDP Data Channel */
#define SOAD_ROUTING_GROUP_1_ENABLED    STD_ON
#define SOAD_ROUTING_GROUP_1_ID         1U
#define SOAD_ROUTING_GROUP_1_DEFAULT    STD_ON

/******************************************************************************
 * Timeout Configuration
 ******************************************************************************/

/* Connection establishment timeout */
#define SOAD_CFG_CONN_TIMEOUT_MS        5000U

/* Transmission timeout */
#define SOAD_CFG_TX_TIMEOUT_MS          1000U

/* Socket close timeout */
#define SOAD_CFG_CLOSE_TIMEOUT_MS       2000U

/* Keep-alive timeout */
#define SOAD_CFG_KEEPALIVE_TIMEOUT_MS   60000U

/* Retry attempts for connection */
#define SOAD_CFG_MAX_CONN_RETRIES       3U

/******************************************************************************
 * DEM Event IDs
 ******************************************************************************/

/* DEM Event IDs - to be configured with actual Dem IDs */
#define SOAD_DEM_EVENT_INIT_FAILED      0x3601U
#define SOAD_DEM_EVENT_SOCKET_ERROR     0x3602U
#define SOAD_DEM_EVENT_TX_FAILED        0x3603U
#define SOAD_DEM_EVENT_RX_FAILED        0x3604U
#define SOAD_DEM_EVENT_ROUTING_ERROR    0x3605U
#define SOAD_DEM_EVENT_CONN_TIMEOUT     0x3606U

/******************************************************************************
 * Callback Configuration
 ******************************************************************************/

/* Enable IF Tx Confirmation callback to PduR */
#define SOAD_UL_IF_TX_CONFIRMATION      STD_ON

/* Enable IF Rx Indication callback to PduR */
#define SOAD_UL_IF_RX_INDICATION        STD_ON

/* Enable TP Tx Confirmation callback to PduR */
#define SOAD_UL_TP_TX_CONFIRMATION      STD_ON

/* Enable TP Rx Indication callback to PduR */
#define SOAD_UL_TP_RX_INDICATION        STD_ON

/* Enable Socket Connection Mode Change callback */
#define SOAD_UL_SO_CON_MODE_CHG         STD_ON

/******************************************************************************
 * Feature Switches
 ******************************************************************************/

/* Enable TCP support */
#define SOAD_TCP_SUPPORT                STD_ON

/* Enable UDP support */
#define SOAD_UDP_SUPPORT                STD_ON

/* Enable TCP server (listen/accept) */
#define SOAD_TCP_SERVER_SUPPORT         STD_ON

/* Enable PDU header option */
#define SOAD_PDU_HEADER_ENABLE          STD_ON

/* Enable routing groups */
#define SOAD_ROUTING_GROUPS_ENABLE      STD_ON

/* Enable automatic connection establishment */
#define SOAD_AUTO_CONNECT_ENABLE        STD_ON

/* Enable connection monitoring (keep-alive) */
#define SOAD_CONN_MONITORING_ENABLE     STD_ON

/******************************************************************************
 * DDS Integration Configuration
 ******************************************************************************/

/* DDS RTPS default port base */
#define SOAD_DDS_PORT_BASE              7400U

/* DDS Discovery port */
#define SOAD_DDS_DISCOVERY_PORT         7400U

/* DDS User traffic port offset */
#define SOAD_DDS_USERTRAFFIC_OFFSET     0U

/* DDS Meta traffic port offset */
#define SOAD_DDS_METATRAFFIC_OFFSET     1U

/* DDS default multicast group */
#define SOAD_DDS_MULTICAST_ADDR         0xEFFFFFF5U  /* 239.255.255.245 */

/******************************************************************************
 * Hardware Platform Selection
 ******************************************************************************/

/* Select target hardware platform */
/* #define SOAD_TARGET_AURIX_TC3XX */
/* #define SOAD_TARGET_AURIX_TC4XX */
/* #define SOAD_TARGET_S32K3XX */
/* #define SOAD_TARGET_S32G3 */
#define SOAD_TARGET_EMULATOR            /* For testing */

#ifdef __cplusplus
}
#endif

#endif /* SOAD_CFG_H */
