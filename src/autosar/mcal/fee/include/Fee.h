/*==================================================================================================
 *                                      FLASH EEPROM EMULATION DRIVER
 *                                      (MCAL LAYER)
 *==================================================================================================
 * FILENAME: Fee.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_FlashEEPROMEmulation.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: MCAL layer Fee driver - provides flash access for EEPROM emulation
 *==================================================================================================
 */

#ifndef FEE_MCAL_H
#define FEE_MCAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "Fee_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define FEE_VENDOR_ID                   (100u)
#define FEE_MODULE_ID                   (30u)
#define FEE_INSTANCE_ID                 (0u)

#define FEE_AR_RELEASE_MAJOR_VERSION    (4u)
#define FEE_AR_RELEASE_MINOR_VERSION    (7u)
#define FEE_AR_RELEASE_REVISION_VERSION (0u)

#define FEE_SW_MAJOR_VERSION            (1u)
#define FEE_SW_MINOR_VERSION            (0u)
#define FEE_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    FILE VERSION CHECKS
 *==================================================================================================*/
#ifndef DISABLE_MCAL_INTERMODULE_ASR_CHECK
    #if ((FEE_AR_RELEASE_MAJOR_VERSION != STD_TYPES_AR_RELEASE_MAJOR_VERSION) || \
         (FEE_AR_RELEASE_MINOR_VERSION != STD_TYPES_AR_RELEASE_MINOR_VERSION))
        #error "AutoSAR Version Numbers of Fee.h and Std_Types.h are different"
    #endif
#endif

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define FEE_SID_INIT                    (0x00u)
#define FEE_SID_DEINIT                  (0x01u)
#define FEE_SID_SETMODE                 (0x02u)
#define FEE_SID_READ                    (0x03u)
#define FEE_SID_WRITE                   (0x04u)
#define FEE_SID_ERASE                   (0x05u)
#define FEE_SID_COMPARE                 (0x06u)
#define FEE_SID_BLANKCHECK              (0x07u)
#define FEE_SID_GETSTATUS               (0x08u)
#define FEE_SID_GETJOBRESULT            (0x09u)
#define FEE_SID_GETVERSIONINFO          (0x0Au)
#define FEE_SID_CANCEL                  (0x0Bu)
#define FEE_SID_SUSPEND                 (0x0Cu)
#define FEE_SID_RESUME                  (0x0Du)
#define FEE_SID_MAINFUNCTION            (0x0Eu)
#define FEE_SID_SECTORERASE             (0x0Fu)
#define FEE_SID_SECTORWRITE             (0x10u)

/*==================================================================================================
 *                                    DET ERROR CODES
 *==================================================================================================*/
#define FEE_E_PARAM_CONFIG              (0x01u)
#define FEE_E_PARAM_ADDRESS             (0x02u)
#define FEE_E_PARAM_LENGTH              (0x03u)
#define FEE_E_PARAM_DATA                (0x04u)
#define FEE_E_UNINIT                    (0x05u)
#define FEE_E_BUSY                      (0x06u)
#define FEE_E_INVALID_LENGTH            (0x07u)
#define FEE_E_INVALID_ADDRESS           (0x08u)
#define FEE_E_PARAM_POINTER             (0x09u)
#define FEE_E_ALREADY_INITIALIZED       (0x0Au)
#define FEE_E_ERASE_FAILED              (0x0Bu)
#define FEE_E_WRITE_FAILED              (0x0Cu)
#define FEE_E_READ_FAILED               (0x0Du)
#define FEE_E_COMPARE_FAILED            (0x0Eu)
#define FEE_E_INVALID_MODE              (0x0Fu)
#define FEE_E_INVALID_SUSPEND           (0x10u)
#define FEE_E_SUSPENDED                 (0x11u)
#define FEE_E_INVALID_RESUME            (0x12u)

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/**
 * @brief Fee address type - represents flash address
 */
typedef uint32 Fee_AddressType;

/**
 * @brief Fee length type - represents length in bytes
 */
typedef uint32 Fee_LengthType;

/**
 * @brief Fee driver state type
 */
typedef enum {
    FEE_UNINIT = 0,         /**< Driver not initialized */
    FEE_IDLE,               /**< Driver initialized, no job running */
    FEE_BUSY                /**< Job currently processing */
} Fee_StateType;

