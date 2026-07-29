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
* Dependencies         : Std_Types
*
* SW Version           : 1.0.0
* Created              : JAN-2025
*==================================================================================================*/
#ifndef SWC_CFG_H
#define SWC_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION DEFINITIONS
==================================================================================================*/
#define SWC_CFG_VENDOR_ID                0x01U
#define SWC_CFG_MODULE_ID                0x50U
#define SWC_CFG_AR_MAJOR_VERSION         4U
#define SWC_CFG_AR_MINOR_VERSION         4U
#define SWC_CFG_AR_PATCH_VERSION         0U
#define SWC_CFG_SW_MAJOR_VERSION         1U
#define SWC_CFG_SW_MINOR_VERSION         0U
#define SWC_CFG_SW_PATCH_VERSION         0U

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/

/* Maximum number of component instances */
#define SWC_MAX_COMPONENT_INSTANCES      16U

/* Maximum number of runnable entities per component */
#define SWC_MAX_RUNNABLES_PER_COMPONENT  8U

/* Maximum number of ports per component */
#define SWC_MAX_PORTS_PER_COMPONENT      16U

/* Maximum number of RTE events */
#define SWC_MAX_RTE_EVENTS               32U

/* Maximum number of pending events in queue */
#define SWC_EVENT_QUEUE_SIZE             64U

/* Enable/Disable Development Error Detection */
#define SWC_DEV_ERROR_DETECT             STD_ON

/* Enable/Disable Version Info API */
#define SWC_VERSION_INFO_API             STD_ON

/* Enable/Disable Component Instance Counting */
#define SWC_INSTANCE_COUNT_ENABLED       STD_ON

/* Maximum component name length */
#define SWC_MAX_NAME_LENGTH              32U

/* Enable cyclic scheduling */
#define SWC_CYCLIC_SCHEDULING_ENABLED    STD_ON

/* Default scheduling period in milliseconds */
#define SWC_DEFAULT_SCHEDULING_PERIOD_MS 10U

/* Enable RTE callback support */
#define SWC_RTE_CALLBACK_ENABLED         STD_ON

/* Enable port data buffering */
#define SWC_PORT_BUFFERING_ENABLED       STD_ON

/* Port buffer size in bytes */
#define SWC_PORT_BUFFER_SIZE             256U

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/

/* Pre-compile configuration for RTE notification callback */
#ifndef SWC_RTE_NOTIFICATION_CALLBACK
    #define SWC_RTE_NOTIFICATION_CALLBACK NULL_PTR
#endif

/* Pre-compile configuration for error hook */
#ifndef SWC_ERROR_HOOK
    #define SWC_ERROR_HOOK NULL_PTR
#endif

/*==================================================================================================
*                                    TYPE DEFINITIONS
==================================================================================================*/

/* Component configuration forward declaration */
struct Swc_ComponentConfigType;
struct Swc_RunnableConfigType;
struct Swc_PortConfigType;

/* Port configuration structure */
typedef struct Swc_PortConfigType
{
    uint16 portId;
    uint16 interfaceId;
    uint8 direction;              /* 0=Provided, 1=Required, 2=PR */
    uint16 dataElementSize;
    uint8 portType;               /* SenderReceiver, ClientServer, etc. */
    boolean isQueued;
    uint16 queueLength;
    const char portName[SWC_MAX_NAME_LENGTH];
} Swc_PortConfigType;

/* Runnable configuration structure */
typedef struct Swc_RunnableConfigType
{
    uint16 runnableId;
    void (*runnableFunc)(void);
    boolean canBeInvokedConcurrently;
    uint16 minStartIntervalMs;
    uint16 priority;
    const char runnableName[SWC_MAX_NAME_LENGTH];
} Swc_RunnableConfigType;

/* RTE event configuration structure */
typedef struct Swc_EventConfigType
{
    uint16 eventId;
    uint8 eventType;              /* INIT, CYCLIC, DATA_RECEIVED, etc. */
    uint16 periodMs;              /* For cyclic events */
    uint16 targetRunnableId;
    uint8 eventMask;
    boolean autoEnable;
} Swc_EventConfigType;

/* Component configuration structure */
typedef struct Swc_ComponentConfigType
{
    uint16 componentId;
    const char componentName[SWC_MAX_NAME_LENGTH];
    uint8 componentType;          /* Application, SensorActuator, ComplexDevice, Service */
    uint8 numPorts;
    const Swc_PortConfigType* portConfigs;
    uint8 numRunnables;
    const Swc_RunnableConfigType* runnableConfigs;
    uint8 numEvents;
    const Swc_EventConfigType* eventConfigs;
    uint16 instanceDataSize;
    void (*initFunc)(void);
    void (*shutdownFunc)(void);
} Swc_ComponentConfigType;

/* Global configuration structure */
typedef struct
{
    uint8 numComponents;
    const Swc_ComponentConfigType* componentConfigs;
    uint16 schedulingPeriodMs;
    boolean enableTracing;
    void (*traceCallback)(uint8 level, const char* msg);
} Swc_ConfigType;

/*==================================================================================================
*                                    CONFIGURATION VARIANTS
==================================================================================================*/

/* Pre-compile configuration pointer (defined in Swc_Lcfg.c) */
extern const Swc_ConfigType Swc_Config;

/* Post-build configuration variant (optional) */
#if (SWC_PB_CONFIG == STD_ON)
extern const Swc_ConfigType* Swc_ConfigPtr;
#endif

#ifdef __cplusplus
}
#endif

#endif /* SWC_CFG_H */
