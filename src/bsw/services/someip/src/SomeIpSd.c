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
 * @file SomeIpSd.c
 * @brief SOME/IP Service Discovery Implementation
 * @req SHALL_SOMEIPSD - SOME/IP Service Discovery Implementation
 */

#include "SomeIpSd.h"
#include "Det.h"

/* Version check */
#if defined(SOMEIPSD_AR_RELEASE_MAJOR_VERSION) && (SOMEIPSD_AR_RELEASE_MAJOR_VERSION != 4u)
#error "SomeIpSd: AR major mismatch"
#endif
#if defined(SOMEIPSD_AR_RELEASE_MINOR_VERSION) && (SOMEIPSD_AR_RELEASE_MINOR_VERSION != 4u)
#error "SomeIpSd: AR minor mismatch"
#endif

/* Internal State */
static boolean SomeIpSd_Initialized = FALSE;
static const SomeIpSd_ConfigType* SomeIpSd_ConfigPtr = NULL_PTR;

/* Service Registry */
#define SOMEIPSD_MAX_SERVICES              32U
static SomeIpSd_ServiceInfoType SomeIpSd_ServiceRegistry[SOMEIPSD_MAX_SERVICES];
static uint16 SomeIpSd_NumRegisteredServices = 0;

/* Version Info */
#define SOMEIPSD_VENDOR_ID                 0x0001
#define SOMEIPSD_INSTANCE_ID               0x00

void SomeIpSd_Init(const SomeIpSd_ConfigType* ConfigPtr)
{
    if (ConfigPtr == NULL_PTR)
    {
#if (STD_ON == SOMEIPSD_DEV_ERROR_DETECT)
        Det_ReportError(SOMEIPSD_MODULE_ID, SOMEIPSD_INSTANCE_ID, SOMEIPSD_INIT_SID, SOMEIPSD_E_INVALID_POINTER);
#endif
        return;
    }

    SomeIpSd_ConfigPtr = ConfigPtr;
    SomeIpSd_NumRegisteredServices = 0;
    
    /* Clear service registry */
    for (uint16 i = 0; (unsigned int)(i) < SOMEIPSD_MAX_SERVICES; i++)
    {
        SomeIpSd_ServiceRegistry[i].IsAvailable = FALSE;
    }
    
    SomeIpSd_Initialized = TRUE;
    
    /* NOTE: Socket creation, multicast group join, and cyclic offer timer
     *       pending network stack integration */
    (void)SomeIpSd_ConfigPtr; /* Config stored for future use */
}

void SomeIpSd_DeInit(void)
{
    if (!SomeIpSd_Initialized)
    {
        return;
    }

    /* Stop all service offers */
    for (uint16 i = 0; (unsigned int)(i) < SOMEIPSD_MAX_SERVICES; i++)
    {
        if ((SomeIpSd_ServiceRegistry[i].IsAvailable) != 0U)
        {
            SomeIpSd_StopOfferService(
                SomeIpSd_ServiceRegistry[i].ServiceId,
                SomeIpSd_ServiceRegistry[i].InstanceId
            );
        }
    }

    SomeIpSd_ConfigPtr = NULL_PTR;
    SomeIpSd_Initialized = FALSE;
}

