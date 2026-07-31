/**
 * @file ComStack_Types.h
 * @brief AUTOSAR Communication Stack Types
 * @version 1.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @implements AUTOSAR_SWS_CommunicationStackTypes.pdf
 */

#ifndef COMSTACK_TYPES_H
#define COMSTACK_TYPES_H

#include "Std_Types.h"

/*==================================================================================================
 *                                    BASIC DEFINITIONS
 *==================================================================================================*/

#ifndef PduIdType
typedef uint16 PduIdType;
#endif

#ifndef PduLengthType
typedef uint32 PduLengthType;
#endif

/*==================================================================================================
 *                                    PDU INFO TYPE
 *==================================================================================================*/
typedef struct {
    uint8*         SduDataPtr;     /**< Pointer to SDU data */
    uint8*         MetaDataPtr;    /**< Pointer to meta data */
    PduLengthType  SduLength;      /**< Length of SDU data */
} PduInfoType;

/*==================================================================================================
 *                                    NOTIF RESULT TYPE
 *==================================================================================================*/
typedef enum {
    NTFRSLT_OK       = 0x00U,      /**< Notification result OK */
    NTFRSLT_NOT_OK   = 0x01U,      /**< Notification result NOT OK */
    NTFRSLT_CANCEL   = 0x02U       /**< Notification cancelled */
} NotifResultType;

/*==================================================================================================
 *                                    BSW SCHEDULER TYPE
 *==================================================================================================*/
typedef enum {
    BSW_CALLBACK = 0U,             /**< Callback type */
    BSW_EVENT    = 1U              /**< Event type */
} BuiltInFlagType;

/*==================================================================================================
 *                                    BUFREQ RETURN TYPE
 *==================================================================================================*/
#ifndef BUFREQ_RETURNTYPE_DEFINED
#define BUFREQ_RETURNTYPE_DEFINED
typedef enum {
    BUFREQ_E_OK     = 0U,
    BUFREQ_E_NOT_OK = 1U,
    BUFREQ_E_BUSY   = 2U,
    BUFREQ_E_OVFL    = 3U
} BufReq_ReturnType;
#endif

/*==================================================================================================
 *                                    TP PARAMETER TYPE
 *==================================================================================================*/
#ifndef TPPARAMETERTYPE_DEFINED
#define TPPARAMETERTYPE_DEFINED
typedef uint8 TPParameterType;
#ifndef TP_STMIN
#define TP_STMIN                (0x01U)
#endif
#ifndef TP_BS
#define TP_BS                   (0x02U)
#endif
#ifndef TP_BC
#define TP_BC                   (0x03U)
#endif
#endif

/*==================================================================================================
 *                                    RETRY INFO TYPE
 *==================================================================================================*/
#ifndef RETRYINFOTYPE_DEFINED
#define RETRYINFOTYPE_DEFINED
/* TP data state */
#ifndef TP_DATACONF
#define TP_DATACONF             (0x00U)
#endif
#ifndef TP_DATARETRY
#define TP_DATARETRY            (0x01U)
#endif
#ifndef TP_CONFPENDING
#define TP_CONFPENDING          (0x02U)
#endif
typedef struct {
    uint16      RetryCounter;
    TPParameterType TpParameter;
    uint8       TpDataState;
    uint16      BufSize;
} RetryInfoType;
#endif

/*==================================================================================================
 *                                    NETWORK MANAGEMENT TYPE
 *==================================================================================================*/
typedef uint8 Nm_PduIdType;
typedef uint8 Nm_ChannelIdType;
typedef uint8 NetworkHandleType;

/*==================================================================================================
 *                                    COMMON TYPES
 *==================================================================================================*/
#ifndef IcomConfigIdType
typedef uint16 IcomConfigIdType;
#endif

#ifndef IcomDuplCmdType
typedef uint8 IcomDuplCmdType;
#endif

#endif /* COMSTACK_TYPES_H */
