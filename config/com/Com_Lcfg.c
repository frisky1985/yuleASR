/*
 * Com_Lcfg.c
 * COM Module Link-Time Configuration
 * Example configuration for Engine Data Application
 * 
 * Updated for T014: Configuration Tools and Engine Examples
 * 
 * Features:
 * - 19 engine and vehicle signals
 * - 4 IPDUs with various transmission modes
 * - 3 IPDU Groups (Engine, Chassis, Body)
 * - 3 Signal Groups
 * - TMC (Transmission Mode Conditions) support
 * - Transmission confirmation support
 */

#include "Com.h"

/*==================[Buffer Declarations]===================================*/

/* IPDU Buffers */
static uint8 ComIPdu_EngineData_Buffer[8];
static uint8 ComIPdu_EngineStatus_Buffer[4];
static uint8 ComIPdu_VehicleSpeed_Buffer[6];
static uint8 ComIPdu_BodyControl_Buffer[2];

/* Shadow Buffers for Signal Groups */
static uint8 ComShadowBuffer_EngineCoreInfo[5];
static uint8 ComShadowBuffer_EngineDiagnostics[4];
static uint8 ComShadowBuffer_VehicleDynamics[6];

/*==================[IPDU Group Configuration]================================*/

static const Com_IPduIdType ComIPduGroup_EngineGroup_Refs[] = {
    ComConf_ComIPdu_EngineData,
    ComConf_ComIPdu_EngineStatus
};

static const Com_IPduIdType ComIPduGroup_ChassisGroup_Refs[] = {
    ComConf_ComIPdu_VehicleSpeed
};

static const Com_IPduIdType ComIPduGroup_BodyGroup_Refs[] = {
    ComConf_ComIPdu_BodyControl
};

static const Com_IPduGroupConfigType ComIPduGroups[] = {
    {
        .IpduGroupId = ComConf_ComIPduGroup_EngineGroup,
        .IPduRefs = ComIPduGroup_EngineGroup_Refs,
        .NumIPdus = 2
    },
    {
        .IpduGroupId = ComConf_ComIPduGroup_ChassisGroup,
        .IPduRefs = ComIPduGroup_ChassisGroup_Refs,
        .NumIPdus = 1
    },
    {
        .IpduGroupId = ComConf_ComIPduGroup_BodyGroup,
        .IPduRefs = ComIPduGroup_BodyGroup_Refs,
        .NumIPdus = 1
    }
};

/*==================[Signal References]=====================================*/

/* Engine Data IPDU Signals */
static const Com_SignalIdType ComEngineData_Signals[] = {
    ComConf_ComSignal_EngineSpeed,
    ComConf_ComSignal_CoolantTemp,
    ComConf_ComSignal_ThrottlePosition,
    ComConf_ComSignal_EngineTorque,
    ComConf_ComSignal_EngineState,
    ComConf_ComSignal_BatteryVoltage
};

/* Engine Status IPDU Signals */
static const Com_SignalIdType ComEngineStatus_Signals[] = {
    ComConf_ComSignal_OilPressure,
    ComConf_ComSignal_OilTemp,
    ComConf_ComSignal_FuelLevel,
    ComConf_ComSignal_IntakeAirTemp
};

/* Vehicle Speed IPDU Signals */
static const Com_SignalIdType ComVehicleSpeed_Signals[] = {
    ComConf_ComSignal_VehicleSpeed,
    ComConf_ComSignal_WheelSpeed_FL,
    ComConf_ComSignal_WheelSpeed_FR
};

/* Body Control IPDU Signals */
static const Com_SignalIdType ComBodyControl_Signals[] = {
    ComConf_ComSignal_GearPosition,
    ComConf_ComSignal_TransmissionMode,
    ComConf_ComSignal_ParkingBrake,
    ComConf_ComSignal_TurnSignalLeft,
    ComConf_ComSignal_TurnSignalRight,
    ComConf_ComSignal_Headlights
};

/*==================[Signal Group References]===============================*/

/* Engine Core Info Signal Group */
static const Com_SignalIdType ComEngineCoreInfo_Signals[] = {
    ComConf_ComSignal_EngineSpeed,
    ComConf_ComSignal_CoolantTemp,
    ComConf_ComSignal_ThrottlePosition,
    ComConf_ComSignal_EngineState
};

/* Engine Diagnostics Signal Group */
static const Com_SignalIdType ComEngineDiagnostics_Signals[] = {
    ComConf_ComSignal_OilPressure,
    ComConf_ComSignal_OilTemp,
    ComConf_ComSignal_FuelLevel,
    ComConf_ComSignal_IntakeAirTemp
};

