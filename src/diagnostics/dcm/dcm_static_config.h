/******************************************************************************
 * @file    dcm_static_config.h
 * @brief   DCM Static Memory Allocation Configuration
 *
 * Provides compile-time static memory allocation for DCM module.
 * Eliminates dynamic memory allocation for safety-critical applications.
 *
 * Features:
 * - All DCM objects pre-allocated at compile time
 * - Deterministic memory usage
 * - No heap fragmentation
 * - Suitable for ASIL-D applications
 *
 * AUTOSAR R22-11 compliant
 * MISRA C:2012 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_STATIC_CONFIG_H
#define DCM_STATIC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"
#include "dcm_compressed_types.h"
#include "dcm_priority_queue.h"
#include "dcm_memory.h"
#include "dcm_routine.h"
#include "dcm_dynamic_did.h"

/******************************************************************************
 * Static Allocation Configuration
 ******************************************************************************/

/* Enable static allocation */
#define DCM_USE_STATIC_ALLOCATION       1

/* Maximum counts for static arrays */
#define DCM_STATIC_MAX_CHANNELS         4U      /* Max diagnostic channels */
#define DCM_STATIC_MAX_SESSIONS         8U      /* Max session configs */
#define DCM_STATIC_MAX_SERVICES         32U     /* Max service handlers */
#define DCM_STATIC_MAX_SECURITY_LEVELS  8U      /* Max security levels */
#define DCM_STATIC_MAX_ROUTINES         16U     /* Max routines */
#define DCM_STATIC_MAX_DYNAMIC_DIDS     16U     /* Max dynamic DIDs */
#define DCM_STATIC_MAX_MEMORY_REGIONS   8U      /* Max memory regions */
#define DCM_STATIC_MAX_PQ_ENTRIES       32U     /* Max priority queue entries */
#define DCM_STATIC_MAX_CACHE_ENTRIES    16U     /* Max cache entries */

/* Buffer sizes */
#define DCM_STATIC_RX_BUFFER_SIZE       4096U   /* Per-channel RX buffer */
#define DCM_STATIC_TX_BUFFER_SIZE       4096U   /* Per-channel TX buffer */

/******************************************************************************
 * Static Memory Pools
 ******************************************************************************/

/* Channel buffers - statically allocated */
extern uint8_t g_dcmStaticRxBuffers[DCM_STATIC_MAX_CHANNELS][DCM_STATIC_RX_BUFFER_SIZE];
extern uint8_t g_dcmStaticTxBuffers[DCM_STATIC_MAX_CHANNELS][DCM_STATIC_TX_BUFFER_SIZE];

/* Channel structures */
#if defined(DCM_USE_COMPRESSED_TYPES)
    extern Dcm_CompChannelType g_dcmStaticChannels[DCM_STATIC_MAX_CHANNELS];
    #define DCM_CHANNEL_TYPE_STATIC Dcm_CompChannelType
#else
    extern Dcm_ChannelType g_dcmStaticChannels[DCM_STATIC_MAX_CHANNELS];
    #define DCM_CHANNEL_TYPE_STATIC Dcm_ChannelType
#endif

/* Session configurations */
#if defined(DCM_USE_COMPRESSED_TYPES)
    extern Dcm_CompSessionConfigType g_dcmStaticSessions[DCM_STATIC_MAX_SESSIONS];
    #define DCM_SESSION_TYPE_STATIC Dcm_CompSessionConfigType
#else
    extern Dcm_SessionConfigType g_dcmStaticSessions[DCM_STATIC_MAX_SESSIONS];
    #define DCM_SESSION_TYPE_STATIC Dcm_SessionConfigType
#endif

/* Service table */
extern Dcm_ServiceConfigType g_dcmStaticServiceTable[DCM_STATIC_MAX_SERVICES];

/* Security configurations */
extern Dcm_SecurityConfigType g_dcmStaticSecurityConfigs[DCM_STATIC_MAX_SECURITY_LEVELS];

