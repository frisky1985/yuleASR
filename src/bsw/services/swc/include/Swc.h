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
#ifndef SWC_H
#define SWC_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Swc_Cfg.h"

/*==================================================================================================
*                                    VERSION DEFINITIONS
==================================================================================================*/
#define SWC_VENDOR_ID                    0x01U
#define SWC_MODULE_ID                    0x50U
#define SWC_AR_MAJOR_VERSION             4U
#define SWC_AR_MINOR_VERSION             4U
#define SWC_AR_PATCH_VERSION             0U
#define SWC_SW_MAJOR_VERSION             1U
#define SWC_SW_MINOR_VERSION             0U
#define SWC_SW_PATCH_VERSION             0U

/*==================================================================================================
*                                     SERVICE IDs
==================================================================================================*/
#define SWC_INIT_SERVICE_ID              0x01U
#define SWC_DEINIT_SERVICE_ID            0x02U
#define SWC_RUNNABLE_SERVICE_ID          0x03U
#define SWC_GETVERSIONINFO_SERVICE_ID    0x04U
#define SWC_ACTIVATERUNNABLE_SERVICE_ID  0x05U
#define SWC_TRIGGEREVENT_SERVICE_ID      0x06U

/*==================================================================================================
*                                     ERROR CODES
==================================================================================================*/
#define SWC_E_PARAM_POINTER              0x01U
#define SWC_E_INVALID_COMPONENT          0x02U
#define SWC_E_INVALID_RUNNABLE           0x03U
#define SWC_E_INVALID_PORT               0x04U
#define SWC_E_UNINIT                     0x05U
#define SWC_E_ALREADY_INITIALIZED        0x06U
#define SWC_E_EVENT_QUEUE_FULL           0x07U

/*==================================================================================================
*                                    TYPE DEFINITIONS
==================================================================================================*/

/* Component state enumeration */
typedef enum
{
    SWC_STATE_UNINIT = 0U,
    SWC_STATE_INIT,
    SWC_STATE_ACTIVE,
    SWC_STATE_SUSPENDED,
    SWC_STATE_ERROR
} Swc_StateType;

/* Runnable execution state */
typedef enum
{
    SWC_RUNNABLE_IDLE = 0U,
    SWC_RUNNABLE_READY,
    SWC_RUNNABLE_RUNNING,
    SWC_RUNNABLE_BLOCKED,
    SWC_RUNNABLE_COMPLETED
} Swc_RunnableStateType;

/* RTE event types */
typedef enum
{
    SWC_EVENT_INIT = 0U,
    SWC_EVENT_CYCLIC,
    SWC_EVENT_DATARECEIVED,
    SWC_EVENT_OPERATIONINVOKED,
    SWC_EVENT_MODECHANGED,
    SWC_EVENT_TIMING,
    SWC_EVENT_SWCNOTIFICATION
} Swc_EventType;

/* Port direction types */
typedef enum
{
    SWC_PORT_PROVIDED = 0U,
    SWC_PORT_REQUIRED,
    SWC_PORT_PROVIDED_REQUIRED
} Swc_PortDirectionType;

/* Component instance handle */
typedef uint16 Swc_ComponentHandleType;

/* Runnable handle */
typedef uint16 Swc_RunnableHandleType;

/* Port handle */
typedef uint16 Swc_PortHandleType;

/* Event handle */
typedef uint16 Swc_EventHandleType;

/* Forward declaration of configuration types */
struct Swc_PortConfigType;
struct Swc_RunnableConfigType;
struct Swc_ComponentConfigType;

/* Runnable function pointer type */
typedef void (*Swc_RunnableFuncType)(void);

/* RTE callback function pointer type */
typedef Std_ReturnType (*Swc_RteCallbackType)(Swc_ComponentHandleType compHandle, 
                                               Swc_EventHandleType eventHandle);

/* Port interface configuration */
typedef struct
{
    uint16 portId;
    uint16 interfaceId;
    Swc_PortDirectionType direction;
    const void* dataElement;
    uint16 dataLength;
} Swc_PortInterfaceType;

/* RTE event configuration */
typedef struct
{
    Swc_EventHandleType eventHandle;
    Swc_EventType eventType;
    uint16 eventMask;
    uint16 periodMs;
    Swc_RunnableHandleType targetRunnable;
    boolean isEnabled;
} Swc_RteEventType;

/* Component instance type */
typedef struct
{
    Swc_ComponentHandleType handle;
    Swc_StateType state;
    const struct Swc_ComponentConfigType* config;
    void* instanceData;
    uint16 portCount;
    uint16 runnableCount;
} Swc_ComponentInstanceType;

/* Runnable entity type */
typedef struct
{
    Swc_RunnableHandleType handle;
    Swc_RunnableStateType state;
    const struct Swc_RunnableConfigType* config;
    Swc_ComponentHandleType ownerComponent;
    uint32 executionCounter;
    uint32 lastExecutionTime;
} Swc_RunnableEntityType;

/*==================================================================================================
*                                     FUNCTION PROTOTYPES
==================================================================================================*/

/* Initialization and Lifecycle */
extern void Swc_Init(const Swc_ConfigType* ConfigPtr);
extern void Swc_DeInit(void);
extern Std_ReturnType Swc_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* Component Lifecycle Management */
extern Std_ReturnType Swc_CreateInstance(Swc_ComponentHandleType componentId, 
                                          void* instanceData,
                                          Swc_ComponentHandleType* outHandle);
extern Std_ReturnType Swc_DestroyInstance(Swc_ComponentHandleType handle);
extern Std_ReturnType Swc_SetComponentState(Swc_ComponentHandleType handle, 
                                            Swc_StateType newState);
extern Swc_StateType Swc_GetComponentState(Swc_ComponentHandleType handle);

/* Runnable Entity Scheduling */
extern Std_ReturnType Swc_ActivateRunnable(Swc_RunnableHandleType runnableHandle);
extern Std_ReturnType Swc_TerminateRunnable(Swc_RunnableHandleType runnableHandle);
extern void Swc_ScheduleRunnables(void);
extern Std_ReturnType Swc_IsRunnableReady(Swc_RunnableHandleType runnableHandle, 
                                          boolean* isReady);

/* Port Interface Management */
extern Std_ReturnType Swc_ConnectPort(Swc_PortHandleType portHandle, 
                                      const void* connectionData);
extern Std_ReturnType Swc_DisconnectPort(Swc_PortHandleType portHandle);
extern Std_ReturnType Swc_WritePortData(Swc_PortHandleType portHandle, 
                                        const void* data, 
                                        uint16 length);
extern Std_ReturnType Swc_ReadPortData(Swc_PortHandleType portHandle, 
                                       void* data, 
                                       uint16* length);

/* RTE Event Handling */
extern Std_ReturnType Swc_RegisterEvent(Swc_ComponentHandleType compHandle,
                                        const Swc_RteEventType* eventConfig);
extern Std_ReturnType Swc_TriggerEvent(Swc_EventHandleType eventHandle);
extern Std_ReturnType Swc_EnableEvent(Swc_EventHandleType eventHandle);
extern Std_ReturnType Swc_DisableEvent(Swc_EventHandleType eventHandle);
extern void Swc_ProcessEvents(void);

/* Main Functions */
extern void Swc_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* SWC_H */
