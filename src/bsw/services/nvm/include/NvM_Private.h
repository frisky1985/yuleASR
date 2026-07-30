/*
 * NvM_Private.h - Private internal header for NvM module
 * AUTOSAR NvM private types and functions
 */
#ifndef NVM_PRIVATE_H
#define NVM_PRIVATE_H

#include "Std_Types.h"
#include "NvM.h"

/* NvM internal queue element */
typedef struct {
    NvM_BlockIdType BlockId;
    uint8 RequestType;
    uint8 Priority;
} NvM_InternalRequestType;

/* NvM internal state */
typedef struct {
    uint8 JobState;
    uint8 PendingRequests;
    NvM_InternalRequestType RequestQueue[4];
    uint8 QueueHead;
    uint8 QueueTail;
} NvM_InternalStateType;

/* Internal function declarations */
void NvM_InternalMainFunction(void);
void NvM_ProcessRequestQueue(void);
void NvM_HandleJobCompletion(uint8 JobId);

#endif /* NVM_PRIVATE_H */
