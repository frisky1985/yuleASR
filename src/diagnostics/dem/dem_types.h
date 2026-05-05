/**
 * @file dem_types.h
 * @brief DEM (Diagnostic Event Manager) Type Definitions
 * @version 1.0
 * @note AUTOSAR R22-11 compliant, MISRA C:2012
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#ifndef DEM_TYPES_H
#define DEM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"

/*============================================================================*
 * Module Version Information
 *============================================================================*/
#define DEM_MODULE_ID                   0x0DU
#define DEM_VENDOR_ID                   0x00U
#define DEM_INSTANCE_ID                 0x00U

#define DEM_AR_RELEASE_MAJOR_VERSION    4U
#define DEM_AR_RELEASE_MINOR_VERSION    4U
#define DEM_AR_RELEASE_REVISION_VERSION 0U

#define DEM_SW_MAJOR_VERSION            1U
#define DEM_SW_MINOR_VERSION            0U
#define DEM_SW_PATCH_VERSION            0U

/*============================================================================*
 * Service IDs for Error Reporting
 *============================================================================*/
#define DEM_SID_INIT                            0x01U
#define DEM_SID_SHUTDOWN                        0x02U
#define DEM_SID_SETEVENTSTATUS                  0x03U
#define DEM_SID_GETEVENTSTATUS                  0x04U
#define DEM_SID_RESETEVENTSTATUS                0x05U
#define DEM_SID_GETDTCOFEVENT                   0x06U
#define DEM_SID_SETDTCFILTER                    0x07U
#define DEM_SID_GETSTATUSOFDTC                  0x08U
#define DEM_SID_GETDTCSTATUSAVAILABILITYMASK    0x09U
#define DEM_SID_GETNUMBEROFFILTEREDDTC           0x0AU
#define DEM_SID_GETNEXTFILTEREDDTC               0x0BU
#define DEM_SID_GETDTCSEVERITYAVAILABILITYMASK  0x0CU
#define DEM_SID_GETNEXTFILTEREDDTCANDSEVERITY    0x0DU
#define DEM_SID_DISABLEDTCRECORDING              0x0EU
#define DEM_SID_ENABLEDTCRECORDING               0x0FU
#define DEM_SID_GETFREEZEFRAMEDATABYDTC          0x10U
#define DEM_SID_GETSIZEOFFREEZEFRAMESELECTION    0x11U
#define DEM_SID_GETEXTENDEDDATARECORDBYDTC       0x12U
#define DEM_SID_GETSIZEOFEXTENDEDDATARECORD      0x13U
#define DEM_SID_CLEARDTC                         0x14U
#define DEM_SID_GETOBDFREEZEFRAMEDATA            0x15U
#define DEM_SID_DISABLEDTCSUPPRESSION            0x16U
#define DEM_SID_ENABLEDTCSUPPRESSION             0x17U

/*============================================================================*
 * Error Codes
 *============================================================================*/
#define DEM_E_NO_ERROR                          0x00U
#define DEM_E_NOT_INITIALIZED                   0x01U
#define DEM_E_PARAM_CONFIG                      0x10U
#define DEM_E_PARAM_POINTER                     0x11U
#define DEM_E_PARAM_DATA                        0x12U
#define DEM_E_PARAM_LENGTH                      0x13U
#define DEM_E_UNINIT                            0x20U
#define DEM_E_NODATAAVAILABLE                   0x21U
#define DEM_E_WRONG_CONFIGURATION               0x22U
#define DEM_E_WRONG_CONDITION                   0x23U
#define DEM_E_OUT_OF_BOUNDS                     0x24U

/*============================================================================*
 * Event Status Types
 *============================================================================*/
/**
 * @brief Event Status Type
 */
typedef uint8_t Dem_EventStatusType;

#define DEM_EVENT_STATUS_PASSED                 0x00U
#define DEM_EVENT_STATUS_FAILED                 0x01U
#define DEM_EVENT_STATUS_PREPASSED              0x02U
#define DEM_EVENT_STATUS_PREFAILED              0x03U
#define DEM_EVENT_STATUS_FDC_THRESHOLD_REACHED  0x04U

/**
 * @brief Event Status Extended Type (UDS DTC status byte)
 */
typedef uint8_t Dem_UdsStatusByteType;

