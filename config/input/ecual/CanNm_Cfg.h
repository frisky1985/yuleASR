/**
 * @file CanNm_Cfg.h
 * @brief CAN Network Management configuration header file
 * @version 4.4.0
 *
 * AUTOSAR CAN Network Management Module Configuration
 * Following AUTOSAR_SWS_CANNetworkManagement specification version 4.4.0
 */

#ifndef CANNM_CFG_H
#define CANNM_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                    PRE-COMPILE CONFIGURATION
 ==================================================================================================*/

/**
 * @brief Vendor and module identification
 */
#define CANNM_VENDOR_ID_CFG                 (uint16)0x0001U
#define CANNM_MODULE_ID_CFG                 (uint16)0x001FU

/**
 * @brief Software version
 */
#define CANNM_CFG_SW_MAJOR_VERSION          (uint8)4U
#define CANNM_CFG_SW_MINOR_VERSION          (uint8)4U
#define CANNM_CFG_SW_PATCH_VERSION          (uint8)0U

/**
 * @brief AUTOSAR version
 */
#define CANNM_CFG_AR_MAJOR_VERSION          (uint8)4U
#define CANNM_CFG_AR_MINOR_VERSION          (uint8)4U
#define CANNM_CFG_AR_PATCH_VERSION          (uint8)0U

/*==================================================================================================
 *                                    FEATURE SWITCHES
 ==================================================================================================*/

/**
 * @brief Enable/disable version info API
 * STD_ON: CanNm_GetVersionInfo API is available
 * STD_OFF: CanNm_GetVersionInfo API is not available
 */
#define CANNM_VERSION_INFO_API              STD_ON

/**
 * @brief Enable/disable communication control
 * STD_ON: CanNm_DisableCommunication and CanNm_EnableCommunication APIs available
 * STD_OFF: Communication control APIs not available
 */
#define CANNM_COM_CONTROL_ENABLED           STD_ON

/**
 * @brief Enable/disable coordinator support
 * STD_ON: Coordinator synchronization feature enabled
 * STD_OFF: Coordinator synchronization feature disabled
 */
#define CANNM_COORDINATOR_SUPPORT_ENABLED   STD_ON

/**
 * @brief Enable/disable passive mode
 * STD_ON: Channel operates in passive mode (no transmission)
 * STD_OFF: Normal operation mode
 */
#define CANNM_PASSIVE_MODE_ENABLED          STD_OFF

/**
 * @brief Enable/disable node detection
 * STD_ON: Node detection enabled (Repeat Message Request supported)
 * STD_OFF: Node detection disabled
 */
#define CANNM_NODE_DETECTION_ENABLED        STD_ON

/**
 * @brief Enable/disable node ID
 * STD_ON: Node ID in NM PDU enabled
 * STD_OFF: Node ID in NM PDU disabled
 */
#define CANNM_NODE_ID_ENABLED               STD_ON

/**
 * @brief Enable/disable remote sleep indication
 * STD_ON: Remote sleep indication detection enabled
 * STD_OFF: Remote sleep indication detection disabled
 */
#define CANNM_REMOTE_SLEEP_IND_ENABLED      STD_ON

/**
 * @brief Enable/disable user data support
 * STD_ON: User data in NM PDU enabled
 * STD_OFF: User data in NM PDU disabled
 */
#define CANNM_USER_DATA_ENABLED             STD_ON

/**
 * @brief Enable/disable partial networking
 * STD_ON: Partial networking feature enabled
 * STD_OFF: Partial networking feature disabled
 */
#define CANNM_PN_ENABLED                    STD_ON

/**
 * @brief Enable/disable state change indication
 * STD_ON: State change notifications to upper layer enabled
 * STD_OFF: State change notifications disabled
 */
#define CANNM_STATE_CHANGE_IND_ENABLED      STD_ON

/**
 * @brief Enable/disable bus synchronization
 * STD_ON: Bus synchronization feature enabled
 * STD_OFF: Bus synchronization feature disabled
 */
#define CANNM_BUS_SYNCHRONIZATION_ENABLED   STD_OFF

/**
 * @brief Enable/disable remote sleep indication callback
 * STD_ON: Remote sleep indication callback to upper layer enabled
 * STD_OFF: Remote sleep indication callback disabled
 */
