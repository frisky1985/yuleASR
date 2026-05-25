/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Dem_Int.h
 * @brief Diagnostic Event Manager - Internal Header
 * @version 1.1.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * CRITICAL FIX: Internal header for Dem module private functions and data structures
 * Separated from public API for better encapsulation
 */

#ifndef DEM_INT_H
#define DEM_INT_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Dem_Types.h"
#include "Dem_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DEM_INT_VENDOR_ID                   (0x01U)
#define DEM_INT_MODULE_ID                   (0x54U)
#define DEM_INT_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define DEM_INT_AR_RELEASE_MINOR_VERSION    (0x04U)
#define DEM_INT_AR_RELEASE_REVISION_VERSION (0x00U)
#define DEM_INT_SW_MAJOR_VERSION            (0x01U)
#define DEM_INT_SW_MINOR_VERSION            (0x01U)
#define DEM_INT_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    INTERNAL CONSTANTS
==================================================================================================*/
#define DEM_INSTANCE_ID                 (0x00U)

/* Module state */
#define DEM_STATE_UNINIT                (0x00U)
#define DEM_STATE_INIT                  (0x01U)
#define DEM_STATE_PREINIT               (0x02U)

/* Invalid indices */
#define DEM_INVALID_EVENT_INDEX         (0xFFFFU)
#define DEM_INVALID_DTC_INDEX           (0xFFU)
#define DEM_INVALID_RECORD_INDEX        (0xFFU)

/* Time-based debounce states */
#define DEM_TIME_DEBOUNCE_IDLE          (0x00U)
#define DEM_TIME_DEBOUNCE_COUNTING_UP   (0x01U)
#define DEM_TIME_DEBOUNCE_COUNTING_DOWN (0x02U)

/*==================================================================================================
*                                    DTC ENTRY TYPE
==================================================================================================*/
/**
 * @brief DTC entry in fault memory
 */
typedef struct
{
    Dem_DTCType DTC;
    Dem_DTCStatusType Status;
    uint32 OccurrenceCounter;
    uint32 AgingCounter;
    boolean IsAged;
    boolean IsSuppressed;
    boolean IsDeleted;
    uint32 LastOccurrenceTimestamp;
    uint32 FirstOccurrenceTimestamp;
    /* CRITICAL FIX: Added for NvM integration */
    uint16 NvMBlockId;
    boolean IsNvMDataValid;
} Dem_DTCEntryType;

/*==================================================================================================
*                                    FREEZE FRAME ENTRY TYPE
==================================================================================================*/
/**
 * @brief Freeze frame data entry
 */
typedef struct
{
    uint8 Data[DEM_FREEZE_FRAME_MAX_SIZE];
    uint16 Length;
    boolean IsValid;
    uint32 Timestamp;
    uint16 DtcIndex;  /* Reference to associated DTC */
    uint8 RecordNumber;
    uint32 OccurrenceCounterSnapshot;
} Dem_FreezeFrameEntryType;

/*==================================================================================================
*                                    EXTENDED DATA ENTRY TYPE
==================================================================================================*/
/**
 * @brief Extended data record entry
 * CRITICAL FIX: Added for extended data support
 */
typedef struct
{
    uint8 Data[DEM_EXTENDED_DATA_MAX_SIZE];
    uint16 Length;
    boolean IsValid;
    uint32 Timestamp;
    uint16 DtcIndex;
    uint8 RecordNumber;
} Dem_ExtendedDataEntryType;

/*==================================================================================================
*                                    TIME-BASED DEBOUNCE STATE TYPE
==================================================================================================*/
/**
 * @brief Time-based debounce tracking
 * CRITICAL FIX: Added for time-based debounce support
 */
typedef struct
{
    uint8 State;  /* DEM_TIME_DEBOUNCE_IDLE/COUNTING_UP/COUNTING_DOWN */
    uint32 ElapsedTimeMs;
    uint32 StartTimestamp;
    boolean ThresholdReached;
} Dem_TimeDebounceStateType;

/*==================================================================================================
*                                    INTERNAL STATE TYPE
==================================================================================================*/
/**
 * @brief Module internal state structure
 */
typedef struct
{
    uint8 State;
    const Dem_ConfigType* ConfigPtr;
    Dem_EventStateType EventStates[DEM_NUM_EVENTS];
    Dem_DTCEntryType DTCEntries[DEM_NUM_DTCS];
    uint8 OperationCycleStates[DEM_NUM_OPERATION_CYCLES];
    boolean EnableConditions[DEM_NUM_ENABLE_CONDITIONS];
    boolean StorageConditions[DEM_NUM_STORAGE_CONDITIONS];
    Dem_DTCType SelectedDTC;
    boolean DTCRecordUpdateDisabled;
    boolean DTCSettingDisabled;
    Dem_FreezeFrameEntryType FreezeFrames[DEM_NUM_FREEZE_FRAME_RECORDS];
    /* CRITICAL FIX: Added extended data storage */
    Dem_ExtendedDataEntryType ExtendedDataRecords[DEM_NUM_EXTENDED_DATA_RECORDS];
    /* CRITICAL FIX: Added time-based debounce states */
    Dem_TimeDebounceStateType TimeDebounceStates[DEM_NUM_EVENTS];
    /* Filter state for DTC filtering */
    uint8 DTCFilterStatusMask;
    Dem_DTCFormatType DTCFilterFormat;
    Dem_DTCOriginType DTCFilterOrigin;
    uint16 FilteredDTCCount;
    uint16 CurrentFilteredIndex;
    /* Main function timestamp */
    uint32 LastMainFunctionTimestamp;
} Dem_InternalStateType;

