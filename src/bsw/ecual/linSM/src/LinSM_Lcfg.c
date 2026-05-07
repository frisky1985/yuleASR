/************************************************************************************
 * File: LinSM_Lcfg.c
 * Description: LIN State Manager - Link-time Configuration
 * AUTOSAR Version: 4.4.0
 *
 * Module: LinSM (LIN State Manager)
 * Purpose: Link-time configuration tables for channels, schedules, and timing parameters
 ************************************************************************************/

#include "LinSM.h"

/*================================================================================
 * Schedule Table ID Arrays
 *===============================================================================*/

/**
 * @brief Schedule table IDs for LIN Channel 0
 */
static const uint8 LinSM_Channel0_Schedules[LINSM_SCHEDULE_COUNT_PER_CHANNEL] =
{
    LINSM_SCHEDULE_NULL,           /* Schedule 0: Null schedule */
    LINSM_SCHEDULE_DIAG_REQUEST,   /* Schedule 1: Diagnostic request */
    LINSM_SCHEDULE_NORMAL,         /* Schedule 2: Normal communication */
    LINSM_SCHEDULE_MASTER          /* Schedule 3: Master command */
};

/**
 * @brief Schedule table IDs for LIN Channel 1
 */
static const uint8 LinSM_Channel1_Schedules[LINSM_SCHEDULE_COUNT_PER_CHANNEL] =
{
    LINSM_SCHEDULE_NULL,            /* Schedule 0: Null schedule */
    LINSM_SCHEDULE_DIAG_REQUEST,    /* Schedule 1: Diagnostic request */
    LINSM_SCHEDULE_DIAG_RESPONSE,   /* Schedule 2: Diagnostic response */
    LINSM_SCHEDULE_NORMAL           /* Schedule 3: Normal communication */
};

/*================================================================================
 * Channel Configuration
 *===============================================================================*/

/**
 * @brief LIN Channel Configuration Table
 *
 * Configuration for each LIN channel including:
 * - Channel ID
 * - ComM channel mapping
 * - Timeout parameters
 * - Schedule table definitions
 * - Wake-up support
 */
static const LinSM_ChannelConfigType LinSM_ChannelConfig[LINSM_CHANNEL_COUNT] =
{
    /* Channel 0 Configuration */
    {
        .ChannelId = LINSM_CHANNEL_0,                   /* LIN channel 0 */
        .ComMChannelId = LINSM_COMM_CHANNEL_0,          /* ComM channel mapping */
        .ConfirmationTimeout = LINSM_SCHEDULE_CONFIRMATION_TIMEOUT, /* 1000ms */
        .ModeRequestRepetitionTime = LINSM_MODE_REQUEST_REPETITION_TIME, /* 50ms */
        .ScheduleCount = LINSM_SCHEDULE_COUNT_PER_CHANNEL, /* 4 schedules */
        .ScheduleIdList = LinSM_Channel0_Schedules,     /* Schedule list */
        .WakeupSupport = TRUE,                          /* Wake-up supported */
        .WakeupSource = LINSM_WAKEUP_SOURCE_CH0         /* ECUM_WKSOURCE_LIN_CH0 */
    },

    /* Channel 1 Configuration */
    {
        .ChannelId = LINSM_CHANNEL_1,                   /* LIN channel 1 */
        .ComMChannelId = LINSM_COMM_CHANNEL_1,          /* ComM channel mapping */
        .ConfirmationTimeout = LINSM_SCHEDULE_CONFIRMATION_TIMEOUT, /* 1000ms */
        .ModeRequestRepetitionTime = LINSM_MODE_REQUEST_REPETITION_TIME, /* 50ms */
        .ScheduleCount = LINSM_SCHEDULE_COUNT_PER_CHANNEL, /* 4 schedules */
        .ScheduleIdList = LinSM_Channel1_Schedules,     /* Schedule list */
        .WakeupSupport = TRUE,                          /* Wake-up supported */
        .WakeupSource = LINSM_WAKEUP_SOURCE_CH1         /* ECUM_WKSOURCE_LIN_CH1 */
    }
};

/*================================================================================
 * Wake-up Source Configuration
 *===============================================================================*/

#if (LINSM_WAKEUP_SUPPORT == STD_ON)
/**
 * @brief Wake-up Source Configuration Table
 *
 * Maps LIN channels to EcuM wake-up sources
 */
static const LinSM_WakeupSourceConfigType LinSM_WakeupSources[LINSM_WAKEUP_SOURCE_COUNT] =
{
    /* Channel 0 Wake-up Source */
    {
        .WakeupSource = LINSM_WAKEUP_SOURCE_CH0,        /* LIN Channel 0 wake-up */
        .ChannelId = LINSM_CHANNEL_0                    /* Maps to channel 0 */
    },

    /* Channel 1 Wake-up Source */
    {
        .WakeupSource = LINSM_WAKEUP_SOURCE_CH1,        /* LIN Channel 1 wake-up */
        .ChannelId = LINSM_CHANNEL_1                    /* Maps to channel 1 */
    }
};
#endif

/*================================================================================
 * Global Configuration
 *===============================================================================*/

/**
 * @brief Global LIN State Manager Configuration
 *
 * This is the main configuration structure used by LinSM_Init().
 */
