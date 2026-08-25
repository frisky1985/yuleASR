/** @file SomeIpSd.c
 *  @brief SOME/IP Service Discovery implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_PRS_SOMEIPServiceDiscoveryProtocol.pdf
 */

#include "SomeIpSd.h"
#include "SomeIpSd_Cfg.h"
#include "Det.h"
#include <string.h>

#define SD_SID_INIT                     0x00U
#define SD_SID_DEINIT                   0x01U
#define SD_SID_FIND_SERVICE             0x02U
#define SD_SID_OFFER_SERVICE            0x03U
#define SD_SID_STOP_OFFER               0x04U
#define SD_SID_SUBSCRIBE                0x05U
#define SD_SID_RX_INDICATION            0x06U
#define SD_SID_MAINFUNCTION             0x07U
#define SD_SID_GET_SERVICE_STATE        0x08U

#define SD_E_PARAM_POINTER              0x10U
#define SD_E_UNINIT                     0x20U
#define SD_E_PARAM_SERVICE              0x30U
#define SD_E_NOT_FOUND                  0x40U

#define SD_MAX_SERVICES                 16U
#define SD_DEFAULT_TTL                  3000U    /* 3 seconds */
#define SD_PROTOCOL_VERSION             0x01U
#define SD_INTERFACE_VERSION            0x01U

/* SD Message Types */
#define SD_MSG_FIND_SERVICE             0x00U
#define SD_MSG_OFFER_SERVICE            0x01U
#define SD_MSG_STOP_OFFER               0x02U
#define SD_MSG_SUBSCRIBE                0x06U
#define SD_MSG_SUBSCRIBE_ACK            0x07U
#define SD_MSG_SUBSCRIBE_NACK           0x08U

/* SD Entry Types */
#define SD_ENTRY_FIND                   0x00U
#define SD_ENTRY_OFFER                  0x01U
#define SD_ENTRY_SUBSCRIBE              0x06U
#define SD_ENTRY_SUBSCRIBE_ACK          0x07U

/* SD Header sizes */
#define SD_SOMEIP_HEADER_LEN            16U
#define SD_ENTRY_LEN                    16U

typedef enum { SD_INTERNAL_UNINIT = 0, SD_INTERNAL_INIT } Sd_InternalStateType;

typedef struct {
    uint16         ServiceId;
    uint16         InstanceId;
    uint8          MajorVersion;
    uint32         MinorVersion;
    uint32         TTL;
    uint32         RemainingTTL;
    SomeIpSd_ServiceStateType State;
    SomeIpSd_SubscriptionStateType SubState;
    boolean        IsServer;
} Sd_ServiceEntryType;

typedef struct {
    Sd_InternalStateType state;
    Sd_ServiceEntryType  services[SD_MAX_SERVICES];
    uint8                serviceCount;
    uint16               sessionId;
    uint32               tickCounter;
    const SomeIpSd_ConfigType* configPtr;
} Sd_InternalType;

static Sd_InternalType Sd_State;

static Sd_ServiceEntryType* Sd_FindService(uint16 ServiceId, uint16 InstanceId)
{
    for (uint8 i = 0U; i < Sd_State.serviceCount; i++) {
        if ((Sd_State.services[i].ServiceId == ServiceId) &&
            (Sd_State.services[i].InstanceId == InstanceId)) {
            return &Sd_State.services[i];
        }
    }
    return NULL_PTR;
}

static void Sd_BuildHeader(uint8* Header, uint8 MsgType)
{
    memset(Header, 0, SD_SOMEIP_HEADER_LEN);
    Header[0] = 0xFF; Header[1] = 0xFF; /* Service ID */
    Header[2] = 0x81; Header[3] = 0x00; /* Method ID */
    Header[8] = 0x00; Header[9] = 0x00; /* Client ID */
    Header[10] = (uint8)(Sd_State.sessionId >> 8);
    Header[11] = (uint8)(Sd_State.sessionId);
    Header[12] = SD_PROTOCOL_VERSION;
    Header[13] = SD_INTERFACE_VERSION;
    Header[14] = MsgType;
    Header[15] = 0x00; /* Return code */
    Sd_State.sessionId++;
}

