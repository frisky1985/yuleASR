/*==================================================================================================
* Project              : YuleASR - AUTOSAR Services Layer
* Platform             : ARM Cortex-M / x86 Simulation
* Peripheral           : N/A
* Dependencies         : Swc_Cfg
*
* SW Version           : 1.0.0
* Created              : JAN-2025
*==================================================================================================*/
#include "Swc_Cfg.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static void AppSensor_Init(void);
static void AppSensor_Shutdown(void);
static void AppSensor_ReadData(void);
static void AppSensor_ProcessData(void);
static void AppSensor_SendData(void);

static void AppActuator_Init(void);
static void AppActuator_Shutdown(void);
static void AppActuator_ReceiveCommand(void);
static void AppActuator_ControlOutput(void);

static void ServiceDiag_Init(void);
static void ServiceDiag_RunDiagnostic(void);
static void ServiceDiag_ReportStatus(void);

/*==================================================================================================
*                                    PORT CONFIGURATIONS
==================================================================================================*/

/* Application Sensor Component Ports */
static const Swc_PortConfigType Swc_Sensor_Ports[] =
{
    {
        /* Sensor Data Output Port */
        .portId = 0x1001U,
        .interfaceId = 0x2001U,
        .direction = 0U,           /* Provided */
        .dataElementSize = 4U,     /* 4 bytes (float/sint32) */
        .portType = 0U,            /* SenderReceiver */
        .isQueued = FALSE,
        .queueLength = 0U,
        .portName = "SensorDataOut"
    },
    {
        /* Sensor Configuration Input Port */
        .portId = 0x1002U,
        .interfaceId = 0x2002U,
        .direction = 1U,           /* Required */
        .dataElementSize = 8U,     /* 8 bytes configuration struct */
        .portType = 0U,            /* SenderReceiver */
        .isQueued = FALSE,
        .queueLength = 0U,
        .portName = "SensorConfigIn"
    },
    {
        /* Diagnostic Service Port */
        .portId = 0x1003U,
        .interfaceId = 0x2003U,
        .direction = 1U,           /* Required */
        .dataElementSize = 0U,     /* ClientServer - no direct data */
        .portType = 1U,            /* ClientServer */
        .isQueued = FALSE,
        .queueLength = 0U,
        .portName = "DiagService"
    }
};

/* Application Actuator Component Ports */
static const Swc_PortConfigType Swc_Actuator_Ports[] =
{
    {
        /* Actuator Command Input Port */
        .portId = 0x2001U,
        .interfaceId = 0x3001U,
        .direction = 1U,           /* Required */
        .dataElementSize = 4U,
        .portType = 0U,
        .isQueued = TRUE,
        .queueLength = 4U,
        .portName = "ActuatorCmdIn"
    },
    {
        /* Actuator Status Output Port */
        .portId = 0x2002U,
        .interfaceId = 0x3002U,
        .direction = 0U,           /* Provided */
        .dataElementSize = 2U,
        .portType = 0U,
        .isQueued = FALSE,
        .queueLength = 0U,
        .portName = "ActuatorStatusOut"
    },
    {
        /* Mode Switch Port */
        .portId = 0x2003U,
        .interfaceId = 0x3003U,
        .direction = 1U,           /* Required */
        .dataElementSize = 1U,
        .portType = 2U,            /* ModeSwitch */
        .isQueued = FALSE,
        .queueLength = 0U,
        .portName = "ModeSwitchIn"
    }
};

/* Diagnostic Service Component Ports */
static const Swc_PortConfigType Swc_Diag_Ports[] =
{
    {
        /* Diagnostic Service Provider Port */
        .portId = 0x3001U,
        .interfaceId = 0x4001U,
        .direction = 0U,           /* Provided */
        .dataElementSize = 0U,
        .portType = 1U,            /* ClientServer */
        .isQueued = FALSE,
        .queueLength = 0U,
        .portName = "DiagServiceProvider"
    },
    {
        /* Event Service Port */
        .portId = 0x3002U,
        .interfaceId = 0x4002U,
        .direction = 0U,           /* Provided */
        .dataElementSize = 8U,
        .portType = 0U,
        .isQueued = TRUE,
        .queueLength = 8U,
        .portName = "EventServiceOut"
    }
};

/*==================================================================================================
*                                  RUNNABLE CONFIGURATIONS
==================================================================================================*/

