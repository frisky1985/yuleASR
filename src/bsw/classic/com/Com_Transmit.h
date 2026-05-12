/******************************************************************************
 * @file    Com_Transmit.h
 * @brief   COM Module - Transmission Scheduler Header
 * 
 * This file defines the transmission scheduler for the AUTOSAR COM module.
 * It provides the interface for:
 * - Send signal and signal group handling
 * - Send request queue management
 * - COM_TriggerIPDUSend scheduling logic
 * - PduR_COMTransmit integration
 * - ASIL-D safety protections
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x1E (COM)
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef COM_TRANSMIT_H
#define COM_TRANSMIT_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "Com_Private.h"

/******************************************************************************
 * Version Information
 ******************************************************************************/
#define COM_TRANSMIT_SW_MAJOR_VERSION       1u
#define COM_TRANSMIT_SW_MINOR_VERSION       0u
#define COM_TRANSMIT_SW_PATCH_VERSION       0u

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/

/** Maximum number of pending send requests in queue */
#ifndef COM_MAX_TX_REQUESTS
#define COM_MAX_TX_REQUESTS                 32u
#endif

/** Maximum number of retry attempts for failed transmissions */
#ifndef COM_MAX_TX_RETRIES
#define COM_MAX_TX_RETRIES                  3u
#endif

/** Transmission timeout in milliseconds (ASIL-D safety) */
#ifndef COM_TX_TIMEOUT_MS
#define COM_TX_TIMEOUT_MS                   100u
#endif

/** Enable ASIL-D safety checks */
#ifndef COM_SAFETY_CHECKS_ENABLE
#define COM_SAFETY_CHECKS_ENABLE            STD_ON
#endif

/** Enable redundancy checks for safety-critical signals */
#ifndef COM_REDUNDANCY_CHECKS_ENABLE
#define COM_REDUNDANCY_CHECKS_ENABLE        STD_ON
#endif

/******************************************************************************
 * Send Request Queue Types
 ******************************************************************************/

/**
 * @brief Send request state
 */
typedef enum {
    COM_TXREQ_IDLE = 0,         /**< Request slot is free */
    COM_TXREQ_PENDING,          /**< Request is pending transmission */
    COM_TXREQ_IN_PROGRESS,      /**< Transmission in progress */
    COM_TXREQ_RETRY,            /**< Request needs retry */
    COM_TXREQ_COMPLETED,        /**< Transmission completed successfully */
    COM_TXREQ_FAILED            /**< Transmission failed after retries */
} Com_TxRequestStateType;

/**
 * @brief Send request type
 */
typedef enum {
    COM_TXREQ_SIGNAL = 0,       /**< Signal send request */
    COM_TXREQ_SIGNALGROUP,      /**< Signal group send request */
    COM_TXREQ_TRIGGERED         /**< Triggered send request */
} Com_TxRequestType;

/**
 * @brief Send request entry
 * 
 * This structure represents a single entry in the send request queue.
 * Used for queuing transmission requests from signals and signal groups.
 */
typedef struct {
    Com_TxRequestStateType State;       /**< Current state of the request */
    Com_TxRequestType Type;             /**< Type of request */
    Com_IPduIdType PduId;               /**< Target I-PDU ID */
    Com_SignalIdType SignalId;          /**< Signal ID (for signal requests) */
    Com_SignalGroupIdType SignalGroupId;/**< Signal Group ID (for group requests) */
    uint32 Timestamp;                   /**< Request timestamp (for timeout detection) */
    uint8 RetryCount;                   /**< Current retry count */
    boolean IsPeriodic;                 /**< TRUE if this is a periodic transmission */
} Com_TxRequestEntryType;

/**
 * @brief Send request queue control structure
 */
typedef struct {
    Com_TxRequestEntryType Entries[COM_MAX_TX_REQUESTS];    /**< Queue entries */
    uint8 Head;                         /**< Head index for FIFO */
    uint8 Tail;                         /**< Tail index for FIFO */
    uint8 Count;                        /**< Current number of pending requests */
    uint32 SequenceCounter;             /**< Sequence counter for request ordering */
} Com_TxRequestQueueType;

