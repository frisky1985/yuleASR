/******************************************************************************
 * @file ComStack_Types.h
 * @brief COM Stack Types (AutoSAR) - Stub
 ******************************************************************************/

#ifndef COMSTACK_TYPES_H
#define COMSTACK_TYPES_H

#include "Std_Types.h"

/* PDU identifier type */
typedef uint16 PduIdType;

/* PDU length type */
typedef uint32 PduLengthType;

/* TP Parameter Type */
typedef enum {
    TP_STMIN = 0,
    TP_BS,
    TP_BC
} TPParameterType;

/* Notifcation status type */
typedef enum {
    NTFRSLT_OK = 0x00,
    NTFRSLT_E_NOT_OK = 0x01,
    NTFRSLT_E_TIMEOUT_A = 0x02,
    NTFRSLT_E_TIMEOUT_BS = 0x03,
    NTFRSLT_E_TIMEOUT_CR = 0x04,
    NTFRSLT_E_WRONG_SN = 0x05,
    NTFRSLT_E_INVALID_FS = 0x06,
    NTFRSLT_E_UNEXP_PDU = 0x07,
    NTFRSLT_E_WFT_OVERRUN = 0x08,
    NTFRSLT_E_NO_IP_MULTICAST = 0x09,
    NTFRSLT_E_ABORT = 0x0A
} TpNotifResultType;

/* BufReq_ReturnType */
typedef enum {
    BUFREQ_OK = 0,
    BUFREQ_E_NOT_OK = 1,
    BUFREQ_E_BUSY = 2,
    BUFREQ_E_OVFL = 3
} BufReq_ReturnType;

/* NotifCallCbk type */
typedef void (*NotifCallCbk)(void);

/* PDU Info Type */
typedef struct {
    PduIdType SduLength;
    uint8* SduDataPtr;    /* AUTOSAR standard name */
    PduLengthType MetaDataLength;
    uint8* MetaDataPtr;
} PduInfoType;

/* PDU MetaData Type */
typedef struct {
    uint16 MetaDataLength;
    uint8* MetaDataPtr;
} PduMetaDataType;

/* Notification callback type */
typedef void (*Pdu_NotifyCallbackType)(void);

/* Retry info type */
typedef struct {
    uint8 TpDataState;
    PduLengthType TxTpDataCnt;
} RetryInfoType;

/* Network handle type */
typedef uint8 NetworkHandleType;

/* I-PDU group vector */
typedef uint8 IpduGroupVector;

/* Protocol configuration type */
typedef uint8 PdulConfigType;

#endif /* COMSTACK_TYPES_H */
