/**
 * @file dem_nvm.h
 * @brief DEM NvM Integration Interface
 * @version 1.0
 * @note AUTOSAR R22-11 compliant, MISRA C:2012
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#ifndef DEM_NVM_H
#define DEM_NVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dem_types.h"

/* Forward declarations for NvM types to avoid header conflicts */
typedef uint16_t NvM_BlockIdType;

typedef enum {
    NVM_REQ_OK = 0,
    NVM_REQ_NOT_OK = 1,
    NVM_REQ_PENDING = 2,
    NVM_REQ_INTEGRITY_FAILED = 3,
    NVM_REQ_BLOCK_SKIPPED = 4,
    NVM_REQ_NV_INVALIDATED = 5,
    NVM_REQ_CANCELLED = 6,
    NVM_REQ_RESTORED_FROM_ROM = 7
} NvM_RequestResultType;

/* NvM API declarations (defined in NvM.c) */
extern Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr);
extern Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr);
extern Std_ReturnType NvM_RestoreBlockDefaults(NvM_BlockIdType BlockId, void* NvM_DstPtr);
extern void NvM_CancelWriteAll(void);

/*============================================================================*
 * Macros and Constants
 *============================================================================*/
#define DEM_NVM_BLOCK_ID_EVENT_STATUS           0x0100U
#define DEM_NVM_BLOCK_ID_DTC_DATA               0x0101U
#define DEM_NVM_BLOCK_ID_FREEZE_FRAME           0x0102U
#define DEM_NVM_BLOCK_ID_EXTENDED_DATA          0x0103U
#define DEM_NVM_BLOCK_ID_OBD_DATA               0x0104U

#define DEM_NVM_WRITE_DELAY_MS                  100U    /* Delay before writing to NvM */
#define DEM_NVM_MAX_WRITE_RETRIES               3U

#ifndef DEM_FREEZE_FRAME_SIZE
#define DEM_FREEZE_FRAME_SIZE                   256U
#endif

#ifndef DEM_MAX_EVENTS
#define DEM_MAX_EVENTS                          128U
#endif

/*============================================================================*
 * NvM State Types
 *============================================================================*/
/**
 * @brief DEM NvM Write State
 */
typedef enum {
    DEM_NVM_WRITE_STATE_IDLE = 0,
    DEM_NVM_WRITE_STATE_PENDING,
    DEM_NVM_WRITE_STATE_IN_PROGRESS,
    DEM_NVM_WRITE_STATE_WAITING_FOR_CB,
    DEM_NVM_WRITE_STATE_COMPLETED,
    DEM_NVM_WRITE_STATE_FAILED
} Dem_NvmWriteStateType;

/**
 * @brief DEM NvM Block State
 */
typedef struct {
    NvM_BlockIdType blockId;
    Dem_NvmWriteStateType writeState;
    uint8_t writeRetries;
    uint32_t lastWriteRequestTime;
    boolean dataModified;
    boolean writePending;
} Dem_NvmBlockStateType;

/*============================================================================*
 * Non-Volatile Data Storage Types
 *============================================================================*/
/**
 * @brief Non-Volatile Event Data
 */
typedef struct {
    uint32_t dtcCode;
    Dem_UdsStatusByteType dtcStatus;
    uint32_t occurrenceCounter;
    uint32_t agingCounter;
    boolean isValid;
    uint8_t reserved[3];
} Dem_NvEventDataType;

/**
 * @brief Non-Volatile Freeze Frame Data
 */
typedef struct {
    uint32_t dtcCode;
    uint8_t recordNumber;
    uint8_t dataSize;
    uint16_t reserved;
    uint8_t data[DEM_FREEZE_FRAME_SIZE];
} Dem_NvFreezeFrameDataType;

/**
 * @brief Non-Volatile Extended Data
 */
typedef struct {
    uint32_t dtcCode;
    uint8_t recordNumber;
    uint8_t dataSize;
    uint16_t reserved;
    uint8_t data[128];
} Dem_NvExtendedDataType;

/**
 * @brief DEM Non-Volatile Data Container
 */
typedef struct {
    uint32_t version;
    uint32_t checksum;
    uint32_t writeCounter;
    uint16_t numEvents;
    uint16_t numDTCs;
    Dem_NvEventDataType events[DEM_MAX_EVENTS];
} Dem_NvDataContainerType;

/*============================================================================*
 * Function Prototypes
 *============================================================================*/
/**
 * @brief Initialize DEM NvM integration
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_NvMInit(void);

/**
 * @brief Read event data from NvM
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_NvMReadEventData(void);

/**
 * @brief Write event data to NvM
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_NvMWriteEventData(void);

/**
 * @brief Restore event data defaults
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_NvMRestoreEventDataDefaults(void);

/**
 * @brief Read freeze frame data from NvM
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_NvMReadFreezeFrameData(void);

/**
 * @brief Write freeze frame data to NvM
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_NvMWriteFreezeFrameData(void);

/**
 * @brief Read extended data from NvM
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_NvMReadExtendedData(void);

/**
 * @brief Write extended data to NvM
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_NvMWriteExtendedData(void);

/**
 * @brief NvM write callback function
 * @param ServiceId The service ID
 * @param JobResult The job result
 */
extern void Dem_NvMWriteCallback(uint8_t ServiceId, NvM_RequestResultType JobResult);

/**
 * @brief Request write of event data to NvM
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_NvMRequestWriteEventData(void);

/**
 * @brief Request write of freeze frame to NvM
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_NvMRequestWriteFreezeFrame(void);

/**
 * @brief Request write of extended data to NvM
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_NvMRequestWriteExtendedData(void);

/**
 * @brief Check if NvM write is pending
 * @return TRUE if write is pending, FALSE otherwise
 */
extern boolean Dem_NvMIsWritePending(void);

/**
 * @brief Cancel pending NvM write
 */
extern void Dem_NvMCancelWrite(void);

/**
 * @brief DEM NvM main function - should be called periodically
 */
extern void Dem_NvMMainFunction(void);

/**
 * @brief Calculate checksum for NvM data
 * @param data Pointer to data
 * @param size Size of data
 * @return Calculated checksum
 */
extern uint32_t Dem_NvMCalculateChecksum(const uint8_t* data, uint32_t size);

/**
 * @brief Validate NvM data integrity
 * @param container Pointer to data container
 * @return TRUE if valid, FALSE otherwise
 */
extern boolean Dem_NvMValidateData(const Dem_NvDataContainerType* container);

/**
 * @brief Mark event data as modified (triggers NvM write)
 */
extern void Dem_NvMMarkEventDataModified(void);

/**
 * @brief Mark freeze frame data as modified
 */
extern void Dem_NvMMarkFreezeFrameModified(void);

/**
 * @brief Mark extended data as modified
 */
extern void Dem_NvMMarkExtendedDataModified(void);

#ifdef __cplusplus
}
#endif

#endif /* DEM_NVM_H */