/**
 * @brief Fee job result type
 */
typedef enum {
    FEE_JOB_OK = 0,         /**< Job completed successfully */
    FEE_JOB_FAILED,         /**< Job failed */
    FEE_JOB_PENDING,        /**< Job is pending/asynchronous */
    FEE_JOB_CANCELLED,      /**< Job was cancelled */
    FEE_BLOCK_INCONSISTENT, /**< Block inconsistent */
    FEE_BLOCK_INVALID       /**< Block invalid */
} Fee_JobResultType;

/**
 * @brief Fee operation mode type
 */
typedef enum {
    FEE_MODE_NORMAL = 0,    /**< Normal operation mode */
    FEE_MODE_FAST           /**< Fast operation mode (if supported) */
} Fee_ModeType;

/**
 * @brief Fee sector type - represents a flash sector
 */
typedef struct {
    Fee_AddressType sectorStartAddr;    /**< Sector start address */
    Fee_LengthType sectorSize;          /**< Sector size in bytes */
    uint32 sectorPageSize;              /**< Page size for write operations */
    uint32 sectorEraseCycles;           /**< Maximum erase cycles for this sector */
    boolean sectorWritable;             /**< Sector can be written */
    boolean sectorErasable;             /**< Sector can be erased */
} Fee_SectorType;

/**
 * @brief Fee block type - represents an EEPROM emulation block
 */
typedef struct {
    uint16 blockNumber;                 /**< Block number/identifier */
    Fee_AddressType blockStartAddr;     /**< Block start address in flash */
    Fee_LengthType blockSize;           /**< Block size in bytes */
    uint32 writeCycleCount;             /**< Maximum write cycles */
    boolean immediateData;              /**< Immediate data block flag */
} Fee_BlockType;

/**
 * @brief Fee block configuration type (for link-time configuration)
 */
typedef struct {
    uint16 FeeBlockNumber;              /**< Block number/identifier */
    uint16 FeeBlockSize;                /**< Block size in bytes */
    boolean FeeImmediateData;           /**< Immediate data block flag */
    uint8 FeeDeviceIndex;               /**< Device index */
    uint32 FeeBlockCycleCount;          /**< Maximum write cycles */
    uint32 FeeDataAlignment;            /**< Data alignment */
} Fee_BlockConfigType;

/**
 * @brief Fee page configuration type (for link-time configuration)
 */
typedef struct {
    Fee_AddressType PageStartAddress;   /**< Page start address */
    Fee_LengthType PageSize;            /**< Page size in bytes */
    uint8 PageNumber;                   /**< Page number */
} Fee_PageConfigType;

/**
 * @brief Fee job type enumeration
 */
typedef enum {
    FEE_JOB_READ = 0,                   /**< Read job */
    FEE_JOB_WRITE,                      /**< Write job */
    FEE_JOB_ERASE_IMMEDIATE,            /**< Erase immediate block job */
    FEE_JOB_GC_PAGE,                    /**< Garbage collection job */
    FEE_JOB_NONE                        /**< No job */
} Fee_JobType;

/**
 * @brief Fee internal state type (for state machine)
 */
typedef enum {
    FEE_STATE_IDLE = 0,                 /**< Idle state */
    FEE_STATE_READ_HEADER,              /**< Read block header state */
    FEE_STATE_READ_DATA,                /**< Read block data state */
    FEE_STATE_WRITE_HEADER,             /**< Write block header state */
    FEE_STATE_WRITE_DATA,               /**< Write block data state */
    FEE_STATE_ERASE_IMMEDIATE,          /**< Erase immediate block state */
    FEE_STATE_GC_COPY,                  /**< Garbage collection copy state */
    FEE_STATE_GC_ERASE                  /**< Garbage collection erase state */
} Fee_InternalStateType;

/**
 * @brief Fee configuration type
 */
