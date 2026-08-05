/******************************************************************************
 * @file    e2e_dds_integration.c
 * @brief   E2E Protection with DDS Integration Implementation
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#include "e2e_dds_integration.h"
#include <string.h>
#include <stdio.h>

/******************************************************************************
 * Private Definitions
 ******************************************************************************/
#define E2E_DDS_INITIALIZED_MAGIC       0x45324444U  /* "E2DD" */
#define E2E_DDS_TOPIC_MAGIC             0x544F5049U  /* "TOPI" */

/******************************************************************************
 * Private Data
 ******************************************************************************/
static uint32_t g_initMagic = 0;
static bool g_initialized = FALSE;
static E2E_DDS_CallbackType g_callbacks = {NULL, NULL};
static uint32_t g_nextTopicId = 1;

/******************************************************************************
 * Private Functions
 ******************************************************************************/

/**
 * @brief Validate topic configuration
 */
static bool E2E_DDS_ValidateConfig(const E2E_DDS_TopicConfigType* config)
{
    if (config == NULL) {
        return FALSE;
    }

    if (config->topicName[0] == '\0') {
        return FALSE;
    }

    /* Validate E2E profile */
    if (config->e2eProfile < E2E_PROFILE_01 ||
        (config->e2eProfile > E2E_PROFILE_11 && config->e2eProfile != E2E_PROFILE_22)) {
        return FALSE;
    }

    /* Validate data length for fixed-length profiles */
    if (config->e2eProfile != E2E_PROFILE_11 && config->e2eProfile != E2E_PROFILE_22) {
        if (config->dataLength == 0) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief Initialize E2E context based on profile
 */
static Std_ReturnType E2E_DDS_InitE2EContext(
    E2E_DDS_TopicHandleType* handle,
    const E2E_DDS_TopicConfigType* config)
{
    Std_ReturnType result = E2E_InitContext(&handle->e2eContext, config->e2eProfile);
    if (result != E_OK) {
        return result;
    }

    /* Configure profile-specific settings */
    switch (config->e2eProfile) {
        case E2E_PROFILE_01:
            handle->e2eContext.config.p01.dataId = config->dataId;
            handle->e2eContext.config.p01.dataLength = config->dataLength;
            handle->e2eContext.config.p01.crcOffset = 0;
            break;

        case E2E_PROFILE_02:
            handle->e2eContext.config.p02.dataId = config->dataId;
            handle->e2eContext.config.p02.dataLength = config->dataLength;
            handle->e2eContext.config.p02.crcOffset = 0;
            handle->e2eContext.config.p02.counterOffset = 1;
            break;

        case E2E_PROFILE_04:
            handle->e2eContext.config.p04.dataId = config->dataId;
            handle->e2eContext.config.p04.dataLength = config->dataLength;
            handle->e2eContext.config.p04.crcOffset = 0;
            break;

        case E2E_PROFILE_05:
            handle->e2eContext.config.p05.dataId = config->dataId;
            handle->e2eContext.config.p05.dataLength = config->dataLength;
            handle->e2eContext.config.p05.crcOffset = 0;
            break;

        case E2E_PROFILE_06:
            handle->e2eContext.config.p06.dataId = config->dataId;
            handle->e2eContext.config.p06.dataLength = config->dataLength;
            handle->e2eContext.config.p06.crcOffset = 0;
            handle->e2eContext.config.p06.counterOffset = 2;
            break;

        case E2E_PROFILE_07:
            handle->e2eContext.config.p07.dataId = config->dataId;
            handle->e2eContext.config.p07.dataLength = config->dataLength;
            handle->e2eContext.config.p07.crcOffset = 0;
            handle->e2eContext.config.p07.counterOffset = 4;
            break;

        case E2E_PROFILE_11:
            handle->e2eContext.config.p11.dataId = config->dataId;
            handle->e2eContext.config.p11.dataLength = config->dataLength;
            handle->e2eContext.config.p11.maxDataLength = config->maxDataLength;
            handle->e2eContext.config.p11.minDataLength = 4;
            handle->e2eContext.config.p11.crcOffset = 0;
            handle->e2eContext.config.p11.counterOffset = 1;
            handle->e2eContext.config.p11.dataIdOffset = 2;
            break;

        case E2E_PROFILE_22:
            handle->e2eContext.config.p22.dataId = config->dataId;
            handle->e2eContext.config.p22.dataLength = config->dataLength;
            handle->e2eContext.config.p22.maxDataLength = config->maxDataLength;
            handle->e2eContext.config.p22.minDataLength = E2E_P22_MIN_DATA_LENGTH;
            handle->e2eContext.config.p22.crcOffset = 0;
            handle->e2eContext.config.p22.counterOffset = 2;
            handle->e2eContext.config.p22.dataIdOffset = 4;
            handle->e2eContext.config.p22.lengthOffset = 6;
            handle->e2eContext.config.p22.includeLengthInCrc = TRUE;
            break;

        default:
            return E_NOT_OK;
    }

    return E_OK;
}

/**
 * @brief Initialize state machine for topic
 */
static Std_ReturnType E2E_DDS_InitStateMachine(
    E2E_DDS_TopicHandleType* handle,
    const E2E_DDS_TopicConfigType* config)
{
    if (!config->enableStateMachine) {
        return E_OK;
    }

    E2E_SM_ConfigType smConfig;
    memset(&smConfig, 0, sizeof(E2E_SM_ConfigType));
    smConfig.profile = config->e2eProfile;
    smConfig.timeoutMs = config->timeoutMs;
    smConfig.enableWindowCheck = TRUE;
    smConfig.window.windowSize = config->windowSize > 0 ? config->windowSize : E2E_SM_WINDOW_SIZE_DEFAULT;
    smConfig.window.maxErrorThreshold = config->maxErrorThreshold > 0 ? 
                                        config->maxErrorThreshold : E2E_SM_MAX_ERROR_THRESHOLD;
    smConfig.window.minOkThreshold = E2E_SM_OK_THRESHOLD;
    smConfig.window.syncCounterThreshold = E2E_SM_SYNC_THRESHOLD;

    return E2E_SM_InitStateMachine(&smConfig, &handle->smState);
}

/******************************************************************************
 * Public Functions - Initialization
 ******************************************************************************/

/**
 * @brief Initialize E2E DDS Integration module
 */
Std_ReturnType E2E_DDS_Init(void)
{
    if (g_initialized) {
        return E_OK;
    }

    /* Initialize E2E module */
    if (E2E_Init() != E_OK) {
        return E_NOT_OK;
    }

    /* Initialize E2E State Machine */
    if (E2E_SM_Init() != E_OK) {
        return E_NOT_OK;
    }

    g_initMagic = E2E_DDS_INITIALIZED_MAGIC;
    g_initialized = TRUE;
    g_nextTopicId = 1;

    return E_OK;
}

/**
 * @brief Deinitialize E2E DDS Integration module
 */
Std_ReturnType E2E_DDS_Deinit(void)
{
    g_initialized = FALSE;
    g_initMagic = 0;
    g_nextTopicId = 0;
    memset(&g_callbacks, 0, sizeof(E2E_DDS_CallbackType));

    E2E_SM_Deinit();
    E2E_Deinit();

    return E_OK;
}

/**
 * @brief Check if E2E DDS Integration is initialized
 */
bool E2E_DDS_IsInitialized(void)
{
    return g_initialized && (g_initMagic == E2E_DDS_INITIALIZED_MAGIC);
}

/**
 * @brief Register error/state change callbacks
 */
Std_ReturnType E2E_DDS_RegisterCallbacks(const E2E_DDS_CallbackType* callbacks)
{
    if (!E2E_DDS_IsInitialized() || callbacks == NULL) {
        return E_NOT_OK;
    }

    memcpy(&g_callbacks, callbacks, sizeof(E2E_DDS_CallbackType));
    return E_OK;
}

/******************************************************************************
 * Public Functions - Topic Management
 ******************************************************************************/

/**
 * @brief Create E2E-protected DDS topic
 */
Std_ReturnType E2E_DDS_CreateTopic(
    const E2E_DDS_TopicConfigType* config,
    E2E_DDS_TopicHandleType* handle)
{
    if (!E2E_DDS_IsInitialized() || config == NULL || handle == NULL) {
        return E_NOT_OK;
    }

    if (!E2E_DDS_ValidateConfig(config)) {
        return E_NOT_OK;
    }

    memset(handle, 0, sizeof(E2E_DDS_TopicHandleType));
    handle->topicId = g_nextTopicId++;
    memcpy(&handle->config, config, sizeof(E2E_DDS_TopicConfigType));

    /* Initialize E2E context */
    if (E2E_DDS_InitE2EContext(handle, config) != E_OK) {
        return E_NOT_OK;
    }

    /* Initialize state machine if enabled */
    if (E2E_DDS_InitStateMachine(handle, config) != E_OK) {
        return E_NOT_OK;
    }

    handle->initialized = TRUE;
    return E_OK;
}

/**
 * @brief Destroy E2E-protected DDS topic
 */
Std_ReturnType E2E_DDS_DestroyTopic(E2E_DDS_TopicHandleType* handle)
{
    if (!E2E_DDS_IsInitialized() || handle == NULL) {
        return E_NOT_OK;
    }

    if (!handle->initialized) {
        return E_NOT_OK;
    }

    /* Deinitialize E2E context */
    E2E_DeinitContext(&handle->e2eContext);

    /* Reset state machine */
    if (handle->config.enableStateMachine) {
        E2E_SM_Reset(&handle->smState);
    }

    handle->initialized = FALSE;
    return E_OK;
}

/**
 * @brief Reset E2E topic state
 */
Std_ReturnType E2E_DDS_ResetTopic(E2E_DDS_TopicHandleType* handle)
{
    if (!E2E_DDS_IsInitialized() || handle == NULL || !handle->initialized) {
        return E_NOT_OK;
    }

    /* Reset counters */
    handle->txCounter = 0;
    handle->rxCounter = 0;
    handle->errorCounter = 0;

    /* Reset E2E state */
    switch (handle->config.e2eProfile) {
        case E2E_PROFILE_01:
            handle->e2eContext.state.p01.counter = 0;
            break;
        case E2E_PROFILE_02:
            handle->e2eContext.state.p02.counter = 0;
            handle->e2eContext.state.p02.synced = FALSE;
            break;
        case E2E_PROFILE_04:
            handle->e2eContext.state.p04.counter = 0;
            break;
        case E2E_PROFILE_05:
            handle->e2eContext.state.p05.counter = 0;
            break;
        case E2E_PROFILE_06:
            handle->e2eContext.state.p06.counter = 0;
            handle->e2eContext.state.p06.synced = FALSE;
            break;
        case E2E_PROFILE_07:
            handle->e2eContext.state.p07.counter = 0;
            handle->e2eContext.state.p07.synced = FALSE;
            break;
        case E2E_PROFILE_11:
            handle->e2eContext.state.p11.counter = 0;
            handle->e2eContext.state.p11.synced = FALSE;
            break;
        case E2E_PROFILE_22:
            handle->e2eContext.state.p22.counter = 0;
            handle->e2eContext.state.p22.synced = FALSE;
            break;
        default:
            break;
    }

    /* Reset state machine */
    if (handle->config.enableStateMachine) {
        E2E_SM_Reset(&handle->smState);
    }

    return E_OK;
}

/******************************************************************************
 * Public Functions - Data Transmission
 ******************************************************************************/

/**
 * @brief Publish data with E2E protection via DDS
 */
Std_ReturnType E2E_DDS_Publish(
    E2E_DDS_TopicHandleType* handle,
    uint8_t* data,
    uint32_t* length)
{
    if (!E2E_DDS_IsInitialized() || handle == NULL || !handle->initialized ||
        data == NULL || length == NULL) {
        return E_NOT_OK;
    }

    /* Apply E2E protection */
    Std_ReturnType result = E2E_Protect(&handle->e2eContext, data, length, handle->config.e2eProfile);
    if (result == E_OK) {
        handle->txCounter++;
        handle->lastTxTimestamp = 0; /* TODO: Get actual timestamp */
    }

    return result;
}

/**
 * @brief Subscribe to data with E2E protection via DDS
 */
Std_ReturnType E2E_DDS_Subscribe(
    E2E_DDS_TopicHandleType* handle,
    const uint8_t* data,
    uint32_t length,
    E2E_DDS_ResultType* result)
{
    if (!E2E_DDS_IsInitialized() || handle == NULL || !handle->initialized ||
        data == NULL || result == NULL) {
        return E_NOT_OK;
    }

    uint16_t e2eStatus = E2E_ERROR_NONE;
    E2E_PCheckStatusType checkStatus = E2E_P_OK;

    /* Perform E2E check */
    Std_ReturnType ret = E2E_Check(&handle->e2eContext, data, length, &e2eStatus, handle->config.e2eProfile);
    if (ret != E_OK) {
        *result = E2E_DDS_ERROR_INIT;
        return E_NOT_OK;
    }

    /* Get check status from profile state */
    switch (handle->config.e2eProfile) {
        case E2E_PROFILE_01:
            checkStatus = handle->e2eContext.state.p01.status;
            break;
        case E2E_PROFILE_02:
            checkStatus = handle->e2eContext.state.p02.status;
            break;
        case E2E_PROFILE_04:
            checkStatus = handle->e2eContext.state.p04.status;
            break;
        case E2E_PROFILE_05:
            checkStatus = handle->e2eContext.state.p05.status;
            break;
        case E2E_PROFILE_06:
            checkStatus = handle->e2eContext.state.p06.status;
            break;
        case E2E_PROFILE_07:
            checkStatus = handle->e2eContext.state.p07.status;
            break;
        case E2E_PROFILE_11:
            checkStatus = handle->e2eContext.state.p11.status;
            break;
        case E2E_PROFILE_22:
            checkStatus = handle->e2eContext.state.p22.status;
            break;
        default:
            checkStatus = E2E_P_ERROR;
            break;
    }

    /* Map E2E status to DDS result */
    switch (checkStatus) {
        case E2E_P_OK:
            *result = E2E_DDS_OK;
            handle->rxCounter++;
            break;
        case E2E_P_REPEATED:
            *result = E2E_DDS_ERROR_SEQUENCE;
            handle->errorCounter++;
            break;
        case E2E_P_WRONGSEQUENCE:
            *result = E2E_DDS_ERROR_SEQUENCE;
            handle->errorCounter++;
            break;
        case E2E_P_ERROR:
        default:
            if (e2eStatus & E2E_ERROR_CRC) {
                *result = E2E_DDS_ERROR_CRC;
            } else if (e2eStatus & E2E_ERROR_LENGTH) {
                *result = E2E_DDS_ERROR_LENGTH;
            } else {
                *result = E2E_DDS_ERROR_PROFILE;
            }
            handle->errorCounter++;
            break;
    }

    handle->lastRxTimestamp = 0; /* TODO: Get actual timestamp */

    /* Invoke error callback if set */
    if (*result != E2E_DDS_OK && g_callbacks.errorCallback != NULL) {
        g_callbacks.errorCallback(handle->topicId, *result, e2eStatus);
    }

    return E_OK;
}

/**
 * @brief Check received data with E2E validation including state machine
 */
Std_ReturnType E2E_DDS_Check(
    E2E_DDS_TopicHandleType* handle,
    const uint8_t* data,
    uint32_t length,
    uint32_t timestamp,
    E2E_DDS_ResultType* result)
{
    if (!E2E_DDS_IsInitialized() || handle == NULL || !handle->initialized ||
        data == NULL || result == NULL) {
        return E_NOT_OK;
    }

    E2E_PCheckStatusType profileResult;

    /* First do E2E profile check */
    uint16_t e2eStatus = E2E_ERROR_NONE;
    Std_ReturnType ret = E2E_Check(&handle->e2eContext, data, length, &e2eStatus, handle->config.e2eProfile);
    if (ret != E_OK) {
        *result = E2E_DDS_ERROR_INIT;
        return E_NOT_OK;
    }

    /* Get profile result from state */
    switch (handle->config.e2eProfile) {
        case E2E_PROFILE_01:
            profileResult = handle->e2eContext.state.p01.status;
            break;
        case E2E_PROFILE_02:
            profileResult = handle->e2eContext.state.p02.status;
            break;
        case E2E_PROFILE_04:
            profileResult = handle->e2eContext.state.p04.status;
            break;
        case E2E_PROFILE_05:
            profileResult = handle->e2eContext.state.p05.status;
            break;
        case E2E_PROFILE_06:
            profileResult = handle->e2eContext.state.p06.status;
            break;
        case E2E_PROFILE_07:
            profileResult = handle->e2eContext.state.p07.status;
            break;
        case E2E_PROFILE_11:
            profileResult = handle->e2eContext.state.p11.status;
            break;
        case E2E_PROFILE_22:
            profileResult = handle->e2eContext.state.p22.status;
            break;
        default:
            profileResult = E2E_P_ERROR;
            break;
    }

    /* If state machine enabled, process through it */
    if (handle->config.enableStateMachine) {
        E2E_SM_ConfigType smConfig;
        memset(&smConfig, 0, sizeof(E2E_SM_ConfigType));
        smConfig.profile = handle->config.e2eProfile;
        smConfig.timeoutMs = handle->config.timeoutMs;
        smConfig.enableWindowCheck = TRUE;
        smConfig.window.windowSize = handle->config.windowSize;
        smConfig.window.maxErrorThreshold = handle->config.maxErrorThreshold;
        smConfig.window.minOkThreshold = E2E_SM_OK_THRESHOLD;
        smConfig.window.syncCounterThreshold = E2E_SM_SYNC_THRESHOLD;

        E2E_SM_StateType oldState = handle->smState.state;
        E2E_SM_CheckResultType smResult;

        ret = E2E_SM_Check(&handle->smState, &smConfig, profileResult, timestamp, &smResult);
        if (ret != E_OK) {
            *result = E2E_DDS_ERROR_STATE_MACHINE;
            return E_NOT_OK;
        }

        /* Invoke state change callback */
        if (g_callbacks.stateCallback != NULL && oldState != smResult.state) {
            g_callbacks.stateCallback(handle->topicId, oldState, smResult.state);
        }

        /* Map SM state to result */
        if (smResult.isValid) {
            *result = E2E_DDS_OK;
            handle->rxCounter++;
        } else if (smResult.state == E2E_SM_INVALID) {
            *result = E2E_DDS_ERROR_SEQUENCE;
            handle->errorCounter++;
        } else if (smResult.state == E2E_SM_SYNC) {
            *result = E2E_DDS_ERROR_SEQUENCE;
        } else {
            *result = E2E_DDS_ERROR_STATE_MACHINE;
        }
    } else {
        /* Direct profile result mapping */
        switch (profileResult) {
            case E2E_P_OK:
                *result = E2E_DDS_OK;
                handle->rxCounter++;
                break;
            case E2E_P_REPEATED:
                *result = E2E_DDS_ERROR_SEQUENCE;
                handle->errorCounter++;
                break;
            case E2E_P_WRONGSEQUENCE:
                *result = E2E_DDS_ERROR_SEQUENCE;
                handle->errorCounter++;
                break;
            case E2E_P_ERROR:
            default:
                *result = E2E_DDS_ERROR_CRC;
                handle->errorCounter++;
                break;
        }
    }

    /* Invoke error callback if error */
    if (*result != E2E_DDS_OK && g_callbacks.errorCallback != NULL) {
        g_callbacks.errorCallback(handle->topicId, *result, e2eStatus);
    }

    return E_OK;
}

/******************************************************************************
 * Public Functions - Dynamic Length Support
 ******************************************************************************/

/**
 * @brief Set dynamic data length for Profile 22
 */
Std_ReturnType E2E_DDS_SetDynamicLength(
    E2E_DDS_TopicHandleType* handle,
    uint16_t dataLength)
{
    if (!E2E_DDS_IsInitialized() || handle == NULL || !handle->initialized) {
        return E_NOT_OK;
    }

    if (handle->config.e2eProfile != E2E_PROFILE_22) {
        return E_NOT_OK;
    }

    return E2E_P22_SetDataLength(&handle->e2eContext, dataLength);
}

/**
 * @brief Get current data length
 */
Std_ReturnType E2E_DDS_GetDataLength(
    const E2E_DDS_TopicHandleType* handle,
    uint16_t* dataLength)
{
    if (!E2E_DDS_IsInitialized() || handle == NULL || !handle->initialized || dataLength == NULL) {
        return E_NOT_OK;
    }

    if (handle->config.e2eProfile == E2E_PROFILE_22) {
        *dataLength = handle->e2eContext.config.p22.dataLength;
    } else if (handle->config.e2eProfile == E2E_PROFILE_11) {
        *dataLength = handle->e2eContext.config.p11.dataLength;
    } else {
        *dataLength = handle->config.dataLength;
    }

    return E_OK;
}

/******************************************************************************
 * Public Functions - Statistics and Diagnostics
 ******************************************************************************/

/**
 * @brief Get topic statistics
 */
Std_ReturnType E2E_DDS_GetStatistics(
    const E2E_DDS_TopicHandleType* handle,
    uint32_t* txCount,
    uint32_t* rxCount,
    uint32_t* errorCount)
{
    if (!E2E_DDS_IsInitialized() || handle == NULL || !handle->initialized) {
        return E_NOT_OK;
    }

    if (txCount != NULL) {
        *txCount = handle->txCounter;
    }
    if (rxCount != NULL) {
        *rxCount = handle->rxCounter;
    }
    if (errorCount != NULL) {
        *errorCount = handle->errorCounter;
    }

    return E_OK;
}

/**
 * @brief Get current E2E state machine state
 */
Std_ReturnType E2E_DDS_GetState(
    const E2E_DDS_TopicHandleType* handle,
    E2E_SM_StateType* state)
{
    if (!E2E_DDS_IsInitialized() || handle == NULL || !handle->initialized || state == NULL) {
        return E_NOT_OK;
    }

    if (handle->config.enableStateMachine) {
        *state = handle->smState.state;
    } else {
        /* Map profile state to SM state */
        E2E_PCheckStatusType status;
        switch (handle->config.e2eProfile) {
            case E2E_PROFILE_01: status = handle->e2eContext.state.p01.status; break;
            case E2E_PROFILE_02: status = handle->e2eContext.state.p02.status; break;
            case E2E_PROFILE_04: status = handle->e2eContext.state.p04.status; break;
            case E2E_PROFILE_05: status = handle->e2eContext.state.p05.status; break;
            case E2E_PROFILE_06: status = handle->e2eContext.state.p06.status; break;
            case E2E_PROFILE_07: status = handle->e2eContext.state.p07.status; break;
            case E2E_PROFILE_11: status = handle->e2eContext.state.p11.status; break;
            case E2E_PROFILE_22: status = handle->e2eContext.state.p22.status; break;
            default: status = E2E_P_ERROR; break;
        }
        *state = (status == E2E_P_OK) ? E2E_SM_VALID : E2E_SM_INVALID;
    }

    return E_OK;
}

/**
 * @brief Clear topic statistics
 */
Std_ReturnType E2E_DDS_ClearStatistics(E2E_DDS_TopicHandleType* handle)
{
    if (!E2E_DDS_IsInitialized() || handle == NULL || !handle->initialized) {
        return E_NOT_OK;
    }

    handle->txCounter = 0;
    handle->rxCounter = 0;
    handle->errorCounter = 0;

    return E_OK;
}

/******************************************************************************
 * Public Functions - Utility
 ******************************************************************************/

/**
 * @brief Convert E2E DDS result to string
 */
const char* E2E_DDS_ResultToString(E2E_DDS_ResultType result)
{
    switch (result) {
        case E2E_DDS_OK:
            return "OK";
        case E2E_DDS_ERROR_INIT:
            return "INIT_ERROR";
        case E2E_DDS_ERROR_PROFILE:
            return "PROFILE_ERROR";
        case E2E_DDS_ERROR_LENGTH:
            return "LENGTH_ERROR";
        case E2E_DDS_ERROR_CRC:
            return "CRC_ERROR";
        case E2E_DDS_ERROR_SEQUENCE:
            return "SEQUENCE_ERROR";
        case E2E_DDS_ERROR_TIMEOUT:
            return "TIMEOUT_ERROR";
        case E2E_DDS_ERROR_STATE_MACHINE:
            return "STATE_MACHINE_ERROR";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Get version string
 */
const char* E2E_DDS_GetVersion(void)
{
    static char version[32];
    (void)snprintf(version, sizeof(version), "%d.%d.%d",
                     E2E_DDS_MAJOR_VERSION, E2E_DDS_MINOR_VERSION, E2E_DDS_PATCH_VERSION);
    return version;
}

/**
 * @brief Map E2E profile to recommended DDS QoS
 */
Std_ReturnType E2E_DDS_GetRecommendedQos(
    uint8_t profile,
    E2E_DDS_TopicConfigType* config)
{
    if (config == NULL) {
        return E_NOT_OK;
    }

    switch (profile) {
        case E2E_PROFILE_01:
        case E2E_PROFILE_02:
            /* CAN-based, reliable transport */
            config->reliability = DDS_RELIABILITY_RELIABLE;
            config->durability = DDS_DURABILITY_VOLATILE;
            config->historyDepth = 1;
            break;

        case E2E_PROFILE_04:
        case E2E_PROFILE_05:
        case E2E_PROFILE_06:
        case E2E_PROFILE_07:
            /* Ethernet-based, higher throughput */
            config->reliability = DDS_RELIABILITY_RELIABLE;
            config->durability = DDS_DURABILITY_TRANSIENT_LOCAL;
            config->historyDepth = 10;
            break;

        case E2E_PROFILE_11:
            /* Dynamic data, best-effort for low latency */
            config->reliability = DDS_RELIABILITY_BEST_EFFORT;
            config->durability = DDS_DURABILITY_VOLATILE;
            config->historyDepth = 1;
            break;

        case E2E_PROFILE_22:
            /* Large dynamic data, reliable with history */
            config->reliability = DDS_RELIABILITY_RELIABLE;
            config->durability = DDS_DURABILITY_TRANSIENT_LOCAL;
            config->historyDepth = 5;
            break;

        default:
            return E_NOT_OK;
    }

    return E_OK;
}
