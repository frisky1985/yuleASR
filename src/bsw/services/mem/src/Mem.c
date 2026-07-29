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
 * FILENAME: Mem.c
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_MemoryServices.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Implementation of Memory Service module
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Mem.h"
#include "Det.h"
#include "SchM_Mem.h"

/*==================================================================================================
 *                                    VERSION CHECK
 *==================================================================================================*/
#if (MEM_AR_RELEASE_MAJOR_VERSION != 4u)
    #error "Mem.c: AR major version mismatch"
#endif

#if (MEM_AR_RELEASE_MINOR_VERSION != 7u)
    #error "Mem.c: AR minor version mismatch"
#endif

/*==================================================================================================
 *                                    LOCAL DEFINES
 *==================================================================================================*/
#define MEM_HEADER_SIZE                 (sizeof(Mem_BlockType))
#define MEM_ALIGN_MASK(align)           ((align) - 1u)
#define MEM_ALIGN_UP(size, align)       (((size) + MEM_ALIGN_MASK(align)) & ~MEM_ALIGN_MASK(align))
#define MEM_IS_POWER_OF_2(n)            (((n) & ((n) - 1u)) == 0u)

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/
typedef struct {
    uint8* poolBase;
    uint32 poolSize;
    Mem_BlockType* firstBlock;
    uint32 usedSize;
    uint32 numBlocks;
    boolean isInitialized;
} Mem_PoolRuntimeType;

/*==================================================================================================
 *                                    LOCAL VARIABLES
 *==================================================================================================*/
#define MEM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Mem_MemMap.h"

static Mem_PoolRuntimeType Mem_Pools[MEM_NUM_POOLS];
static uint32 Mem_NextHandle = 1u;
static boolean Mem_Initialized = FALSE;

/* Static memory pools */
static uint8 Mem_FastPool[MEM_FAST_POOL_SIZE];
static uint8 Mem_StandardPool[MEM_STANDARD_POOL_SIZE];
static uint8 Mem_LargePool[MEM_LARGE_POOL_SIZE];

#define MEM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Mem_MemMap.h"

/*==================================================================================================
 *                                    GLOBAL VARIABLES
 *==================================================================================================*/
#define MEM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Mem_MemMap.h"

Mem_StatusType Mem_Status = MEM_UNINIT;
const Mem_ConfigType* Mem_ConfigPtr = NULL_PTR;

#define MEM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Mem_MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/
#define MEM_START_SEC_CODE
#include "Mem_MemMap.h"

/**
 * @brief Calculate checksum for memory block
 */
static uint32 Mem_CalculateChecksum(const Mem_BlockType* block)
{
    uint32 checksum = 0u;
    const uint8* ptr = (const uint8*)block;
    uint32 i;
    
    for (i = 0u; i < offsetof(Mem_BlockType, checksum); i++) {
        checksum ^= ((uint32)ptr[i]) << ((i % 4u) * 8u);
    }
    return checksum;
}

/**
 * @brief Validate block integrity
 */
static boolean Mem_ValidateBlock(const Mem_BlockType* block)
{
    if (block == NULL_PTR) {
        return FALSE;
    }
    if (block->magic != MEM_MAGIC_NUMBER) {
        return FALSE;
    }
#if (MEM_ENABLE_CHECKSUM == STD_ON)
    if (block->checksum != Mem_CalculateChecksum(block)) {
        return FALSE;
    }
#endif
    return TRUE;
}

/**
 * @brief Update block checksum
 */
static void Mem_UpdateChecksum(Mem_BlockType* block)
{
#if (MEM_ENABLE_CHECKSUM == STD_ON)
    block->checksum = Mem_CalculateChecksum(block);
#else
    (void)block;
#endif
}

/**
 * @brief Initialize a memory pool
 */
static void Mem_InitPool(uint8 poolIndex, uint8* base, uint32 size, uint32 maxBlockSize)
{
    Mem_BlockType* block;
    
    Mem_Pools[poolIndex].poolBase = base;
    Mem_Pools[poolIndex].poolSize = size;
    Mem_Pools[poolIndex].usedSize = MEM_HEADER_SIZE;
    Mem_Pools[poolIndex].numBlocks = 1u;
    Mem_Pools[poolIndex].isInitialized = TRUE;
    
    /* Initialize first block as free block covering entire pool */
    block = (Mem_BlockType*)base;
    block->size = size;
    block->isFree = TRUE;
    block->handle = MEM_INVALID_HANDLE;
    block->next = NULL_PTR;
    block->prev = NULL_PTR;
    block->magic = MEM_MAGIC_NUMBER;
    Mem_UpdateChecksum(block);
    
    Mem_Pools[poolIndex].firstBlock = block;
}

