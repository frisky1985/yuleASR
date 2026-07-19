/**
 * @file CanTSyn_Cfg.h
 * @brief CAN Time Synchronization Configuration - AutoSAR R22-11 Service Layer
 * @version 4.7.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: CAN Time Synchronization (CANTSYN)
 * Module ID: 0xA4U
 * Layer: Service Layer
 */

#ifndef CANTSYN_CFG_H
#define CANTSYN_CFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define CANTSYN_CFG_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define CANTSYN_CFG_MODULE_ID                   (0xA4U) /* CANTSYN Module ID */
#define CANTSYN_CFG_INSTANCE_ID                 (0x00U)

#define CANTSYN_CFG_AR_RELEASE_MAJOR_VERSION    (0x22U)
#define CANTSYN_CFG_AR_RELEASE_MINOR_VERSION    (0x11U)
#define CANTSYN_CFG_AR_RELEASE_REVISION_VERSION (0x00U)

#define CANTSYN_CFG_SW_MAJOR_VERSION            (0x04U)
#define CANTSYN_CFG_SW_MINOR_VERSION            (0x07U)
#define CANTSYN_CFG_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/

/** @brief Development error detection enable/disable */
#define CANTSYN_DEV_ERROR_DETECT                (STD_ON)

/** @brief Version info API enable/disable */
#define CANTSYN_VERSION_INFO_API                (STD_ON)

/** @brief Time Master support enable/disable */
#define CANTSYN_TIME_MASTER_SUPPORT             (STD_ON)

/** @brief Time Slave support enable/disable */
#define CANTSYN_TIME_SLAVE_SUPPORT              (STD_ON)

/** @brief Immediate Time Transmission enable/disable (Direct sending of time in SYNC message) */
#define CANTSYN_IMMEDIATE_TIME_TRANSMISSION     (STD_OFF)

/** @brief Enable/disable cancellation of TX confirmation for time sync messages */
#define CANTSYN_CANCEL_TX_CONFIRMATION          (STD_OFF)

/** @brief Enable/disable global time support for hardware timestamps */
#define CANTSYN_GLOBAL_TIME_SUPPORT             (STD_ON)

/** @brief Enable/disable debounce counter for continuous time sync */
#define CANTSYN_DEBOUNCE_COUNTER                (STD_ON)

/** @brief CRC secured mode for SYNC messages enable/disable */
#define CANTSYN_CRC_SECURED                     (STD_OFF)

/** @brief Message counter support for SYNC messages enable/disable */
#define CANTSYN_MESSAGE_COUNTER_SUPPORT         (STD_ON)

/** @brief Sequence counter support for SYNC messages enable/disable */
#define CANTSYN_SEQUENCE_COUNTER_SUPPORT        (STD_ON)

/** @brief TLV (Type-Length-Value) support for user data enable/disable */
#define CANTSYN_TLV_SUPPORT                     (STD_ON)

/** @brief User Data support (OCS - Optional Content SYNC) enable/disable */
#define CANTSYN_USER_DATA_SUPPORT               (STD_ON)

/** @brief Rate correction support enable/disable */
#define CANTSYN_RATE_CORRECTION_SUPPORT         (STD_ON)

/** @brief Offset correction support enable/disable */
#define CANTSYN_OFFSET_CORRECTION_SUPPORT       (STD_ON)

/*==================================================================================================
*                                    TIME BASE CONFIGURATION
==================================================================================================*/

/** @brief Maximum number of time bases supported */
#define CANTSYN_MAX_TIME_BASES                  (4U)

/** @brief Maximum number of Time Masters per time base */
#define CANTSYN_MAX_TIME_MASTERS                (2U)

/** @brief Maximum number of Time Slaves per time base */
#define CANTSYN_MAX_TIME_SLAVES                 (2U)

/** @brief Number of configured time domains */
#define CANTSYN_NUM_TIME_DOMAINS                (2U)

/*==================================================================================================
*                                    SYNC MESSAGE CONFIGURATION
==================================================================================================*/

/** @brief SYNC message transmission period in ms (default 10ms for 100Hz) */
#define CANTSYN_SYNC_PERIOD_MS                  (10U)

/** @brief FUP message transmission delay in us (after SYNC) */
#define CANTSYN_FUP_DELAY_US                    (100U)

/** @brief SYNC/FUP message cycle time in main function ticks */
#define CANTSYN_SYNC_CYCLE_TIME                 (10U)

/** @brief Maximum allowed time jump for time correction in us */
#define CANTSYN_MAX_TIME_JUMP_US                (1000000UL) /* 1 second */

