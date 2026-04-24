/******************************************************************************
 * @file    adas_types.h
 * @brief   ADAS Perception System - Data Types Definition
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level for critical data
 *
 * Topics:
 *   - LiDAR Point Cloud
 *   - Camera Images
 *   - Sensor Fusion
 *   - Object Detection
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ADAS_TYPES_H
#define ADAS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../src/common/autosar_types.h"

/* ============================================================================
 * Version Information
 * ============================================================================ */
#define ADAS_MAJOR_VERSION      1
#define ADAS_MINOR_VERSION      0
#define ADAS_PATCH_VERSION      0

/* ============================================================================
 * Constants
 * ============================================================================ */
#define ADAS_MAX_LIDAR_POINTS           10000
#define ADAS_MAX_CAMERA_WIDTH           1920
#define ADAS_MAX_CAMERA_HEIGHT          1080
#define ADAS_MAX_DETECTED_OBJECTS       256
#define ADAS_MAX_SENSOR_COUNT           16
#define ADAS_MAX_LANE_MARKINGS          8
#define ADAS_TIMESTAMP_RESOLUTION_US    1

/* Coordinate system: Vehicle-centric ISO 8855
 * X: Forward
 * Y: Left
 * Z: Up
 */

/* ============================================================================
 * Common Types
 * ============================================================================ */

typedef enum {
    SENSOR_TYPE_LIDAR = 0,
    SENSOR_TYPE_CAMERA,
    SENSOR_TYPE_RADAR,
    SENSOR_TYPE_ULTRASONIC,
    SENSOR_TYPE_IMU,
    SENSOR_TYPE_GPS
} AdasSensorTypeType;

typedef enum {
    SENSOR_STATUS_OK = 0,
    SENSOR_STATUS_DEGRADED,
    SENSOR_STATUS_FAULT,
    SENSOR_STATUS_NOT_CALIBRATED,
    SENSOR_STATUS_OFFLINE
} AdasSensorStatusType;

typedef enum {
    OBJ_CLASS_UNKNOWN = 0,
    OBJ_CLASS_PEDESTRIAN,
    OBJ_CLASS_VEHICLE,
    OBJ_CLASS_CYCLIST,
    OBJ_CLASS_MOTORCYCLE,
    OBJ_CLASS_TRUCK,
    OBJ_CLASS_ANIMAL,
    OBJ_CLASS_TRAFFIC_SIGN,
    OBJ_CLASS_TRAFFIC_LIGHT,
    OBJ_CLASS_OBSTACLE
} AdasObjectClassType;

typedef enum {
    LANE_TYPE_UNKNOWN = 0,
    LANE_TYPE_SOLID,
    LANE_TYPE_DASHED,
    LANE_TYPE_DOUBLE_SOLID,
    LANE_TYPE_DOUBLE_DASHED,
    LANE_TYPE_BOTTS_DOTS,
    LANE_TYPE_ROAD_EDGE
} AdasLaneTypeType;

typedef struct {
    float32 x;      /* meters */
    float32 y;      /* meters */
    float32 z;      /* meters */
} AdasPoint3DType;

typedef struct {
    float32 x;      /* meters */
    float32 y;      /* meters */
    float32 z;      /* meters */
    float32 roll;   /* radians */
    float32 pitch;  /* radians */
    float32 yaw;    /* radians */
} AdasPoseType;

typedef struct {
    float32 vx;     /* m/s */
    float32 vy;     /* m/s */
    float32 vz;     /* m/s */
} AdasVelocityType;

/* ============================================================================
 * LiDAR Data Types
 * ============================================================================ */

typedef struct {
    float32 x;          /* meters */
    float32 y;          /* meters */
    float32 z;          /* meters */
    float32 intensity;  /* 0.0 - 1.0 */
    uint8_t ring;       /* LiDAR ring/channel ID */
    uint32_t timestamp; /* microseconds */
} AdasLidarPointType;

typedef struct {
    uint32_t sensorId;
    uint32_t frameId;
    uint64_t timestampUs;
    uint32_t pointCount;
    AdasSensorStatusType status;
    AdasPoseType sensorPose;
    AdasLidarPointType points[ADAS_MAX_LIDAR_POINTS];
} AdasLidarPointCloudType;

/* ============================================================================
 * Camera Data Types
 * ============================================================================ */

