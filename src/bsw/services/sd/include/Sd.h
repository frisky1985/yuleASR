/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : SoAd, SomeIp, Det
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Sd.h
 * @brief Service Discovery — AUTOSAR Service Layer
 * @version 1.0.0
 *
 * AUTOSAR Service Discovery module for SOME/IP-based service
 * offer/find/subscribe workflows.  Maintains a local service registry
 * and handles SD message serialization/deserialization.
 *
 * @implements AUTOSAR_SWS_ServiceDiscovery.pdf
 */

#ifndef SD_H
#define SD_H

#include "Std_Types.h"
#include "Sd_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define SD_VENDOR_ID                            (0x0001U)
#define SD_MODULE_ID                            (0x71U)
#define SD_INSTANCE_ID                          (0x00U)

#define SD_AR_RELEASE_MAJOR_VERSION             (0x04U)
#define SD_AR_RELEASE_MINOR_VERSION             (0x04U)
#define SD_AR_RELEASE_REVISION_VERSION          (0x00U)
#define SD_SW_MAJOR_VERSION                     (0x01U)
#define SD_SW_MINOR_VERSION                     (0x00U)
#define SD_SW_PATCH_VERSION                     (0x00U)

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define SD_SID_INIT                             (0x01U)
#define SD_SID_DEINIT                           (0x02U)
#define SD_SID_GETVERSIONINFO                   (0x03U)
#define SD_SID_FINDSERVICE                      (0x10U)
#define SD_SID_OFFERSERVICE                     (0x11U)
#define SD_SID_STOPSERVICE                      (0x12U)
#define SD_SID_SUBSCRIBEEVENTGROUP              (0x13U)
#define SD_SID_UNSUBSCRIBEEVENTGROUP            (0x14U)
#define SD_SID_SETEVENTSTATUS                   (0x15U)
#define SD_SID_GETSERVICEURL                    (0x16U)
#define SD_SID_MAINFUNCTION                     (0x17U)
#define SD_SID_HANDLEMSG                        (0x18U)

/*==================================================================================================
 *                                    DET ERROR CODES
 *==================================================================================================*/
#define SD_E_PARAM_POINTER                      (0x01U)
#define SD_E_PARAM_CONFIG                       (0x02U)
#define SD_E_UNINIT                             (0x03U)
#define SD_E_ALREADY_INITIALIZED                (0x04U)
#define SD_E_INVALID_SERVICEID                  (0x05U)
#define SD_E_INVALID_INSTANCEID                 (0x06U)
#define SD_E_INVALID_EVENTGROUPID               (0x07U)
#define SD_E_SERVICE_NOT_FOUND                  (0x08U)
#define SD_E_SERVICE_ALREADY_OFFERED            (0x09U)
#define SD_E_NO_FREE_ENTRY                      (0x0AU)
#define SD_E_NOT_SUPPORTED                      (0x0BU)

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/** Service ID (16-bit SOME/IP Service ID) */
typedef uint16 Sd_ServiceIdType;

/** Instance ID (16-bit) */
typedef uint16 Sd_InstanceIdType;

/** Event group ID (16-bit) */
typedef uint16 Sd_EventGroupIdType;

/** Major version */
typedef uint8  Sd_MajorVersionType;

/** Minor version (32-bit) */
typedef uint32 Sd_MinorVersionType;

/** Time-To-Live (seconds) */
typedef uint32 Sd_TtlType;

/** Service entry type */
typedef uint8 Sd_EntryType;
#define SD_ENTRY_FIND_SERVICE                   (0x00U)
#define SD_ENTRY_OFFER_SERVICE                  (0x01U)
#define SD_ENTRY_SUBSCRIBE_EVENTGROUP           (0x06U)
#define SD_ENTRY_SUBSCRIBE_EVENTGROUP_ACK       (0x07U)

/** Protocol type */
typedef uint8 Sd_ProtocolType;
#define SD_PROTO_TCP                            (0x06U)
#define SD_PROTO_UDP                            (0x11U)

/** Service status */
typedef uint8 Sd_ServiceStatusType;
#define SD_SERVICE_STATUS_NOT_OFFERED           (0x00U)
#define SD_SERVICE_STATUS_OFFERED               (0x01U)
#define SD_SERVICE_STATUS_AVAILABLE             (0x02U)

/** Subscriber status */
typedef uint8 Sd_SubscriberStatusType;
#define SD_SUBSCRIBER_NOT_SUBSCRIBED            (0x00U)
#define SD_SUBSCRIBER_SUBSCRIBED                (0x01U)
#define SD_SUBSCRIBER_SUBSCRIBE_PENDING         (0x02U)

