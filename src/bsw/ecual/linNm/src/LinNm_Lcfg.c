/**
 * @file LinNm_Lcfg.c
 * @brief LIN Network Management module link-time configuration following AutoSAR Classic Platform 4.x
 * @version 1.0.0
 * @date 2026-05-05
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: LIN Network Management (LINNM)
 * Layer: ECU Abstraction Layer (ECUAL)
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "LinNm.h"
#include "LinNm_Cfg.h"
#include <string.h>

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/

/**
 * @brief LIN NM Channel Configuration Array
 * 
 * This array contains the configuration for each LIN NM channel.
 * Each channel represents one LIN network that participates in network management.
 */
static const LinNm_ChannelConfigType LinNm_ChannelConfig[LINNM_NUMBER_OF_CHANNELS] = {
    /* Channel 0 - Master Channel Configuration */
    {
        .NetworkHandle = LINNM_CH0_NETWORK_HANDLE,                  /**< ComM network handle */
        .LinIfChannelHandle = LINNM_CH0_LINIF_CHANNEL,              /**< LinIf channel reference */
        .NodeId = LINNM_CH0_NODE_ID,                                /**< Node identifier */
        .NodeType = LINNM_CH0_NODE_TYPE,                            /**< Master node */
        .PassiveModeEnabled = LINNM_CH0_PASSIVE_MODE,               /**< Active mode (sends) */
        .StateReportEnabled = LINNM_CH0_STATE_REPORT,               /**< State reporting on */
        .TimeoutTimeMs = LINNM_CH0_TIMEOUT_TIME,                    /**< NM timeout: 100ms */
        .WaitBusSleepTimeMs = LINNM_CH0_WAIT_BUSSLEEP_TIME,         /**< Wait bus sleep: 50ms */
        .RemoteSleepIndTimeMs = LINNM_CH0_REMOTE_SLEEP_IND_TIME,    /**< Remote sleep ind: 500ms */
        .MsgCycleTimeMs = LINNM_CH0_MSG_CYCLE_TIME,                 /**< Message cycle: 20ms */
        .MsgReducedTimeMs = LINNM_CH0_MSG_REDUCED_TIME,             /**< Reduced cycle: 50ms */
        .MsgCycleOffsetMs = LINNM_CH0_MSG_CYCLE_OFFSET,             /**< Cycle offset: 5ms */
        .UserDataLength = LINNM_CH0_USER_DATA_LENGTH,               /**< User data: 6 bytes */
        .BusSynchronizationEnabled = LINNM_CH0_BUS_SYNC_ENABLED,    /**< Bus sync enabled */
        .RemoteSleepIndEnabled = LINNM_CH0_REMOTE_SLEEP_ENABLED,    /**< Remote sleep ind enabled */
        .ComControlEnabled = LINNM_CH0_COM_CONTROL_ENABLED,         /**< Com control enabled */
        .CoordinatorSyncSupport = LINNM_CH0_COORDINATOR_SYNC        /**< Coordinator sync off */
    },
    
#if (LINNM_NUMBER_OF_CHANNELS > 1)
    /* Channel 1 - Slave Channel Configuration */
    {
        .NetworkHandle = LINNM_CH1_NETWORK_HANDLE,                  /**< ComM network handle */
        .LinIfChannelHandle = LINNM_CH1_LINIF_CHANNEL,              /**< LinIf channel reference */
        .NodeId = LINNM_CH1_NODE_ID,                                /**< Node identifier */
        .NodeType = LINNM_CH1_NODE_TYPE,                            /**< Slave node */
        .PassiveModeEnabled = LINNM_CH1_PASSIVE_MODE,               /**< Active mode */
        .StateReportEnabled = LINNM_CH1_STATE_REPORT,               /**< State reporting on */
        .TimeoutTimeMs = LINNM_CH1_TIMEOUT_TIME,                    /**< NM timeout: 100ms */
        .WaitBusSleepTimeMs = LINNM_CH1_WAIT_BUSSLEEP_TIME,         /**< Wait bus sleep: 50ms */
        .RemoteSleepIndTimeMs = LINNM_CH1_REMOTE_SLEEP_IND_TIME,    /**< Remote sleep ind: 500ms */
        .MsgCycleTimeMs = LINNM_CH1_MSG_CYCLE_TIME,                 /**< Message cycle: 20ms */
        .MsgReducedTimeMs = LINNM_CH1_MSG_REDUCED_TIME,             /**< Reduced cycle: 50ms */
        .MsgCycleOffsetMs = LINNM_CH1_MSG_CYCLE_OFFSET,             /**< Cycle offset: 10ms */
        .UserDataLength = LINNM_CH1_USER_DATA_LENGTH,               /**< User data: 6 bytes */
        .BusSynchronizationEnabled = LINNM_CH1_BUS_SYNC_ENABLED,    /**< Bus sync enabled */
        .RemoteSleepIndEnabled = LINNM_CH1_REMOTE_SLEEP_ENABLED,    /**< Remote sleep ind enabled */
        .ComControlEnabled = LINNM_CH1_COM_CONTROL_ENABLED,         /**< Com control enabled */
        .CoordinatorSyncSupport = LINNM_CH1_COORDINATOR_SYNC        /**< Coordinator sync off */
    }
#endif
};

