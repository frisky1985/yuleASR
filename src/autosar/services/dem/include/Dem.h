/**
 * @file Dem.h
 * @brief Diagnostic Event Manager module following AutoSAR Classic Platform 4.x standard
 * @version 1.1.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Diagnostic Event Manager (DEM)
 * Layer: Service Layer
 * Purpose: DTC (Diagnostic Trouble Code) management and fault memory handling
 * 
 * CRITICAL FIX (v1.1.0):
 * - Separated type definitions to Dem_Types.h
 * - Fixed null pointer dereference at line 132 (Events -> EventParameters)
 * - Added complete time-based debounce support
 * - Added extended data record support
 */

#ifndef DEM_H
#define DEM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Dem_Types.h"
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
#define DEM_SW_MINOR_VERSION            (0x01U) /* Updated for bug fixes */
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

/* Service ID aliases used in implementation */
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
#define DEM_E_PARAM_EVENT_ID            (0x13U)  /* Extended error code */

/*==================================================================================================
*                                    DTC GROUP DEFINITIONS
==================================================================================================*/
#define DEM_DTC_GROUP_ALL               ((Dem_DtcType)0xFFFFFFU)
#define DEM_DTC_GROUP_EMISSION_RELATED  ((Dem_DtcType)0x000001U)
#define DEM_DTC_GROUP_POWERTRAIN        ((Dem_DtcType)0x010000U)
#define DEM_DTC_GROUP_CHASSIS           ((Dem_DtcType)0x020000U)
#define DEM_DTC_GROUP_BODY              ((Dem_DtcType)0x030000U)
#define DEM_DTC_GROUP_NETWORK_COM       ((Dem_DtcType)0x040000U)

/*==================================================================================================
*                                    DTC KIND DEFINITIONS
==================================================================================================*/
#define DEM_DTC_KIND_ALL_DTCS           (0x01U)
#define DEM_DTC_KIND_EMISSION_REL_DTCS  (0x02U)

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define DEM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Diagnostic Event Manager
 * @param ConfigPtr Pointer to configuration structure
 * 
 * CRITICAL FIX: Now uses Dem_Config from Dem_Cfg.c instead of test configuration
 */
extern void Dem_Init(const Dem_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the DEM
 */
extern void Dem_DeInit(void);

/**
 * @brief Shuts down the DEM (alias for Dem_DeInit)
 */
extern void Dem_Shutdown(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
extern void Dem_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Sets event status
 * @param EventId Event ID
 * @param EventStatus Event status
 * @return Result of operation
 * 
 * CRITICAL FIX: Now properly handles time-based debounce algorithm
 */
extern Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);

/**
 * @brief Resets event status
 * @param EventId Event ID
 * @return Result of operation
 */
extern Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType EventId);

/**
 * @brief Gets event status
 * @param EventId Event ID
 * @param EventStatus Pointer to store status
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType* EventStatus);

/**
 * @brief Gets event failed status
 * @param EventId Event ID
 * @param EventFailed Pointer to store failed status
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetEventFailed(Dem_EventIdType EventId, boolean* EventFailed);

/**
 * @brief Gets event tested status
 * @param EventId Event ID
 * @param EventTested Pointer to store tested status
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetEventTested(Dem_EventIdType EventId, boolean* EventTested);

/**
 * @brief Gets fault detection counter
 * @param EventId Event ID
 * @param FaultDetectionCounter Pointer to store counter
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetFaultDetectionCounter(Dem_EventIdType EventId, sint8* FaultDetectionCounter);

/**
 * @brief Pre-stores freeze frame data
 * @param EventId Event ID
 * @return Result of operation
 */
extern Std_ReturnType Dem_PrestoreFreezeFrame(Dem_EventIdType EventId);

/**
 * @brief Clears pre-stored freeze frame data
 * @param EventId Event ID
 * @return Result of operation
 */
extern Std_ReturnType Dem_ClearPrestoredFreezeFrame(Dem_EventIdType EventId);

/**
 * @brief Gets status of DTC
 * @param DTC DTC number
 * @param DTCOrigin DTC origin
 * @param DTCStatus Pointer to store status
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetStatusOfDTC(Dem_DtcType DTC,
                                         Dem_DTCOriginType DTCOrigin,
                                         Dem_UdsStatusByteType* DTCStatus);

/**
 * @brief Gets DTC status availability mask
 * @param DTCStatusMask Pointer to store mask
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetDTCStatusAvailabilityMask(uint8* DTCStatusMask);

/**
 * @brief Gets number of filtered DTCs
 * @param NumberOfFilteredDTC Pointer to store count
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetNumberOfFilteredDTC(uint16* NumberOfFilteredDTC);

/**
 * @brief Gets next filtered DTC
 * @param DTC Pointer to store DTC
 * @param DTCStatus Pointer to store status
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetNextFilteredDTC(Dem_DtcType* DTC, Dem_UdsStatusByteType* DTCStatus);

/**
 * @brief Clears DTC
 * @param DTC DTC to clear
 * @param DTCFormat DTC format
 * @param DTCOrigin DTC origin
 * @return Result of operation
 */
extern Std_ReturnType Dem_ClearDTC(Dem_DtcType DTC,
                                   Dem_DTCFormatType DTCFormat,
                                   Dem_DTCOriginType DTCOrigin);

/**
 * @brief Selects a DTC for subsequent operations
 * @param DTC DTC to select
 * @param DTCFormat DTC format
 * @param DTCOrigin DTC origin
 * @return Result of operation
 */
