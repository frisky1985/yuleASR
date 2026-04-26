/******************************************************************************
 * @file    sm_interface.c
 * @brief   State Manager Interface - Request Protocol Implementation
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ara/sm/sm_interface.h"
#include <string.h>

/******************************************************************************
 * Private Data
 ******************************************************************************/

static bool g_initialized = false;
static SM_RequestType g_requests[SM_INTERFACE_MAX_REQUESTS];
static SM_ClientType g_clients[SM_INTERFACE_MAX_CLIENTS];
static SM_RequestHandle g_nextHandle = 1U;

/******************************************************************************
 * Private Functions
 ******************************************************************************/

static SM_RequestType* FindRequest(SM_RequestHandle handle) {
    for (uint32_t i = 0U; i < SM_INTERFACE_MAX_REQUESTS; i++) {
        if (g_requests[i].handle == handle) {
            return &g_requests[i];
        }
    }
    return NULL;
}

static SM_RequestType* FindFreeRequestSlot(void) {
    for (uint32_t i = 0U; i < SM_INTERFACE_MAX_REQUESTS; i++) {
        if (g_requests[i].handle == 0U) {
            return &g_requests[i];
        }
    }
    return NULL;
}

static SM_ClientType* FindClient(uint32_t clientId) {
    for (uint32_t i = 0U; i < SM_INTERFACE_MAX_CLIENTS; i++) {
        if (g_clients[i].clientId == clientId && g_clients[i].isActive) {
            return &g_clients[i];
        }
    }
    return NULL;
}

static SM_ClientType* FindFreeClientSlot(void) {
    for (uint32_t i = 0U; i < SM_INTERFACE_MAX_CLIENTS; i++) {
        if (!g_clients[i].isActive) {
            return &g_clients[i];
        }
    }
    return NULL;
}

static void ResetRequest(SM_RequestType* request) {
    if (request != NULL) {
        memset(request, 0, sizeof(SM_RequestType));
    }
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Std_ReturnType SM_Interface_Init(void) {
    if (g_initialized) {
        return E_OK;
    }
    
    memset(g_requests, 0, sizeof(g_requests));
    memset(g_clients, 0, sizeof(g_clients));
    g_nextHandle = 1U;
    
    g_initialized = true;
    return E_OK;
}

Std_ReturnType SM_Interface_Deinit(void) {
    if (!g_initialized) {
        return E_OK;
    }
    
    /* Cancel all pending requests */
    for (uint32_t i = 0U; i < SM_INTERFACE_MAX_REQUESTS; i++) {
        if (g_requests[i].handle != 0U) {
            SM_CancelRequest(g_requests[i].handle);
        }
    }
    
    memset(g_requests, 0, sizeof(g_requests));
    memset(g_clients, 0, sizeof(g_clients));
    g_initialized = false;
    
    return E_OK;
}

bool SM_Interface_IsInitialized(void) {
    return g_initialized;
}

Std_ReturnType SM_RegisterClient(const char* name, uint32_t* clientId) {
    if (!g_initialized || name == NULL || clientId == NULL) {
        return E_NOT_OK;
    }
    
    SM_ClientType* client = FindFreeClientSlot();
    if (client == NULL) {
        return E_NOT_OK;  /* No free slots */
    }
    
    /* Generate client ID */
    static uint32_t nextClientId = 1U;
    
    client->clientId = nextClientId++;
    strncpy(client->name, name, sizeof(client->name) - 1U);
    client->name[sizeof(client->name) - 1U] = '\0';
    client->isActive = true;
    client->requestCount = 0U;
    
    *clientId = client->clientId;
    return E_OK;
}

Std_ReturnType SM_UnregisterClient(uint32_t clientId) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    SM_ClientType* client = FindClient(clientId);
    if (client == NULL) {
        return E_NOT_OK;
    }
    
    /* Cancel all requests for this client */
    SM_CancelClientRequests(clientId);
    
    client->isActive = false;
    return E_OK;
}

