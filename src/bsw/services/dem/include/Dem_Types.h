/**
 * @file Dem_Types.h
 * @brief Diagnostic Event Manager - Type Definitions
 * @version 1.1.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Diagnostic Event Manager (DEM) Types
 * Layer: Service Layer
 * 
 * CRITICAL FIX: Separated type definitions from Dem.h for better modularity
 * and AUTOSAR compliance (SWS_Dem_00951, SWS_Dem_00952)
 */

#ifndef DEM_TYPES_H
#define DEM_TYPES_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DEM_TYPES_VENDOR_ID                   (0x01U)
#define DEM_TYPES_MODULE_ID                   (0x54U)
#define DEM_TYPES_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define DEM_TYPES_AR_RELEASE_MINOR_VERSION    (0x04U)
#define DEM_TYPES_AR_RELEASE_REVISION_VERSION (0x00U)
#define DEM_TYPES_SW_MAJOR_VERSION            (0x01U)
#define DEM_TYPES_SW_MINOR_VERSION            (0x01U)
#define DEM_TYPES_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    DEM EVENT STATUS TYPE
==================================================================================================*/
/**
 * @brief Event status reported by monitors
 * SWS_Dem_00951
 */
typedef enum {
    DEM_EVENT_STATUS_PASSED = 0,
    DEM_EVENT_STATUS_FAILED,
    DEM_EVENT_STATUS_PREPASSED,
    DEM_EVENT_STATUS_PREFAILED,
    DEM_EVENT_STATUS_FDC_THRESHOLD_REACHED
} Dem_EventStatusType;

/*==================================================================================================
*                                    DEM UDS STATUS BYTE TYPE
==================================================================================================*/
/**
 * @brief UDS DTC status byte type (ISO 14229-1)
 */
typedef uint8 Dem_UdsStatusByteType;

/* UDS Status Byte Bits */
#define DEM_UDS_STATUS_TF               (0x01U)  /* Test Failed */
#define DEM_UDS_STATUS_TFTOC            (0x02U)  /* Test Failed This Operation Cycle */
#define DEM_UDS_STATUS_PDTC             (0x04U)  /* Pending DTC */
#define DEM_UDS_STATUS_CDTC             (0x08U)  /* Confirmed DTC */
#define DEM_UDS_STATUS_TNCSLC           (0x10U)  /* Test Not Completed Since Last Clear */
#define DEM_UDS_STATUS_TFSLC            (0x20U)  /* Test Failed Since Last Clear */
#define DEM_UDS_STATUS_TNCTOC           (0x40U)  /* Test Not Completed This Operation Cycle */
#define DEM_UDS_STATUS_WIR              (0x80U)  /* Warning Indicator Requested */

/* Legacy aliases for backward compatibility */
#define DEM_DTC_STATUS_TEST_FAILED                          DEM_UDS_STATUS_TF
#define DEM_DTC_STATUS_TEST_FAILED_THIS_OPERATION_CYCLE     DEM_UDS_STATUS_TFTOC
#define DEM_DTC_STATUS_PENDING_DTC                          DEM_UDS_STATUS_PDTC
#define DEM_DTC_STATUS_CONFIRMED_DTC                        DEM_UDS_STATUS_CDTC
#define DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR  DEM_UDS_STATUS_TNCSLC
#define DEM_DTC_STATUS_TEST_FAILED_SINCE_LAST_CLEAR         DEM_UDS_STATUS_TFSLC
#define DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE DEM_UDS_STATUS_TNCTOC
#define DEM_DTC_STATUS_WARNING_INDICATOR_REQUESTED          DEM_UDS_STATUS_WIR

/*==================================================================================================
*                                    DEM DTC ORIGIN TYPE
==================================================================================================*/
/**
 * @brief DTC memory origin type
 * SWS_Dem_00952
 */
typedef enum {
    DEM_DTC_ORIGIN_PRIMARY_MEMORY = 0x01,
    DEM_DTC_ORIGIN_MIRROR_MEMORY = 0x02,
    DEM_DTC_ORIGIN_PERMANENT_MEMORY = 0x04,
    DEM_DTC_ORIGIN_OBD_RELEVANT_MEMORY = 0x08
} Dem_DTCOriginType;

