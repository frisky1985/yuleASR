/**
 * @file ComStack_Types.h
 * @brief COM Stack Common Types
 * @details Common types used across communication stack modules
 */

#ifndef COMSTACK_TYPES_H
#define COMSTACK_TYPES_H

#include "Std_Types.h"

/* PDU related types */
typedef uint16 PduIdType;
typedef uint32 PduLengthType;
typedef uint8 PduDataType;

/* PDU Info Type */
typedef struct
{
    uint8*           SduDataPtr;
    uint8*           MetaDataPtr;
    PduLengthType    SduLength;
} PduInfoType;

/* Retry Info Type */
typedef struct
{
    uint8            TpDataState;
    PduLengthType    TxTpDataCnt;
} RetryInfoType;

/* Buffer request return codes */
#define BUFREQ_OK           (0x00U)
#define BUFREQ_E_NOT_OK     (0x01U)
#define BUFREQ_E_BUSY       (0x02U)
#define BUFREQ_E_OVFL       (0x03U)

typedef uint8 BufReq_ReturnType;

/* TP Data State */
#define TP_DATACONF         (0x00U)
#define TP_DATARETRY        (0x01U)
#define TP_CONFPENDING      (0x02U)

typedef uint8 TPDataStateType;

/* Physical/PduR addresses */
typedef uint16 ComStack_PdusType;

#endif /* COMSTACK_TYPES_H */