/* Application Sensor Component Runnables */
static const Swc_RunnableConfigType Swc_Sensor_Runnables[] =
{
    {
        .runnableId = 0x01U,
        .runnableFunc = AppSensor_ReadData,
        .canBeInvokedConcurrently = FALSE,
        .minStartIntervalMs = 10U,
        .priority = 3U,
        .runnableName = "ReadSensorData"
    },
    {
        .runnableId = 0x02U,
        .runnableFunc = AppSensor_ProcessData,
        .canBeInvokedConcurrently = FALSE,
        .minStartIntervalMs = 20U,
        .priority = 2U,
        .runnableName = "ProcessSensorData"
    },
    {
        .runnableId = 0x03U,
        .runnableFunc = AppSensor_SendData,
        .canBeInvokedConcurrently = FALSE,
        .minStartIntervalMs = 10U,
        .priority = 1U,
        .runnableName = "SendSensorData"
    }
};

/* Application Actuator Component Runnables */
static const Swc_RunnableConfigType Swc_Actuator_Runnables[] =
{
    {
        .runnableId = 0x11U,
        .runnableFunc = AppActuator_ReceiveCommand,
        .canBeInvokedConcurrently = FALSE,
        .minStartIntervalMs = 5U,
        .priority = 2U,
        .runnableName = "ReceiveActuatorCmd"
    },
    {
        .runnableId = 0x12U,
        .runnableFunc = AppActuator_ControlOutput,
        .canBeInvokedConcurrently = FALSE,
        .minStartIntervalMs = 5U,
        .priority = 1U,
        .runnableName = "ControlActuatorOutput"
    }
};

/* Diagnostic Service Component Runnables */
static const Swc_RunnableConfigType Swc_Diag_Runnables[] =
{
    {
        .runnableId = 0x21U,
        .runnableFunc = ServiceDiag_RunDiagnostic,
        .canBeInvokedConcurrently = FALSE,
        .minStartIntervalMs = 100U,
        .priority = 5U,
        .runnableName = "RunDiagnostic"
    },
    {
        .runnableId = 0x22U,
        .runnableFunc = ServiceDiag_ReportStatus,
        .canBeInvokedConcurrently = TRUE,
        .minStartIntervalMs = 1000U,
        .priority = 10U,
        .runnableName = "ReportDiagStatus"
    }
};

/*==================================================================================================
*                                   EVENT CONFIGURATIONS
==================================================================================================*/

/* Application Sensor Events */
static const Swc_EventConfigType Swc_Sensor_Events[] =
{
    {
        .eventId = 0x01U,
        .eventType = 0U,           /* INIT */
        .periodMs = 0U,
        .targetRunnableId = 0x01U,
        .eventMask = 0x01U,
        .autoEnable = TRUE
    },
    {
        .eventId = 0x02U,
        .eventType = 1U,           /* CYCLIC */
        .periodMs = 10U,
        .targetRunnableId = 0x01U,
        .eventMask = 0x02U,
        .autoEnable = TRUE
    },
    {
        .eventId = 0x03U,
        .eventType = 1U,           /* CYCLIC */
        .periodMs = 20U,
        .targetRunnableId = 0x02U,
        .eventMask = 0x04U,
        .autoEnable = TRUE
    },
    {
        .eventId = 0x04U,
        .eventType = 1U,           /* CYCLIC */
        .periodMs = 10U,
        .targetRunnableId = 0x03U,
        .eventMask = 0x08U,
        .autoEnable = TRUE
    }
};

/* Application Actuator Events */
static const Swc_EventConfigType Swc_Actuator_Events[] =
{
    {
        .eventId = 0x11U,
        .eventType = 0U,           /* INIT */
        .periodMs = 0U,
        .targetRunnableId = 0x11U,
        .eventMask = 0x01U,
        .autoEnable = TRUE
    },
    {
        .eventId = 0x12U,
        .eventType = 1U,           /* CYCLIC */
        .periodMs = 5U,
        .targetRunnableId = 0x11U,
        .eventMask = 0x02U,
        .autoEnable = TRUE
    },
    {
        .eventId = 0x13U,
        .eventType = 4U,           /* MODE_CHANGED */
        .periodMs = 0U,
        .targetRunnableId = 0x12U,
        .eventMask = 0x04U,
        .autoEnable = TRUE
    }
};

/* Diagnostic Service Events */
static const Swc_EventConfigType Swc_Diag_Events[] =
{
    {
        .eventId = 0x21U,
        .eventType = 0U,           /* INIT */
        .periodMs = 0U,
        .targetRunnableId = 0x21U,
        .eventMask = 0x01U,
        .autoEnable = TRUE
    },
    {
        .eventId = 0x22U,
        .eventType = 1U,           /* CYCLIC */
        .periodMs = 100U,
        .targetRunnableId = 0x21U,
        .eventMask = 0x02U,
        .autoEnable = TRUE
    },
    {
        .eventId = 0x23U,
        .eventType = 1U,           /* CYCLIC */
        .periodMs = 1000U,
        .targetRunnableId = 0x22U,
        .eventMask = 0x04U,
        .autoEnable = TRUE
    }
};

