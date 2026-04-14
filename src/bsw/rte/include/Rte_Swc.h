/**
 * @file Rte_Swc.h
 * @brief RTE Software Component Template following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Runtime Environment (RTE)
 * Layer: RTE (Runtime Environment)
 * Purpose: Software Component template and interface definitions
 */

#ifndef RTE_SWC_H
#define RTE_SWC_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte.h"

/*==================================================================================================
*                                    SWC COMPONENT TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief SWC Component Type ID
 */
typedef uint8 Swc_ComponentTypeIdType;

/**
 * @brief SWC Component Instance ID
 */
typedef uint8 Swc_InstanceIdType;

/**
 * @brief SWC Runnable ID
 */
typedef uint8 Swc_RunnableIdType;

/**
 * @brief SWC Port ID
 */
typedef uint16 Swc_PortIdType;

/**
 * @brief SWC Interface ID
 */
typedef uint16 Swc_InterfaceIdType;

/**
 * @brief SWC Component Handle
 */
typedef struct {
    Swc_ComponentTypeIdType componentTypeId;
    Swc_InstanceIdType instanceId;
    Rte_InstanceHandleType rteHandle;
} Swc_ComponentHandleType;

/*==================================================================================================
*                                    SWC RUNNABLE TYPES
==================================================================================================*/

/**
 * @brief Runnable entity function type
 */
typedef void (*Swc_RunnableFuncType)(void);

/**
 * @brief Runnable entity configuration
 */
typedef struct {
    Swc_RunnableIdType runnableId;
    Swc_RunnableFuncType runnableFunc;
    uint8 eventType;
    uint32 periodMs;
    boolean canBeInvokedConcurrently;
} Swc_RunnableConfigType;

/**
 * @brief Runnable entity information
 */
typedef struct {
    Swc_RunnableIdType runnableId;
    uint8 state;
    uint32 lastExecutionTime;
    uint32 executionCount;
    uint32 maxExecutionTime;
    uint32 minExecutionTime;
} Swc_RunnableInfoType;

/*==================================================================================================
*                                    SWC PORT TYPES
==================================================================================================*/

/**
 * @brief Port direction type
 */
typedef enum {
    SWC_PORT_DIRECTION_PROVIDED = 0,
    SWC_PORT_DIRECTION_REQUIRED
} Swc_PortDirectionType;

/**
 * @brief Port interface type
 */
typedef enum {
    SWC_PORT_INTERFACE_SENDER_RECEIVER = 0,
    SWC_PORT_INTERFACE_CLIENT_SERVER,
    SWC_PORT_INTERFACE_MODE_SWITCH,
    SWC_PORT_INTERFACE_TRIGGER,
    SWC_PORT_INTERFACE_PARAMETER,
    SWC_PORT_INTERFACE_NON_VOLATILE_DATA
} Swc_PortInterfaceType;

/**
 * @brief Port configuration
 */
typedef struct {
    Swc_PortIdType portId;
    Swc_PortDirectionType direction;
    Swc_PortInterfaceType interfaceType;
    Swc_InterfaceIdType interfaceId;
    boolean isQueued;
    uint32 queueLength;
} Swc_PortConfigType;

/**
 * @brief Port connection information
 */
typedef struct {
    Swc_PortIdType sourcePortId;
    Swc_ComponentHandleType sourceComponent;
    Swc_PortIdType targetPortId;
    Swc_ComponentHandleType targetComponent;
    boolean isConnected;
} Swc_PortConnectionType;

/*==================================================================================================
*                                    SWC COMPONENT CONFIGURATION
==================================================================================================*/

typedef struct {
    Swc_ComponentTypeIdType componentTypeId;
    const char* componentName;
    uint8 numRunnables;
    uint8 numPorts;
    const Swc_RunnableConfigType* runnableConfigs;
    const Swc_PortConfigType* portConfigs;
    uint32 pimSize;
    uint32 numIRVs;
} Swc_ComponentTypeConfigType;