/* Routine configurations */
extern Dcm_RoutineConfigType g_dcmStaticRoutines[DCM_STATIC_MAX_ROUTINES];

/* Dynamic DID storage */
extern Dcm_DynamicDidConfigType g_dcmStaticDynamicDids[DCM_STATIC_MAX_DYNAMIC_DIDS];

/* Memory regions */
extern Dcm_MemoryRegionConfigType g_dcmStaticMemoryRegions[DCM_STATIC_MAX_MEMORY_REGIONS];

/* Priority queue entries */
#if defined(DCM_USE_COMPRESSED_TYPES)
    extern Dcm_CompPqEntryType g_dcmStaticPqEntries[DCM_STATIC_MAX_PQ_ENTRIES];
    #define DCM_PQ_ENTRY_TYPE_STATIC Dcm_CompPqEntryType
#else
    extern Dcm_PqEntry g_dcmStaticPqEntries[DCM_STATIC_MAX_PQ_ENTRIES];
    #define DCM_PQ_ENTRY_TYPE_STATIC Dcm_PqEntry
#endif

/******************************************************************************
 * Static Context
 ******************************************************************************/

#if defined(DCM_USE_COMPRESSED_TYPES)
    extern Dcm_CompContextType g_dcmStaticContext;
    #define DCM_CONTEXT_TYPE_STATIC Dcm_CompContextType
#else
    extern Dcm_ContextType g_dcmStaticContext;
    #define DCM_CONTEXT_TYPE_STATIC Dcm_ContextType
#endif

/******************************************************************************
 * Memory Allocation Macros (Static Mode)
 ******************************************************************************/

#if defined(DCM_USE_STATIC_ALLOCATION)
    /* Static allocation - uses pre-allocated arrays */
    
    #define DCM_ALLOC_CHANNEL()         Dcm_StaticAllocChannel()
    #define DCM_ALLOC_SESSION()         Dcm_StaticAllocSession()
    #define DCM_ALLOC_SERVICE()         Dcm_StaticAllocService()
    #define DCM_ALLOC_SECURITY()        Dcm_StaticAllocSecurity()
    #define DCM_ALLOC_ROUTINE()         Dcm_StaticAllocRoutine()
    #define DCM_ALLOC_DYNAMIC_DID()     Dcm_StaticAllocDynamicDid()
    #define DCM_ALLOC_MEMORY_REGION()   Dcm_StaticAllocMemoryRegion()
    #define DCM_ALLOC_PQ_ENTRY()        Dcm_StaticAllocPqEntry()
    
    #define DCM_FREE_CHANNEL(ch)        Dcm_StaticFreeChannel(ch)
    #define DCM_FREE_SESSION(s)         Dcm_StaticFreeSession(s)
    #define DCM_FREE_SERVICE(s)         Dcm_StaticFreeService(s)
    #define DCM_FREE_SECURITY(s)        Dcm_StaticFreeSecurity(s)
    #define DCM_FREE_ROUTINE(r)         Dcm_StaticFreeRoutine(r)
    #define DCM_FREE_DYNAMIC_DID(d)     Dcm_StaticFreeDynamicDid(d)
    #define DCM_FREE_MEMORY_REGION(r)   Dcm_StaticFreeMemoryRegion(r)
    #define DCM_FREE_PQ_ENTRY(e)        Dcm_StaticFreePqEntry(e)
    
    #define DCM_GET_RX_BUFFER(ch)       g_dcmStaticRxBuffers[ch]
    #define DCM_GET_TX_BUFFER(ch)       g_dcmStaticTxBuffers[ch]
    