/**
 * @brief Find free block using configured strategy
 */
static Mem_BlockType* Mem_FindFreeBlock(uint8 poolIndex, uint32 size, Mem_AllocStrategyType strategy)
{
    Mem_BlockType* current = Mem_Pools[poolIndex].firstBlock;
    Mem_BlockType* best = NULL_PTR;
    uint32 bestSize = 0xFFFFFFFFu;
    uint32 worstSize = 0u;
    
    while (current != NULL_PTR) {
        if (current->isFree && current->size >= size) {
            switch (strategy) {
                case MEM_ALLOC_FIRST_FIT:
                    return current;
                    
                case MEM_ALLOC_BEST_FIT:
                    if (current->size < bestSize) {
                        bestSize = current->size;
                        best = current;
                    }
                    break;
                    
                case MEM_ALLOC_WORST_FIT:
                    if (current->size > worstSize) {
                        worstSize = current->size;
                        best = current;
                    }
                    break;
                    
                default:
                    return current;
            }
        }
        current = current->next;
    }
    
    return best;
}

/**
 * @brief Split a block if large enough
 */
static void Mem_SplitBlock(Mem_BlockType* block, uint32 size)
{
    uint32 minBlockSize = MEM_HEADER_SIZE + MEM_MIN_BLOCK_SIZE;
    
    if (block->size >= (size + minBlockSize)) {
        Mem_BlockType* newBlock = (Mem_BlockType*)((uint8*)block + size);
        
        newBlock->size = block->size - size;
        newBlock->isFree = TRUE;
        newBlock->handle = MEM_INVALID_HANDLE;
        newBlock->next = block->next;
        newBlock->prev = block;
        newBlock->magic = MEM_MAGIC_NUMBER;
        Mem_UpdateChecksum(newBlock);
        
        if (block->next != NULL_PTR) {
            block->next->prev = newBlock;
            Mem_UpdateChecksum(block->next);
        }
        
        block->size = size;
        block->next = newBlock;
        Mem_UpdateChecksum(block);
        
        Mem_Pools[0].numBlocks++;
    }
}

/**
 * @brief Merge adjacent free blocks
 */
static void Mem_MergeFreeBlocks(Mem_BlockType* block)
{
    /* Merge with next block if free */
    if ((block->next != NULL_PTR) && block->next->isFree) {
        block->size += block->next->size;
        block->next = block->next->next;
        if (block->next != NULL_PTR) {
            block->next->prev = block;
            Mem_UpdateChecksum(block->next);
        }
        Mem_Pools[0].numBlocks--;
        Mem_UpdateChecksum(block);
    }
    
    /* Merge with previous block if free */
    if ((block->prev != NULL_PTR) && block->prev->isFree) {
        block->prev->size += block->size;
        block->prev->next = block->next;
        if (block->next != NULL_PTR) {
            block->next->prev = block->prev;
            Mem_UpdateChecksum(block->next);
        }
        Mem_Pools[0].numBlocks--;
        Mem_UpdateChecksum(block->prev);
    }
}

/**
 * @brief Get pool index for allocation size
 */
static uint8 Mem_GetPoolForSize(uint32 size)
{
    if (size <= MEM_FAST_POOL_MAX_BLOCK) {
        return 0u;  /* Fast pool */
    } else if (size <= MEM_STANDARD_POOL_MAX_BLOCK) {
        return 1u;  /* Standard pool */
    } else {
        return 2u;  /* Large pool */
    }
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initializes the Memory Service module
 */
void Mem_Init(const Mem_ConfigType* ConfigPtr)
{
    uint8 i;
    
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_Initialized == TRUE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_INIT, MEM_E_ALREADY_INITIALIZED);
        return;
    }
    
    if (ConfigPtr == NULL_PTR) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_INIT, MEM_E_PARAM_POINTER);
        return;
    }
