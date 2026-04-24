/******************************************************************************
 * @file    infotainment_types.h
 * @brief   Infotainment System - Data Types Definition
 *
 * Non-safety critical data (QM - Quality Managed)
 *
 * Topics:
 *   - HUD Display Data
 *   - Navigation Information
 *   - Media/Entertainment
 *   - User Interface
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef INFOTAINMENT_TYPES_H
#define INFOTAINMENT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../src/common/autosar_types.h"

#define INFOTAINMENT_MAJOR_VERSION      1
#define INFOTAINMENT_MINOR_VERSION      0
#define INFOTAINMENT_PATCH_VERSION      0

#define NAV_MAX_WAYPOINTS               50
#define NAV_MAX_STREET_NAME_LEN         128
#define MEDIA_MAX_TRACK_TITLE_LEN       256
#define MEDIA_MAX_ARTIST_LEN            128
#define MEDIA_MAX_ALBUM_LEN             128
#define HUD_MAX_TEXT_LEN                64

/* ============================================================================
 * Navigation Types
 * ============================================================================ */

typedef enum {
    NAV_MANEUVER_UNKNOWN = 0,
    NAV_MANEUVER_STRAIGHT,
    NAV_MANEUVER_TURN_SLIGHT_LEFT,
    NAV_MANEUVER_TURN_LEFT,
    NAV_MANEUVER_TURN_SHARP_LEFT,
    NAV_MANEUVER_UTURN_LEFT,
    NAV_MANEUVER_UTURN_RIGHT,
    NAV_MANEUVER_TURN_SHARP_RIGHT,
    NAV_MANEUVER_TURN_RIGHT,
    NAV_MANEUVER_TURN_SLIGHT_RIGHT,
    NAV_MANEUVER_RAMP_LEFT,
    NAV_MANEUVER_RAMP_RIGHT,
    NAV_MANEUVER_MERGE,
    NAV_MANEUVER_FORK_LEFT,
    NAV_MANEUVER_FORK_RIGHT,
    NAV_MANEUVER_ROUNDABOUT_ENTER,
    NAV_MANEUVER_ROUNDABOUT_EXIT,
    NAV_MANEUVER_DESTINATION
} NavManeuverType;

typedef enum {
    NAV_STATUS_IDLE = 0,
    NAV_STATUS_CALCULATING,
    NAV_STATUS_NAVIGATING,
    NAV_STATUS_REROUTING,
    NAV_STATUS_ARRIVED,
    NAV_STATUS_ERROR
} NavStatusType;

typedef enum {
    TRAFFIC_CONDITION_CLEAR = 0,
    TRAFFIC_CONDITION_LIGHT,
    TRAFFIC_CONDITION_MODERATE,
    TRAFFIC_CONDITION_HEAVY,
    TRAFFIC_CONDITION_SEVERE
} TrafficConditionType;

typedef struct {
    float32 latitude;
    float32 longitude;
    float32 altitude;
    float32 heading;
    float32 speedKph;
    uint32_t accuracyM;
    uint64_t timestampUs;
} GpsPositionType;

typedef struct {
    uint32_t waypointId;
    GpsPositionType position;
    char streetName[NAV_MAX_STREET_NAME_LEN];
    uint32_t distanceToNextM;
    uint32_t timeToNextSec;
    NavManeuverType maneuver;
} NavWaypointType;

typedef struct {
    uint32_t guidanceId;
    NavStatusType status;
    
    /* Current position */
    GpsPositionType currentPosition;
    
    /* Destination */
    GpsPositionType destination;
    char destinationName[NAV_MAX_STREET_NAME_LEN];
    
    /* Route information */
    uint32_t totalDistanceM;
    uint32_t totalTimeSec;
    uint32_t remainingDistanceM;
    uint32_t remainingTimeSec;
    uint32_t arrivalTimeEpoch;
    
    /* Current instruction */
    NavManeuverType nextManeuver;
    uint32_t distanceToManeuverM;
    char nextStreetName[NAV_MAX_STREET_NAME_LEN];
    
    /* Lane guidance */
    uint8_t currentLane;
    uint8_t recommendedLane;
    bool laneGuidanceActive;
    
    /* Traffic */
    TrafficConditionType trafficCondition;
    uint32_t delaySec;
    bool trafficOnRoute;
    bool alternateRouteAvailable;
    
    /* Waypoints */
    uint32_t waypointCount;
    NavWaypointType waypoints[NAV_MAX_WAYPOINTS];
    
    /* Map data */
    uint8_t currentRoadSpeedLimit;
    bool speedLimitWarning;
    bool approachingSchoolZone;
    
    uint64_t timestampUs;
} NavigationStatusType;

/* ============================================================================
 * HUD Types
 * ============================================================================ */

typedef enum {
    HUD_ELEMENT_SPEED = 0,
    HUD_ELEMENT_NAVIGATION,
    HUD_ELEMENT_ADAS,
    HUD_ELEMENT_LANE_GUIDANCE,
    HUD_ELEMENT_WARNING,
    HUD_ELEMENT_PHONE,
    HUD_ELEMENT_MEDIA
} HudElementType;

typedef enum {
    HUD_PRIORITY_LOW = 0,
    HUD_PRIORITY_MEDIUM,
    HUD_PRIORITY_HIGH,
    HUD_PRIORITY_CRITICAL
} HudPriorityType;