typedef enum {
    CAM_FORMAT_GRAY8 = 0,
    CAM_FORMAT_RGB888,
    CAM_FORMAT_YUV422,
    CAM_FORMAT_RAW10,
    CAM_FORMAT_RAW12,
    CAM_FORMAT_COMPRESSED_JPEG,
    CAM_FORMAT_COMPRESSED_H264
} AdasCameraFormatType;

typedef struct {
    uint32_t sensorId;
    uint32_t frameId;
    uint64_t timestampUs;
    uint16_t width;
    uint16_t height;
    AdasCameraFormatType format;
    AdasSensorStatusType status;
    float32 exposureTimeMs;
    float32 gainDb;
    uint32_t imageDataSize;
    uint8_t imageData[ADAS_MAX_CAMERA_WIDTH * ADAS_MAX_CAMERA_HEIGHT * 3];
} AdasCameraImageType;

/* ============================================================================
 * Object Detection Types
 * ============================================================================ */

typedef struct {
    uint32_t objectId;
    AdasObjectClassType objectClass;
    float32 confidence;         /* 0.0 - 1.0 */
    
    /* Bounding box in 3D */
    AdasPoint3DType center;
    float32 length;             /* meters */
    float32 width;              /* meters */
    float32 height;             /* meters */
    float32 heading;            /* radians */
    
    /* Motion state */
    AdasVelocityType velocity;
    float32 acceleration;       /* m/s^2 */
    
    /* Prediction */
    float32 predictedPath[10][2];   /* 10 future points, x,y in meters */
    
    /* Metadata */
    uint64_t timestampUs;
    uint32_t age;               /* frames */
    bool tracked;
} AdasDetectedObjectType;

typedef struct {
    uint32_t frameId;
    uint64_t timestampUs;
    uint32_t objectCount;
    uint32_t sensorId;
    AdasDetectedObjectType objects[ADAS_MAX_DETECTED_OBJECTS];
} AdasObjectListType;

/* ============================================================================
 * Lane Detection Types
 * ============================================================================ */

typedef struct {
    uint8_t laneId;
    AdasLaneTypeType laneType;
    float32 confidence;
    
    /* Lane marking as polynomial: y = c0 + c1*x + c2*x^2 + c3*x^3 */
    float32 c0;     /* lateral offset at ego position */
    float32 c1;     /* heading angle difference */
    float32 c2;     /* curvature */
    float32 c3;     /* curvature change rate */
    
    float32 validRangeMin;      /* meters */
    float32 validRangeMax;      /* meters */
    bool isValid;
} AdasLaneMarkingType;

typedef struct {
    uint32_t frameId;
    uint64_t timestampUs;
    uint8_t laneCount;
    uint8_t egoLaneIndex;
    AdasLaneMarkingType lanes[ADAS_MAX_LANE_MARKINGS];
    float32 laneWidth;          /* meters */
} AdasLaneDataType;

/* ============================================================================
 * Sensor Fusion Types
 * ============================================================================ */

typedef struct {
    uint32_t sensorId;
    AdasSensorTypeType type;
    float32 weight;             /* fusion weight 0.0 - 1.0 */
    AdasSensorStatusType status;
    uint64_t lastUpdateUs;
} AdasFusionSourceType;

typedef struct {
    uint32_t fusionId;
    uint64_t timestampUs;
    
    /* Ego vehicle state */
    AdasPoseType egoPose;
    AdasVelocityType egoVelocity;
    float32 egoAcceleration;
    
    /* Fused objects */
    uint32_t objectCount;
    AdasDetectedObjectType fusedObjects[ADAS_MAX_DETECTED_OBJECTS];
    
    /* Lane information */
    AdasLaneDataType laneData;
    
    /* Source sensors */
    uint8_t sourceCount;
    AdasFusionSourceType sources[ADAS_MAX_SENSOR_COUNT];
    
    /* Fusion quality */
    float32 fusionQuality;      /* 0.0 - 1.0 */
} AdasFusionResultType;

/* ============================================================================
 * E2E Protection Configuration
 * ============================================================================ */

#define ADAS_E2E_PROFILE        E2E_PROFILE_07
#define ADAS_E2E_DATAID_LIDAR   0x0001
#define ADAS_E2E_DATAID_CAMERA  0x0002
#define ADAS_E2E_DATAID_FUSION  0x0003
#define ADAS_E2E_DATAID_OBJECTS 0x0004

#ifdef __cplusplus
}
#endif

#endif /* ADAS_TYPES_H */