#endif

    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0();
    
    /* Initialize pools */
    Mem_InitPool(0u, Mem_FastPool, MEM_FAST_POOL_SIZE, MEM_FAST_POOL_MAX_BLOCK);
    Mem_InitPool(1u, Mem_StandardPool, MEM_STANDARD_POOL_SIZE, MEM_STANDARD_POOL_MAX_BLOCK);
    Mem_InitPool(2u, Mem_LargePool, MEM_LARGE_POOL_SIZE, MEM_LARGE_POOL_MAX_BLOCK);
    
    Mem_ConfigPtr = ConfigPtr;
    Mem_Status = MEM_IDLE;
    Mem_Initialized = TRUE;
    Mem_NextHandle = 1u;
    
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0();
}

/**
 * @brief Deinitializes the Memory Service module
 */
void Mem_DeInit(void)
{
    uint8 i;
    
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_Initialized == FALSE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_DEINIT, MEM_E_UNINIT);
        return;
    }
#endif

    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0();
    
    /* Reset all pools */
    for (i = 0u; i < MEM_NUM_POOLS; i++) {
        Mem_Pools[i].isInitialized = FALSE;
        Mem_Pools[i].usedSize = 0u;
        Mem_Pools[i].numBlocks = 0u;
    }
    
    Mem_ConfigPtr = NULL_PTR;
    Mem_Status = MEM_UNINIT;
    Mem_Initialized = FALSE;
    
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0();
}

/**
 * @brief Gets version information
 */
#if (MEM_VERSION_INFO_API == STD_ON)
void Mem_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_GETVERSIONINFO, MEM_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = MEM_VENDOR_ID;
    versioninfo->moduleID = MEM_MODULE_ID;
    versioninfo->sw_major_version = MEM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = MEM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = MEM_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Allocates a memory block
 */
Mem_HandleType Mem_Allocate(uint32 Size, uint8 Alignment)
{
    Mem_HandleType handle = MEM_INVALID_HANDLE;
    uint32 totalSize;
    uint8 poolIndex;
    Mem_BlockType* block;
    Mem_AllocStrategyType strategy = MEM_ALLOC_FIRST_FIT;
    
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_Initialized == FALSE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_ALLOCATE, MEM_E_UNINIT);
        return MEM_INVALID_HANDLE;
    }
    
    if (Size == 0u) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_ALLOCATE, MEM_E_PARAM_SIZE);
        return MEM_INVALID_HANDLE;
    }
    
    if ((!MEM_IS_POWER_OF_2(Alignment)) || (Alignment < MEM_MIN_ALIGNMENT)) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_ALLOCATE, MEM_E_PARAM_ALIGN);
        return MEM_INVALID_HANDLE;
    }
#endif

    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0();
    
    /* Calculate total size including header */
    totalSize = MEM_ALIGN_UP(MEM_HEADER_SIZE + Size, Alignment);
    
    /* Select pool based on size */
    poolIndex = Mem_GetPoolForSize(totalSize);
    
    /* Find suitable block */
    block = Mem_FindFreeBlock(poolIndex, totalSize, strategy);
    
    if (block != NULL_PTR) {
        /* Split block if large enough */
        Mem_SplitBlock(block, totalSize);
        
        /* Mark as allocated */
        block->isFree = FALSE;
        block->handle = Mem_NextHandle++;
        Mem_UpdateChecksum(block);
        
        Mem_Pools[poolIndex].usedSize += block->size;
        handle = block->handle;
    }
    
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0();
    
    return handle;
}

/**
 * @brief Frees a previously allocated memory block
 */
Std_ReturnType Mem_Free(Mem_HandleType Handle)
{
    uint8 poolIndex;
    Mem_BlockType* block;
    Std_ReturnType result = E_NOT_OK;
    
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_Initialized == FALSE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_FREE, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (Handle == MEM_INVALID_HANDLE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_FREE, MEM_E_INVALID_HANDLE);
        return E_NOT_OK;
    }
