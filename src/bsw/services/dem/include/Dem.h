/**
 * @file Dem.h
 * @brief Diagnostic Event Manager module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Diagnostic Event Manager (DEM)
 * Layer: Service Layer
 * Purpose: DTC (Diagnostic Trouble Code) management and fault memory handling
 */

#ifndef DEM_H
#define DEM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Dem_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DEM_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define DEM_MODULE_ID                   (0x54U) /* DEM Module ID */
#define DEM_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define DEM_AR_RELEASE_MINOR_VERSION    (0x04U)
#define DEM_AR_RELEASE_REVISION_VERSION (0x00U)
#define DEM_SW_MAJOR_VERSION            (0x01U)
#define DEM_SW_MINOR_VERSION            (0x00U)
#define DEM_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define DEM_SID_INIT                    (0x01U)
#define DEM_SID_SHUTDOWN                (0x02U)
#define DEM_SID_GETVERSIONINFO          (0x03U)
#define DEM_SID_SETEVENTSTATUS          (0x04U)
#define DEM_SID_RESETEVENTSTATUS        (0x05U)
#define DEM_SID_PRESTORAGE              (0x06U)
#define DEM_SID_CLEARPRESTOREDFF        (0x07U)
#define DEM_SID_GETSTATUSOFDTC          (0x08U)
#define DEM_SID_GETDTCSTATUSAVAILABILITYMASK (0x09U)
#define DEM_SID_GETNUMBEROFFILTEREDDTC  (0x0AU)
#define DEM_SID_GETNEXTFILTEREDDTC      (0x0BU)
#define DEM_SID_GETDTCOFCHECKFAILED     (0x0CU)
#define DEM_SID_GETSEVERITYOFDTC        (0x0DU)
#define DEM_SID_GETFUNCTIONALUNITOFDTC  (0x0EU)
#define DEM_SID_CLEARDTC                (0x0FU)
#define DEM_SID_DISABLEDTCSETTING       (0x10U)
#define DEM_SID_ENABLEDTCSETTING        (0x11U)
#define DEM_SID_GETINDICATORSTATUS      (0x12U)
#define DEM_SID_SETINDICATORSTATUS      (0x13U)
#define DEM_SID_GETFREEZEFRAMEDATABYDTC (0x14U)
#define DEM_SID_GETEXTENDEDDATARECORDBYDTC (0x15U)
#define DEM_SID_GETSIZEOFEXTENDEDDATARECORDBYDTC (0x16U)
#define DEM_SID_GETSIZEOFFREEZEFRAMEBYDTC (0x17U)
#define DEM_SID_GETDTCBYOCCURRENCETIME  (0x18U)
#define DEM_SID_GETFAULTDETECTIONCOUNTER (0x19U)
#define DEM_SID_MAINFUNCTION            (0x1AU)
#define DEM_SID_SETOPERATIONCYCLESTATE  (0x1BU)
#define DEM_SID_RESTARTOPERATIONCYCLE   (0x1CU)
#define DEM_SID_SETAGINGCYCLESTATE      (0x1DU)
#define DEM_SID_GETCYCLECOUNTER         (0x1EU)
#define DEM_SID_GETDTCOFCHECKWARMUP     (0x1FU)
#define DEM_SID_GETDTCOFCHECKWARMUPCOUNTER (0x20U)

/* Service ID aliases used in Dem.c */
#define DEM_SERVICE_ID_INIT             DEM_SID_INIT
#define DEM_SERVICE_ID_DEINIT           DEM_SID_SHUTDOWN
#define DEM_SERVICE_ID_SETEVENTSTATUS   DEM_SID_SETEVENTSTATUS
#define DEM_SERVICE_ID_RESETEVENTSTATUS DEM_SID_RESETEVENTSTATUS
#define DEM_SERVICE_ID_GETEVENTSTATUS   DEM_SID_GETSTATUSOFDTC
#define DEM_SERVICE_ID_GETEVENTFAILED   DEM_SID_GETSTATUSOFDTC
#define DEM_SERVICE_ID_GETEVENTTESTED   DEM_SID_GETSTATUSOFDTC
#define DEM_SERVICE_ID_GETFAULTDETECTION DEM_SID_GETFAULTDETECTIONCOUNTER
#define DEM_SERVICE_ID_GETDTCSTATUS     DEM_SID_GETSTATUSOFDTC
#define DEM_SERVICE_ID_CLEARDTC         DEM_SID_CLEARDTC
#define DEM_SERVICE_ID_SELECTEDDTC      DEM_SID_GETSTATUSOFDTC
#define DEM_SERVICE_ID_DISABLEDTCRECORD DEM_SID_DISABLEDTCSETTING
#define DEM_SERVICE_ID_ENABLEDTCRECORD  DEM_SID_ENABLEDTCSETTING
#define DEM_SERVICE_ID_GETVERSIONINFO   DEM_SID_GETVERSIONINFO

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define DEM_E_PARAM_CONFIG              (0x10U)
#define DEM_E_PARAM_DATA                (0x11U)
#define DEM_E_PARAM_POINTER             (0x12U)
#define DEM_E_UNINIT                    (0x20U)
#define DEM_E_NODATAAVAILABLE           (0x30U)
#define DEM_E_WRONG_CONDITION           (0x40U)
#define DEM_E_WRONG_CONFIGURATION       (0x50U)

