/**
 * @file Swc_ModeManager.h
 * @brief Mode Manager Software Component Header
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: System mode management and state machine coordination
 */

#ifndef SWC_MODEMANAGER_H
#define SWC_MODEMANAGER_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte_Swc.h"
#include "Std_Types.h"

/*==================================================================================================
*                                    COMPONENT TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief System mode type
 */
typedef enum {
    SYSTEM_MODE_OFF = 0,
    SYSTEM_MODE_INIT,
    SYSTEM_MODE_STANDBY,
    SYSTEM_MODE_NORMAL,
    SYSTEM_MODE_DIAGNOSTIC,
    SYSTEM_MODE_SLEEP,
    SYSTEM_MODE_EMERGENCY
} Swc_SystemModeType;

/**
 * @brief System state type
 */
typedef enum {
    SYSTEM_STATE_OFF = 0,
    SYSTEM_STATE_INITIALIZING,
    SYSTEM_STATE_READY,
    SYSTEM_STATE_RUNNING,
    SYSTEM_STATE_DEGRADED,
    SYSTEM_STATE_SHUTDOWN,
    SYSTEM_STATE_ERROR
} Swc_SystemStateType;

/**
 * @brief Mode transition request
 */
typedef struct {
    Swc_SystemModeType targetMode;
    uint8 requestSource;
    uint32 requestTime;
    uint8 priority;
    boolean isForced;
} Swc_ModeTransitionRequestType;

/**
 * @brief Mode transition result
 */
typedef enum {
    MODE_TRANSITION_OK = 0,
    MODE_TRANSITION_PENDING,
    MODE_TRANSITION_REJECTED,
    MODE_TRANSITION_FAILED
} Swc_ModeTransitionResultType;

/**
 * @brief Mode manager status
 */
typedef struct {
    Swc_SystemModeType currentMode;
    Swc_SystemModeType previousMode;
    Swc_SystemModeType requestedMode;
    Swc_SystemStateType systemState;
    boolean transitionInProgress;
    uint32 modeEntryTime;
    uint32 modeDuration;
} Swc_ModeManagerStatusType;

/**
 * @brief Component mode notification
 */
typedef struct {
    uint8 componentId;
    Swc_SystemModeType currentMode;
    boolean modeAcknowledged;
    boolean modeReady;
} Swc_ComponentModeType;

/*==================================================================================================
*                                    RUNNABLE IDS
==================================================================================================*/
#define RTE_RUNNABLE_ModeManager_Init                0x61
#define RTE_RUNNABLE_ModeManager_50ms                0x62
#define RTE_RUNNABLE_ModeManager_ModeSwitch          0x63

/*==================================================================================================
*                                    PORT IDS
==================================================================================================*/
#define SWC_MODEMANAGER_PORT_SYSTEM_MODE_P           0x61
#define SWC_MODEMANAGER_PORT_SYSTEM_STATE_P          0x62
#define SWC_MODEMANAGER_PORT_MODE_REQUEST_R          0x63
#define SWC_MODEMANAGER_PORT_MODE_NOTIFICATION_P     0x64

/*==================================================================================================
*                                    COMPONENT API
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Mode Manager component
 */
extern void Swc_ModeManager_Init(void);

/**
 * @brief 50ms cyclic runnable - mode management
 */
extern void Swc_ModeManager_50ms(void);

/**
 * @brief Mode switch runnable
 */
extern void Swc_ModeManager_ModeSwitch(void);

/**
 * @brief Requests mode transition
 * @param request Mode transition request
 * @return Mode transition result
 */
extern Swc_ModeTransitionResultType Swc_ModeManager_RequestModeTransition(
    const Swc_ModeTransitionRequestType* request);

/**
 * @brief Gets current system mode
 * @param mode Pointer to store mode
 * @return RTE status
 */
extern Rte_StatusType Swc_ModeManager_GetCurrentMode(Swc_SystemModeType* mode);

/**
 * @brief Gets previous system mode
 * @param mode Pointer to store mode
 * @return RTE status
 */
extern Rte_StatusType Swc_ModeManager_GetPreviousMode(Swc_SystemModeType* mode);

/**
 * @brief Gets system state
 * @param state Pointer to store state
 * @return RTE status
 */
extern Rte_StatusType Swc_ModeManager_GetSystemState(Swc_SystemStateType* state);

/**
 * @brief Gets mode manager status
 * @param status Pointer to store status
 * @return RTE status
 */
extern Rte_StatusType Swc_ModeManager_GetStatus(Swc_ModeManagerStatusType* status);

/**
 * @brief Notifies component mode change
 * @param componentId Component ID
 * @param mode New mode
 * @return RTE status
 */
extern Rte_StatusType Swc_ModeManager_NotifyComponentMode(uint8 componentId,
                                                           Swc_SystemModeType mode);

/**
 * @brief Acknowledges mode change from component
 * @param componentId Component ID
 * @param acknowledged TRUE if acknowledged
 * @return RTE status
 */
extern Rte_StatusType Swc_ModeManager_AcknowledgeModeChange(uint8 componentId,
                                                             boolean acknowledged);

/**
 * @brief Checks if all components are ready for mode transition
 * @return TRUE if all ready
 */
extern boolean Swc_ModeManager_AreAllComponentsReady(void);

/**
 * @brief Forces mode transition (emergency use)
 * @param targetMode Target mode
 * @return RTE status
 */
extern Rte_StatusType Swc_ModeManager_ForceModeTransition(Swc_SystemModeType targetMode);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE INTERFACE MACROS
==================================================================================================*/
#define Rte_Write_SystemMode(data) \
    Rte_Write_SWC_MODEMANAGER_PORT_SYSTEM_MODE_P(data)

#define Rte_Write_SystemState(data) \
    Rte_Write_SWC_MODEMANAGER_PORT_SYSTEM_STATE_P(data)

#define Rte_Read_ModeRequest(data) \
    Rte_Read_SWC_MODEMANAGER_PORT_MODE_REQUEST_R(data)

#define Rte_Write_ModeNotification(data) \
    Rte_Write_SWC_MODEMANAGER_PORT_MODE_NOTIFICATION_P(data)

#endif /* SWC_MODEMANAGER_H */