#endif

    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0();
    
    /* Search for block with matching handle in all pools */
    for (poolIndex = 0u; poolIndex < MEM_NUM_POOLS; poolIndex++) {
        block = Mem_Pools[poolIndex].firstBlock;
        
        while (block != NULL_PTR) {
            if ((block->handle == Handle) && (!block->isFree)) {
                /* Validate block integrity */
                if (Mem_ValidateBlock(block)) {
                    /* Mark as free */
                    block->isFree = TRUE;
                    block->handle = MEM_INVALID_HANDLE;
                    Mem_UpdateChecksum(block);
                    
                    Mem_Pools[poolIndex].usedSize -= block->size;
                    
                    /* Merge with adjacent free blocks */
                    Mem_MergeFreeBlocks(block);
                    
                    result = E_OK;
                }
                break;
            }
            block = block->next;
        }
        
        if (result == E_OK) {
            break;
        }
    }
    
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (result != E_OK) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_FREE, MEM_E_INVALID_HANDLE);
    }
#endif

    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Reallocates a memory block with new size
 */
Mem_HandleType Mem_Reallocate(Mem_HandleType Handle, uint32 NewSize)
{
    Mem_HandleType newHandle = MEM_INVALID_HANDLE;
    void* oldPtr = NULL_PTR;
    void* newPtr = NULL_PTR;
    uint32 oldSize = 0u;
    uint8 poolIndex;
    Mem_BlockType* block;
    
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_Initialized == FALSE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_REALLOCATE, MEM_E_UNINIT);
        return MEM_INVALID_HANDLE;
    }
    
    if (Handle == MEM_INVALID_HANDLE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_REALLOCATE, MEM_E_INVALID_HANDLE);
        return MEM_INVALID_HANDLE;
    }
    
    if (NewSize == 0u) {
        (void)Mem_Free(Handle);
        return MEM_INVALID_HANDLE;
    }
#endif

    /* Find original block */
    for (poolIndex = 0u; poolIndex < MEM_NUM_POOLS; poolIndex++) {
        block = Mem_Pools[poolIndex].firstBlock;
        
        while (block != NULL_PTR) {
            if ((block->handle == Handle) && (!block->isFree)) {
                if (Mem_ValidateBlock(block)) {
                    oldSize = block->size - MEM_HEADER_SIZE;
                    oldPtr = (void*)((uint8*)block + MEM_HEADER_SIZE);
                }
                break;
            }
            block = block->next;
        }
        
        if (oldPtr != NULL_PTR) {
            break;
        }
    }
    
    if (oldPtr == NULL_PTR) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_REALLOCATE, MEM_E_INVALID_HANDLE);
#endif
        return MEM_INVALID_HANDLE;
    }
    
    /* Allocate new block */
    newHandle = Mem_Allocate(NewSize, MEM_DEFAULT_ALIGNMENT);
    
    if (newHandle != MEM_INVALID_HANDLE) {
        newPtr = Mem_GetPointer(newHandle);
        
        /* Copy data */
        if (oldSize > 0u) {
            uint32 copySize = (oldSize < NewSize) ? oldSize : NewSize;
            uint8* src = (uint8*)oldPtr;
            uint8* dst = (uint8*)newPtr;
            uint32 i;
            
            for (i = 0u; i < copySize; i++) {
                dst[i] = src[i];
            }
        }
        
        /* Free old block */
        (void)Mem_Free(Handle);
    }
    
    return newHandle;
}

/**
 * @brief Gets pointer from memory handle
 */
void* Mem_GetPointer(Mem_HandleType Handle)
{
    uint8 poolIndex;
    Mem_BlockType* block;
    void* ptr = NULL_PTR;
    
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_Initialized == FALSE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, 0x15u, MEM_E_UNINIT);
        return NULL_PTR;
    }
#endif

    if (Handle == MEM_INVALID_HANDLE) {
        return NULL_PTR;
    }

    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0();
    
    for (poolIndex = 0u; poolIndex < MEM_NUM_POOLS; poolIndex++) {
        block = Mem_Pools[poolIndex].firstBlock;
        
        while (block != NULL_PTR) {
            if ((block->handle == Handle) && (!block->isFree)) {
                if (Mem_ValidateBlock(block)) {
                    ptr = (void*)((uint8*)block + MEM_HEADER_SIZE);
                }
                break;
            }
            block = block->next;
        }
        
        if (ptr != NULL_PTR) {
            break;
        }
    }
    
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0();
    
    return ptr;
}

/**
 * @brief Gets current module status
 */
