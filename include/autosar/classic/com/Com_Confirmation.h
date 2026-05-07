/*
 * Com_Confirmation.h
 * AUTOSAR COM Module - Transmission Confirmation and Retry Management
 * According to AUTOSAR SWS COM 4.4.0
 * 
 * Specifications:
 * - SWS_Com_00450: Transmission confirmation handling
 * - SWS_Com_00455: Retry mechanism support
 */

#ifndef COM_CONFIRMATION_H
#define COM_CONFIRMATION_H

/*==================[Includes]=============================================*/

#include "Com_Types.h"
#include "PduR.h"

/*==================[Version Information]==================================*/

#define COM_CONF_SW_MAJOR_VERSION   0x01u
#define COM_CONF_SW_MINOR_VERSION   0x00u
#define COM_CONF_SW_PATCH_VERSION   0x00u

/*==================[Transmission Status Types]============================*/

/* Transmission State Machine States */
typedef enum {
    COM_TX_IDLE = 0,            /*!< No transmission pending */
    COM_TX_PENDING,             /*!< Transmission in progress, awaiting confirmation */
    COM_TX_CONFIRMED,           /*!< Transmission confirmed successfully */
    COM_TX_ERROR,               /*!< Transmission error occurred */
    COM_TX_RETRY_PENDING        /*!< Retrying transmission */
} Com_TxStatusType;

/* Transmission Result Types */
typedef enum {
    COM_TX_RES_NONE = 0,        /*!< No result yet */
    COM_TX_RES_OK,              /*!< Transmission successful */
    COM_TX_RES_TIMEOUT,         /*!< Transmission timeout */
    COM_TX_RES_NOT_OK,          /*!< Transmission failed */
    COM_TX_RES_CANCELLED        /*!< Transmission cancelled */
} Com_TxResultType;

/*==================[Confirmation Configuration]===========================*/

/* Transmission Confirmation Configuration per IPdu */
typedef struct {
    boolean EnableConfirmation;     /*!< Enable transmission confirmation */
    uint32 TxTimeout;               /*!< Transmission timeout in ms (ComTxTimeout) */
    uint8 MaxRetries;               /*!< Maximum retry attempts (ComTxRetries) */
    void (*ComTxConfirmation)(void); /*!< Success notification callback */
    void (*ComTxErrorNotification)(void); /*!< Error notification callback */
    void (*ComTxTimeoutNotification)(void); /*!< Timeout notification callback */
} Com_TxConfirmationConfigType;

/*==================[Runtime Types]========================================*/

/* Transmission Confirmation Runtime Data */
typedef struct {
    Com_TxStatusType Status;        /*!< Current transmission status */
    Com_TxResultType LastResult;    /*!< Last transmission result */
    uint32 TimeoutTimer;            /*!< Timeout countdown timer */
    uint8 RetryCount;               /*!< Current retry count */
    boolean ConfirmationPending;    /*!< Waiting for confirmation flag */
    uint32 TxTimestamp;             /*!< Transmission start timestamp */
} Com_TxConfirmationRunTimeType;

/* Retry Queue Entry */
typedef struct {
    Com_IPduIdType PduId;           /*!< PDU ID to retry */
    uint8 RetryCount;               /*!< Remaining retry count */
    uint32 NextRetryTime;           /*!< Next retry timestamp */
    boolean Active;                 /*!< Entry is active */
} Com_RetryQueueEntryType;

/* Retry Queue */
typedef struct {
    Com_RetryQueueEntryType Entries[COM_MAX_RETRY_QUEUE_SIZE];
    uint8 Head;                     /*!< Queue head index */
    uint8 Tail;                     /*!< Queue tail index */
    uint8 Count;                    /*!< Current queue count */
} Com_RetryQueueType;

/*==================[Configuration Constants]==============================*/

#ifndef COM_MAX_RETRY_QUEUE_SIZE
#define COM_MAX_RETRY_QUEUE_SIZE    16u  /*!< Maximum retry queue entries */
#endif

#ifndef COM_DEFAULT_TX_TIMEOUT
#define COM_DEFAULT_TX_TIMEOUT      100u /*!< Default TX timeout in ms */
#endif

#ifndef COM_DEFAULT_MAX_RETRIES
#define COM_DEFAULT_MAX_RETRIES     3u   /*!< Default max retry count */
#endif

#ifndef COM_RETRY_DELAY_MS
#define COM_RETRY_DELAY_MS          10u  /*!< Delay between retries in ms */
#endif

/*==================[Callback Function Types]==============================*/

/* Transmission Confirmation Callback Type */
typedef void (*Com_TxConfirmationCbkType)(PduIdType TxPduId);

/* Transmission Error Callback Type */
typedef void (*Com_TxErrorCbkType)(PduIdType TxPduId, Com_TxResultType Result);

/*==================[API Function Declarations]============================*/

/*------------------[Confirmation Callback from PduR]----------------------*/

/**
 * @brief Transmission confirmation callback from PduR
 * @param TxPduId Transmit PDU ID
 * @param result Transmission result (E_OK or E_NOT_OK)
 * 
 * This function is called by PduR to confirm a transmission.
 * Implements SWS_Com_00450.
 */
