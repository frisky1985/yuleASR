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

/***********************************************************************************************************************
 * File:        dem_queue.c
 * Description: Dem Event Queue Management Implementation
 *              Provides asynchronous event processing with priority queue
 **********************************************************************************************************************/

#include "dem_queue.h"
#include <string.h>

/*==================================================================================================
 *                                      LOCAL VARIABLES
==================================================================================================*/

/* Event queue instance */
static Dem_EventQueueType Dem_EventQueue;

/* Queue statistics */
static uint32 Dem_QueueTotalEnqueued = 0;
static uint32 Dem_QueueTotalDequeued = 0;
static uint32 Dem_QueueTotalDropped = 0;

/*==================================================================================================
 *                                      LOCAL FUNCTIONS
==================================================================================================*/

/**
 * rief   Get next index in circular buffer
 */
static uint8 Dem_QueueNextIndex(uint8 Index)
{
    return (Index + 1) % DEM_CFG_EVENT_QUEUE_SIZE;
}

/**
 * rief   Get previous index in circular buffer
 */
static uint8 Dem_QueuePrevIndex(uint8 Index)
{
    return (Index == 0) ? (DEM_CFG_EVENT_QUEUE_SIZE - 1) : (Index - 1);
}

/**
 * rief   Find insertion position for priority queue
 */
static uint8 Dem_QueueFindInsertPosition(uint8 Priority)
{
    uint8 pos = Dem_EventQueue.Tail;
    uint8 count = Dem_EventQueue.Count;
    
    while (count > 0) {
        uint8 prev = Dem_QueuePrevIndex(pos);
        if (Dem_EventQueue.Entries[prev].Priority <= Priority) {
            break;
        }
        pos = prev;
        count--;
    }
    
    return pos;
}

/**
 * rief   Shift entries to make room for insertion
 */
static void Dem_QueueShiftEntries(uint8 InsertPos)
{
    uint8 pos = Dem_EventQueue.Tail;
    
    while (pos != InsertPos) {
        uint8 prev = Dem_QueuePrevIndex(pos);
        Dem_EventQueue.Entries[pos] = Dem_EventQueue.Entries[prev];
        pos = prev;
    }
}

/*==================================================================================================
 *                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * rief   Initialize the event queue
 */
void Dem_QueueInit(void)
{
    memset(&Dem_EventQueue, 0, sizeof(Dem_EventQueue));
    Dem_EventQueue.State = DEM_QUEUE_EMPTY;
    Dem_QueueTotalEnqueued = 0;
    Dem_QueueTotalDequeued = 0;
    Dem_QueueTotalDropped = 0;
}

/**
 * rief   Reset the event queue
 */
void Dem_QueueReset(void)
{
    Dem_EnterCritical();
    
    memset(&Dem_EventQueue.Entries, 0, sizeof(Dem_EventQueue.Entries));
    Dem_EventQueue.Head = 0;
    Dem_EventQueue.Tail = 0;
    Dem_EventQueue.Count = 0;
    Dem_EventQueue.State = DEM_QUEUE_EMPTY;
    
    Dem_ExitCritical();
}

/**
 * rief   Enqueue an event (FIFO order)
 */
