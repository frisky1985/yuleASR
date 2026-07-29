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

/*==================================================================================================
 *                                      MEMORY SERVICE (Mem)
 *==================================================================================================
 * FILENAME: Mem.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_MemoryServices.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Public header file for Memory Service module
 *==================================================================================================
 */

#ifndef MEM_H
#define MEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "Mem_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define MEM_VENDOR_ID                   (100u)
#define MEM_MODULE_ID                   (88u)
#define MEM_INSTANCE_ID                 (0u)

#define MEM_AR_RELEASE_MAJOR_VERSION    (4u)
#define MEM_AR_RELEASE_MINOR_VERSION    (7u)
#define MEM_AR_RELEASE_REVISION_VERSION (0u)

#define MEM_SW_MAJOR_VERSION            (1u)
#define MEM_SW_MINOR_VERSION            (0u)
#define MEM_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    FILE VERSION CHECKS
 *==================================================================================================*/
#ifndef DISABLE_MCAL_INTERMODULE_ASR_CHECK
    #if ((MEM_AR_RELEASE_MAJOR_VERSION != STD_TYPES_AR_RELEASE_MAJOR_VERSION) || \
         (MEM_AR_RELEASE_MINOR_VERSION != STD_TYPES_AR_RELEASE_MINOR_VERSION))
        #error "AutoSAR Version Numbers of Mem.h and Std_Types.h are different"
    #endif
#endif

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define MEM_SID_INIT                    (0x00u)
#define MEM_SID_DEINIT                  (0x01u)
#define MEM_SID_GETVERSIONINFO          (0x02u)
#define MEM_SID_ALLOCATE                (0x10u)
#define MEM_SID_FREE                    (0x11u)
#define MEM_SID_REALLOCATE              (0x12u)
#define MEM_SID_GETMEMSTATUS            (0x13u)
#define MEM_SID_GETMEMINFO              (0x14u)
#define MEM_SID_MAINFUNCTION            (0x20u)

/*==================================================================================================
 *                                    ERROR CODES
 *==================================================================================================*/
/* Development error codes */
#define MEM_E_PARAM_POINTER             (0x01u)  /* API called with NULL pointer */
#define MEM_E_PARAM_SIZE                (0x02u)  /* Invalid size parameter */
#define MEM_E_UNINIT                    (0x03u)  /* API called before initialization */
#define MEM_E_ALREADY_INITIALIZED       (0x04u)  /* Multiple initialization call */
#define MEM_E_ALLOC_FAILED              (0x05u)  /* Memory allocation failed */
#define MEM_E_INVALID_HANDLE            (0x06u)  /* Invalid memory handle */
#define MEM_E_PARAM_ALIGN               (0x07u)  /* Invalid alignment parameter */

/* Runtime error codes */
#define MEM_E_MEM_CORRUPTED             (0x01u)  /* Memory corruption detected */
#define MEM_E_OUT_OF_MEMORY             (0x02u)  /* Out of memory */
#define MEM_E_FRAGMENTATION             (0x03u)  /* Excessive fragmentation */

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/**
 * @brief Memory handle type - unique identifier for allocated memory blocks
 */
typedef uint32 Mem_HandleType;

/**
 * @brief Memory status type
 */
typedef enum {
    MEM_UNINIT = 0,         /* Module not initialized */
    MEM_IDLE,               /* Module initialized, ready */
    MEM_BUSY                /* Operation in progress */
} Mem_StatusType;

/**
 * @brief Memory allocation strategy type
 */
typedef enum {
    MEM_ALLOC_FIRST_FIT = 0,    /* First fit allocation */
    MEM_ALLOC_BEST_FIT,         /* Best fit allocation */
    MEM_ALLOC_WORST_FIT         /* Worst fit allocation */
} Mem_AllocStrategyType;

/**
 * @brief Memory pool type
 */
typedef enum {
    MEM_POOL_FAST = 0,      /* Fast access pool (small blocks) */
    MEM_POOL_STANDARD,      /* Standard pool (medium blocks) */
    MEM_POOL_LARGE          /* Large pool (big blocks) */
} Mem_PoolType;

/**
 * @brief Memory information structure
 */
typedef struct {
    uint32 totalSize;           /* Total memory size */
    uint32 freeSize;            /* Available memory size */
    uint32 usedSize;            /* Used memory size */
    uint32 maxFreeBlock;        /* Maximum contiguous free block */
    uint32 numAllocations;      /* Number of active allocations */
    uint32 numFragments;        /* Number of fragments */
    uint8 fragmentationRatio;   /* Fragmentation ratio (0-100%) */
} Mem_InfoType;

/**
 * @brief Memory block header (internal use)
 */
typedef struct Mem_BlockType {
    uint32 size;                    /* Block size including header */
    boolean isFree;                 /* Block is free flag */
    Mem_HandleType handle;          /* Block handle */
    struct Mem_BlockType* next;     /* Next block pointer */
    struct Mem_BlockType* prev;     /* Previous block pointer */
    uint32 magic;                   /* Magic number for corruption check */
    uint32 checksum;                /* Checksum for integrity */
} Mem_BlockType;