#else
    /* Dynamic allocation - uses heap */
    
    #define DCM_ALLOC_CHANNEL()         (Dcm_ChannelType *)DCM_ALLOC(sizeof(Dcm_ChannelType))
    #define DCM_ALLOC_SESSION()         (Dcm_SessionConfigType *)DCM_ALLOC(sizeof(Dcm_SessionConfigType))
    #define DCM_ALLOC_SERVICE()         (Dcm_ServiceConfigType *)DCM_ALLOC(sizeof(Dcm_ServiceConfigType))
    #define DCM_ALLOC_SECURITY()        (Dcm_SecurityConfigType *)DCM_ALLOC(sizeof(Dcm_SecurityConfigType))
    #define DCM_ALLOC_ROUTINE()         (Dcm_RoutineConfigType *)DCM_ALLOC(sizeof(Dcm_RoutineConfigType))
    #define DCM_ALLOC_DYNAMIC_DID()     (Dcm_DynamicDidConfigType *)DCM_ALLOC(sizeof(Dcm_DynamicDidConfigType))
    #define DCM_ALLOC_MEMORY_REGION()   (Dcm_MemoryRegionConfigType *)DCM_ALLOC(sizeof(Dcm_MemoryRegionConfigType))
    #define DCM_ALLOC_PQ_ENTRY()        (Dcm_PqEntry *)DCM_ALLOC(sizeof(Dcm_PqEntry))
    
    #define DCM_FREE_CHANNEL(ch)        DCM_FREE(ch)
    #define DCM_FREE_SESSION(s)         DCM_FREE(s)
    #define DCM_FREE_SERVICE(s)         DCM_FREE(s)
    #define DCM_FREE_SECURITY(s)        DCM_FREE(s)
    #define DCM_FREE_ROUTINE(r)         DCM_FREE(r)
    #define DCM_FREE_DYNAMIC_DID(d)     DCM_FREE(d)
    #define DCM_FREE_MEMORY_REGION(r)   DCM_FREE(r)
    #define DCM_FREE_PQ_ENTRY(e)        DCM_FREE(e)
    
    #define DCM_GET_RX_BUFFER(ch)       DCM_ALLOC(DCM_STATIC_RX_BUFFER_SIZE)
    #define DCM_GET_TX_BUFFER(ch)       DCM_ALLOC(DCM_STATIC_TX_BUFFER_SIZE)
    
#endif /* DCM_USE_STATIC_ALLOCATION */

/******************************************************************************
 * Static Allocation API
 ******************************************************************************/

#if defined(DCM_USE_STATIC_ALLOCATION)

/**
 * @brief Initialize static allocation system
 *
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_StaticInit(void);

/**
 * @brief Deinitialize static allocation system
 *
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_StaticDeInit(void);

/* Allocation functions */
DCM_CHANNEL_TYPE_STATIC* Dcm_StaticAllocChannel(void);
DCM_SESSION_TYPE_STATIC* Dcm_StaticAllocSession(void);
Dcm_ServiceConfigType* Dcm_StaticAllocService(void);
Dcm_SecurityConfigType* Dcm_StaticAllocSecurity(void);
Dcm_RoutineConfigType* Dcm_StaticAllocRoutine(void);
Dcm_DynamicDidConfigType* Dcm_StaticAllocDynamicDid(void);
Dcm_MemoryRegionConfigType* Dcm_StaticAllocMemoryRegion(void);
DCM_PQ_ENTRY_TYPE_STATIC* Dcm_StaticAllocPqEntry(void);

/* Free functions */
void Dcm_StaticFreeChannel(DCM_CHANNEL_TYPE_STATIC *channel);
void Dcm_StaticFreeSession(DCM_SESSION_TYPE_STATIC *session);
void Dcm_StaticFreeService(Dcm_ServiceConfigType *service);
void Dcm_StaticFreeSecurity(Dcm_SecurityConfigType *security);
void Dcm_StaticFreeRoutine(Dcm_RoutineConfigType *routine);
void Dcm_StaticFreeDynamicDid(Dcm_DynamicDidConfigType *did);
void Dcm_StaticFreeMemoryRegion(Dcm_MemoryRegionConfigType *region);
void Dcm_StaticFreePqEntry(DCM_PQ_ENTRY_TYPE_STATIC *entry);

