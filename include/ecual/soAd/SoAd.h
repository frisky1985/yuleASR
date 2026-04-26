/******************************************************************************
 * @file    SoAd.h
 * @brief   Socket Adapter (SoAd) - AUTOSAR R22-11
 *
 * This module provides an abstraction layer between PDU Router (PduR) and
 * TCP/IP stack (TcpIp). It manages Socket connections and PDU routing for
 * DDS over IP communication.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x36 (SoAd)
 * ASIL-B Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef SOAD_H
#define SOAD_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "common/autosar_types.h"
#include "common/autosar_errors.h"
#include "autosar/service/Det/Det.h"
#include "tcpip/tcpip_types.h"
#include "tcpip/tcpip_socket.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Module Version Information
 ******************************************************************************/
#define SOAD_VENDOR_ID                  0x01U
#define SOAD_MODULE_ID                  0x36U  /* SoAd module ID per AUTOSAR */
#define SOAD_AR_RELEASE_MAJOR_VERSION   22U
#define SOAD_AR_RELEASE_MINOR_VERSION   11U
#define SOAD_AR_RELEASE_REVISION_VERSION 0U
#define SOAD_SW_MAJOR_VERSION           1U
#define SOAD_SW_MINOR_VERSION           0U
#define SOAD_SW_PATCH_VERSION           0U

/******************************************************************************
 * API Service IDs
 ******************************************************************************/
#define SOAD_SID_INIT                   0x01U
#define SOAD_SID_DEINIT                 0x02U
#define SOAD_SID_GET_VERSION_INFO       0x03U
#define SOAD_SID_MAIN_FUNCTION          0x04U
#define SOAD_SID_IF_TRANSMIT            0x05U
#define SOAD_SID_TP_TRANSMIT            0x06U
#define SOAD_SID_OPEN_SO_CON            0x07U
#define SOAD_SID_CLOSE_SO_CON           0x08U
#define SOAD_SID_GET_SO_CON_STATE       0x09U
#define SOAD_SID_SET_SO_CON_MODE        0x0AU
#define SOAD_SID_REQUEST_IP_ADDR        0x0BU
#define SOAD_SID_RELEASE_IP_ADDR        0x0CU
#define SOAD_SID_GET_LOCAL_ADDR         0x0DU
#define SOAD_SID_GET_PHYS_ADDR          0x0EU
#define SOAD_SID_TP_PROVIDE_RX_BUFFER   0x0FU
#define SOAD_SID_TP_RX_INDICATION       0x10U
#define SOAD_SID_OPEN_ROUTING_GROUP     0x11U
#define SOAD_SID_CLOSE_ROUTING_GROUP    0x12U
#define SOAD_SID_ENABLE_ROUTING         0x13U
#define SOAD_SID_DISABLE_ROUTING        0x14U
#define SOAD_SID_SET_PDU_ROUTING_STATUS 0x15U

/* Upper Layer Callback SIDs */
#define SOAD_SID_IF_TX_CONFIRMATION     0x20U
#define SOAD_SID_IF_RX_INDICATION       0x21U
#define SOAD_SID_TP_TX_CONFIRMATION     0x22U
#define SOAD_SID_TP_START_RECEPTION     0x23U
#define SOAD_SID_TP_COPY_RX_DATA        0x24U
#define SOAD_SID_TP_COPY_TX_DATA        0x25U
#define SOAD_SID_SO_CON_MODE_CHG        0x30U
#define SOAD_SID_TCP_ACCEPTED           0x40U
#define SOAD_SID_TCP_CONNECTED          0x41U
#define SOAD_SID_UDP_RECV               0x42U

/******************************************************************************
 * Error Codes (SoAd specific)
 ******************************************************************************/
#define SOAD_E_NOT_INITIALIZED          0x11U
#define SOAD_E_ALREADY_INITIALIZED      0x12U
#define SOAD_E_INVALID_POINTER          0x13U
#define SOAD_E_INVALID_PDUID            0x14U
#define SOAD_E_INVALID_SOCKET           0x15U
#define SOAD_E_INVALID_PARAMETER        0x16U
#define SOAD_E_PDU_TOO_LARGE            0x17U
#define SOAD_E_ROUTING_ERROR            0x1AU
#define SOAD_E_TIMEOUT                  0x18U
#define SOAD_E_NO_BUFFER                0x19U
#define SOAD_E_BUSY                     0x1BU

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/
#define SOAD_MAX_SOCKET_CONNECTIONS     16U
#define SOAD_MAX_SOCKET_ROUTING_GROUPS  8U
#define SOAD_MAX_PDU_ROUTES             32U
#define SOAD_MAX_SOCKETS                8U
#define SOAD_MAX_PDUS_PER_SOCKET        16U
#define SOAD_PDU_HEADER_SIZE            8U
#define SOAD_MAX_PDU_SIZE               4096U
#define SOAD_SOCKET_RX_BUFFER_SIZE      8192U
#define SOAD_SOCKET_TX_BUFFER_SIZE      8192U
#define SOAD_INVALID_SOCKET_ID          0xFFFFU
#define SOAD_INVALID_PDU_ROUTE_ID       0xFFFFU
#define SOAD_INVALID_PDU_ID             0xFFFFU