typedef struct {
    const Fee_SectorType* sectorList;   /**< Pointer to sector configuration array */
    const Fee_BlockType* blockList;     /**< Pointer to block configuration array */
    uint32 sectorCount;                 /**< Number of configured sectors */
    uint32 blockCount;                  /**< Number of configured blocks */
    Fee_ModeType defaultMode;           /**< Default operation mode */
    uint32 virtualPageSize;             /**< Virtual page size for EEPROM emulation */
    uint32 maxReadNormalMode;           /**< Max read bytes in normal mode per cycle */
    uint32 maxReadFastMode;             /**< Max read bytes in fast mode per cycle */
    uint32 maxWriteNormalMode;          /**< Max write bytes in normal mode per cycle */
    uint32 maxWriteFastMode;            /**< Max write bytes in fast mode per cycle */
    boolean eraseSuspendSupport;        /**< Erase suspend/resume support */
    boolean writeVerifySupport;         /**< Write verification support */
    boolean compareSupport;             /**< Compare operation support */
    boolean blankCheckSupport;          /**< Blank check operation support */
    /* Link-time config additions */
    const Fee_PageConfigType* FeePageConfig;          /**< Page configuration */
    uint8 FeeNumberOfPages;                           /**< Number of pages */
    const Fee_BlockConfigType* FeeBlockConfig;        /**< Block configuration */
    uint16 FeeNumberOfBlocks;                         /**< Number of blocks */
    uint8 FeeGarbageCollectThreshold;                 /**< GC threshold */
    uint8 FeeGcRepetitions;                           /**< GC repetitions */
    boolean FeeNvmJobEndNotificationEnabled;          /**< NVM notification enabled */
    boolean FeeUseEraseSuspend;                       /**< Erase suspend support */
} Fee_ConfigType;

/*==================================================================================================
 *                                    GLOBAL CONFIG POINTER
 *==================================================================================================*/
#define FEE_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Fee_ConfigType Fee_Config;

#define FEE_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                    FUNCTION PROTOTYPES
 *==================================================================================================*/
#define FEE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Fee driver
 * @param ConfigPtr Pointer to configuration structure
 * @return Std_ReturnType
 *         - E_OK: Initialization successful
 *         - E_NOT_OK: Initialization failed
 * @req SWS_Fee_00001
 */
extern Std_ReturnType Fee_Init(const Fee_ConfigType* ConfigPtr);

/**
 * @brief De-initializes the Fee driver
 * @return Std_ReturnType
 *         - E_OK: De-initialization successful
 *         - E_NOT_OK: De-initialization failed
 * @req SWS_Fee_00002
 */
extern Std_ReturnType Fee_DeInit(void);

/**
 * @brief Sets the operation mode
 * @param Mode Mode to set (NORMAL/FAST)
 * @return Std_ReturnType
 *         - E_OK: Mode change successful
 *         - E_NOT_OK: Mode change failed
 * @req SWS_Fee_00003
 */
extern Std_ReturnType Fee_SetMode(Fee_ModeType Mode);

/**
 * @brief Reads data from flash
 * @param SourceAddress Source address in flash
 * @param Length Number of bytes to read
 * @param DestPtr Pointer to destination buffer
 * @return Std_ReturnType
 *         - E_OK: Read operation started successfully
 *         - E_NOT_OK: Read operation failed
 * @req SWS_Fee_00004
 */
extern Std_ReturnType Fee_Read(Fee_AddressType SourceAddress,
                                Fee_LengthType Length,
                                uint8* DestPtr);

/**
 * @brief Writes data to flash
 * @param TargetAddress Target address in flash
 * @param Length Number of bytes to write
 * @param SourcePtr Pointer to source data
 * @return Std_ReturnType
 *         - E_OK: Write operation started successfully
 *         - E_NOT_OK: Write operation failed
 * @req SWS_Fee_00005
 */
extern Std_ReturnType Fee_Write(Fee_AddressType TargetAddress,
                                 Fee_LengthType Length,
                                 const uint8* SourcePtr);

/**
 * @brief Erases flash sector(s)
 * @param TargetAddress Target address (must be sector aligned)
 * @param Length Number of bytes to erase
 * @return Std_ReturnType
 *         - E_OK: Erase operation started successfully
 *         - E_NOT_OK: Erase operation failed
 * @req SWS_Fee_00006
 */
extern Std_ReturnType Fee_Erase(Fee_AddressType TargetAddress,
                                 Fee_LengthType Length);

/**
 * @brief Compares flash data with buffer
 * @param SourceAddress Source address in flash
 * @param Length Number of bytes to compare
 * @param DataPtr Pointer to data to compare against
 * @return Std_ReturnType
 *         - E_OK: Compare operation started successfully
 *         - E_NOT_OK: Compare operation failed
 * @req SWS_Fee_00007
 */