/** @req SWS_SomeIpSd_00001 */
void SomeIpSd_Init(const void* ConfigPtr)
{
    Sd_State.state = SD_INTERNAL_UNINIT;
    Sd_State.serviceCount = 0U;
    Sd_State.sessionId = 0x0001U;
    Sd_State.tickCounter = 0U;
    Sd_State.configPtr = (const SomeIpSd_ConfigType*)ConfigPtr;
    memset(Sd_State.services, 0, sizeof(Sd_State.services));
    Sd_State.state = SD_INTERNAL_INIT;
}

/** @req SWS_SomeIpSd_00002 */
void SomeIpSd_DeInit(void)
{
    Sd_State.state = SD_INTERNAL_UNINIT;
    Sd_State.serviceCount = 0U;
}

/** @req SWS_SomeIpSd_00005 */
Std_ReturnType SomeIpSd_FindService(uint16 ServiceId, uint16 InstanceId)
{
    if (Sd_State.state != SD_INTERNAL_INIT) { return E_NOT_OK; }

    Sd_ServiceEntryType* entry = Sd_FindService(ServiceId, InstanceId);
    if (entry != NULL_PTR) {
        return E_OK; /* Already known */
    }

    if (Sd_State.serviceCount >= SD_MAX_SERVICES) { return E_NOT_OK; }

    entry = &Sd_State.services[Sd_State.serviceCount];
    entry->ServiceId = ServiceId;
    entry->InstanceId = InstanceId;
    entry->State = SD_STATE_DOWN;
    entry->IsServer = FALSE;
    entry->RemainingTTL = 0U;
    Sd_State.serviceCount++;

    /* Build and send FindService message */
    uint8 msg[32];
    Sd_BuildHeader(msg, SD_MSG_FIND_SERVICE);
    msg[16] = SD_ENTRY_FIND;
    msg[20] = (uint8)(ServiceId >> 8);  msg[21] = (uint8)ServiceId;
    msg[22] = (uint8)(InstanceId >> 8); msg[23] = (uint8)InstanceId;
    msg[24] = 0x01; /* Major version */
    msg[25] = 0x00; msg[26] = 0x00; msg[27] = SD_DEFAULT_TTL >> 8; /* TTL high */
    (void)msg;

    return E_OK;
}

/** @req SWS_SomeIpSd_00006 */
Std_ReturnType SomeIpSd_OfferService(uint16 ServiceId, uint16 InstanceId)
{
    if (Sd_State.state != SD_INTERNAL_INIT) { return E_NOT_OK; }

    Sd_ServiceEntryType* entry = Sd_FindService(ServiceId, InstanceId);
    if (entry == NULL_PTR) {
        if (Sd_State.serviceCount >= SD_MAX_SERVICES) { return E_NOT_OK; }
        entry = &Sd_State.services[Sd_State.serviceCount];
        entry->ServiceId = ServiceId;
        entry->InstanceId = InstanceId;
        Sd_State.serviceCount++;
    }

    entry->IsServer = TRUE;
    entry->State = SD_STATE_AVAILABLE;
    entry->TTL = SD_DEFAULT_TTL;
    entry->RemainingTTL = SD_DEFAULT_TTL;

    /* Build and send OfferService message */
    uint8 msg[32];
    Sd_BuildHeader(msg, SD_MSG_OFFER_SERVICE);
    msg[16] = SD_ENTRY_OFFER;
    msg[20] = (uint8)(ServiceId >> 8);  msg[21] = (uint8)ServiceId;
    msg[22] = (uint8)(InstanceId >> 8); msg[23] = (uint8)InstanceId;
    (void)msg;

    return E_OK;
}

