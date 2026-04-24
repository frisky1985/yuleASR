/******************************************************************************
 * @file    body_types.h
 * @brief   Body Electronics System - Data Types Definition
 *
 * AUTOSAR R22-11 compliant
 * ASIL-B Safety Level for body control data
 *
 * Topics:
 *   - Seat Control
 *   - HVAC (Climate Control)
 *   - Door Management
 *   - Lighting System
 *   - Mirror Control
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef BODY_TYPES_H
#define BODY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../src/common/autosar_types.h"

/* ============================================================================
 * Version Information
 * ============================================================================ */
#define BODY_MAJOR_VERSION      1
#define BODY_MINOR_VERSION      0
#define BODY_PATCH_VERSION      0

/* ============================================================================
 * Constants
 * ============================================================================ */
#define BODY_MAX_SEATS          8
#define BODY_MAX_DOORS          8
#define BODY_MAX_ZONES          4
#define BODY_MAX_LIGHTS         32
#define BODY_MAX_FAULT_CODES    32

/* ============================================================================
 * Common Types
 * ============================================================================ */

typedef enum {
    BODY_ZONE_FRONT = 0,
    BODY_ZONE_REAR,
    BODY_ZONE_LEFT,
    BODY_ZONE_RIGHT,
    BODY_ZONE_CENTER
} BodyZoneType;

typedef enum {
    BODY_STATUS_OK = 0,
    BODY_STATUS_DEGRADED,
    BODY_STATUS_FAULT,
    BODY_STATUS_OFFLINE
} BodyStatusType;

/* ============================================================================
 * Seat Control Types
 * ============================================================================ */

typedef enum {
    SEAT_POSITION_DRIVER = 0,
    SEAT_POSITION_FRONT_PASSENGER,
    SEAT_POSITION_REAR_LEFT,
    SEAT_POSITION_REAR_RIGHT,
    SEAT_POSITION_REAR_CENTER,
    SEAT_POSITION_THIRD_ROW_LEFT,
    SEAT_POSITION_THIRD_ROW_RIGHT,
    SEAT_POSITION_THIRD_ROW_CENTER
} SeatPositionType;

typedef struct {
    uint8_t seatId;
    SeatPositionType position;
    bool occupied;
    bool beltBuckled;
    
    /* Position settings */
    float32 slidePosition;      /* 0.0 - 1.0 (rearward to forward) */
    float32 reclineAngle;       /* 0.0 - 1.0 (upright to reclined) */
    float32 heightPosition;     /* 0.0 - 1.0 (low to high) */
    float32 tiltPosition;       /* 0.0 - 1.0 (cushion tilt) */
    float32 lumbarSupport;      /* 0.0 - 1.0 (lumbar support level) */
    
    /* Memory positions */
    uint8_t activeMemorySlot;   /* 0 = none, 1-3 = memory slots */
    bool memoryRecallActive;
    
    /* Heating/Cooling */
    int8_t heatingLevel;        /* -3 to +3 (cooling to heating) */
    bool ventilationEnabled;
    float32 surfaceTemp;        /* Celsius */
    
    /* Massage */
    bool massageEnabled;
    uint8_t massageProgram;     /* 0-5 */
    uint8_t massageIntensity;   /* 0-3 */
    
    /* Safety */
    bool airbagEnabled;
    uint8_t airbagSuppression;  /* 0 = enabled, 1 = suppressed */
    
    uint64_t timestampUs;
} SeatStatusType;

typedef struct {
    uint32_t systemId;
    uint8_t seatCount;
    SeatStatusType seats[BODY_MAX_SEATS];
    BodyStatusType systemStatus;
    uint64_t timestampUs;
} SeatSystemStatusType;

/* ============================================================================
 * HVAC Types
 * ============================================================================ */

typedef enum {
    HVAC_MODE_OFF = 0,
    HVAC_MODE_AUTO,
    HVAC_MODE_COOL,
    HVAC_MODE_HEAT,
    HVAC_MODE_DEFROST,
    HVAC_MODE_DEFOG,
    HVAC_MODE_RECIRCULATE
} HvacModeType;

typedef enum {
    FAN_SPEED_OFF = 0,
    FAN_SPEED_LOW = 1,
    FAN_SPEED_MEDIUM = 3,
    FAN_SPEED_HIGH = 6,
    FAN_SPEED_MAX = 8
} FanSpeedType;

