/**
 * @file SomeIpSd.h
 * @brief SOME/IP Service Discovery Module
 * @version 1.0.0
 */

#ifndef SOMEIPSD_H
#define SOMEIPSD_H

#include "Std_Types.h"
#include "SoAd.h"

#define SOMEIPSD_MODULE_ID          0x81U
#define SOMEIPSD_VENDOR_ID          0x0001U

/* Error Codes */
#define SOMEIPSD_E_NO_ERROR         0x00U
#define SOMEIPSD_E_PARAM_POINTER    0x01U
#define SOMEIPSD_E_UNINIT           0x02U

/* SD Message Types */
#define SOMEIPSD_FIND_SERVICE       0x00U
#define SOMEIPSD_OFFER_SERVICE      0x01U
#define SOMEIPSD_STOP_OFFER_SERVICE 0x02U
#define SOMEIPSD_SUBSCRIBE_EVENTGROUP 0x06U
#define SOMEIPSD_SUBSCRIBE_ACK      0x07U
#define SOMEIPSD_SUBSCRIBE_NACK     0x08U

/* SD Entry Types */
typedef enum {
    SD_ENTRY_FIND_SERVICE = 0x00,
    SD_ENTRY_OFFER_SERVICE = 0x01,
    SD_ENTRY_SUBSCRIBE_EVENTGROUP = 0x06,
    SD_ENTRY_SUBSCRIBE_ACK = 0x07
} SomeIpSd_EntryTypeType;

/* SD Entry Structure */
typedef struct {
    SomeIpSd_EntryTypeType Type;
    uint16 ServiceId;
    uint16 InstanceId;
    uint8 MajorVersion;
    uint32 MinorVersion;
    uint32 TTL;
} SomeIpSd_EntryType;

/* Service Discovery State */
typedef enum {
    SD_STATE_DOWN = 0,
    SD_STATE_AVAILABLE,
    SD_STATE_NOT_AVAILABLE
} SomeIpSd_ServiceStateType;

/* Subscription State */
typedef enum {
    SD_SUBSCRIPTION_NOT_REQUESTED = 0,
    SD_SUBSCRIPTION_PENDING,
    SD_SUBSCRIPTION_ACKNOWLEDGED,
    SD_SUBSCRIPTION_REJECTED
} SomeIpSd_SubscriptionStateType;

/* Configuration */
typedef struct {
    uint16 ServiceId;
    uint16 InstanceId;
    uint32 TTL;
    boolean IsServer;
    uint16 EndpointTcp;
    uint16 EndpointUdp;
} SomeIpSd_ServiceConfigType;

/* Functions */
void SomeIpSd_Init(const void* ConfigPtr);
void SomeIpSd_DeInit(void);
void SomeIpSd_MainFunction(void);
Std_ReturnType SomeIpSd_FindService(uint16 ServiceId, uint16 InstanceId);
Std_ReturnType SomeIpSd_OfferService(uint16 ServiceId, uint16 InstanceId);
Std_ReturnType SomeIpSd_StopOffer(uint16 ServiceId, uint16 InstanceId);
Std_ReturnType SomeIpSd_SubscribeEventGroup(uint16 ServiceId, uint16 EventGroupId);
void SomeIpSd_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

#endif
