/******************************************************************************
 * @file    ecum.h
 * @brief   ECU State Manager (EcuM) Interface
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ECUM_H
#define ECUM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/autosar_types.h"
#include "common/autosar_errors.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * EcuM Version Information
 ******************************************************************************/
#define ECUM_VENDOR_ID                  0x01U
#define ECUM_MODULE_ID                  0x0CU  /* EcuM module ID per AUTOSAR */
#define ECUM_SW_MAJOR_VERSION           1U
#define ECUM_SW_MINOR_VERSION           0U
#define ECUM_SW_PATCH_VERSION           0U

/******************************************************************************
 * EcuM State Definitions
 ******************************************************************************/

typedef enum {
    ECUM_STATE_STARTUP              = 0x00U,
    ECUM_STATE_RUN                  = 0x10U,
    ECUM_STATE_SLEEP                = 0x20U,
    ECUM_STATE_WAKE_SLEEP           = 0x30U,
    ECUM_STATE_SHUTDOWN             = 0x40U,
    ECUM_STATE_OFF                  = 0x50U,
    ECUM_STATE_RESET                = 0x60U,
    ECUM_STATE_PREP_SHUTDOWN        = 0x70U,
    ECUM_STATE_GO_SLEEP             = 0x80U,
    ECUM_STATE_GO_OFF_ONE           = 0x90U,
    ECUM_STATE_GO_OFF_TWO           = 0xA0U,
    ECUM_STATE_WAKEUP_ONE           = 0xB0U,
    ECUM_STATE_WAKEUP_TWO           = 0xC0U
} EcuM_StateType;

/******************************************************************************
 * EcuM Sub-States
 ******************************************************************************/

typedef enum {
    /* STARTUP sub-states */
    ECUM_SUBSTATE_STARTUP_FIRST     = 0x01U,
    ECUM_SUBSTATE_STARTUP_SECOND    = 0x02U,
    ECUM_SUBSTATE_STARTUP_THIRD     = 0x03U,
    ECUM_SUBSTATE_WAKEUP_FIRST      = 0x04U,
    ECUM_SUBSTATE_WAKEUP_SECOND     = 0x05U,
    ECUM_SUBSTATE_WAKEUP_THIRD      = 0x06U,
    
    /* RUN sub-states */
    ECUM_SUBSTATE_RUN_APP           = 0x10U,
    ECUM_SUBSTATE_RUN_OS            = 0x11U,
    
    /* SLEEP sub-states */
    ECUM_SUBSTATE_SLEEP_POLLING     = 0x20U,
    ECUM_SUBSTATE_SLEEP_HALT        = 0x21U,
    
    /* SHUTDOWN sub-states */
    ECUM_SUBSTATE_SHUTDOWN_FIRST    = 0x30U,
    ECUM_SUBSTATE_SHUTDOWN_SECOND   = 0x31U
} EcuM_SubStateType;

/******************************************************************************
 * Wakeup Source Definitions
 ******************************************************************************/

typedef uint32 EcuM_WakeupSourceType;

#define ECUM_WKSOURCE_POWER             ((EcuM_WakeupSourceType)0x00000001U)
#define ECUM_WKSOURCE_RESET             ((EcuM_WakeupSourceType)0x00000002U)
#define ECUM_WKSOURCE_INTERNAL_RESET    ((EcuM_WakeupSourceType)0x00000004U)
#define ECUM_WKSOURCE_INTERNAL_WDG      ((EcuM_WakeupSourceType)0x00000008U)
#define ECUM_WKSOURCE_EXTERNAL_WDG      ((EcuM_WakeupSourceType)0x00000010U)
#define ECUM_WKSOURCE_CAN               ((EcuM_WakeupSourceType)0x00000100U)
#define ECUM_WKSOURCE_CAN_RX            ((EcuM_WakeupSourceType)0x00000200U)
#define ECUM_WKSOURCE_LIN               ((EcuM_WakeupSourceType)0x00000400U)
#define ECUM_WKSOURCE_FLEXRAY           ((EcuM_WakeupSourceType)0x00000800U)
#define ECUM_WKSOURCE_ETHERNET          ((EcuM_WakeupSourceType)0x00001000U)
#define ECUM_WKSOURCE_SYSTEM            ((EcuM_WakeupSourceType)0x00010000U)
#define ECUM_WKSOURCE_IO                ((EcuM_WakeupSourceType)0x00020000U)
#define ECUM_WKSOURCE_TIMER             ((EcuM_WakeupSourceType)0x00040000U)
#define ECUM_WKSOURCE_ALL_SOURCES       ((EcuM_WakeupSourceType)0xFFFFFFFFU)