SM_RequestHandle SM_CreateStateRequest(uint32_t clientId, MachineStateType targetState) {
    if (!g_initialized) {
        return 0U;
    }
    
    if (FindClient(clientId) == NULL) {
        return 0U;
    }
    
    SM_RequestType* request = FindFreeRequestSlot();
    if (request == NULL) {
        return 0U;
    }
    
    request->handle = g_nextHandle++;
    request->type = SM_REQUEST_STATE_TRANSITION;
    request->status = SM_REQUEST_STATUS_PENDING;
    request->clientId = clientId;
    request->requestTime = 0U;  /* Would use actual timestamp */
    request->timeoutMs = SM_INTERFACE_DEFAULT_TIMEOUT_MS;
    request->data.targetState = targetState;
    request->completionCallback = NULL;
    
    return request->handle;
}

SM_RequestHandle SM_CreateFGStateRequest(uint32_t clientId,
                                          FunctionGroupNameType fg,
                                          FunctionGroupStateType state) {
    if (!g_initialized) {
        return 0U;
    }
    
    if (FindClient(clientId) == NULL) {
        return 0U;
    }
    
    SM_RequestType* request = FindFreeRequestSlot();
    if (request == NULL) {
        return 0U;
    }
    
    request->handle = g_nextHandle++;
    request->type = SM_REQUEST_FG_STATE_CHANGE;
    request->status = SM_REQUEST_STATUS_PENDING;
    request->clientId = clientId;
    request->requestTime = 0U;
    request->timeoutMs = SM_INTERFACE_DEFAULT_TIMEOUT_MS;
    request->data.fgRequest.fgName = fg;
    request->data.fgRequest.fgState = state;
    request->completionCallback = NULL;
    
    return request->handle;
}

Std_ReturnType SM_SubmitRequest(SM_RequestHandle handle) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    SM_RequestType* request = FindRequest(handle);
    if (request == NULL) {
        return E_NOT_OK;
    }
    
    if (request->status != SM_REQUEST_STATUS_PENDING) {
        return E_NOT_OK;
    }
    
    /* Update client request count */
    SM_ClientType* client = FindClient(request->clientId);
    if (client != NULL) {
        client->requestCount++;
    }
    
    /* Process the request based on type */
    switch (request->type) {
        case SM_REQUEST_STATE_TRANSITION: {
            StateRequestResultType result = 
                StateMachine_RequestTransition(request->data.targetState);
            
            if (result == STATE_REQUEST_ACCEPTED) {
                request->status = SM_REQUEST_STATUS_ACCEPTED;
            } else if (result == STATE_REQUEST_REJECTED) {
                request->status = SM_REQUEST_STATUS_REJECTED;
            } else {
                request->status = SM_REQUEST_STATUS_FAILED;
            }
            break;
        }
        
        case SM_REQUEST_FG_STATE_CHANGE: {
            Std_ReturnType result = StateMachine_SetFGState(
                request->data.fgRequest.fgName,
                request->data.fgRequest.fgState);
            
            request->status = (result == E_OK) ? SM_REQUEST_STATUS_ACCEPTED 
                                               : SM_REQUEST_STATUS_REJECTED;
            break;
        }
        
        default:
            request->status = SM_REQUEST_STATUS_FAILED;
            return E_NOT_OK;
    }
    
    return E_OK;
}

Std_ReturnType SM_CancelRequest(SM_RequestHandle handle) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    SM_RequestType* request = FindRequest(handle);
    if (request == NULL) {
        return E_NOT_OK;
    }
    
    if (request->status == SM_REQUEST_STATUS_COMPLETED ||
        request->status == SM_REQUEST_STATUS_FAILED ||
        request->status == SM_REQUEST_STATUS_CANCELLED) {
        return E_NOT_OK;  /* Already finished */
    }
    
    /* Update client request count */
    SM_ClientType* client = FindClient(request->clientId);
    if (client != NULL && client->requestCount > 0U) {
        client->requestCount--;
    }
    
    request->status = SM_REQUEST_STATUS_CANCELLED;
    
    /* Notify callback */
    if (request->completionCallback != NULL) {
        request->completionCallback(handle, SM_REQUEST_STATUS_CANCELLED);
    }
    
    /* Free the request slot */
    ResetRequest(request);
    
    return E_OK;
}

