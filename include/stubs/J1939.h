/**
 * @file J1939.h
 * @brief J1939 Protocol Header - stub for compilation
 */
#ifndef J1939_H
#define J1939_H

#include "Std_Types.h"

/* J1939 PGN (Parameter Group Number) */
typedef uint32 J1939_PGNType;

/* J1939 address */
typedef uint8 J1939_AddressType;

/* J1939 priority */
typedef uint8 J1939_PriorityType;

/* J1939 source address */
typedef uint8 J1939_SourceAddressType;

/* J1939 destination address */
typedef uint8 J1939_DestinationAddressType;

/* J1939 NAME (Identity Number) */
typedef uint64 J1939_NAME_Type;

/* J1939 frame format */
typedef struct {
    J1939_PriorityType Priority;
    J1939_PGNType PGN;
    J1939_SourceAddressType SourceAddress;
    J1939_DestinationAddressType DestinationAddress;
    uint8 Data[8];
    uint8 DataLength;
} J1939_FrameType;

/* J1939 protocol types */
typedef uint8 J1939_ProtocolType;
#define J1939_TP_CM           0x01u   /* Transport Protocol - Connection Mode */
#define J1939_TP_DT           0x02u   /* Transport Protocol - Data Transfer */
#define J1939_TP_BAM          0x03u   /* Transport Protocol - Broadcast Announce Message */
#define J1939_TP_ACK          0x04u   /* Transport Protocol - Acknowledgement */
#define J1939_TP_ABORT        0x05u   /* Transport Protocol - Abort */

/* J1939 status */
typedef uint8 J1939_StatusType;
#define J1939_INIT            0x00u
#define J1939_READY           0x01u
#define J1939_BUSY            0x02u
#define J1939_TIMEOUT         0x03u
#define J1939_ERROR           0x04u

/* J1939 transport connection state */
typedef uint8 J1939_TpStateType;

#endif /* J1939_H */
