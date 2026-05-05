/***********************************************************************************************************************
 * File:        dem_queue.h
 * Description: Dem Event Queue Management Header
 *              Provides asynchronous event processing with priority queue
 **********************************************************************************************************************/

#ifndef DEM_QUEUE_H
#define DEM_QUEUE_H

#include "Dem.h"
#include "Dem_Cfg.h"

/*==================================================================================================
 *                                      QUEUE CONFIGURATION
==================================================================================================*/

/* Queue priorities */
#define DEM_QUEUE_PRIORITY_HIGH      (0U)
#define DEM_QUEUE_PRIORITY_NORMAL    (1U)
#define DEM_QUEUE_PRIORITY_LOW       (2U)
#define DEM_QUEUE_NUM_PRIORITIES     (3U)

/* Queue states */
typedef enum {
    DEM_QUEUE_EMPTY = 0,
    DEM_QUEUE_READY,
    DEM_QUEUE_PROCESSING,
    DEM_QUEUE_OVERFLOW
} Dem_QueueStateType;

/* Queue entry structure */
typedef struct {
    Dem_EventIdType EventId;
    Dem_EventStatusType EventStatus;
    uint8 Priority;
    uint32 Timestamp;
    boolean Valid;
} Dem_QueueEntryType;

/* Queue control structure */
typedef struct {
    Dem_QueueEntryType Entries[DEM_CFG_EVENT_QUEUE_SIZE];
    uint8 Head;
    uint8 Tail;
    uint8 Count;
    Dem_QueueStateType State;
    uint32 OverflowCounter;
} Dem_EventQueueType;

/*==================================================================================================
 *                                      FUNCTION PROTOTYPES
==================================================================================================*/

/* Queue initialization */
extern void Dem_QueueInit(void);

/* Queue reset */
extern void Dem_QueueReset(void);

/* Enqueue event */
extern Std_ReturnType Dem_QueueEnqueue(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatus,
    uint8 Priority
);

/* Dequeue event */
extern Std_ReturnType Dem_QueueDequeue(Dem_QueueEntryType* Entry);

/* Get queue count */
extern uint8 Dem_QueueGetCount(void);

/* Check if queue is full */
extern boolean Dem_QueueIsFull(void);

/* Check if queue is empty */
extern boolean Dem_QueueIsEmpty(void);

/* Get queue state */
extern Dem_QueueStateType Dem_QueueGetState(void);

/* Process queue (called from main function) */
extern void Dem_QueueProcess(void);

/* Get overflow count */
extern uint32 Dem_QueueGetOverflowCount(void);

/* Insert event with priority (higher priority first) */
extern Std_ReturnType Dem_QueueInsertPriority(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatus,
    uint8 Priority
);

/* Remove specific event from queue */
extern Std_ReturnType Dem_QueueRemoveEvent(Dem_EventIdType EventId);

/* Peek at next entry without removing */
extern Std_ReturnType Dem_QueuePeek(Dem_QueueEntryType* Entry);

#endif /* DEM_QUEUE_H */
