# SoAd (Socket Adapter) Module

## Overview

The SoAd module provides an adaptation layer between the AUTOSAR PDU-based communication and the socket-based TCP/IP communication. It enables AUTOSAR services to communicate over standard TCP/IP networks, supporting both connection-oriented (TCP) and connectionless (UDP) protocols.

**AUTOSAR Standard**: Classic Platform 4.4.0  
**Layer**: Services (Service Layer)  
**Protocol**: TCP, UDP, IPv4, IPv6  
**Hardware**: Ethernet-based systems  
**ASIL Level**: QM (configurable up to ASIL-D)

## Features

- **TCP/UDP Support**: Both connection-oriented and connectionless protocols
- **IPv4/IPv6**: Dual-stack IP support
- **PDU-based Interface**: Maps AUTOSAR PDUs to socket connections
- **Multiple Connections**: Support for multiple simultaneous socket connections
- **Automatic Reconnection**: Automatic TCP reconnection on connection loss
- **Routing Groups**: Group-based connection management
- **Nagle Algorithm**: TCP Nagle algorithm control for latency optimization
- **Keep-alive**: TCP keep-alive for connection monitoring
- **Socket Options**: Configurable socket options (buffer sizes, timeouts)
- **PDU Header Option**: Optional PDU header for protocol identification

## Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│  (DDS, SOME/IP, DoIP, MQTT)         │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│          SoAd Module                 │
│  ┌─────────────────────────────────────┐ │
│  │  PDU Routing (PduR)               │ │
│  │  Socket Connection Management      │ │
│  │  TCP Connection Handling           │ │
│  │  UDP Endpoint Management           │ │
│  └─────────────────────────────────────┘ │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│       TcpIp (TCP/IP Stack)           │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│       EthIf (Ethernet Interface)    │
└─────────────────────────────────────┘
```

## API Reference

### Core Functions

```c
/* Initialization */
void SoAd_Init(const SoAd_ConfigType* ConfigPtr);
void SoAd_DeInit(void);

/* Connection Control */
Std_ReturnType SoAd_OpenTcpServerConnection(SoAd_SoConIdType SoConId, 
                                            SoAd_SoConIdType* ConIdPtr);
Std_ReturnType SoAd_OpenTcpClientConnection(SoAd_SoConIdType SoConId);
Std_ReturnType SoAd_CloseConnection(SoAd_SoConIdType SoConId, boolean Abort);
Std_ReturnType SoAd_RequestIpAddrAssignment(SoAd_SoConIdType SoConId, 
                                            TcpIp_IpAddrAssignmentType Type, 
                                            const TcpIp_SockAddrType* LocalAddrPtr);
Std_ReturnType SoAd_ReleaseIpAddrAssignment(SoAd_SoConIdType SoConId);
Std_ReturnType SoAd_RequestCommMode(uint16 SoAdPduRouteDest, 
                                    SoAd_TcpIpConnectionModeType Mode);

/* Data Transmission */
Std_ReturnType SoAd_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
Std_ReturnType SoAd_IfRoutingGroupTransmit(SoAd_RoutingGroupIdType RoutingGroupId);
Std_ReturnType SoAd_TpTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
Std_ReturnType SoAd_TpCancelTransmit(PduIdType TxPduId);
Std_ReturnType SoAd_TpCancelReceive(PduIdType RxPduId);

/* Main Functions */
void SoAd_MainFunction(void);

/* Version Info */
void SoAd_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

### Callback Functions

```c
/* TCP/IP Event Callbacks */
void SoAd_TcpIpEvent(SoAd_SoConIdType SoConId, SoAd_TcpIpEventType Event);
void SoAd_TcpIpEventAssignment(SoAd_SoConIdType SoConId, SoAd_TcpIpEventType Event);

/* PDU Notification Callbacks */
void PduR_SoAdIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void PduR_SoAdTpRxIndication(PduIdType RxPduId, Std_ReturnType Result);
void PduR_SoAdIfTxConfirmation(PduIdType TxPduId, Std_ReturnType Result);
void PduR_SoAdTpTxConfirmation(PduIdType TxPduId, Std_ReturnType Result);
BufReq_ReturnType PduR_SoAdTpCopyTxData(PduIdType TxPduId, 
                                        const PduInfoType* PduInfoPtr, 
                                        RetryInfoType* RetryInfoPtr, 
                                        PduLengthType* AvailableDataPtr);
BufReq_ReturnType PduR_SoAdTpCopyRxData(PduIdType RxPduId, 
                                        const PduInfoType* PduInfoPtr, 
                                        PduLengthType* BufferSizePtr);
BufReq_ReturnType PduR_SoAdTpStartOfReception(PduIdType RxPduId, 
                                              const PduInfoType* SduInfoPtr, 
                                              PduLengthType TpSduLength, 
                                              PduLengthType* BufferSizePtr);

/* Local IP Address Assignment Callback */
void SoAd_LocalIpAddrAssignmentChg(SoAd_SoConIdType SoConId, 
                                   TcpIp_IpAddrStateType State);
```

