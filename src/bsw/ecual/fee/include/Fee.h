/*==================================================================================================
 *                                      FEE DRIVER
 *==================================================================================================
 * FILENAME: Fee.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_FlashEEPROMEmulation.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Public header file for Flash EEPROM Emulation module
 *==================================================================================================
 */

#ifndef FEE_H
#define FEE_H

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
#define FEE_SID_SETMODE                 (0x01u)
#define FEE_SID_READ                    (0x02u)
#define FEE_SID_WRITE                   (0x03u)
#define FEE_SID_CANCEL                  (0x04u)
#define FEE_SID_GETSTATUS               (0x05u)
#define FEE_SID_GETJOBRESULT            (0x06u)
#define FEE_SID_INVALIDATEBLOCK         (0x07u)
#define FEE_SID_ERASEIMMEDIATEBLOCK     (0x08u)
#define FEE_SID_JOBENDNOTIFICATION      (0x09u)
#define FEE_SID_JOBERRORNOTIFICATION    (0x0Au)
#define FEE_SID_GETVERSIONINFO          (0x0Bu)
#define FEE_SID_GETCYCLECOUNT           (0x0Cu)
#define FEE_SID_GETERASECYCLECOUNT      (0x0Du)
#define FEE_SID_GETWRITECYCLECOUNT      (0x0Eu)
#define FEE_SID_GETVENDORINFO           (0x0Fu)
#define FEE_SID_MAINFUNCTION            (0x12u)
#define FEE_SID_READIMMEDIATE           (0x13u)
#define FEE_SID_WRITEIMMEDIATE          (0x14u)

/*==================================================================================================
 *                                    DET ERROR CODES
 *==================================================================================================*/
/* Development error codes */
#define FEE_E_UNINIT                    (0x01u)
#define FEE_E_INVALID_BLOCK_NO          (0x02u)
#define FEE_E_INVALID_BLOCK_OFS         (0x03u)
#define FEE_E_INVALID_DATA_PTR          (0x04u)
#define FEE_E_INVALID_BLOCK_LEN         (0x05u)
#define FEE_E_BUSY                      (0x06u)
#define FEE_E_BUSY_INTERNAL             (0x07u)
#define FEE_E_INVALID_CANCEL            (0x08u)
#define FEE_E_GC_BUSY                   (0x09u)
#define FEE_E_GC_READ                   (0x0Au)
#define FEE_E_GC_WRITE                  (0x0Bu)
#define FEE_E_GC_ERASE                  (0x0Cu)
#define FEE_E_INVALID_SUSPEND           (0x0Du)
#define FEE_E_INVALID_RESUME            (0x0Eu)
#define FEE_E_INVALID_MODE              (0x0Fu)
#define FEE_E_INVALID_CFG               (0x10u)
#define FEE_E_NOTIFICATION              (0x11u)
#define FEE_E_INVALID_POLLING           (0x12u)
#define FEE_E_PARAM_POINTER             (0x13u)
#define FEE_E_PARAM_CONFIG              (0x14u)

/*==================================================================================================
 *                                    FEE STATUS TYPE
 *==================================================================================================*/
typedef enum {
    FEE_IDLE = 0,
    FEE_BUSY,
    FEE_BUSY_INTERNAL,
    FEE_CANCELLED
} Fee_StatusType;

/*==================================================================================================
 *                                    FEE JOB RESULT TYPE
 *==================================================================================================*/
typedef enum {
    FEE_JOB_OK = 0,
    FEE_JOB_FAILED,
    FEE_JOB_PENDING,
    FEE_JOB_CANCELLED,
    FEE_BLOCK_INCONSISTENT,
    FEE_BLOCK_INVALID
} Fee_JobResultType;

/*==================================================================================================
 *                                    FEE MODE TYPE
 *==================================================================================================*/
typedef enum {
    FEE_MODE_SLOW = 0,
    FEE_MODE_FAST
} Fee_ModeType;

/*==================================================================================================
 *                                    FEE BLOCK ID TYPE
 *==================================================================================================*/
typedef uint16 Fee_BlockIdType;

/*==================================================================================================
 *                                    FEE ADDRESS TYPE
 *==================================================================================================*/
typedef uint32 Fee_AddressType;

/*==================================================================================================
 *                                    FEE LENGTH TYPE
 *==================================================================================================*/
typedef uint32 Fee_LengthType;

/*==================================================================================================
 *                                    FEE BLOCK CONFIG TYPE
 *==================================================================================================*/
typedef struct {
    Fee_BlockIdType BlockId;
    uint16 BlockSize;
    uint16 ImmediateData;
    uint32 NumberOfWriteCycles;
    boolean BlockCrc;
    boolean BlockCrcType;
    boolean ImmediateDataEnabled;
    const uint8* RomBlockData;
} Fee_BlockConfigType;

/*==================================================================================================
 *                                    FEE SECTOR CONFIG TYPE
 *==================================================================================================*/