SM_RequestStatus SM_GetRequestStatus(SM_RequestHandle handle) {
    if (!g_initialized) {
        return SM_REQUEST_STATUS_FAILED;
    }
    
    SM_RequestType* request = FindRequest(handle);
    if (request == NULL) {
        return SM_REQUEST_STATUS_FAILED;
    }
    
    return request->status;
}

Std_ReturnType SM_ConfirmRequest(SM_RequestHandle handle) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    SM_RequestType* request = FindRequest(handle);
    if (request == NULL) {
        return E_NOT_OK;
    }
    
    if (request->status != SM_REQUEST_STATUS_ACCEPTED &&
        request->status != SM_REQUEST_STATUS_EXECUTING) {
        return E_NOT_OK;
    }
    
    /* Complete the request */
    request->status = SM_REQUEST_STATUS_COMPLETED;
    
    /* Notify callback */
    if (request->completionCallback != NULL) {
        request->completionCallback(handle, SM_REQUEST_STATUS_COMPLETED);
    }
    
    /* Update client request count */
    SM_ClientType* client = FindClient(request->clientId);
    if (client != NULL && client->requestCount > 0U) {
        client->requestCount--;
    }
    
    /* Free the request slot */
    ResetRequest(request);
    
    return E_OK;
}

Std_ReturnType SM_RejectRequest(SM_RequestHandle handle, int32_t errorCode,
                                const char* errorMessage) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    (void)errorCode;
    (void)errorMessage;
    
    SM_RequestType* request = FindRequest(handle);
    if (request == NULL) {
        return E_NOT_OK;
    }
    
    /* Update client request count */
    SM_ClientType* client = FindClient(request->clientId);
    if (client != NULL && client->requestCount > 0U) {
        client->requestCount--;
    }
    
    request->status = SM_REQUEST_STATUS_REJECTED;
    
    /* Notify callback */
    if (request->completionCallback != NULL) {
        request->completionCallback(handle, SM_REQUEST_STATUS_REJECTED);
    }
    
    /* Free the request slot */
    ResetRequest(request);
    
    return E_OK;
}

Std_ReturnType SM_CompleteRequest(SM_RequestHandle handle, bool success) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    SM_RequestType* request = FindRequest(handle);
    if (request == NULL) {
        return E_NOT_OK;
    }
    
    /* Update client request count */
    SM_ClientType* client = FindClient(request->clientId);
    if (client != NULL && client->requestCount > 0U) {
        client->requestCount--;
    }
    
    request->status = success ? SM_REQUEST_STATUS_COMPLETED : SM_REQUEST_STATUS_FAILED;
    
    /* Notify callback */
    if (request->completionCallback != NULL) {
        request->completionCallback(handle, request->status);
    }
    
    /* Free the request slot */
    ResetRequest(request);
    
    return E_OK;
}

Std_ReturnType SM_SetCompletionCallback(SM_RequestHandle handle,
                                        void (*callback)(SM_RequestHandle, SM_RequestStatus)) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    SM_RequestType* request = FindRequest(handle);
    if (request == NULL) {
        return E_NOT_OK;
    }
    
    request->completionCallback = callback;
    return E_OK;
}