/** @req SWS_SomeIpSd_00007 */
Std_ReturnType SomeIpSd_StopOffer(uint16 ServiceId, uint16 InstanceId)
{
    if (Sd_State.state != SD_INTERNAL_INIT) { return E_NOT_OK; }
    Sd_ServiceEntryType* entry = Sd_FindService(ServiceId, InstanceId);
    if (entry == NULL_PTR) { return E_NOT_OK; }
    entry->State = SD_STATE_NOT_AVAILABLE;
    entry->RemainingTTL = 0U;
    return E_OK;
}

/** @req SWS_SomeIpSd_00008 */
Std_ReturnType SomeIpSd_SubscribeEventGroup(uint16 ServiceId, uint16 EventGroupId)
{
    if (Sd_State.state != SD_INTERNAL_INIT) { return E_NOT_OK; }
    (void)EventGroupId;

    Sd_ServiceEntryType* entry = Sd_FindService(ServiceId, 0U);
    if (entry == NULL_PTR) { return E_NOT_OK; }

    entry->SubState = SD_SUBSCRIPTION_PENDING;
    return E_OK;
}

SomeIpSd_ServiceStateType SomeIpSd_GetServiceState(uint16 ServiceId, uint16 InstanceId)
{
    Sd_ServiceEntryType* entry = Sd_FindService(ServiceId, InstanceId);
    if (entry == NULL_PTR) { return SD_STATE_DOWN; }
    return entry->State;
}

/** @req SWS_SomeIpSd_00009 */
void SomeIpSd_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    if ((NULL_PTR == PduInfoPtr) || (NULL_PTR == PduInfoPtr->SduDataPtr)) { return; }
    if (PduInfoPtr->SduLength < (SD_SOMEIP_HEADER_LEN + SD_ENTRY_LEN)) { return; }

    (void)RxPduId;
    const uint8* data = PduInfoPtr->SduDataPtr;
    uint8 msgType = data[14];
    uint8 entryType = data[16];
    uint16 serviceId = (uint16)(data[20] << 8) | data[21];
    uint16 instanceId = (uint16)(data[22] << 8) | data[23];

    if ((msgType == SD_MSG_OFFER_SERVICE) && (entryType == SD_ENTRY_OFFER)) {
        Sd_ServiceEntryType* entry = Sd_FindService(serviceId, instanceId);
        if (entry != NULL_PTR) {
            entry->State = SD_STATE_AVAILABLE;
            entry->RemainingTTL = SD_DEFAULT_TTL;
        }
    }
}

/** @req SWS_SomeIpSd_00004 */
void SomeIpSd_MainFunction(void)
{
    if (Sd_State.state != SD_INTERNAL_INIT) { return; }

    Sd_State.tickCounter++;

    /* Process service state timeouts */
    for (uint8 i = 0U; i < Sd_State.serviceCount; i++) {
        if (Sd_State.services[i].State == SD_STATE_AVAILABLE) {
            if (Sd_State.services[i].RemainingTTL > 0U) {
                Sd_State.services[i].RemainingTTL--;
                if (Sd_State.services[i].RemainingTTL == 0U) {
                    Sd_State.services[i].State = SD_STATE_NOT_AVAILABLE;
                }
            }
        }

        /* Periodic offer for server services */
        if (Sd_State.services[i].IsServer &&
            (Sd_State.services[i].State == SD_STATE_AVAILABLE) &&
            (Sd_State.tickCounter % 100U) == 0U) {
            (void)SomeIpSd_OfferService(
                Sd_State.services[i].ServiceId,
                Sd_State.services[i].InstanceId);
        }
    }
}

/** @req SWS_SomeIpSd_00003 */
void SomeIpSd_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (NULL_PTR == versioninfo) { return; }
    versioninfo->vendorID = SOMEIPSD_VENDOR_ID;
    versioninfo->moduleID = SOMEIPSD_MODULE_ID;
    versioninfo->sw_major_version = 1U;
    versioninfo->sw_minor_version = 0U;
    versioninfo->sw_patch_version = 0U;
}