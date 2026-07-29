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
 * @file    dcm_priority_queue.c
 * @brief   DCM Priority Queue Implementation
 *
 * High-performance binary heap-based priority queue.
 * O(log n) insertion and removal, O(1) peek.
 *
 * AUTOSAR R22-11 compliant
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_priority_queue.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_PQ_MAGIC_INIT               0x50513031U  /* "PQ01" */
#define DCM_PQ_PARENT(i)                (((i) - 1U) / 2U)
#define DCM_PQ_LEFT_CHILD(i)            (((i) * 2U) + 1U)
#define DCM_PQ_RIGHT_CHILD(i)           (((i) * 2U) + 2U)

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;
    Dcm_PqEntry heap[DCM_PQ_MAX_SIZE];
    uint8_t size;
    uint32_t sequenceCounter;
    bool initialized;
    Dcm_PqStats stats;
} Dcm_PqStateType;

static Dcm_PqStateType s_pqState;

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Compare two entries for priority ordering
 * @return int32_t Negative if a has higher priority, positive if b has higher priority
 */
static int32_t compareEntries(const Dcm_PqEntry *a, const Dcm_PqEntry *b)
{
    /* First compare by priority level (lower number = higher priority) */
    if (a->priority < b->priority) {
        return -1;
    }
    if (a->priority > b->priority) {
        return 1;
    }
    
    /* Same priority: compare by sequence (FIFO within same priority) */
    if (a->sequence < b->sequence) {
        return -1;
    }
    if (a->sequence > b->sequence) {
        return 1;
    }
    
    /* Same priority and sequence: compare by timestamp */
    if (a->timestamp < b->timestamp) {
        return -1;
    }
    return 1;
}

/**
 * @brief Swap two entries
 */
static void swapEntries(Dcm_PqEntry *a, Dcm_PqEntry *b)
{
    Dcm_PqEntry temp;
    (void)memcpy(&temp, a, sizeof(Dcm_PqEntry));
    (void)memcpy(a, b, sizeof(Dcm_PqEntry));
    (void)memcpy(b, &temp, sizeof(Dcm_PqEntry));
}

/**
 * @brief Heapify up - restore heap property after insertion
 */
static void heapifyUp(uint8_t index)
{
    uint8_t current = index;
    
    while ((current > 0U) && 
           (compareEntries(&s_pqState.heap[current], 
                          &s_pqState.heap[DCM_PQ_PARENT(current)]) < 0)) {
        swapEntries(&s_pqState.heap[current], 
                   &s_pqState.heap[DCM_PQ_PARENT(current)]);
        current = DCM_PQ_PARENT(current);
    }
}

/**
 * @brief Heapify down - restore heap property after removal
 */
static void heapifyDown(uint8_t index)
{
    uint8_t current = index;
    uint8_t minIndex;
    
    while (DCM_PQ_LEFT_CHILD(current) < s_pqState.size) {
        minIndex = current;
        
        uint8_t left = DCM_PQ_LEFT_CHILD(current);
        uint8_t right = DCM_PQ_RIGHT_CHILD(current);
        
        if (compareEntries(&s_pqState.heap[left], 
                          &s_pqState.heap[minIndex]) < 0) {
            minIndex = left;
        }
        
        if ((right < s_pqState.size) && 
            (compareEntries(&s_pqState.heap[right], 
                          &s_pqState.heap[minIndex]) < 0)) {
            minIndex = right;
        }
        
        if (minIndex == current) {
            break;
        }
        
        swapEntries(&s_pqState.heap[current], &s_pqState.heap[minIndex]);
        current = minIndex;
    }
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_PqInit(void)
{
    (void)memset(&s_pqState, 0, sizeof(s_pqState));
    
    s_pqState.magic = DCM_PQ_MAGIC_INIT;
    s_pqState.size = 0U;
    s_pqState.sequenceCounter = 0U;
    s_pqState.initialized = true;
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_PqInsert(const Dcm_PqEntry *entry)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_pqState.initialized || (entry == NULL_PTR)) {
        return result;
    }
    
    /* Check if queue is full */
    if (s_pqState.size >= DCM_PQ_MAX_SIZE) {
        s_pqState.stats.overflowCount++;
        return result;
    }
    
    /* Add entry at end of heap */
    uint8_t index = s_pqState.size;
    (void)memcpy(&s_pqState.heap[index], entry, sizeof(Dcm_PqEntry));
    s_pqState.heap[index].sequence = s_pqState.sequenceCounter++;
    s_pqState.size++;
    
    /* Restore heap property */
    heapifyUp(index);
    
    /* Update statistics */
    s_pqState.stats.insertCount++;
    s_pqState.stats.currentDepth = s_pqState.size;
    if (s_pqState.size > s_pqState.stats.maxDepth) {
        s_pqState.stats.maxDepth = s_pqState.size;
    }
    
    result = DCM_E_OK;
    return result;
}