/*==================================================================================================
*                                    CHANNEL RUNTIME DATA
==================================================================================================*/

/**
 * @brief LIN NM Channel Runtime Data Array
 * 
 * This array contains the runtime data for each LIN NM channel.
 * This data is modified during operation by the LinNm module.
 */
static LinNm_ChannelRuntimeType LinNm_ChannelRuntime[LINNM_NUMBER_OF_CHANNELS];

/*==================================================================================================
*                                    GENERAL CONFIGURATION
==================================================================================================*/

/**
 * @brief LIN NM General Configuration
 * 
 * This structure contains global configuration parameters for the LIN NM module.
 */
static const LinNm_GeneralConfigType LinNm_GeneralConfig = {
    .BusSynchronizationEnabled = LINNM_BUS_SYNCHRONIZATION_ENABLED,     /**< Bus sync enabled */
    .ComControlEnabled = LINNM_COM_CONTROL_ENABLED,                     /**< Com control enabled */
    .CoordinatorSyncEnabled = LINNM_COORDINATOR_SYNC_SUPPORT,           /**< Coordinator sync */
    .PassiveModeEnabled = LINNM_PASSIVE_MODE_ENABLED,                   /**< Passive mode */
    .RemoteSleepIndEnabled = LINNM_REMOTE_SLEEP_IND_ENABLED,            /**< Remote sleep ind */
    .StateChangeIndEnabled = LINNM_STATE_CHANGE_IND_ENABLED,            /**< State change ind */
    .UserDataEnabled = LINNM_USER_DATA_ENABLED,                         /**< User data support */
    .NodeIdEnabled = LINNM_NODE_ID_ENABLED,                             /**< Node ID support */
    .NumOfChannels = LINNM_NUMBER_OF_CHANNELS                           /**< Number of channels */
};

/*==================================================================================================
*                                    EXTERNAL CONFIGURATION
==================================================================================================*/

/**
 * @brief LIN NM Configuration Structure (External Reference)
 * 
 * This is the main configuration structure that is passed to LinNm_Init().
 * It provides the module with all necessary configuration and runtime data.
 */
const LinNm_ConfigType LinNm_Config = {
    .GeneralConfig = &LinNm_GeneralConfig,      /**< Pointer to general configuration */
    .ChannelConfig = LinNm_ChannelConfig,       /**< Pointer to channel config array */
    .ChannelRuntime = LinNm_ChannelRuntime      /**< Pointer to channel runtime array */
};

/*==================================================================================================
*                                    POST-BUILD CONFIGURATION VARIANTS
==================================================================================================*/

#if defined(LINNM_CONFIGURATION_VARIANT_POSTBUILD)
/**
 * @brief Alternative configuration variant 1 (Post-build selectable)
 * Used for different network configurations based on ECU variant.
 */