typedef struct {
    Fee_AddressType SectorStartAddress;
    Fee_LengthType SectorSize;
    uint32 SectorEraseCycleCount;
    boolean SectorIsValid;
} Fee_SectorConfigType;

/*==================================================================================================
 *                                    FEE CONFIG TYPE
 *==================================================================================================*/
typedef struct {
    const Fee_BlockConfigType* BlockConfig;
    const Fee_SectorConfigType* SectorConfig;
    uint16 NumBlocks;
    uint8 NumSectors;
    uint32 FeeVirtualPageSize;
    uint32 FeeMaximumBlockingTime;
    uint32 FeeMaxGcCycles;
    uint32 FeeMaxGcErases;
    uint32 FeeMaxWriteCycles;
    boolean FeeNvmJobEndNotification;
    boolean FeeNvmJobErrorNotification;
    boolean FeeUseEraseSuspend;
    boolean FeePollMode;
    boolean FeeSetModeSupported;
    boolean FeeVersionInfoApi;
    boolean FeeDevErrorDetect;
} Fee_ConfigType;

/*==================================================================================================
 *                                    CALLBACK TYPES
 *==================================================================================================*/
typedef void (*Fee_JobEndNotificationType)(void);
typedef void (*Fee_JobErrorNotificationType)(void);

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
 * @brief Initializes the Flash EEPROM Emulation module
 * @param ConfigPtr Pointer to configuration structure
 * @req SWS_Fee_00153
 */
extern void Fee_Init(const Fee_ConfigType* ConfigPtr);

/**
 * @brief De-initializes the Flash EEPROM Emulation module
 * @req SWS_Fee_00154
 */
extern void Fee_DeInit(void);

/**
 * @brief Sets the operation mode
 * @param Mode Mode to set (SLOW/FAST)
 * @req SWS_Fee_00155
 */
extern void Fee_SetMode(Fee_ModeType Mode);

/**
 * @brief Reads data from a block
 * @param BlockNumber Block number
 * @param BlockOffset Block offset
 * @param DataBufferPtr Data buffer pointer
 * @param Length Data length
 * @return Result of operation
 * @req SWS_Fee_00156
 */
extern Std_ReturnType Fee_Read(Fee_BlockIdType BlockNumber,
                                uint16 BlockOffset,
                                uint8* DataBufferPtr,
                                uint16 Length);

/**
 * @brief Writes data to a block
 * @param BlockNumber Block number
 * @param DataBufferPtr Data buffer pointer
 * @return Result of operation
 * @req SWS_Fee_00157
 */
extern Std_ReturnType Fee_Write(Fee_BlockIdType BlockNumber, const uint8* DataBufferPtr);

/**
 * @brief Cancels ongoing operation
 * @req SWS_Fee_00158
 */
extern void Fee_Cancel(void);

/**
 * @brief Gets module status
 * @return Module status
 * @req SWS_Fee_00159
 */
extern Fee_StatusType Fee_GetStatus(void);

/**
 * @brief Gets job result
 * @return Job result
 * @req SWS_Fee_00160
 */
extern Fee_JobResultType Fee_GetJobResult(void);

/**
 * @brief Invalidates a block
 * @param BlockNumber Block number
 * @return Result of operation
 * @req SWS_Fee_00161
 */
extern Std_ReturnType Fee_InvalidateBlock(Fee_BlockIdType BlockNumber);

/**
 * @brief Erases immediate block
 * @param BlockNumber Block number
 * @return Result of operation
 * @req SWS_Fee_00162
 */
extern Std_ReturnType Fee_EraseImmediateBlock(Fee_BlockIdType BlockNumber);

/**
 * @brief Job end notification callback
 * @req SWS_Fee_00163
 */
extern void Fee_JobEndNotification(void);

/**
 * @brief Job error notification callback
 * @req SWS_Fee_00164
 */
extern void Fee_JobErrorNotification(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @req SWS_Fee_00165
 */
#if (FEE_VERSION_INFO_API == STD_ON)
extern void Fee_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Gets cycle count
 * @return Cycle count
 */
extern uint32 Fee_GetCycleCount(void);

/**
 * @brief Gets erase cycle count
 * @return Erase cycle count
 */
extern uint32 Fee_GetEraseCycleCount(void);

/**
 * @brief Gets write cycle count
 * @return Write cycle count
 */
extern uint32 Fee_GetWriteCycleCount(void);

/**
 * @brief Main function for periodic processing
 * @req SWS_Fee_00169
 */
extern void Fee_MainFunction(void);

/**
 * @brief Internal function to process Fee jobs via Fls
 * @note This function is called by Fee_MainFunction to execute flash operations
 */
extern void Fee_ProcessFlsJob(void);

/**
 * @brief Callback function for Fls job end notification
 * @note This function is called by Fls when a job completes successfully
 */
extern void Fee_FlsJobEndNotification(void);

/**
 * @brief Callback function for Fls job error notification
 * @note This function is called by Fls when a job fails
 */
extern void Fee_FlsJobErrorNotification(void);

#define FEE_STOP_SEC_CODE
#include "MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* FEE_H */