Std_ReturnType SomeIpSd_OfferService(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId,
    SomeIpSd_MajorVersionType MajorVersion,
    SomeIpSd_MinorVersionType MinorVersion,
    SomeIpSd_TtlType Ttl
)
{
    SomeIpSd_ServiceInfoType* entryPtr = NULL_PTR;
    
    if (!SomeIpSd_Initialized)
    {
        return E_NOT_OK;
    }
    
    /* Check if already offered */
    for (uint16 i = 0; (unsigned int)(i) < SOMEIPSD_MAX_SERVICES; i++)
    {
        if (SomeIpSd_ServiceRegistry[i].IsAvailable &&
            ((SomeIpSd_ServiceRegistry[i].ServiceId == ServiceId)) &&
            (SomeIpSd_ServiceRegistry[i].InstanceId == InstanceId))
        {
#if (STD_ON == SOMEIPSD_DEV_ERROR_DETECT)
            Det_ReportError(SOMEIPSD_MODULE_ID, SOMEIPSD_INSTANCE_ID, SOMEIPSD_OFFERSERVICE_SID, SOMEIPSD_E_ALREADY_OFFERED);
#endif
            return E_NOT_OK;
        }
        
        /* Find free entry */
        if (!SomeIpSd_ServiceRegistry[i].IsAvailable && (entryPtr == NULL_PTR))
        {
            entryPtr = &SomeIpSd_ServiceRegistry[i];
        }
    }
    
    if (entryPtr == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    /* Fill service entry */
    entryPtr->ServiceId = ServiceId;
    entryPtr->InstanceId = InstanceId;
    entryPtr->MajorVersion = MajorVersion;
    entryPtr->MinorVersion = MinorVersion;
    entryPtr->Ttl = Ttl;
    entryPtr->IpAddress = SomeIpSd_ConfigPtr->LocalIpAddress;
    entryPtr->Port = SomeIpSd_ConfigPtr->LocalPort;
    entryPtr->Protocol = SOMEIPSD_PROTO_UDP;
    entryPtr->IsAvailable = TRUE;
    
    SomeIpSd_NumRegisteredServices++;
    
    /* NOTE: Offer Service message transmission pending network stack integration */
    
    return E_OK;
}

Std_ReturnType SomeIpSd_StopOfferService(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId
)
{
    if (!SomeIpSd_Initialized)
    {
        return E_NOT_OK;
    }
    
    /* Find service entry */
    for (uint16 i = 0; (unsigned int)(i) < SOMEIPSD_MAX_SERVICES; i++)
    {
        if (SomeIpSd_ServiceRegistry[i].IsAvailable &&
            ((SomeIpSd_ServiceRegistry[i].ServiceId == ServiceId)) &&
            (SomeIpSd_ServiceRegistry[i].InstanceId == InstanceId))
        {
            /* Send Stop Offer (TTL=0) */
            SomeIpSd_ServiceRegistry[i].Ttl = 0;
            
            /* NOTE: Stop Offer message transmission pending network stack integration */
            
            SomeIpSd_ServiceRegistry[i].IsAvailable = FALSE;
            SomeIpSd_NumRegisteredServices--;
            
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

Std_ReturnType SomeIpSd_FindService(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType* InstanceIdPtr,
    SomeIpSd_ServiceInfoType* ServiceInfoPtr
)
{
    if (!SomeIpSd_Initialized)
    {
        return E_NOT_OK;
    }
    
    /* Search local registry */
    for (uint16 i = 0; (unsigned int)(i) < SOMEIPSD_MAX_SERVICES; i++)
    {
        if (SomeIpSd_ServiceRegistry[i].IsAvailable &&
            (SomeIpSd_ServiceRegistry[i].ServiceId == ServiceId))
        {
            if (InstanceIdPtr != NULL_PTR)
            {
                *InstanceIdPtr = SomeIpSd_ServiceRegistry[i].InstanceId;
            }
            if (ServiceInfoPtr != NULL_PTR)
            {
                *ServiceInfoPtr = SomeIpSd_ServiceRegistry[i];
            }
            return E_OK;
        }
    }
    
    /* Service not found locally, send Find Service */
    /* NOTE: Find Service multicast message pending network stack integration */
    
    return E_NOT_OK;
}

Std_ReturnType SomeIpSd_SubscribeEventGroup(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId,
    uint16 EventGroupId,
    SomeIpSd_TtlType Ttl
)
{
    if (!SomeIpSd_Initialized)
    {
        return E_NOT_OK;
    }
    
    /* NOTE: Subscribe Event Group message pending network stack integration */
    
    return E_OK;
}

Std_ReturnType SomeIpSd_UnsubscribeEventGroup(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId,
    uint16 EventGroupId
)
{
    if (!SomeIpSd_Initialized)
    {
        return E_NOT_OK;
    }
    
    /* NOTE: Stop Subscribe message (TTL=0) pending network stack integration */
    
    return E_OK;
}

void SomeIpSd_RxIndication(const uint8* Data, uint32 Length)
{
    SomeIpSd_MessageType sdMessage;
    
    if (!SomeIpSd_Initialized || (Data == NULL_PTR) || ((unsigned int)(Length) < SOMEIP_HEADER_SIZE))
    {
        return;
    }
    
    /* Parse SOME/IP header */
    if (SomeIp_ParseHeader(Data, &sdMessage.SomeIpHeader) != E_OK)
    {
        return;
    }
    
    /* Verify it's an SD message */
    if (sdMessage.SomeIpHeader.MessageId != (((uint32)SOMEIPSD_SERVICE_ID << 16) | SOMEIPSD_METHOD_ID))
    {
        return;
    }
    
    /* Parse SD message content */
    /* Flags = Data[16] */
/*     sdMessage.Flags = Data[SOMEIP_HEADER_SIZE]; */
    
    /* Reserved = Data[17-20] */
/*     sdMessage.Reserved = ((uint32)Data[17] << 24) | */
                         ((uint32)Data[18] << 16) |
                         ((uint32)Data[19] << 8) |
                         (uint32)Data[20];
    
    /* NOTE: Parse entries and options */
    
    /* Process based on message type */
    /* NOTE: Handle Find Service, Offer Service, Subscribe, etc. */
}

void SomeIpSd_MainFunction(void)
{
    static uint32 cyclicCounter = 0;
    
    if (!SomeIpSd_Initialized)
    {
        return;
    }
    
    cyclicCounter++;
    
    /* Cyclic offer service (every 3 seconds typically) */
    if (cyclicCounter >= 300U)  /* Assuming 10ms task cycle */
    {
        cyclicCounter = 0;
        
        /* Re-offer all active services */
        for (uint16 i = 0; (unsigned int)(i) < SOMEIPSD_MAX_SERVICES; i++)
        {
            if ((SomeIpSd_ServiceRegistry[i].IsAvailable) != 0U)
            {
                /* NOTE: Offer Service message pending network stack integration */
            }
        }
    }
    
    /* NOTE: TTL expiration and subscription management pending network integration */
}

void SomeIpSd_ServiceAvailableCallback(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId,
    boolean IsAvailable
)
{
    /* Application callback - to be overridden */
    (void)ServiceId;
    (void)InstanceId;
    (void)IsAvailable;
}

void SomeIpSd_EventSubscriptionCallback(
    SomeIp_ServiceIdType ServiceId,
    SomeIpSd_InstanceIdType InstanceId,
    uint16 EventGroupId,
    boolean IsSubscribed
)
{
    /* Application callback - to be overridden */
    (void)ServiceId;
    (void)InstanceId;
    (void)EventGroupId;
    (void)IsSubscribed;
}

#if (SOMEIPSD_VERSION_INFO_API == STD_ON)
void SomeIpSd_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (SOMEIPSD_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) {
        Det_ReportError(SOMEIPSD_MODULE_ID, SOMEIPSD_INSTANCE_ID, 0x02U, SOMEIPSD_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID = SOMEIPSD_VENDOR_ID;
    versioninfo->moduleID = SOMEIPSD_MODULE_ID;
    versioninfo->sw_major_version = SOMEIPSD_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SOMEIPSD_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SOMEIPSD_SW_PATCH_VERSION;
}
#endif