static const LinNm_ChannelConfigType LinNm_ChannelConfig_Variant1[LINNM_NUMBER_OF_CHANNELS] = {
    /* Channel 0 - Master with shorter timeouts for fast wake-up */
    {
        .NetworkHandle = 0U,
        .LinIfChannelHandle = 0U,
        .NodeId = 0x01U,
        .NodeType = LINNM_NODE_TYPE_MASTER,
        .PassiveModeEnabled = STD_OFF,
        .StateReportEnabled = STD_ON,
        .TimeoutTimeMs = 50U,                   /**< Faster timeout: 50ms */
        .WaitBusSleepTimeMs = 25U,              /**< Faster sleep: 25ms */
        .RemoteSleepIndTimeMs = 250U,           /**< Faster ind: 250ms */
        .MsgCycleTimeMs = 10U,                  /**< Faster cycle: 10ms */
        .MsgReducedTimeMs = 25U,                /**< Reduced: 25ms */
        .MsgCycleOffsetMs = 2U,                 /**< Offset: 2ms */
        .UserDataLength = 6U,
        .BusSynchronizationEnabled = STD_ON,
        .RemoteSleepIndEnabled = STD_ON,
        .ComControlEnabled = STD_ON,
        .CoordinatorSyncSupport = STD_OFF
    },
#if (LINNM_NUMBER_OF_CHANNELS > 1)
    {
        .NetworkHandle = 1U,
        .LinIfChannelHandle = 1U,
        .NodeId = 0x02U,
        .NodeType = LINNM_NODE_TYPE_SLAVE,
        .PassiveModeEnabled = STD_OFF,
        .StateReportEnabled = STD_ON,
        .TimeoutTimeMs = 50U,
        .WaitBusSleepTimeMs = 25U,
        .RemoteSleepIndTimeMs = 250U,
        .MsgCycleTimeMs = 10U,
        .MsgReducedTimeMs = 25U,
        .MsgCycleOffsetMs = 5U,
        .UserDataLength = 6U,
        .BusSynchronizationEnabled = STD_ON,
        .RemoteSleepIndEnabled = STD_ON,
        .ComControlEnabled = STD_ON,
        .CoordinatorSyncSupport = STD_OFF
    }
#endif
};

/**
 * @brief Alternative configuration variant 2 (Post-build selectable)
 * Used for diagnostic/debug mode with extended user data.
 */
static const LinNm_ChannelConfigType LinNm_ChannelConfig_Variant2[LINNM_NUMBER_OF_CHANNELS] = {
    /* Channel 0 - Master with extended user data */
    {
        .NetworkHandle = 0U,
        .LinIfChannelHandle = 0U,
        .NodeId = 0x01U,
        .NodeType = LINNM_NODE_TYPE_MASTER,
        .PassiveModeEnabled = STD_OFF,
        .StateReportEnabled = STD_ON,
        .TimeoutTimeMs = 200U,                  /**< Extended timeout: 200ms */
        .WaitBusSleepTimeMs = 100U,             /**< Extended sleep: 100ms */
        .RemoteSleepIndTimeMs = 1000U,          /**< Extended ind: 1000ms */
        .MsgCycleTimeMs = 50U,                  /**< Extended cycle: 50ms */
        .MsgReducedTimeMs = 100U,               /**< Reduced: 100ms */
        .MsgCycleOffsetMs = 10U,                /**< Offset: 10ms */
        .UserDataLength = 8U,                   /**< Full user data: 8 bytes */
        .BusSynchronizationEnabled = STD_ON,
        .RemoteSleepIndEnabled = STD_ON,
        .ComControlEnabled = STD_ON,
        .CoordinatorSyncSupport = STD_OFF
    },
#if (LINNM_NUMBER_OF_CHANNELS > 1)
    {
        .NetworkHandle = 1U,
        .LinIfChannelHandle = 1U,
        .NodeId = 0x02U,
        .NodeType = LINNM_NODE_TYPE_SLAVE,
        .PassiveModeEnabled = STD_OFF,
        .StateReportEnabled = STD_ON,
        .TimeoutTimeMs = 200U,
        .WaitBusSleepTimeMs = 100U,
        .RemoteSleepIndTimeMs = 1000U,
        .MsgCycleTimeMs = 50U,
        .MsgReducedTimeMs = 100U,
        .MsgCycleOffsetMs = 20U,
        .UserDataLength = 8U,
        .BusSynchronizationEnabled = STD_ON,
        .RemoteSleepIndEnabled = STD_ON,
        .ComControlEnabled = STD_ON,
        .CoordinatorSyncSupport = STD_OFF
    }
#endif
};

