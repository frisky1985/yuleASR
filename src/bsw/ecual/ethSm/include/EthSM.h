/**
 * @file EthSM.h
 * @brief Ethernet State Manager (EthSM) API Header
 * @version 1.0.0
 * @date 2026-05-05
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Ethernet State Manager (ETHSM)
 * Layer: ECU Abstraction Layer (ECUAL)
 * AUTOSAR Version: 4.4.0
 *
 * Description:
 * The Ethernet State Manager manages the communication states of Ethernet networks.
 * It implements a state machine that handles transitions between NO_COM, WAIT_TRCVLINK,
 * WAIT_ONLINE, and COM_READY states.
 */

#ifndef ETHSM_H
#define ETHSM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "EthSM_Cfg.h"
#include "ComStack_Types.h"
#include "ComM.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define ETHSM_VENDOR_ID                     (0x01U) /* YuleTech Vendor ID */
#define ETHSM_MODULE_ID                     (0x43U) /* ETHSM Module ID per AUTOSAR */
#define ETHSM_AR_RELEASE_MAJOR_VERSION      (0x04U)
#define ETHSM_AR_RELEASE_MINOR_VERSION      (0x04U)
#define ETHSM_AR_RELEASE_REVISION_VERSION   (0x00U)
#define ETHSM_SW_MAJOR_VERSION              (0x01U)
#define ETHSM_SW_MINOR_VERSION              (0x00U)
#define ETHSM_SW_PATCH_VERSION              (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define ETHSM_SID_INIT                      (0x01U)
#define ETHSM_SID_DEINIT                    (0x02U)
#define ETHSM_SID_REQUESTCOMMODE            (0x03U)
#define ETHSM_SID_GETCURRENTCOMMODE         (0x04U)
#define ETHSM_SID_MAINFUNCTION              (0x05U)
#define ETHSM_SID_GETVERSIONINFO            (0x06U)
#define ETHSM_SID_TCPIPMODEINDICATION       (0x07U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define ETHSM_E_NOT_INITIALIZED             (0x01U)
#define ETHSM_E_INVALID_NETWORK_HANDLE      (0x02U)
#define ETHSM_E_INVALID_POINTER             (0x03U)
#define ETHSM_E_INVALID_PARAMETER           (0x04U)
#define ETHSM_E_ALREADY_INITIALIZED         (0x05U)
#define ETHSM_E_NOT_SUPPORTED               (0x06U)
#define ETHSM_E_TCPIP_MODE_FAILED           (0x07U)
#define ETHSM_E_TRANSCEIVER_ERROR           (0x08U)

/*==================================================================================================
*                                    TIMEOUT VALUES
==================================================================================================*/
#define ETHSM_DEFAULT_TIMEOUT_WAIT_TRCVLINK (100U)  /* 100ms default */
#define ETHSM_DEFAULT_TIMEOUT_WAIT_ONLINE   (5000U) /* 5s default */
#define ETHSM_MAIN_FUNCTION_CYCLE_MS        (10U)   /* 10ms cycle time */

/*==================================================================================================
*                                    NETWORK STATE TYPE
==================================================================================================*/
/**
 * @brief Ethernet State Manager State Type
 * @details Represents the internal state of the EthSM state machine
 */
typedef enum {
    ETHSM_STATE_UNINIT = 0,         /**< Module not initialized */
    ETHSM_STATE_NO_COM,             /**< No communication requested */
    ETHSM_STATE_WAIT_TRCVLINK,      /**< Waiting for transceiver link up */
    ETHSM_STATE_WAIT_ONLINE,        /**< Waiting for TcpIp online */
    ETHSM_STATE_ONHOLD,             /**< Communication on hold */
    ETHSM_STATE_COM_READY           /**< Communication ready */
} EthSM_StateType;

/*==================================================================================================
*                                    NETWORK MODE TYPE
==================================================================================================*/
/**
 * @brief Ethernet Network Mode Type
 * @details Communication modes as defined by ComM
 */
typedef enum {
    ETHSM_MODE_NONE = 0,            /**< No mode (uninitialized) */
    ETHSM_MODE_NO_COM,              /**< No communication */
    ETHSM_MODE_WAIT_TRCVLINK,       /**< Waiting for transceiver link */
    ETHSM_MODE_WAIT_ONLINE,         /**< Waiting for TcpIp online */
    ETHSM_MODE_ONHOLD,              /**< Communication on hold */
    ETHSM_MODE_COM_READY            /**< Communication ready/full communication */
} EthSM_NetworkModeType;

/*==================================================================================================
*                                    TCP/IP MODE TYPE
==================================================================================================*/
/**
 * @brief TcpIp State Type (mapped from TcpIp module)
 * @details These states are reported by TcpIp module via EthSM_TcpIpModeIndication
 */
typedef enum {
    TCPIP_STATE_OFFLINE = 0,        /**< TcpIp offline */
    TCPIP_STATE_STARTUP,            /**< TcpIp starting up */
    TCPIP_STATE_ONLINE,             /**< TcpIp online (active communication) */
    TCPIP_STATE_ONHOLD              /**< TcpIp on hold */
} TcpIp_StateType;

/*==================================================================================================
*                                    NETWORK HANDLE TYPE
==================================================================================================*/
typedef uint8 EthSM_NetworkHandleType;

/*==================================================================================================
*                                    CONFIGURATION TYPE
==================================================================================================*/
/**
 * @brief Ethernet State Manager Configuration Type
 */
typedef struct {
    uint8 dummy;                    /**< Configuration placeholder */
} EthSM_ConfigType;