/** Event group status */
typedef uint8 Sd_EventGroupStatusType;
#define SD_EVENTGROUP_NOT_READY                 (0x00U)
#define SD_EVENTGROUP_READY                     (0x01U)

/** Return codes for SD subscribe */
typedef uint8 Sd_ReturnCodeType;
#define SD_RET_OK                               (0x00U)
#define SD_RET_E_NOT_OK                         (0x01U)
#define SD_RET_E_UNKNOWN_SERVICE                (0x02U)
#define SD_RET_E_UNKNOWN_INSTANCE               (0x03U)
#define SD_RET_E_REJECTED                       (0x04U)

/** IPv4 endpoint for SD options */
typedef struct {
    uint32              Addr;
    uint16              Port;
    Sd_ProtocolType     Protocol;
} Sd_Ipv4EndpointType;

/** Service instance identifier */
typedef struct {
    Sd_ServiceIdType    ServiceId;
    Sd_InstanceIdType   InstanceId;
} Sd_ServiceInstanceType;

/** Service offer entry */
typedef struct {
    Sd_ServiceInstanceType  Service;
    Sd_MajorVersionType     MajorVersion;
    Sd_MinorVersionType     MinorVersion;
    Sd_TtlType              Ttl;
    Sd_Ipv4EndpointType     Endpoint;
    Sd_ServiceStatusType    Status;
    uint32                  RemainingLifetimeMs;
} Sd_ServiceEntryType;

/** Event group entry */
typedef struct {
    Sd_ServiceInstanceType  Service;
    Sd_EventGroupIdType     EventGroupId;
    Sd_SubscriberStatusType SubscriberStatus;
    Sd_EventGroupStatusType EventStatus;
} Sd_EventGroupEntryType;

/** Global configuration */
typedef struct {
    uint8               MaxServices;
    uint8               MaxSubscriptions;
    uint32              OfferCycleTimeMs;
    uint32              FindCycleTimeMs;
    uint32              TtlDefault;
    boolean             DevErrorDetect;
    boolean             VersionInfoApi;
} Sd_ConfigType;

/*==================================================================================================
 *                                    FUNCTION DECLARATIONS
 *==================================================================================================*/

/** @brief Initialise the Service Discovery module */
void Sd_Init(const Sd_ConfigType* ConfigPtr);

/** @brief De-initialise the Service Discovery module */
void Sd_DeInit(void);

/** @brief Get version information */
#if (SD_VERSION_INFO_API == STD_ON)
void Sd_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Find an offered service on the network.
 * @param ServiceId  SOME/IP Service ID
 * @param InstanceId Instance ID (0xFFFF = any)
 * @param[out] Endpoint Endpoint of found service
 * @return E_OK if found, E_NOT_OK otherwise
 */
Std_ReturnType Sd_FindService(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                              Sd_Ipv4EndpointType* Endpoint);

/**
 * @brief Start offering a service.
 * @param ServiceId    Service ID
 * @param InstanceId   Instance ID
 * @param MajorVersion Major version
 * @param MinorVersion Minor version
 * @param Endpoint     Local endpoint for the service
 * @return E_OK on success
 */
Std_ReturnType Sd_OfferService(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                               Sd_MajorVersionType MajorVersion, Sd_MinorVersionType MinorVersion,
                               const Sd_Ipv4EndpointType* Endpoint);

/**
 * @brief Stop offering a service.
 */
Std_ReturnType Sd_StopService(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId);

/**
 * @brief Subscribe to an event group.
 * @param ServiceId    Service ID
 * @param InstanceId   Instance ID
 * @param EventGroupId Event group ID
 * @return E_OK on success
 */
Std_ReturnType Sd_SubscribeEventGroup(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                                      Sd_EventGroupIdType EventGroupId);

/**
 * @brief Unsubscribe from an event group.
 */
Std_ReturnType Sd_UnsubscribeEventGroup(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                                        Sd_EventGroupIdType EventGroupId);

/**
 * @brief Set event status for an event group (ready / not ready).
 */
Std_ReturnType Sd_SetEventStatus(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                                 Sd_EventGroupIdType EventGroupId,
                                 Sd_EventGroupStatusType Status);

/**
 * @brief Main function — periodic SD message handling.
 */
void Sd_MainFunction(void);

/**
 * @brief Handle an incoming SD message.
 * @param Data   Pointer to SD message payload
 * @param Length Message length
 */
Std_ReturnType Sd_HandleMessage(const uint8* Data, uint16 Length);

#endif /* SD_H */
