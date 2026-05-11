/**
 * @file Srp.h
 * @brief SRP - Stream Reservation Protocol
 * @version 1.0.0
 */

#ifndef SRP_H
#define SRP_H

#include "Std_Types.h"
#include "ComStack_Types.h"

#define SRP_MODULE_ID           0x90U
#define SRP_VENDOR_ID           0x0001U

/* Error Codes */
#define SRP_E_NO_ERROR          0x00U
#define SRP_E_PARAM_POINTER     0x01U
#define SRP_E_UNINIT            0x02U

/* Stream ID Size (64-bit) */
#define SRP_STREAM_ID_SIZE      8U

/* Stream ID Type */
typedef uint8 Srp_StreamIdType[SRP_STREAM_ID_SIZE];

/* Reservation Type */
typedef enum {
    SRP_RESERVE_TALKER = 0,
    SRP_RESERVE_LISTENER
} Srp_ReservationTypeType;

/* Reservation State */
typedef enum {
    SRP_STATE_IDLE = 0,
    SRP_STATE_REGISTERED,
    SRP_STATE_READY,
    SRP_STATE_FAILED
} Srp_ReservationStateType;

/* Talker Advertise Structure */
typedef struct {
    Srp_StreamIdType StreamId;
    uint8 DataFrameParameters[20];
    uint8 TSpec[12];
    uint8 PriorityAndRank;
    uint16 AccumulatedLatency;
} Srp_TalkerAdvertiseType;

/* Listener Ready Structure */
typedef struct {
    Srp_StreamIdType StreamId;
} Srp_ListenerReadyType;

/* Stream Configuration */
typedef struct {
    Srp_StreamIdType StreamId;
    uint16 StreamVlanId;
    uint8 Priority;
    uint16 FrameSize;
    uint16 IntervalFrames;
    Srp_ReservationTypeType Role;
} Srp_StreamConfigType;

/* Functions */
void Srp_Init(const void* ConfigPtr);
void Srp_DeInit(void);
Std_ReturnType Srp_RegisterTalker(const Srp_TalkerAdvertiseType* TalkerInfo);
Std_ReturnType Srp_RegisterListener(const Srp_StreamIdType StreamId);
Std_ReturnType Srp_DeregisterStream(const Srp_StreamIdType StreamId);
void Srp_RxIndication(const uint8* DataPtr, uint16 Length);
void Srp_MainFunction(void);

#endif
