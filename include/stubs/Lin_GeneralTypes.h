/**
 * @file Lin_GeneralTypes.h
 * @brief LIN General Types - stub for compilation
 */
#ifndef LIN_GENERAL_TYPES_H
#define LIN_GENERAL_TYPES_H

#include "Std_Types.h"

/* LIN Frame types */
typedef uint8 Lin_FrameCsModelType;
typedef uint8 Lin_FrameRespErrorType;
typedef uint8 Lin_FrameVersionType;

/* LIN status types */
typedef uint8 Lin_StatusType;

/* LIN slave types */
typedef uint8 Lin_SlaveType;

/* LIN baudrate */
typedef uint32 Lin_BaudrateType;

/* LIN PID */
typedef uint8 Lin_PidType;

/* LIN identifier */
typedef uint8 Lin_IdType;

/* Checksum type */
typedef uint8 Lin_ChecksumType;

/* LIN Frame type */
#define LIN_FRAME_RESERVED       0x00u
#define LIN_FRAME_UNCONDITIONAL  0x01u
#define LIN_FRAME_EVENT_TRIGGERED 0x02u
#define LIN_FRAME_SPORADIC       0x03u
#define LIN_FRAME_DIAGNOSTIC     0x04u
#define LIN_FRAME_USER_DEFINED   0x05u

/* LIN Response Error */
#define LIN_RESP_OK               0x00u
#define LIN_RESP_NOT_OK           0x01u
#define LIN_RESP_BUSY             0x02u

/* Checksum model */
#define LIN_ENHANCED_CS    0x01u
#define LIN_CLASSIC_CS     0x00u

#endif /* LIN_GENERAL_TYPES_H */
