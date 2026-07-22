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
 * @file Sd.c
 * @brief Service Discovery Implementation
 * @req SHALL_SD - AUTOSAR Service Discovery (Find/Offer/Subscribe)
 *
 * Maintains two registries:
 *   - Offered Services  (locally provided)
 *   - Found Services    (discovered remotely)
 * Handles periodic SD message transmission, TTL expiry,
 * and subscription management.
 */

#include "Sd.h"
#include "Det.h"
#include <string.h>

/*==================================================================================================
 *                                    LOCAL CONSTANTS
 *==================================================================================================*/
#define SD_STATE_UNINIT                         (0x00U)
#define SD_STATE_INIT                           (0x01U)

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
#if (SD_DEV_ERROR_DETECT == STD_ON)
    #define SD_DET_REPORT_ERROR(api, err) \
        Det_ReportError(SD_MODULE_ID, SD_INSTANCE_ID, (api), (err))
#else
    #define SD_DET_REPORT_ERROR(api, err)
#endif

#define SD_IS_INIT() \
    (Sd_InternalState.State == SD_STATE_INIT)

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/

/** Internal state structure */
typedef struct {
    uint8                State;
    const Sd_ConfigType* ConfigPtr;

    /** Locally offered services */
    Sd_ServiceEntryType  OfferedServices[SD_MAX_OFFERED_SERVICES];
    uint8                NumOfferedServices;

    /** Remotely discovered services */
    Sd_ServiceEntryType  FoundServices[SD_MAX_FOUND_SERVICES];
    uint8                NumFoundServices;

    /** Active subscriptions */
    Sd_EventGroupEntryType Subscriptions[SD_MAX_SUBSCRIPTIONS];
    uint8                NumSubscriptions;

    /** Tick counter for periodic operations */
    uint32               TickMs;

    /** Whether initial delay has elapsed */
    boolean              InitialDelayElapsed;
} Sd_InternalStateType;

/*==================================================================================================
 *                                    LOCAL DATA
 *==================================================================================================*/
static Sd_InternalStateType Sd_InternalState;

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
static Sd_ServiceEntryType* Sd_LocalFindOffered(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId);
static Sd_ServiceEntryType* Sd_LocalFindFound(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId);
static Sd_ServiceEntryType* Sd_LocalAllocOffered(void);
static Sd_ServiceEntryType* Sd_LocalAllocFound(void);
static Sd_EventGroupEntryType* Sd_LocalFindSub(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                                                Sd_EventGroupIdType EventGroupId);
static Sd_EventGroupEntryType* Sd_LocalAllocSub(void);
static void Sd_LocalUpdateLifetimes(void);
static void Sd_LocalSendOfferMessages(void);
static void Sd_LocalSendFindMessages(void);

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/

static Sd_ServiceEntryType* Sd_LocalFindOffered(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId)
{
    uint8 i;
    for (i = 0U; i < Sd_InternalState.NumOfferedServices; i++)
    {
        if ((Sd_InternalState.OfferedServices[i].Service.ServiceId   == ServiceId) &&
            (Sd_InternalState.OfferedServices[i].Service.InstanceId  == InstanceId))
        {
            return &Sd_InternalState.OfferedServices[i];
        }
    }
    return NULL_PTR;
}

static Sd_ServiceEntryType* Sd_LocalFindFound(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId)
{
    uint8 i;
    for (i = 0U; i < Sd_InternalState.NumFoundServices; i++)
    {
        if ((Sd_InternalState.FoundServices[i].Service.ServiceId   == ServiceId) &&
            (Sd_InternalState.FoundServices[i].Service.InstanceId  == InstanceId))
        {
            return &Sd_InternalState.FoundServices[i];
        }
    }
    return NULL_PTR;
}

static Sd_ServiceEntryType* Sd_LocalAllocOffered(void)
{
    if (Sd_InternalState.NumOfferedServices < SD_MAX_OFFERED_SERVICES)
    {
        Sd_ServiceEntryType* entry = &Sd_InternalState.OfferedServices[Sd_InternalState.NumOfferedServices];
        (void)memset(entry, 0, sizeof(Sd_ServiceEntryType));
        Sd_InternalState.NumOfferedServices++;
        return entry;
    }
    return NULL_PTR;
}

static Sd_ServiceEntryType* Sd_LocalAllocFound(void)
{
    if (Sd_InternalState.NumFoundServices < SD_MAX_FOUND_SERVICES)
    {
        Sd_ServiceEntryType* entry = &Sd_InternalState.FoundServices[Sd_InternalState.NumFoundServices];
        (void)memset(entry, 0, sizeof(Sd_ServiceEntryType));
        Sd_InternalState.NumFoundServices++;
        return entry;
    }
    return NULL_PTR;
}