typedef struct {
    Swc_ComponentHandleType handle;
    const Swc_ComponentTypeConfigType* componentTypeConfig;
    void* pimData;
    uint8 state;
} Swc_ComponentInstanceType;

/*==================================================================================================
*                                    SWC COMPONENT STATE
==================================================================================================*/
typedef enum {
    SWC_STATE_UNINITIALIZED = 0,
    SWC_STATE_INITIALIZING,
    SWC_STATE_INITIALIZED,
    SWC_STATE_RUNNING,
    SWC_STATE_STOPPING,
    SWC_STATE_STOPPED,
    SWC_STATE_ERROR
} Swc_ComponentStateType;

/*==================================================================================================
*                                    SWC API FUNCTION PROTOTYPES
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes a software component
 * @param componentHandle Component handle
 * @return RTE status
 */
extern Rte_StatusType Swc_Init(Swc_ComponentHandleType componentHandle);

/**
 * @brief Deinitializes a software component
 * @param componentHandle Component handle
 * @return RTE status
 */
extern Rte_StatusType Swc_Deinit(Swc_ComponentHandleType componentHandle);

/**
 * @brief Starts a software component
 * @param componentHandle Component handle
 * @return RTE status
 */
extern Rte_StatusType Swc_Start(Swc_ComponentHandleType componentHandle);

/**
 * @brief Stops a software component
 * @param componentHandle Component handle
 * @return RTE status
 */
extern Rte_StatusType Swc_Stop(Swc_ComponentHandleType componentHandle);

/**
 * @brief Gets component state
 * @param componentHandle Component handle
 * @param state Pointer to store state
 * @return RTE status
 */
extern Rte_StatusType Swc_GetState(Swc_ComponentHandleType componentHandle, 
                                    Swc_ComponentStateType* state);

/**
 * @brief Executes a runnable
 * @param componentHandle Component handle
 * @param runnableId Runnable ID
 * @return RTE status
 */
extern Rte_StatusType Swc_ExecuteRunnable(Swc_ComponentHandleType componentHandle, 
                                           Swc_RunnableIdType runnableId);

/**
 * @brief Gets runnable information
 * @param componentHandle Component handle
 * @param runnableId Runnable ID
 * @param info Pointer to store info
 * @return RTE status
 */
extern Rte_StatusType Swc_GetRunnableInfo(Swc_ComponentHandleType componentHandle,
                                           Swc_RunnableIdType runnableId,
                                           Swc_RunnableInfoType* info);

/**
 * @brief Connects two ports
 * @param sourceComponent Source component
 * @param sourcePortId Source port ID
 * @param targetComponent Target component
 * @param targetPortId Target port ID
 * @return RTE status
 */
extern Rte_StatusType Swc_ConnectPorts(Swc_ComponentHandleType sourceComponent,
                                        Swc_PortIdType sourcePortId,
                                        Swc_ComponentHandleType targetComponent,
                                        Swc_PortIdType targetPortId);

/**
 * @brief Disconnects two ports
 * @param sourceComponent Source component
 * @param sourcePortId Source port ID
 * @param targetComponent Target component
 * @param targetPortId Target port ID
 * @return RTE status
 */
extern Rte_StatusType Swc_DisconnectPorts(Swc_ComponentHandleType sourceComponent,
                                           Swc_PortIdType sourcePortId,
                                           Swc_ComponentHandleType targetComponent,
                                           Swc_PortIdType targetPortId);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    SWC TEMPLATE MACROS
==================================================================================================*/

/**
 * @brief Macro to declare a software component
 */
#define SWC_DECLARE_COMPONENT(componentName, numRunnables, numPorts) \
    typedef struct { \
        Swc_ComponentHandleType handle; \
        Swc_RunnableConfigType runnables[numRunnables]; \
        Swc_PortConfigType ports[numPorts]; \
    } Swc_##componentName##Type

/**
 * @brief Macro to define a runnable entity
 */