/*==================================================================================================
*                                    CALLBACK FUNCTION TYPES
==================================================================================================*/
/**
 * @brief ComM mode indication callback function type
 * @details Called by EthSM to notify ComM about mode changes
 */
typedef void (*EthSM_ComMModeIndicationType)(
    EthSM_NetworkHandleType NetworkHandle,
    EthSM_NetworkModeType Mode
);

/*==================================================================================================
*                                    CONFIGURATION TYPES
==================================================================================================*/

/* TcpIp controller mapping */
typedef struct {
    EthSM_NetworkHandleType networkHandle;
    uint8  tcpIpCtrlIdx;
    boolean dhcpEnabled;
    uint32 staticIpAddress;
    uint32 subnetMask;
    uint32 gatewayAddress;
} EthSM_TcpIpMappingType;

/* Transceiver configuration */
typedef struct {
    uint8 trcvIdx;
    uint8 wakeUpMode;
    boolean autoNegotiation;
    uint8 speed;
    uint8 duplexMode;
} EthSM_TrcvConfigType;

/* Controller configuration */
typedef struct {
    uint8 ctrlIdx;
    uint8 macAddress[6];
    uint16 mtu;
    boolean vlanSupport;
    uint16 vlanId;
} EthSM_CtrlConfigType;

/* Network configuration */
typedef struct {
    EthSM_NetworkHandleType networkHandle;
    uint8 ctrlIdx;
    uint8 trcvIdx;
    uint8 tcpIpCtrlIdx;
    ComM_ChannelHandleType comMChannel;
    uint16 timeoutWaitTrcvLink;
    uint16 timeoutWaitOnline;
    boolean wakeUpSupport;
    uint8 wakeUpSource;
    boolean wakeUpByBus;
} EthSM_NetworkConfigType;

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/

/*------------------------- Lifecycle Functions -------------------------*/

/**
 * @brief Initializes the Ethernet State Manager
 * @param ConfigPtr Pointer to configuration structure
 * @details Initializes all internal state variables and transitions to NO_COM state
 * @pre None
 * @post Module is initialized and ready for use
 * @note Shall be called before any other EthSM API function
 */
extern void EthSM_Init(const EthSM_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the Ethernet State Manager
 * @details Shuts down all Ethernet communication and returns to uninitialized state
 * @pre Module shall be initialized
 * @post Module is deinitialized
 * @note All ongoing communication will be aborted
 */
extern void EthSM_DeInit(void);

/**
 * @brief Returns version information of the EthSM module
 * @param VersionInfo Pointer to version information structure
 * @details Fills the provided structure with version details
 * @pre None
 * @post VersionInfo contains valid version data
 * @note Available if ETHSM_VERSION_INFO_API is enabled
 */
#if (ETHSM_VERSION_INFO_API == STD_ON)
extern void EthSM_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

/*------------------------- Mode Control Functions -------------------------*/

/**
 * @brief Requests a communication mode change
 * @param NetworkHandle Handle of the Ethernet network
 * @param ComMode Requested communication mode
 * @return E_OK: Request accepted
 *         E_NOT_OK: Request failed (invalid network handle or mode)
 * @details Triggers state machine transition to requested mode
 * @pre Module shall be initialized
 * @post State machine initiates transition to requested mode
 * @note Actual mode change is asynchronous, use EthSM_GetCurrentComMode to poll
 */
extern Std_ReturnType EthSM_RequestComMode(
    EthSM_NetworkHandleType NetworkHandle,
    ComM_ModeType ComMode
);

/**
 * @brief Gets the current communication mode of a network
 * @param NetworkHandle Handle of the Ethernet network
 * @param ComMode Pointer to store the current mode
 * @return E_OK: Mode retrieved successfully
 *         E_NOT_OK: Retrieval failed (invalid network handle or null pointer)
 * @details Returns the current state of the state machine mapped to ComM mode
 * @pre Module shall be initialized
 * @post ComMode contains the current communication mode
 */
extern Std_ReturnType EthSM_GetCurrentComMode(
    EthSM_NetworkHandleType NetworkHandle,
    ComM_ModeType* ComMode
);

/*------------------------- TcpIp Integration Functions -------------------------*/

/**
 * @brief TcpIp mode indication callback
 * @param NetworkHandle Handle of the Ethernet network
 * @param TcpIpMode Current TcpIp state
 * @details Called by TcpIp module to report state changes
 * @pre Module shall be initialized
 * @post EthSM state machine is updated based on TcpIp state
 * @note This is a callback function registered with TcpIp
 */
extern void EthSM_TcpIpModeIndication(
    EthSM_NetworkHandleType NetworkHandle,
    TcpIp_StateType TcpIpMode
);

/*------------------------- Cyclic Functions -------------------------*/

/**
 * @brief Main function for cyclic processing
 * @details Handles state machine transitions and timeout supervision
 * @pre Module shall be initialized
 * @post State machine is processed, timers are decremented
 * @note Shall be called cyclically (e.g., every 10ms)
 */
extern void EthSM_MainFunction(void);

/*------------------------- Internal State Access (for testing/debug) -------------------------*/

/**
 * @brief Gets the internal state of the state machine
 * @param NetworkHandle Handle of the Ethernet network
 * @return Current internal state of the state machine
 * @details Returns the detailed internal state (for diagnostic purposes)
 * @pre Module shall be initialized
 * @note This function is not part of the standard AUTOSAR API
 */
extern EthSM_StateType EthSM_GetInternalState(EthSM_NetworkHandleType NetworkHandle);

#endif /* ETHSM_H */
