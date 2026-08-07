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

/*==================================================================================================
* Project              : YuleASR - AUTOSAR Services Layer
* Platform             : ARM Cortex-M / x86 Simulation
* Peripheral           : N/A
* Dependencies         : RTE, Std_Types, Swc_Cfg
*
* SW Version           : 1.0.0
* Created              : JAN-2025
*==================================================================================================*/
#include "Swc.h"
#include "Swc_Cfg.h"
#include "Det.h"

/*==================================================================================================
*                                      LOCAL MACROS
==================================================================================================*/
#define SWC_INSTANCE_ID                  0x00U

/*==================================================================================================
*                                    LOCAL DATA TYPES
==================================================================================================*/

typedef struct
{
    Swc_EventHandleType eventHandle;
    Swc_ComponentHandleType componentHandle;
    uint32 timestamp;
} Swc_EventQueueEntryType;

typedef struct
{
    Swc_ComponentInstanceType instances[SWC_MAX_COMPONENT_INSTANCES];
    Swc_RunnableEntityType runnables[SWC_MAX_COMPONENT_INSTANCES * SWC_MAX_RUNNABLES_PER_COMPONENT];
    Swc_EventQueueEntryType eventQueue[SWC_EVENT_QUEUE_SIZE];
    uint8 eventQueueHead;
    uint8 eventQueueTail;
    uint8 eventQueueCount;
    boolean initialized;
    uint32 cycleCounter;
} Swc_InternalDataType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
static Swc_InternalDataType Swc_InternalData;
static const Swc_ConfigType* Swc_CurrentConfig = NULL_PTR;

/*==================================================================================================
*                               LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static void Swc_InternalInitInstances(void);
static void Swc_InternalInitRunnables(void);
static void Swc_InternalInitEventQueue(void);
static Std_ReturnType Swc_ValidateComponentHandle(Swc_ComponentHandleType handle);
static Std_ReturnType Swc_ValidateRunnableHandle(Swc_RunnableHandleType handle);
static void Swc_ExecuteRunnable(Swc_RunnableHandleType runnableHandle);
static void Swc_ProcessQueuedEvents(void);
static Swc_RunnableHandleType Swc_FindRunnableById(uint16 runnableId);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/

static void Swc_InternalInitInstances(void)
{
    uint16 i;
    for (i = 0U; i < SWC_MAX_COMPONENT_INSTANCES; i++)
    {
        Swc_InternalData.instances[i].handle = i;
        Swc_InternalData.instances[i].state = SWC_STATE_UNINIT;
        Swc_InternalData.instances[i].config = NULL_PTR;
        Swc_InternalData.instances[i].instanceData = NULL_PTR;
        Swc_InternalData.instances[i].portCount = 0U;
        Swc_InternalData.instances[i].runnableCount = 0U;
    }
}

static void Swc_InternalInitRunnables(void)
{
    uint16 i;
    uint16 maxRunnables = SWC_MAX_COMPONENT_INSTANCES * SWC_MAX_RUNNABLES_PER_COMPONENT;
    
    for (i = 0U; i < maxRunnables; i++)
    {
        Swc_InternalData.runnables[i].handle = i;
        Swc_InternalData.runnables[i].state = SWC_RUNNABLE_IDLE;
        Swc_InternalData.runnables[i].config = NULL_PTR;
        Swc_InternalData.runnables[i].ownerComponent = 0xFFFFU;
        Swc_InternalData.runnables[i].executionCounter = 0U;
        Swc_InternalData.runnables[i].lastExecutionTime = 0U;
    }
}

static void Swc_InternalInitEventQueue(void)
{
    Swc_InternalData.eventQueueHead = 0U;
    Swc_InternalData.eventQueueTail = 0U;
    Swc_InternalData.eventQueueCount = 0U;
}

static Std_ReturnType Swc_ValidateComponentHandle(Swc_ComponentHandleType handle)
{
    Std_ReturnType ret = E_NOT_OK;
    
    if (handle < SWC_MAX_COMPONENT_INSTANCES)
    {
        if (Swc_InternalData.instances[handle].state != SWC_STATE_UNINIT)
        {
            ret = E_OK;
        }
    }
    
    return ret;
}

static Std_ReturnType Swc_ValidateRunnableHandle(Swc_RunnableHandleType handle)
{
    Std_ReturnType ret = E_NOT_OK;
    uint16 maxRunnables = SWC_MAX_COMPONENT_INSTANCES * SWC_MAX_RUNNABLES_PER_COMPONENT;
    
    if (handle < maxRunnables)
    {
        if (Swc_InternalData.runnables[handle].config != NULL_PTR)
        {
            ret = E_OK;
        }
    }
    
    return ret;
}

static void Swc_ExecuteRunnable(Swc_RunnableHandleType runnableHandle)
{
    Swc_RunnableEntityType* runnable = &Swc_InternalData.runnables[runnableHandle];
    
    if ((runnable->state == SWC_RUNNABLE_READY) || (runnable->state == SWC_RUNNABLE_IDLE))
    {
        runnable->state = SWC_RUNNABLE_RUNNING;
        runnable->executionCounter++;
        
        if (runnable->config != NULL_PTR)
        {
            if (runnable->config->runnableFunc != NULL_PTR)
            {
                runnable->config->runnableFunc();
            }
        }
        
        runnable->state = SWC_RUNNABLE_COMPLETED;
        runnable->lastExecutionTime = Swc_InternalData.cycleCounter;
    }
}

static void Swc_ProcessQueuedEvents(void)
{
    while (Swc_InternalData.eventQueueCount > 0U)
    {
        const Swc_EventQueueEntryType* entry ;
        
        /* Process the event - trigger associated runnable */
        /* In a real implementation, this would look up the event and trigger the runnable */
        
        /* Advance queue */
        Swc_InternalData.eventQueueHead = (Swc_InternalData.eventQueueHead + 1U) % SWC_EVENT_QUEUE_SIZE;
        Swc_InternalData.eventQueueCount--;
    }
}

