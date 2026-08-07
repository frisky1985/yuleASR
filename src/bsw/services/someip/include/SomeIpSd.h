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

/**
 * @file SomeIpSd.h
 * @brief SOME/IP Service Discovery
 * @version 1.0.0
 * 
 * Service Discovery protocol for SOME/IP
 * Handles service offering, finding, and subscription
 */

#ifndef SOMEIP_SD_H
#define SOMEIP_SD_H

#include "SomeIp.h"

/* Module ID */
#define SOMEIPSD_AR_RELEASE_MAJOR_VERSION   4U
#define SOMEIPSD_AR_RELEASE_MINOR_VERSION   4U
#define SOMEIPSD_AR_RELEASE_REVISION_VERSION 0U
#define SOMEIPSD_SW_MAJOR_VERSION           1U
#define SOMEIPSD_SW_MINOR_VERSION           0U
#define SOMEIPSD_SW_PATCH_VERSION           0U
#define SOMEIPSD_MODULE_ID                 0x71

/* Service IDs */
#define SOMEIPSD_INIT_SID                  0x01
#define SOMEIPSD_DEINIT_SID                0x02
#define SOMEIPSD_OFFERSERVICE_SID          0x03
#define SOMEIPSD_STOPSERVICE_SID           0x04
#define SOMEIPSD_FINDSERVICE_SID           0x05
#define SOMEIPSD_SUBSCRIBEEVENT_SID        0x06
#define SOMEIPSD_UNSUBSCRIBEEVENT_SID      0x07
#define SOMEIPSD_HANDLEMSG_SID             0x08
#define SOMEIPSD_MAINFUNCTION_SID          0x09

/* Error Codes */
#define SOMEIPSD_E_NOT_INITIALIZED         0x01
#define SOMEIPSD_E_INVALID_POINTER         0x02
#define SOMEIPSD_E_INVALID_SERVICE         0x03
#define SOMEIPSD_E_INVALID_INSTANCE        0x04
#define SOMEIPSD_E_INVALID_EVENT           0x05
#define SOMEIPSD_E_ALREADY_OFFERED         0x06
#define SOMEIPSD_E_NOT_OFFERED             0x07
#define SOMEIPSD_E_NO_FREE_ENTRY           0x08

/* SOME/IP-SD Specific Constants */
#define SOMEIPSD_SERVICE_ID                0xFFFF
#define SOMEIPSD_METHOD_ID                 0x8100U

/* SD Message Types */
#define SOMEIPSD_MSG_FIND_SERVICE          0x00
#define SOMEIPSD_MSG_OFFER_SERVICE         0x01
#define SOMEIPSD_MSG_STOP_OFFER            0x01  /* Same as offer with TTL=0 */
#define SOMEIPSD_MSG_SUBSCRIBE             0x06
#define SOMEIPSD_MSG_SUBSCRIBE_ACK         0x07
#define SOMEIPSD_MSG_SUBSCRIBE_NACK        0x07  /* Same with different return code */
#define SOMEIPSD_MSG_STOP_SUBSCRIBE        0x06  /* Same as subscribe with TTL=0 */

/* Entry Types */
typedef uint8 SomeIpSd_EntryTypeType;
#define SOMEIPSD_ENTRY_FIND_SERVICE        0x00
#define SOMEIPSD_ENTRY_OFFER_SERVICE       0x01
#define SOMEIPSD_ENTRY_SUBSCRIBE           0x06
#define SOMEIPSD_ENTRY_SUBSCRIBE_ACK       0x07

/* Option Types */
typedef uint8 SomeIpSd_OptionTypeType;
#define SOMEIPSD_OPT_CONFIGURATION         0x01
#define SOMEIPSD_OPT_LOAD_BALANCING        0x02
#define SOMEIPSD_OPT_IPV4_ENDPOINT         0x04
#define SOMEIPSD_OPT_IPV4_MULTICAST        0x14
#define SOMEIPSD_OPT_IPV6_ENDPOINT         0x06
#define SOMEIPSD_OPT_IPV6_MULTICAST        0x16

/* Protocol Types */
typedef uint8 SomeIpSd_ProtocolType;
#define SOMEIPSD_PROTO_TCP                 0x06
#define SOMEIPSD_PROTO_UDP                 0x11