/*==================================================================================================
*                                    EXTERNAL VARIABLES
==================================================================================================*/
#define DEM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

extern Dem_InternalStateType Dem_InternalState;

#define DEM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    INTERNAL FUNCTION PROTOTYPES
==================================================================================================*/
#define DEM_START_SEC_CODE
#include "MemMap.h"

/* Event configuration lookup */
extern const Dem_EventParameterType* Dem_IntFindEventConfig(Dem_EventIdType EventId);

/* DTC configuration lookup */
extern const Dem_DtcParameterType* Dem_IntFindDTCConfig(Dem_DTCType DTC);
extern uint8 Dem_IntFindDTCIndex(Dem_DTCType DTC);
extern uint8 Dem_IntFindOrCreateDTCEntry(Dem_DTCType DTC);

/* Debounce processing */
extern void Dem_IntUpdateDebounceCounter(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);
extern void Dem_IntResetDebounceCounter(Dem_EventIdType EventId);
extern void Dem_IntProcessTimeBasedDebounce(Dem_EventIdType EventId, 
                                            Dem_EventStatusType EventStatus,
                                            uint32 DeltaTimeMs);
extern void Dem_IntProcessMonitorInternalDebounce(Dem_EventIdType EventId, 
                                                  Dem_EventStatusType EventStatus);

/* DTC status management */
extern void Dem_IntUpdateDTCStatus(Dem_EventIdType EventId);
extern void Dem_IntUpdateDTCStatusFromDebounce(Dem_EventIdType EventId, 
                                               boolean DebounceResult);
extern void Dem_IntSetDTCStatusBits(uint8 DtcIndex, uint8 StatusBits);
extern void Dem_IntClearDTCStatusBits(uint8 DtcIndex, uint8 StatusBits);

/* Freeze frame management */
extern void Dem_IntStoreFreezeFrame(uint8 DtcIndex);
extern void Dem_IntUpdateFreezeFrame(uint8 DtcIndex);
extern void Dem_IntClearFreezeFrame(uint8 DtcIndex);
extern Std_ReturnType Dem_IntGetFreezeFrame(uint8 DtcIndex, 
                                            uint8 RecordNumber,
                                            uint8* DestBuffer, 
                                            uint16* BufferSize);

/* Extended data management - CRITICAL FIX: Added */
extern void Dem_IntStoreExtendedData(uint8 DtcIndex, uint8 RecordNumber);
extern void Dem_IntClearExtendedData(uint8 DtcIndex);
extern Std_ReturnType Dem_IntGetExtendedData(uint8 DtcIndex,
                                             uint8 RecordNumber,
                                             uint8* DestBuffer,
                                             uint16* BufferSize);
extern void Dem_IntUpdateOccurrenceCounterExtendedData(uint8 DtcIndex);

/* Aging processing */
extern void Dem_IntProcessAging(void);
extern void Dem_IntAgeDTCEntry(uint8 DtcIndex);

/* Operation cycle management */
extern void Dem_IntHandleOperationCycleStart(uint8 CycleIndex);
extern void Dem_IntHandleOperationCycleEnd(uint8 CycleIndex);

/* Filter processing */
extern boolean Dem_IntMatchDTCFilter(uint8 DtcIndex);
extern void Dem_IntUpdateFilteredCount(void);

/* DTC clearing */
extern void Dem_IntClearSingleDTC(uint8 DtcIndex);
extern void Dem_IntClearAllDTCs(void);

/* Event status callbacks */
extern void Dem_IntNotifyEventStatusChange(Dem_EventIdType EventId,
                                           Dem_EventStatusType NewStatus,
                                           Dem_EventStatusType OldStatus);

/* Notification handlers */
extern void Dem_IntCallErrorIntCallbacks(uint8 DtcIndex);

/* Pre-storage handling */
extern void Dem_IntPrestoreFreezeFrame(Dem_EventIdType EventId);
extern void Dem_IntClearPrestoredFreezeFrame(Dem_EventIdType EventId);

/* Utility functions */
extern boolean Dem_IntIsDTCCleared(uint8 DtcIndex);
extern boolean Dem_IntCanAgeDTC(uint8 DtcIndex);
extern uint8 Dem_IntGetEventIndex(Dem_EventIdType EventId);
extern Std_ReturnType Dem_IntCheckInit(void);
extern Std_ReturnType Dem_IntValidateEventId(Dem_EventIdType EventId);
extern Std_ReturnType Dem_IntValidateDTC(Dem_DTCType DTC, Dem_DTCOriginType Origin);

/* Main function helpers */
extern void Dem_IntProcessDebounceMainFunction(uint32 DeltaTimeMs);
extern void Dem_IntProcessAgingMainFunction(void);

#define DEM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DEM_INT_H */