extern void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/*------------------[Internal Confirmation Management]---------------------*/

/**
 * @brief Initialize confirmation module
 */
extern void Com_InitConfirmation(void);

/**
 * @brief Deinitialize confirmation module
 */
extern void Com_DeInitConfirmation(void);

/**
 * @brief Start transmission confirmation monitoring
 * @param PduId IPdu ID
 * @return E_OK if started successfully, E_NOT_OK otherwise
 */
extern Std_ReturnType Com_StartTxConfirmation(Com_IPduIdType PduId);

/**
 * @brief Cancel transmission confirmation monitoring
 * @param PduId IPdu ID
 */
extern void Com_CancelTxConfirmation(Com_IPduIdType PduId);

/**
 * @brief Handle transmission timeout
 * @param PduId IPdu ID
 */
extern void Com_HandleTxTimeout(Com_IPduIdType PduId);

/**
 * @brief Get current transmission status
 * @param PduId IPdu ID
 * @return Current transmission status
 */
extern Com_TxStatusType Com_GetTxStatus(Com_IPduIdType PduId);

/**
 * @brief Get last transmission result
 * @param PduId IPdu ID
 * @return Last transmission result
 */
extern Com_TxResultType Com_GetTxResult(Com_IPduIdType PduId);

/*------------------[Retry Mechanism]--------------------------------------*/

/**
 * @brief Initialize retry queue
 */
extern void Com_InitRetryQueue(void);

/**
 * @brief Add PDU to retry queue
 * @param PduId IPdu ID
 * @param RetryCount Number of retries remaining
 * @return E_OK if added successfully, E_NOT_OK otherwise
 */
extern Std_ReturnType Com_AddToRetryQueue(Com_IPduIdType PduId, uint8 RetryCount);

/**
 * @brief Remove PDU from retry queue
 * @param PduId IPdu ID
 */
extern void Com_RemoveFromRetryQueue(Com_IPduIdType PduId);

/**
 * @brief Process retry queue - called from MainFunction
 */
extern void Com_ProcessRetryQueue(void);

/**
 * @brief Check if PDU is in retry queue
 * @param PduId IPdu ID
 * @return TRUE if in queue, FALSE otherwise
 */
extern boolean Com_IsInRetryQueue(Com_IPduIdType PduId);

/**
 * @brief Get remaining retries for a PDU
 * @param PduId IPdu ID
 * @return Remaining retry count
 */
extern uint8 Com_GetRemainingRetries(Com_IPduIdType PduId);

/**
 * @brief Perform retry transmission
 * @param PduId IPdu ID
 * @return E_OK if retry initiated, E_NOT_OK otherwise
 */
extern Std_ReturnType Com_PerformRetry(Com_IPduIdType PduId);

/*------------------[Timeout Handling]-------------------------------------*/

/**
 * @brief Process all timeout monitoring - called from MainFunctionTx
 */
extern void Com_ProcessTxTimeouts(void);

/**
 * @brief Reset timeout timer for a PDU
 * @param PduId IPdu ID
 */
extern void Com_ResetTxTimeout(Com_IPduIdType PduId);

/**
 * @brief Check if PDU has timed out
 * @param PduId IPdu ID
 * @return TRUE if timed out, FALSE otherwise
 */
extern boolean Com_IsTxTimedOut(Com_IPduIdType PduId);

/*------------------[Transmission Mode Switch Handling]--------------------*/

/**
 * @brief Handle transmission mode switch confirmation
 * @param PduId IPdu ID
 * @param OldMode Previous transmission mode
 * @param NewMode New transmission mode
 * 
 * Called when transmission mode is switched while confirmation is pending.
 */
extern void Com_HandleModeSwitchConfirmation(Com_IPduIdType PduId, 
                                              Com_TransferModeType OldMode,
                                              Com_TransferModeType NewMode);

/**
 * @brief Check if mode switch is allowed during pending confirmation
 * @param PduId IPdu ID
 * @return TRUE if switch allowed, FALSE otherwise
 */
extern boolean Com_CanSwitchModeDuringPending(Com_IPduIdType PduId);

/*==================[Service IDs for Error Reporting]======================*/

#define COM_SERVICE_ID_TX_CONFIRMATION      0x20u
#define COM_SERVICE_ID_START_TX_CONF        0x21u
#define COM_SERVICE_ID_CANCEL_TX_CONF       0x22u
#define COM_SERVICE_ID_HANDLE_TIMEOUT       0x23u
#define COM_SERVICE_ID_ADD_RETRY            0x24u
#define COM_SERVICE_ID_PROCESS_RETRY        0x25u

/*==================[Error Codes]==========================================*/

#define COM_E_CONFIRMATION_TIMEOUT          0x30u
#define COM_E_MAX_RETRIES_EXCEEDED          0x31u
#define COM_E_RETRY_QUEUE_FULL              0x32u
#define COM_E_INVALID_RETRY                 0x33u

#endif /* COM_CONFIRMATION_H */
