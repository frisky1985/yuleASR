/******************************************************************************
 * @file    powertrain_types.h
 * @brief   Powertrain Control System - Data Types Definition
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level for critical control data
 *
 * Topics:
 *   - Motor Status
 *   - Battery Management System (BMS)
 *   - Vehicle Control (VCU)
 *   - Energy Recovery
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef POWERTRAIN_TYPES_H
#define POWERTRAIN_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../src/common/autosar_types.h"

/* ============================================================================
 * Version Information
 * ============================================================================ */
#define POWERTRAIN_MAJOR_VERSION        1
#define POWERTRAIN_MINOR_VERSION        0
#define POWERTRAIN_PATCH_VERSION        0

/* ============================================================================
 * Constants
 * ============================================================================ */
#define PT_MAX_MOTORS                   4
#define PT_MAX_BATTERY_CELLS            192
#define PT_MAX_TEMPERATURE_SENSORS      32
#define PT_MAX_FAULT_CODES              64
#define PT_MAX_CAN_NODES                16

/* ============================================================================
 * Common Types
 * ============================================================================ */

typedef enum {
    PT_MODE_STANDBY = 0,
    PT_MODE_DRIVE,
    PT_MODE_REVERSE,
    PT_MODE_CHARGE,
    PT_MODE_FAULT,
    PT_MODE_EMERGENCY_SHUTDOWN
} PtOperatingModeType;

typedef enum {
    PT_FAULT_NONE = 0,
    PT_FAULT_OVER_CURRENT,
    PT_FAULT_OVER_VOLTAGE,
    PT_FAULT_UNDER_VOLTAGE,
    PT_FAULT_OVER_TEMP,
    PT_FAULT_INSULATION_FAULT,
    PT_FAULT_COMMUNICATION,
    PT_FAULT_SENSOR_FAULT,
    PT_FAULT_EMERGENCY_STOP,
    PT_FAULT_PRECHARGE_FAIL,
    PT_FAULT_ISOLATION_FAULT
} PtFaultType;

typedef enum {
    PT_HEALTH_OK = 0,
    PT_HEALTH_DEGRADED,
    PT_HEALTH_CRITICAL,
    PT_HEALTH_EMERGENCY
} PtHealthStatusType;

/* ============================================================================
 * Motor Control Types
 * ============================================================================ */

typedef enum {
    MOTOR_STATE_OFF = 0,
    MOTOR_STATE_READY,
    MOTOR_STATE_RUNNING,
    MOTOR_STATE_REGEN,
    MOTOR_STATE_FAULT,
    MOTOR_STATE_DIAGNOSTIC
} MotorStateType;

typedef enum {
    MOTOR_TYPE_PMSM = 0,    /* Permanent Magnet Synchronous Motor */
    MOTOR_TYPE_ACIM,        /* AC Induction Motor */
    MOTOR_TYPE_SRM,         /* Switched Reluctance Motor */
    MOTOR_TYPE_BLDC         /* Brushless DC Motor */
} MotorTypeType;

typedef struct {
    uint32_t motorId;
    MotorStateType state;
    MotorTypeType type;
    
    /* Electrical parameters */
    float32 dcLinkVoltage;      /* Volts */
    float32 phaseCurrentA;      /* Amps */
    float32 phaseCurrentB;      /* Amps */
    float32 phaseCurrentC;      /* Amps */
    float32 busCurrent;         /* Amps */
    
    /* Mechanical parameters */
    float32 speedRpm;           /* RPM */
    float32 torqueNm;           /* Newton-meters */
    float32 torqueCommand;      /* Newton-meters */
    float32 rotorPosition;      /* Radians */
    
    /* Temperature */
    float32 statorTempC;        /* Celsius */
    float32 inverterTempC;      /* Celsius */
    float32 rotorTempC;         /* Celsius */
    
    /* Control */
    uint16_t pwmDutyCycle;      /* 0-1000 (0.1% resolution) */
    float32 electricalAngle;    /* Radians */
    float32 modulationIndex;    /* 0.0 - 1.0 */
    
    /* Status */
    uint32_t faultCode;
    uint32_t warningFlags;
    bool activeDischargeEnabled;
    bool prechargeComplete;
    
    /* Timestamp */
    uint64_t timestampUs;
} MotorStatusType;