Dcm_ReturnType Dcm_PqRemove(Dcm_PqEntry *entry)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_pqState.initialized || (entry == NULL_PTR)) {
        return result;
    }
    
    if (s_pqState.size == 0U) {
        return result;
    }
    
    /* Copy highest priority entry */
    (void)memcpy(entry, &s_pqState.heap[0], sizeof(Dcm_PqEntry));
    
    /* Move last entry to root and reduce size */
    s_pqState.size--;
    if (s_pqState.size > 0U) {
        (void)memcpy(&s_pqState.heap[0], 
                    &s_pqState.heap[s_pqState.size], 
                    sizeof(Dcm_PqEntry));
        /* Restore heap property */
        heapifyDown(0);
    }
    
    /* Update statistics */
    s_pqState.stats.removeCount++;
    s_pqState.stats.currentDepth = s_pqState.size;
    if (entry->priority <= DCM_PRIO_HIGH) {
        s_pqState.stats.highPriorityCount++;
    }
    
    result = DCM_E_OK;
    return result;
}

Dcm_ReturnType Dcm_PqPeek(Dcm_PqEntry *entry)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_pqState.initialized || (entry == NULL_PTR)) {
        return result;
    }
    
    if (s_pqState.size == 0U) {
        return result;
    }
    
    /* Copy highest priority entry without removing */
    (void)memcpy(entry, &s_pqState.heap[0], sizeof(Dcm_PqEntry));
    s_pqState.stats.peekCount++;
    
    result = DCM_E_OK;
    return result;
}

bool Dcm_PqIsEmpty(void)
{
    return (!s_pqState.initialized) || (s_pqState.size == 0U);
}

bool Dcm_PqIsFull(void)
{
    return (s_pqState.initialized) && (s_pqState.size >= DCM_PQ_MAX_SIZE);
}

uint8_t Dcm_PqGetSize(void)
{
    if (!s_pqState.initialized) {
        return 0U;
    }
    return s_pqState.size;
}

Dcm_PriorityLevel Dcm_PqGetServicePriority(uint8_t serviceId)
{
    /* Critical services - highest priority */
    switch (serviceId) {
        case UDS_SVC_DIAGNOSTIC_SESSION_CONTROL:
        case UDS_SVC_ECU_RESET:
            return DCM_PRIO_CRITICAL;
            
        case UDS_SVC_SECURITY_ACCESS:
        case UDS_SVC_TESTER_PRESENT:
            return DCM_PRIO_HIGH;
            
        case UDS_SVC_READ_DATA_BY_IDENTIFIER:
        case UDS_SVC_READ_DTC_INFORMATION:
        case UDS_SVC_WRITE_DATA_BY_IDENTIFIER:
            return DCM_PRIO_NORMAL;
            
        case UDS_SVC_ROUTINE_CONTROL:
        case UDS_SVC_REQUEST_DOWNLOAD:
        case UDS_SVC_TRANSFER_DATA:
        case UDS_SVC_WRITE_MEMORY_BY_ADDRESS:
            return DCM_PRIO_LOW;
            
        default:
            return DCM_PRIO_BACKGROUND;
    }
}

Dcm_ReturnType Dcm_PqInsertServiceRequest(uint8_t serviceId, 
                                          const Dcm_RequestType *request)
{
    Dcm_PqEntry entry;
    
    if (request == NULL_PTR) {
        return DCM_E_NOT_OK;
    }
    
    (void)memset(&entry, 0, sizeof(entry));
    
    entry.entryType = DCM_PQ_ENTRY_SERVICE_REQUEST;
    entry.priority = Dcm_PqGetServicePriority(serviceId);
    entry.timestamp = 0U; /* 时间戳依赖系统定时器集成 */
    
    entry.data.request.serviceId = serviceId;
    entry.data.request.data = request->data;
    entry.data.request.length = request->length;
    entry.data.request.sourceAddr = request->sourceAddress;
    entry.data.request.protocol = request->protocol;
    
    if (request->length > 1U) {
        entry.data.request.subfunction = request->data[1] & DCM_SUBFUNCTION_MASK;
    } else {
        entry.data.request.subfunction = 0U;
    }
    
    return Dcm_PqInsert(&entry);
}

Dcm_ReturnType Dcm_PqGetStats(Dcm_PqStats *stats)
{
    if (!s_pqState.initialized || (stats == NULL_PTR)) {
        return DCM_E_NOT_OK;
    }
    
    (void)memcpy(stats, &s_pqState.stats, sizeof(Dcm_PqStats));
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_PqClear(void)
{
    if (!s_pqState.initialized) {
        return DCM_E_NOT_OK;
    }
    
    s_pqState.size = 0U;
    s_pqState.stats.currentDepth = 0U;
    
    return DCM_E_OK;
}

bool Dcm_PqHasHighPriorityEntries(void)
{
    if (!s_pqState.initialized || (s_pqState.size == 0U)) {
        return false;
    }
    
    /* Check if highest priority entry is high priority or above */
    return (s_pqState.heap[0].priority <= DCM_PRIO_HIGH);
}