extern Std_ReturnType Fee_Compare(Fee_AddressType SourceAddress,
                                   Fee_LengthType Length,
                                   const uint8* DataPtr);

/**
 * @brief Checks if flash area is blank (erased)
 * @param TargetAddress Target address in flash
 * @param Length Number of bytes to check
 * @return Std_ReturnType
 *         - E_OK: Blank check started successfully
 *         - E_NOT_OK: Blank check failed
 * @req SWS_Fee_00008
 */
extern Std_ReturnType Fee_BlankCheck(Fee_AddressType TargetAddress,
                                      Fee_LengthType Length);

/**
 * @brief Gets the driver status
 * @return Fee_StateType Current driver status
 * @req SWS_Fee_00009
 */
extern Fee_StateType Fee_GetStatus(void);

/**
 * @brief Gets the result of the last job
 * @return Fee_JobResultType Job result
 * @req SWS_Fee_00010
 */
extern Fee_JobResultType Fee_GetJobResult(void);

/**
 * @brief Cancels the ongoing job
 * @return Std_ReturnType
 *         - E_OK: Cancel successful
 *         - E_NOT_OK: Cancel failed
 * @req SWS_Fee_00011
 */
extern Std_ReturnType Fee_Cancel(void);

/**
 * @brief Suspends the ongoing erase operation
 * @return Std_ReturnType
 *         - E_OK: Suspend successful
 *         - E_NOT_OK: Suspend failed
 * @req SWS_Fee_00012
 */
extern Std_ReturnType Fee_Suspend(void);

/**
 * @brief Resumes a suspended erase operation
 * @return Std_ReturnType
 *         - E_OK: Resume successful
 *         - E_NOT_OK: Resume failed
 * @req SWS_Fee_00013
 */
extern Std_ReturnType Fee_Resume(void);

/**
 * @brief Gets version information
 * @param VersionInfoPtr Pointer to version info structure
 * @req SWS_Fee_00014
 */
#if (FEE_VERSION_INFO_API == STD_ON)
extern void Fee_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);
#endif

/**
 * @brief Main function for processing asynchronous jobs
 * @note Must be called periodically by the scheduler
 * @req SWS_Fee_00015
 */
extern void Fee_MainFunction(void);

/**
 * @brief Job end notification callback
 * @note Called by lower layer when job completes successfully
 */
extern void Fee_JobEndNotification(void);

/**
 * @brief Job error notification callback
 * @note Called by lower layer when job fails
 */
extern void Fee_JobErrorNotification(void);

/*==================================================================================================
 *                                    HELPER FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Get next state based on current state and job type
 * @param CurrentState Current state
 * @param JobType Job type
 * @return Fee_InternalStateType Next state
 */
extern Fee_InternalStateType Fee_GetNextState(Fee_InternalStateType CurrentState, Fee_JobType JobType);

/**
 * @brief Check if state transition is valid
 * @param CurrentState Current state
 * @param JobType Job type
 * @return boolean TRUE if valid, FALSE otherwise
 */
extern boolean Fee_IsStateTransitionValid(Fee_InternalStateType CurrentState, Fee_JobType JobType);

/**
 * @brief Update wear leveling counters
 * @param PageIndex Page index
 * @param Operation Operation type (0=erase, 1=write, 2=GC)
 */
extern void Fee_UpdateWearLeveling(uint8 PageIndex, uint8 Operation);

/**
 * @brief Get preferred page for garbage collection
 * @return uint8 Page index with lowest erase count
 */
extern uint8 Fee_GetPreferredPageForGc(void);

/**
 * @brief Get block configuration by block number
 * @param BlockNumber Block number
 * @return const Fee_BlockConfigType* Pointer to block config, NULL if not found
 */
extern const Fee_BlockConfigType* Fee_GetBlockConfig(uint16 BlockNumber);

/**
 * @brief Get page configuration by page number
 * @param PageNumber Page number
 * @return const Fee_PageConfigType* Pointer to page config, NULL if not found
 */
extern const Fee_PageConfigType* Fee_GetPageConfig(uint8 PageNumber);

#define FEE_STOP_SEC_CODE
#include "MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* FEE_MCAL_H */