static Swc_RunnableHandleType Swc_FindRunnableById(uint16 runnableId)
{
    Swc_RunnableHandleType handle = 0xFFFFU;
    uint16 i;
    uint16 maxRunnables = SWC_MAX_COMPONENT_INSTANCES * SWC_MAX_RUNNABLES_PER_COMPONENT;
    
    for (i = 0U; i < maxRunnables; i++)
    {
        if ((Swc_InternalData.runnables[i].config != NULL_PTR) &&
            (Swc_InternalData.runnables[i].config->runnableId == runnableId))
        {
            handle = i;
            break;
        }
    }
    
    return handle;
}

/*==================================================================================================
*                                     GLOBAL FUNCTIONS
==================================================================================================*/

void Swc_Init(const Swc_ConfigType* ConfigPtr)
{
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if ((Swc_InternalData.initialized) != 0U)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, SWC_INIT_SERVICE_ID, SWC_E_ALREADY_INITIALIZED);
        return;
    }
    
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, SWC_INIT_SERVICE_ID, SWC_E_PARAM_POINTER);
        return;
    }
    #endif
    
    Swc_CurrentConfig = ConfigPtr;
    Swc_InternalData.cycleCounter = 0U;
    
    /* Initialize internal data structures */
    Swc_InternalInitInstances();
    Swc_InternalInitRunnables();
    Swc_InternalInitEventQueue();
    
    Swc_InternalData.initialized = TRUE;
}

void Swc_DeInit(void)
{
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if (!Swc_InternalData.initialized)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, SWC_DEINIT_SERVICE_ID, SWC_E_UNINIT);
        return;
    }
    #endif
    
    Swc_InternalData.initialized = FALSE;
    Swc_CurrentConfig = NULL_PTR;
    
    /* Clean up all instances */
    Swc_InternalInitInstances();
    Swc_InternalInitRunnables();
    Swc_InternalInitEventQueue();
}

Std_ReturnType Swc_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, SWC_GETVERSIONINFO_SERVICE_ID, SWC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    versioninfo->vendorID = SWC_VENDOR_ID;
    versioninfo->moduleID = SWC_MODULE_ID;
    versioninfo->sw_major_version = SWC_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SWC_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SWC_SW_PATCH_VERSION;
    
    return E_OK;
}