/******************************************************************************
 * Socket Types
 ******************************************************************************/
/** SoAd Socket Connection ID */
typedef uint16_t SoAd_SoConIdType;

/** SoAd PDU Route ID */
typedef uint16_t SoAd_PduRouteIdType;

/** Socket Type */
typedef enum {
    SOAD_SOCKET_TYPE_STREAM = 0,   /* TCP Socket */
    SOAD_SOCKET_TYPE_DGRAM         /* UDP Socket */
} SoAd_SocketType;

/** Socket State */
typedef enum {
    SOAD_SOCON_UNINIT = 0,
    SOAD_SOCON_CLOSED,
    SOAD_SOCON_INITIALIZED,
    SOAD_SOCON_CONNECTING,
    SOAD_SOCON_CONNECTED,
    SOAD_SOCON_LISTENING,
    SOAD_SOCON_SHUTDOWN,
    SOAD_SOCON_ERROR
} SoAd_SoConStateType;

/** Socket Connection Mode */
typedef enum {
    SOAD_SOCON_MODE_TCPIP = 0,     /* Standard TCP/UDP mode */
    SOAD_SOCON_MODE_UDP_EXT        /* UDP extended mode */
} SoAd_SoConModeType;

/** Socket Protocol */
typedef enum {
    SOAD_SOCKET_PROTOCOL_TCP = 0,
    SOAD_SOCKET_PROTOCOL_UDP
} SoAd_SocketProtocolType;

/******************************************************************************
 * Address Types
 ******************************************************************************/
/** Socket Address Type */
typedef struct {
    uint32_t addr;                 /* IP address (IPv4, network byte order) */
    uint16_t port;                 /* Port number (network byte order) */
    boolean isMulticast;
} SoAd_SocketAddrType;

/** Physical Address Type (MAC) */
typedef uint8 SoAd_PhysAddrType[6];

/******************************************************************************
 * PDU Types
 ******************************************************************************/
/** PDU Direction */
typedef enum {
    SOAD_PDU_DIR_TX = 0,           /* Transmit direction */
    SOAD_PDU_DIR_RX,               /* Receive direction */
    SOAD_PDU_DIR_TX_RX             /* Bidirectional */
} SoAd_PduDirectionType;

/** Upper Layer Type */
typedef enum {
    SOAD_UPPER_PDUR = 0,           /* PDU Router */
    SOAD_UPPER_DOIP,               /* Diagnostic over IP */
    SOAD_UPPER_SD,                 /* Service Discovery */
    SOAD_UPPER_DDS                 /* DDS Transport */
} SoAd_UpperLayerType;

/** PDU Info Type */
typedef struct {
    uint8 *SduDataPtr;
    uint16 SduLength;
    uint16 PduId;
    SoAd_PduRouteIdType RouteId;
} SoAd_PduInfoType;

/******************************************************************************
 * Configuration Types
 ******************************************************************************/
/** Socket Connection Configuration */
typedef struct {
    SoAd_SoConIdType SoConId;              /* Socket connection ID */
    SoAd_SocketType SocketType;            /* TCP or UDP */
    SoAd_SocketProtocolType Protocol;      /* Protocol type */
    SoAd_SoConModeType Mode;               /* Connection mode */
    uint16 LocalPort;                      /* Local port */
    uint32 LocalIpAddr;                    /* Local IP address */
    uint16 RemotePort;                     /* Remote port (for client) */
    uint32 RemoteIpAddr;                   /* Remote IP (for client) */
    boolean AutoConnect;                   /* Auto connect enable */
    boolean IsServer;                      /* Server socket flag */
    boolean EnableKeepAlive;               /* TCP keep-alive enable */
    uint16 KeepAliveTime;                  /* Keep-alive time (seconds) */
    boolean EnableNagle;                   /* Nagle algorithm enable */
    uint16 RxBufferSize;                   /* RX buffer size */
    uint16 TxBufferSize;                   /* TX buffer size */
    uint16 ConnTimeoutMs;                  /* Connection timeout */
    uint16 TxTimeoutMs;                    /* Transmission timeout */
    uint16 RoutingGroup;                   /* Associated routing group */
} SoAd_SocketConnectionCfgType;