#define CANNM_REMOTE_SLEEP_IND_CALLBACK     STD_ON

/**
 * @brief Enable/disable Car Wakeup support
 * STD_ON: Car Wakeup handling enabled
 * STD_OFF: Car Wakeup handling disabled
 */
#define CANNM_CAR_WAKEUP_RX_ENABLED         STD_OFF

/**
 * @brief Enable/disable immediate transmission
 * STD_ON: Immediate NM transmission on network request enabled
 * STD_OFF: Immediate transmission disabled
 */
#define CANNM_IMMEDIATE_TRANSMIT_ENABLED    STD_ON

/**
 * @brief Enable/disable active wakeup bit
 * STD_ON: Active wakeup bit in CBV enabled
 * STD_OFF: Active wakeup bit disabled
 */
#define CANNM_ACTIVE_WAKEUP_BIT_ENABLED     STD_ON

/**
 * @brief Enable/disable PDU Rx indication
 * STD_ON: CanNm_RxIndication callback available
 * STD_OFF: CanNm_RxIndication callback not available
 */
#define CANNM_PDU_RX_INDICATION_ENABLED     STD_ON

/**
 * @brief Enable/disable development error detection
 * STD_ON: Development error detection enabled (DET module used)
 * STD_OFF: Development error detection disabled
 */
#define CANNM_DEV_ERROR_DETECT              STD_ON

/*==================================================================================================
 *                                    NUMBER OF CHANNELS
 ==================================================================================================*/

/**
 * @brief Number of CAN NM channels configured
 */
#define CANNM_NUMBER_OF_CHANNELS            (uint8)2U

/**
 * @brief Maximum number of channels (should match CANNM_NUMBER_OF_CHANNELS)
 */
#define CANNM_MAX_NUMBER_OF_CHANNELS        (uint8)2U

/*==================================================================================================
 *                                    CHANNEL CONFIGURATION
 ==================================================================================================*/

/**
 * @brief Channel handles (mapped to ComM channel handles)
 */
#define CANNM_CHANNEL_0                     (NetworkHandleType)0U
#define CANNM_CHANNEL_1                     (NetworkHandleType)1U

/**
 * @brief Channel names (for debugging)
 */
#define CANNM_CHANNEL_0_NAME                "CanNm_Channel_0"
#define CANNM_CHANNEL_1_NAME                "CanNm_Channel_1"

/*==================================================================================================
 *                                    TIMING PARAMETERS (in ms)
 ==================================================================================================*/

/**
 * @brief NM timeout time (NmTimeoutTime)
 * Time for NM messages on the bus before considering bus-off
 * Range: 10ms - 5000ms
 * Typical: 1000ms
 */
#define CANNM_NM_TIMEOUT_TIME_DEFAULT       (uint16)1000U

/**
 * @brief Repeat message time (RepeatMessageTime)
 * Duration of Repeat Message State
 * Range: 10ms - 65535ms
 * Typical: 400ms
 */
#define CANNM_REPEAT_MESSAGE_TIME_DEFAULT   (uint16)400U

/**
 * @brief Wait bus sleep time (WaitBusSleepTime)
 * Duration of Prepare Bus Sleep State
 * Range: 10ms - 65535ms
 * Typical: 1000ms
 */
#define CANNM_WAIT_BUS_SLEEP_TIME_DEFAULT   (uint16)1000U

/**
 * @brief Message cycle time (MessageCycleTime)
 * Period between NM message transmissions
 * Range: 10ms - 65535ms
 * Typical: 100ms
 */
#define CANNM_MESSAGE_CYCLE_TIME_DEFAULT    (uint16)100U

/**
 * @brief Message cycle offset (MessageCycleOffset)
 * Offset for first transmission after bus-off
 * Range: 0ms - 65535ms
 * Typical: 10ms
 */
#define CANNM_MESSAGE_CYCLE_OFFSET_DEFAULT  (uint16)10U

/**
 * @brief Immediate NM cycle time (ImmediateNmCycleTime)
 * Cycle time for immediate transmissions
 * Range: 10ms - 65535ms
 * Typical: 20ms
 */
#define CANNM_IMMEDIATE_NM_CYCLE_TIME       (uint16)20U