typedef struct {
    uint32_t systemId;
    uint8_t motorCount;
    MotorStatusType motors[PT_MAX_MOTORS];
    
    /* Combined status */
    float32 totalPowerKw;       /* Kilowatts */
    float32 totalTorqueNm;      /* Newton-meters */
    float32 mechanicalPowerKw;  /* Kilowatts */
    float32 electricalPowerKw;  /* Kilowatts */
    float32 efficiency;         /* 0.0 - 1.0 */
    
    /* Thermal management */
    float32 coolantTempInC;     /* Celsius */
    float32 coolantTempOutC;    /* Celsius */
    float32 coolantFlowLpm;     /* Liters per minute */
    uint8_t coolingPumpSpeed;   /* 0-100% */
    
    PtHealthStatusType health;
    uint64_t timestampUs;
} MotorSystemStatusType;

/* ============================================================================
 * Battery Management System (BMS) Types
 * ============================================================================ */

typedef enum {
    BMS_STATE_INIT = 0,
    BMS_STATE_STANDBY,
    BMS_STATE_PRECHARGE,
    BMS_STATE_READY,
    BMS_STATE_DRIVE,
    BMS_STATE_CHARGE,
    BMS_STATE_FAULT,
    BMS_STATE_SHUTDOWN
} BmsStateType;

typedef enum {
    CELL_STATUS_OK = 0,
    CELL_STATUS_UNDERVOLTAGE,
    CELL_STATUS_OVERVOLTAGE,
    CELL_STATUS_OVERTEMP,
    CELL_STATUS_UNDERTEMP,
    CELL_STATUS_IMBALANCE,
    CELL_STATUS_FAULT
} CellStatusType;

typedef struct {
    uint16_t cellId;
    float32 voltage;            /* Volts */
    float32 temperature;        /* Celsius */
    float32 internalResistance; /* Ohms */
    CellStatusType status;
    uint8_t balanceStatus;      /* 0-100% */
    bool balancingActive;
} BatteryCellType;

typedef struct {
    uint32_t packId;
    BmsStateType state;
    
    /* Voltage */
    float32 packVoltage;        /* Volts */
    float32 minCellVoltage;     /* Volts */
    float32 maxCellVoltage;     /* Volts */
    float32 avgCellVoltage;     /* Volts */
    uint16_t minCellId;
    uint16_t maxCellId;
    float32 voltageImbalance;   /* Volts */
    
    /* Current */
    float32 packCurrent;        /* Amps (positive = discharge) */
    float32 chargeCurrentLimit; /* Amps */
    float32 dischargeCurrentLimit; /* Amps */
    float32 peakCurrent;        /* Amps */
    
    /* Temperature */
    float32 maxTemp;            /* Celsius */
    float32 minTemp;            /* Celsius */
    float32 avgTemp;            /* Celsius */
    uint16_t maxTempSensorId;
    uint16_t minTempSensorId;
    
    /* State of Charge / Energy */
    float32 socPercent;         /* 0.0 - 100.0 */
    float32 sohPercent;         /* 0.0 - 100.0 */
    float32 availableEnergyKwh; /* Kilowatt-hours */
    float32 remainingCapacityAh; /* Amp-hours */
    
    /* Power */
    float32 instantaneousPowerKw; /* Kilowatts */
    float32 maxDischargePowerKw;  /* Kilowatts */
    float32 maxRegenPowerKw;      /* Kilowatts */
    
    /* Contactors */
    bool mainContactorPositive;
    bool mainContactorNegative;
    bool prechargeContactor;
    bool chargeContactor;
    
    /* Isolation */
    float32 isolationResistanceKohm; /* Kiloohms */
    bool isolationFault;
    
    /* Cell data */
    uint16_t cellCount;
    uint16_t modulesInSeries;
    uint16_t cellsInParallel;
    BatteryCellType cells[PT_MAX_BATTERY_CELLS];
    
    /* Health */
    uint32_t cycleCount;
    uint32_t faultCount;
    uint32_t warningFlags;
    PtHealthStatusType health;
    
    uint64_t timestampUs;
} BmsStatusType;

/* ============================================================================
 * Vehicle Control Unit (VCU) Types
 * ============================================================================ */

typedef enum {
    VCU_STATE_INIT = 0,
    VCU_STATE_STANDBY,
    VCU_STATE_READY,
    VCU_STATE_DRIVE,
    VCU_STATE_REGEN,
    VCU_STATE_CHARGE,
    VCU_STATE_FAULT,
    VCU_STATE_EMERGENCY
} VcuStateType;

typedef enum {
    GEAR_PARK = 0,
    GEAR_REVERSE,
    GEAR_NEUTRAL,
    GEAR_DRIVE,
    GEAR_LOW
} GearPositionType;