/**
 * @brief Memory pool configuration
 */
typedef struct {
    uint8* baseAddress;         /* Pool base address */
    uint32 poolSize;            /* Pool size in bytes */
    uint32 minBlockSize;        /* Minimum allocation size */
    uint32 maxBlockSize;        /* Maximum allocation size */
    uint8 alignment;            /* Required alignment (1, 2, 4, 8) */
    Mem_AllocStrategyType strategy; /* Allocation strategy */
} Mem_PoolConfigType;

/**
 * @brief Memory service configuration
 */
typedef struct {
    const Mem_PoolConfigType* pools;    /* Array of pool configurations */
    uint8 numPools;                     /* Number of pools */
    uint8 defragThreshold;              /* Defragmentation threshold (%) */
    boolean enableChecksum;             /* Enable integrity checksums */
    boolean enableMonitoring;           /* Enable memory monitoring */
} Mem_ConfigType;

/*==================================================================================================
 *                                    GLOBAL CONSTANTS
 *==================================================================================================*/
#define MEM_INVALID_HANDLE              (0xFFFFFFFFu)
#define MEM_MIN_ALIGNMENT               (4u)
#define MEM_MAGIC_NUMBER                (0x4D454D21u)   /* "MEM!" */

/*==================================================================================================
 *                                    GLOBAL VARIABLES (extern)
 *==================================================================================================*/
#define MEM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Mem_MemMap.h"

extern Mem_StatusType Mem_Status;
extern const Mem_ConfigType* Mem_ConfigPtr;

#define MEM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Mem_MemMap.h"

/*==================================================================================================
 *                                     API DECLARATIONS
 *==================================================================================================*/
#define MEM_START_SEC_CODE
#include "Mem_MemMap.h"

/**
 * @brief Initializes the Memory Service module
 * @param ConfigPtr Pointer to configuration structure
 * @return None
 * @req SWS_Mem_00001
 */
extern void Mem_Init(const Mem_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the Memory Service module
 * @return None
 * @req SWS_Mem_00002
 */
extern void Mem_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @return None
 * @req SWS_Mem_00003
 */
#if (MEM_VERSION_INFO_API == STD_ON)
extern void Mem_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Allocates a memory block
 * @param Size Size of memory to allocate in bytes
 * @param Alignment Required alignment (power of 2, >= MEM_MIN_ALIGNMENT)
 * @return Handle to allocated memory or MEM_INVALID_HANDLE
 * @req SWS_Mem_00010
 */
extern Mem_HandleType Mem_Allocate(uint32 Size, uint8 Alignment);

/**
 * @brief Frees a previously allocated memory block
 * @param Handle Memory handle returned by Mem_Allocate
 * @return E_OK if successful, E_NOT_OK if invalid handle
 * @req SWS_Mem_00011
 */
extern Std_ReturnType Mem_Free(Mem_HandleType Handle);

/**
 * @brief Reallocates a memory block with new size
 * @param Handle Existing memory handle
 * @param NewSize New size in bytes
 * @return Handle to reallocated memory or MEM_INVALID_HANDLE
 * @req SWS_Mem_00012
 */
extern Mem_HandleType Mem_Reallocate(Mem_HandleType Handle, uint32 NewSize);

/**
 * @brief Gets pointer from memory handle
 * @param Handle Memory handle
 * @return Pointer to memory or NULL if invalid
 * @req SWS_Mem_00013
 */
extern void* Mem_GetPointer(Mem_HandleType Handle);

/**
 * @brief Gets current module status
 * @return Current status
 * @req SWS_Mem_00020
 */
extern Mem_StatusType Mem_GetStatus(void);

/**
 * @brief Gets memory information
 * @param PoolIndex Pool index (0 = default pool)
 * @param InfoPtr Pointer to info structure to fill
 * @return E_OK if successful, E_NOT_OK if invalid pool
 * @req SWS_Mem_00021
 */
extern Std_ReturnType Mem_GetMemInfo(uint8 PoolIndex, Mem_InfoType* InfoPtr);

/**
 * @brief Main function for periodic processing
 * @return None
 * @req SWS_Mem_00030
 */
extern void Mem_MainFunction(void);

/**
 * @brief Checks memory integrity
 * @return E_OK if integrity OK, E_NOT_OK if corruption detected
 * @req SWS_Mem_00040
 */
extern Std_ReturnType Mem_CheckIntegrity(void);

/**
 * @brief Defragments memory pool
 * @param PoolIndex Pool index to defragment
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_Mem_00041
 */
extern Std_ReturnType Mem_Defragment(uint8 PoolIndex);

#define MEM_STOP_SEC_CODE
#include "Mem_MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* MEM_H */