/**
 * @brief Number of immediate NM transmissions
 * Number of immediate transmissions after bus wakeup
 * Range: 1-255
 * Typical: 3
 */
#define CANNM_IMMEDIATE_NM_TRANSMSSIONS     (uint8)3U

/**
 * @brief Remote sleep indication time
 * Time for remote sleep indication detection
 * Range: 10ms - 65535ms
 * Typical: 2000ms
 */
#define CANNM_REMOTE_SLEEP_IND_TIME         (uint16)2000U

/*==================================================================================================
 *                                    PDU CONFIGURATION
 ==================================================================================================*/

/**
 * @brief PDU length (bytes)
 * Standard CAN NM PDU length is 8 bytes
 */
#define CANNM_PDU_LENGTH                    (uint8)8U

/**
 * @brief Node ID byte position in NM PDU
 * Position 0: First byte
 * Position 1: Second byte
 * Position 0xFF: Node ID not used
 */
#define CANNM_PDU_NID_POSITION              (uint8)0U

/**
 * @brief Control Bit Vector (CBV) position in NM PDU
 * Position 0: First byte
 * Position 1: Second byte
 * Position 0xFF: CBV not used
 */
#define CANNM_PDU_CBV_POSITION              (uint8)1U

/**
 * @brief User data start position
 * User data starts after NID and CBV if enabled
 */
#if (CANNM_PDU_NID_POSITION == 0U) && (CANNM_PDU_CBV_POSITION == 1U)
    #define CANNM_USER_DATA_POSITION        (uint8)2U
    #define CANNM_USER_DATA_LENGTH          (uint8)6U
#elif (CANNM_PDU_NID_POSITION == 0U) || (CANNM_PDU_CBV_POSITION == 0U)
    #define CANNM_USER_DATA_POSITION        (uint8)1U
    #define CANNM_USER_DATA_LENGTH          (uint8)7U
#else
    #define CANNM_USER_DATA_POSITION        (uint8)0U
    #define CANNM_USER_DATA_LENGTH          (uint8)8U
#endif

/*==================================================================================================
 *                                    NODE CONFIGURATION
 ==================================================================================================*/

/**
 * @brief Node IDs for each channel
 */
#define CANNM_CHANNEL_0_NODE_ID             (uint8)0x01U
#define CANNM_CHANNEL_1_NODE_ID             (uint8)0x02U

/*==================================================================================================
 *                                    TX PDU CONFIGURATION
 ==================================================================================================*/

/**
 * @brief Transmit PDU IDs for each channel
 * These are handles for CanIf_Transmit calls
 */
#define CANNM_CHANNEL_0_TX_PDUID            (PduIdType)0U
#define CANNM_CHANNEL_1_TX_PDUID            (PduIdType)1U

/**
 * @brief Receive PDU IDs for each channel
 * These are handles for CanNm_RxIndication callback
 */
#define CANNM_CHANNEL_0_RX_PDUID            (PduIdType)0U
#define CANNM_CHANNEL_1_RX_PDUID            (PduIdType)1U

/*==================================================================================================
 *                                    CALLBACK CONFIGURATION
 ===================================================================================================*/

/**
 * @brief State change callback function name
 * Called when NM state changes
 */
#define CANNM_STATE_CHANGE_IND_FCT          "Nm_StateChangeNotification"

/**
 * @brief Remote sleep indication callback function name
 * Called when remote sleep is detected
 */
#define CANNM_REMOTE_SLEEP_IND_FCT          "Nm_RemoteSleepIndication"

/**
 * @brief Remote sleep cancellation callback function name
 * Called when remote sleep is cancelled
 */
#define CANNM_REMOTE_SLEEP_CANCEL_FCT       "Nm_RemoteSleepCancellation"

/*==================================================================================================
 *                                    MAIN FUNCTION PERIOD
 ==================================================================================================*/

/**
 * @brief Main function period in seconds (as floating point)
 * Used for timer calculations
 */
#define CANNM_MAIN_FUNCTION_PERIOD          (0.01f)  /**< 10ms */

/**
 * @brief Main function period in milliseconds
 */
#define CANNM_MAIN_FUNCTION_PERIOD_MS       (uint16)10U

#ifdef __cplusplus
}
#endif

#endif /* CANNM_CFG_H */
