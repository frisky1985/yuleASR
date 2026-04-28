/**
 * @file Swc_StorageManager.h
 * @brief Storage Manager Software Component Header
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Non-volatile data storage management at application layer
 */

#ifndef SWC_STORAGEMANAGER_H
#define SWC_STORAGEMANAGER_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte_Swc.h"
#include "Std_Types.h"

/*==================================================================================================
*                                    COMPONENT TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief Storage block state
 */
typedef enum {
    STORAGE_BLOCK_EMPTY = 0,
    STORAGE_BLOCK_VALID,
    STORAGE_BLOCK_INVALID,
    STORAGE_BLOCK_INCONSISTENT,
    STORAGE_BLOCK_WRITING
} Swc_StorageBlockStateType;

/**
 * @brief Storage operation result
 */
typedef enum {
    STORAGE_RESULT_OK = 0,
    STORAGE_RESULT_NOT_OK,
    STORAGE_RESULT_PENDING,
    STORAGE_RESULT_INVALID_BLOCK,
    STORAGE_RESULT_INVALID_DATA,
    STORAGE_RESULT_WRITE_PROTECTED,
    STORAGE_RESULT_MEMORY_FULL
} Swc_StorageResultType;

/**
 * @brief Storage block configuration
 */
typedef struct {
    uint16 blockId;
    uint16 blockSize;
    uint8 deviceId;
    boolean writeCycleCounter;
    boolean immediateWrite;
    uint32 writeFrequency;
} Swc_StorageBlockConfigType;

/**
 * @brief Storage block status
 */
typedef struct {
    uint16 blockId;
    Swc_StorageBlockStateType state;
    uint32 writeCycleCounter;
    uint32 lastWriteTime;
    uint16 dataLength;
    uint16 crc;
} Swc_StorageBlockStatusType;

/**
 * @brief Storage statistics
 */
typedef struct {
    uint32 readOperations;
    uint32 writeOperations;
    uint32 eraseOperations;
    uint32 readErrors;
    uint32 writeErrors;
    uint32 memoryUsed;
    uint32 memoryTotal;
} Swc_StorageStatisticsType;

/*==================================================================================================
*                                    RUNNABLE IDS
==================================================================================================*/
#define RTE_RUNNABLE_StorageManager_Init             0x41
#define RTE_RUNNABLE_StorageManager_100ms            0x42
#define RTE_RUNNABLE_StorageManager_WriteCycle       0x43

/*==================================================================================================
*                                    PORT IDS
==================================================================================================*/
#define SWC_STORAGEMANAGER_PORT_STORAGE_STATE_P      0x41
#define SWC_STORAGEMANAGER_PORT_BLOCK_STATUS_P       0x42
#define SWC_STORAGEMANAGER_PORT_NVM_REQUEST_P        0x43
#define SWC_STORAGEMANAGER_PORT_NVM_RESULT_R         0x44

/*==================================================================================================
*                                    COMPONENT API
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Storage Manager component
 */
extern void Swc_StorageManager_Init(void);

/**
 * @brief 100ms cyclic runnable - storage management
 */
extern void Swc_StorageManager_100ms(void);

/**
 * @brief Write cycle management runnable
 */
extern void Swc_StorageManager_WriteCycle(void);

/**
 * @brief Reads data from storage block
 * @param blockId Block ID
 * @param data Pointer to store data
 * @param length Data length
 * @return Storage result
 */
extern Swc_StorageResultType Swc_StorageManager_ReadBlock(uint16 blockId,
                                                           void* data,
                                                           uint16 length);

/**
 * @brief Writes data to storage block
 * @param blockId Block ID
 * @param data Data to write
 * @param length Data length
 * @return Storage result
 */
extern Swc_StorageResultType Swc_StorageManager_WriteBlock(uint16 blockId,
                                                            const void* data,
                                                            uint16 length);

/**
 * @brief Gets block status
 * @param blockId Block ID
 * @param status Pointer to store status
 * @return Storage result
 */
extern Swc_StorageResultType Swc_StorageManager_GetBlockStatus(
    uint16 blockId,
    Swc_StorageBlockStatusType* status);

/**
 * @brief Invalidates storage block
 * @param blockId Block ID
 * @return Storage result
 */
extern Swc_StorageResultType Swc_StorageManager_InvalidateBlock(uint16 blockId);

/**
 * @brief Erases storage block
 * @param blockId Block ID
 * @return Storage result
 */
extern Swc_StorageResultType Swc_StorageManager_EraseBlock(uint16 blockId);

/**
 * @brief Gets storage statistics
 * @param stats Pointer to store statistics
 * @return Storage result
 */
extern Swc_StorageResultType Swc_StorageManager_GetStatistics(
    Swc_StorageStatisticsType* stats);

/**
 * @brief Sets write protection
 * @param blockId Block ID
 * @param protected TRUE to protect, FALSE to unprotect
 * @return Storage result
 */
extern Swc_StorageResultType Swc_StorageManager_SetWriteProtection(uint16 blockId,
                                                                    boolean protected);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE INTERFACE MACROS
==================================================================================================*/
#define Rte_Write_StorageState(data) \
    Rte_Write_SWC_STORAGEMANAGER_PORT_STORAGE_STATE_P(data)

#define Rte_Write_BlockStatus(data) \
    Rte_Write_SWC_STORAGEMANAGER_PORT_BLOCK_STATUS_P(data)

#define Rte_Write_NvmRequest(data) \
    Rte_Write_SWC_STORAGEMANAGER_PORT_NVM_REQUEST_P(data)

#define Rte_Read_NvmResult(data) \
    Rte_Read_SWC_STORAGEMANAGER_PORT_NVM_RESULT_R(data)

#endif /* SWC_STORAGEMANAGER_H */