#define DEM_UDS_STATUS_TF                       0x01U   /* Test Failed */
#define DEM_UDS_STATUS_TFTOC                    0x02U   /* Test Failed This Operation Cycle */
#define DEM_UDS_STATUS_PDTC                     0x04U   /* Pending DTC */
#define DEM_UDS_STATUS_CDTC                     0x08U   /* Confirmed DTC */
#define DEM_UDS_STATUS_TNCSLC                   0x10U   /* Test Not Completed Since Last Clear */
#define DEM_UDS_STATUS_TFSLC                    0x20U   /* Test Failed Since Last Clear */
#define DEM_UDS_STATUS_TNCTOC                   0x40U   /* Test Not Completed This Operation Cycle */
#define DEM_UDS_STATUS_WIR                      0x80U   /* Warning Indicator Requested */

/*============================================================================*
 * Event ID Types
 *============================================================================*/
/**
 * @brief Event ID Type
 */
typedef uint16_t Dem_EventIdType;

#define DEM_EVENT_ID_INVALID                    0x0000U
#define DEM_EVENT_ID_FIRST                      0x0001U
#define DEM_EVENT_ID_MAX                        0x0FFFU

/*============================================================================*
 * DTC Types
 *============================================================================*/
/**
 * @brief DTC Format Type
 */
typedef uint8_t Dem_DTCFormatType;

#define DEM_DTC_FORMAT_OBD                      0x00U
#define DEM_DTC_FORMAT_UDS                      0x01U
#define DEM_DTC_FORMAT_J1939                    0x02U

/**
 * @brief DTC Kind Type
 */
typedef uint8_t Dem_DTCKindType;

#define DEM_DTC_KIND_ALL_DTCS                   0x01U
#define DEM_DTC_KIND_EMISSION_REL_DTCS          0x02U

/**
 * @brief DTC Origin Type
 */
typedef uint8_t Dem_DTCOriginType;

#define DEM_DTC_ORIGIN_PRIMARY_MEMORY           0x01U
#define DEM_DTC_ORIGIN_MIRROR_MEMORY            0x02U
#define DEM_DTC_ORIGIN_PERMANENT_MEMORY         0x03U
#define DEM_DTC_ORIGIN_OBD_RELEVANT_MEMORY      0x04U

/**
 * @brief DTC Severity Type
 */
typedef uint8_t Dem_DTCSeverityType;

#define DEM_SEVERITY_NO_SEVERITY                0x00U
#define DEM_SEVERITY_MAINTENANCE_ONLY           0x20U
#define DEM_SEVERITY_CHECK_AT_NEXT_HALT         0x40U
#define DEM_SEVERITY_CHECK_IMMEDIATELY          0x80U

/*============================================================================*
 * Debounce Types
 *============================================================================*/
/**
 * @brief Debounce State Type
 */
typedef uint8_t Dem_DebounceStateType;

#define DEM_DEBOUNCE_STATUS_NONE                0x00U
#define DEM_DEBOUNCE_STATUS_PREPASSED           0x01U
#define DEM_DEBOUNCE_STATUS_PASSED              0x02U
#define DEM_DEBOUNCE_STATUS_PREFAILED           0x03U
#define DEM_DEBOUNCE_STATUS_FAILED              0x04U

/**
 * @brief Debounce Counter-Based Configuration
 */
typedef struct {
    uint8_t debounceCounterPassedThreshold;      /* Threshold for passed */
    uint8_t debounceCounterFailedThreshold;      /* Threshold for failed */
    sint8 debounceCounterIncrementStepSize;    /* Step size for increment */
    sint8 debounceCounterDecrementStepSize;    /* Step size for decrement */
    boolean jumpUp;                             /* Jump to failed allowed */
    boolean jumpDown;                           /* Jump to passed allowed */
} Dem_DebounceCounterBasedConfigType;

/**
 * @brief Debounce Time-Based Configuration
 */
typedef struct {
    uint32_t debounceTimePassedThreshold;        /* Time threshold for passed (ms) */
    uint32_t debounceTimeFailedThreshold;        /* Time threshold for failed (ms) */
    uint32_t debounceTimeIncrementStepSize;      /* Time increment step (ms) */
} Dem_DebounceTimeBasedConfigType;

