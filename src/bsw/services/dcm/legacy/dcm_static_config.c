/******************************************************************************
 * @file    dcm_static_config.c
 * @brief   DCM Static Memory Allocation Implementation
 *
 * Compile-time static memory allocation for DCM module.
 * All memory is pre-allocated; no heap usage.
 *
 * AUTOSAR R22-11 compliant
 * MISRA C:2012 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_static_config.h"
#include <string.h>

#if defined(DCM_USE_STATIC_ALLOCATION)

/******************************************************************************
 * Static Memory Definitions
 ******************************************************************************/

/* Channel buffers */
uint8_t g_dcmStaticRxBuffers[DCM_STATIC_MAX_CHANNELS][DCM_STATIC_RX_BUFFER_SIZE];
uint8_t g_dcmStaticTxBuffers[DCM_STATIC_MAX_CHANNELS][DCM_STATIC_TX_BUFFER_SIZE];

/* Channel structures */
#if defined(DCM_USE_COMPRESSED_TYPES)
Dcm_CompChannelType g_dcmStaticChannels[DCM_STATIC_MAX_CHANNELS];
#else
Dcm_ChannelType g_dcmStaticChannels[DCM_STATIC_MAX_CHANNELS];
#endif

/* Session configurations */
#if defined(DCM_USE_COMPRESSED_TYPES)
Dcm_CompSessionConfigType g_dcmStaticSessions[DCM_STATIC_MAX_SESSIONS];
#else
Dcm_SessionConfigType g_dcmStaticSessions[DCM_STATIC_MAX_SESSIONS];
#endif

/* Service table */
Dcm_ServiceConfigType g_dcmStaticServiceTable[DCM_STATIC_MAX_SERVICES];

/* Security configurations */
Dcm_SecurityConfigType g_dcmStaticSecurityConfigs[DCM_STATIC_MAX_SECURITY_LEVELS];

/* Routine configurations */
Dcm_RoutineConfigType g_dcmStaticRoutines[DCM_STATIC_MAX_ROUTINES];

/* Dynamic DID storage */
Dcm_DynamicDidConfigType g_dcmStaticDynamicDids[DCM_STATIC_MAX_DYNAMIC_DIDS];

/* Memory regions */
Dcm_MemoryRegionConfigType g_dcmStaticMemoryRegions[DCM_STATIC_MAX_MEMORY_REGIONS];

/* Priority queue entries */
#if defined(DCM_USE_COMPRESSED_TYPES)
Dcm_CompPqEntryType g_dcmStaticPqEntries[DCM_STATIC_MAX_PQ_ENTRIES];
#else
Dcm_PqEntry g_dcmStaticPqEntries[DCM_STATIC_MAX_PQ_ENTRIES];
#endif

/* Static context */
#if defined(DCM_USE_COMPRESSED_TYPES)
Dcm_CompContextType g_dcmStaticContext;
#else
Dcm_ContextType g_dcmStaticContext;
#endif

/******************************************************************************
 * Allocation Tracking
 ******************************************************************************/

typedef struct {
    bool channelUsed[DCM_STATIC_MAX_CHANNELS];
    bool sessionUsed[DCM_STATIC_MAX_SESSIONS];
    bool serviceUsed[DCM_STATIC_MAX_SERVICES];
    bool securityUsed[DCM_STATIC_MAX_SECURITY_LEVELS];
    bool routineUsed[DCM_STATIC_MAX_ROUTINES];
    bool dynamicDidUsed[DCM_STATIC_MAX_DYNAMIC_DIDS];
    bool memoryRegionUsed[DCM_STATIC_MAX_MEMORY_REGIONS];
    bool pqEntryUsed[DCM_STATIC_MAX_PQ_ENTRIES];
    bool initialized;
} Dcm_StaticState;

static Dcm_StaticState s_staticState;

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

static void clearAllUsage(void)
{
    (void)memset(&s_staticState, 0, sizeof(s_staticState));
}

static int16_t findFreeSlot(bool *usedArray, uint8_t maxCount)
{
    for (uint8_t i = 0U; i < maxCount; i++) {
        if (!usedArray[i]) {
            return (int16_t)i;
        }
    }
    return -1;
}