/** @brief Debounce time in ms for continuous time sync */
#define CANTSYN_DEBOUNCE_TIME_MS                (5U)

/** @brief Timeout for time sync in ms */
#define CANTSYN_SYNC_TIMEOUT_MS                 (100U)

/** @brief Number of SYNC messages without valid response before sync lost */
#define CANTSYN_SYNC_LOST_THRESHOLD             (10U)

/*==================================================================================================
*                                    MESSAGE FORMAT CONFIGURATION
==================================================================================================*/

/** @brief SYNC message CAN ID (base, can be configured per time base) */
#define CANTSYN_SYNC_CAN_ID_BASE                (0x180U)

/** @brief FUP message CAN ID (base, can be configured per time base) */
#define CANTSYN_FUP_CAN_ID_BASE                 (0x280U)

/** @brief OCS (User Data) message CAN ID (base, can be configured per time base) */
#define CANTSYN_OCS_CAN_ID_BASE                 (0x380U)

/** @brief SYNC message DLC (Data Length Code) */
#define CANTSYN_SYNC_DLC                        (8U)

/** @brief FUP message DLC */
#define CANTSYN_FUP_DLC                         (8U)

/** @brief OCS message DLC */
#define CANTSYN_OCS_DLC                         (8U)

/** @brief Extended CAN ID enable/disable */
#define CANTSYN_EXTENDED_CAN_ID                 (STD_OFF)

/*==================================================================================================
*                                    PROTOCOL CONFIGURATION
==================================================================================================*/

/** @brief Protocol version (1 for AutoSAR 4.x, 2 for AutoSAR R22-11) */
#define CANTSYN_PROTOCOL_VERSION                (2U)

/** @brief Time base representation (0: 48-bit seconds + 32-bit nanoseconds, 1: 64-bit nanoseconds) */
#define CANTSYN_TIME_REPRESENTATION             (0U)

/** @brief SGW (Synchronization Gateway) support */
#define CANTSYN_SGW_SUPPORT                     (STD_ON)

/** @brief OFS (Offset) support */
#define CANTSYN_OFS_SUPPORT                     (STD_ON)

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/

/** @brief TX confirmation callback enable/disable */
#define CANTSYN_TX_CONFIRMATION                 (STD_ON)

/** @brief RX indication callback enable/disable */
#define CANTSYN_RX_INDICATION                   (STD_ON)

/** @brief Time notification callback enable/disable */
#define CANTSYN_TIME_NOTIFICATION               (STD_ON)

/** @brief Sync loss notification callback enable/disable */
#define CANTSYN_SYNC_LOSS_NOTIFICATION          (STD_ON)

/*==================================================================================================
*                                    INSTANCE CONFIGURATION
==================================================================================================*/

/** @brief Number of CanTSyn instances */
#define CANTSYN_NUM_INSTANCES                   (1U)

/** @brief Instance ID for this CanTSyn module */
#define CANTSYN_INSTANCE_ID                     (0U)

/*==================================================================================================
*                                    MAIN FUNCTION CONFIGURATION
==================================================================================================*/

/** @brief Main function period in ms */
#define CANTSYN_MAIN_FUNCTION_PERIOD_MS         (1U)

/** @brief Number of main function ticks per SYNC transmission */
#define CANTSYN_TICKS_PER_SYNC                  (CANTSYN_SYNC_PERIOD_MS / CANTSYN_MAIN_FUNCTION_PERIOD_MS)

/*==================================================================================================
*                                    TIME BASE ID MAPPING
==================================================================================================*/

/** @brief Time Base ID for Global Time */
#define CANTSYN_TIMEBASE_GLOBAL_ID              (0U)

/** @brief Time Base ID for Offset Time */
#define CANTSYN_TIMEBASE_OFFSET_ID              (1U)

/** @brief Time Base ID for Local Time */
#define CANTSYN_TIMEBASE_LOCAL_ID               (2U)

/*==================================================================================================
*                                    COMPATIBILITY ALIASES
*==================================================================================================*/
/* Code uses NUMBER_OF naming; config uses NUM naming */
#define CANTSYN_NUMBER_OF_TIME_DOMAINS          CANTSYN_NUM_TIME_DOMAINS
#define CANTSYN_NUMBER_OF_PDUS                  CANTSYN_NUM_TIME_DOMAINS
#define CANTSYN_NUMBER_OF_TIME_BASES            CANTSYN_MAX_TIME_BASES

/* Error codes used in CanTSyn.c */
#define CANTSYN_E_INVALID_PDU_SDU_ID            (0x01U)

#endif /* CANTSYN_CFG_H */