/**
 * @brief Transmission statistics for monitoring
 */
typedef struct {
    uint32 TotalRequests;               /**< Total send requests */
    uint32 SuccessfulTransmissions;     /**< Successful transmissions */
    uint32 FailedTransmissions;         /**< Failed transmissions */
    uint32 RetryAttempts;               /**< Total retry attempts */
    uint32 TimeoutErrors;               /**< Timeout error count */
    uint32 QueueOverflows;              /**< Queue overflow count */
    uint32 LastErrorTimestamp;          /**< Timestamp of last error */
} Com_TxStatisticsType;

/**
 * @brief I-PDU transmission context
 * 
 * Runtime data for managing I-PDU transmissions with safety checks.
 */
typedef struct {
    boolean IsActive;                   /**< Transmission is active */
    uint32 StartTime;                   /**< Transmission start timestamp */
    uint32 Timeout;                     /**< Timeout value for this transmission */
    uint8 RetryCounter;                 /**< Current retry counter */
    uint16 CrcValue;                    /**< CRC for redundancy check */
    uint32 DataHash;                    /**< Data hash for integrity check */
} Com_IPduTxContextType;

/******************************************************************************
 * External Variables
 ******************************************************************************/

/** Send request queue - defined in Com_Transmit.c */
extern Com_TxRequestQueueType Com_TxRequestQueue;

/** Transmission statistics - defined in Com_Transmit.c */
extern Com_TxStatisticsType Com_TxStatistics;

/** I-PDU transmission contexts - defined in Com_Transmit.c */
extern Com_IPduTxContextType Com_IPduTxContexts[COM_MAX_IPDUS];

/******************************************************************************
 * Send Request Queue API
 ******************************************************************************/

/**
 * @brief Initialize the send request queue
 * 
 * This function initializes the send request queue to empty state.
 * Called during Com_Init().
 */
void Com_TxQueueInit(void);

/**
 * @brief Add a send request to the queue
 * 
 * @param Type Request type (signal, signal group, or triggered)
 * @param PduId Target I-PDU ID
 * @param SignalId Signal ID (valid if Type is COM_TXREQ_SIGNAL)
 * @param SignalGroupId Signal Group ID (valid if Type is COM_TXREQ_SIGNALGROUP)
 * @return E_OK if request queued successfully, E_NOT_OK otherwise
 */
Std_ReturnType Com_TxQueueAddRequest(
    Com_TxRequestType Type,
    Com_IPduIdType PduId,
    Com_SignalIdType SignalId,
    Com_SignalGroupIdType SignalGroupId);

/**
 * @brief Get next pending request from the queue
 * 
 * @param RequestPtr Pointer to store the request entry
 * @return E_OK if a pending request was found, E_NOT_OK if queue is empty
 */
Std_ReturnType Com_TxQueueGetNextRequest(Com_TxRequestEntryType** RequestPtr);

/**
 * @brief Remove a completed request from the queue
 * 
 * @param RequestPtr Pointer to the request entry to remove
 */
void Com_TxQueueRemoveRequest(Com_TxRequestEntryType* RequestPtr);

/**
 * @brief Mark a request for retry
 * 
 * @param RequestPtr Pointer to the request entry
 */
void Com_TxQueueMarkRetry(Com_TxRequestEntryType* RequestPtr);

/**
 * @brief Clear all pending requests for an I-PDU
 * 
 * @param PduId I-PDU ID to clear requests for
 */
void Com_TxQueueClearForPdu(Com_IPduIdType PduId);

/**
 * @brief Get current queue fill level
 * 
 * @return Number of pending requests in queue
 */
uint8 Com_TxQueueGetFillLevel(void);

/******************************************************************************
 * Signal Send API
 ******************************************************************************/