/** PDU Route Configuration */
typedef struct {
    SoAd_PduRouteIdType RouteId;           /* Route ID */
    SoAd_SoConIdType SoConId;              /* Associated socket */
    uint16 TxPduId;                        /* Transmit PDU ID (from PduR) */
    uint16 RxPduId;                        /* Receive PDU ID (to PduR) */
    SoAd_PduDirectionType Direction;       /* PDU direction */
    SoAd_UpperLayerType UpperLayer;        /* Upper layer module */
    boolean EnableHeader;                  /* PDU header enable */
    uint8 HeaderSize;                      /* Header size in bytes */
    uint16 PduSize;                        /* Maximum PDU size */
    boolean UseTp;                         /* Use Transport Protocol */
    uint16 RoutingGroup;                   /* Associated routing group */
} SoAd_PduRouteCfgType;

/** Routing Group Configuration */
typedef struct {
    uint16 RoutingGroupId;                 /* Routing group ID */
    boolean IsEnabled;                     /* Default enabled state */
    uint16 NumPduRoutes;                   /* Number of PDU routes */
    const SoAd_PduRouteIdType *PduRouteIds; /* Array of PDU route IDs */
} SoAd_RoutingGroupCfgType;

/** SoAd Module Configuration */
typedef struct {
    const SoAd_SocketConnectionCfgType *SocketConnections;
    uint16 NumSocketConnections;
    const SoAd_PduRouteCfgType *PduRoutes;
    uint16 NumPduRoutes;
    const SoAd_RoutingGroupCfgType *RoutingGroups;
    uint16 NumRoutingGroups;
    uint32 MainFunctionPeriodMs;           /* Main function period in ms */
} SoAd_ConfigType;

/******************************************************************************
 * Status Types
 ******************************************************************************/
/** Socket Connection Status */
typedef struct {
    SoAd_SoConStateType State;
    SoAd_SocketAddrType LocalAddr;
    SoAd_SocketAddrType RemoteAddr;
    boolean IsServer;
    uint32 BytesSent;
    uint32 BytesReceived;
    uint32 ConnectTime;
    uint32 LastActivity;
    uint16 ErrorCount;
    tcpip_socket_id_t TcpIpSocketId;
} SoAd_SoConStatusType;

/******************************************************************************
 * Callback Function Types
 ******************************************************************************/
/** IF Transmit Confirmation Callback */
typedef void (*SoAd_IfTxConfirmation_FuncType)(uint16 PduId);

/** IF Receive Indication Callback */
typedef void (*SoAd_IfRxIndication_FuncType)(uint16 PduId, const uint8 *Data, uint16 Length);

/** TP Transmit Confirmation Callback */
typedef void (*SoAd_TpTxConfirmation_FuncType)(uint16 PduId, Std_ReturnType Result);

/** Buffer Request Return Type */
typedef enum {
    BUFREQ_OK = 0,          /*!< Buffer request successful */
    BUFREQ_E_NOT_OK,        /*!< Buffer request not successful */
    BUFREQ_E_BUSY,          /*!< Temporarily no buffer available */
    BUFREQ_E_OVFL           /*!< Receiver aborted reception, requested length not available */
} BufReq_ReturnType;

/** TP Receive Indication Callback */
typedef BufReq_ReturnType (*SoAd_TpRxIndication_FuncType)(uint16 RxPduId,
                                                           const SoAd_PduInfoType *PduInfoPtr,
                                                           uint16 *BufferSizePtr);

/** Socket Connection Mode Change Callback */
typedef void (*SoAd_SoConModeChg_FuncType)(SoAd_SoConIdType SoConId, SoAd_SoConStateType NewMode);

/** Socket Error Callback */
typedef void (*SoAd_SoConError_FuncType)(SoAd_SoConIdType SoConId, uint8 ErrorCode);

/******************************************************************************
 * Core API Functions
 ******************************************************************************/
/**
 * @brief Initialize SoAd module
 * @param ConfigPtr Pointer to module configuration
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_Init(const SoAd_ConfigType *ConfigPtr);

/**
 * @brief Deinitialize SoAd module
 */