/******************************************************************************
 * Sleep Mode Definitions
 ******************************************************************************/

typedef uint8 EcuM_SleepModeType;

#define ECUM_SLEEP_MODE_POLLING         ((EcuM_SleepModeType)0x00U)
#define ECUM_SLEEP_MODE_HALT            ((EcuM_SleepModeType)0x01U)
#define ECUM_SLEEP_MODE_DEEP_HALT       ((EcuM_SleepModeType)0x02U)
#define ECUM_MAX_SLEEP_MODES            8U

/******************************************************************************
 * Wakeup Validation Status
 ******************************************************************************/

typedef enum {
    ECUM_WKSTATUS_NONE              = 0x00U,
    ECUM_WKSTATUS_PENDING           = 0x01U,
    ECUM_WKSTATUS_VALIDATED         = 0x02U,
    ECUM_WKSTATUS_EXPIRED           = 0x03U,
    ECUM_WKSTATUS_DISABLED          = 0x04U
} EcuM_WakeupStatusType;

/******************************************************************************
 * EcuM User Type
 ******************************************************************************/

typedef uint8 EcuM_UserType;
#define ECUM_USER_NONE                  ((EcuM_UserType)0xFFU)
#define ECUM_MAX_USERS                  32U

/******************************************************************************
 * Shutdown Target Type
 ******************************************************************************/

typedef enum {
    ECUM_TARGET_SLEEP               = 0x00U,
    ECUM_TARGET_RESET               = 0x01U,
    ECUM_TARGET_OFF                 = 0x02U
} EcuM_ShutdownTargetType;

/******************************************************************************
 * EcuM Configuration Types
 ******************************************************************************/

typedef struct {
    EcuM_WakeupSourceType source;
    uint16 validationTimeout;           /* Validation timeout in ms */
    uint16 validationCounter;           /* Number of validation checks */
    bool checkWakeupSupported;          /* Check wakeup API supported */
    bool comMChannelSupported;          /* ComM channel associated */
    uint8 comMChannel;                  /* ComM channel ID */
} EcuM_WakeupSourceConfigType;

typedef struct {
    EcuM_SleepModeType sleepMode;
    EcuM_WakeupSourceType wakeupSourceMask;
    bool mcuModeSupported;
    uint8 mcuMode;                      /* MCU specific sleep mode */
    bool pollWakeupSupported;
    bool sleepCallbackSupported;
} EcuM_SleepModeConfigType;

typedef struct {
    /* General configuration */
    uint16 normalMcuWakeupTime;         /* Normal MCU wake-up time */
    uint16 minShutdownTime;             /* Minimum shutdown time */
    
    /* Wakeup sources */
    const EcuM_WakeupSourceConfigType *wakeupSources;
    uint8 numWakeupSources;
    
    /* Sleep modes */
    const EcuM_SleepModeConfigType *sleepModes;
    uint8 numSleepModes;
    
    /* Default shutdown target */
    EcuM_ShutdownTargetType defaultTarget;
    EcuM_SleepModeType defaultSleepMode;
    EcuM_WakeupSourceType defaultResetMode;
    
    /* Mode request ports */
    uint8 numModeRequestPorts;
} EcuM_ConfigType;

/******************************************************************************
 * EcuM Status Type
 ******************************************************************************/

typedef struct {
    bool initialized;
    EcuM_StateType currentState;
    EcuM_SubStateType subState;
    EcuM_WakeupSourceType pendingWakeupEvents;
    EcuM_WakeupSourceType validatedWakeupEvents;
    EcuM_WakeupSourceType expiredWakeupEvents;
    EcuM_WakeupSourceType disabledWakeupEvents;
    EcuM_ShutdownTargetType shutdownTarget;
    EcuM_SleepModeType sleepMode;
    uint32 stateRequestMask;            /* Bitmask of user requests */
    uint32 runCounter;                  /* RUN state request counter */
    uint32 postRunCounter;              /* POST RUN state request counter */
    uint32 lastWakeupTime;              /* Timestamp of last wakeup */
} EcuM_StatusType;

/******************************************************************************
 * Callback Function Types
 ******************************************************************************/

typedef void (*EcuM_StateChangeCallback)(
    EcuM_StateType oldState,
    EcuM_StateType newState
);

typedef void (*EcuM_WakeupCallback)(
    EcuM_WakeupSourceType source,
    EcuM_WakeupStatusType status
);

typedef void (*EcuM_SleepModeCallback)(
    EcuM_SleepModeType sleepMode,
    bool entering
);

/******************************************************************************
 * EcuM Core API Functions
 ******************************************************************************/