/**
 * @brief Send a signal (internal implementation)
 * 
 * This is the internal implementation of Com_SendSignal that handles
 * the actual signal transmission logic including:
 * - Input validation (ASIL-D)
 * - Signal packing
 * - Transmission triggering based on transfer property
 * - Queue management
 * 
 * @param SignalId Signal identifier
 * @param SignalDataPtr Pointer to signal data
 * @return uint8 Status code (E_OK, COM_SERVICE_NOT_AVAILABLE, etc.)
 */
uint8 Com_SendSignal_Internal(Com_SignalIdType SignalId, const void* SignalDataPtr);

/**
 * @brief Invalidate a signal (internal implementation)
 * 
 * Marks the signal as invalid and triggers transmission if configured.
 * Used by Com_InvalidateSignal.
 * 
 * @param SignalId Signal identifier
 * @return uint8 Status code
 */
uint8 Com_InvalidateSignal_Internal(Com_SignalIdType SignalId);

/******************************************************************************
 * Signal Group Send API
 ******************************************************************************/

/**
 * @brief Send a signal group (internal implementation)
 * 
 * This is the internal implementation of Com_SendSignalGroup.
 * Copies shadow buffer to I-PDU and triggers transmission.
 * 
 * @param SignalGroupId Signal group identifier
 * @return uint8 Status code
 */
uint8 Com_SendSignalGroup_Internal(Com_SignalGroupIdType SignalGroupId);

/**
 * @brief Invalidate a signal group (internal implementation)
 * 
 * Marks all signals in the group as invalid.
 * 
 * @param SignalGroupId Signal group identifier
 * @return uint8 Status code
 */
uint8 Com_InvalidateSignalGroup_Internal(Com_SignalGroupIdType SignalGroupId);

/******************************************************************************
 * I-PDU Transmission API
 ******************************************************************************/

/**
 * @brief Trigger I-PDU send (internal implementation)
 * 
 * This is the internal implementation of Com_TriggerIPDUSend.
 * Schedules the I-PDU for immediate transmission.
 * 
 * @param PduId I-PDU identifier
 * @return Std_ReturnType E_OK if triggered successfully
 */
Std_ReturnType Com_TriggerIPDUSend_Internal(Com_IPduIdType PduId);

/**
 * @brief Process transmission of an I-PDU
 * 
 * This function handles the actual transmission of an I-PDU:
 * - Checks transmission conditions
 * - Calls PduR_COMTransmit
 * - Updates statistics
 * 
 * @param PduId I-PDU identifier
 * @return Std_ReturnType Transmission result
 */
Std_ReturnType Com_TransmitIPdu(Com_IPduIdType PduId);

/**
 * @brief Check if I-PDU should be transmitted based on mode
 * 
 * @param PduId I-PDU identifier
 * @return boolean TRUE if transmission should occur
 */
boolean Com_ShouldTransmitIPdu(Com_IPduIdType PduId);

/**
 * @brief Handle transmission confirmation from PduR
 * 
 * Called when PduR_ComTxConfirmation is received.
 * 
 * @param PduId I-PDU identifier
 * @param Result Transmission result (E_OK or E_NOT_OK)
 */
void Com_HandleTxConfirmation(Com_IPduIdType PduId, Std_ReturnType Result);

/**
 * @brief Process transmission retry logic
 * 
 * Handles retry attempts for failed transmissions.
 * Called from Com_MainFunctionTx.
 */
void Com_ProcessTxRetries(void);

/******************************************************************************
 * Safety and Protection API (ASIL-D)
 ******************************************************************************/

/**
 * @brief Validate send signal parameters (ASIL-D)
 * 
 * Performs comprehensive validation of Com_SendSignal parameters:
 * - Module initialization check
 * - Signal ID range check
 * - Data pointer null check
 * - IPdu group status check
 * 
 * @param SignalId Signal identifier
 * @param SignalDataPtr Pointer to signal data
 * @return Std_ReturnType E_OK if validation passes
 */
Std_ReturnType Com_ValidateSendSignalParams(
    Com_SignalIdType SignalId, 
    const void* SignalDataPtr);

/**
 * @brief Validate send request queue integrity (ASIL-D)
 * 
 * Performs runtime checks on queue integrity:
 * - Queue bounds checking
 * - State consistency checking
 * - Counter validation
 * 
 * @return Std_ReturnType E_OK if queue is valid
 */
