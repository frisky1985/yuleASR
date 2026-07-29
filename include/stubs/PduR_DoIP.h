/**
 * @file PduR_DoIP.h
 * @brief PDU Router DoIP Interface - stub for compilation
 */
#ifndef PDUR_DOIP_H
#define PDUR_DOIP_H

#include "Std_Types.h"
#include "PduR.h"

/* DoIP PDU ID */
typedef uint16 PduR_DoIP_PduIdType;

/* DoIP buffer type */
typedef struct {
    uint8* DataPtr;
    uint16 Length;
    uint32 SourceAddress;
    uint32 TargetAddress;
} PduR_DoIP_BufferType;

/* PduR DoIP routing path */
typedef struct {
    PduR_DoIP_PduIdType DoIPPduId;
    PduIdType PduRId;
    PduIdType DestPduId;
} PduR_DoIP_RoutingPathType;

#endif /* PDUR_DOIP_H */