/**
 * @brief Configuration structure variant 1 (Fast timing)
 */
const LinNm_ConfigType LinNm_Config_Fast = {
    .GeneralConfig = &LinNm_GeneralConfig,
    .ChannelConfig = LinNm_ChannelConfig_Variant1,
    .ChannelRuntime = LinNm_ChannelRuntime
};

/**
 * @brief Configuration structure variant 2 (Extended mode)
 */
const LinNm_ConfigType LinNm_Config_Extended = {
    .GeneralConfig = &LinNm_GeneralConfig,
    .ChannelConfig = LinNm_ChannelConfig_Variant2,
    .ChannelRuntime = LinNm_ChannelRuntime
};
#endif /* LINNM_CONFIGURATION_VARIANT_POSTBUILD */

/*==================================================================================================
*                                    CHANNEL MAPPING TABLES
==================================================================================================*/

/**
 * @brief Network Handle to Channel Index Mapping Table
 * 
 * This table provides fast lookup from ComM network handle to internal channel index.
 */
static const uint8 LinNm_NetworkHandleToIndex[LINNM_NUMBER_OF_CHANNELS] = {
    LINNM_CHANNEL_0,
#if (LINNM_NUMBER_OF_CHANNELS > 1)
    LINNM_CHANNEL_1
#endif
};

/**
 * @brief LinIf Channel Handle to Channel Index Mapping Table
 * 
 * This table provides fast lookup from LinIf channel handle to internal channel index.
 */
static const uint8 LinNm_LinIfChannelToIndex[LINNM_NUMBER_OF_CHANNELS] = {
    LINNM_CHANNEL_0,
#if (LINNM_NUMBER_OF_CHANNELS > 1)
    LINNM_CHANNEL_1
#endif
};

/*==================================================================================================
*                                    TIMING CONSTANTS
==================================================================================================*/

/**
 * @brief LIN NM Timing Constants Table
 * 
 * This table provides centralized access to timing parameters for debugging/monitoring.
 */
const uint16 LinNm_TimingConstants[] = {
    LINNM_TIMEOUT_TIME,              /**< NM timeout time (ms) */
    LINNM_WAIT_BUSSLEEP_TIME,        /**< Wait bus sleep time (ms) */
    LINNM_REMOTE_SLEEP_IND_TIME,     /**< Remote sleep indication time (ms) */
    LINNM_MSG_CYCLE_TIME,            /**< Message cycle time (ms) */
    LINNM_MSG_REDUCED_TIME,          /**< Reduced message cycle time (ms) */
    LINNM_MSG_CYCLE_OFFSET,          /**< Message cycle offset (ms) */
    LINNM_REPEAT_MESSAGE_TIME        /**< Repeat message time (ms) */
};

/**
 * @brief Number of timing constants
 */
const uint8 LinNm_NumTimingConstants = 7U;

/*==================================================================================================
*                                    USER DATA DEFAULTS
==================================================================================================*/

/**
 * @brief Default user data values
 * Used to initialize user data buffer at startup.
 */
static const uint8 LinNm_DefaultUserData[LINNM_PDU_USER_DATA_SIZE] = {
    0x00U,  /**< Byte 0 - Reserved */
    0x00U,  /**< Byte 1 - Reserved */
    0x00U,  /**< Byte 2 - Reserved */
    0x00U,  /**< Byte 3 - Reserved */
    0x00U,  /**< Byte 4 - Reserved */
    0x00U   /**< Byte 5 - Reserved */
};

/*==================================================================================================
*                                    HELPER FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize channel runtime data
 * 
 * This function initializes the runtime data for all channels.
 * Called by LinNm_Init() during module initialization.
 */
