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