Std_ReturnType Com_ValidateTxQueueIntegrity(void);

/**
 * @brief Check transmission timeout (ASIL-D)
 * 
 * Detects if an I-PDU transmission has timed out.
 * Called periodically from Com_MainFunctionTx.
 * 
 * @param PduId I-PDU identifier
 * @return boolean TRUE if timeout detected
 */
boolean Com_CheckTxTimeout(Com_IPduIdType PduId);

/**
 * @brief Calculate CRC for redundancy check (ASIL-D)
 * 
 * Calculates CRC16 over I-PDU data for redundancy checking.
 * 
 * @param DataPtr Pointer to data
 * @param Length Data length in bytes
 * @return uint16 CRC value
 */
uint16 Com_CalculateCRC(const uint8* DataPtr, uint8 Length);

/**
 * @brief Calculate data hash for integrity check (ASIL-D)
 * 
 * Calculates a simple hash over data for integrity verification.
 * 
 * @param DataPtr Pointer to data
 * @param Length Data length in bytes
 * @return uint32 Hash value
 */
uint32 Com_CalculateDataHash(const uint8* DataPtr, uint8 Length);

/**
 * @brief Verify I-PDU data integrity before transmission (ASIL-D)
 * 
 * Performs redundancy checks on I-PDU data:
 * - CRC verification
 * - Data hash comparison
 * 
 * @param PduId I-PDU identifier
 * @return Std_ReturnType E_OK if integrity check passes
 */
Std_ReturnType Com_VerifyIPduIntegrity(Com_IPduIdType PduId);

/******************************************************************************
 * Statistics and Diagnostics API
 ******************************************************************************/

/**
 * @brief Get transmission statistics
 * 
 * @param StatsPtr Pointer to statistics structure
 */
void Com_GetTxStatistics(Com_TxStatisticsType* StatsPtr);

/**
 * @brief Reset transmission statistics
 */
void Com_ResetTxStatistics(void);

/**
 * @brief Get I-PDU transmission context
 * 
 * @param PduId I-PDU identifier
 * @return Com_IPduTxContextType* Pointer to context, NULL if invalid
 */
Com_IPduTxContextType* Com_GetIPduTxContext(Com_IPduIdType PduId);

/******************************************************************************
 * PduR Integration API
 ******************************************************************************/

/**
 * @brief Call PduR for I-PDU transmission
 * 
 * Wrapper function for PduR_ComTransmit that provides:
 * - Error handling
 * - Statistics updates
 * - Timeout management
 * 
 * @param PduId I-PDU identifier
 * @param PduInfoPtr Pointer to PDU information
 * @return Std_ReturnType Transmission result
 */
Std_ReturnType Com_CallPduRTransmit(
    Com_IPduIdType PduId, 
    const PduInfoType* PduInfoPtr);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Get current timestamp in milliseconds
 * 
 * Platform-specific function to get current time.
 * Used for timeout detection.
 * 
 * @return uint32 Timestamp in milliseconds
 */
uint32 Com_GetCurrentTimestamp(void);

/**
 * @brief Find I-PDU ID containing a signal
 * 
 * Searches configuration to find which I-PDU contains the given signal.
 * 
 * @param SignalId Signal identifier
 * @return Com_IPduIdType I-PDU ID, or COM_MAX_IPDUS if not found
 */
Com_IPduIdType Com_FindPduForSignal(Com_SignalIdType SignalId);

/**
 * @brief Find I-PDU ID containing a signal group
 * 
 * Searches configuration to find which I-PDU contains the given signal group.
 * 
 * @param SignalGroupId Signal group identifier
 * @return Com_IPduIdType I-PDU ID, or COM_MAX_IPDUS if not found
 */
Com_IPduIdType Com_FindPduForSignalGroup(Com_SignalGroupIdType SignalGroupId);

#ifdef __cplusplus
}
#endif

#endif /* COM_TRANSMIT_H */