const LinSM_ConfigType LinSM_Config =
{
    .ChannelCount = LINSM_CHANNEL_COUNT,                /* 2 channels */
    .ChannelConfig = LinSM_ChannelConfig,               /* Channel configuration */
#if (LINSM_WAKEUP_SUPPORT == STD_ON)
    .WakeupSources = LinSM_WakeupSources,               /* Wake-up sources */
    .WakeupSourceCount = LINSM_WAKEUP_SOURCE_COUNT      /* 2 wake-up sources */
#else
    .WakeupSources = NULL_PTR,
    .WakeupSourceCount = 0U
#endif
};

/*================================================================================
 * Schedule Table Timing Parameters
 *===============================================================================*/

/**
 * @brief Schedule Table Timing Configuration
 *
 * Defines timing parameters for each schedule table.
 * This table is optional and can be used for detailed timing control.
 */
typedef struct
{
    uint8  ScheduleId;           /* Schedule table identifier */
    uint16 EntryDelay;           /* Entry point delay in ms */
    uint16 ResumeDelay;          /* Resume delay in ms */
    boolean IsEventTriggered;    /* Event-triggered flag */
} LinSM_ScheduleTimingType;

/**
 * @brief Schedule timing for Channel 0
 */
static const LinSM_ScheduleTimingType LinSM_Channel0_Timing[LINSM_SCHEDULE_COUNT_PER_CHANNEL] =
{
    /* Schedule 0 - Null */
    {
        .ScheduleId = LINSM_SCHEDULE_NULL,
        .EntryDelay = 0U,
        .ResumeDelay = 0U,
        .IsEventTriggered = FALSE
    },
    /* Schedule 1 - Diagnostic Request */
    {
        .ScheduleId = LINSM_SCHEDULE_DIAG_REQUEST,
        .EntryDelay = 10U,
        .ResumeDelay = 5U,
        .IsEventTriggered = TRUE
    },
    /* Schedule 2 - Normal */
    {
        .ScheduleId = LINSM_SCHEDULE_NORMAL,
        .EntryDelay = 0U,
        .ResumeDelay = 0U,
        .IsEventTriggered = FALSE
    },
    /* Schedule 3 - Master */
    {
        .ScheduleId = LINSM_SCHEDULE_MASTER,
        .EntryDelay = 5U,
        .ResumeDelay = 2U,
        .IsEventTriggered = FALSE
    }
};

/**
 * @brief Schedule timing for Channel 1
 */
static const LinSM_ScheduleTimingType LinSM_Channel1_Timing[LINSM_SCHEDULE_COUNT_PER_CHANNEL] =
{
    /* Schedule 0 - Null */
    {
        .ScheduleId = LINSM_SCHEDULE_NULL,
        .EntryDelay = 0U,
        .ResumeDelay = 0U,
        .IsEventTriggered = FALSE
    },
    /* Schedule 1 - Diagnostic Request */
    {
        .ScheduleId = LINSM_SCHEDULE_DIAG_REQUEST,
        .EntryDelay = 10U,
        .ResumeDelay = 5U,
        .IsEventTriggered = TRUE
    },
    /* Schedule 2 - Diagnostic Response */
    {
        .ScheduleId = LINSM_SCHEDULE_DIAG_RESPONSE,
        .EntryDelay = 10U,
        .ResumeDelay = 5U,
        .IsEventTriggered = TRUE
    },
    /* Schedule 3 - Normal */
    {
        .ScheduleId = LINSM_SCHEDULE_NORMAL,
        .EntryDelay = 0U,
        .ResumeDelay = 0U,
        .IsEventTriggered = FALSE
    }
};

/*================================================================================
 * Configuration Pointer Array
 *===============================================================================*/

/**
 * @brief Configuration pointer array for multi-configuration support
 *
 * Supports post-build configuration selection
 */
const LinSM_ConfigType * const LinSM_ConfigPtrs[1] =
{
    &LinSM_Config
};

/*================================================================================
 * Post-Build Configuration Variant Support
 *===============================================================================*/

#if (LINSM_POSTBUILD_VARIANT_SUPPORT == STD_ON)
/**
 * @brief Get configuration for specific variant
 * @param variantIdx - Configuration variant index
 * @return Pointer to configuration structure
 */
const LinSM_ConfigType * LinSM_GetConfig(uint8 variantIdx)
{
    if (variantIdx < 1U)
    {
        return LinSM_ConfigPtrs[variantIdx];
    }
    return NULL_PTR;
}
#endif

/*================================================================================
 * Module Information
 *===============================================================================*/

/**
 * @brief Module version information
 */
static const Std_VersionInfoType LinSM_VersionInfo =
{
    .vendorID = LINSM_VENDOR_ID,
    .moduleID = LINSM_MODULE_ID,
    .sw_major_version = LINSM_SW_MAJOR_VERSION,
    .sw_minor_version = LINSM_SW_MINOR_VERSION,
    .sw_patch_version = LINSM_SW_PATCH_VERSION
};

/**
 * @brief Get module version information
 * @return Pointer to version info structure
 */
const Std_VersionInfoType * LinSM_GetCompiledVersionInfo(void)
{
    return &LinSM_VersionInfo;
}