Std_ReturnType Swc_CreateInstance(Swc_ComponentHandleType componentId, 
                                   void* instanceData,
                                   Swc_ComponentHandleType* outHandle)
{
    Std_ReturnType ret = E_NOT_OK;
    uint16 i;
    
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if (!Swc_InternalData.initialized)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, 0x10U, SWC_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (outHandle == NULL_PTR)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, 0x10U, SWC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (Swc_CurrentConfig == NULL_PTR)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, 0x10U, SWC_E_INVALID_COMPONENT);
        return E_NOT_OK;
    }
    
    if (componentId >= Swc_CurrentConfig->numComponents)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, 0x10U, SWC_E_INVALID_COMPONENT);
        return E_NOT_OK;
    }
    #endif
    
    /* Find free instance slot */
    for (i = 0U; i < SWC_MAX_COMPONENT_INSTANCES; i++)
    {
        if (Swc_InternalData.instances[i].state == SWC_STATE_UNINIT)
        {
            Swc_InternalData.instances[i].config = &Swc_CurrentConfig->componentConfigs[componentId];
            Swc_InternalData.instances[i].instanceData = instanceData;
            Swc_InternalData.instances[i].state = SWC_STATE_INIT;
            Swc_InternalData.instances[i].portCount = Swc_InternalData.instances[i].config->numPorts;
            Swc_InternalData.instances[i].runnableCount = Swc_InternalData.instances[i].config->numRunnables;
            
            *outHandle = i;
            ret = E_OK;
            break;
        }
    }
    
    return ret;
}

Std_ReturnType Swc_DestroyInstance(Swc_ComponentHandleType handle)
{
    Std_ReturnType ret;
    
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if (!Swc_InternalData.initialized)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, 0x11U, SWC_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    ret = Swc_ValidateComponentHandle(handle);
    
    if (ret == E_OK)
    {
        Swc_InternalData.instances[handle].state = SWC_STATE_UNINIT;
        Swc_InternalData.instances[handle].config = NULL_PTR;
        Swc_InternalData.instances[handle].instanceData = NULL_PTR;
        Swc_InternalData.instances[handle].portCount = 0U;
        Swc_InternalData.instances[handle].runnableCount = 0U;
    }
    
    return ret;
}

Std_ReturnType Swc_SetComponentState(Swc_ComponentHandleType handle, Swc_StateType newState)
{
    Std_ReturnType ret;
    
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if (!Swc_InternalData.initialized)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, 0x12U, SWC_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    ret = Swc_ValidateComponentHandle(handle);
    
    if (ret == E_OK)
    {
        Swc_InternalData.instances[handle].state = newState;
    }
    
    return ret;
}

Swc_StateType Swc_GetComponentState(Swc_ComponentHandleType handle)
{
    Swc_StateType state = SWC_STATE_UNINIT;
    
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if (!Swc_InternalData.initialized)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, 0x13U, SWC_E_UNINIT);
        return SWC_STATE_ERROR;
    }
    #endif
    
    if (Swc_ValidateComponentHandle(handle) == E_OK)
    {
        state = Swc_InternalData.instances[handle].state;
    }
    
    return state;
}

Std_ReturnType Swc_ActivateRunnable(Swc_RunnableHandleType runnableHandle)
{
    Std_ReturnType ret;
    
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if (!Swc_InternalData.initialized)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, SWC_ACTIVATERUNNABLE_SERVICE_ID, SWC_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    ret = Swc_ValidateRunnableHandle(runnableHandle);
    
    if (ret == E_OK)
    {
        if (Swc_InternalData.runnables[runnableHandle].state == SWC_RUNNABLE_IDLE)
        {
            Swc_InternalData.runnables[runnableHandle].state = SWC_RUNNABLE_READY;
        }
    }
    
    return ret;
}

Std_ReturnType Swc_TerminateRunnable(Swc_RunnableHandleType runnableHandle)
{
    Std_ReturnType ret;
    
    ret = Swc_ValidateRunnableHandle(runnableHandle);
    
    if (ret == E_OK)
    {
        Swc_InternalData.runnables[runnableHandle].state = SWC_RUNNABLE_IDLE;
    }
    
    return ret;
}