void SoAd_DeInit(void);

/**
 * @brief Get version information
 * @param versioninfo Pointer to store version info
 */
void SoAd_GetVersionInfo(Std_VersionInfoType *versioninfo);

/**
 * @brief SoAd main function - called cyclically
 */
void SoAd_MainFunction(void);

/******************************************************************************
 * Socket Connection Management API
 ******************************************************************************/
/**
 * @brief Open Socket connection
 * @param SoConId Socket connection ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_OpenSoCon(SoAd_SoConIdType SoConId);

/**
 * @brief Close Socket connection
 * @param SoConId Socket connection ID
 * @param Abort Abort connection immediately if TRUE
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_CloseSoCon(SoAd_SoConIdType SoConId, boolean Abort);

/**
 * @brief Get Socket connection state
 * @param SoConId Socket connection ID
 * @param StatePtr Pointer to store state
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_GetSoConState(SoAd_SoConIdType SoConId, SoAd_SoConStateType *StatePtr);

/**
 * @brief Set Socket connection mode
 * @param SoConId Socket connection ID
 * @param Mode Connection mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_SetSoConMode(SoAd_SoConIdType SoConId, SoAd_SoConModeType Mode);

/**
 * @brief Request IP address assignment
 * @param SoConId Socket connection ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_RequestIpAddrAssignment(SoAd_SoConIdType SoConId);

/**
 * @brief Release IP address assignment
 * @param SoConId Socket connection ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_ReleaseIpAddrAssignment(SoAd_SoConIdType SoConId);

/**
 * @brief Get local address
 * @param SoConId Socket connection ID
 * @param LocalAddrPtr Pointer to store local address
 * @param NetmaskPtr Pointer to store netmask (optional)
 * @param DefaultRouterPtr Pointer to store default router (optional)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_GetLocalAddr(SoAd_SoConIdType SoConId, 
                                  SoAd_SocketAddrType *LocalAddrPtr,
                                  uint32 *NetmaskPtr,
                                  uint32 *DefaultRouterPtr);

/**
 * @brief Get physical address (MAC)
 * @param SoConId Socket connection ID
 * @param PhysAddrPtr Pointer to store physical address
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_GetPhysAddr(SoAd_SoConIdType SoConId, SoAd_PhysAddrType *PhysAddrPtr);

/******************************************************************************
 * PDU Transmission API
 ******************************************************************************/
/**
 * @brief Interface Transmit - Send PDU via IF interface (PduR -> SoAd)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_IfTransmit(uint16 TxPduId, const SoAd_PduInfoType *PduInfoPtr);

/**
 * @brief TP Transmit - Send PDU via TP interface (PduR -> SoAd)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * @param RetryInfoPtr Retry information (NULL if no retry)
 * @param TpDataLength Total TP data length
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_TpTransmit(uint16 TxPduId, 
                                const SoAd_PduInfoType *PduInfoPtr,
                                const uint8 *RetryInfoPtr,
                                uint16 TpDataLength);

/**
 * @brief TP Provide RX Buffer - Provide buffer for TP reception
 * @param RxPduId Receive PDU ID
 * @param TpSduLength TP SDU length
 * @param PduInfoPtr Pointer to PDU information (buffer info)
 * @return BufReq_ReturnType buffer request result
 */
Std_ReturnType SoAd_TpProvideRxBuffer(uint16 RxPduId, 
                                       uint16 TpSduLength,
                                       SoAd_PduInfoType *PduInfoPtr);

/**
 * @brief TP RX Indication - Indicate TP reception complete
 * @param RxPduId Receive PDU ID
 * @param Result Reception result
 */
void SoAd_TpRxIndication(uint16 RxPduId, Std_ReturnType Result);

/******************************************************************************
 * Routing Group API
 ******************************************************************************/
/**
 * @brief Enable routing group
 * @param RoutingGroup Routing group ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_EnableRouting(uint16 RoutingGroup);

/**
 * @brief Disable routing group
 * @param RoutingGroup Routing group ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_DisableRouting(uint16 RoutingGroup);

/**
 * @brief Open routing group (establish connections)
 * @param RoutingGroup Routing group ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_OpenRoutingGroup(uint16 RoutingGroup);

/**
 * @brief Close routing group (close connections)
 * @param RoutingGroup Routing group ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_CloseRoutingGroup(uint16 RoutingGroup);

/**
 * @brief Set PDU routing status
 * @param PduId PDU ID
 * @param Status TRUE to enable, FALSE to disable
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_SetPduRoutingStatus(uint16 PduId, boolean Status);

/******************************************************************************
 * Upper Layer Callback Functions (implemented by PduR)
 ******************************************************************************/