typedef struct {
    BodyZoneType zone;
    
    /* Temperature control */
    float32 targetTempC;        /* Celsius */
    float32 actualTempC;        /* Celsius */
    float32 ventTempC;          /* Celsius */
    
    /* Airflow */
    FanSpeedType fanSpeed;
    uint8_t fanSpeedPercent;    /* 0-100 */
    
    /* Vent positions */
    bool ventFace;
    bool ventFoot;
    bool ventDefrost;
    
    /* Dual zone control */
    bool syncEnabled;
    float32 tempOffset;         /* Temperature offset from master */
    
    uint64_t timestampUs;
} HvacZoneType;

typedef struct {
    uint32_t systemId;
    HvacModeType mode;
    bool autoMode;
    bool acEnabled;
    bool recirculationEnabled;
    
    /* Zones */
    uint8_t zoneCount;
    HvacZoneType zones[BODY_MAX_ZONES];
    
    /* Air quality */
    float32 cabinTempC;
    float32 outsideTempC;
    float32 cabinHumidity;      /* 0.0 - 1.0 */
    uint16_t co2LevelPpm;
    uint16_t airQualityIndex;   /* 0-500 AQI */
    
    /* Compressor */
    bool compressorRunning;
    float32 compressorSpeedRpm;
    float32 refrigerantPressure; /* Bar */
    
    /* Filters */
    uint8_t cabinFilterLifePercent;
    bool filterReplaceWarning;
    
    /* Rear defrost */
    bool rearDefrostEnabled;
    uint16_t rearDefrostRemainingSec;
    
    /* Heated elements */
    bool heatedWindshield;
    bool heatedRearWindow;
    bool heatedSteeringWheel;
    
    /* Energy */
    float32 powerConsumptionKw;
    
    BodyStatusType systemStatus;
    uint64_t timestampUs;
} HvacStatusType;

/* ============================================================================
 * Door Management Types
 * ============================================================================ */

typedef enum {
    DOOR_POSITION_FRONT_LEFT = 0,
    DOOR_POSITION_FRONT_RIGHT,
    DOOR_POSITION_REAR_LEFT,
    DOOR_POSITION_REAR_RIGHT,
    DOOR_POSITION_TAILGATE,
    DOOR_POSITION_HOOD,
    DOOR_POSITION_CHARGE_PORT,
    DOOR_POSITION_FUEL_DOOR
} DoorPositionType;

typedef enum {
    DOOR_STATE_CLOSED_LOCKED = 0,
    DOOR_STATE_CLOSED_UNLOCKED,
    DOOR_STATE_AJAR,
    DOOR_STATE_OPEN,
    DOOR_STATE_FAULT
} DoorStateType;

typedef struct {
    uint8_t doorId;
    DoorPositionType position;
    DoorStateType state;
    
    /* Position sensor (for sliding/power doors) */
    float32 openPercent;        /* 0.0 - 1.0 */
    
    /* Lock status */
    bool locked;
    bool childLockEnabled;
    bool deadlockEnabled;
    
    /* Window */
    float32 windowPosition;     /* 0.0 - 1.0 (closed to open) */
    bool windowObstruction;
    bool autoCloseEnabled;
    
    /* Handle sensors */
    bool handleTouched;
    bool handlePulled;
    bool keylessEntryRequest;
    
    /* Power operation */
    bool powerOperationEnabled;
    bool powerOperationActive;
    uint8_t powerOperationProgress; /* 0-100 */
    
    /* Safety */
    bool antiPinchTriggered;
    uint8_t antiPinchCount;
    
    uint64_t timestampUs;
} DoorStatusType;

typedef struct {
    uint32_t systemId;
    uint8_t doorCount;
    DoorStatusType doors[BODY_MAX_DOORS];
    
    /* Central locking */
    bool centralLockLocked;
    bool centralLockDeadlocked;
    uint32_t lockCommandSource; /* Key fob, button, etc. */
    
    /* Walk-away locking */
    bool walkAwayLockEnabled;
    bool walkAwayLockActive;
    
    /* Speed auto-lock */
    bool speedAutoLockEnabled;
    bool speedAutoLockTriggered;
    
    /* Keyless entry */
    bool keylessEntryEnabled;
    uint8_t keyFobCount;
    uint32_t detectedKeyFobs[4];
    
    BodyStatusType systemStatus;
    uint64_t timestampUs;
} DoorSystemStatusType;

/* ============================================================================
 * Lighting System Types
 * ============================================================================ */

