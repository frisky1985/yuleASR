/******************************************************************************
 * @file    sm_interface.h
 * @brief   State Manager Interface - Request Protocol
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef SM_INTERFACE_H
#define SM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sm_internal.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/

#define SM_INTERFACE_MAX_REQUESTS       8U
#define SM_INTERFACE_MAX_CLIENTS        16U
#define SM_INTERFACE_DEFAULT_TIMEOUT_MS 5000U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Request types */
typedef enum {
    SM_REQUEST_STATE_TRANSITION = 0,
    SM_REQUEST_FG_STATE_CHANGE,
    SM_REQUEST_RESET,
    SM_REQUEST_SHUTDOWN
} SM_RequestType;

/* Request handle */
typedef uint32_t SM_RequestHandle;

/* Request status */
typedef enum {
    SM_REQUEST_STATUS_PENDING = 0,
    SM_REQUEST_STATUS_ACCEPTED,
    SM_REQUEST_STATUS_REJECTED,
    SM_REQUEST_STATUS_EXECUTING,
    SM_REQUEST_STATUS_COMPLETED,
    SM_REQUEST_STATUS_FAILED,
    SM_REQUEST_STATUS_TIMEOUT,
    SM_REQUEST_STATUS_CANCELLED
} SM_RequestStatus;

/* Request structure */
typedef struct {
    SM_RequestHandle handle;
    SM_RequestType type;
    SM_RequestStatus status;
    uint32_t clientId;
    uint32_t requestTime;
    uint32_t timeoutMs;
    union {
        MachineStateType targetState;
        struct {
            FunctionGroupNameType fgName;
            FunctionGroupStateType fgState;
        } fgRequest;
    } data;
    void (*completionCallback)(SM_RequestHandle, SM_RequestStatus);
} SM_RequestType;

/* Client registration */
typedef struct {
    uint32_t clientId;
    char name[32];
    bool isActive;
    uint32_t requestCount;
} SM_ClientType;

/* Response structure */
typedef struct {
    SM_RequestHandle handle;
    SM_RequestStatus status;
    int32_t errorCode;
    char errorMessage[128];
} SM_ResponseType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/* Initialize SM interface */
Std_ReturnType SM_Interface_Init(void);

/* Deinitialize SM interface */
Std_ReturnType SM_Interface_Deinit(void);

/* Check if initialized */
bool SM_Interface_IsInitialized(void);

/* Register a client */
Std_ReturnType SM_RegisterClient(const char* name, uint32_t* clientId);

/* Unregister a client */
Std_ReturnType SM_UnregisterClient(uint32_t clientId);

/* Create state transition request */
SM_RequestHandle SM_CreateStateRequest(uint32_t clientId, MachineStateType targetState);

/* Create function group state change request */
SM_RequestHandle SM_CreateFGStateRequest(uint32_t clientId, 
                                          FunctionGroupNameType fg,
                                          FunctionGroupStateType state);

/* Submit a request */
Std_ReturnType SM_SubmitRequest(SM_RequestHandle handle);

/* Cancel a request */
Std_ReturnType SM_CancelRequest(SM_RequestHandle handle);

/* Get request status */
SM_RequestStatus SM_GetRequestStatus(SM_RequestHandle handle);

/* Confirm a request (positive acknowledgment) */
Std_ReturnType SM_ConfirmRequest(SM_RequestHandle handle);

/* Reject a request */
Std_ReturnType SM_RejectRequest(SM_RequestHandle handle, int32_t errorCode, 
                                const char* errorMessage);

/* Complete a request */
Std_ReturnType SM_CompleteRequest(SM_RequestHandle handle, bool success);

/* Set request completion callback */
Std_ReturnType SM_SetCompletionCallback(SM_RequestHandle handle,
                                        void (*callback)(SM_RequestHandle, SM_RequestStatus));

/* Wait for request completion (blocking) */
SM_RequestStatus SM_WaitForCompletion(SM_RequestHandle handle, uint32_t timeoutMs);

/* Get request details */
Std_ReturnType SM_GetRequest(SM_RequestHandle handle, SM_RequestType* request);

/* Get response */
Std_ReturnType SM_GetResponse(SM_RequestHandle handle, SM_ResponseType* response);

/* Process requests (call periodically) */
void SM_Interface_MainFunction(void);

/* Get active request count */
uint32_t SM_GetActiveRequestCount(void);

/* Cancel all requests for a client */
Std_ReturnType SM_CancelClientRequests(uint32_t clientId);

#ifdef __cplusplus
}
#endif

#endif /* SM_INTERFACE_H */