/**
 * @brief Debounce Algorithm Type
 */
typedef uint8_t Dem_DebounceAlgorithmType;

#define DEM_DEBOUNCE_ALGORITHM_NONE             0x00U
#define DEM_DEBOUNCE_ALGORITHM_COUNTER_BASED    0x01U
#define DEM_DEBOUNCE_ALGORITHM_TIME_BASED       0x02U
#define DEM_DEBOUNCE_ALGORITHM_MONITOR_BASED    0x03U

/*============================================================================*
 * Freeze Frame Types
 *============================================================================*/
/**
 * @brief Freeze Frame Record Number Type
 */
typedef uint8_t Dem_FreezeFrameRecordNumberType;

#define DEM_FREEZE_FRAME_RECORD_NUMBER_INVALID  0x00U
#define DEM_FREEZE_FRAME_RECORD_NUMBER_0        0x01U
#define DEM_FREEZE_FRAME_RECORD_NUMBER_1        0x02U
#define DEM_FREEZE_FRAME_RECORD_NUMBER_2        0x03U

/**
 * @brief Freeze Frame Record Type
 */
typedef struct {
    uint32_t timestamp;                          /* Record timestamp (ms) */
    Dem_FreezeFrameRecordNumberType recordNumber;
    boolean recordValid;
    uint16_t dataSize;
    uint8_t data[256];                          /* Freeze frame data */
} Dem_FreezeFrameRecordType;

/**
 * @brief Freeze Frame Class Type
 */
typedef struct {
    uint16_t dataId;                            /* Data Identifier (DID) */
    uint16_t dataSize;
    uint8_t* dataPtr;
} Dem_FreezeFrameClassType;

/*============================================================================*
 * Extended Data Record Types
 *============================================================================*/
/**
 * @brief Extended Data Record Number Type
 */
typedef uint8_t Dem_ExtendedDataRecordNumberType;

#define DEM_EXTENDED_DATA_RECORD_NUMBER_INVALID 0x00U
#define DEM_EXTENDED_DATA_RECORD_NUMBER_1       0x01U
#define DEM_EXTENDED_DATA_RECORD_NUMBER_2       0x02U
#define DEM_EXTENDED_DATA_RECORD_NUMBER_3       0x03U
#define DEM_EXTENDED_DATA_RECORD_NUMBER_4       0x04U
#define DEM_EXTENDED_DATA_RECORD_NUMBER_5       0x05U
#define DEM_EXTENDED_DATA_RECORD_NUMBER_6       0x06U
#define DEM_EXTENDED_DATA_RECORD_NUMBER_OBD     0x8FU
#define DEM_EXTENDED_DATA_RECORD_NUMBER_ALL     0xFFU

/**
 * @brief Extended Data Record Type
 */
typedef struct {
    Dem_ExtendedDataRecordNumberType recordNumber;
    uint16_t dataSize;
    uint32_t timestamp;
    boolean recordValid;
    uint8_t data[128];                          /* Extended data */
} Dem_ExtendedDataRecordType;

/**
 * @brief Extended Data Record Class Type
 */
typedef struct {
    Dem_ExtendedDataRecordNumberType recordNumber;
    uint16_t dataSize;
    boolean isOBD;
    boolean storeOnTestFailed;
    boolean storeOnTestPassed;
    boolean storeOnConfirmed;
} Dem_ExtendedDataRecordClassType;

/*============================================================================*
 * Filter Types
 *============================================================================*/
/**
 * @brief DTC Filter Type
 */
typedef struct {
    uint8_t statusMask;
    Dem_DTCKindType dtcKind;
    Dem_DTCFormatType dtcFormat;
    Dem_DTCOriginType dtcOrigin;
    boolean filterIsSet;
    uint16_t currentIndex;
} Dem_DtcFilterType;

/**
 * @brief DTC Request Type
 */
typedef uint8_t Dem_DTCRequestType;

#define DEM_DTC_REQUEST_NONE                    0x00U
#define DEM_DTC_REQUEST_UDS                     0x01U
#define DEM_DTC_REQUEST_OBD                     0x02U
#define DEM_DTC_REQUEST_J1939                   0x03U

/*============================================================================*
 * Configuration Types
 *============================================================================*/
