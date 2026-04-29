/**
 * @file dem_freeze_frame.h
 * @brief DEM Freeze Frame Management Interface
 * @version 1.0
 * @note AUTOSAR R22-11 compliant, MISRA C:2012
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#ifndef DEM_FREEZE_FRAME_H
#define DEM_FREEZE_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dem_types.h"

/*============================================================================*
 * Macros and Constants
 *============================================================================*/
#define DEM_MAX_FREEZE_FRAME_RECORDS            10U
#define DEM_MAX_FREEZE_FRAME_SIZE               256U
#define DEM_MAX_FREEZE_FRAME_CLASSES            16U

/* OBD Freeze Frame */
#define DEM_OBD_FREEZE_FRAME_RECORD_NUMBER      0x00U

/*============================================================================*
 * Freeze Frame Entry Type
 *============================================================================*/
/**
 * @brief Freeze Frame Entry (Internal Storage)
 */
typedef struct {
    uint32_t dtcCode;
    Dem_FreezeFrameRecordNumberType recordNumber;
    uint32_t timestamp;
    uint16_t dataSize;
    boolean isValid;
    uint8_t data[DEM_MAX_FREEZE_FRAME_SIZE];
} Dem_FreezeFrameEntryType;

/*============================================================================*
 * Function Prototypes
 *============================================================================*/
/**
 * @brief Initialize freeze frame management
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_FreezeFrameInit(void);

/**
 * @brief Store freeze frame for a DTC
 * @param dtcCode The DTC code
 * @param recordNumber The freeze frame record number
 * @param data Pointer to freeze frame data
 * @param dataSize Size of data
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_StoreFreezeFrame(
    uint32_t dtcCode,
    Dem_FreezeFrameRecordNumberType recordNumber,
    const uint8_t* data,
    uint16_t dataSize
);

/**
 * @brief Get freeze frame data by DTC
 * @param DTC The DTC code
 * @param DTCOrigin The DTC origin
 * @param RecordNumber The freeze frame record number
 * @param DestBuffer Pointer to destination buffer
 * @param BufSize Pointer to buffer size (input/output)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetFreezeFrameDataByDTC(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_FreezeFrameRecordNumberType RecordNumber,
    uint8_t* DestBuffer,
    uint16_t* BufSize
);

/**
 * @brief Get size of freeze frame selection
 * @param DTC The DTC code
 * @param DTCOrigin The DTC origin
 * @param RecordNumber The freeze frame record number
 * @param SizeOfFreezeFrame Pointer to store size
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetSizeOfFreezeFrameSelection(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_FreezeFrameRecordNumberType RecordNumber,
    uint16_t* SizeOfFreezeFrame
);

/**
 * @brief Get OBD freeze frame data
 * @param PID The Parameter ID
 * @param DestBuffer Pointer to destination buffer
 * @param BufSize Pointer to buffer size (input/output)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetOBDFreezeFrameData(
    uint8_t PID,
    uint8_t* DestBuffer,
    uint16_t* BufSize
);

/**
 * @brief Delete freeze frame for a DTC
 * @param dtcCode The DTC code
 * @param recordNumber The freeze frame record number
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_DeleteFreezeFrame(
    uint32_t dtcCode,
    Dem_FreezeFrameRecordNumberType recordNumber
);

/**
 * @brief Clear all freeze frames
 */
extern void Dem_ClearAllFreezeFrames(void);

/**
 * @brief Find freeze frame entry
 * @param dtcCode The DTC code
 * @param recordNumber The record number
 * @return Pointer to freeze frame entry, or NULL if not found
 */
extern Dem_FreezeFrameEntryType* Dem_FindFreezeFrameEntry(
    uint32_t dtcCode,
    Dem_FreezeFrameRecordNumberType recordNumber
);

/**
 * @brief Capture freeze frame data
 * @param entry Pointer to freeze frame entry to populate
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_CaptureFreezeFrameData(Dem_FreezeFrameEntryType* entry);

/**
 * @brief Get number of stored freeze frames
 * @return Number of valid freeze frames
 */
extern uint8_t Dem_GetNumberOfFreezeFrames(void);

/**
 * @brief Store extended data record
 * @param dtcCode The DTC code
 * @param recordNumber The extended data record number
 * @param data Pointer to data
 * @param dataSize Size of data
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_StoreExtendedDataRecord(
    uint32_t dtcCode,
    Dem_ExtendedDataRecordNumberType recordNumber,
    const uint8_t* data,
    uint16_t dataSize
);

/**
 * @brief Get extended data record by DTC
 * @param DTC The DTC code
 * @param DTCOrigin The DTC origin
 * @param ExtendedDataNumber The extended data record number
 * @param DestBuffer Pointer to destination buffer
 * @param BufSize Pointer to buffer size (input/output)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetExtendedDataRecordByDTC(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ExtendedDataRecordNumberType ExtendedDataNumber,
    uint8_t* DestBuffer,
    uint16_t* BufSize
);

/**
 * @brief Get size of extended data record
 * @param DTC The DTC code
 * @param DTCOrigin The DTC origin
 * @param ExtendedDataNumber The extended data record number
 * @param SizeOfExtendedDataRecord Pointer to store size
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetSizeOfExtendedDataRecordSelection(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ExtendedDataRecordNumberType ExtendedDataNumber,
    uint16_t* SizeOfExtendedDataRecord
);

/**
 * @brief Clear all extended data records
 */
extern void Dem_ClearAllExtendedDataRecords(void);

#ifdef __cplusplus
}
#endif

#endif /* DEM_FREEZE_FRAME_H */
