/** @file Srp.c
 *  @brief Stream Reservation Protocol implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements IEEE 802.1Qat / AUTOSAR SRP
 */

#include "Srp.h"
#include "Srp_Cfg.h"
#include "Det.h"
#include <string.h>

/* Version check */
#if defined(SRP_AR_RELEASE_MAJOR_VERSION) && (SRP_AR_RELEASE_MAJOR_VERSION != 4u)
#error "Srp: AR major mismatch"
#endif
#if defined(SRP_AR_RELEASE_MINOR_VERSION) && (SRP_AR_RELEASE_MINOR_VERSION != 4u)
#error "Srp: AR minor mismatch"
#endif

#define SRP_SID_INIT                0x00U
#define SRP_SID_DEINIT              0x01U
#define SRP_SID_REGISTER_TALKER     0x02U
#define SRP_SID_REGISTER_LISTENER   0x03U
#define SRP_SID_DEREGISTER_STREAM   0x04U
#define SRP_SID_RX_INDICATION       0x05U
#define SRP_SID_MAINFUNCTION        0x06U
#define SRP_SID_GET_STREAM_STATUS   0x07U

#define SRP_E_PARAM_POINTER         0x10U
#define SRP_E_UNINIT                0x20U
#define SRP_E_PARAM_STREAM          0x30U
#define SRP_E_NO_RESOURCES          0x40U

#define SRP_MAX_STREAMS             16U

typedef enum { SRP_INTERNAL_UNINIT = 0, SRP_INTERNAL_INIT } Srp_InternalStateType;

typedef struct {
    Srp_StreamIdType          StreamId;
    Srp_ReservationStateType  State;
    Srp_ReservationTypeType   Role;
    uint16                    VlanId;
    uint8                     Priority;
    uint16                    FrameSize;
    uint16                    IntervalFrames;
    uint32                    AccumulatedLatency;
    uint32                    TTL;
} Srp_StreamEntryType;

typedef struct {
    Srp_InternalStateType   state;
    Srp_StreamEntryType     streams[SRP_MAX_STREAMS];
    uint8                   streamCount;
    const Srp_ConfigType*   configPtr;
} Srp_InternalType;

static Srp_InternalType Srp_Internal;
static const uint8 Srp_HdrLen = 8U; /* SRP frame header length */

static Srp_StreamEntryType* Srp_FindStream(const Srp_StreamIdType StreamId)
{
    for (uint8 i = 0U; i < Srp_Internal.streamCount; i++) {
        if (memcmp(Srp_Internal.streams[i].StreamId, StreamId, SRP_STREAM_ID_SIZE) == 0) {
            return &Srp_Internal.streams[i];
        }
    }
    return NULL_PTR;
}

void Srp_Init(const void* ConfigPtr)
{
    Srp_Internal.state = SRP_INTERNAL_UNINIT;
    Srp_Internal.streamCount = 0U;
    Srp_Internal.configPtr = (const Srp_ConfigType*)ConfigPtr;
    memset(Srp_Internal.streams, 0, sizeof(Srp_Internal.streams));
    Srp_Internal.state = SRP_INTERNAL_INIT;
}

void Srp_DeInit(void)
{
    Srp_Internal.state = SRP_INTERNAL_UNINIT;
    Srp_Internal.streamCount = 0U;
}

Std_ReturnType Srp_RegisterTalker(const Srp_TalkerAdvertiseType* TalkerInfo)
{
#if (SRP_DEV_ERROR_DETECT == STD_ON)
    if (Srp_Internal.state != SRP_INTERNAL_INIT) return E_NOT_OK;
    if (NULL_PTR == TalkerInfo) return E_NOT_OK;
#endif
    Srp_StreamEntryType* existing = Srp_FindStream(TalkerInfo->StreamId);
    if (existing != NULL_PTR) {
        existing->State = SRP_STATE_REGISTERED;
        existing->AccumulatedLatency = TalkerInfo->AccumulatedLatency;
        return E_OK;
    }
    if (Srp_Internal.streamCount >= SRP_MAX_STREAMS) return E_NOT_OK;

    Srp_StreamEntryType* entry = &Srp_Internal.streams[Srp_Internal.streamCount];
    memcpy(entry->StreamId, TalkerInfo->StreamId, SRP_STREAM_ID_SIZE);
    entry->State = SRP_STATE_REGISTERED;
    entry->Role = SRP_RESERVE_TALKER;
    entry->AccumulatedLatency = TalkerInfo->AccumulatedLatency;
    entry->Priority = TalkerInfo->PriorityAndRank & 0x07U;
    Srp_Internal.streamCount++;
    return E_OK;
}

Std_ReturnType Srp_RegisterListener(const Srp_StreamIdType StreamId)
{
    if (Srp_Internal.state != SRP_INTERNAL_INIT) return E_NOT_OK;
    Srp_StreamEntryType* entry = Srp_FindStream(StreamId);
    if (entry != NULL_PTR) {
        entry->State = SRP_STATE_REGISTERED;
        return E_OK;
    }
    if (Srp_Internal.streamCount >= SRP_MAX_STREAMS) return E_NOT_OK;
    entry = &Srp_Internal.streams[Srp_Internal.streamCount];
    memcpy(entry->StreamId, StreamId, SRP_STREAM_ID_SIZE);
    entry->State = SRP_STATE_REGISTERED;
    entry->Role = SRP_RESERVE_LISTENER;
    Srp_Internal.streamCount++;
    return E_OK;
}

Std_ReturnType Srp_DeregisterStream(const Srp_StreamIdType StreamId)
{
    if (Srp_Internal.state != SRP_INTERNAL_INIT) return E_NOT_OK;
    for (uint8 i = 0U; i < Srp_Internal.streamCount; i++) {
        if (memcmp(Srp_Internal.streams[i].StreamId, StreamId, SRP_STREAM_ID_SIZE) == 0) {
            if (i < (Srp_Internal.streamCount - 1U)) {
                Srp_Internal.streams[i] = Srp_Internal.streams[Srp_Internal.streamCount - 1U];
            }
            Srp_Internal.streamCount--;
            return E_OK;
        }
    }
    return E_NOT_OK;
}

Std_ReturnType Srp_GetStreamStatus(const Srp_StreamIdType StreamId, Srp_ReservationStateType* Status)
{
    if (NULL_PTR == Status) return E_NOT_OK;
    Srp_StreamEntryType* entry = Srp_FindStream(StreamId);
    if (entry == NULL_PTR) return E_NOT_OK;
    *Status = entry->State;
    return E_OK;
}

void Srp_RxIndication(const uint8* DataPtr, uint16 Length)
{
    if (NULL_PTR == DataPtr || Length < Srp_HdrLen) return;
    /* Parse SRP frame (simplified - checks for Talker Advertise/Listener Ready) */
    uint8 subtype = DataPtr[0] & 0x0FU;
    (void)subtype;
}

void Srp_MainFunction(void)
{
    if (Srp_Internal.state != SRP_INTERNAL_INIT) return;

    /* Process stream state transitions */
    for (uint8 i = 0U; i < Srp_Internal.streamCount; i++) {
        if (Srp_Internal.streams[i].State == SRP_STATE_REGISTERED) {
            Srp_Internal.streams[i].State = SRP_STATE_READY;
        }
    }
}

void Srp_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (NULL_PTR == versioninfo) return;
    versioninfo->vendorID = SRP_VENDOR_ID;
    versioninfo->moduleID = SRP_MODULE_ID;
    versioninfo->sw_major_version = 1U;
    versioninfo->sw_minor_version = 0U;
    versioninfo->sw_patch_version = 0U;
}