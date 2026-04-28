/*
 * Com_Lcfg.c
 * COM Module Link-Time Configuration
 * Example configuration for Engine Data Application
 */

#include "Com.h"

/*==================[Buffer Declarations]===================================*/

/* IPDU Buffers */
static uint8 ComIPdu_EngineData_Buffer[8];
static uint8 ComIPdu_VehicleDynamics_Buffer[8];
static uint8 ComIPdu_BodyControl_Buffer[8];

/* Shadow Buffers for Signal Groups */
static uint8 ComShadowBuffer_EngineInfo[5];
static uint8 ComShadowBuffer_VehicleSpeed[4];

/*==================[IPDU Group Configuration]================================*/

static const Com_IPduGroupConfigType ComIPduGroups[] = {
    {
        .IpduGroupId = ComConf_ComIPduGroup_EngineGroup,
        .IPduRefs = (Com_IPduIdType[]){ComConf_ComIPdu_EngineData},
        .NumIPdus = 1
    },
    {
        .IpduGroupId = ComConf_ComIPduGroup_ChassisGroup,
        .IPduRefs = (Com_IPduIdType[]){ComConf_ComIPdu_VehicleSpeed},
        .NumIPdus = 1
    },
    {
        .IpduGroupId = ComConf_ComIPduGroup_BodyGroup,
        .IPduRefs = (Com_IPduIdType[]){ComConf_ComIPdu_GearPosition},
        .NumIPdus = 1
    }
};

/*==================[Signal References]=====================================*/

/* Engine Data IPDU Signals */
static const Com_SignalIdType ComEngineData_Signals[] = {
    ComConf_ComSignal_EngineSpeed,
    ComConf_ComSignal_EngineTemp,
    ComConf_ComSignal_AcceleratorPos
};

/* Vehicle Dynamics IPDU Signals */
static const Com_SignalIdType ComVehicleSpeed_Signals[] = {
    ComConf_ComSignal_VehicleSpeed
};

/* Body Control IPDU Signals */
static const Com_SignalIdType ComGearPosition_Signals[] = {
    ComConf_ComSignal_GearPosition
};

/*==================[Signal Group References]===============================*/

/* Engine Info Signal Group */
static const Com_SignalIdType ComEngineInfo_Signals[] = {
    ComConf_ComSignal_EngineSpeed,
    ComConf_ComSignal_EngineTemp
};

/*==================[IPDU Configuration]====================================*/

static const Com_IPduConfigType ComIPdus[] = {
    /* Engine Data IPDU - Periodic Transmission */
    {
        .IPduId = ComConf_ComIPdu_EngineData,
        .DataPtr = ComIPdu_EngineData_Buffer,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = ComEngineData_Signals,
        .NumSignals = 3,
        .SignalGroupRefs = (Com_SignalGroupIdType[]){ComConf_ComSignalGroup_EngineInfo},
        .NumSignalGroups = 1,
        .TxMode = {
            .Mode = COM_PERIODIC,
            .Period = 100,          /* 100ms */
            .RepetitionPeriod = 0,
            .NumRepetitions = 0,
            .TimeOffset = 0
        },
        .IpduGroupRefs = (Com_IpduGroupIdType[]){ComConf_ComIPduGroup_EngineGroup},
        .NumIpduGroups = 1,
        .Timeout = 0,
        .ComIPduCallout = NULL_PTR
    },
    /* Vehicle Speed IPDU - Event Triggered */
    {
        .IPduId = ComConf_ComIPdu_VehicleSpeed,
        .DataPtr = ComIPdu_VehicleDynamics_Buffer,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = ComVehicleSpeed_Signals,
        .NumSignals = 1,
        .SignalGroupRefs = NULL_PTR,
        .NumSignalGroups = 0,
        .TxMode = {
            .Mode = COM_DIRECT,
            .Period = 0,
            .RepetitionPeriod = 0,
            .NumRepetitions = 0,
            .TimeOffset = 0
        },
        .IpduGroupRefs = (Com_IpduGroupIdType[]){ComConf_ComIPduGroup_ChassisGroup},
        .NumIpduGroups = 1,
        .Timeout = 0,
        .ComIPduCallout = NULL_PTR
    },
    /* Gear Position IPDU - Mixed Mode */
    {
        .IPduId = ComConf_ComIPdu_GearPosition,
        .DataPtr = ComIPdu_BodyControl_Buffer,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = ComGearPosition_Signals,
        .NumSignals = 1,
        .SignalGroupRefs = NULL_PTR,
        .NumSignalGroups = 0,
        .TxMode = {
            .Mode = COM_MIXED,
            .Period = 500,          /* 500ms periodic */
            .RepetitionPeriod = 20, /* 20ms between repetitions */
            .NumRepetitions = 3,    /* 3 repetitions on trigger */
            .TimeOffset = 100       /* 100ms initial offset */
        },
        .IpduGroupRefs = (Com_IpduGroupIdType[]){ComConf_ComIPduGroup_BodyGroup},
        .NumIpduGroups = 1,
        .Timeout = 0,
        .ComIPduCallout = NULL_PTR
    }
};

/*==================[Signal Configuration]=================================*/

/* Initial Values */
static const uint16 ComInitValue_EngineSpeed = 0;
static const uint8 ComInitValue_EngineTemp = 20;
static const uint8 ComInitValue_AcceleratorPos = 0;
static const uint16 ComInitValue_VehicleSpeed = 0;
static const uint8 ComInitValue_GearPosition = 0;

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
    /* Engine Temperature - -40 to 200°C, 8-bit */
    {
        .SignalId = ComConf_ComSignal_EngineTemp,
        .DataPtr = &ComIPdu_EngineData_Buffer[2],
        .BitPosition = 0,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_EngineTemp
    },
    /* Accelerator Position - 0-100%, 8-bit */
    {
        .SignalId = ComConf_ComSignal_AcceleratorPos,
        .DataPtr = &ComIPdu_EngineData_Buffer[3],
        .BitPosition = 0,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_AcceleratorPos
    },
    /* Vehicle Speed - 0-300 km/h, 16-bit */
    {
        .SignalId = ComConf_ComSignal_VehicleSpeed,
        .DataPtr = &ComIPdu_VehicleDynamics_Buffer[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_VehicleSpeed
    },
    /* Gear Position - 0-10 (P,R,N,D...), 8-bit */
    {
        .SignalId = ComConf_ComSignal_GearPosition,
        .DataPtr = &ComIPdu_BodyControl_Buffer[0],
        .BitPosition = 0,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = &ComInitValue_GearPosition
    }
};

/*==================[Signal Group Configuration]============================*/

static const Com_SignalGroupConfigType ComSignalGroups[] = {
    {
        .SignalGroupId = ComConf_ComSignalGroup_EngineInfo,
        .SignalRefs = ComEngineInfo_Signals,
        .NumSignals = 2,
        .ShadowBuffer = ComShadowBuffer_EngineInfo,
        .ComNotification = NULL_PTR
    }
};

/*==================[Global Configuration]==================================*/

const Com_ConfigType ComConfig = {
    .Signals = ComSignals,
    .NumSignals = 5,
    .SignalGroups = ComSignalGroups,
    .NumSignalGroups = 1,
    .IPdus = ComIPdus,
    .NumIPdus = 3,
    .IPduGroups = ComIPduGroups,
    .NumIPduGroups = 3
};

/*==================[End of File]==========================================*/