/**
 * @brief Initialize EcuM module
 * @param config Pointer to EcuM configuration
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_Init(const EcuM_ConfigType *config);

/**
 * @brief Deinitialize EcuM module
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_DeInit(void);

/**
 * @brief Start ECU startup sequence (called from main)
 * @param selectedConfig Index of selected configuration
 */
void EcuM_StartupTwo(void);

/**
 * @brief Complete startup and enter RUN state
 */
void EcuM_EnterRunMode(void);

/**
 * @brief Main function - called cyclically
 */
void EcuM_MainFunction(void);

/******************************************************************************
 * State Management Functions
 ******************************************************************************/

/**
 * @brief Get current ECU state
 * @return Current state
 */
EcuM_StateType EcuM_GetState(void);

/**
 * @brief Get current ECU sub-state
 * @return Current sub-state
 */
EcuM_SubStateType EcuM_GetSubState(void);

/**
 * @brief Set ECU state
 * @param state New state
 */
void EcuM_SetState(EcuM_StateType state);

/**
 * @brief Request RUN state
 * @param user User ID requesting state
 * @return E_OK if successful
 */
Std_ReturnType EcuM_RequestRUN(EcuM_UserType user);

/**
 * @brief Release RUN state request
 * @param user User ID releasing state
 * @return E_OK if successful
 */
Std_ReturnType EcuM_ReleaseRUN(EcuM_UserType user);

/**
 * @brief Request POST RUN state
 * @param user User ID requesting state
 * @return E_OK if successful
 */
Std_ReturnType EcuM_RequestPOST_RUN(EcuM_UserType user);

/**
 * @brief Release POST RUN state request
 * @param user User ID releasing state
 * @return E_OK if successful
 */
Std_ReturnType EcuM_ReleasePOST_RUN(EcuM_UserType user);

/**
 * @brief Kill all RUN requests (emergency)
 * @param user User ID requesting kill
 * @return E_OK if successful
 */
Std_ReturnType EcuM_KillAllRUNRequests(void);

/******************************************************************************
 * Wakeup Management Functions
 ******************************************************************************/

/**
 * @brief Set wakeup event
 * @param sources Wakeup source(s) to set
 */
void EcuM_SetWakeupEvent(EcuM_WakeupSourceType sources);

/**
 * @brief Clear wakeup event
 * @param sources Wakeup source(s) to clear
 */
void EcuM_ClearWakeupEvent(EcuM_WakeupSourceType sources);

/**
 * @brief Get pending wakeup events
 * @return Bitmask of pending wakeup events
 */
EcuM_WakeupSourceType EcuM_GetPendingWakeupEvents(void);

/**
 * @brief Get validated wakeup events
 * @return Bitmask of validated wakeup events
 */
EcuM_WakeupSourceType EcuM_GetValidatedWakeupEvents(void);

/**
 * @brief Get expired wakeup events
 * @return Bitmask of expired wakeup events
 */
EcuM_WakeupSourceType EcuM_GetExpiredWakeupEvents(void);

/**
 * @brief Validate wakeup event
 * @param sources Wakeup source(s) to validate
 */
void EcuM_ValidateWakeupEvent(EcuM_WakeupSourceType sources);

/**
 * @brief Check wakeup status of source
 * @param source Wakeup source to check
 * @return Current status
 */
EcuM_WakeupStatusType EcuM_GetWakeupStatus(EcuM_WakeupSourceType source);

/**
 * @brief Enable wakeup source
 * @param sources Wakeup source(s) to enable
 */
void EcuM_EnableWakeupSources(EcuM_WakeupSourceType sources);

/**
 * @brief Disable wakeup source
 * @param sources Wakeup source(s) to disable
 */
void EcuM_DisableWakeupSources(EcuM_WakeupSourceType sources);

/******************************************************************************
 * Sleep Management Functions
 ******************************************************************************/

/**
 * @brief Select shutdown target
 * @param target Shutdown target (SLEEP/RESET/OFF)
 * @param mode Sleep mode or reset mode
 * @return E_OK if successful
 */
Std_ReturnType EcuM_SelectShutdownTarget(
    EcuM_ShutdownTargetType target,
    uint8 mode
);

/**
 * @brief Get shutdown target
 * @param target Pointer to store target
 * @param mode Pointer to store mode
 * @return E_OK if successful
 */
Std_ReturnType EcuM_GetShutdownTarget(
    EcuM_ShutdownTargetType *target,
    uint8 *mode
);

/**
 * @brief Get last shutdown target
 * @param target Pointer to store target
 * @param mode Pointer to store mode
 * @return E_OK if successful
 */
Std_ReturnType EcuM_GetLastShutdownTarget(
    EcuM_ShutdownTargetType *target,
    uint8 *mode
);