SM_RequestStatus SM_WaitForCompletion(SM_RequestHandle handle, uint32_t timeoutMs) {
    if (!g_initialized) {
        return SM_REQUEST_STATUS_FAILED;
    }
    
    SM_RequestType* request = FindRequest(handle);
    if (request == NULL) {
        return SM_REQUEST_STATUS_FAILED;
    }
    
    uint32_t startTime = 0U;  /* Would use actual timestamp */
    
    while (request->status == SM_REQUEST_STATUS_PENDING ||
           request->status == SM_REQUEST_STATUS_ACCEPTED ||
           request->status == SM_REQUEST_STATUS_EXECUTING) {
        
        uint32_t currentTime = 0U;
        if (currentTime - startTime > timeoutMs) {
            request->status = SM_REQUEST_STATUS_TIMEOUT;
            return SM_REQUEST_STATUS_TIMEOUT;
        }
        
        /* In real implementation, would yield/sleep */
    }
    
    return request->status;
}

Std_ReturnType SM_GetRequest(SM_RequestHandle handle, SM_RequestType* request) {
    if (!g_initialized || request == NULL) {
        return E_NOT_OK;
    }
    
    SM_RequestType* req = FindRequest(handle);
    if (req == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(request, req, sizeof(SM_RequestType));
    return E_OK;
}

Std_ReturnType SM_GetResponse(SM_RequestHandle handle, SM_ResponseType* response) {
    if (!g_initialized || response == NULL) {
        return E_NOT_OK;
    }
    
    SM_RequestType* request = FindRequest(handle);
    if (request == NULL) {
        return E_NOT_OK;
    }
    
    response->handle = handle;
    response->status = request->status;
    response->errorCode = 0;
    memset(response->errorMessage, 0, sizeof(response->errorMessage));
    
    return E_OK;
}

void SM_Interface_MainFunction(void) {
    if (!g_initialized) {
        return;
    }
    
    uint32_t currentTime = 0U;  /* Would use actual timestamp */
    
    /* Check for timed out requests */
    for (uint32_t i = 0U; i < SM_INTERFACE_MAX_REQUESTS; i++) {
        SM_RequestType* request = &g_requests[i];
        
        if (request->handle == 0U) {
            continue;
        }
        
        if (request->status != SM_REQUEST_STATUS_PENDING &&
            request->status != SM_REQUEST_STATUS_ACCEPTED &&
            request->status != SM_REQUEST_STATUS_EXECUTING) {
            continue;
        }
        
        if (currentTime - request->requestTime > request->timeoutMs) {
            /* Request timed out */
            request->status = SM_REQUEST_STATUS_TIMEOUT;
            
            /* Update client request count */
            SM_ClientType* client = FindClient(request->clientId);
            if (client != NULL && client->requestCount > 0U) {
                client->requestCount--;
            }
            
            /* Notify callback */
            if (request->completionCallback != NULL) {
                request->completionCallback(request->handle, SM_REQUEST_STATUS_TIMEOUT);
            }
            
            /* Free the request slot */
            ResetRequest(request);
        }
    }
}

uint32_t SM_GetActiveRequestCount(void) {
    if (!g_initialized) {
        return 0U;
    }
    
    uint32_t count = 0U;
    for (uint32_t i = 0U; i < SM_INTERFACE_MAX_REQUESTS; i++) {
        if (g_requests[i].handle != 0U &&
            (g_requests[i].status == SM_REQUEST_STATUS_PENDING ||
             g_requests[i].status == SM_REQUEST_STATUS_ACCEPTED ||
             g_requests[i].status == SM_REQUEST_STATUS_EXECUTING)) {
            count++;
        }
    }
    return count;
}

Std_ReturnType SM_CancelClientRequests(uint32_t clientId) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    if (FindClient(clientId) == NULL) {
        return E_NOT_OK;
    }
    
    for (uint32_t i = 0U; i < SM_INTERFACE_MAX_REQUESTS; i++) {
        if (g_requests[i].handle != 0U && g_requests[i].clientId == clientId) {
            SM_CancelRequest(g_requests[i].handle);
        }
    }
    
    return E_OK;
}
