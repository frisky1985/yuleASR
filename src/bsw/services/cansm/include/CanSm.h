/**
 * @file CanSm.h
 * @brief CAN State Management module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: CAN State Management (CanSM)
 * Layer: Service Layer
 */

#ifndef CANSM_H
#define CANSM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "CanSm_Cfg.h"
#include "ComM.h"
#include "ComStack_Types.h"
#include "CanIf.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define CANSM_VENDOR_ID                         (0x01U) /* YuleTech Vendor ID */
#define CANSM_MODULE_ID                         (0x08U) /* CANSM Module ID */
#define CANSM_INSTANCE_ID                       (0x00U)
#define CANSM_AR_RELEASE_MAJOR_VERSION          (0x04U)
#define CANSM_AR_RELEASE_MINOR_VERSION          (0x04U)
#define CANSM_AR_RELEASE_REVISION_VERSION       (0x00U)
#define CANSM_SW_MAJOR_VERSION                  (0x01U)
#define CANSM_SW_MINOR_VERSION                  (0x00U)
#define CANSM_SW_PATCH_VERSION                  (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define CANSM_SID_INIT                          (0x00U)
#define CANSM_SID_DEINIT                        (0x01U)
#define CANSM_SID_REQUESTCOMMODE                (0x02U)
#define CANSM_SID_GETCURRENTCOMMODE             (0x03U)
#define CANSM_SID_CONTROLLERBUSOFF              (0x04U)
#define CANSM_SID_MAINFUNCTION                  (0x05U)
#define CANSM_SID_CONTROLLERMODEINDICATION      (0x07U)
#define CANSM_SID_GETVERSIONINFO                (0x09U)
#define CANSM_SID_CONTROLLERERRORSSTATUSINDICATION (0x3CU)
#define CANSM_SID_SETECUPASSIVE                 (0x10U)
#define CANSM_SID_TXTIMEOUTEXCEPTION            (0x11U)
#define CANSM_SID_GETCURRENTINTERNALSTATE       (0x12U)
#define CANSM_SID_GETCBKSTATUS                  (0x13U)
#define CANSM_SID_SETBAUDRATE                   (0x14U)
#define CANSM_SID_GETBAUDRATE                   (0x15U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define CANSM_E_PARAM_POINTER                   (0x01U)
#define CANSM_E_PARAM_CONTROLLER                (0x02U)
#define CANSM_E_PARAM_INVALID_NETWORK_MODE      (0x03U)
#define CANSM_E_INVALID_COMM_REQUEST            (0x04U)
#define CANSM_E_MODE_REQUEST_TIMEOUT            (0x05U)
#define CANSM_E_UNEXPECTED_EXECUTION            (0x06U)
#define CANSM_E_NOT_INITIALIZED                 (0x07U)
#define CANSM_E_INVALID_BAUDRATE                (0x08U)
#define CANSM_E_BUSOFF_RECOVERY_ACTIVE          (0x09U)

/*==================================================================================================
*                                    CANSM STATES (BSM - Bus State Machine)
==================================================================================================*/
/**
 * @brief CANSM Controller States
 * These represent the internal state machine states for each CAN controller
 */
typedef enum {
    /* Not Initialized State */
    CANSM_BSM_S_NOTINITIALIZED = 0,
    
    /* No Communication State */
    CANSM_BSM_S_NOCOM,
    
    /* Silent Communication State (Listen Only) */
    CANSM_BSM_S_SILENTCOM,
    
    /* Full Communication State */
    CANSM_BSM_S_FULLCOM,
    
    /* Silent Communication with BusOff Recovery */
    CANSM_BSM_S_SILENTCOM_BOR,
    
    /* Wait State for Mode Transitions */
    CANSM_BSM_S_WAIT_MODE_CHANGE,
    
    /* Check Wakeup State */
    CANSM_BSM_S_CHECKWAKEUP,
    
    /* Change Baudrate State */
    CANSM_BSM_S_CHANGEBAUDRATE
} CanSm_BsmStateType;

/**
 * @brief CANSM Network Sub-states for BSM_S_NOCOM
 */
/* Note: Each sub-state enum uses unique value names to avoid C enum namespace conflicts
 * (ARM GCC errors on same enumerator in different enums within the same scope)
 */
typedef enum {
    CANSM_S_NOCOM_NOP = 0,
    CANSM_NOCOM_S_RESTART_CC,
    CANSM_NOCOM_S_RESTART_CC_WAIT,
    CANSM_NOCOM_S_CC_STOPPED,
    CANSM_NOCOM_S_CC_STOPPED_WAIT,
    CANSM_NOCOM_S_CC_SLEEP,
    CANSM_NOCOM_S_CC_SLEEP_WAIT,
    CANSM_NOCOM_S_CC_OFFLINE
} CanSm_NoComSubStateType;

/**
 * @brief CANSM Network Sub-states for BSM_S_SILENTCOM
 */
typedef enum {
    CANSM_S_SILENTCOM_NOP = 0,
    CANSM_SILENTCOM_S_CC_ONLINE
} CanSm_SilentComSubStateType;

/**
 * @brief CANSM Network Sub-states for BSM_S_FULLCOM
 */
typedef enum {
    CANSM_S_FULLCOM_NOP = 0,
    CANSM_FULLCOM_S_CC_START,
    CANSM_FULLCOM_S_CC_START_WAIT,
    CANSM_FULLCOM_S_CC_ONLINE
} CanSm_FullComSubStateType;