/*==================================================================================================
*                                    DEM DTC FORMAT TYPE
==================================================================================================*/
typedef enum {
    DEM_DTC_FORMAT_OBD = 0,
    DEM_DTC_FORMAT_UDS,
    DEM_DTC_FORMAT_J1939
} Dem_DTCFormatType;

/*==================================================================================================
*                                    DEM DTC SEVERITY TYPE
==================================================================================================*/
typedef uint8 Dem_DTCSeverityType;

/* Severity bits */
#define DEM_SEVERITY_NO_SEVERITY        (0x00U)
#define DEM_SEVERITY_MAINTENANCE_ONLY   (0x01U)
#define DEM_SEVERITY_CHECK_AT_NEXT_HALT (0x02U)
#define DEM_SEVERITY_CHECK_IMMEDIATELY  (0x04U)

/*==================================================================================================
*                                    DEM OPERATION CYCLE TYPE
==================================================================================================*/
typedef enum {
    DEM_OPCYC_POWER = 0,
    DEM_OPCYC_IGNITION,
    DEM_OPCYC_WARMUP,
    DEM_OPCYC_OBD_DCY,
    DEM_OPCYC_OTHER
} Dem_OperationCycleType;

/*==================================================================================================
*                                    DEM OPERATION CYCLE STATE TYPE
==================================================================================================*/
typedef enum {
    DEM_CYCLE_STATE_START = 0,
    DEM_CYCLE_STATE_END
} Dem_OperationCycleStateType;

/*==================================================================================================
*                                    DEM INDICATOR STATUS TYPE
==================================================================================================*/
typedef enum {
    DEM_INDICATOR_OFF = 0,
    DEM_INDICATOR_CONTINUOUS,
    DEM_INDICATOR_BLINKING,
    DEM_INDICATOR_BLINKING_CONT,
    DEM_INDICATOR_SLOW_BLINK,
    DEM_INDICATOR_FAST_BLINK,
    DEM_INDICATOR_ON_DEMAND,
    DEM_INDICATOR_SHORT
} Dem_IndicatorStatusType;

/*==================================================================================================
*                                    DEM EVENT ID TYPE
==================================================================================================*/
typedef uint16 Dem_EventIdType;

/*==================================================================================================
*                                    DEM DTC TYPE
==================================================================================================*/
typedef uint32 Dem_DtcType;
typedef Dem_DtcType Dem_DTCType;  /* Alias for compatibility */

/*==================================================================================================
*                                    DEM DTC STATUS TYPE
==================================================================================================*/
typedef uint8 Dem_DTCStatusType;

/*==================================================================================================
*                                    DEM FAULT DETECTION COUNTER TYPE
==================================================================================================*/
typedef sint8 Dem_FaultDetectionCounterType;

/*==================================================================================================
*                                    DEM EVENT STATE TYPE (Internal)
==================================================================================================*/
/**
 * @brief Internal event state structure
 */
typedef struct {
    Dem_EventStatusType LastReportedStatus;
    uint8 DTCStatus;
    Dem_FaultDetectionCounterType FaultDetectionCounter;
    sint16 DebounceCounter;
    boolean TestFailedThisOperationCycle;
    boolean TestCompletedThisOperationCycle;
    uint8 OccurrenceCounter;
    uint8 AgingCounter;
    boolean IsAged;
    /* CRITICAL FIX: Added timestamp for time-based debounce */
    uint32 LastReportTimestamp;
    uint32 TimeInCurrentStatus;
} Dem_EventStateType;

/*==================================================================================================
*                                    DEM DEBOUNCE ALGORITHM TYPE
==================================================================================================*/
typedef enum {
    DEM_DEBOUNCE_ALGORITHM_NONE = 0,
    DEM_DEBOUNCE_ALGORITHM_COUNTER,
    DEM_DEBOUNCE_ALGORITHM_TIME,
    DEM_DEBOUNCE_ALGORITHM_MONITOR
} Dem_DebounceAlgorithmType;