typedef enum {
    LIGHT_TYPE_HEADLIGHT_LOW = 0,
    LIGHT_TYPE_HEADLIGHT_HIGH,
    LIGHT_TYPE_DAYTIME_RUNNING,
    LIGHT_TYPE_FOG_FRONT,
    LIGHT_TYPE_FOG_REAR,
    LIGHT_TYPE_TAIL,
    LIGHT_TYPE_BRAKE,
    LIGHT_TYPE_TURN_LEFT,
    LIGHT_TYPE_TURN_RIGHT,
    LIGHT_TYPE_HAZARD,
    LIGHT_TYPE_REVERSE,
    LIGHT_TYPE_LICENSE_PLATE,
    LIGHT_TYPE_INTERIOR_FRONT,
    LIGHT_TYPE_INTERIOR_REAR,
    LIGHT_TYPE_AMBIENT,
    LIGHT_TYPE_PUDDLE,
    LIGHT_TYPE_WELCOME,
    LIGHT_TYPE_THUNDERBOLT
} LightTypeType;

typedef enum {
    LIGHT_STATE_OFF = 0,
    LIGHT_STATE_ON,
    LIGHT_STATE_DIMMED,
    LIGHT_STATE_BLINKING,
    LIGHT_STATE_FAULT
} LightStateType;

typedef struct {
    uint8_t lightId;
    LightTypeType type;
    LightStateType state;
    
    /* Brightness control */
    uint8_t brightnessPercent;  /* 0-100 */
    bool autoBrightness;
    
    /* Color (for RGB/ambient lights) */
    uint8_t red;                /* 0-255 */
    uint8_t green;              /* 0-255 */
    uint8_t blue;               /* 0-255 */
    
    /* Status */
    bool bulbFault;
    uint16_t currentMa;         /* Current draw in mA */
    
    uint64_t timestampUs;
} LightStatusType;

typedef struct {
    uint32_t systemId;
    uint8_t lightCount;
    LightStatusType lights[BODY_MAX_LIGHTS];
    
    /* Headlight control */
    bool autoHeadlights;
    bool adaptiveHeadlights;
    float32 headlightLeveling;  /* 0.0 - 1.0 */
    bool highBeamAssist;
    bool highBeamActive;
    
    /* Ambient lighting */
    bool ambientLightingEnabled;
    uint8_t ambientBrightness;  /* 0-100 */
    uint8_t ambientTheme;       /* 0-10 */
    
    /* Light sensor */
    float32 ambientLightLevel;  /* Lux */
    bool darknessDetected;
    
    /* Turn signals */
    bool hazardLightsActive;
    uint8_t turnSignalState;    /* 0=off, 1=left, 2=right, 3=hazard */
    
    /* Brake lights */
    bool brakeLightsActive;
    bool emergencyBrakingSignal;
    
    BodyStatusType systemStatus;
    uint64_t timestampUs;
} LightingSystemStatusType;

/* ============================================================================
 * Mirror Control Types
 * ============================================================================ */

typedef enum {
    MIRROR_POSITION_DRIVER = 0,
    MIRROR_POSITION_PASSENGER,
    MIRROR_POSITION_INTERIOR
} MirrorPositionType;

typedef struct {
    uint8_t mirrorId;
    MirrorPositionType position;
    
    /* Position */
    float32 panPosition;        /* -1.0 (left) to +1.0 (right) */
    float32 tiltPosition;       /* -1.0 (down) to +1.0 (up) */
    
    /* Features */
    bool heated;
    bool heatingActive;
    float32 heatingLevel;       /* 0.0 - 1.0 */
    
    bool dimmingEnabled;
    float32 dimmingLevel;       /* 0.0 - 1.0 */
    bool autoDimming;
    
    /* Turn signal (side mirrors) */
    bool turnSignalEnabled;
    bool turnSignalActive;
    
    /* Blind spot indicator */
    bool blindSpotMonitoring;
    bool blindSpotAlert;
    
    /* Memory */
    uint8_t memorySlot;
    bool memoryRecallActive;
    
    /* Folding */
    bool powerFold;
    bool folded;
    bool foldInProgress;
    
    uint64_t timestampUs;
} MirrorStatusType;

typedef struct {
    uint32_t systemId;
    uint8_t mirrorCount;
    MirrorStatusType mirrors[3];  /* Driver, Passenger, Interior */
    BodyStatusType systemStatus;
    uint64_t timestampUs;
} MirrorSystemStatusType;

/* ============================================================================
 * E2E Protection Configuration
 * ============================================================================ */

#define BODY_E2E_PROFILE            E2E_PROFILE_05  /* CRC16 for body electronics */
#define BODY_E2E_DATAID_SEAT        0x2001
#define BODY_E2E_DATAID_HVAC        0x2002
#define BODY_E2E_DATAID_DOOR        0x2003
#define BODY_E2E_DATAID_LIGHTING    0x2004
#define BODY_E2E_DATAID_MIRROR      0x2005

#ifdef __cplusplus
}
#endif

#endif /* BODY_TYPES_H */