/**
 * @brief Event Configuration Type
 */
typedef struct {
    Dem_EventIdType eventId;
    uint32_t dtcCode;                           /* UDS DTC code */
    uint8_t dtcSeverity;
    Dem_DTCOriginType dtcOrigin;
    Dem_DebounceAlgorithmType debounceAlgorithm;
    union {
        Dem_DebounceCounterBasedConfigType counterConfig;
        Dem_DebounceTimeBasedConfigType timeConfig;
    } debounceConfig;
    uint8_t maxNumberFreezeFrameRecords;
    boolean eventRecoverable;
    boolean eventAvailable;
    boolean eventEnabled;
} Dem_EventConfigType;

/**
 * @brief DTC Configuration Type
 */
typedef struct {
    uint32_t dtcCode;
    uint16_t eventIdCount;
    Dem_EventIdType* eventIdList;
    Dem_DTCSeverityType dtcSeverity;
    uint8_t functionalUnit;
} Dem_DtcConfigType;

/**
 * @brief DEM Configuration Type
 */
typedef struct {
    const Dem_EventConfigType* eventConfigTable;
    uint16_t eventCount;
    const Dem_DtcConfigType* dtcConfigTable;
    uint16_t dtcCount;
    uint16_t maxFreezeFrameRecords;
    uint16_t maxExtendedDataRecords;
    boolean clearDtcLimitation;
    boolean clearDtcBehavior;
} Dem_ConfigType;

/*============================================================================*
 * Operation Cycle Types
 *============================================================================*/
/**
 * @brief Operation Cycle Type
 */
typedef uint8_t Dem_OperationCycleType;

#define DEM_OPCYC_POWER                         0x00U
#define DEM_OPCYC_IGNITION                      0x01U
#define DEM_OPCYC_WARMUP                        0x02U
#define DEM_OPCYC_OBD_DCY                       0x03U

/**
 * @brief Operation Cycle State Type
 */
typedef uint8_t Dem_OperationCycleStateType;

#define DEM_CYCLE_STATE_START                   0x00U
#define DEM_CYCLE_STATE_END                     0x01U

/*============================================================================*
 * Callback Types
 *============================================================================*/
/**
 * @brief Event Status Changed Callback
 */
typedef void (*Dem_EventStatusChangedCallbackType)(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatusOld,
    Dem_EventStatusType EventStatusNew
);

/**
 * @brief DTC Status Changed Callback
 */
typedef void (*Dem_DtcStatusChangedCallbackType)(
    uint32_t DTC,
    Dem_UdsStatusByteType DTCStatusOld,
    Dem_UdsStatusByteType DTCStatusNew
);

/**
 * @brief Data Element Callback (for Freeze Frame / Extended Data)
 */
typedef Std_ReturnType (*Dem_DataElementCallbackType)(
    uint8_t* DataElement,
    uint16_t* DataLength
);

/*============================================================================*
 * Internal State Types
 *============================================================================*/
/**
 * @brief Event Memory Entry Type
 */
typedef struct {
    Dem_EventIdType eventId;
    uint32_t dtcCode;
    Dem_UdsStatusByteType dtcStatus;
    Dem_EventStatusType eventStatus;
    sint8 debounceCounter;
    uint32_t occurrenceCounter;
    uint32_t faultDetectionCounter;
    uint32_t agingCounter;
    boolean isActive;
    boolean isSuppressed;
    Dem_FreezeFrameRecordType freezeFrame;
    Dem_ExtendedDataRecordType extendedData;
} Dem_EventMemoryEntryType;

/**
 * @brief DEM Module State Type
 */
typedef enum {
    DEM_STATE_UNINIT = 0,
    DEM_STATE_INIT,
    DEM_STATE_PREINIT
} Dem_StateType;

#ifdef __cplusplus
}
#endif

#endif /* DEM_TYPES_H */


/*==================================================================================================
 *                                      ADDITIONAL TYPE DEFINITIONS
 * CRITICAL FIX: Added missing types for AUTOSAR compliance
==================================================================================================*/

