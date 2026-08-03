/******************************************************************************
 * @file    function_group.c
 * @brief   Function Group Manager Implementation
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ara/sm/function_group.h"
#include "ara/sm/sm_internal.h"
#include <string.h>

/******************************************************************************
 * Private Data
 ******************************************************************************/

static bool g_initialized = false;
static FG_ConfigType g_configs[FUNCTION_GROUP_COUNT];
static FG_RuntimeType g_runtimes[FUNCTION_GROUP_COUNT];

/******************************************************************************
 * Private Functions
 ******************************************************************************/

static FG_RuntimeType* FindRuntime(FunctionGroupNameType name) {
    if (name >= FUNCTION_GROUP_COUNT) {
        return NULL;
    }
    return &g_runtimes[name];
}

static FG_ConfigType* FindConfig(FunctionGroupNameType name) {
    if (name >= FUNCTION_GROUP_COUNT) {
        return NULL;
    }
    return &g_configs[name];
}

static bool IsTransitionComplete(FunctionGroupNameType name) {
    FG_RuntimeType* runtime = FindRuntime(name);
    if (runtime == NULL) {
        return false;
    }
    
    return runtime->currentState == runtime->targetState;
}

static bool AreAllMembersActive(FunctionGroupNameType name) {
    FG_ConfigType* config = FindConfig(name);
    FG_RuntimeType* runtime = FindRuntime(name);
    
    if (config == NULL || runtime == NULL) {
        return false;
    }
    
    return runtime->activeMemberCount >= config->memberCount;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Std_ReturnType FG_Init(void) {
    if (g_initialized) {
        return E_OK;
    }
    
    memset(g_configs, 0, sizeof(g_configs));
    memset(g_runtimes, 0, sizeof(g_runtimes));
    
    /* Initialize runtime states */
    for (uint32_t i = 0U; i < FUNCTION_GROUP_COUNT; i++) {
        g_runtimes[i].name = (FunctionGroupNameType)i;
        g_runtimes[i].currentState = FUNCTION_GROUP_STATE_OFF;
        g_runtimes[i].targetState = FUNCTION_GROUP_STATE_OFF;
        g_runtimes[i].transitionInProgress = false;
        g_runtimes[i].isStable = true;
        
        /* Set default configuration */
        strncpy(g_configs[i].displayName, StateMachine_GetFGNameString((FunctionGroupNameType)i),
                FG_MAX_NAME_LENGTH - 1U);
        g_configs[i].displayName[FG_MAX_NAME_LENGTH - 1U] = '\0';
        g_configs[i].name = (FunctionGroupNameType)i;
        g_configs[i].transitionTimeoutMs = FG_TRANSITION_TIMEOUT_MS;
        g_configs[i].autoRestartMembers = false;
    }
    
    g_initialized = true;
    return E_OK;
}

Std_ReturnType FG_Deinit(void) {
    if (!g_initialized) {
        return E_OK;
    }
    
    /* Stop all function groups */
    for (uint32_t i = 0U; i < FUNCTION_GROUP_COUNT; i++) {
        FG_Stop((FunctionGroupNameType)i);
    }
    
    memset(g_configs, 0, sizeof(g_configs));
    memset(g_runtimes, 0, sizeof(g_runtimes));
    
    g_initialized = false;
    return E_OK;
}

bool FG_IsInitialized(void) {
    return g_initialized;
}

Std_ReturnType FG_CreateConfig(FunctionGroupNameType name, FG_ConfigType* config) {
    if (!g_initialized || config == NULL || name >= FUNCTION_GROUP_COUNT) {
        return E_NOT_OK;
    }
    
    memcpy(config, &g_configs[name], sizeof(FG_ConfigType));
    return E_OK;
}

Std_ReturnType FG_AddMember(FG_ConfigType* config, const FG_MemberType* member) {
    if (!g_initialized || config == NULL || member == NULL) {
        return E_NOT_OK;
    }
    
    if (config->memberCount >= FG_MAX_MEMBERS) {
        return E_NOT_OK;
    }
    
    /* Check if member already exists */
    for (uint32_t i = 0U; i < config->memberCount; i++) {
        if (strcmp(config->members[i].name, member->name) == 0) {
            return E_NOT_OK;  /* Member already exists */
        }
    }
    
    memcpy(&config->members[config->memberCount], member, sizeof(FG_MemberType));
    config->memberCount++;
    
    return E_OK;
}

Std_ReturnType FG_RemoveMember(FG_ConfigType* config, const char* memberName) {
    if (!g_initialized || config == NULL || memberName == NULL) {
        return E_NOT_OK;
    }
    
    int32_t index = -1;
    for (uint32_t i = 0U; i < config->memberCount; i++) {
        if (strcmp(config->members[i].name, memberName) == 0) {
            index = (int32_t)i;
            break;
        }
    }
    
    if (index < 0) {
        return E_NOT_OK;  /* Member not found */
    }
    
    /* Shift remaining members */
    for (uint32_t i = (uint32_t)index; i < config->memberCount - 1U; i++) {
        memcpy(&config->members[i], &config->members[i + 1U], sizeof(FG_MemberType));
    }
    
    config->memberCount--;
    memset(&config->members[config->memberCount], 0, sizeof(FG_MemberType));
    
    return E_OK;
}

Std_ReturnType FG_GetMember(const FG_ConfigType* config, const char* memberName,
                            FG_MemberType* member) {
    if (!g_initialized || config == NULL || memberName == NULL || member == NULL) {
        return E_NOT_OK;
    }
    
    for (uint32_t i = 0U; i < config->memberCount; i++) {
        if (strcmp(config->members[i].name, memberName) == 0) {
            memcpy(member, &config->members[i], sizeof(FG_MemberType));
            return E_OK;
        }
    }
    
    return E_NOT_OK;  /* Member not found */
}

Std_ReturnType FG_Configure(FunctionGroupNameType name, const FG_ConfigType* config) {
    if (!g_initialized || config == NULL || name >= FUNCTION_GROUP_COUNT) {
        return E_NOT_OK;
    }
    
    /* Cannot configure while transitioning */
    FG_RuntimeType* runtime = FindRuntime(name);
    if (runtime != NULL && runtime->transitionInProgress) {
        return E_NOT_OK;
    }
    
    memcpy(&g_configs[name], config, sizeof(FG_ConfigType));
    g_configs[name].name = name;  /* Ensure name is correct */
    
    return E_OK;
}

Std_ReturnType FG_Start(FunctionGroupNameType name) {
    if (!g_initialized || name >= FUNCTION_GROUP_COUNT) {
        return E_NOT_OK;
    }
    
    FG_RuntimeType* runtime = FindRuntime(name);
    if (runtime == NULL) {
        return E_NOT_OK;
    }
    
    /* Already in target state */
    if (runtime->currentState == FUNCTION_GROUP_STATE_ON) {
        return E_OK;
    }
    
    /* Start transition */
    runtime->targetState = FUNCTION_GROUP_STATE_ON;
    runtime->currentState = FUNCTION_GROUP_STATE_STARTING;
    runtime->transitionInProgress = true;
    runtime->transitionStartTime = 0U;  /* Would use actual timestamp */
    runtime->isStable = false;
    
    /* Notify state machine */
    StateMachine_SetFGState(name, FUNCTION_GROUP_STATE_STARTING);
    
    return E_OK;
}

Std_ReturnType FG_Stop(FunctionGroupNameType name) {
    if (!g_initialized || name >= FUNCTION_GROUP_COUNT) {
        return E_NOT_OK;
    }
    
    FG_RuntimeType* runtime = FindRuntime(name);
    if (runtime == NULL) {
        return E_NOT_OK;
    }
    
    /* Already in target state */
    if (runtime->currentState == FUNCTION_GROUP_STATE_OFF) {
        return E_OK;
    }
    
    /* Start transition */
    runtime->targetState = FUNCTION_GROUP_STATE_OFF;
    runtime->currentState = FUNCTION_GROUP_STATE_STOPPING;
    runtime->transitionInProgress = true;
    runtime->transitionStartTime = 0U;
    runtime->isStable = false;
    
    /* Notify state machine */
    StateMachine_SetFGState(name, FUNCTION_GROUP_STATE_STOPPING);
    
    return E_OK;
}

Std_ReturnType FG_GetRuntimeState(FunctionGroupNameType name, FG_RuntimeType* runtime) {
    if (!g_initialized || runtime == NULL || name >= FUNCTION_GROUP_COUNT) {
        return E_NOT_OK;
    }
    
    FG_RuntimeType* r = FindRuntime(name);
    if (r == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(runtime, r, sizeof(FG_RuntimeType));
    return E_OK;
}

bool FG_IsStable(FunctionGroupNameType name) {
    if (!g_initialized || name >= FUNCTION_GROUP_COUNT) {
        return false;
    }
    
    FG_RuntimeType* runtime = FindRuntime(name);
    if (runtime == NULL) {
        return false;
    }
    
    return runtime->isStable;
}

Std_ReturnType FG_WaitForStable(FunctionGroupNameType name, uint32_t timeoutMs) {
    if (!g_initialized || name >= FUNCTION_GROUP_COUNT) {
        return E_NOT_OK;
    }
    
    uint32_t startTime = 0U;  /* Would use actual timestamp */
    
    while (!FG_IsStable(name)) {
        uint32_t currentTime = 0U;
        if (currentTime - startTime > timeoutMs) {
            return E_NOT_OK;  /* Timeout */
        }
        
        /* In real implementation, would yield/sleep */
    }
    
    return E_OK;
}

Std_ReturnType FG_NotifyMemberState(FunctionGroupNameType name, const char* memberName,
                                     bool isActive) {
    if (!g_initialized || name >= FUNCTION_GROUP_COUNT || memberName == NULL) {
        return E_NOT_OK;
    }
    
    FG_ConfigType* config = FindConfig(name);
    FG_RuntimeType* runtime = FindRuntime(name);
    
    if (config == NULL || runtime == NULL) {
        return E_NOT_OK;
    }
    
    /* Find member */
    FG_MemberType* member = NULL;
    for (uint32_t i = 0U; i < config->memberCount; i++) {
        if (strcmp(config->members[i].name, memberName) == 0) {
            member = &config->members[i];
            break;
        }
    }
    
    if (member == NULL) {
        return E_NOT_OK;
    }
    
    /* Update active count */
    if (member->isActive != isActive) {
        member->isActive = isActive;
        if (isActive) {
            runtime->activeMemberCount++;
        } else {
            runtime->activeMemberCount--;
        }
    }
    
    return E_OK;
}

uint32_t FG_GetActiveMemberCount(FunctionGroupNameType name) {
    if (!g_initialized || name >= FUNCTION_GROUP_COUNT) {
        return 0U;
    }
    
    FG_RuntimeType* runtime = FindRuntime(name);
    if (runtime == NULL) {
        return 0U;
    }
    
    return runtime->activeMemberCount;
}

void FG_MainFunction(void) {
    if (!g_initialized) {
        return;
    }
    
    uint32_t currentTime = 0U;  /* Would use actual timestamp */
    
    for (uint32_t i = 0U; i < FUNCTION_GROUP_COUNT; i++) {
        FG_RuntimeType* runtime = &g_runtimes[i];
        
        if (!runtime->transitionInProgress) {
            continue;
        }
        
        /* Check for timeout */
        FG_ConfigType* config = &g_configs[i];
        if (currentTime - runtime->transitionStartTime > config->transitionTimeoutMs) {
            /* Transition timed out - force to target state */
            runtime->currentState = runtime->targetState;
            runtime->transitionInProgress = false;
            runtime->isStable = true;
            
            StateMachine_SetFGState((FunctionGroupNameType)i, runtime->currentState);
            continue;
        }
        
        /* Check if transition is complete */
        if (runtime->targetState == FUNCTION_GROUP_STATE_ON) {
            /* Check if all members are active */
            if (AreAllMembersActive((FunctionGroupNameType)i)) {
                runtime->currentState = FUNCTION_GROUP_STATE_ON;
                runtime->transitionInProgress = false;
                runtime->isStable = true;
                
                StateMachine_SetFGState((FunctionGroupNameType)i, FUNCTION_GROUP_STATE_ON);
            }
        } else if (runtime->targetState == FUNCTION_GROUP_STATE_OFF) {
            /* Check if all members are inactive */
            if (runtime->activeMemberCount == 0U) {
                runtime->currentState = FUNCTION_GROUP_STATE_OFF;
                runtime->transitionInProgress = false;
                runtime->isStable = true;
                
                StateMachine_SetFGState((FunctionGroupNameType)i, FUNCTION_GROUP_STATE_OFF);
            }
        }
    }
}
