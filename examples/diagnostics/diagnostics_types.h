/******************************************************************************
 * @file    diagnostics_types.h
 * @brief   Vehicle Diagnostics System - Data Types Definition
 *
 * AUTOSAR R22-11 compliant
 * ASIL-B Safety Level for diagnostic data
 *
 * Topics:
 *   - DTC (Diagnostic Trouble Codes)
 *   - Snapshot Records
 *   - Extended Data
 *   - Routine Results
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DIAGNOSTICS_TYPES_H
#define DIAGNOSTICS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../src/common/autosar_types.h"

#define DIAG_MAJOR_VERSION          1
#define DIAG_MINOR_VERSION          0
#define DIAG_PATCH_VERSION          0

#define DIAG_MAX_DTCS               256
#define DIAG_MAX_SNAPSHOT_DATA      256
#define DIAG_MAX_EXTENDED_DATA      32
#define DIAG_MAX_RID_RESULTS        64
#define DIAG_MAX_FID_COUNT          32
#define DIAG_MAX_ENV_DATA_SIZE      512

/* ============================================================================
 * DTC Types
 * ============================================================================ */

typedef enum {
    DTC_SEVERITY_NO_SYMPTOM = 0,
    DTC_SEVERITY_LIGHT_NO_WARNING,
    DTC_SEVERITY_CHECK_AT_NEXT_HALT,
    DTC_SEVERITY_CHECK_IMMEDIATELY,
    DTC_SEVERITY_WARNING_L1,
    DTC_SEVERITY_WARNING_L2,
    DTC_SEVERITY_WARNING_L3
} DtcSeverityType;

typedef enum {
    DTC_STATUS_TEST_FAILED = 0x01,
    DTC_STATUS_TEST_FAILED_THIS_OPERATION_CYCLE = 0x02,
    DTC_STATUS_PENDING_DTC = 0x04,
    DTC_STATUS_CONFIRMED_DTC = 0x08,
    DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR = 0x10,
    DTC_STATUS_TEST_FAILED_SINCE_LAST_CLEAR = 0x20,
    DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE = 0x40,
    DTC_STATUS_WARNING_INDICATOR_REQUESTED = 0x80
} DtcStatusMaskType;

typedef enum {
    DTC_ORIGIN_PRIMARY_MEMORY = 0,
    DTC_ORIGIN_MIRROR_MEMORY,
    DTC_ORIGIN_PERMANENT_MEMORY,
    DTC_ORIGIN_USER_DEFINED
} DtcOriginType;

typedef struct {
    uint32_t dtcNumber;             /* SAE J2012-DA DTC format */
    DtcSeverityType severity;
    uint8_t statusByte;
    DtcOriginType origin;
    
    /* Occurrence info */
    uint8_t occurrenceCounter;
    uint32_t agingCounter;
    uint32_t faultDetectionCounter;
    
    /* Timestamps */
    uint64_t firstDetectionTime;
    uint64_t lastDetectionTime;
    uint64_t agingTime;
    
    /* Snapshot availability */
    bool snapshotAvailable;
    uint8_t snapshotRecordNumber;
    
    /* Extended data availability */
    bool extendedDataAvailable;
    uint8_t extendedDataRecordNumber;
} DtcType;

typedef struct {
    uint32_t ecuId;
    uint8_t dtcCount;
    uint8_t confirmedDtcCount;
    uint8_t pendingDtcCount;
    DtcType dtcs[DIAG_MAX_DTCS];
    uint64_t lastClearTime;
    uint32_t operationCycleCount;
    uint64_t timestampUs;
} DtcStatusType;

/* ============================================================================
 * Snapshot Record Types
 * ============================================================================ */

typedef enum {
    SNAPSHOT_RECORD_ENV = 0x01,
    SNAPSHOT_RECORD_SLOW,
    SNAPSHOT_RECORD_FAST,
    SNAPSHOT_RECORD_LOCK,
    SNAPSHOT_RECORD_USER_DEFINED
} SnapshotRecordType;

typedef struct {
    uint16_t dataId;
    uint8_t dataSize;
    uint8_t data[32];
} SnapshotDataElementType;

typedef struct {
    uint32_t dtcNumber;
    uint8_t recordNumber;
    SnapshotRecordType recordType;
    uint64_t snapshotTime;
    
    /* Vehicle state at fault */
    uint32_t odometerKm;
    float32 vehicleSpeedKph;
    float32 batteryVoltage;
    float32 engineTempC;
    uint8_t gearPosition;
    uint16_t engineRpm;
    
    /* Environment data */
    uint16_t envDataSize;
    uint8_t envData[DIAG_MAX_ENV_DATA_SIZE];
    
    /* Additional data elements */
    uint8_t elementCount;
    SnapshotDataElementType elements[16];
    
    uint64_t timestampUs;
} SnapshotRecordTypeDef;