/* Vehicle Dynamics Signal Group */
static const Com_SignalIdType ComVehicleDynamics_Signals[] = {
    ComConf_ComSignal_VehicleSpeed,
    ComConf_ComSignal_WheelSpeed_FL,
    ComConf_ComSignal_WheelSpeed_FR
};

/*==================[IPDU Configuration]====================================*/

static const Com_IPduConfigType ComIPdus[] = {
    /* Engine Data IPDU - Periodic Transmission with TMC */
    {
        .IPduId = ComConf_ComIPdu_EngineData,
        .DataPtr = ComIPdu_EngineData_Buffer,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = ComEngineData_Signals,
        .NumSignals = 6,
        .SignalGroupRefs = (Com_SignalGroupIdType[]){ComConf_ComSignalGroup_EngineCoreInfo},
        .NumSignalGroups = 1,
        .TxMode = {
            /* TxModeFalse: Default periodic transmission (100ms) */
            .TxModeFalse = {
                .Mode = COM_MODE_PERIODIC,
                .CycleTime = 100,
                .RepetitionPeriod = 0,
                .NumRepetitions = 0,
                .TimeOffset = 0,
                .RepeatingEnabled = FALSE
            },
            /* TxModeTrue: Fast transmission when engine speed > 3000 RPM */
            .TxModeTrue = {
                .Mode = COM_MODE_MIXED,
                .CycleTime = 50,
                .RepetitionPeriod = 10,
                .NumRepetitions = 2,
                .TimeOffset = 0,
                .RepeatingEnabled = TRUE
            },
            /* TMC: Switch to fast mode when EngineSpeed > 3000 RPM */
            .TmcConfig = {
                .SignalId = ComConf_ComSignal_EngineSpeed,
                .ThresholdValue = 3000,
                .UseGreaterThan = TRUE,
                .IsConfigured = TRUE
            },
            .UseTmc = TRUE
        },
        .IpduGroupRefs = (Com_IpduGroupIdType[]){ComConf_ComIPduGroup_EngineGroup},
        .NumIpduGroups = 1,
        .Timeout = 0,
        .ComIPduCallout = NULL_PTR,
        .TxConfirmation = {
            .EnableConfirmation = TRUE,
            .TxTimeout = 100,
            .MaxRetries = 3,
            .ComTxConfirmation = NULL_PTR,
            .ComTxErrorNotification = NULL_PTR,
            .ComTxTimeoutNotification = NULL_PTR
        }
    },
    /* Engine Status IPDU - Slow Periodic (500ms) */
    {
        .IPduId = ComConf_ComIPdu_EngineStatus,
        .DataPtr = ComIPdu_EngineStatus_Buffer,
        .Length = 4,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = ComEngineStatus_Signals,
        .NumSignals = 4,
        .SignalGroupRefs = (Com_SignalGroupIdType[]){ComConf_ComSignalGroup_EngineDiagnostics},
        .NumSignalGroups = 1,
        .TxMode = {
            .TxModeFalse = {
                .Mode = COM_MODE_PERIODIC,
                .CycleTime = 500,
                .RepetitionPeriod = 0,
                .NumRepetitions = 0,
                .TimeOffset = 0,
                .RepeatingEnabled = FALSE
            },
            .TxModeTrue = {
                .Mode = COM_MODE_NONE,
                .CycleTime = 0,
                .RepetitionPeriod = 0,
                .NumRepetitions = 0,
                .TimeOffset = 0,
                .RepeatingEnabled = FALSE
            },
            .TmcConfig = {
                .SignalId = 0,
                .ThresholdValue = 0,
                .UseGreaterThan = TRUE,
                .IsConfigured = FALSE
            },
            .UseTmc = FALSE
        },
        .IpduGroupRefs = (Com_IpduGroupIdType[]){ComConf_ComIPduGroup_EngineGroup},
        .NumIpduGroups = 1,
        .Timeout = 0,
        .ComIPduCallout = NULL_PTR,
        .TxConfirmation = {
            .EnableConfirmation = FALSE,
            .TxTimeout = 0,
            .MaxRetries = 0,
            .ComTxConfirmation = NULL_PTR,
            .ComTxErrorNotification = NULL_PTR,
            .ComTxTimeoutNotification = NULL_PTR
        }
    },
    /* Vehicle Speed IPDU - Event Triggered */
    {
        .IPduId = ComConf_ComIPdu_VehicleSpeed,
        .DataPtr = ComIPdu_VehicleSpeed_Buffer,
        .Length = 6,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = ComVehicleSpeed_Signals,
        .NumSignals = 3,
        .SignalGroupRefs = (Com_SignalGroupIdType[]){ComConf_ComSignalGroup_VehicleDynamics},
        .NumSignalGroups = 1,
        .TxMode = {
            .TxModeFalse = {
                .Mode = COM_MODE_DIRECT,
                .CycleTime = 0,
                .RepetitionPeriod = 20,
                .NumRepetitions = 1,
                .TimeOffset = 0,
                .RepeatingEnabled = TRUE
            },
            .TxModeTrue = {
                .Mode = COM_MODE_NONE,
                .CycleTime = 0,
                .RepetitionPeriod = 0,
                .NumRepetitions = 0,
                .TimeOffset = 0,
                .RepeatingEnabled = FALSE
            },
            .TmcConfig = {
                .SignalId = 0,
                .ThresholdValue = 0,
                .UseGreaterThan = TRUE,
                .IsConfigured = FALSE
            },
            .UseTmc = FALSE
        },
        .IpduGroupRefs = (Com_IpduGroupIdType[]){ComConf_ComIPduGroup_ChassisGroup},
        .NumIpduGroups = 1,
        .Timeout = 0,
        .ComIPduCallout = NULL_PTR,
        .TxConfirmation = {
            .EnableConfirmation = TRUE,
            .TxTimeout = 50,
            .MaxRetries = 2,
            .ComTxConfirmation = NULL_PTR,
            .ComTxErrorNotification = NULL_PTR,
            .ComTxTimeoutNotification = NULL_PTR
        }
    },
    /* Body Control IPDU - Mixed Mode with TMC */
    {
        .IPduId = ComConf_ComIPdu_BodyControl,
        .DataPtr = ComIPdu_BodyControl_Buffer,
        .Length = 2,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = ComBodyControl_Signals,
        .NumSignals = 6,
        .SignalGroupRefs = NULL_PTR,
        .NumSignalGroups = 0,
        .TxMode = {
            .TxModeFalse = {
                .Mode = COM_MODE_MIXED,
                .CycleTime = 500,
                .RepetitionPeriod = 20,
                .NumRepetitions = 3,
                .TimeOffset = 100,
                .RepeatingEnabled = TRUE
            },
            .TxModeTrue = {
                .Mode = COM_MODE_DIRECT,
                .CycleTime = 0,
                .RepetitionPeriod = 10,
                .NumRepetitions = 5,
                .TimeOffset = 0,
                .RepeatingEnabled = TRUE
            },
            .TmcConfig = {
                .SignalId = ComConf_ComSignal_GearPosition,
                .ThresholdValue = 0,
                .UseGreaterThan = FALSE,
                .IsConfigured = TRUE
            },
            .UseTmc = TRUE
        },
        .IpduGroupRefs = (Com_IpduGroupIdType[]){ComConf_ComIPduGroup_BodyGroup},
        .NumIpduGroups = 1,
        .Timeout = 0,
        .ComIPduCallout = NULL_PTR,
        .TxConfirmation = {
            .EnableConfirmation = TRUE,
            .TxTimeout = 100,
            .MaxRetries = 3,
            .ComTxConfirmation = NULL_PTR,
            .ComTxErrorNotification = NULL_PTR,
            .ComTxTimeoutNotification = NULL_PTR
        }
    }
};