Std_ReturnType Dem_QueueEnqueue(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatus,
    uint8 Priority)
{
    Std_ReturnType result = E_NOT_OK;
    
    Dem_EnterCritical();
    
    if (Dem_EventQueue.Count < DEM_CFG_EVENT_QUEUE_SIZE) {
        Dem_QueueEntryType* entry = &Dem_EventQueue.Entries[Dem_EventQueue.Tail];
        
        entry->EventId = EventId;
        entry->EventStatus = EventStatus;
        entry->Priority = Priority;
        entry->Timestamp = Dem_GetCurrentTimestamp();
        entry->Valid = TRUE;
        
        Dem_EventQueue.Tail = Dem_QueueNextIndex(Dem_EventQueue.Tail);
        Dem_EventQueue.Count++;
        Dem_EventQueue.State = DEM_QUEUE_READY;
        Dem_QueueTotalEnqueued++;
        
        result = E_OK;
    } else {
        /* Queue overflow */
        Dem_EventQueue.State = DEM_QUEUE_OVERFLOW;
        Dem_EventQueue.OverflowCounter++;
        Dem_QueueTotalDropped++;
        
        /* Handle overflow - drop lowest priority entry if current is high priority */
        if (Priority == DEM_QUEUE_PRIORITY_HIGH) {
            /* Find and replace lowest priority entry */
            uint8 lowestPrioIdx = Dem_EventQueue.Head;
            uint8 lowestPrio = DEM_QUEUE_PRIORITY_LOW;
            uint8 idx = Dem_EventQueue.Head;
            uint8 count = Dem_EventQueue.Count;
            
            while (count > 0) {
                if (Dem_EventQueue.Entries[idx].Priority > lowestPrio) {
                    lowestPrio = Dem_EventQueue.Entries[idx].Priority;
                    lowestPrioIdx = idx;
                }
                idx = Dem_QueueNextIndex(idx);
                count--;
            }
            
            if (lowestPrio > Priority) {
                Dem_EventQueue.Entries[lowestPrioIdx].EventId = EventId;
                Dem_EventQueue.Entries[lowestPrioIdx].EventStatus = EventStatus;
                Dem_EventQueue.Entries[lowestPrioIdx].Priority = Priority;
                Dem_EventQueue.Entries[lowestPrioIdx].Timestamp = Dem_GetCurrentTimestamp();
                result = E_OK;
            }
        }
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Dequeue an event (FIFO order)
 */
Std_ReturnType Dem_QueueDequeue(Dem_QueueEntryType* Entry)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (Entry == NULL_PTR) {
        return E_NOT_OK;
    }
    
    Dem_EnterCritical();
    
    if (Dem_EventQueue.Count > 0) {
        *Entry = Dem_EventQueue.Entries[Dem_EventQueue.Head];
        Dem_EventQueue.Entries[Dem_EventQueue.Head].Valid = FALSE;
        
        Dem_EventQueue.Head = Dem_QueueNextIndex(Dem_EventQueue.Head);
        Dem_EventQueue.Count--;
        Dem_QueueTotalDequeued++;
        
        if (Dem_EventQueue.Count == 0) {
            Dem_EventQueue.State = DEM_QUEUE_EMPTY;
        }
        
        result = E_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Insert event with priority (higher priority first)
 */
Std_ReturnType Dem_QueueInsertPriority(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatus,
    uint8 Priority)
{
    Std_ReturnType result = E_NOT_OK;
    
    Dem_EnterCritical();
    
    if (Dem_EventQueue.Count < DEM_CFG_EVENT_QUEUE_SIZE) {
        uint8 insertPos = Dem_QueueFindInsertPosition(Priority);
        
        /* Shift entries to make room */
        Dem_QueueShiftEntries(insertPos);
        
        /* Insert new entry */
        Dem_EventQueue.Entries[insertPos].EventId = EventId;
        Dem_EventQueue.Entries[insertPos].EventStatus = EventStatus;
        Dem_EventQueue.Entries[insertPos].Priority = Priority;
        Dem_EventQueue.Entries[insertPos].Timestamp = Dem_GetCurrentTimestamp();
        Dem_EventQueue.Entries[insertPos].Valid = TRUE;
        
        Dem_EventQueue.Tail = Dem_QueueNextIndex(Dem_EventQueue.Tail);
        Dem_EventQueue.Count++;
        Dem_EventQueue.State = DEM_QUEUE_READY;
        Dem_QueueTotalEnqueued++;
        
        result = E_OK;
    } else {
        /* Queue full - use overflow handling */
        result = Dem_QueueEnqueue(EventId, EventStatus, Priority);
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Remove specific event from queue
 */
Std_ReturnType Dem_QueueRemoveEvent(Dem_EventIdType EventId)
{
    Std_ReturnType result = E_NOT_OK;
    
    Dem_EnterCritical();
    
    uint8 idx = Dem_EventQueue.Head;
    uint8 count = Dem_EventQueue.Count;
    
    while (count > 0) {
        if (Dem_EventQueue.Entries[idx].EventId == EventId && 
            Dem_EventQueue.Entries[idx].Valid) {
            
            /* Mark as invalid */
            Dem_EventQueue.Entries[idx].Valid = FALSE;
            
            /* Compact queue if entry is at head */
            if (idx == Dem_EventQueue.Head) {
                Dem_EventQueue.Head = Dem_QueueNextIndex(Dem_EventQueue.Head);
                Dem_EventQueue.Count--;
            }
            
            result = E_OK;
            break;
        }
        idx = Dem_QueueNextIndex(idx);
        count--;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Peek at next entry without removing
 */
Std_ReturnType Dem_QueuePeek(Dem_QueueEntryType* Entry)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (Entry == NULL_PTR) {
        return E_NOT_OK;
    }
    
    Dem_EnterCritical();
    
    if (Dem_EventQueue.Count > 0) {
        *Entry = Dem_EventQueue.Entries[Dem_EventQueue.Head];
        if (Entry->Valid) {
            result = E_OK;
        }
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Get queue count
 */
uint8 Dem_QueueGetCount(void)
{
    return Dem_EventQueue.Count;
}

/**
 * rief   Check if queue is full
 */
boolean Dem_QueueIsFull(void)
{
    return (Dem_EventQueue.Count >= DEM_CFG_EVENT_QUEUE_SIZE);
}

/**
 * rief   Check if queue is empty
 */
boolean Dem_QueueIsEmpty(void)
{
    return (Dem_EventQueue.Count == 0);
}

/**
 * rief   Get queue state
 */
Dem_QueueStateType Dem_QueueGetState(void)
{
    return Dem_EventQueue.State;
}

/**
 * rief   Get overflow count
 */
uint32 Dem_QueueGetOverflowCount(void)
{
    return Dem_EventQueue.OverflowCounter;
}

/**
 * rief   Process queue (called from main function)
 */
void Dem_QueueProcess(void)
{
    Dem_QueueEntryType entry;
    uint8 maxProcess = 5; /* Limit processing per cycle */
    
    while (maxProcess > 0 && Dem_QueueDequeue(&entry) == E_OK) {
        if (entry.Valid) {
            /* Process the event */
            Dem_ProcessEvent(entry.EventId, entry.EventStatus);
        }
        maxProcess--;
    }
}

/*==================================================================================================
 *                                      STATISTICS FUNCTIONS
==================================================================================================*/

/**
 * rief   Get queue statistics
 */
void Dem_QueueGetStats(uint32* TotalEnqueued, uint32* TotalDequeued, uint32* TotalDropped)
{
    if (TotalEnqueued != NULL_PTR) {
        *TotalEnqueued = Dem_QueueTotalEnqueued;
    }
    if (TotalDequeued != NULL_PTR) {
        *TotalDequeued = Dem_QueueTotalDequeued;
    }
    if (TotalDropped != NULL_PTR) {
        *TotalDropped = Dem_QueueTotalDropped;
    }
}