#define SWC_DEFINE_RUNNABLE(componentName, runnableName, eventType, periodMs) \
    static void Swc_##componentName##_Runnable_##runnableName(void); \
    static const Swc_RunnableConfigType Swc_##componentName##_RunnableConfig_##runnableName = { \
        .runnableId = RTE_RUNNABLE_##runnableName, \
        .runnableFunc = Swc_##componentName##_Runnable_##runnableName, \
        .eventType = eventType, \
        .periodMs = periodMs, \
        .canBeInvokedConcurrently = FALSE \
    }; \
    static void Swc_##componentName##_Runnable_##runnableName(void)

/**
 * @brief Macro to declare a sender-receiver port
 */
#define SWC_DECLARE_SR_PORT(componentName, portName, dataType, direction) \
    typedef struct { \
        dataType data; \
        boolean isUpdated; \
        uint32 age; \
    } Swc_##componentName##_Port_##portName##Type

/**
 * @brief Macro to declare a client-server port
 */
#define SWC_DECLARE_CS_PORT(componentName, portName, direction) \
    typedef struct { \
        uint8 operationId; \
        void* requestData; \
        void* responseData; \
        boolean isPending; \
    } Swc_##componentName##_Port_##portName##Type

/**
 * @brief Macro to declare a mode switch port
 */
#define SWC_DECLARE_MODE_PORT(componentName, portName, modeGroupType, direction) \
    typedef struct { \
        modeGroupType currentMode; \
        modeGroupType nextMode; \
        boolean transitionPending; \
    } Swc_##componentName##_Port_##portName##Type

/**
 * @brief Macro to declare a trigger port
 */
#define SWC_DECLARE_TRIGGER_PORT(componentName, portName) \
    typedef struct { \
        boolean triggered; \
        uint32 triggerCount; \
    } Swc_##componentName##_Port_##portName##Type

/**
 * @brief Macro to declare per-instance memory
 */
#define SWC_DECLARE_PIM(componentName, pimName, dataType) \
    typedef dataType Swc_##componentName##_Pim_##pimName##Type

/**
 * @brief Macro to declare inter-runnable variable
 */
#define SWC_DECLARE_IRV(componentName, irvName, dataType) \
    typedef struct { \
        dataType data; \
        boolean isWritten; \
    } Swc_##componentName##_Irv_##irvName##Type

/**
 * @brief Macro to access PIM
 */
#define SWC_PIM(componentName, pimName) \
    (&((Swc_##componentName##_Pim_##pimName##Type*)Rte_PimAddr_##componentName##_##pimName()))

/**
 * @brief Macro to access IRV
 */
#define SWC_IRV(componentName, irvName) \
    ((Swc_##componentName##_Irv_##irvName##Type*)Rte_IrvRead_##componentName##_##irvName)

/*==================================================================================================
*                                    EXAMPLE SWC DEFINITIONS
==================================================================================================*/

/**
 * @brief Example: Engine Control SWC
 */
SWC_DECLARE_COMPONENT(EngineControl, 4, 6);

/**
 * @brief Example: Vehicle Dynamics SWC
 */
SWC_DECLARE_COMPONENT(VehicleDynamics, 3, 4);

/**
 * @brief Example: Diagnostic Manager SWC
 */
SWC_DECLARE_COMPONENT(DiagnosticManager, 5, 8);

/**
 * @brief Example: Communication Manager SWC
 */
SWC_DECLARE_COMPONENT(CommunicationManager, 2, 6);

/**
 * @brief Example: Storage Manager SWC
 */
SWC_DECLARE_COMPONENT(StorageManager, 3, 4);

/**
 * @brief Example: IO Control SWC
 */
SWC_DECLARE_COMPONENT(IOControl, 4, 8);

/**
 * @brief Example: Mode Manager SWC
 */
SWC_DECLARE_COMPONENT(ModeManager, 2, 6);

/**
 * @brief Example: Watchdog Manager SWC
 */
SWC_DECLARE_COMPONENT(WatchdogManager, 2, 3);

#endif /* RTE_SWC_H */