### Data Types

```c
typedef uint16 SoAd_SoConIdType;          /* Socket connection ID */
typedef uint16 SoAd_RoutingGroupIdType;   /* Routing group ID */

typedef enum {
    SOAD_TCP_IP_EVENT_TCP_FINISHED = 0,
    SOAD_TCP_IP_EVENT_TCP_RST_FINISHED,
    SOAD_TCP_IP_EVENT_TCP_CLOSED,
    SOAD_TCP_IP_EVENT_TCP_INCOMING_CONNECTION,
    SOAD_TCP_IP_EVENT_UDP_CLOSED,
    SOAD_TCP_IP_EVENT_UDP_LISTEN_REQ_FAILED,
    SOAD_TCP_IP_EVENT_TCP_LISTEN_REQ_FAILED,
    SOAD_TCP_IP_EVENT_TCP_CONNECT_REQ_FAILED,
    SOAD_TCP_IP_EVENT_UDP_TX_FAILED,
    SOAD_TCP_IP_EVENT_TCP_TX_FAILED,
    SOAD_TCP_IP_EVENT_TCP_ACKED,
    SOAD_TCP_IP_EVENT_TCP_HANDSHAKE_COMPLETE,
    SOAD_TCP_IP_EVENT_TCP_HANDSHAKE_FAILED
} SoAd_TcpIpEventType;

typedef enum {
    SOAD_TCP_IP_CONNECTION_MODE_TCP_SERVER = 0,
    SOAD_TCP_IP_CONNECTION_MODE_TCP_CLIENT,
    SOAD_TCP_IP_CONNECTION_MODE_UDP_CLIENT,
    SOAD_TCP_IP_CONNECTION_MODE_UDP_SERVER
} SoAd_TcpIpConnectionModeType;

typedef enum {
    SOAD_SOCON_OFFLINE = 0,
    SOAD_SOCON_ONLINE,
    SOAD_SOCON_RECONNECT
} SoAd_SoConModeType;

/* Socket Address Type */
typedef struct {
    uint16 port;                    /* Port number */
    uint32 addr[4];                 /* IPv4 or IPv6 address */
    boolean isIPv6;                 /* TRUE for IPv6 */
} SoAd_SockAddrType;
```

## Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `SoAdSocketConnectionId` | uint16 | Unique socket connection identifier |
| `SoAdSocketConnectionGroup` | struct | Connection group configuration |
| `SoAdPduRoute` | struct | PDU routing configuration |
| `SoAdSocketRemoteAddr` | struct | Remote IP address and port |
| `SoAdSocketLocalAddr` | struct | Local IP address and port |
| `SoAdSocketProtocol` | enum | TCP or UDP |
| `SoAdSocketAutomaticSoConSetup` | boolean | Automatic connection setup |
| `SoAdSocketSoConMode` | enum | Server or Client mode |
| `SoAdPduRouteDest` | struct | PDU route destination |
| `SoAdRoutingGroup` | struct | Routing group for connection control |
| `SoAdSocketTcpInitiate` | boolean | TCP active open (client) |
| `SoAdSocketTcpKeepAlive` | boolean | TCP keep-alive enable |
| `SoAdSocketTcpKeepAliveTime` | uint16 | Keep-alive time in seconds |
| `SoAdSocketNagleAlgorithm` | boolean | Nagle algorithm enable |
| `SoAdSocketUdpListenTimeout` | uint16 | UDP listen timeout |
| `SoAdSocketUdpTriggerTimeout` | uint16 | UDP trigger timeout |
| `SoAdPduHeaderEnable` | boolean | Enable PDU header |

## Usage Example

### TCP Client Connection

```c
#include "SoAd.h"
#include "SoAd_Cfg.h"

void SoAd_TcpClientExample(void)
{
    Std_ReturnType status;
    
    /* Initialize SoAd */
    SoAd_Init(&SoAd_Config);
    
    /* Open TCP client connection (connection ID 0) */
    status = SoAd_OpenTcpClientConnection(0);
    
    if (status == E_OK) {
        /* Connection request initiated */
        /* Wait for SOAD_TCP_IP_EVENT_TCP_HANDSHAKE_COMPLETE event */
    }
}

void SoAd_TcpTransmitExample(void)
{
    PduInfoType PduInfo;
    uint8 txData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    PduInfo.SduDataPtr = txData;
    PduInfo.SduLength = sizeof(txData);
    
    /* Transmit PDU over TCP connection */
    SoAd_IfTransmit(0, &PduInfo);
}

void SoAd_TcpEventCallback(SoAd_SoConIdType SoConId, SoAd_TcpIpEventType Event)
{
    switch (Event) {
        case SOAD_TCP_IP_EVENT_TCP_HANDSHAKE_COMPLETE:
            /* TCP connection established */
            ConnectionEstablished = TRUE;
            break;
            
        case SOAD_TCP_IP_EVENT_TCP_CLOSED:
            /* TCP connection closed */
            ConnectionEstablished = FALSE;
            break;
            
        case SOAD_TCP_IP_EVENT_TCP_TX_FAILED:
            /* Transmission failed */
            break;
            
        default:
            break;
    }
}
```

