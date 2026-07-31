/**
 * @file SoAd.h
 * @brief Socket Adapter module - AutoSAR R22-11 Service Layer
 * @version 4.7.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Socket Adapter (SOAD)
 * Module ID: 0x43
 * Layer: Service Layer
 */

#ifndef SOAD_H
#define SOAD_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "SoAd_Cfg.h"
#include "ComStack_Types.h"
#include "TcpIp.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define SOAD_VENDOR_ID                          (0x01U) /* YuleTech Vendor ID */
#define SOAD_MODULE_ID                          (0x43U) /* SOAD Module ID */
#define SOAD_INSTANCE_ID                        (0x00U)

#define SOAD_AR_RELEASE_MAJOR_VERSION           (0x22U)
#define SOAD_AR_RELEASE_MINOR_VERSION           (0x11U)
#define SOAD_AR_RELEASE_REVISION_VERSION        (0x00U)

#define SOAD_SW_MAJOR_VERSION                   (0x04U)
#define SOAD_SW_MINOR_VERSION                   (0x07U)
#define SOAD_SW_PATCH_VERSION                   (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define SOAD_SID_INIT                           (0x01U)
#define SOAD_SID_DEINIT                         (0x02U)
#define SOAD_SID_GETVERSIONINFO                 (0x03U)
#define SOAD_SID_OPENTCPCONNECTION              (0x04U)
#define SOAD_SID_OPENUdpConnection              (0x05U)
#define SOAD_SID_CLOSETCPConnection             (0x06U)
#define SOAD_SID_CLOSEUDPCONNECTION             (0x07U)
#define SOAD_SID_SEND                           (0x08U)
#define SOAD_SID_RECEIVE                        (0x09U)
#define SOAD_SID_GETREMOTESADDR                 (0x0AU)
#define SOAD_SID_SETREMOTESADDR                 (0x0BU)
#define SOAD_SID_RELEASEIPASSIGNMENT            (0x0CU)
#define SOAD_SID_REQUESTIPASSIGNMENT            (0x0DU)
#define SOAD_SID_MAINFUNCTION                   (0x0EU)
#define SOAD_SID_REQUESTCONNMODE                (0x0FU)
#define SOAD_SID_RXINDICATION                   (0x10U)
#define SOAD_SID_TXCONFIRMATION                 (0x11U)
#define SOAD_SID_TCPEVENT                       (0x12U)
#define SOAD_SID_IPADDRASSIGNMENTCHG            (0x13U)
#define SOAD_SID_LOCALIPADDRASSIGNMENTCHG       (0x14U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define SOAD_E_PARAM_POINTER                    (0x01U)
#define SOAD_E_PARAM_CONFIG                     (0x02U)
#define SOAD_E_UNINIT                           (0x03U)
#define SOAD_E_ALREADY_INITIALIZED              (0x04U)
#define SOAD_E_INVALID_CONNID                   (0x05U)
#define SOAD_E_INVALID_PDUID                    (0x06U)
#define SOAD_E_INVALID_SOCKETID                 (0x07U)
#define SOAD_E_INVALID_ADDRESS                  (0x08U)
#define SOAD_E_CONNECTION_STATE                 (0x09U)
#define SOAD_E_BUFFER_OVERFLOW                  (0x0AU)
#define SOAD_E_INIT_FAILED                      (0x0BU)
#define SOAD_E_NOT_SUPPORTED                    (0x0CU)

/*==================================================================================================
*                                    CONNECTION STATES
==================================================================================================*/
typedef enum {
    SOAD_CONN_STATE_CLOSED = 0,
    SOAD_CONN_STATE_CONNECTING,
    SOAD_CONN_STATE_CONNECTED,
    SOAD_CONN_STATE_DISCONNECTING,
    SOAD_CONN_STATE_LISTENING
} SoAd_ConnStateType;

/*==================================================================================================
*                                    PROTOCOL TYPES
==================================================================================================*/
typedef enum {
    SOAD_PROT_TCP = 0,
    SOAD_PROT_UDP
} SoAd_ProtocolType;

/*==================================================================================================
*                                    CONNECTION MODE
==================================================================================================*/
typedef enum {
    SOAD_CONNMODE_REQUESTED_NONE = 0,
    SOAD_CONNMODE_REQUESTED_TCP_CLIENT,
    SOAD_CONNMODE_REQUESTED_TCP_SERVER,
    SOAD_CONNMODE_REQUESTED_UDP
} SoAd_ConnModeRequestType;

/*==================================================================================================
*                                    SOCKET CONFIGURATION TYPE
==================================================================================================*/
typedef struct {
    TcpIp_SocketIdType SocketId;
    SoAd_ProtocolType Protocol;
    uint16 LocalPort;
    TcpIp_IpAddrStateType IpAddrState;
    boolean AutomaticSoConSetup;
    boolean NagleAlgorithm;
} SoAd_SocketConfigType;

/*==================================================================================================
*                                    CONNECTION GROUP CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint16 ConnGrpId;
    SoAd_ProtocolType Protocol;
    boolean IsServer;
    uint16 LocalPort;
    uint16 NumConnections;
    const uint16* ConnectionIds;
} SoAd_ConnGrpConfigType;

/*==================================================================================================
*                                    CONNECTION CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint16 SoConId;
    uint16 ConnGrpId;
    TcpIp_DomainType Domain;
    TcpIp_SocketIdType SocketId;
    boolean AutomaticSoConSetup;
    uint16 TxPduId;
    uint16 RxPduId;
    uint16 RemotePort;
    uint8 RemoteAddr[16]; /* IPv4 or IPv6 */
} SoAd_ConnectionConfigType;