static uint8_t countUsedSlots(const bool *usedArray, uint8_t maxCount)
{
    uint8_t count = 0U;
    for (uint8_t i = 0U; i < maxCount; i++) {
        if (usedArray[i]) {
            count++;
        }
    }
    return count;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_StaticInit(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;

    /* Clear all static memory */
    (void)memset(g_dcmStaticRxBuffers, 0, sizeof(g_dcmStaticRxBuffers));
    (void)memset(g_dcmStaticTxBuffers, 0, sizeof(g_dcmStaticTxBuffers));
    (void)memset(g_dcmStaticChannels, 0, sizeof(g_dcmStaticChannels));
    (void)memset(g_dcmStaticSessions, 0, sizeof(g_dcmStaticSessions));
    (void)memset(g_dcmStaticServiceTable, 0, sizeof(g_dcmStaticServiceTable));
    (void)memset(g_dcmStaticSecurityConfigs, 0, sizeof(g_dcmStaticSecurityConfigs));
    (void)memset(g_dcmStaticRoutines, 0, sizeof(g_dcmStaticRoutines));
    (void)memset(g_dcmStaticDynamicDids, 0, sizeof(g_dcmStaticDynamicDids));
    (void)memset(g_dcmStaticMemoryRegions, 0, sizeof(g_dcmStaticMemoryRegions));
    (void)memset(g_dcmStaticPqEntries, 0, sizeof(g_dcmStaticPqEntries));
    (void)memset(&g_dcmStaticContext, 0, sizeof(g_dcmStaticContext));

    /* Clear allocation tracking */
    clearAllUsage();

    /* Initialize channel buffers */
    for (uint8_t i = 0U; i < DCM_STATIC_MAX_CHANNELS; i++) {
#if defined(DCM_USE_COMPRESSED_TYPES)
        g_dcmStaticChannels[i].rxBuffer = g_dcmStaticRxBuffers[i];
        g_dcmStaticChannels[i].txBuffer = g_dcmStaticTxBuffers[i];
        g_dcmStaticChannels[i].rxBufferSize = DCM_STATIC_RX_BUFFER_SIZE;
        g_dcmStaticChannels[i].txBufferSize = DCM_STATIC_TX_BUFFER_SIZE;
        g_dcmStaticChannels[i].channelId = i;
#else
        g_dcmStaticChannels[i].rxBuffer = g_dcmStaticRxBuffers[i];
        g_dcmStaticChannels[i].txBuffer = g_dcmStaticTxBuffers[i];
        g_dcmStaticChannels[i].rxBufferSize = DCM_STATIC_RX_BUFFER_SIZE;
        g_dcmStaticChannels[i].txBufferSize = DCM_STATIC_TX_BUFFER_SIZE;
        g_dcmStaticChannels[i].channelId = i;
#endif
    }

    s_staticState.initialized = true;
    result = DCM_E_OK;

    return result;
}

Dcm_ReturnType Dcm_StaticDeInit(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;

    if (s_staticState.initialized) {
        clearAllUsage();
        s_staticState.initialized = false;
        result = DCM_E_OK;
    }

    return result;
}

/******************************************************************************
 * Channel Allocation
 ******************************************************************************/

DCM_CHANNEL_TYPE_STATIC* Dcm_StaticAllocChannel(void)
{
    DCM_CHANNEL_TYPE_STATIC *channel = NULL;

    if (!s_staticState.initialized) {
        return NULL;
    }

    int16_t slot = findFreeSlot(s_staticState.channelUsed, DCM_STATIC_MAX_CHANNELS);
    if (slot >= 0) {
        s_staticState.channelUsed[(uint8_t)slot] = true;
        channel = &g_dcmStaticChannels[(uint8_t)slot];
        (void)memset(channel, 0, sizeof(DCM_CHANNEL_TYPE_STATIC));
        channel->rxBuffer = g_dcmStaticRxBuffers[(uint8_t)slot];
        channel->txBuffer = g_dcmStaticTxBuffers[(uint8_t)slot];
#if defined(DCM_USE_COMPRESSED_TYPES)
        channel->rxBufferSize = DCM_STATIC_RX_BUFFER_SIZE;
        channel->txBufferSize = DCM_STATIC_TX_BUFFER_SIZE;
        channel->channelId = (uint16_t)slot;
#else
        channel->rxBufferSize = DCM_STATIC_RX_BUFFER_SIZE;
        channel->txBufferSize = DCM_STATIC_TX_BUFFER_SIZE;
        channel->channelId = (uint8_t)slot;
#endif
    }

    return channel;
}

void Dcm_StaticFreeChannel(DCM_CHANNEL_TYPE_STATIC *channel)
{
    if ((channel == NULL) || (!s_staticState.initialized)) {
        return;
    }

    /* Find which slot this channel is */
    for (uint8_t i = 0U; i < DCM_STATIC_MAX_CHANNELS; i++) {
        if (&g_dcmStaticChannels[i] == channel) {
            s_staticState.channelUsed[i] = false;
            (void)memset(channel, 0, sizeof(DCM_CHANNEL_TYPE_STATIC));
            return;
        }
    }
}

/******************************************************************************
 * Session Allocation
 ******************************************************************************/

DCM_SESSION_TYPE_STATIC* Dcm_StaticAllocSession(void)
{
    DCM_SESSION_TYPE_STATIC *session = NULL;

    if (!s_staticState.initialized) {
        return NULL;
    }

    int16_t slot = findFreeSlot(s_staticState.sessionUsed, DCM_STATIC_MAX_SESSIONS);
    if (slot >= 0) {
        s_staticState.sessionUsed[(uint8_t)slot] = true;
        session = &g_dcmStaticSessions[(uint8_t)slot];
        (void)memset(session, 0, sizeof(DCM_SESSION_TYPE_STATIC));
    }

    return session;
}

void Dcm_StaticFreeSession(DCM_SESSION_TYPE_STATIC *session)
{
    if ((session == NULL) || (!s_staticState.initialized)) {
        return;
    }

    for (uint8_t i = 0U; i < DCM_STATIC_MAX_SESSIONS; i++) {
        if (&g_dcmStaticSessions[i] == session) {
            s_staticState.sessionUsed[i] = false;
            (void)memset(session, 0, sizeof(DCM_SESSION_TYPE_STATIC));
            return;
        }
    }
}

/******************************************************************************
 * Service Allocation
 ******************************************************************************/

Dcm_ServiceConfigType* Dcm_StaticAllocService(void)
{
    Dcm_ServiceConfigType *service = NULL;

    if (!s_staticState.initialized) {
        return NULL;
    }

    int16_t slot = findFreeSlot(s_staticState.serviceUsed, DCM_STATIC_MAX_SERVICES);
    if (slot >= 0) {
        s_staticState.serviceUsed[(uint8_t)slot] = true;
        service = &g_dcmStaticServiceTable[(uint8_t)slot];
        (void)memset(service, 0, sizeof(Dcm_ServiceConfigType));
    }

    return service;
}

void Dcm_StaticFreeService(Dcm_ServiceConfigType *service)
{
    if ((service == NULL) || (!s_staticState.initialized)) {
        return;
    }

    for (uint8_t i = 0U; i < DCM_STATIC_MAX_SERVICES; i++) {
        if (&g_dcmStaticServiceTable[i] == service) {
            s_staticState.serviceUsed[i] = false;
            (void)memset(service, 0, sizeof(Dcm_ServiceConfigType));
            return;
        }
    }
}

/******************************************************************************
 * Security Allocation
 ******************************************************************************/

Dcm_SecurityConfigType* Dcm_StaticAllocSecurity(void)
{
    Dcm_SecurityConfigType *security = NULL;

    if (!s_staticState.initialized) {
        return NULL;
    }

    int16_t slot = findFreeSlot(s_staticState.securityUsed, DCM_STATIC_MAX_SECURITY_LEVELS);
    if (slot >= 0) {
        s_staticState.securityUsed[(uint8_t)slot] = true;
        security = &g_dcmStaticSecurityConfigs[(uint8_t)slot];
        (void)memset(security, 0, sizeof(Dcm_SecurityConfigType));
    }

    return security;
}

void Dcm_StaticFreeSecurity(Dcm_SecurityConfigType *security)
{
    if ((security == NULL) || (!s_staticState.initialized)) {
        return;
    }

    for (uint8_t i = 0U; i < DCM_STATIC_MAX_SECURITY_LEVELS; i++) {
        if (&g_dcmStaticSecurityConfigs[i] == security) {
            s_staticState.securityUsed[i] = false;
            (void)memset(security, 0, sizeof(Dcm_SecurityConfigType));
            return;
        }
    }
}

/******************************************************************************
 * Routine Allocation
 ******************************************************************************/

Dcm_RoutineConfigType* Dcm_StaticAllocRoutine(void)
{
    Dcm_RoutineConfigType *routine = NULL;

    if (!s_staticState.initialized) {
        return NULL;
    }

    int16_t slot = findFreeSlot(s_staticState.routineUsed, DCM_STATIC_MAX_ROUTINES);
    if (slot >= 0) {
        s_staticState.routineUsed[(uint8_t)slot] = true;
        routine = &g_dcmStaticRoutines[(uint8_t)slot];
        (void)memset(routine, 0, sizeof(Dcm_RoutineConfigType));
    }

    return routine;
}

void Dcm_StaticFreeRoutine(Dcm_RoutineConfigType *routine)
{
    if ((routine == NULL) || (!s_staticState.initialized)) {
        return;
    }

    for (uint8_t i = 0U; i < DCM_STATIC_MAX_ROUTINES; i++) {
        if (&g_dcmStaticRoutines[i] == routine) {
            s_staticState.routineUsed[i] = false;
            (void)memset(routine, 0, sizeof(Dcm_RoutineConfigType));
            return;
        }
    }
}

/******************************************************************************
 * Dynamic DID Allocation
 ******************************************************************************/

Dcm_DynamicDidConfigType* Dcm_StaticAllocDynamicDid(void)
{
    Dcm_DynamicDidConfigType *did = NULL;

    if (!s_staticState.initialized) {
        return NULL;
    }

    int16_t slot = findFreeSlot(s_staticState.dynamicDidUsed, DCM_STATIC_MAX_DYNAMIC_DIDS);
    if (slot >= 0) {
        s_staticState.dynamicDidUsed[(uint8_t)slot] = true;
        did = &g_dcmStaticDynamicDids[(uint8_t)slot];
        (void)memset(did, 0, sizeof(Dcm_DynamicDidConfigType));
    }

    return did;
}

void Dcm_StaticFreeDynamicDid(Dcm_DynamicDidConfigType *did)
{
    if ((did == NULL) || (!s_staticState.initialized)) {
        return;
    }

    for (uint8_t i = 0U; i < DCM_STATIC_MAX_DYNAMIC_DIDS; i++) {
        if (&g_dcmStaticDynamicDids[i] == did) {
            s_staticState.dynamicDidUsed[i] = false;
            (void)memset(did, 0, sizeof(Dcm_DynamicDidConfigType));
            return;
        }
    }
}

/******************************************************************************
 * Memory Region Allocation
 ******************************************************************************/

Dcm_MemoryRegionConfigType* Dcm_StaticAllocMemoryRegion(void)
{
    Dcm_MemoryRegionConfigType *region = NULL;

    if (!s_staticState.initialized) {
        return NULL;
    }

    int16_t slot = findFreeSlot(s_staticState.memoryRegionUsed, DCM_STATIC_MAX_MEMORY_REGIONS);
    if (slot >= 0) {
        s_staticState.memoryRegionUsed[(uint8_t)slot] = true;
        region = &g_dcmStaticMemoryRegions[(uint8_t)slot];
        (void)memset(region, 0, sizeof(Dcm_MemoryRegionConfigType));
    }

    return region;
}

void Dcm_StaticFreeMemoryRegion(Dcm_MemoryRegionConfigType *region)
{
    if ((region == NULL) || (!s_staticState.initialized)) {
        return;
    }

    for (uint8_t i = 0U; i < DCM_STATIC_MAX_MEMORY_REGIONS; i++) {
        if (&g_dcmStaticMemoryRegions[i] == region) {
            s_staticState.memoryRegionUsed[i] = false;
            (void)memset(region, 0, sizeof(Dcm_MemoryRegionConfigType));
            return;
        }
    }
}

/******************************************************************************
 * Priority Queue Entry Allocation
 ******************************************************************************/

DCM_PQ_ENTRY_TYPE_STATIC* Dcm_StaticAllocPqEntry(void)
{
    DCM_PQ_ENTRY_TYPE_STATIC *entry = NULL;

    if (!s_staticState.initialized) {
        return NULL;
    }

    int16_t slot = findFreeSlot(s_staticState.pqEntryUsed, DCM_STATIC_MAX_PQ_ENTRIES);
    if (slot >= 0) {
        s_staticState.pqEntryUsed[(uint8_t)slot] = true;
        entry = &g_dcmStaticPqEntries[(uint8_t)slot];
        (void)memset(entry, 0, sizeof(DCM_PQ_ENTRY_TYPE_STATIC));
    }

    return entry;
}

void Dcm_StaticFreePqEntry(DCM_PQ_ENTRY_TYPE_STATIC *entry)
{
    if ((entry == NULL) || (!s_staticState.initialized)) {
        return;
    }

    for (uint8_t i = 0U; i < DCM_STATIC_MAX_PQ_ENTRIES; i++) {
        if (&g_dcmStaticPqEntries[i] == entry) {
            s_staticState.pqEntryUsed[i] = false;
            (void)memset(entry, 0, sizeof(DCM_PQ_ENTRY_TYPE_STATIC));
            return;
        }
    }
}

/******************************************************************************
 * Context Access
 ******************************************************************************/

DCM_CONTEXT_TYPE_STATIC* Dcm_StaticGetContext(void)
{
    return &g_dcmStaticContext;
}

DCM_CHANNEL_TYPE_STATIC* Dcm_StaticGetChannels(void)
{
    return g_dcmStaticChannels;
}

uint8_t Dcm_StaticGetChannelCount(void)
{
    if (!s_staticState.initialized) {
        return 0U;
    }
    return countUsedSlots(s_staticState.channelUsed, DCM_STATIC_MAX_CHANNELS);
}

Dcm_ReturnType Dcm_StaticGetUsage(uint32_t *totalBytes, uint32_t *usedBytes)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;

    if ((totalBytes == NULL) || (usedBytes == NULL)) {
        return result;
    }

    *totalBytes = DCM_STATIC_TOTAL_SIZE;

    *usedBytes = 0U;
    *usedBytes += countUsedSlots(s_staticState.channelUsed, DCM_STATIC_MAX_CHANNELS) *
                  sizeof(DCM_CHANNEL_TYPE_STATIC);
    *usedBytes += countUsedSlots(s_staticState.sessionUsed, DCM_STATIC_MAX_SESSIONS) *
                  sizeof(DCM_SESSION_TYPE_STATIC);
    *usedBytes += countUsedSlots(s_staticState.serviceUsed, DCM_STATIC_MAX_SERVICES) *
                  sizeof(Dcm_ServiceConfigType);
    *usedBytes += countUsedSlots(s_staticState.securityUsed, DCM_STATIC_MAX_SECURITY_LEVELS) *
                  sizeof(Dcm_SecurityConfigType);
    *usedBytes += countUsedSlots(s_staticState.routineUsed, DCM_STATIC_MAX_ROUTINES) *
                  sizeof(Dcm_RoutineConfigType);
    *usedBytes += countUsedSlots(s_staticState.dynamicDidUsed, DCM_STATIC_MAX_DYNAMIC_DIDS) *
                  sizeof(Dcm_DynamicDidConfigType);
    *usedBytes += countUsedSlots(s_staticState.memoryRegionUsed, DCM_STATIC_MAX_MEMORY_REGIONS) *
                  sizeof(Dcm_MemoryRegionConfigType);
    *usedBytes += countUsedSlots(s_staticState.pqEntryUsed, DCM_STATIC_MAX_PQ_ENTRIES) *
                  sizeof(DCM_PQ_ENTRY_TYPE_STATIC);

    result = DCM_E_OK;

    return result;
}