/**
 * @brief CANSM Network Sub-states for BSM_S_SILENTCOM_BOR (BusOff Recovery)
 */
typedef enum {
    CANSM_S_BUSOFF_CHECK = 0,
    CANSM_S_BUSOFF_RECOVERY_L1,
    CANSM_S_BUSOFF_RECOVERY_L2,
    CANSM_BOR_S_RESTART_CC,
    CANSM_BOR_S_RESTART_CC_WAIT,
    CANSM_BOR_S_CC_STOPPED,
    CANSM_BOR_S_CC_STOPPED_WAIT
} CanSm_SilentComBorSubStateType;

/**
 * @brief CANSM Baudrate Configuration Type
 */
typedef struct {
    uint16 BaudRate;                /**< Baudrate in kbps */
    uint32 BaudRateConfig;          /**< Hardware-specific configuration */
} CanSm_BaudrateConfigType;

/**
 * @brief CANSM Network Configuration Type
 */
typedef struct {
    uint8 NetworkHandle;            /**< ComM Channel Handle */
    uint8 ControllerId;             /**< CAN Controller ID */
    uint8 NumBaudrates;             /**< Number of supported baudrates */
    const CanSm_BaudrateConfigType* BaudrateConfigs; /**< Baudrate configurations */
    uint16 MainFunctionPeriodMs;    /**< Main function period in milliseconds */
    uint16 BusOffRecoveryTimeMs;    /**< BusOff recovery timeout in milliseconds */
    uint8  BusOffThreshold;         /**< BusOff counter threshold before recovery */
    boolean WakeupSupport;          /**< Wakeup support enabled */
    boolean BusOffRecoveryEnabled;  /**< Automatic BusOff recovery enabled */
    boolean TransceiverSupport;     /**< Transceiver management support */
    uint8  TransceiverId;           /**< Transceiver ID (if supported) */
} CanSm_NetworkConfigType;

/**
 * @brief CANSM Configuration Type
 */
typedef struct {
    const CanSm_NetworkConfigType* Networks;    /**< Network configurations */
    uint8 NumNetworks;                          /**< Number of networks */
    boolean DevErrorDetect;                     /**< Development error detection */
    boolean VersionInfoApi;                     /**< Version info API enabled */
    boolean SetBaudrateApi;                     /**< Set baudrate API enabled */
} CanSm_ConfigType;

/*==================================================================================================
*                                    EXTERNAL DATA
==================================================================================================*/
#define CANSM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const CanSm_ConfigType CanSm_Config;

#define CANSM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    GLOBAL DATA
==================================================================================================*/
typedef uint8 CanSm_NetworkHandleType;

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define CANSM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the CAN State Management module
 * @param ConfigPtr Pointer to configuration structure
 * @details This function initializes all CAN networks to CANSM_BSM_S_NOTINITIALIZED state
 */
void CanSM_Init(const CanSm_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the CAN State Management module
 * @details This function deinitializes the CanSM and transitions all networks to CANSM_BSM_S_NOCOM
 */
void CanSM_DeInit(void);

/**
 * @brief Requests a communication mode change for a network
 * @param Network Network handle
 * @param ComM_Mode Requested communication mode
 * @return E_OK if request was accepted, E_NOT_OK otherwise
 * @details This is the main API used by ComM to request communication mode changes
 */
Std_ReturnType CanSM_RequestComMode(ComM_UserHandleType Network, ComM_ModeType ComM_Mode);

/**
 * @brief Gets the current communication mode of a network
 * @param Network Network handle
 * @param ComM_ModePtr Pointer to store the current communication mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanSM_GetCurrentComMode(ComM_UserHandleType Network, ComM_ModeType* ComM_ModePtr);

/**
 * @brief Main function for the CAN State Management module
 * @details This function must be called cyclically to process state machine transitions
 */
void CanSM_MainFunction(void);

/**
 * @brief BusOff indication callback from CanIf
 * @param ControllerId Controller that experienced BusOff
 * @details Called by CanIf when a BusOff event is detected
 */
void CanSM_ControllerBusOff(uint8 ControllerId);

/**
 * @brief Controller mode indication callback from CanIf
 * @param ControllerId Controller that changed mode
 * @param ControllerMode New controller mode
 * @details Called by CanIf to confirm controller mode changes
 */
void CanSM_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);

/**
 * @brief Gets version information
 * @param VersionInfo Pointer to version info structure
 */
void CanSM_GetVersionInfo(Std_VersionInfoType* VersionInfo);

/**
 * @brief Sets the baudrate for a network
 * @param Network Network handle
 * @param BaudRate New baudrate to set
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanSM_SetBaudrate(ComM_UserHandleType Network, uint16 BaudRate);

/**
 * @brief Gets the baudrate of a network
 * @param Network Network handle
 * @param BaudRatePtr Pointer to store the baudrate
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanSM_GetBaudrate(ComM_UserHandleType Network, uint16* BaudRatePtr);

/**
 * @brief Gets the current internal state of a network
 * @param Network Network handle
 * @param StatePtr Pointer to store the internal state
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanSM_GetCurrentInternalState(uint8 Network, CanSm_BsmStateType* StatePtr);

#define CANSM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* CANSM_H */
