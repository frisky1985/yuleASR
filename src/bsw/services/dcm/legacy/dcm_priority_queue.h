/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/******************************************************************************
 * @file    dcm_priority_queue.h
 * @brief   DCM Priority Queue Implementation
 *
 * High-performance priority queue for DCM pending operations.
 * Replaces linear search with O(log n) operations.
 *
 * AUTOSAR R22-11 compliant
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_PRIORITY_QUEUE_H
#define DCM_PRIORITY_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * Priority Queue Configuration
 ******************************************************************************/
#define DCM_PQ_MAX_SIZE                 32U
#define DCM_PQ_HIGH_PRIORITY            0U
#define DCM_PQ_NORMAL_PRIORITY          1U
#define DCM_PQ_LOW_PRIORITY             2U

/******************************************************************************
 * Priority Queue Entry Types
 ******************************************************************************/
typedef enum {
    DCM_PQ_ENTRY_NONE = 0,
    DCM_PQ_ENTRY_SERVICE_REQUEST,       /* UDS service request */
    DCM_PQ_ENTRY_ASYNC_RESPONSE,        /* Async operation response */
    DCM_PQ_ENTRY_TIMER_CALLBACK,        /* Timer expiration callback */
    DCM_PQ_ENTRY_PROTOCOL_EVENT         /* Protocol event */
} Dcm_PqEntryType;

/* Priority levels for different services */
typedef enum {
    DCM_PRIO_CRITICAL = 0,              /* Session control, ECU reset */
    DCM_PRIO_HIGH,                      /* Security access, tester present */
    DCM_PRIO_NORMAL,                    /* Read/Write data */
    DCM_PRIO_LOW,                       /* Routine control, memory ops */
    DCM_PRIO_BACKGROUND                 /* Audit logging, stats */
} Dcm_PriorityLevel;

/* Queue entry structure - optimized for cache alignment */
typedef struct {
    Dcm_PqEntryType entryType;          /* Entry type */
    Dcm_PriorityLevel priority;         /* Priority level */
    uint32_t sequence;                  /* Sequence number for FIFO within same priority */
    uint32_t timestamp;                 /* Insertion timestamp */
    union {
        /* Service request data */
        struct {
            uint8_t serviceId;
            uint8_t subfunction;
            uint16_t sourceAddr;
            const uint8_t *data;
            uint32_t length;
            Dcm_ProtocolType protocol;
        } request;
        
        /* Async response data */
        struct {
            uint16_t handle;
            Dcm_ReturnType result;
            uint8_t *responseData;
            uint32_t responseLength;
        } async;
        
        /* Timer callback */
        struct {
            uint32_t timerId;
            void (*callback)(void);
            void *userData;
        } timer;
        
        /* Protocol event */
        struct {
            uint8_t eventType;
            uint16_t channelId;
            uint32_t eventData;
        } event;
    } data;
} Dcm_PqEntry;

/******************************************************************************
 * Priority Queue Statistics
 ******************************************************************************/
typedef struct {
    uint32_t insertCount;               /* Total insertions */
    uint32_t removeCount;               /* Total removals */
    uint32_t peekCount;                 /* Total peeks */
    uint32_t overflowCount;             /* Queue full events */
    uint32_t maxDepth;                  /* Maximum queue depth reached */
    uint32_t currentDepth;              /* Current queue depth */
    uint32_t highPriorityCount;         /* High priority entries processed */
    uint32_t avgWaitTime;               /* Average wait time (ms) */
} Dcm_PqStats;

/******************************************************************************
 * Priority Queue API
 ******************************************************************************/

/**
 * @brief Initialize priority queue
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_PqInit(void);

/**
 * @brief Insert entry into priority queue
 * @param entry Pointer to entry to insert
 * @return Dcm_ReturnType DCM_E_OK on success, DCM_E_NOT_OK if full
 */
Dcm_ReturnType Dcm_PqInsert(const Dcm_PqEntry *entry);

/**
 * @brief Remove and return highest priority entry
 * @param entry Pointer to store removed entry
 * @return Dcm_ReturnType DCM_E_OK on success, DCM_E_NOT_OK if empty
 */
Dcm_ReturnType Dcm_PqRemove(Dcm_PqEntry *entry);

/**
 * @brief Peek at highest priority entry without removing
 * @param entry Pointer to store entry
 * @return Dcm_ReturnType DCM_E_OK on success, DCM_E_NOT_OK if empty
 */
Dcm_ReturnType Dcm_PqPeek(Dcm_PqEntry *entry);

/**
 * @brief Check if queue is empty
 * @return bool True if empty
 */
bool Dcm_PqIsEmpty(void);

/**
 * @brief Check if queue is full
 * @return bool True if full
 */
bool Dcm_PqIsFull(void);

/**
 * @brief Get current queue size
 * @return uint8_t Number of entries in queue
 */
uint8_t Dcm_PqGetSize(void);

/**
 * @brief Get priority level for a service ID
 * @param serviceId UDS service ID
 * @return Dcm_PriorityLevel Priority level
 */
Dcm_PriorityLevel Dcm_PqGetServicePriority(uint8_t serviceId);

/**
 * @brief Insert service request with automatic priority assignment
 * @param serviceId UDS service ID
 * @param request Pointer to request
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_PqInsertServiceRequest(uint8_t serviceId, 
                                          const Dcm_RequestType *request);

/**
 * @brief Get queue statistics
 * @param stats Pointer to statistics structure
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_PqGetStats(Dcm_PqStats *stats);

/**
 * @brief Clear all entries from queue
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_PqClear(void);

/**
 * @brief Check for high priority entries waiting
 * @return bool True if high priority entries exist
 */
bool Dcm_PqHasHighPriorityEntries(void);

#ifdef __cplusplus
}
#endif

#endif /* DCM_PRIORITY_QUEUE_H */