bool Dcm_StaticIsStaticPtr(const void *ptr)
{
    if (ptr == NULL) {
        return false;
    }

    /* Check all static arrays */
    if (((const uint8_t *)ptr >= (const uint8_t *)g_dcmStaticChannels) &&
        ((const uint8_t *)ptr < (const uint8_t *)&g_dcmStaticChannels[DCM_STATIC_MAX_CHANNELS])) {
        return true;
    }

    if (((const uint8_t *)ptr >= (const uint8_t *)g_dcmStaticSessions) &&
        ((const uint8_t *)ptr < (const uint8_t *)&g_dcmStaticSessions[DCM_STATIC_MAX_SESSIONS])) {
        return true;
    }

    if (((const uint8_t *)ptr >= (const uint8_t *)g_dcmStaticServiceTable) &&
        ((const uint8_t *)ptr < (const uint8_t *)&g_dcmStaticServiceTable[DCM_STATIC_MAX_SERVICES])) {
        return true;
    }

    if (((const uint8_t *)ptr >= (const uint8_t *)g_dcmStaticRoutines) &&
        ((const uint8_t *)ptr < (const uint8_t *)&g_dcmStaticRoutines[DCM_STATIC_MAX_ROUTINES])) {
        return true;
    }

    if (((const uint8_t *)ptr >= (const uint8_t *)g_dcmStaticPqEntries) &&
        ((const uint8_t *)ptr < (const uint8_t *)&g_dcmStaticPqEntries[DCM_STATIC_MAX_PQ_ENTRIES])) {
        return true;
    }

    return false;
}

#endif /* DCM_USE_STATIC_ALLOCATION */