/**
 * @brief Select application sleep mode
 * @param sleepMode Sleep mode to select
 * @return E_OK if successful
 */
Std_ReturnType EcuM_SelectSleepMode(EcuM_SleepModeType sleepMode);

/**
 * @brief Get selected sleep mode
 * @return Selected sleep mode
 */
EcuM_SleepModeType EcuM_GetSleepMode(void);

/**
 * @brief Go to selected sleep mode
 * @return E_OK if successful
 */
Std_ReturnType EcuM_GoToSleep(void);

/**
 * @brief Go to halt (sleep) mode
 * @return E_OK if successful
 */
Std_ReturnType EcuM_GoToHalt(void);

/**
 * @brief Go to poll mode (sleep with polling)
 * @return E_OK if successful
 */
Std_ReturnType EcuM_GoToPoll(void);

/******************************************************************************
 * Shutdown Functions
 ******************************************************************************/

/**
 * @brief Go to shutdown sequence
 */
void EcuM_GoToShutdown(void);

/**
 * @brief Go to reset
 * @param resetMode Reset mode to use
 */
void EcuM_GoToReset(EcuM_WakeupSourceType resetMode);

/******************************************************************************
 * Callback Registration
 ******************************************************************************/

/**
 * @brief Register state change callback
 * @param callback Callback function
 * @return E_OK if successful
 */
Std_ReturnType EcuM_RegisterStateChangeCallback(EcuM_StateChangeCallback callback);

/**
 * @brief Unregister state change callback
 * @param callback Callback function
 * @return E_OK if successful
 */
Std_ReturnType EcuM_UnregisterStateChangeCallback(EcuM_StateChangeCallback callback);

/**
 * @brief Register wakeup callback
 * @param callback Callback function
 * @return E_OK if successful
 */
Std_ReturnType EcuM_RegisterWakeupCallback(EcuM_WakeupCallback callback);

/**
 * @brief Unregister wakeup callback
 * @param callback Callback function
 * @return E_OK if successful
 */
Std_ReturnType EcuM_UnregisterWakeupCallback(EcuM_WakeupCallback callback);

/******************************************************************************
 * Application Callbacks (to be implemented by application)
 ******************************************************************************/

/**
 * @brief Called at the very beginning of startup (before OS init)
 */
void EcuM_AL_DriverInitZero(void);

/**
 * @brief Called to determine post-build configuration
 * @return Pointer to selected configuration
 */
const EcuM_ConfigType* EcuM_DeterminePbConfiguration(void);

/**
 * @brief Called after OS initialization
 */
void EcuM_AL_DriverInitOne(const EcuM_ConfigType *config);

/**
 * @brief Called when entering RUN state
 */
void EcuM_OnEnterRun(void);

/**
 * @brief Called when exiting RUN state
 */
void EcuM_OnExitRun(void);

/**
 * @brief Called when entering POST RUN state
 */
void EcuM_OnEnterPostRun(void);

/**
 * @brief Called when exiting POST RUN state
 */
void EcuM_OnExitPostRun(void);

/**
 * @brief Called when preparing shutdown
 */
void EcuM_OnPrepShutdown(void);

/**
 * @brief Called when entering sleep
 */
void EcuM_AL_SwitchOff(void);

/**
 * @brief Called when waking from sleep
 */
void EcuM_AL_Wakeup(void);

/**
 * @brief Called when entering specific sleep mode
 * @param sleepMode Sleep mode being entered
 */
void EcuM_SleepActivity(EcuM_SleepModeType sleepMode);

/**
 * @brief Check if sleep conditions are met
 * @return TRUE if can go to sleep
 */
boolean EcuM_CheckSleep(void);

/**
 * @brief Check wakeup source validity
 * @param wakeupSource Wakeup source to check
 * @return TRUE if valid
 */
boolean EcuM_CheckWakeup(EcuM_WakeupSourceType wakeupSource);

/**
 * @brief Get boot target
 * @return Boot target
 */
uint8 EcuM_GetBootTarget(void);

/**
 * @brief Set boot target
 * @param target Boot target
 */
void EcuM_SetBootTarget(uint8 target);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Get module status
 * @return Pointer to status structure
 */
const EcuM_StatusType* EcuM_GetStatus(void);

/**
 * @brief Check if module is initialized
 * @return TRUE if initialized
 */
boolean EcuM_IsInitialized(void);

/**
 * @brief Get version information
 * @param version Pointer to version structure
 */
void EcuM_GetVersionInfo(Std_VersionInfoType *version);

#ifdef __cplusplus
}
#endif

#endif /* ECUM_H */