typedef struct {
    uint32_t vcuId;
    VcuStateType state;
    PtOperatingModeType mode;
    
    /* Vehicle dynamics */
    float32 vehicleSpeedKph;    /* km/h */
    float32 accelerationMss;    /* m/s² */
    float32 decelerationMss;    /* m/s² */
    float32 lateralAcceleration; /* m/s² */
    float32 yawRateDps;         /* deg/s */
    
    /* Pedal positions */
    float32 acceleratorPedal;   /* 0.0 - 1.0 */
    float32 brakePedal;         /* 0.0 - 1.0 */
    bool brakeSwitch;
    bool brakeSwitch2;
    
    /* Steering */
    float32 steeringAngle;      /* degrees */
    float32 steeringRate;       /* deg/s */
    
    /* Gear */
    GearPositionType gearPosition;
    GearPositionType requestedGear;
    bool gearShifterLocked;
    
    /* Driver commands */
    float32 requestedTorque;    /* Nm */
    float32 requestedPower;     /* kW */
    bool cruiseControlEnabled;
    float32 cruiseControlSpeed; /* km/h */
    
    /* Vehicle state */
    float32 odometerKm;         /* km */
    float32 tripMeterKm;        /* km */
    uint32_t ignitionCycles;
    
    /* Ready state */
    bool readyToDrive;
    bool highVoltageActive;
    bool chargingConnected;
    bool chargingActive;
    
    /* Status */
    uint32_t faultCodes[PT_MAX_FAULT_CODES];
    uint32_t faultCount;
    PtHealthStatusType health;
    
    uint64_t timestampUs;
} VcuStatusType;

/* ============================================================================
 * Energy Recovery Types
 * ============================================================================ */

typedef enum {
    REGEN_MODE_OFF = 0,
    REGEN_MODE_LOW,
    REGEN_MODE_MEDIUM,
    REGEN_MODE_HIGH,
    REGEN_MODE_AUTO
} RegenModeType;

typedef struct {
    uint32_t systemId;
    RegenModeType mode;
    bool enabled;
    
    /* Current regeneration */
    float32 regenCurrent;       /* Amps */
    float32 regenPowerKw;       /* Kilowatts */
    float32 regenTorqueNm;      /* Newton-meters */
    
    /* Efficiency */
    float32 energyRecoveredKwh; /* Total kWh recovered */
    float32 regenEfficiency;    /* 0.0 - 1.0 */
    
    /* Limits */
    float32 maxRegenPowerKw;    /* kW */
    float32 maxRegenTorqueNm;   /* Nm */
    float32 regenSpeedThreshold; /* RPM below which regen is disabled */
    
    /* SOC-based limits */
    float32 socRegenLimitStart; /* SOC% where regen starts limiting */
    float32 socRegenLimitEnd;   /* SOC% where regen is disabled */
    
    /* Thermal limits */
    float32 batteryTempLimitC;  /* Temp where regen is reduced */
    float32 motorTempLimitC;    /* Temp where regen is reduced */
    
    /* Statistics */
    float32 totalDistanceRegenKm; /* km driven with regen active */
    uint32_t regenEvents;
    
    uint64_t timestampUs;
} RegenStatusType;

/* ============================================================================
 * Powertrain Command Types
 * ============================================================================ */

typedef struct {
    uint32_t commandId;
    
    /* Motor commands */
    float32 torqueCommand[PT_MAX_MOTORS];     /* Nm */
    float32 speedCommand[PT_MAX_MOTORS];      /* RPM */
    bool enableInverter[PT_MAX_MOTORS];
    
    /* Mode commands */
    PtOperatingModeType requestedMode;
    GearPositionType requestedGear;
    
    /* Regen command */
    RegenModeType regenMode;
    float32 regenLevel;          /* 0.0 - 1.0 */
    
    /* Torque limits */
    float32 maxTorqueLimit;      /* Nm */
    float32 maxPowerLimit;       /* kW */
    
    /* Safety */
    bool emergencyShutdown;
    bool clearFaults;
    
    /* E2E Protection */
    uint32_t sequenceCounter;
    uint32_t crc;
    
    uint64_t timestampUs;
} PowertrainCommandType;

/* ============================================================================
 * E2E Protection Configuration
 * ============================================================================ */

#define PT_E2E_PROFILE              E2E_PROFILE_07
#define PT_E2E_DATAID_MOTOR         0x1001
#define PT_E2E_DATAID_BMS           0x1002
#define PT_E2E_DATAID_VCU           0x1003
#define PT_E2E_DATAID_REGEN         0x1004
#define PT_E2E_DATAID_COMMAND       0x1005

#ifdef __cplusplus
}
#endif

#endif /* POWERTRAIN_TYPES_H */
