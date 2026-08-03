/******************************************************************************
 * @file    e2e_dds_integration.h
 * @brief   E2E Protection with DDS Integration Layer
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * This module provides integration between E2E protection and DDS middleware,
 * enabling safety-critical DDS communication with end-to-end protection.
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef E2E_DDS_INTEGRATION_H
#define E2E_DDS_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "autosar_types.h"
#include "e2e_protection.h"
#include "e2e_state_machine.h"

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* E2E DDS Integration Version */
#define E2E_DDS_MAJOR_VERSION           22
#define E2E_DDS_MINOR_VERSION           11
#define E2E_DDS_PATCH_VERSION           0

/* Maximum number of E2E-protected DDS topics */
#define E2E_DDS_MAX_TOPICS              32U
#define E2E_DDS_MAX_TOPIC_NAME_LEN      64U

/* Default QoS settings */
#define E2E_DDS_DEFAULT_RELIABILITY     DDS_RELIABILITY_RELIABLE
#define E2E_DDS_DEFAULT_HISTORY_DEPTH   10U

/******************************************************************************
 * Data Types
 ******************************************************************************/

/* DDS Reliability QoS */
typedef enum {
    DDS_RELIABILITY_BEST_EFFORT = 0,
    DDS_RELIABILITY_RELIABLE
} E2E_DDS_ReliabilityQos;

/* DDS Durability QoS */
typedef enum {
    DDS_DURABILITY_VOLATILE = 0,
    DDS_DURABILITY_TRANSIENT_LOCAL,
    DDS_DURABILITY_TRANSIENT,
    DDS_DURABILITY_PERSISTENT
} E2E_DDS_DurabilityQos;

/* E2E DDS Topic configuration */
typedef struct {
    char topicName[E2E_DDS_MAX_TOPIC_NAME_LEN]; /* DDS topic name */
    uint8_t e2eProfile;                         /* E2E Profile ID */
    uint16_t dataId;                            /* E2E Data ID */
    uint16_t dataLength;                        /* Fixed data length (0 = dynamic) */
    uint16_t maxDataLength;                     /* Max length for dynamic data */
    
    /* DDS QoS Settings */
    E2E_DDS_ReliabilityQos reliability;
    E2E_DDS_DurabilityQos durability;
    uint32_t historyDepth;
    uint32_t deadlineMs;
    
    /* E2E State Machine settings */
    bool enableStateMachine;                    /* Enable E2E_SM */
    uint8_t windowSize;                         /* Window check size */
    uint8_t maxErrorThreshold;                  /* Error threshold */
    uint32_t timeoutMs;                         /* Freshness timeout */
} E2E_DDS_TopicConfigType;

/* E2E DDS Topic handle */
typedef struct {
    uint32_t topicId;                           /* Unique topic ID */
    E2E_DDS_TopicConfigType config;             /* Topic configuration */
    E2E_ContextType e2eContext;                 /* E2E protection context */
    E2E_SM_InternalStateType smState;           /* State machine state (if enabled) */
    bool initialized;                           /* Initialization flag */
    uint32_t txCounter;                         /* Transmit counter */
    uint32_t rxCounter;                         /* Receive counter */
    uint32_t errorCounter;                      /* Error counter */
    uint32_t lastTxTimestamp;                   /* Last TX timestamp */
    uint32_t lastRxTimestamp;                   /* Last RX timestamp */
} E2E_DDS_TopicHandleType;

/* E2E DDS Result status */
typedef enum {
    E2E_DDS_OK = 0,                             /* Success */
    E2E_DDS_ERROR_INIT,                         /* Initialization error */
    E2E_DDS_ERROR_PROFILE,                      /* Invalid profile */
    E2E_DDS_ERROR_LENGTH,                       /* Invalid length */
    E2E_DDS_ERROR_CRC,                          /* CRC error */
    E2E_DDS_ERROR_SEQUENCE,                     /* Sequence error */
    E2E_DDS_ERROR_TIMEOUT,                      /* Timeout error */
    E2E_DDS_ERROR_STATE_MACHINE                 /* State machine error */
} E2E_DDS_ResultType;

/* E2E DDS Callback functions */
typedef void (*E2E_DDS_ErrorCallback)(
    uint32_t topicId,
    E2E_DDS_ResultType result,
    uint16_t e2eStatus);

typedef void (*E2E_DDS_StateChangeCallback)(
    uint32_t topicId,
    E2E_SM_StateType oldState,
    E2E_SM_StateType newState);

