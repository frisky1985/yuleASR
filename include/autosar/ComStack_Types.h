/******************************************************************************
 * @file ComStack_Types.h
 * @brief AutoSAR ComStack Types (Native Stub)
 ******************************************************************************/
#ifndef COMSTACK_TYPES_H
#define COMSTACK_TYPES_H

#include "Std_Types.h"

/* PduIdType */
typedef uint16 PduIdType;

/* PduLengthType */
typedef uint32 PduLengthType;

/* PduInfoType */
typedef struct {
    uint8          *SduDataPtr;
    uint8          *MetaDataPtr;
    PduLengthType   SduLength;
} PduInfoType;

/* BufReq_ReturnType */
typedef uint8 BufReq_ReturnType;
#define BUFREQ_OK           ((BufReq_ReturnType)0U)
#define BUFREQ_E_NOT_OK     ((BufReq_ReturnType)1U)
#define BUFREQ_E_BUSY       ((BufReq_ReturnType)2U)
#define BUFREQ_E_OVFL       ((BufReq_ReturnType)3U)

/* NotifResultType */
typedef uint8 NotifResultType;
#define NTFRSLT_OK          ((NotifResultType)0U)
#define NTFRSLT_E_NOT_OK    ((NotifResultType)1U)

/* TP parameter IDs */
#define TP_PARAM_SA        ((uint8)0x01U)
#define TP_PARAM_TA        ((uint8)0x02U)
#define TP_PARAM_AE        ((uint8)0x03U)

#endif /* COMSTACK_TYPES_H */