/* ============================================================================
 * Extended Data Types
 * ============================================================================ */

typedef enum {
    EXTENDED_DATA_OCCURRENCE_COUNTER = 0x01,
    EXTENDED_DATA_AGING_COUNTER = 0x02,
    EXTENDED_DATA_FAULT_DETECTION_COUNTER = 0x03,
    EXTENDED_DATA_SPECIFIC = 0xFE
} ExtendedDataType;

typedef struct {
    uint32_t dtcNumber;
    uint8_t recordNumber;
    ExtendedDataType dataType;
    uint8_t dataSize;
    uint8_t data[DIAG_MAX_EXTENDED_DATA];
    uint64_t timestampUs;
} ExtendedDataRecordType;

/* ============================================================================
 * Routine Control Types
 * ============================================================================ */

typedef enum {
    ROUTINE_STATUS_NOT_STARTED = 0,
    ROUTINE_STATUS_IN_PROGRESS,
    ROUTINE_STATUS_COMPLETED,
    ROUTINE_STATUS_ABORTED,
    ROUTINE_STATUS_FAILED
} RoutineStatusType;

typedef struct {
    uint16_t rid;                   /* Routine Identifier */
    char name[64];
    RoutineStatusType status;
    uint8_t progressPercent;
    
    /* Results */
    uint16_t resultDataSize;
    uint8_t resultData[128];
    
    /* Timing */
    uint32_t estimatedDurationMs;
    uint32_t elapsedTimeMs;
    uint32_t remainingTimeMs;
    
    /* Security */
    uint8_t securityLevelRequired;
    bool securityAccessGranted;
    
    uint64_t timestampUs;
} RoutineResultType;

/* ============================================================================
 * FID (Function Inhibition) Types
 * ============================================================================ */

typedef struct {
    uint16_t fidNumber;
    char description[64];
    bool inhibited;
    uint32_t inhibitingDtc;
    uint8_t inhibitionReason;
    uint64_t inhibitionTime;
    uint64_t timestampUs;
} FunctionInhibitionType;

typedef struct {
    uint32_t ecuId;
    uint8_t fidCount;
    FunctionInhibitionType fids[DIAG_MAX_FID_COUNT];
    uint64_t timestampUs;
} FidStatusType;

/* ============================================================================
 * Diagnostic Session Types
 * ============================================================================ */

typedef enum {
    DIAG_SESSION_DEFAULT = 0x01,
    DIAG_SESSION_PROGRAMMING = 0x02,
    DIAG_SESSION_EXTENDED = 0x03,
    DIAG_SESSION_SAFETY_SYSTEM = 0x04
} DiagnosticSessionType;

typedef struct {
    uint32_t ecuId;
    DiagnosticSessionType currentSession;
    uint32_t sessionTimeoutMs;
    uint32_t remainingTimeMs;
    uint8_t securityLevel;
    bool securityLocked;
    uint8_t seed[16];
    uint8_t key[16];
    uint64_t timestampUs;
} DiagnosticSessionStatusType;

/* ============================================================================
 * Combined Diagnostics Status
 * ============================================================================ */

typedef struct {
    uint32_t ecuId;
    char ecuName[32];
    
    /* DTC Status */
    DtcStatusType dtcStatus;
    
    /* Session Status */
    DiagnosticSessionStatusType sessionStatus;
    
    /* FID Status */
    FidStatusType fidStatus;
    
    /* Active Routines */
    uint8_t activeRoutineCount;
    RoutineResultType activeRoutines[8];
    
    /* Communication Status */
    bool communicationEnabled;
    bool dtcSettingEnabled;
    bool controlDtcSettingEnabled;
    
    /* System Health */
    uint8_t overallHealthPercent;
    uint8_t criticalDtcCount;
    uint8_t warningDtcCount;
    uint8_t infoDtcCount;
    
    uint64_t timestampUs;
} DiagnosticsSystemStatusType;

/* ============================================================================
 * E2E Protection Configuration
 * ============================================================================ */

#define DIAG_E2E_PROFILE            E2E_PROFILE_06  /* CRC16 + Counter for diagnostics */
#define DIAG_E2E_DATAID_DTC         0x4001
#define DIAG_E2E_DATAID_SNAPSHOT    0x4002
#define DIAG_E2E_DATAID_EXTENDED    0x4003
#define DIAG_E2E_DATAID_ROUTINE     0x4004
#define DIAG_E2E_DATAID_FID         0x4005
#define DIAG_E2E_DATAID_SESSION     0x4006
#define DIAG_E2E_DATAID_SYSTEM      0x4007

#ifdef __cplusplus
}
#endif

#endif /* DIAGNOSTICS_TYPES_H */