/* Error code aliases used in Dem.c */
#define DEM_E_PARAM_EVENT_ID            DEM_E_PARAM_DATA

/*==================================================================================================
*                                    DEM EVENT STATUS TYPE
==================================================================================================*/
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
typedef uint8 Dem_UdsStatusByteType;

/*==================================================================================================
*                                    DEM DTC STATUS MASK BITS
==================================================================================================*/
#define DEM_UDS_STATUS_TF               (0x01U)  /* Test Failed */
#define DEM_UDS_STATUS_TFTOC            (0x02U)  /* Test Failed This Operation Cycle */
#define DEM_UDS_STATUS_PDTC             (0x04U)  /* Pending DTC */
#define DEM_UDS_STATUS_CDTC             (0x08U)  /* Confirmed DTC */
#define DEM_UDS_STATUS_TNCSLC           (0x10U)  /* Test Not Completed Since Last Clear */
#define DEM_UDS_STATUS_TFSLC            (0x20U)  /* Test Failed Since Last Clear */
#define DEM_UDS_STATUS_TNCTOC           (0x40U)  /* Test Not Completed This Operation Cycle */
#define DEM_UDS_STATUS_WIR              (0x80U)  /* Warning Indicator Requested */

/* DTC status bit aliases used in Dem.c */
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

/* Cycle state aliases */
#define DEM_OPCYC_POWER                 (0U)
#define DEM_OPCYC_IGNITION              (1U)
#define DEM_OPCYC_WARMUP                (2U)
#define DEM_OPCYC_OBD_DCY               (3U)
#define DEM_OPCYC_OTHER                 (4U)

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
*                                    DEM DTC TYPE ALIASES
==================================================================================================*/
typedef Dem_DtcType Dem_DTCType;
typedef uint8 Dem_DTCStatusType;
typedef sint8 Dem_FaultDetectionCounterType;

/*==================================================================================================
*                                    DEM EVENT STATE TYPE
==================================================================================================*/
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
} Dem_EventStateType;

/*==================================================================================================
*                                    DEM DTC TYPE
==================================================================================================*/
typedef uint32 Dem_DtcType;

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
    boolean EventDebounceAlgorithm;
    boolean EventCounterBased;
    boolean EventTimeBased;
    boolean EventMonitorInternal;
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
} Dem_DtcParameterType;

/*==================================================================================================
*                                    DEM FREEZE FRAME RECORD TYPE
==================================================================================================*/
typedef struct {
    uint8 RecordNumber;
    uint8 NumDids;
    const uint16* DidIds;
} Dem_FreezeFrameRecordType;

/*==================================================================================================
*                                    DEM EXTENDED DATA RECORD TYPE
==================================================================================================*/
typedef struct {
    uint8 RecordNumber;
    uint16 DataSize;
} Dem_ExtendedDataRecordType;

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
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean ClearDtcSupported;
    boolean ClearDtcLimitation;
    boolean DtcStatusAvailabilityMask;
    boolean OBDRelevantSupport;
    boolean J1939Support;
} Dem_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define DEM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Dem_ConfigType Dem_Config;

#define DEM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define DEM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Diagnostic Event Manager
 * @param ConfigPtr Pointer to configuration structure
 */
void Dem_Init(const Dem_ConfigType* ConfigPtr);

/**
 * @brief Shuts down the DEM
 */
void Dem_Shutdown(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Dem_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Sets event status
 * @param EventId Event ID
 * @param EventStatus Event status
 * @return Result of operation
 */
Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);

/**
 * @brief Resets event status
 * @param EventId Event ID
 * @return Result of operation
 */
Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType EventId);

/**
 * @brief Pre-stores freeze frame data
 * @param EventId Event ID
 * @return Result of operation
 */
Std_ReturnType Dem_PrestoreFreezeFrame(Dem_EventIdType EventId);

/**
 * @brief Clears pre-stored freeze frame data
 * @param EventId Event ID
 * @return Result of operation
 */
Std_ReturnType Dem_ClearPrestoredFreezeFrame(Dem_EventIdType EventId);

/**
 * @brief Gets status of DTC
 * @param DTC DTC number
 * @param DTCOrigin DTC origin
 * @param DTCStatus Pointer to store status
 * @return Result of operation
 */
Std_ReturnType Dem_GetStatusOfDTC(Dem_DtcType DTC,
                                   Dem_DTCOriginType DTCOrigin,
                                   Dem_UdsStatusByteType* DTCStatus);

/**
 * @brief Gets DTC status availability mask
 * @param DTCStatusMask Pointer to store mask
 * @return Result of operation
 */
Std_ReturnType Dem_GetDTCStatusAvailabilityMask(uint8* DTCStatusMask);

/**
 * @brief Gets number of filtered DTCs
 * @param NumberOfFilteredDTC Pointer to store count
 * @return Result of operation
 */
Std_ReturnType Dem_GetNumberOfFilteredDTC(uint16* NumberOfFilteredDTC);