### UDP Communication

```c
void SoAd_UdpExample(void)
{
    PduInfoType PduInfo;
    uint8 udpData[] = "Hello UDP!";
    
    /* Initialize UDP connection (connection ID 1) */
    SoAd_Init(&SoAd_Config);
    
    PduInfo.SduDataPtr = udpData;
    PduInfo.SduLength = sizeof(udpData) - 1;
    
    /* Transmit PDU over UDP */
    SoAd_IfTransmit(1, &PduInfo);
}
```

### PDU Reception

```c
/* PduR callback for PDU reception */
void PduR_SoAdIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    /* Process received PDU */
    if (RxPduId == 0) {
        /* Process TCP received data */
        ProcessTcpData(PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
    } else if (RxPduId == 1) {
        /* Process UDP received data */
        ProcessUdpData(PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
    }
}

/* Transmission confirmation */
void PduR_SoAdIfTxConfirmation(PduIdType TxPduId, Std_ReturnType Result)
{
    if (Result == E_OK) {
        /* Transmission successful */
    } else {
        /* Transmission failed */
    }
}
```

### Routing Group Control

```c
void SoAd_RoutingGroupExample(void)
{
    /* Enable routing group (activate all connections in group) */
    SoAd_IfRoutingGroupTransmit(0);
    
    /* Enable specific PDU route destination */
    SoAd_RequestCommMode(0, SOAD_TCP_IP_CONNECTION_MODE_TCP_CLIENT);
}
```

### IP Address Management

```c
void SoAd_IpAddrExample(void)
{
    TcpIp_SockAddrType localAddr;
    
    /* Configure local address */
    localAddr.port = 5001;
    localAddr.isIPv6 = FALSE;
    localAddr.addr[0] = 0xC0A80101;  /* 192.168.1.1 */
    
    /* Request IP address assignment */
    SoAd_RequestIpAddrAssignment(0, TCPIP_IPADDR_ASSIGNMENT_STATIC, &localAddr);
    
    /* Release IP address when done */
    SoAd_ReleaseIpAddrAssignment(0);
}
```

## PDU Header Format

When PDU header is enabled:

```
+--------+--------+--------+--------+--------+--------+
|      PDU Length (4 bytes)       |  PDU ID (2 bytes) |
+--------+--------+--------+--------+--------+--------+
|                       Data                      ...
+--------+--------+--------+--------+--------+--------+
```

- **PDU Length**: 32-bit length of PDU data (big endian)
- **PDU ID**: 16-bit PDU identifier (big endian)
- **Data**: Actual PDU payload

## Error Handling

| Error Code | Description | Detection |
|------------|-------------|-----------|
| `SOAD_E_UNINIT` | Module not initialized | API check |
| `SOAD_E_PARAM_POINTER` | NULL pointer | Parameter validation |
| `SOAD_E_PARAM_ID` | Invalid ID | Parameter validation |
| `SOAD_E_PARAM_CONFIG` | Invalid configuration | Validation |
| `SOAD_E_NO_CONNECTION` | No connection available | Connection check |
| `SOAD_E_NOT_OK` | General error | Operation failure |

## Connection State Machine

```
        +-------------+
        |   OFFLINE   |
        +------+------+
               | SoAd_OpenTcpClientConnection()
               | SoAd_OpenTcpServerConnection()
               v
        +-------------+
        | RECONNECT   |<---+
        +------+------+     |
               |            | Connection lost
               | Connected  |
               v            |
        +-------------+     |
        |   ONLINE    |-----+
        +------+------+
               | SoAd_CloseConnection()
               v
        +-------------+
        |   OFFLINE   |
        +-------------+
```

## Hardware Requirements

### Resource Usage
| Resource | Typical Usage |
|----------|---------------|
| RAM | ~10-50 KB (connection dependent) |
| ROM | ~30-50 KB |
| Sockets | 2-32 (configurable) |
| PDU Routes | 1-64 (configurable) |

## Dependencies

### Required Modules
- `Std_Types`, `Platform_Types`, `Compiler`
- `Det` - Error tracing
- `SchM_SoAd` - Schedule manager
- `TcpIp` - TCP/IP stack
- `PduR` - PDU router

### Optional Modules
- `EthIf` - Ethernet interface (for EthIf-based TcpIp)
- `Xcp` - XCP over Ethernet
- `DoIP` - Diagnostic over IP
- `DDS` - Data Distribution Service

## References

- AUTOSAR SWS Socket Adapter
- IETF RFC 793 (TCP)
- IETF RFC 768 (UDP)
- IETF RFC 791 (IPv4)
- IETF RFC 2460 (IPv6)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-01 | Initial release |
| 1.1.0 | 2024-05 | Added IPv6 support |
| 1.2.0 | 2024-09 | TP API optimization |
| 1.3.0 | 2024-12 | Connection multiplexing |