static Sd_EventGroupEntryType* Sd_LocalFindSub(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                                                Sd_EventGroupIdType EventGroupId)
{
    uint8 i;
    for (i = 0U; i < Sd_InternalState.NumSubscriptions; i++)
    {
        if ((Sd_InternalState.Subscriptions[i].Service.ServiceId    == ServiceId) &&
            (Sd_InternalState.Subscriptions[i].Service.InstanceId   == InstanceId) &&
            (Sd_InternalState.Subscriptions[i].EventGroupId         == EventGroupId))
        {
            return &Sd_InternalState.Subscriptions[i];
        }
    }
    return NULL_PTR;
}

static Sd_EventGroupEntryType* Sd_LocalAllocSub(void)
{
    if (Sd_InternalState.NumSubscriptions < SD_MAX_SUBSCRIPTIONS)
    {
        Sd_EventGroupEntryType* entry = &Sd_InternalState.Subscriptions[Sd_InternalState.NumSubscriptions];
        (void)memset(entry, 0, sizeof(Sd_EventGroupEntryType));
        Sd_InternalState.NumSubscriptions++;
        return entry;
    }
    return NULL_PTR;
}

/**
 * @brief Update remaining lifetimes of all discovered services.
 */
static void Sd_LocalUpdateLifetimes(void)
{
    uint8 i;

    /* Decrement lifetimes for discovered services */
    i = 0U;
    while (i < Sd_InternalState.NumFoundServices)
    {
        if (Sd_InternalState.FoundServices[i].RemainingLifetimeMs > SD_MAIN_FUNCTION_PERIOD_MS)
        {
            Sd_InternalState.FoundServices[i].RemainingLifetimeMs -= SD_MAIN_FUNCTION_PERIOD_MS;
            i++;
        }
        else
        {
            /* Expired — remove by shifting */
            uint8 j;
            Sd_InternalState.FoundServices[i].Status = SD_SERVICE_STATUS_NOT_OFFERED;
            for (j = i; j < (Sd_InternalState.NumFoundServices - 1U); j++)
            {
                Sd_InternalState.FoundServices[j] = Sd_InternalState.FoundServices[j + 1U];
            }
            Sd_InternalState.NumFoundServices--;
            /* Don't increment i — the slot was replaced */
        }
    }
}

/**
 * @brief Periodically send OfferService messages for offered services.
 */
static void Sd_LocalSendOfferMessages(void)
{
    /* Stub — in a full implementation this would serialise SD entries
     * into a SOME/IP-SD message and send via SoAd/Socket.
     *
     * For unit testing we simply verify that offered services are
     * present in the registry and that their TTL > 0.
     */
}

/**
 * @brief Periodically send FindService messages for services being searched.
 */