/*==================================================================================================
*                                COMPONENT CONFIGURATIONS
==================================================================================================*/

static const Swc_ComponentConfigType Swc_ComponentConfigs[] =
{
    {
        /* Application Sensor Component */
        .componentId = 0x01U,
        .componentName = "AppSensor",
        .componentType = 1U,       /* SensorActuator */
        .numPorts = 3U,
        .portConfigs = Swc_Sensor_Ports,
        .numRunnables = 3U,
        .runnableConfigs = Swc_Sensor_Runnables,
        .numEvents = 4U,
        .eventConfigs = Swc_Sensor_Events,
        .instanceDataSize = 64U,
        .initFunc = AppSensor_Init,
        .shutdownFunc = AppSensor_Shutdown
    },
    {
        /* Application Actuator Component */
        .componentId = 0x02U,
        .componentName = "AppActuator",
        .componentType = 1U,       /* SensorActuator */
        .numPorts = 3U,
        .portConfigs = Swc_Actuator_Ports,
        .numRunnables = 2U,
        .runnableConfigs = Swc_Actuator_Runnables,
        .numEvents = 3U,
        .eventConfigs = Swc_Actuator_Events,
        .instanceDataSize = 32U,
        .initFunc = AppActuator_Init,
        .shutdownFunc = AppActuator_Shutdown
    },
    {
        /* Diagnostic Service Component */
        .componentId = 0x03U,
        .componentName = "ServiceDiag",
        .componentType = 3U,       /* Service */
        .numPorts = 2U,
        .portConfigs = Swc_Diag_Ports,
        .numRunnables = 2U,
        .runnableConfigs = Swc_Diag_Runnables,
        .numEvents = 3U,
        .eventConfigs = Swc_Diag_Events,
        .instanceDataSize = 128U,
        .initFunc = ServiceDiag_Init,
        .shutdownFunc = NULL_PTR
    }
};

/*==================================================================================================
*                                GLOBAL CONFIGURATION
==================================================================================================*/

const Swc_ConfigType Swc_Config =
{
    .numComponents = 3U,
    .componentConfigs = Swc_ComponentConfigs,
    .schedulingPeriodMs = 10U,
    .enableTracing = FALSE,
    .traceCallback = NULL_PTR
};

/* Post-build configuration pointer (if needed) */
const Swc_ConfigType* Swc_ConfigPtr = &Swc_Config;

/*==================================================================================================
*                               RUNNABLE IMPLEMENTATIONS
==================================================================================================*/

/* Application Sensor Component Runnables */
static void AppSensor_Init(void)
{
    /* Initialize sensor hardware and internal state */
    /* Placeholder for actual initialization code */
}

static void AppSensor_Shutdown(void)
{
    /* Shutdown sensor hardware and cleanup */
    /* Placeholder for actual shutdown code */
}

static void AppSensor_ReadData(void)
{
    /* Read sensor data from hardware */
    /* Placeholder for actual read operation */
}

static void AppSensor_ProcessData(void)
{
    /* Process raw sensor data */
    /* Placeholder for actual data processing */
}

static void AppSensor_SendData(void)
{
    /* Send processed data via RTE */
    /* Placeholder for actual data transmission */
}

/* Application Actuator Component Runnables */
static void AppActuator_Init(void)
{
    /* Initialize actuator hardware */
    /* Placeholder for actual initialization code */
}

static void AppActuator_Shutdown(void)
{
    /* Shutdown actuator hardware */
    /* Placeholder for actual shutdown code */
}

static void AppActuator_ReceiveCommand(void)
{
    /* Receive control command from RTE */
    /* Placeholder for actual command reception */
}

static void AppActuator_ControlOutput(void)
{
    /* Control actuator output based on command */
    /* Placeholder for actual output control */
}

/* Diagnostic Service Component Runnables */
static void ServiceDiag_Init(void)
{
    /* Initialize diagnostic service */
    /* Placeholder for actual initialization code */
}

static void ServiceDiag_RunDiagnostic(void)
{
    /* Run diagnostic checks */
    /* Placeholder for actual diagnostic routine */
}

static void ServiceDiag_ReportStatus(void)
{
    /* Report diagnostic status */
    /* Placeholder for actual status reporting */
}
