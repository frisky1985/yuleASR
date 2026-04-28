/*
 * Com_Types.h
 * AUTOSAR COM Module - Type Definitions
 * According to AUTOSAR SWS COM 4.4.0
 */

#ifndef COM_TYPES_H
#define COM_TYPES_H

#include "Std_Types.h"

/*==================[Type Definitions]======================================*/

/* Signal ID type */
typedef uint16 Com_SignalIdType;

/* Signal Group ID type */
typedef uint16 Com_SignalGroupIdType;

/* I-PDU ID type */
typedef uint16 Com_IPduIdType;

/* I-PDU Group ID type */
typedef uint16 Com_IpduGroupIdType;

/* Signal Type enumeration */
typedef enum {
    COM_BOOLEAN,
    COM_UINT8,
    COM_UINT16,
    COM_UINT32,
    COM_UINT64,
    COM_SINT8,
    COM_SINT16,
    COM_SINT32,
    COM_SINT64,
    COM_FLOAT32,
    COM_FLOAT64,
    COM_UINT8_N,
    COM_UINT16_N,
    COM_UINT32_N,
    COM_UINT64_N
} Com_SignalTypeType;

/* Signal Endianness */
typedef enum {
    COM_LITTLE_ENDIAN,
    COM_BIG_ENDIAN,
    COM_OPAQUE
} Com_SignalEndiannessType;

/* Transfer Property */
typedef enum {
    COM_PENDING,
    COM_TRIGGERED,
    COM_TRIGGERED_ON_CHANGE,
    COM_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION,
    COM_TRIGGERED_WITHOUT_REPETITION
} Com_TransferPropertyType;

/* IPdu Direction */
typedef enum {
    COM_SEND,
    COM_RECEIVE
} Com_IPduDirectionType;

/* IPdu Type */
typedef enum {
    COM_NORMAL,
    COM_TP
} Com_IPduType;

/* IPdu Signal Processing */
typedef enum {
    COM_IMMEDIATE,
    COM_DEFERRED
} Com_IPduSignalProcessingType;

/* Transmission Mode */
typedef enum {
    COM_DIRECT,
    COM_MIXED,
    COM_NONE,
    COM_PERIODIC
} Com_TransferModeType;

/* IPdu Group Status */
typedef enum {
    COM_IPDU_GROUP_STOPPED = 0,
    COM_IPDU_GROUP_STARTED = 1
} Com_IpduGroupStatusType;

/* Module Status */
typedef enum {
    COM_UNINIT = 0,
    COM_READY = 1
} Com_StatusType;

/*==================[Configuration Types]===================================*/

/* Signal Configuration */
typedef struct {
    Com_SignalIdType SignalId;
    uint8* DataPtr;
    uint16 BitPosition;
    uint8 BitSize;
    Com_SignalEndiannessType Endianness;
    Com_SignalTypeType SignalType;
    Com_TransferPropertyType TransferProperty;
    void (*ComNotification)(void);
    uint32 Timeout;
    const void* InitValue;
} Com_SignalConfigType;

/* Signal Group Configuration */
typedef struct {
    Com_SignalGroupIdType SignalGroupId;
    Com_SignalIdType* SignalRefs;
    uint8 NumSignals;
    uint8* ShadowBuffer;
    void (*ComNotification)(void);
} Com_SignalGroupConfigType;

/* Tx Mode Configuration */
typedef struct {
    Com_TransferModeType Mode;
    uint32 Period;
    uint32 RepetitionPeriod;
    uint8 NumRepetitions;
    uint32 TimeOffset;
} Com_TxModeConfigType;

/* IPdu Configuration */
typedef struct {
    Com_IPduIdType IPduId;
    uint8* DataPtr;
    uint8 Length;
    Com_IPduDirectionType Direction;
    Com_IPduType Type;
    Com_IPduSignalProcessingType SignalProcessing;
    Com_SignalIdType* SignalRefs;
    uint8 NumSignals;
    Com_SignalGroupIdType* SignalGroupRefs;
    uint8 NumSignalGroups;
    Com_TxModeConfigType TxMode;
    Com_IpduGroupIdType* IpduGroupRefs;
    uint8 NumIpduGroups;
    uint32 Timeout;
    void (*ComIPduCallout)(PduIdType PduId, PduInfoType* PduInfoPtr);
} Com_IPduConfigType;

/* IPdu Group Configuration */
typedef struct {
    Com_IpduGroupIdType IpduGroupId;
    Com_IPduIdType* IPduRefs;
    uint8 NumIPdus;
} Com_IPduGroupConfigType;

/* Global Configuration */
typedef struct {
    const Com_SignalConfigType* Signals;
    uint16 NumSignals;
    const Com_SignalGroupConfigType* SignalGroups;
    uint16 NumSignalGroups;
    const Com_IPduConfigType* IPdus;
    uint16 NumIPdus;
    const Com_IPduGroupConfigType* IPduGroups;
    uint16 NumIPduGroups;
} Com_ConfigType;

/*==================[Return Types]=========================================*/

#ifndef COM_SERVICE_NOT_AVAILABLE
#define COM_SERVICE_NOT_AVAILABLE 0x80
#endif

#ifndef COM_BUSY
#define COM_BUSY 0x81
#endif

#endif /* COM_TYPES_H */
