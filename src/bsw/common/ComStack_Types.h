/**
 * @file ComStack_Types.h
 * @brief Communication Stack common types following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Communication Stack Types
 */

#ifndef COMSTACK_TYPES_H
#define COMSTACK_TYPES_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define COMSTACK_TYPES_VENDOR_ID                (0x01U) /* YuleTech Vendor ID */
#define COMSTACK_TYPES_MODULE_ID                (0xFFU) /* Common Types Module ID */
#define COMSTACK_TYPES_AR_RELEASE_MAJOR_VERSION (0x04U)
#define COMSTACK_TYPES_AR_RELEASE_MINOR_VERSION (0x04U)
#define COMSTACK_TYPES_AR_RELEASE_REVISION_VERSION (0x00U)
#define COMSTACK_TYPES_SW_MAJOR_VERSION         (0x01U)
#define COMSTACK_TYPES_SW_MINOR_VERSION         (0x00U)
#define COMSTACK_TYPES_SW_PATCH_VERSION         (0x00U)

/*==================================================================================================
*                                    PDU RELATED TYPES
==================================================================================================*/

/** @brief Type for PDU identifier */
typedef uint16 PduIdType;

/** @brief Type for PDU length */
typedef uint16 PduLengthType;

/** @brief Type for CAN identifier */
typedef uint32 Can_IdType;

/** @brief Type for CAN ID type (standard/extended) */
typedef enum {
    CANIF_CANID_TYPE_STANDARD = 0,
    CANIF_CANID_TYPE_EXTENDED,
    CANIF_CANID_TYPE_STANDARD_FD,
    CANIF_CANID_TYPE_EXTENDED_FD
} CanIf_CanIdTypeType;

/*==================================================================================================
*                                    PDU INFO TYPE
==================================================================================================*/
typedef struct {
    uint8* SduDataPtr;
    uint8* MetaDataPtr;
    PduLengthType SduLength;
} PduInfoType;

/*==================================================================================================
*                                    RETRY INFO TYPE
==================================================================================================*/
typedef enum {
    TP_DATACONF = 0,
    TP_DATARETRY,
    TP_CONFPENDING
} TpDataStateType;

typedef struct {
    TpDataStateType TpDataState;
    PduLengthType TxTpDataCnt;
} RetryInfoType;

/*==================================================================================================
*                                    TP PARAMETER TYPE
==================================================================================================*/
typedef enum {
    TP_STMIN = 0,
    TP_BS,
    TP_BC
} TPParameterType;

/*==================================================================================================
*                                    BUF REQ RETURN TYPE
==================================================================================================*/
typedef enum {
    BUFREQ_OK = 0,
    BUFREQ_E_NOT_OK,
    BUFREQ_E_BUSY,
    BUFREQ_E_OVFL
} BufReq_ReturnType;

/*==================================================================================================
*                                    NETWORK HANDLE TYPE
==================================================================================================*/
typedef uint8 NetworkHandleType;

/*==================================================================================================
*                                    I-PDU GROUP VECTOR TYPE
==================================================================================================*/
typedef uint8 IcomConfigIdType;

/*==================================================================================================
*                                    ECUM WAKEUP SOURCE TYPE
==================================================================================================*/
typedef uint32 EcuM_WakeupSourceType;

/*==================================================================================================
*                                    I-PDU HANDLE ID TYPE
==================================================================================================*/
typedef uint16 IpduM_IdType;

/*==================================================================================================
*                                    BUS TRAC STATE TYPE
==================================================================================================*/
typedef uint8 BusTrcvStateType;

/*==================================================================================================
*                                    BUS TRAC ERROR TYPE
==================================================================================================*/
typedef uint8 BusTrcvErrorType;

/*==================================================================================================
*                                    PHYSICAL STATE TYPE
==================================================================================================*/
typedef uint8 PhysStateType;

/*==================================================================================================
*                                    NETWORK MODE TYPE
==================================================================================================*/
typedef uint8 NetworkModeType;

/*==================================================================================================
*                                    COM MODE TYPE
==================================================================================================*/
typedef enum {
    COMM_NO_COMMUNICATION = 0,
    COMM_SILENT_COMMUNICATION,
    COMM_FULL_COMMUNICATION
} ComM_ModeType;

/*==================================================================================================
*                                    PN LEARN TYPE
==================================================================================================*/
typedef enum {
    PN_CLEARED = 0,
    PN_LEARN_IN_PROGRESS,
    PN_LEARNED
} PNCHandleType;

/*==================================================================================================
*                                    REMOTE TYPE
==================================================================================================*/
typedef enum {
    REMOTE_STATUS_OK = 0,
    REMOTE_STATUS_NOT_OK
} RemoteType;

/*==================================================================================================
*                                    PN LEARNING TYPE
==================================================================================================*/
typedef enum {
    PN_LEARNING_REQUEST = 0,
    PN_LEARNING_RESET
} PNLearningType;

#endif /* COMSTACK_TYPES_H */