/**
 * @brief IF Transmit Confirmation (SoAd -> PduR)
 * @param TxPduId Transmit PDU ID
 */
extern void SoAd_IfTxConfirmation(uint16 TxPduId);

/**
 * @brief IF Receive Indication (SoAd -> PduR)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU information
 */
extern void SoAd_IfRxIndication(uint16 RxPduId, const SoAd_PduInfoType *PduInfoPtr);

/**
 * @brief TP Transmit Confirmation (SoAd -> PduR)
 * @param TxPduId Transmit PDU ID
 * @param Result Transmission result
 */
extern void SoAd_TpTxConfirmation(uint16 TxPduId, Std_ReturnType Result);

/**
 * @brief TP Start Reception callback (PduR -> SoAd)
 * @param RxPduId Receive PDU ID
 * @param TpSduLength Total length of TP SDU
 * @param BufferSizePtr Available buffer size
 * @return BufReq_ReturnType buffer request result
 */
extern Std_ReturnType SoAd_TpStartOfReception(uint16 RxPduId,
                                               uint16 TpSduLength,
                                               uint16 *BufferSizePtr);

/**
 * @brief TP Copy RX Data callback (PduR -> SoAd)
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU info with data
 * @param BufferSizePtr Remaining buffer size
 * @return BufReq_ReturnType buffer request result
 */
extern Std_ReturnType SoAd_TpCopyRxData(uint16 RxPduId,
                                         const SoAd_PduInfoType *PduInfoPtr,
                                         uint16 *BufferSizePtr);

/**
 * @brief TP Copy TX Data callback (PduR -> SoAd)
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU info for data
 * @param RetryInfoPtr Retry information
 * @param AvailableDataPtr Available data for transmission
 * @return BufReq_ReturnType buffer request result
 */
extern Std_ReturnType SoAd_TpCopyTxData(uint16 TxPduId,
                                         const SoAd_PduInfoType *PduInfoPtr,
                                         const uint8 *RetryInfoPtr,
                                         uint16 *AvailableDataPtr);

/**
 * @brief Socket Connection Mode Change callback (SoAd -> Upper Layer)
 * @param SoConId Socket connection ID
 * @param NewMode New socket connection mode
 */
extern void SoAd_SoConModeChg(SoAd_SoConIdType SoConId, SoAd_SoConStateType NewMode);

/******************************************************************************
 * TcpIp Callback Functions (called by TcpIp module)
 ******************************************************************************/
/**
 * @brief TCP Connection Accepted callback
 * @param SocketId Socket ID from TcpIp
 * @param RemoteAddr Remote address
 */
void SoAd_TcpAccepted(tcpip_socket_id_t SocketId, const tcpip_sockaddr_t *RemoteAddr);

/**
 * @brief TCP Connected callback
 * @param SocketId Socket ID from TcpIp
 * @param Result Connection result
 */
void SoAd_TcpConnected(tcpip_socket_id_t SocketId, tcpip_error_t Result);

/**
 * @brief TCP/IP Event callback
 * @param SocketId Socket ID
 * @param Event Event type (connected, disconnected, data received, etc.)
 * @param UserData User data pointer
 */
void SoAd_TcpIpEvent(tcpip_socket_id_t SocketId, uint8 Event, void *UserData);

/**
 * @brief UDP Data Received callback
 * @param SocketId Socket ID from TcpIp
 * @param Data Data pointer
 * @param Length Data length
 * @param RemoteAddr Remote address
 */
void SoAd_UdpRecv(tcpip_socket_id_t SocketId, 
                  const uint8 *Data, 
                  uint16 Length,
                  const tcpip_sockaddr_t *RemoteAddr);

/******************************************************************************
 * Tx Confirmation from Lower Layer (EthIf)
 ******************************************************************************/
/**
 * @brief Transmit confirmation from lower layer
 * @param TxPduId Transmit PDU ID
 * @param Result Transmission result
 */
void SoAd_TxConfirmation(uint16 TxPduId, Std_ReturnType Result);

/**
 * @brief Receive indication from lower layer
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU information
 */
void SoAd_RxIndication(uint16 RxPduId, const SoAd_PduInfoType *PduInfoPtr);

#ifdef __cplusplus
}
#endif

#endif /* SOAD_H */