/* Data Types */
typedef uint16 SomeIpSd_InstanceIdType;
typedef uint8  SomeIpSd_MajorVersionType;
typedef uint32 SomeIpSd_MinorVersionType;
typedef uint32 SomeIpSd_TtlType;

/* Entry Header */
typedef struct {
    SomeIpSd_EntryTypeType Type;
    uint8 IndexFirst;
    uint8 IndexSecond;
    uint8 NumOptions1;
    uint8 NumOptions2;
    SomeIp_ServiceIdType ServiceId;
    SomeIpSd_InstanceIdType InstanceId;
    SomeIpSd_MajorVersionType MajorVersion;
    SomeIpSd_TtlType Ttl;
    SomeIpSd_MinorVersionType MinorVersion;
} SomeIpSd_EntryHeaderType;

/* Service Entry */
typedef struct {
    SomeIpSd_EntryHeaderType Header;
    boolean IsOffered;
} SomeIpSd_ServiceEntryType;

/* Event Group Entry */
typedef struct {
    SomeIpSd_EntryHeaderType Header;
    uint16 EventGroupId;
    uint8 Counter;
} SomeIpSd_EventGroupEntryType;

/* IPv4 Endpoint Option */
typedef struct {
    SomeIpSd_OptionTypeType Type;
    uint16 Length;
    SomeIpSd_ProtocolType Protocol;
    uint16 Port;
    uint32 IpAddress;  /* IPv4 in network byte order */
} SomeIpSd_Ipv4EndpointOptionType;

/* SD Message */
typedef struct {
    SomeIp_HeaderType SomeIpHeader;
    uint8 Flags;
    uint32 Reserved;
    SomeIpSd_EntryHeaderType* Entries;
    uint8* Options;
} SomeIpSd_MessageType;

/* Service Information */
typedef struct {
    SomeIp_ServiceIdType ServiceId;
    SomeIpSd_InstanceIdType InstanceId;
    SomeIpSd_MajorVersionType MajorVersion;
    SomeIpSd_MinorVersionType MinorVersion;
    SomeIpSd_TtlType Ttl;
    uint32 IpAddress;
    uint16 Port;
    SomeIpSd_ProtocolType Protocol;
    boolean IsAvailable;
} SomeIpSd_ServiceInfoType;

/* Configuration */
typedef struct {
    const SomeIpSd_ServiceInfoType* Services;
    uint16 NumServices;
    uint32 LocalIpAddress;
    uint16 LocalPort;
    uint32 MulticastIpAddress;
    uint16 MulticastPort;
    boolean DevErrorDetect;
} SomeIpSd_ConfigType;

/* Function Prototypes */
extern void SomeIpSd_Init(const SomeIpSd_ConfigType* ConfigPtr);
extern void SomeIpSd_DeInit(void);

/* Service Offering */
extern Std_ReturnType SomeIpSd_OfferService(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId,
    SomeIpSd_MajorVersionType MajorVersion,
    SomeIpSd_MinorVersionType MinorVersion,
    SomeIpSd_TtlType Ttl
);

extern Std_ReturnType SomeIpSd_StopOfferService(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId
);

/* Service Discovery */
extern Std_ReturnType SomeIpSd_FindService(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType* InstanceIdPtr,
    SomeIpSd_ServiceInfoType* ServiceInfoPtr
);

/* Event Subscription */
extern Std_ReturnType SomeIpSd_SubscribeEventGroup(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId,
    uint16 EventGroupId,
    SomeIpSd_TtlType Ttl
);

extern Std_ReturnType SomeIpSd_UnsubscribeEventGroup(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId,
    uint16 EventGroupId
);

/* Message Handling */
extern void SomeIpSd_RxIndication(const uint8* Data, uint32 Length);
extern void SomeIpSd_MainFunction(void);

/* Callbacks */
extern void SomeIpSd_ServiceAvailableCallback(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId,
    boolean IsAvailable
);

extern void SomeIpSd_EventSubscriptionCallback(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId,
    uint16 EventGroupId,
    boolean IsSubscribed
);

#endif /* SOMEIP_SD_H */