static void LinNm_InitChannelRuntime(void)
{
    uint8 i;
    uint8 j;
    
    for (i = 0; i < LINNM_NUMBER_OF_CHANNELS; i++) {
        /* Initialize state machine */
        LinNm_ChannelRuntime[i].State = LINNM_STATE_BUS_SLEEP;
        LinNm_ChannelRuntime[i].Mode = LINNM_BUSNM_MODE_BUS_SLEEP;
        
        /* Initialize flags */
        LinNm_ChannelRuntime[i].CommunicationEnabled = FALSE;
        LinNm_ChannelRuntime[i].RemoteSleepIndication = FALSE;
        LinNm_ChannelRuntime[i].RemoteSleepIndStatus = FALSE;
        LinNm_ChannelRuntime[i].BusSynchronizationActive = FALSE;
        LinNm_ChannelRuntime[i].NetworkRequested = FALSE;
        LinNm_ChannelRuntime[i].StateChanged = FALSE;
        LinNm_ChannelRuntime[i].BusLoadReductionActive = FALSE;
        
        /* Initialize timers */
        LinNm_ChannelRuntime[i].TimeoutTimer = 0U;
        LinNm_ChannelRuntime[i].RemoteSleepTimer = 0U;
        LinNm_ChannelRuntime[i].MessageCycleTimer = 0U;
        LinNm_ChannelRuntime[i].RepeatMessageCounter = 0U;
        
        /* Initialize user data */
        LinNm_ChannelRuntime[i].UserDataLength = LinNm_ChannelConfig[i].UserDataLength;
        for (j = 0; j < 8U; j++) {
            if (j < LINNM_PDU_USER_DATA_SIZE) {
                LinNm_ChannelRuntime[i].UserData[j] = LinNm_DefaultUserData[j];
            } else {
                LinNm_ChannelRuntime[i].UserData[j] = 0x00U;
            }
        }
    }
}

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/

/**
 * @brief Nm State Change Callback Configuration
 * 
 * Determines if the Nm_StateChangeNotification callback is invoked.
 */
#if (LINNM_STATE_CHANGE_IND_ENABLED == STD_ON)
const boolean LinNm_StateChangeCallbackEnabled = TRUE;
#else
const boolean LinNm_StateChangeCallbackEnabled = FALSE;
#endif

/**
 * @brief Nm Remote Sleep Callback Configuration
 * 
 * Determines if the Nm_RemoteSleepIndication/Cancellation callbacks are invoked.
 */
#if (LINNM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
const boolean LinNm_RemoteSleepCallbackEnabled = TRUE;
#else
const boolean LinNm_RemoteSleepCallbackEnabled = FALSE;
#endif

/**
 * @brief Nm Synchronization Point Callback Configuration
 * 
 * Determines if the Nm_SynchronizationPoint callback is invoked.
 */
#if (LINNM_SYNC_POINT_CALLBACK == STD_ON)
const boolean LinNm_SyncPointCallbackEnabled = TRUE;
#else
const boolean LinNm_SyncPointCallbackEnabled = FALSE;
#endif

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/

/**
 * @brief Module Version Information (Read-only)
 * 
 * This structure provides version information for diagnostic purposes.
 */
const Std_VersionInfoType LinNm_VersionInfo = {
    .vendorID = LINNM_VENDOR_ID,
    .moduleID = LINNM_MODULE_ID,
    .sw_major_version = LINNM_SW_MAJOR_VERSION,
    .sw_minor_version = LINNM_SW_MINOR_VERSION,
    .sw_patch_version = LINNM_SW_PATCH_VERSION
};

/**
 * @brief Configuration Version Information
 */
const Std_VersionInfoType LinNm_CfgVersionInfo = {
    .vendorID = LINNM_CFG_VENDOR_ID,
    .moduleID = LINNM_CFG_MODULE_ID,
    .sw_major_version = LINNM_CFG_SW_MAJOR_VERSION,
    .sw_minor_version = LINNM_CFG_SW_MINOR_VERSION,
    .sw_patch_version = LINNM_CFG_SW_PATCH_VERSION
};
