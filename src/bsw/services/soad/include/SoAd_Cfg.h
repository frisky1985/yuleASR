/**
 * @file SoAd_Cfg.h
 * @brief Socket Adapter configuration header - AutoSAR R22-11
 * @version 4.7.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef SOAD_CFG_H
#define SOAD_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define SOAD_DEV_ERROR_DETECT                   (STD_ON)
#define SOAD_VERSION_INFO_API                   (STD_ON)

/*==================================================================================================
*                                    SOCKET CONFIGURATION
==================================================================================================*/
#define SOAD_NUMBER_OF_SOCKETS                  (8U)
#define SOAD_NUMBER_OF_CONNECTION_GROUPS        (4U)
#define SOAD_NUMBER_OF_CONNECTIONS              (16U)
#define SOAD_NUMBER_OF_PDU_ROUTES               (32U)

/*==================================================================================================
*                                    BUFFER CONFIGURATION
==================================================================================================*/
#define SOAD_MAX_PDU_LENGTH                     (1500U)
#define SOAD_MAX_HEADER_LENGTH                  (8U)
#define SOAD_RX_BUFFER_SIZE                     (8192U)
#define SOAD_TX_BUFFER_SIZE                     (8192U)

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION
==================================================================================================*/
#define SOAD_CONNECT_TIMEOUT_MS                 (5000U)
#define SOAD_DISCONNECT_TIMEOUT_MS              (2000U)
#define SOAD_ACK_TIMEOUT_MS                     (1000U)
#define SOAD_RECONNECT_DELAY_MS                 (1000U)

/*==================================================================================================
*                                    FEATURE CONFIGURATION
==================================================================================================*/
#define SOAD_PDU_HEADER_ENABLE                  (STD_ON)
#define SOAD_PDU_HEADER_LENGTH                  (8U)
#define SOAD_ENABLE_TCP_KEEP_ALIVE              (STD_ON)
#define SOAD_ENABLE_NAGLE_ALGORITHM             (STD_OFF)
#define SOAD_ENABLE_SO_CON_MODE_CHANGE          (STD_ON)
#define SOAD_ENABLE_AUTO_CONNECTION_SETUP       (STD_ON)
#define SOAD_ENABLE_IP_ADDR_CHANGE_NOTIFICATION (STD_ON)

/*==================================================================================================
*                                    CONNECTION IDs
==================================================================================================*/
#define SOAD_CONN_ID_TCP_SERVER_0               (0U)
#define SOAD_CONN_ID_TCP_CLIENT_0               (1U)
#define SOAD_CONN_ID_UDP_UNICAST_0              (2U)
#define SOAD_CONN_ID_UDP_MULTICAST_0            (3U)

/*==================================================================================================
*                                    PDU IDs
==================================================================================================*/
#define SOAD_PDU_ID_SOMEIP_MSG                  (0U)
#define SOAD_PDU_ID_SOMEIP_SD                   (1U)
#define SOAD_PDU_ID_DOIP_DIAG                   (2U)
#define SOAD_PDU_ID_DOIP_ANNOUNCE               (3U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define SOAD_MAIN_FUNCTION_PERIOD_MS            (10U)

#endif /* SOAD_CFG_H */
