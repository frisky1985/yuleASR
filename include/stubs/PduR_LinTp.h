/**
 * @file PduR_LinTp.h
 * @brief PDU Router LIN Transport Protocol Interface - stub for compilation
 */
#ifndef PDUR_LINTP_H
#define PDUR_LINTP_H

#include "Std_Types.h"
#include "PduR.h"

/* LIN TP PDU ID */
typedef uint16 PduR_LinTp_PduIdType;

/* LIN TP buffer type */
typedef struct {
    uint8* DataPtr;
    uint16 Length;
} PduR_LinTp_BufferType;

/* PduR LIN TP routing path */
typedef struct {
    PduR_LinTp_PduIdType LinTpPduId;
    PduIdType PduRId;
    PduIdType DestPduId;
} PduR_LinTp_RoutingPathType;

#endif /* PDUR_LINTP_H */