/* E2E DDS Callback configuration */
typedef struct {
    E2E_DDS_ErrorCallback errorCallback;        /* Error callback */
    E2E_DDS_StateChangeCallback stateCallback;  /* State change callback */
} E2E_DDS_CallbackType;

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief Initialize E2E DDS Integration module
 * @return E_OK on success, E_NOT_OK on failure
 */
Std_ReturnType E2E_DDS_Init(void);

/**
 * @brief Deinitialize E2E DDS Integration module
 * @return E_OK on success, E_NOT_OK on failure
 */
Std_ReturnType E2E_DDS_Deinit(void);

/**
 * @brief Check if E2E DDS Integration is initialized
 * @return TRUE if initialized
 */
bool E2E_DDS_IsInitialized(void);

/**
 * @brief Register error/state change callbacks
 * @param callbacks Callback configuration
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_RegisterCallbacks(const E2E_DDS_CallbackType* callbacks);

/******************************************************************************
 * Function Prototypes - Topic Management
 ******************************************************************************/

/**
 * @brief Create E2E-protected DDS topic
 * @param config Topic configuration
 * @param handle Output topic handle
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_CreateTopic(
    const E2E_DDS_TopicConfigType* config,
    E2E_DDS_TopicHandleType* handle);

/**
 * @brief Destroy E2E-protected DDS topic
 * @param handle Topic handle
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_DestroyTopic(E2E_DDS_TopicHandleType* handle);

/**
 * @brief Reset E2E topic state
 * @param handle Topic handle
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_ResetTopic(E2E_DDS_TopicHandleType* handle);

/******************************************************************************
 * Function Prototypes - Data Transmission
 ******************************************************************************/

/**
 * @brief Publish data with E2E protection via DDS
 * @param handle Topic handle
 * @param data Data buffer
 * @param length Data length (input/output)
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_Publish(
    E2E_DDS_TopicHandleType* handle,
    uint8_t* data,
    uint32_t* length);

/**
 * @brief Subscribe to data with E2E protection via DDS
 * @param handle Topic handle
 * @param data Data buffer
 * @param length Data length (input/output)
 * @param result E2E check result
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_Subscribe(
    E2E_DDS_TopicHandleType* handle,
    const uint8_t* data,
    uint32_t length,
    E2E_DDS_ResultType* result);

/**
 * @brief Check received data with E2E validation
 * @param handle Topic handle
 * @param data Received data
 * @param length Data length
 * @param timestamp Current timestamp
 * @param result Output result
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_Check(
    E2E_DDS_TopicHandleType* handle,
    const uint8_t* data,
    uint32_t length,
    uint32_t timestamp,
    E2E_DDS_ResultType* result);

/******************************************************************************
 * Function Prototypes - Dynamic Length Support
 ******************************************************************************/

/**
 * @brief Set dynamic data length for Profile 22
 * @param handle Topic handle
 * @param dataLength New data length
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_SetDynamicLength(
    E2E_DDS_TopicHandleType* handle,
    uint16_t dataLength);

/**
 * @brief Get current data length
 * @param handle Topic handle
 * @param dataLength Output length
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_GetDataLength(
    const E2E_DDS_TopicHandleType* handle,
    uint16_t* dataLength);

/******************************************************************************
 * Function Prototypes - Statistics and Diagnostics
 ******************************************************************************/

/**
 * @brief Get topic statistics
 * @param handle Topic handle
 * @param txCount Output TX counter
 * @param rxCount Output RX counter
 * @param errorCount Output error counter
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_GetStatistics(
    const E2E_DDS_TopicHandleType* handle,
    uint32_t* txCount,
    uint32_t* rxCount,
    uint32_t* errorCount);

/**
 * @brief Get current E2E state machine state
 * @param handle Topic handle
 * @param state Output state
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_GetState(
    const E2E_DDS_TopicHandleType* handle,
    E2E_SM_StateType* state);

/**
 * @brief Clear topic statistics
 * @param handle Topic handle
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_ClearStatistics(E2E_DDS_TopicHandleType* handle);

/******************************************************************************
 * Function Prototypes - Utility
 ******************************************************************************/

/**
 * @brief Convert E2E DDS result to string
 * @param result Result code
 * @return Result string
 */
const char* E2E_DDS_ResultToString(E2E_DDS_ResultType result);

/**
 * @brief Get version string
 * @return Version string
 */
const char* E2E_DDS_GetVersion(void);

/**
 * @brief Map E2E profile to recommended DDS QoS
 * @param profile E2E Profile ID
 * @param config Output DDS configuration
 * @return E_OK on success
 */
Std_ReturnType E2E_DDS_GetRecommendedQos(
    uint8_t profile,
    E2E_DDS_TopicConfigType* config);

#ifdef __cplusplus
}
#endif

#endif /* E2E_DDS_INTEGRATION_H */