Mem_StatusType Mem_GetStatus(void)
{
    return Mem_Status;
}

/**
 * @brief Gets memory information
 */
Std_ReturnType Mem_GetMemInfo(uint8 PoolIndex, Mem_InfoType* InfoPtr)
{
    Mem_BlockType* block;
    uint32 freeSize = 0u;
    uint32 maxFreeBlock = 0u;
    uint32 numFragments = 0u;
    
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_Initialized == FALSE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_GETMEMINFO, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (InfoPtr == NULL_PTR) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_GETMEMINFO, MEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (PoolIndex >= MEM_NUM_POOLS) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_GETMEMINFO, MEM_E_PARAM_SIZE);
        return E_NOT_OK;
    }
#endif

    if ((!Mem_Pools[PoolIndex].isInitialized) || (InfoPtr == NULL_PTR)) {
        return E_NOT_OK;
    }

    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0();
    
    block = Mem_Pools[PoolIndex].firstBlock;
    
    while (block != NULL_PTR) {
        if (block->isFree) {
            freeSize += block->size;
            numFragments++;
            if (block->size > maxFreeBlock) {
                maxFreeBlock = block->size;
            }
        }
        block = block->next;
    }
    
    InfoPtr->totalSize = Mem_Pools[PoolIndex].poolSize;
    InfoPtr->freeSize = freeSize;
    InfoPtr->usedSize = Mem_Pools[PoolIndex].usedSize;
    InfoPtr->maxFreeBlock = maxFreeBlock;
    InfoPtr->numAllocations = Mem_Pools[PoolIndex].numBlocks - (uint32)numFragments;
    InfoPtr->numFragments = numFragments;
    InfoPtr->fragmentationRatio = (uint8)((numFragments * 100u) / Mem_Pools[PoolIndex].numBlocks);
    
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0();
    
    return E_OK;
}

/**
 * @brief Main function for periodic processing
 */
void Mem_MainFunction(void)
{
    uint8 poolIndex;
    
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_Initialized == FALSE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_MAINFUNCTION, MEM_E_UNINIT);
        return;
    }
#endif

#if (MEM_ENABLE_MONITORING == STD_ON)
    (void)Mem_CheckIntegrity();
    
    /* Check fragmentation levels */
    for (poolIndex = 0u; poolIndex < MEM_NUM_POOLS; poolIndex++) {
        Mem_InfoType info;
        if (Mem_GetMemInfo(poolIndex, &info) == E_OK) {
            if (info.fragmentationRatio > MEM_DEFRAG_THRESHOLD) {
                (void)Mem_Defragment(poolIndex);
            }
        }
    }
#endif
}

/**
 * @brief Checks memory integrity
 */
Std_ReturnType Mem_CheckIntegrity(void)
{
    uint8 poolIndex;
    Mem_BlockType* block;
    Std_ReturnType result = E_OK;
    
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_Initialized == FALSE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, 0x40u, MEM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0();
    
    for (poolIndex = 0u; poolIndex < MEM_NUM_POOLS; poolIndex++) {
        if (!Mem_Pools[poolIndex].isInitialized) {
            continue;
        }
        
        block = Mem_Pools[poolIndex].firstBlock;
        
        while (block != NULL_PTR) {
            if (!Mem_ValidateBlock(block)) {
                result = E_NOT_OK;
                /* Report runtime error */
                (void)Det_ReportRuntimeError(MEM_MODULE_ID, MEM_INSTANCE_ID, 0x40u, MEM_E_MEM_CORRUPTED);
                break;
            }
            block = block->next;
        }
        
        if (result != E_OK) {
            break;
        }
    }
    
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Defragments memory pool
 */
Std_ReturnType Mem_Defragment(uint8 PoolIndex)
{
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_Initialized == FALSE) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, 0x41u, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (PoolIndex >= MEM_NUM_POOLS) {
        (void)Det_ReportError(MEM_MODULE_ID, MEM_INSTANCE_ID, 0x41u, MEM_E_PARAM_SIZE);
        return E_NOT_OK;
    }
#endif

    /* Defragmentation is done incrementally during free operations */
    /* This function can be used for more aggressive defragmentation */
    
    return E_OK;
}

#define MEM_STOP_SEC_CODE
#include "Mem_MemMap.h"
