/**
 * @file ComStack_Types.h
 * @brief AUTOSAR Communication Stack Types
 * @version 1.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @implements AUTOSAR_SWS_CommunicationStackTypes.pdf
 * Contains all common type definitions for the AUTOSAR Communication Stack.
 * This includes buffer request types, TP parameter types, and retry info.
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
    uint8*         SduDataPtr;    /**< Pointer to SDU data */
    uint8*         MetaDataPtr;   /**< Pointer to meta data */
    PduLengthType  SduLength;     /**< Length of SDU data */
} PduInfoType;

/*==================================================================================================
 *                                    NOTIF RESULT TYPE
 *==================================================================================================*/
typedef enum {
    NTFRSLT_E_OK       = 0x00U,   /**< Notification result OK */
    NTFRSLT_E_NOT_OK   = 0x01U,   /**< Notification result NOT OK */
    NTFRSLT_E_CANCEL   = 0x02U    /**< Notification cancelled */
} NotifResultType;

/*==================================================================================================
 *                                    BSW SCHEDULER TYPE
 *==================================================================================================*/
typedef enum {
    BUILT_IN_FLAG_CALLBACK = 0U,   /**< Callback type */
    BUILT_IN_FLAG_EVENT    = 1U    /**< Event type */
} BuiltInFlagType;

/*==================================================================================================
 *                                    COM BUS TYPE
 *==================================================================================================*/
typedef enum {
    COMM_BUS_TYPE_CAN      = 0x00U,
    COMM_BUS_TYPE_LIN      = 0x01U,
    COMM_BUS_TYPE_FLEXRAY  = 0x02U,
    COMM_BUS_TYPE_ETH      = 0x03U
} ComBusTypeType;

/*==================================================================================================
 *                                    BUFREQ RETURN TYPE
 *==================================================================================================*/
#ifndef BUFREQ_RETURNTYPE_DEFINED
#define BUFREQ_RETURNTYPE_DEFINED
typedef enum {
    BUFREQ_E_OK     = 0U,  /**< Buffer request succeeded */
    BUFREQ_E_NOT_OK = 1U,  /**< Buffer request failed */
    BUFREQ_E_BUSY   = 2U   /**< Buffer request busy */
} BufReq_ReturnType;
#endif

/*==================================================================================================
 *                                    TP PARAMETER TYPE
 *==================================================================================================*/
#ifndef TPPARAMETERTYPE_DEFINED
#define TPPARAMETERTYPE_DEFINED
typedef uint8 TPParameterType;
#endif

/*==================================================================================================
 *                                    RETRY INFO TYPE
 *==================================================================================================*/
#ifndef RETRYINFOTYPE_DEFINED
#define RETRYINFOTYPE_DEFINED
typedef struct {
    uint16      RetryCounter;   /**< Number of retries attempted */
    TPParameterType TpParameter; /**< TP parameter for retry */
} RetryInfoType;
#endif

/*==================================================================================================
 *                                    NETWORK MANAGEMENT TYPE
 *==================================================================================================*/
typedef uint8 Nm_PduIdType;
typedef uint8 Nm_ChannelIdType;

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