/**
 * @brief Get static context
 *
 * @return DCM_CONTEXT_TYPE_STATIC* Pointer to static context
 */
DCM_CONTEXT_TYPE_STATIC* Dcm_StaticGetContext(void);

/**
 * @brief Get static channel array
 *
 * @return DCM_CHANNEL_TYPE_STATIC* Pointer to channel array
 */
DCM_CHANNEL_TYPE_STATIC* Dcm_StaticGetChannels(void);

/**
 * @brief Get number of allocated channels
 *
 * @return uint8_t Number of channels in use
 */
uint8_t Dcm_StaticGetChannelCount(void);

/**
 * @brief Get static allocation usage statistics
 *
 * @param totalBytes Total bytes statically allocated
 * @param usedBytes Bytes currently in use
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_StaticGetUsage(uint32_t *totalBytes, uint32_t *usedBytes);

/**
 * @brief Check if pointer is from static pool
 *
 * @param ptr Pointer to check
 * @return bool True if from static pool
 */
bool Dcm_StaticIsStaticPtr(const void *ptr);

#endif /* DCM_USE_STATIC_ALLOCATION */

/******************************************************************************
 * Static Memory Size Calculations
 ******************************************************************************/

/* Calculate total static memory usage */
#define DCM_STATIC_TOTAL_SIZE           \
    (sizeof(g_dcmStaticRxBuffers) +     \
     sizeof(g_dcmStaticTxBuffers) +     \
     sizeof(g_dcmStaticChannels) +      \
     sizeof(g_dcmStaticSessions) +      \
     sizeof(g_dcmStaticServiceTable) +  \
     sizeof(g_dcmStaticSecurityConfigs) + \
     sizeof(g_dcmStaticRoutines) +      \
     sizeof(g_dcmStaticDynamicDids) +   \
     sizeof(g_dcmStaticMemoryRegions) + \
     sizeof(g_dcmStaticPqEntries) +     \
     sizeof(g_dcmStaticContext))

/* Channel memory */
#define DCM_STATIC_CHANNEL_MEM          \
    (sizeof(g_dcmStaticChannels) +      \
     sizeof(g_dcmStaticRxBuffers) +     \
     sizeof(g_dcmStaticTxBuffers))

/* Control structures memory */
#define DCM_STATIC_CONTROL_MEM          \
    (sizeof(g_dcmStaticSessions) +      \
     sizeof(g_dcmStaticServiceTable) +  \
     sizeof(g_dcmStaticSecurityConfigs) + \
     sizeof(g_dcmStaticContext))

/* Dynamic objects memory */
#define DCM_STATIC_DYNAMIC_MEM          \
    (sizeof(g_dcmStaticRoutines) +      \
     sizeof(g_dcmStaticDynamicDids) +   \
     sizeof(g_dcmStaticMemoryRegions) + \
     sizeof(g_dcmStaticPqEntries))

/******************************************************************************
 * Configuration Validation
 ******************************************************************************/

/* Validate configuration at compile time */
#if (DCM_STATIC_MAX_CHANNELS == 0)
    #error "DCM_STATIC_MAX_CHANNELS must be at least 1"
#endif

#if (DCM_STATIC_MAX_SESSIONS == 0)
    #error "DCM_STATIC_MAX_SESSIONS must be at least 1"
#endif

#if (DCM_STATIC_MAX_SERVICES == 0)
    #error "DCM_STATIC_MAX_SERVICES must be at least 1"
#endif

#if (DCM_STATIC_RX_BUFFER_SIZE < 64)
    #error "DCM_STATIC_RX_BUFFER_SIZE must be at least 64 bytes"
#endif

#if (DCM_STATIC_TX_BUFFER_SIZE < 64)
    #error "DCM_STATIC_TX_BUFFER_SIZE must be at least 64 bytes"
#endif

#ifdef __cplusplus
}
#endif

#endif /* DCM_STATIC_CONFIG_H */