typedef struct {
    uint32_t elementId;
    HudElementType type;
    HudPriorityType priority;
    
    /* Position (normalized 0.0 - 1.0) */
    float32 posX;
    float32 posY;
    
    /* Content */
    char text[HUD_MAX_TEXT_LEN];
    uint32_t numericValue;
    float32 floatValue;
    
    /* Visual */
    uint32_t color;
    uint8_t fontSize;
    bool blink;
    uint16_t blinkRateMs;
    bool visible;
    
    /* Icon */
    uint32_t iconId;
    uint8_t iconSize;
    
    uint64_t timestampUs;
} HudElementTypeDef;

typedef struct {
    uint32_t hudId;
    bool enabled;
    uint8_t brightnessLevel;
    
    /* Display elements */
    uint8_t elementCount;
    HudElementTypeDef elements[16];
    
    /* Current speed display */
    uint32_t currentSpeedKph;
    uint32_t speedLimit;
    bool speedLimitExceeded;
    
    /* Navigation overlay */
    bool navActive;
    char navNextTurn[HUD_MAX_TEXT_LEN];
    uint32_t navDistanceToTurnM;
    NavManeuverType navManeuver;
    
    /* ADAS overlay */
    bool adasActive;
    bool laneDepartureWarning;
    bool collisionWarning;
    float32 followingDistanceS;
    
    uint64_t timestampUs;
} HudDisplayType;

/* ============================================================================
 * Media Types
 * ============================================================================ */

typedef enum {
    MEDIA_SOURCE_NONE = 0,
    MEDIA_SOURCE_RADIO_FM,
    MEDIA_SOURCE_RADIO_AM,
    MEDIA_SOURCE_RADIO_DAB,
    MEDIA_SOURCE_RADIO_SIRIUS,
    MEDIA_SOURCE_BLUETOOTH,
    MEDIA_SOURCE_USB,
    MEDIA_SOURCE_AUX,
    MEDIA_SOURCE_CD,
    MEDIA_SOURCE_STREAMING,
    MEDIA_SOURCE_CARPLAY,
    MEDIA_SOURCE_ANDROID_AUTO
} MediaSourceType;

typedef enum {
    MEDIA_STATE_STOPPED = 0,
    MEDIA_STATE_PLAYING,
    MEDIA_STATE_PAUSED,
    MEDIA_STATE_BUFFERING,
    MEDIA_STATE_SEEKING,
    MEDIA_STATE_ERROR
} MediaStateType;

typedef enum {
    REPEAT_MODE_OFF = 0,
    REPEAT_MODE_ONE,
    REPEAT_MODE_ALL
} RepeatModeType;

typedef struct {
    uint32_t mediaId;
    MediaSourceType source;
    MediaStateType state;
    
    /* Track info */
    char title[MEDIA_MAX_TRACK_TITLE_LEN];
    char artist[MEDIA_MAX_ARTIST_LEN];
    char album[MEDIA_MAX_ALBUM_LEN];
    uint32_t trackNumber;
    uint32_t totalTracks;
    uint32_t durationSec;
    
    /* Playback */
    uint32_t currentPositionSec;
    uint8_t volumePercent;
    bool muted;
    uint8_t balance;      /* 0-100 (left-right) */
    uint8_t fade;         /* 0-100 (rear-front) */
    
    /* Audio settings */
    int8_t bassLevel;     /* -10 to +10 */
    int8_t trebleLevel;   /* -10 to +10 */
    int8_t midLevel;      /* -10 to +10 */
    
    /* Playback modes */
    bool shuffleEnabled;
    RepeatModeType repeatMode;
    
    /* Radio specific */
    float32 radioFrequency;  /* MHz for FM, kHz for AM */
    uint32_t radioStationId;
    char radioStationName[MEDIA_MAX_ARTIST_LEN];
    uint8_t signalStrength;  /* 0-100 */
    bool stereo;
    
    /* Album art */
    bool albumArtAvailable;
    uint32_t albumArtSize;
    
    uint64_t timestampUs;
} MediaStatusType;

/* ============================================================================
 * System Status Types
 * ============================================================================ */

typedef struct {
    uint32_t systemId;
    
    /* Display */
    bool displayOn;
    uint8_t displayBrightness;
    uint32_t screenResolutionX;
    uint32_t screenResolutionY;
    
    /* Touch */
    bool touchEnabled;
    bool touchActive;
    uint32_t lastTouchX;
    uint32_t lastTouchY;
    
    /* Voice */
    bool voiceActive;
    bool voiceListening;
    float32 voiceConfidence;
    
    /* Phone */
    bool phoneConnected;
    char phoneName[MEDIA_MAX_ARTIST_LEN];
    bool callActive;
    char callerName[MEDIA_MAX_ARTIST_LEN];
    uint32_t callDurationSec;
    
    /* Settings */
    uint8_t language;
    uint8_t units;  /* 0=metric, 1=imperial */
    bool demoMode;
    
    uint64_t timestampUs;
} InfotainmentSystemType;

/* ============================================================================
 * E2E Configuration (Non-safety, QM level)
 * ============================================================================ */

#define INFO_E2E_PROFILE            E2E_PROFILE_01  /* CRC8 for non-critical data */
#define INFO_E2E_DATAID_NAV         0x3001
#define INFO_E2E_DATAID_HUD         0x3002
#define INFO_E2E_DATAID_MEDIA       0x3003
#define INFO_E2E_DATAID_SYSTEM      0x3004

#ifdef __cplusplus
}
#endif

#endif /* INFOTAINMENT_TYPES_H */