/*==================[Signal Configuration]=================================*/

/* Initial Values */
static const uint16 ComInitValue_EngineSpeed = 0;
static const sint8 ComInitValue_CoolantTemp = 20;
static const uint8 ComInitValue_ThrottlePosition = 0;
static const uint16 ComInitValue_EngineTorque = 0;
static const uint8 ComInitValue_EngineState = 0;
static const uint8 ComInitValue_BatteryVoltage = 120;
static const uint8 ComInitValue_OilPressure = 0;
static const sint8 ComInitValue_OilTemp = 20;
static const uint8 ComInitValue_FuelLevel = 0;
static const sint8 ComInitValue_IntakeAirTemp = 20;
static const uint16 ComInitValue_VehicleSpeed = 0;
static const uint16 ComInitValue_WheelSpeed_FL = 0;
static const uint16 ComInitValue_WheelSpeed_FR = 0;
static const uint8 ComInitValue_GearPosition = 0;
static const uint8 ComInitValue_TransmissionMode = 1;
static const boolean ComInitValue_ParkingBrake = FALSE;
static const boolean ComInitValue_TurnSignalLeft = FALSE;
static const boolean ComInitValue_TurnSignalRight = FALSE;
static const boolean ComInitValue_Headlights = FALSE;

/* Signal Configurations */
static const Com_SignalConfigType ComSignals[] = {
    /* Engine Speed - 0-8000 RPM, 16-bit */
    {
        .SignalId = ComConf_ComSignal_EngineSpeed,
        .DataPtr = &ComIPdu_EngineData_Buffer[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_EngineSpeed
    },
    /* Coolant Temperature - -40 to 215°C, 8-bit with offset */
    {
        .SignalId = ComConf_ComSignal_CoolantTemp,
        .DataPtr = &ComIPdu_EngineData_Buffer[2],
        .BitPosition = 16,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_SINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_CoolantTemp
    },
    /* Throttle Position - 0-100%, 8-bit */
    {
        .SignalId = ComConf_ComSignal_ThrottlePosition,
        .DataPtr = &ComIPdu_EngineData_Buffer[3],
        .BitPosition = 24,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_ThrottlePosition
    },
    /* Engine Torque - 0-8000 Nm, 16-bit */
    {
        .SignalId = ComConf_ComSignal_EngineTorque,
        .DataPtr = &ComIPdu_EngineData_Buffer[4],
        .BitPosition = 32,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_EngineTorque
    },
    /* Engine State - 0=Stopped, 1=Starting, 2=Running, 3=Stopping */
    {
        .SignalId = ComConf_ComSignal_EngineState,
        .DataPtr = &ComIPdu_EngineData_Buffer[6],
        .BitPosition = 48,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_EngineState
    },
    /* Battery Voltage - 0-25.5V, resolution 0.1V/bit */
    {
        .SignalId = ComConf_ComSignal_BatteryVoltage,
        .DataPtr = &ComIPdu_EngineData_Buffer[7],
        .BitPosition = 56,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_BatteryVoltage
    },
    /* Oil Pressure - 0-12.75 bar, resolution 0.05 bar/bit */
    {
        .SignalId = ComConf_ComSignal_OilPressure,
        .DataPtr = &ComIPdu_EngineStatus_Buffer[0],
        .BitPosition = 0,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_OilPressure
    },
    /* Oil Temperature - -40 to 215°C, 8-bit with offset */
    {
        .SignalId = ComConf_ComSignal_OilTemp,
        .DataPtr = &ComIPdu_EngineStatus_Buffer[1],
        .BitPosition = 8,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_SINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_OilTemp
    },
    /* Fuel Level - 0-100%, 8-bit */
    {
        .SignalId = ComConf_ComSignal_FuelLevel,
        .DataPtr = &ComIPdu_EngineStatus_Buffer[2],
        .BitPosition = 16,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_FuelLevel
    },
    /* Intake Air Temperature - -40 to 215°C, 8-bit with offset */
    {
        .SignalId = ComConf_ComSignal_IntakeAirTemp,
        .DataPtr = &ComIPdu_EngineStatus_Buffer[3],
        .BitPosition = 24,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_SINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_IntakeAirTemp
    },
    /* Vehicle Speed - 0-300 km/h, 16-bit */
    {
        .SignalId = ComConf_ComSignal_VehicleSpeed,
        .DataPtr = &ComIPdu_VehicleSpeed_Buffer[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_VehicleSpeed
    },
    /* Front Left Wheel Speed - 0-300 km/h, 16-bit */
    {
        .SignalId = ComConf_ComSignal_WheelSpeed_FL,
        .DataPtr = &ComIPdu_VehicleSpeed_Buffer[2],
        .BitPosition = 16,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_WheelSpeed_FL
    },
    /* Front Right Wheel Speed - 0-300 km/h, 16-bit */
    {
        .SignalId = ComConf_ComSignal_WheelSpeed_FR,
        .DataPtr = &ComIPdu_VehicleSpeed_Buffer[4],
        .BitPosition = 32,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_WheelSpeed_FR
    },
    /* Gear Position - 0=P, 1=R, 2=N, 3=D, 4-10=Gear 1-7 */
    {
        .SignalId = ComConf_ComSignal_GearPosition,
        .DataPtr = &ComIPdu_BodyControl_Buffer[0],
        .BitPosition = 0,
        .BitSize = 4,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_GearPosition
    },
    /* Transmission Mode - 0=Eco, 1=Normal, 2=Sport */
    {
        .SignalId = ComConf_ComSignal_TransmissionMode,
        .DataPtr = &ComIPdu_BodyControl_Buffer[0],
        .BitPosition = 4,
        .BitSize = 2,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_TransmissionMode
    },
    /* Parking Brake - Boolean */
    {
        .SignalId = ComConf_ComSignal_ParkingBrake,
        .DataPtr = &ComIPdu_BodyControl_Buffer[0],
        .BitPosition = 6,
        .BitSize = 1,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_BOOLEAN,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_ParkingBrake
    },
    /* Left Turn Signal - Boolean */
    {
        .SignalId = ComConf_ComSignal_TurnSignalLeft,
        .DataPtr = &ComIPdu_BodyControl_Buffer[0],
        .BitPosition = 7,
        .BitSize = 1,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_BOOLEAN,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_TurnSignalLeft
    },
    /* Right Turn Signal - Boolean */
    {
        .SignalId = ComConf_ComSignal_TurnSignalRight,
        .DataPtr = &ComIPdu_BodyControl_Buffer[1],
        .BitPosition = 8,
        .BitSize = 1,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_BOOLEAN,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_TurnSignalRight
    },
    /* Headlights - Boolean */
    {
        .SignalId = ComConf_ComSignal_Headlights,
        .DataPtr = &ComIPdu_BodyControl_Buffer[1],
        .BitPosition = 9,
        .BitSize = 1,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_BOOLEAN,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_Headlights
    }
};

/*==================[Signal Group Configuration]============================*/

static const Com_SignalGroupConfigType ComSignalGroups[] = {
    /* Engine Core Info - Primary engine parameters */
    {
        .SignalGroupId = ComConf_ComSignalGroup_EngineCoreInfo,
        .SignalRefs = ComEngineCoreInfo_Signals,
        .NumSignals = 4,
        .ShadowBuffer = ComShadowBuffer_EngineCoreInfo,
        .ComNotification = NULL_PTR
    },
    /* Engine Diagnostics - Secondary engine parameters */
    {
        .SignalGroupId = ComConf_ComSignalGroup_EngineDiagnostics,
        .SignalRefs = ComEngineDiagnostics_Signals,
        .NumSignals = 4,
        .ShadowBuffer = ComShadowBuffer_EngineDiagnostics,
        .ComNotification = NULL_PTR
    },
    /* Vehicle Dynamics - Speed-related parameters */
    {
        .SignalGroupId = ComConf_ComSignalGroup_VehicleDynamics,
        .SignalRefs = ComVehicleDynamics_Signals,
        .NumSignals = 3,
        .ShadowBuffer = ComShadowBuffer_VehicleDynamics,
        .ComNotification = NULL_PTR
    }
};

/*==================[T013: Error Handling Configuration]====================*/

#include "Com_ErrorHandling.h"

/* Error handling configuration for each I-PDU */
const Com_ErrorHandlingConfigType Com_ErrorHandlingConfig[COM_MAX_IPDUS] = {
    /* Engine Data I-PDU - Use DROP_OLDEST for critical engine data */
    {
        .OverflowStrategy = COM_TXQUEUE_DROP_OLDEST,
        .EnableErrorNotification = TRUE,
        .ErrorNotification = NULL_PTR,  /* No custom notification callback */
        .MaxErrorsBeforeNotification = 5u
    },
    /* Engine Status I-PDU - Use REJECT_NEWEST for non-critical data */
    {
        .OverflowStrategy = COM_TXQUEUE_REJECT_NEWEST,
        .EnableErrorNotification = TRUE,
        .ErrorNotification = NULL_PTR,
        .MaxErrorsBeforeNotification = 10u
    },
    /* Vehicle Speed I-PDU - Use DROP_OLDEST for chassis critical data */
    {
        .OverflowStrategy = COM_TXQUEUE_DROP_OLDEST,
        .EnableErrorNotification = TRUE,
        .ErrorNotification = NULL_PTR,
        .MaxErrorsBeforeNotification = 3u
    },
    /* Body Control I-PDU - Use REJECT_NEWEST for body control */
    {
        .OverflowStrategy = COM_TXQUEUE_REJECT_NEWEST,
        .EnableErrorNotification = FALSE,
        .ErrorNotification = NULL_PTR,
        .MaxErrorsBeforeNotification = 0u
    },
    /* Default configuration for remaining I-PDUs */
    [4 ... (COM_MAX_IPDUS - 1)] = {
        .OverflowStrategy = COM_TXQUEUE_REJECT_NEWEST,
        .EnableErrorNotification = FALSE,
        .ErrorNotification = NULL_PTR,
        .MaxErrorsBeforeNotification = 0u
    }
};

/*==================[Global Configuration]==================================*/

const Com_ConfigType ComConfig = {
    .Signals = ComSignals,
    .NumSignals = 19,
    .SignalGroups = ComSignalGroups,
    .NumSignalGroups = 3,
    .IPdus = ComIPdus,
    .NumIPdus = 4,
    .IPduGroups = ComIPduGroups,
    .NumIPduGroups = 3
};

/*==================[End of File]==========================================*/