/*==================================================================================================
*                                    DEM EVENT PARAMETER TYPE
==================================================================================================*/
typedef struct {
    Dem_EventIdType EventId;
    Dem_DtcType Dtc;
    uint8 EventPriority;
    boolean EventAvailable;
    boolean EventReporting;
    uint8 EventFailureCycleCounterThreshold;
    uint8 EventConfirmationThreshold;
    Dem_DebounceAlgorithmType DebounceAlgorithm;
    boolean EventCounterBased;
    boolean EventTimeBased;
    boolean EventMonitorInternal;
    /* CRITICAL FIX: Added debounce threshold configuration */
    sint16 DebounceCounterFailedThreshold;
    sint16 DebounceCounterPassedThreshold;
    uint16 DebounceTimeFailedThresholdMs;
    uint16 DebounceTimePassedThresholdMs;
} Dem_EventParameterType;

/*==================================================================================================
*                                    DEM DTC PARAMETER TYPE
==================================================================================================*/
typedef struct {
    Dem_DtcType Dtc;
    Dem_DTCSeverityType DtcSeverity;
    uint8 DtcFunctionalUnit;
    Dem_DTCOriginType DtcOrigin;
    boolean DtcAvailable;
    boolean DtcReporting;
    uint8 AgingThreshold;
    boolean MemoryEntryOverflow;
} Dem_DtcParameterType;

/*==================================================================================================
*                                    DEM FREEZE FRAME RECORD TYPE
==================================================================================================*/
typedef struct {
    uint8 RecordNumber;
    uint8 NumDids;
    const uint16* DidIds;
    boolean RecordUpdateEnabled;
} Dem_FreezeFrameRecordType;

/*==================================================================================================
*                                    DEM EXTENDED DATA RECORD TYPE
==================================================================================================*/
/**
 * @brief Extended data record configuration
 * CRITICAL FIX: Added for extended data support
 */
typedef struct {
    uint8 RecordNumber;
    uint16 DataSize;
    boolean RecordUpdateEnabled;
    boolean IsOverflowAllowed;
} Dem_ExtendedDataRecordType;

/*==================================================================================================
*                                    DEM INDICATOR TYPE
==================================================================================================*/
typedef struct {
    uint8 IndicatorId;
    uint8 IndicatorBehavior;
    uint8 IndicatorFailureCycleThreshold;
} Dem_IndicatorType;

/*==================================================================================================
*                                    DEM ENABLE CONDITION TYPE
==================================================================================================*/
typedef struct {
    uint8 ConditionId;
    boolean DefaultStatus;
} Dem_EnableConditionType;

/*==================================================================================================
*                                    DEM STORAGE CONDITION TYPE
==================================================================================================*/
typedef struct {
    uint8 ConditionId;
    boolean DefaultStatus;
} Dem_StorageConditionType;

/*==================================================================================================
*                                    DEM CLEAR DTC NOTIFICATION TYPE
==================================================================================================*/
typedef void (*Dem_ClearDTCLambdaNotificationType)(void);
typedef void (*Dem_ClearDTCStartNotificationType)(void);
typedef void (*Dem_ClearDTCFinishNotificationType)(void);

/*==================================================================================================
*                                    DEM CALLBACK FUNCTION TYPES
==================================================================================================*/
typedef void (*Dem_EventStatusChangedCallbackType)(Dem_EventIdType EventId, 
                                                     Dem_EventStatusType newStatus,
                                                     Dem_EventStatusType oldStatus);

typedef Std_ReturnType (*Dem_FreezeFrameStorageConditionType)(void);

typedef Std_ReturnType (*Dem_ExtendedDataStorageConditionType)(void);

/*==================================================================================================
*                                    DEM CONFIG TYPE
==================================================================================================*/
typedef struct {
    const Dem_EventParameterType* EventParameters;
    uint16 NumEvents;
    const Dem_DtcParameterType* DtcParameters;
    uint16 NumDtcs;
    const Dem_FreezeFrameRecordType* FreezeFrameRecords;
    uint8 NumFreezeFrameRecords;
    const Dem_ExtendedDataRecordType* ExtendedDataRecords;
    uint8 NumExtendedDataRecords;
    const Dem_IndicatorType* Indicators;
    uint8 NumIndicators;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean ClearDtcSupported;
    boolean ClearDtcLimitation;
    uint8 DtcStatusAvailabilityMask;
    boolean OBDRelevantSupport;
    boolean J1939Support;
    boolean TriggerFimReports;
    boolean TriggerMonitorInitBeforeClearOk;
    Dem_ClearDTCLambdaNotificationType ClearDTCLambdaNotification;
    Dem_ClearDTCStartNotificationType ClearDTCStartNotification;
    Dem_ClearDTCFinishNotificationType ClearDTCFinishNotification;
} Dem_ConfigType;