extern Std_ReturnType Dem_SelectDTC(Dem_DtcType DTC, 
                                    Dem_DTCFormatType DTCFormat, 
                                    Dem_DTCOriginType DTCOrigin);

/**
 * @brief Disables DTC setting
 * @param DTCGroup DTC group
 * @param DTCKind DTC kind
 * @return Result of operation
 */
extern Std_ReturnType Dem_DisableDTCSetting(Dem_DtcType DTCGroup, uint8 DTCKind);

/**
 * @brief Enables DTC setting
 * @param DTCGroup DTC group
 * @param DTCKind DTC kind
 * @return Result of operation
 */
extern Std_ReturnType Dem_EnableDTCSetting(Dem_DtcType DTCGroup, uint8 DTCKind);

/**
 * @brief Disables DTC record update
 * @return Result of operation
 */
extern Std_ReturnType Dem_DisableDTCRecordUpdate(void);

/**
 * @brief Enables DTC record update
 * @return Result of operation
 */
extern Std_ReturnType Dem_EnableDTCRecordUpdate(void);

/**
 * @brief Gets indicator status
 * @param IndicatorId Indicator ID
 * @param IndicatorStatus Pointer to store status
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetIndicatorStatus(uint8 IndicatorId, Dem_IndicatorStatusType* IndicatorStatus);

/**
 * @brief Sets indicator status (internal use)
 * @param IndicatorId Indicator ID
 * @param IndicatorStatus Indicator status
 * @return Result of operation
 */
extern Std_ReturnType Dem_SetIndicatorStatus(uint8 IndicatorId, Dem_IndicatorStatusType IndicatorStatus);

/**
 * @brief Gets freeze frame data by DTC
 * @param DTC DTC number
 * @param DTCOrigin DTC origin
 * @param RecordNumber Record number
 * @param DestBuffer Destination buffer
 * @param BufferSize Buffer size pointer
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetFreezeFrameDataByDTC(Dem_DtcType DTC,
                                                  Dem_DTCOriginType DTCOrigin,
                                                  uint8 RecordNumber,
                                                  uint8* DestBuffer,
                                                  uint16* BufferSize);

/**
 * @brief Gets extended data record by DTC
 * @param DTC DTC number
 * @param DTCOrigin DTC origin
 * @param ExtendedDataNumber Extended data record number
 * @param DestBuffer Destination buffer
 * @param BufferSize Buffer size pointer
 * @return Result of operation
 * 
 * CRITICAL FIX: Added extended data record support
 */
extern Std_ReturnType Dem_GetExtendedDataRecordByDTC(Dem_DtcType DTC,
                                                     Dem_DTCOriginType DTCOrigin,
                                                     uint8 ExtendedDataNumber,
                                                     uint8* DestBuffer,
                                                     uint16* BufferSize);

/**
 * @brief Gets size of extended data record
 * @param DTC DTC number
 * @param DTCOrigin DTC origin
 * @param ExtendedDataNumber Extended data record number
 * @param SizeOfExtendedDataRecord Pointer to store size
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetSizeOfExtendedDataRecordByDTC(Dem_DtcType DTC,
                                                           Dem_DTCOriginType DTCOrigin,
                                                           uint8 ExtendedDataNumber,
                                                           uint16* SizeOfExtendedDataRecord);

/**
 * @brief Gets DTC by occurrence time
 * @param DTCRequest Request type
 * @param DTC Pointer to store DTC
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetDTCByOccurrenceTime(uint8 DTCRequest, Dem_DtcType* DTC);

/**
 * @brief Gets DTC of check failed
 * @param DTC Pointer to store DTC
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetDTCOfCheckFailed(Dem_DtcType* DTC);

/**
 * @brief Gets severity of DTC
 * @param DTC DTC number
 * @param DTCSeverity Pointer to store severity
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetSeverityOfDTC(Dem_DtcType DTC, Dem_DTCSeverityType* DTCSeverity);

/**
 * @brief Gets functional unit of DTC
 * @param DTC DTC number
 * @param DTCFunctionalUnit Pointer to store functional unit
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetFunctionalUnitOfDTC(Dem_DtcType DTC, uint8* DTCFunctionalUnit);

/**
 * @brief Sets operation cycle state
 * @param OperationCycleType Operation cycle type
 * @param CycleState Cycle state
 * @return Result of operation
 */
extern Std_ReturnType Dem_SetOperationCycleState(Dem_OperationCycleType OperationCycleType,
                                                 Dem_OperationCycleStateType CycleState);

/**
 * @brief Gets operation cycle state
 * @param OperationCycleType Operation cycle type
 * @param CycleState Pointer to store cycle state
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetOperationCycleState(Dem_OperationCycleType OperationCycleType,
                                                 Dem_OperationCycleStateType* CycleState);

/**
 * @brief Restarts operation cycle
 * @param OperationCycleType Operation cycle type
 * @return Result of operation
 */
extern Std_ReturnType Dem_RestartOperationCycle(Dem_OperationCycleType OperationCycleType);

/**
 * @brief Gets operation cycle counter
 * @param OperationCycleType Operation cycle type
 * @param CycleCounter Pointer to store counter
 * @return Result of operation
 */
extern Std_ReturnType Dem_GetCycleCounter(Dem_OperationCycleType OperationCycleType, uint16* CycleCounter);

/**
 * @brief Main function - periodic processing
 * 
 * CRITICAL FIX: Now properly handles time-based debounce and aging
 */
extern void Dem_MainFunction(void);

#define DEM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DEM_H */