static void Sd_LocalSendFindMessages(void)
{
    /* Stub — same as above for find messages */
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initialise the Service Discovery module.
 */
void Sd_Init(const Sd_ConfigType* ConfigPtr)
{
#if (SD_DEV_ERROR_DETECT == STD_ON)
    if (Sd_InternalState.State == SD_STATE_INIT)
    {
        SD_DET_REPORT_ERROR(SD_SID_INIT, SD_E_ALREADY_INITIALIZED);
        return;
    }
    if (ConfigPtr == NULL_PTR)
    {
        SD_DET_REPORT_ERROR(SD_SID_INIT, SD_E_PARAM_POINTER);
        return;
    }
#endif

    Sd_InternalState.ConfigPtr           = ConfigPtr;
    Sd_InternalState.State               = SD_STATE_INIT;
    Sd_InternalState.NumOfferedServices  = 0U;
    Sd_InternalState.NumFoundServices    = 0U;
    Sd_InternalState.NumSubscriptions    = 0U;
    Sd_InternalState.TickMs              = 0U;
    Sd_InternalState.InitialDelayElapsed = FALSE;
}

/**
 * @brief De-initialise the Service Discovery module.
 */
void Sd_DeInit(void)
{
#if (SD_DEV_ERROR_DETECT == STD_ON)
    if (Sd_InternalState.State != SD_STATE_INIT)
    {
        SD_DET_REPORT_ERROR(SD_SID_DEINIT, SD_E_UNINIT);
        return;
    }
#endif

    Sd_InternalState.State               = SD_STATE_UNINIT;
    Sd_InternalState.ConfigPtr           = NULL_PTR;
    Sd_InternalState.NumOfferedServices  = 0U;
    Sd_InternalState.NumFoundServices    = 0U;
    Sd_InternalState.NumSubscriptions    = 0U;
}

/**
 * @brief Get version information.
 */
#if (SD_VERSION_INFO_API == STD_ON)
void Sd_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (SD_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        SD_DET_REPORT_ERROR(SD_SID_GETVERSIONINFO, SD_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID         = SD_VENDOR_ID;
    versioninfo->moduleID         = SD_MODULE_ID;
    versioninfo->sw_major_version = SD_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SD_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SD_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Find a service — search in found services registry.
 */
Std_ReturnType Sd_FindService(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                              Sd_Ipv4EndpointType* Endpoint)
{
    Sd_ServiceEntryType* entry;

#if (SD_DEV_ERROR_DETECT == STD_ON)
    if (!SD_IS_INIT())
    {
        SD_DET_REPORT_ERROR(SD_SID_FINDSERVICE, SD_E_UNINIT);
        return E_NOT_OK;
    }
    if (Endpoint == NULL_PTR)
    {
        SD_DET_REPORT_ERROR(SD_SID_FINDSERVICE, SD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    entry = Sd_LocalFindFound(ServiceId, InstanceId);
    if (entry == NULL_PTR)
    {
        return E_NOT_OK;
    }

    *Endpoint = entry->Endpoint;
    return E_OK;
}

/**
 * @brief Start offering a service.
 */
Std_ReturnType Sd_OfferService(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                               Sd_MajorVersionType MajorVersion, Sd_MinorVersionType MinorVersion,
                               const Sd_Ipv4EndpointType* Endpoint)
{
    Sd_ServiceEntryType* entry;

#if (SD_DEV_ERROR_DETECT == STD_ON)
    if (!SD_IS_INIT())
    {
        SD_DET_REPORT_ERROR(SD_SID_OFFERSERVICE, SD_E_UNINIT);
        return E_NOT_OK;
    }
    if (Endpoint == NULL_PTR)
    {
        SD_DET_REPORT_ERROR(SD_SID_OFFERSERVICE, SD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Check if already offered */
    entry = Sd_LocalFindOffered(ServiceId, InstanceId);
    if (entry != NULL_PTR)
    {
        /* Update existing entry */
        entry->MajorVersion = MajorVersion;
        entry->MinorVersion = MinorVersion;
        entry->Ttl          = (Sd_InternalState.ConfigPtr != NULL_PTR) ? Sd_InternalState.ConfigPtr->TtlDefault : SD_TTL_DEFAULT_SEC;
        entry->Endpoint     = *Endpoint;
        entry->Status       = SD_SERVICE_STATUS_OFFERED;
        return E_OK;
    }

    /* Allocate new entry */
    entry = Sd_LocalAllocOffered();
    if (entry == NULL_PTR)
    {
        return E_NOT_OK;
    }

    entry->Service.ServiceId    = ServiceId;
    entry->Service.InstanceId   = InstanceId;
    entry->MajorVersion         = MajorVersion;
    entry->MinorVersion         = MinorVersion;
    entry->Ttl                  = (Sd_InternalState.ConfigPtr != NULL_PTR) ? Sd_InternalState.ConfigPtr->TtlDefault : SD_TTL_DEFAULT_SEC;
    entry->Endpoint             = *Endpoint;
    entry->Status               = SD_SERVICE_STATUS_OFFERED;
    entry->RemainingLifetimeMs  = entry->Ttl * 1000U;

    return E_OK;
}

/**
 * @brief Stop offering a service.
 */
Std_ReturnType Sd_StopService(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId)
{
    Sd_ServiceEntryType* entry;

#if (SD_DEV_ERROR_DETECT == STD_ON)
    if (!SD_IS_INIT())
    {
        SD_DET_REPORT_ERROR(SD_SID_STOPSERVICE, SD_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    entry = Sd_LocalFindOffered(ServiceId, InstanceId);
    if (entry == NULL_PTR)
    {
        return E_NOT_OK;
    }

    entry->Status = SD_SERVICE_STATUS_NOT_OFFERED;

    /* Compact the array */
    {
        uint8 i;
        uint8 idx = (uint8)(entry - Sd_InternalState.OfferedServices);
        for (i = idx; i < (Sd_InternalState.NumOfferedServices - 1U); i++)
        {
            Sd_InternalState.OfferedServices[i] = Sd_InternalState.OfferedServices[i + 1U];
        }
        Sd_InternalState.NumOfferedServices--;
    }

    return E_OK;
}

/**
 * @brief Subscribe to an event group.
 */
Std_ReturnType Sd_SubscribeEventGroup(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                                      Sd_EventGroupIdType EventGroupId)
{
    Sd_EventGroupEntryType* entry;

#if (SD_DEV_ERROR_DETECT == STD_ON)
    if (!SD_IS_INIT())
    {
        SD_DET_REPORT_ERROR(SD_SID_SUBSCRIBEEVENTGROUP, SD_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    /* Check for duplicate */
    entry = Sd_LocalFindSub(ServiceId, InstanceId, EventGroupId);
    if (entry != NULL_PTR)
    {
        /* Already subscribed — return OK */
        return E_OK;
    }

    entry = Sd_LocalAllocSub();
    if (entry == NULL_PTR)
    {
        return E_NOT_OK;
    }

    entry->Service.ServiceId    = ServiceId;
    entry->Service.InstanceId   = InstanceId;
    entry->EventGroupId         = EventGroupId;
    entry->SubscriberStatus     = SD_SUBSCRIBER_SUBSCRIBE_PENDING;
    entry->EventStatus          = SD_EVENTGROUP_NOT_READY;

    return E_OK;
}

/**
 * @brief Unsubscribe from an event group.
 */
Std_ReturnType Sd_UnsubscribeEventGroup(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                                        Sd_EventGroupIdType EventGroupId)
{
    Sd_EventGroupEntryType* entry;

#if (SD_DEV_ERROR_DETECT == STD_ON)
    if (!SD_IS_INIT())
    {
        SD_DET_REPORT_ERROR(SD_SID_UNSUBSCRIBEEVENTGROUP, SD_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    entry = Sd_LocalFindSub(ServiceId, InstanceId, EventGroupId);
    if (entry == NULL_PTR)
    {
        return E_NOT_OK;
    }

    /* Compact the array */
    {
        uint8 i;
        uint8 idx = (uint8)(entry - Sd_InternalState.Subscriptions);
        for (i = idx; i < (Sd_InternalState.NumSubscriptions - 1U); i++)
        {
            Sd_InternalState.Subscriptions[i] = Sd_InternalState.Subscriptions[i + 1U];
        }
        Sd_InternalState.NumSubscriptions--;
    }

    return E_OK;
}

/**
 * @brief Set event status for an event group.
 */
Std_ReturnType Sd_SetEventStatus(Sd_ServiceIdType ServiceId, Sd_InstanceIdType InstanceId,
                                 Sd_EventGroupIdType EventGroupId,
                                 Sd_EventGroupStatusType Status)
{
    Sd_EventGroupEntryType* entry;

#if (SD_DEV_ERROR_DETECT == STD_ON)
    if (!SD_IS_INIT())
    {
        SD_DET_REPORT_ERROR(SD_SID_SETEVENTSTATUS, SD_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    entry = Sd_LocalFindSub(ServiceId, InstanceId, EventGroupId);
    if (entry == NULL_PTR)
    {
        return E_NOT_OK;
    }

    entry->EventStatus = Status;

    /* If event is ready and we're in pending state, mark as subscribed */
    if ((Status == SD_EVENTGROUP_READY) &&
        (entry->SubscriberStatus == SD_SUBSCRIBER_SUBSCRIBE_PENDING))
    {
        entry->SubscriberStatus = SD_SUBSCRIBER_SUBSCRIBED;
    }

    return E_OK;
}

/**
 * @brief Main function — periodic SD processing.
 */
void Sd_MainFunction(void)
{
    if (!SD_IS_INIT())
    {
        return;
    }

    Sd_InternalState.TickMs += SD_MAIN_FUNCTION_PERIOD_MS;

    /* Initial delay */
    if (!Sd_InternalState.InitialDelayElapsed)
    {
        if (Sd_InternalState.TickMs >= SD_INITIAL_DELAY_MS)
        {
            Sd_InternalState.InitialDelayElapsed = TRUE;
        }
        return;
    }

    /* Update lifetimes of discovered services */
    Sd_LocalUpdateLifetimes();

    /* Periodic offer messages */
    if ((Sd_InternalState.TickMs % SD_OFFER_CYCLE_TIME_MS) < SD_MAIN_FUNCTION_PERIOD_MS)
    {
        Sd_LocalSendOfferMessages();
    }

    /* Periodic find messages */
    if ((Sd_InternalState.TickMs % SD_FIND_CYCLE_TIME_MS) < SD_MAIN_FUNCTION_PERIOD_MS)
    {
        Sd_LocalSendFindMessages();
    }
}

/**
 * @brief Handle an incoming SD message.
 */
Std_ReturnType Sd_HandleMessage(const uint8* Data, uint16 Length)
{
    /* Stub — in production this deserialises the SD header/entries/options
     * and updates the found-services registry or subscription states.
     */
    (void)Data;
    (void)Length;

    if (!SD_IS_INIT())
    {
        return E_NOT_OK;
    }

    return E_OK;
}