/* DTC Group Type */
typedef uint32 Dem_DTCGroupType;
#define DEM_DTC_GROUP_ALL_DTCS                (0x000000U)
#define DEM_DTC_GROUP_EMISSION_DTCS           (0x010000U)
#define DEM_DTC_GROUP_CHASSIS_DTCS            (0x020000U)
#define DEM_DTC_GROUP_POWERTRAIN_DTCS         (0x030000U)
#define DEM_DTC_GROUP_NETWORK_DTCS            (0x040000U)
#define DEM_DTC_GROUP_BODY_DTCS               (0x050000U)
#define DEM_DTC_GROUP_UDS_DTCS                (0x060000U)

/* Initialize Monitor Reason Type */
typedef uint8 Dem_InitMonitorReasonType;
#define DEM_INIT_MONITOR_CLEAR                (0x00U)
#define DEM_INIT_MONITOR_RESTART              (0x01U)
#define DEM_INIT_MONITOR_REENABLED            (0x02U)
#define DEM_INIT_MONITOR_STORAGE_REENABLED    (0x03U)

/* IUMPR Denominator Condition ID Type */
typedef uint8 Dem_IumprDenomCondIdType;
#define DEM_IUMPR_DENOM_COND_COLDSTART        (0x00U)
#define DEM_IUMPR_DENOM_COND_EVAP             (0x01U)
#define DEM_IUMPR_DENOM_COND_500MI            (0x02U)
#define DEM_IUMPR_DENOM_COND_NOIDLE           (0x03U)

/* IUMPR Denominator Condition Status Type */
typedef uint8 Dem_IumprDenomCondStatusType;
#define DEM_IUMPR_DENOM_COND_NOT_REACHED      (0x00U)
#define DEM_IUMPR_DENOM_COND_REACHED          (0x01U)
#define DEM_IUMPR_DENOM_COND_INHIBITED        (0x02U)

/* Operation Cycle ID Type */
typedef uint8 Dem_OperationCycleIdType;
#define DEM_OPCYC_IGNITION                    (0x00U)
#define DEM_OPCYC_OBD_DCY                     (0x01U)
#define DEM_OPCYC_WARMUP                      (0x02U)
#define DEM_OPCYC_POWER                       (0x03U)

/* Operation Cycle Type */
typedef uint8 Dem_OperationCycleType;
#define DEM_OPCYC_IGNITION_TYPE               (0x00U)
#define DEM_OPCYC_OBD_DCY_TYPE                (0x01U)
#define DEM_OPCYC_WARMUP_TYPE                 (0x02U)
#define DEM_OPCYC_POWER_TYPE                  (0x03U)

/* Operation Cycle State Type */
typedef uint8 Dem_OperationCycleStateType;
#define DEM_CYCLE_STATE_START                 (0x00U)
#define DEM_CYCLE_STATE_END                   (0x01U)

/* Debounce Algorithm Class Type */
typedef uint8 Dem_DebounceAlgorithmClassType;
#define DEM_DEBOUNCE_COUNTER_BASED            (0x00U)
#define DEM_DEBOUNCE_TIME_BASED               (0x01U)
#define DEM_DEBOUNCE_MONITOR_BASED            (0x02U)
#define DEM_DEBOUNCE_FREQUENCY_BASED          (0x03U)

/* Indicator Type */
typedef uint8 Dem_IndicatorStatusType;
#define DEM_INDICATOR_OFF                     (0x00U)
#define DEM_INDICATOR_CONTINUOUS              (0x01U)
#define DEM_INDICATOR_BLINKING                (0x02U)
#define DEM_INDICATOR_BLINKING_CONT           (0x03U)
#define DEM_INDICATOR_SLOW_BLINKING           (0x04U)
#define DEM_INDICATOR_FAST_BLINKING           (0x05U)

/* Indicator Behavior */
#define DEM_INDICATOR_BEHAVIOR_FAILURE        (0x00U)
#define DEM_INDICATOR_BEHAVIOR_HEALING        (0x01U)

/* Update Rule Type */
typedef uint8 Dem_UpdateRuleType;
#define DEM_UPDATE_RECORD_NO                  (0x00U)
#define DEM_UPDATE_RECORD_YES                 (0x01U)
#define DEM_UPDATE_RECORD_UPDATE              (0x02U)

/* Event Status Extended Type */
typedef uint8 Dem_EventStatusExtendedType;
#define DEM_EVENT_STATUS_EXTENDED_INIT        (0x00U)

