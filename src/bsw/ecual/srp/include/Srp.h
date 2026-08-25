/**
 * @file Srp.h
 * @brief SRP - Stream Reservation Protocol (IEEE 802.1Qat)
 * @version 2.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @implements IEEE 802.1Qat / AUTOSAR SRP Specification
 */

#ifndef SRP_H
#define SRP_H

#include "Std_Types.h"
#include "ComStack_Types.h"

#define SRP_AR_RELEASE_MAJOR_VERSION   4U
#define SRP_AR_RELEASE_MINOR_VERSION   4U
#define SRP_AR_RELEASE_REVISION_VERSION 0U
#define SRP_SW_MAJOR_VERSION           1U
#define SRP_SW_MINOR_VERSION           0U
#define SRP_SW_PATCH_VERSION           0U
#define SRP_MODULE_ID               0x90U
#define SRP_VENDOR_ID               0x0055U
#define SRP_STREAM_ID_SIZE          8U

typedef uint8 Srp_StreamIdType[SRP_STREAM_ID_SIZE];

typedef enum {
    SRP_RESERVE_TALKER = 0,
    SRP_RESERVE_LISTENER
} Srp_ReservationTypeType;

typedef enum {
    SRP_STATE_IDLE = 0,
    SRP_STATE_REGISTERED,
    SRP_STATE_READY,
    SRP_STATE_FAILED
} Srp_ReservationStateType;

typedef struct {
    Srp_StreamIdType StreamId;
    uint8 DataFrameParameters[20];
    uint8 TSpec[12];
    uint8 PriorityAndRank;
    uint16 AccumulatedLatency;
} Srp_TalkerAdvertiseType;

typedef struct {
    Srp_StreamIdType StreamId;
    uint8 EgmData[4];
} Srp_ListenerReadyType;

typedef struct {
    Srp_StreamIdType StreamId;
    uint16 StreamVlanId;
    uint8 Priority;
    uint16 FrameSize;
    uint16 IntervalFrames;
    Srp_ReservationTypeType Role;
} Srp_StreamConfigType;

typedef struct {
    uint8 MaxStreams;
    uint32 SrpMacAddress[2];
    uint16 SrpEtherType;
} Srp_ConfigType;

/** @req SWS_Srp_00001 */
void Srp_Init(const void* ConfigPtr);
/** @req SWS_Srp_00002 */
void Srp_DeInit(void);
/** @req SWS_Srp_00005 */
Std_ReturnType Srp_RegisterTalker(const Srp_TalkerAdvertiseType* TalkerInfo);
/** @req SWS_Srp_00006 */
Std_ReturnType Srp_RegisterListener(const Srp_StreamIdType StreamId);
/** @req SWS_Srp_00007 */
Std_ReturnType Srp_DeregisterStream(const Srp_StreamIdType StreamId);
/** @req SWS_Srp_00008 */
Std_ReturnType Srp_GetStreamStatus(const Srp_StreamIdType StreamId, Srp_ReservationStateType* Status);
/** @req SWS_Srp_00009 */
void Srp_RxIndication(const uint8* DataPtr, uint16 Length);
/** @req SWS_Srp_00004 */
void Srp_MainFunction(void);
/** @req SWS_Srp_00003 */
void Srp_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* SRP_H */