/*==================================================================================================
*                                    DEM RETURN TYPES
==================================================================================================*/
/* Extended return types for specific APIs */
typedef uint8 Dem_ReturnGetStatusOfDTCType;
#define DEM_STATUS_OK                   (0x00U)
#define DEM_STATUS_WRONG_DTC            (0x01U)
#define DEM_STATUS_WRONG_DTCORIGIN      (0x02U)
#define DEM_STATUS_FAILED               (0x03U)
#define DEM_STATUS_PENDING              (0x04U)

typedef uint8 Dem_ReturnClearDTCType;
#define DEM_CLEAR_OK                    (0x00U)
#define DEM_CLEAR_WRONG_DTC             (0x01U)
#define DEM_CLEAR_WRONG_DTCORIGIN       (0x02U)
#define DEM_CLEAR_FAILED                (0x03U)
#define DEM_CLEAR_PENDING               (0x04U)
#define DEM_CLEAR_BUSY                  (0x05U)
#define DEM_CLEAR_MEMORY_ERROR          (0x06U)

typedef uint8 Dem_ReturnDisableDTCRecordUpdateType;
#define DEM_DISABLEDTCRECUP_OK          (0x00U)
#define DEM_DISABLEDTCRECUP_WRONG_DTC   (0x01U)
#define DEM_DISABLEDTCRECUP_WRONG_DTCORIGIN (0x02U)
#define DEM_DISABLEDTCRECUP_DISABLED    (0x03U)

typedef uint8 Dem_ReturnGetFreezeFrameDataByDTCType;
#define DEM_GET_FREEZEFRAME_OK          (0x00U)
#define DEM_GET_FREEZEFRAME_WRONG_DTC   (0x01U)
#define DEM_GET_FREEZEFRAME_WRONG_DTCORIGIN (0x02U)
#define DEM_GET_FREEZEFRAME_WRONG_RECORDNUMBER (0x03U)
#define DEM_GET_FREEZEFRAME_WRONG_BUFFERSIZE (0x04U)
#define DEM_GET_FREEZEFRAME_PENDING     (0x05U)

typedef uint8 Dem_ReturnGetExtendedDataRecordByDTCType;
#define DEM_RECORD_OK                   (0x00U)
#define DEM_RECORD_WRONG_DTC            (0x01U)
#define DEM_RECORD_WRONG_DTCORIGIN      (0x02U)
#define DEM_RECORD_WRONG_NUMBER         (0x03U)
#define DEM_RECORD_WRONG_BUFFERSIZE     (0x04U)
#define DEM_RECORD_PENDING              (0x05U)

typedef uint8 Dem_ReturnGetNumberOfFilteredDTCType;
#define DEM_NUMBER_OK                   (0x00U)
#define DEM_NUMBER_FAILED               (0x01U)
#define DEM_NUMBER_PENDING              (0x02U)

typedef uint8 Dem_ReturnGetNextFilteredDTCType;
#define DEM_FILTERED_OK                 (0x00U)
#define DEM_FILTERED_NO_MATCHING_ELEMENT (0x01U)
#define DEM_FILTERED_PENDING            (0x02U)
#define DEM_FILTERED_BUFFER_TOO_SMALL   (0x03U)

typedef uint8 Dem_ReturnGetDTCByOccurrenceTimeType;
#define DEM_OCCURR_OK                   (0x00U)
#define DEM_OCCURR_NOT_AVAILABLE        (0x01U)
#define DEM_OCCURR_PENDING              (0x02U)

typedef uint8 Dem_ReturnSetFilterType;
#define DEM_FILTER_ACCEPT               (0x00U)
#define DEM_FILTER_REJECT               (0x01U)

#endif /* DEM_TYPES_H */
