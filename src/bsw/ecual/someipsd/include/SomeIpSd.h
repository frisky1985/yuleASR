/**
 * @file SomeIpSd.h
 * @brief SOME/IP Service Discovery - AUTOSAR ECUAL Module
 * @version 2.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @implements AUTOSAR_PRS_SOMEIPServiceDiscoveryProtocol.pdf
 */

#ifndef SOMEIPSD_H
#define SOMEIPSD_H

#include "Std_Types.h"
#include "ComStack_Types.h"
#include "ComStack_Types.h"

#define SOMEIPSD_MODULE_ID          0x81U
#define SOMEIPSD_VENDOR_ID          0x0055U
#define SOMEIPSD_PROTOCOL_VERSION   0x01U
#define SOMEIPSD_INTERFACE_VERSION  0x01U

typedef enum {
    SD_ENTRY_FIND_SERVICE = 0x00,
    SD_ENTRY_OFFER_SERVICE = 0x01,
    SD_ENTRY_SUBSCRIBE_EVENTGROUP = 0x06,
    SD_ENTRY_SUBSCRIBE_ACK = 0x07
} SomeIpSd_EntryTypeType;

typedef struct {
    SomeIpSd_EntryTypeType Type;
    uint16 ServiceId;
    uint16 InstanceId;
    uint8 MajorVersion;
    uint32 MinorVersion;
    uint32 TTL;
} SomeIpSd_EntryType;

typedef enum {
    SD_STATE_DOWN = 0,
    SD_STATE_AVAILABLE,
    SD_STATE_NOT_AVAILABLE
} SomeIpSd_ServiceStateType;

typedef enum {
    SD_SUBSCRIPTION_NOT_REQUESTED = 0,
    SD_SUBSCRIPTION_PENDING,
    SD_SUBSCRIPTION_ACKNOWLEDGED,
    SD_SUBSCRIPTION_REJECTED
} SomeIpSd_SubscriptionStateType;

typedef struct {
    uint16 ServiceId;
    uint16 InstanceId;
    uint32 TTL;
    boolean IsServer;
    uint16 EndpointTcp;
    uint16 EndpointUdp;
    uint8  MajorVersion;
    uint32 MinorVersion;
} SomeIpSd_ServiceConfigType;

typedef struct {
    uint8 NumServices;
    const SomeIpSd_ServiceConfigType* Services;
} SomeIpSd_ConfigType;

/** @req SWS_SomeIpSd_00001 */
void SomeIpSd_Init(const void* ConfigPtr);
/** @req SWS_SomeIpSd_00002 */
void SomeIpSd_DeInit(void);
/** @req SWS_SomeIpSd_00004 */
void SomeIpSd_MainFunction(void);
/** @req SWS_SomeIpSd_00005 */
Std_ReturnType SomeIpSd_FindService(uint16 ServiceId, uint16 InstanceId);
/** @req SWS_SomeIpSd_00006 */
Std_ReturnType SomeIpSd_OfferService(uint16 ServiceId, uint16 InstanceId);
/** @req SWS_SomeIpSd_00007 */
Std_ReturnType SomeIpSd_StopOffer(uint16 ServiceId, uint16 InstanceId);
/** @req SWS_SomeIpSd_00008 */
Std_ReturnType SomeIpSd_SubscribeEventGroup(uint16 ServiceId, uint16 EventGroupId);
SomeIpSd_ServiceStateType SomeIpSd_GetServiceState(uint16 ServiceId, uint16 InstanceId);
/** @req SWS_SomeIpSd_00009 */
void SomeIpSd_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
/** @req SWS_SomeIpSd_00003 */
void SomeIpSd_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* SOMEIPSD_H */