void Swc_ScheduleRunnables(void)
{
    uint16 i;
    uint16 maxRunnables = SWC_MAX_COMPONENT_INSTANCES * SWC_MAX_RUNNABLES_PER_COMPONENT;
    
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if (!Swc_InternalData.initialized)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, 0x14U, SWC_E_UNINIT);
        return;
    }
    #endif
    
    for (i = 0U; i < maxRunnables; i++)
    {
        if (Swc_InternalData.runnables[i].state == SWC_RUNNABLE_READY)
        {
            Swc_ExecuteRunnable(i);
        }
    }
}

Std_ReturnType Swc_IsRunnableReady(Swc_RunnableHandleType runnableHandle, boolean* isReady)
{
    Std_ReturnType ret;
    
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if (isReady == NULL_PTR)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, 0x15U, SWC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    ret = Swc_ValidateRunnableHandle(runnableHandle);
    
    if (ret == E_OK)
    {
        *isReady = (Swc_InternalData.runnables[runnableHandle].state == SWC_RUNNABLE_READY) ? TRUE : FALSE;
    }
    
    return ret;
}

Std_ReturnType Swc_ConnectPort(Swc_PortHandleType portHandle, const void* connectionData)
{
    /* Placeholder implementation */
    (void)portHandle;
    (void)connectionData;
    return E_OK;
}

Std_ReturnType Swc_DisconnectPort(Swc_PortHandleType portHandle)
{
    /* Placeholder implementation */
    (void)portHandle;
    return E_OK;
}

Std_ReturnType Swc_WritePortData(Swc_PortHandleType portHandle, const void* data, uint16 length)
{
    /* Placeholder implementation */
    (void)portHandle;
    (void)data;
    (void)length;
    return E_OK;
}

Std_ReturnType Swc_ReadPortData(Swc_PortHandleType portHandle, void* data, uint16* length)
{
    /* Placeholder implementation */
    (void)portHandle;
    (void)data;
    (void)length;
    return E_OK;
}

Std_ReturnType Swc_RegisterEvent(Swc_ComponentHandleType compHandle, const Swc_RteEventType* eventConfig)
{
    /* Placeholder implementation */
    (void)compHandle;
    (void)eventConfig;
    return E_OK;
}

Std_ReturnType Swc_TriggerEvent(Swc_EventHandleType eventHandle)
{
    Std_ReturnType ret = E_NOT_OK;
    
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if (!Swc_InternalData.initialized)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, SWC_TRIGGEREVENT_SERVICE_ID, SWC_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    if (Swc_InternalData.eventQueueCount < SWC_EVENT_QUEUE_SIZE)
    {
        Swc_InternalData.eventQueue[Swc_InternalData.eventQueueTail].eventHandle = eventHandle;
        Swc_InternalData.eventQueue[Swc_InternalData.eventQueueTail].timestamp = Swc_InternalData.cycleCounter;
        
        Swc_InternalData.eventQueueTail = (Swc_InternalData.eventQueueTail + 1U) % SWC_EVENT_QUEUE_SIZE;
        Swc_InternalData.eventQueueCount++;
        
        ret = E_OK;
    }
    else
    {
        #if (SWC_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, SWC_TRIGGEREVENT_SERVICE_ID, SWC_E_EVENT_QUEUE_FULL);
        #endif
    }
    
    return ret;
}

Std_ReturnType Swc_EnableEvent(Swc_EventHandleType eventHandle)
{
    /* Placeholder implementation */
    (void)eventHandle;
    return E_OK;
}

Std_ReturnType Swc_DisableEvent(Swc_EventHandleType eventHandle)
{
    /* Placeholder implementation */
    (void)eventHandle;
    return E_OK;
}

void Swc_ProcessEvents(void)
{
    #if (SWC_DEV_ERROR_DETECT == STD_ON)
    if (!Swc_InternalData.initialized)
    {
        Det_ReportError(SWC_MODULE_ID, SWC_INSTANCE_ID, 0x16U, SWC_E_UNINIT);
        return;
    }
    #endif
    
    Swc_ProcessQueuedEvents();
}

void Swc_MainFunction(void)
{
    if ((Swc_InternalData.initialized) != 0U)
    {
        Swc_InternalData.cycleCounter++;
        
        /* Process events */
        Swc_ProcessEvents();
        
        /* Schedule runnables */
        Swc_ScheduleRunnables();
    }
}