/*==================================================================================================
*                                    PDU ROUTE CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType TxPduId;
    PduIdType RxPduId;
    uint16 SoConId;
    boolean HeaderEnabled;
    uint16 HeaderLength;
} SoAd_PduRouteConfigType;

/*==================================================================================================
*                                    SOAD CONFIG TYPE
==================================================================================================*/
typedef struct {
    const SoAd_SocketConfigType* SocketConfigs;
    uint16 NumSocketConfigs;
    const SoAd_ConnGrpConfigType* ConnGrpConfigs;
    uint16 NumConnGrpConfigs;
    const SoAd_ConnectionConfigType* ConnectionConfigs;
    uint16 NumConnectionConfigs;
    const SoAd_PduRouteConfigType* PduRouteConfigs;
    uint16 NumPduRouteConfigs;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean EnablePduHeader;
} SoAd_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define SOAD_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const SoAd_ConfigType SoAd_Config;

#define SOAD_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define SOAD_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Socket Adapter module
 * @param ConfigPtr Pointer to configuration structure
 */
void SoAd_Init(const SoAd_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the Socket Adapter module
 */
void SoAd_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
#if (SOAD_VERSION_INFO_API == STD_ON)
void SoAd_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Opens a TCP connection
 * @param SoConId Connection ID
 * @return Result of operation
 */
typedef uint16 SoAd_SoConIdType;
Std_ReturnType SoAd_OpenTcpConnection(uint16 SoConId);
Std_ReturnType SoAd_CloseConnection(SoAd_SoConIdType SoConId);
Std_ReturnType SoAd_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Opens a UDP connection
 * @param SoConId Connection ID
 * @return Result of operation
 */
Std_ReturnType SoAd_OpenUdpConnection(uint16 SoConId);

/**
 * @brief Closes a TCP connection
 * @param SoConId Connection ID
 * @param Abort TRUE to abort immediately
 * @return Result of operation
 */
Std_ReturnType SoAd_CloseTcpConnection(uint16 SoConId, boolean Abort);

/**
 * @brief Closes a UDP connection
 * @param SoConId Connection ID
 * @return Result of operation
 */
Std_ReturnType SoAd_CloseUdpConnection(uint16 SoConId);

/**
 * @brief Sends data through a socket connection
 * @param SoConId Connection ID
 * @param PduInfoPtr Pointer to PDU data
 * @return Result of operation
 */
Std_ReturnType SoAd_Send(uint16 SoConId, const PduInfoType* PduInfoPtr);

/**
 * @brief Receives data from a socket connection
 * @param SoConId Connection ID
 * @param PduInfoPtr Pointer to PDU data buffer
 * @param Length Length of data received
 * @return Result of operation
 */
Std_ReturnType SoAd_Receive(uint16 SoConId, PduInfoType* PduInfoPtr, PduLengthType* Length);

/**
 * @brief Gets remote socket address
 * @param SoConId Connection ID
 * @param IpAddrPtr Pointer to store IP address
 * @param PortPtr Pointer to store port
 * @return Result of operation
 */
Std_ReturnType SoAd_GetRemoteAddr(uint16 SoConId, TcpIp_SockAddrType* IpAddrPtr, uint16* PortPtr);

/**
 * @brief Sets remote socket address
 * @param SoConId Connection ID
 * @param IpAddrPtr Pointer to IP address
 * @return Result of operation
 */
Std_ReturnType SoAd_SetRemoteAddr(uint16 SoConId, const TcpIp_SockAddrType* IpAddrPtr);

/**
 * @brief Releases IP address assignment
 * @param LocalAddrId Local address ID
 * @return Result of operation
 */
Std_ReturnType SoAd_ReleaseIpAddrAssignment(uint16 LocalAddrId);

/**
 * @brief Requests IP address assignment
 * @param LocalAddrId Local address ID
 * @param Type IP address assignment type
 * @return Result of operation
 */
Std_ReturnType SoAd_RequestIpAddrAssignment(uint16 LocalAddrId, TcpIp_IpAddrAssignmentType Type);

/**
 * @brief Requests connection mode change
 * @param SoConId Connection ID
 * @param Mode Requested connection mode
 * @return Result of operation
 */
Std_ReturnType SoAd_RequestConnMode(uint16 SoConId, SoAd_ConnModeRequestType Mode);

/**
 * @brief Main function for periodic processing
 */
void SoAd_MainFunction(void);

/*==================================================================================================
*                                    CALLBACK FUNCTIONS
==================================================================================================*/

/**
 * @brief RxIndication callback from TcpIp
 * @param SocketId Socket ID
 * @param RemoteAddrPtr Remote address
 * @param BufPtr Data buffer
 * @param Length Data length
 */
void SoAd_RxIndication(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* RemoteAddrPtr,
                       const uint8* BufPtr, uint16 Length);

/**
 * @brief TxConfirmation callback from TcpIp
 * @param SocketId Socket ID
 * @param Length Transmitted length
 */
void SoAd_TxConfirmation(TcpIp_SocketIdType SocketId, uint16 Length);

/**
 * @brief TcpIp event callback
 * @param SocketId Socket ID
 * @param Event Event type
 * @param EventStatus Event status
 */
void SoAd_TcpIpEvent(TcpIp_SocketIdType SocketId, TcpIp_EventType Event, 
                     TcpIp_ReturnType EventStatus);

/**
 * @brief IP address assignment changed callback
 * @param LocalAddrId Local address ID
 * @param State New state
 */
void SoAd_LocalIpAddrAssignmentChg(uint16 LocalAddrId, TcpIp_IpAddrStateType State);

#define SOAD_STOP_SEC_CODE
#include "MemMap.h"

#endif /* SOAD_H */