/* DTC Status Mask Type */
typedef uint8 Dem_DTCStatusMaskType;

/* Severity Type */
typedef uint8 Dem_DTCSeverityType;
#define DEM_SEVERITY_NO_SEVERITY              (0x00U)
#define DEM_SEVERITY_MAINTENANCE_ONLY         (0x01U)
#define DEM_SEVERITY_CHECK_AT_NEXT_HALT       (0x02U)
#define DEM_SEVERITY_CHECK_IMMEDIATELY        (0x04U)

/* Functional Unit Type */
typedef uint8 Dem_FunctionalUnitType;

/* Client ID Type */
typedef uint8 Dem_ClientIdType;

/* Clear DTC Type */
typedef uint8 Dem_ClearDTCType;
#define DEM_CLEAR_ALL_DTCS                    (0x01U)
#define DEM_CLEAR_EMISSION_RELATED_DTCS       (0x02U)

/* Control DTC Setting Type */
typedef uint8 Dem_ControlDTCSettingType;
#define DEM_CONTROL_DTC_SETTING_ON            (0x00U)
#define DEM_CONTROL_DTC_SETTING_OFF           (0x01U)

/* Enable Condition Type */
typedef uint8 Dem_EnableConditionType;
#define DEM_ENABLE_CONDITION_GENERIC          (0x00U)

/* Storage Condition Type */
typedef uint8 Dem_StorageConditionType;
#define DEM_STORAGE_CONDITION_GENERIC         (0x00U)

/* Event Memory Entry Type */
typedef struct {
    Dem_EventIdType EventId;
    uint32 DTC;
    Dem_EventStatusExtendedType EventStatus;
    Dem_DTCStatusMaskType DTCStatus;
    uint16 OccurrenceCounter;
    uint8 AgingCounter;
    uint16 Timestamp;
    boolean ExtendedDataRecorded;
    boolean FreezeFrameRecorded;
} Dem_EventMemoryEntryType;

/* Event Queue Entry Type */
typedef struct {
    Dem_EventIdType EventId;
    Dem_EventStatusType EventStatus;
    uint8 Priority;
    uint32 Timestamp;
} Dem_EventQueueEntryType;

/* Debounce Counter Based Type */
typedef struct {
    sint16 Counter;
    sint16 IncrementStep;
    sint16 DecrementStep;
    sint16 FailedThreshold;
    sint16 PassedThreshold;
} Dem_DebounceCounterBasedType;

/* Debounce Time Based Type */
typedef struct {
    uint16 Timer;
    uint16 FailedThreshold;
    uint16 PassedThreshold;
    boolean TimerDirection;
} Dem_DebounceTimeBasedType;

/* Debounce Info Type */
typedef struct {
    Dem_DebounceAlgorithmClassType Algorithm;
    union {
        Dem_DebounceCounterBasedType Counter;
        Dem_DebounceTimeBasedType Time;
    } Data;
} Dem_DebounceInfoType;

/* Aging Data Type */
typedef struct {
    uint8 AgingCounter;
    boolean AgingAllowed;
    Dem_OperationCycleIdType AgingCycle;
} Dem_AgingDataType;

/* OCC (Occurrence Counter) Type */
typedef struct {
    uint16 Counter;
    uint16 Threshold;
} Dem_OCCType;

/* Counters Type */
typedef struct {
    uint16 FailureCounter;
    uint16 HealingCounter;
    uint8 ConsecutiveFailedCounter;
    uint8 ConsecutivePassedCounter;
} Dem_CountersType;

/* Indicator Attribute Type */
typedef struct {
    uint8 IndicatorId;
    uint8 Behavior;
    uint8 FailureCycleThreshold;
    uint8 HealingCycleThreshold;
} Dem_IndicatorAttributeType;

/* Freeze Frame Data Type */
typedef struct {
    uint8 RecordNumber;
    uint8 Data[DEM_CFG_MAX_FREEZEFRAME_SIZE];
    uint16 DataSize;
    uint32 Timestamp;
} Dem_FreezeFrameDataType;

/* Extended Data Type */
typedef struct {
    uint8 RecordNumber;
    uint8 Data[DEM_CFG_MAX_EXTENDED_DATA_SIZE];
    uint16 DataSize;
} Dem_ExtendedDataType;