/**
 * @brief Gets next filtered DTC
 * @param DTC Pointer to store DTC
 * @param DTCStatus Pointer to store status
 * @return Result of operation
 */
Std_ReturnType Dem_GetNextFilteredDTC(Dem_DtcType* DTC, Dem_UdsStatusByteType* DTCStatus);

/**
 * @brief Clears DTC
 * @param DTC DTC to clear
 * @param DTCFormat DTC format
 * @param DTCOrigin DTC origin
 * @return Result of operation
 */
Std_ReturnType Dem_ClearDTC(Dem_DtcType DTC,
                             Dem_DTCFormatType DTCFormat,
                             Dem_DTCOriginType DTCOrigin);

/**
 * @brief Disables DTC setting
 * @param DTCGroup DTC group
 * @param DTCKind DTC kind
 * @return Result of operation
 */
Std_ReturnType Dem_DisableDTCSetting(Dem_DtcType DTCGroup, uint8 DTCKind);

/**
 * @brief Enables DTC setting
 * @param DTCGroup DTC group
 * @param DTCKind DTC kind
 * @return Result of operation
 */
Std_ReturnType Dem_EnableDTCSetting(Dem_DtcType DTCGroup, uint8 DTCKind);

/**
 * @brief Gets indicator status
 * @param IndicatorId Indicator ID
 * @param IndicatorStatus Pointer to store status
 * @return Result of operation
 */
Std_ReturnType Dem_GetIndicatorStatus(uint8 IndicatorId, Dem_IndicatorStatusType* IndicatorStatus);

/**
 * @brief Sets operation cycle state
 * @param OperationCycleType Operation cycle type
 * @param CycleState Cycle state
 * @return Result of operation
 */
Std_ReturnType Dem_SetOperationCycleState(Dem_OperationCycleType OperationCycleType,
                                           Dem_OperationCycleStateType CycleState);

/**
 * @brief Restarts operation cycle
 * @param OperationCycleType Operation cycle type
 * @return Result of operation
 */
Std_ReturnType Dem_RestartOperationCycle(Dem_OperationCycleType OperationCycleType);

/**
 * @brief Gets fault detection counter
 * @param EventId Event ID
 * @param FaultDetectionCounter Pointer to store counter
 * @return Result of operation
 */
Std_ReturnType Dem_GetFaultDetectionCounter(Dem_EventIdType EventId, sint8* FaultDetectionCounter);

/**
 * @brief Gets event status
 * @param EventId Event ID
 * @param EventStatus Pointer to store status
 * @return Result of operation
 */
Std_ReturnType Dem_GetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType* EventStatus);

/**
 * @brief Gets event failed status
 * @param EventId Event ID
 * @param EventFailed Pointer to store failed status
 * @return Result of operation
 */
Std_ReturnType Dem_GetEventFailed(Dem_EventIdType EventId, boolean* EventFailed);

/**
 * @brief Gets event tested status
 * @param EventId Event ID
 * @param EventTested Pointer to store tested status
 * @return Result of operation
 */
Std_ReturnType Dem_GetEventTested(Dem_EventIdType EventId, boolean* EventTested);

/**
 * @brief Gets DTC status
 * @param DTC DTC number
 * @param DTCOrigin DTC origin
 * @param DTCStatus Pointer to store status
 * @return Result of operation
 */
Std_ReturnType Dem_GetDTCStatus(Dem_DTCType DTC, Dem_DTCOriginType DTCOrigin, Dem_DTCStatusType* DTCStatus);

/**
 * @brief Selects a DTC
 * @param DTC DTC to select
 * @param DTCFormat DTC format
 * @param DTCOrigin DTC origin
 * @return Result of operation
 */
Std_ReturnType Dem_SelectDTC(Dem_DTCType DTC, Dem_DTCFormatType DTCFormat, Dem_DTCOriginType DTCOrigin);

/**
 * @brief Disables DTC record update
 * @return Result of operation
 */
Std_ReturnType Dem_DisableDTCRecordUpdate(void);

/**
 * @brief Enables DTC record update
 * @return Result of operation
 */
Std_ReturnType Dem_EnableDTCRecordUpdate(void);

/**
 * @brief Gets operation cycle state
 * @param OperationCycleId Operation cycle ID
 * @param CycleState Pointer to store cycle state
 * @return Result of operation
 */
Std_ReturnType Dem_GetOperationCycleState(uint8 OperationCycleId, uint8* CycleState);

/**
 * @brief Gets freeze frame data by DTC
 * @param DTC DTC number
 * @param DTCOrigin DTC origin
 * @param RecordNumber Record number
 * @param DestBuffer Destination buffer
 * @param BufferSize Buffer size
 * @return Result of operation
 */
Std_ReturnType Dem_GetFreezeFrameDataByDTC(Dem_DtcType DTC, Dem_DTCOriginType DTCOrigin,
                                            uint8 RecordNumber, uint8* DestBuffer,
                                            uint16* BufferSize);

/**
 * @brief Main function for periodic processing
 */
void Dem_MainFunction(void);

#define DEM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DEM_H */
