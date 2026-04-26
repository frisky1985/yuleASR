/******************************************************************************
 * @file    wdgM.h
 * @brief   Watchdog Manager (WdgM) Interface
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef WDGM_H
#define WDGM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/autosar_types.h"
#include "common/autosar_errors.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * WdgM Version Information
 ******************************************************************************/
#define WDGM_VENDOR_ID                  0x01U
#define WDGM_MODULE_ID                  0x0EU  /* WdgM module ID per AUTOSAR */
#define WDGM_SW_MAJOR_VERSION           1U
#define WDGM_SW_MINOR_VERSION           0U
#define WDGM_SW_PATCH_VERSION           0U

/******************************************************************************
 * WdgM Configuration Constants
 ******************************************************************************/
#define WDGM_MAX_SUPERVISED_ENTITIES    32U
#define WDGM_MAX_CHECKPOINTS            256U
#define WDGM_MAX_TRANSITIONS            128U
#define WDGM_MAX_MODES                  8U
#define WDGM_MAX_SE_PER_MODE            32U
#define WDGM_MAX_EXTERNAL_LOGICALS      16U
#define WDGM_INVALID_SE_ID              0xFFU
#define WDGM_INVALID_CP_ID              0xFFU
#define WDGM_INVALID_MODE_ID            0xFFU

/******************************************************************************
 * WdgM Service IDs for Error Reporting
 ******************************************************************************/
#define WDGM_SID_INIT                   0x01U
#define WDGM_SID_DEINIT                 0x02U
#define WDGM_SID_SETMODE                0x03U
#define WDGM_SID_GETMODE                0x04U
#define WDGM_SID_CHECKPOINTREACHED      0x05U
#define WDGM_SID_GETLOCALSTATUS         0x06U
#define WDGM_SID_GETGLOBALSTATUS        0x07U
#define WDGM_SID_PERFORMRESET           0x08U
#define WDGM_SID_GETFIRSTEXPIREDSEID    0x09U
#define WDGM_SID_MAINFUNCTION           0x10U
#define WDGM_SID_UPDATEALIVECOUNTER     0x11U
#define WDGM_SID_CHECKPOINTREACHEDEXP   0x12U

/******************************************************************************
 * WdgM Error Codes (Production Error/Event IDs)
 ******************************************************************************/
#define WDGM_E_NO_ERROR                 0x00U
#define WDGM_E_ALIVE_SUPERVISION        0x01U  /* Alive supervision failure */
#define WDGM_E_DEADLINE_SUPERVISION     0x02U  /* Deadline supervision failure */
#define WDGM_E_LOGICAL_SUPERVISION      0x03U  /* Logical supervision failure */
#define WDGM_E_SET_MODE                 0x04U  /* Set mode failed */
#define WDGM_E_IMPROPER_CALLER          0x05U  /* Improper caller */
#define WDGM_E_PARAM_POINTER            0x06U  /* Null pointer parameter */
#define WDGM_E_PARAM_CONFIG             0x07U  /* Invalid configuration */
#define WDGM_E_PARAM_MODE               0x08U  /* Invalid mode */
#define WDGM_E_PARAM_SEID               0x09U  /* Invalid SE ID */
#define WDGM_E_PARAM_CP                 0x0AU  /* Invalid checkpoint ID */
#define WDGM_E_UNINIT                   0x10U  /* Module not initialized */
#define WDGM_E_DEINIT                   0x11U  /* Module already deinitialized */
#define WDGM_E_SAFETY_CRITICAL          0x20U  /* Safety critical error */

/******************************************************************************
 * WdgM Status Types
 ******************************************************************************/

typedef enum {
    WDGM_GLOBAL_STATUS_OK = 0,          /* All SEs are OK */
    WDGM_GLOBAL_STATUS_FAILED,          /* At least one SE failed */
    WDGM_GLOBAL_STATUS_EXPIRED,         /* At least one SE expired */
    WDGM_GLOBAL_STATUS_STOPPED          /* WdgM stopped */
} WdgM_GlobalStatusType;

typedef enum {
    WDGM_LOCAL_STATUS_OK = 0,           /* SE is alive within configured limits */
    WDGM_LOCAL_STATUS_FAILED,           /* SE failed but within tolerance */
    WDGM_LOCAL_STATUS_EXPIRED,          /* SE exceeded failure tolerance */
    WDGM_LOCAL_STATUS_DEACTIVATED       /* SE is deactivated */
} WdgM_LocalStatusType;

typedef enum {
    WDGM_SUPERVISION_STATUS_OK = 0,     /* Supervision passed */
    WDGM_SUPERVISION_STATUS_FAILED,     /* Supervision failed */
    WDGM_SUPERVISION_STATUS_DEACTIVATED /* Supervision deactivated */
} WdgM_SupervisionStatusType;

/******************************************************************************
 * WdgM Supervision Types
 ******************************************************************************/
typedef enum {
    WDGM_ALIVE_SUPERVISION = 0,         /* Alive counter supervision */
    WDGM_DEADLINE_SUPERVISION,          /* Deadline monitoring */
    WDGM_LOGICAL_SUPERVISION            /* Program flow monitoring */
} WdgM_SupervisionType;

typedef enum {
    WDGM_AUTOSAR_INTERNAL = 0,          /* Internal checkpoint (AUTOSAR) */
    WDGM_EXTERNAL                       /* External checkpoint (user-defined) */
} WdgM_CheckpointSourceType;

/******************************************************************************
 * WdgM Mode Types
 ******************************************************************************/
typedef uint8 WdgM_ModeType;
#define WDGM_MODE_OFF                   ((WdgM_ModeType)0x00U)
#define WDGM_MODE_SLOW                  ((WdgM_ModeType)0x01U)
#define WDGM_MODE_FAST                  ((WdgM_ModeType)0x02U)
#define WDGM_MODE_RESERVED              ((WdgM_ModeType)0xFFU)

/******************************************************************************
 * WdgM Handle Types
 ******************************************************************************/
typedef uint8 WdgM_SupervisedEntityIdType;
typedef uint8 WdgM_CheckpointIdType;
typedef uint16 WdgM_CycleCounterType;

/******************************************************************************
 * Alive Supervision Configuration
 ******************************************************************************/
typedef struct {
    WdgM_SupervisedEntityIdType seId;       /* SE identifier */
    WdgM_CheckpointIdType checkpointId;     /* Checkpoint to monitor */
    uint16 expectedAliveIndications;        /* Expected alive indications per supervision cycle */
    uint16 minMargin;                       /* Minimum margin for alive indications */
    uint16 maxMargin;                       /* Maximum margin for alive indications */
    uint16 supervisionCycle;                /* Supervision cycle in WdgM main function periods */
    boolean isInternal;                     /* TRUE if AUTOSAR internal checkpoint */
} WdgM_AliveSupervisionConfigType;

/******************************************************************************
 * Deadline Supervision Configuration
 ******************************************************************************/
typedef struct {
    WdgM_SupervisedEntityIdType seId;       /* SE identifier */
    WdgM_CheckpointIdType startCheckpoint;  /* Start checkpoint */
    WdgM_CheckpointIdType endCheckpoint;    /* End checkpoint */
    uint32 minDeadline;                     /* Minimum deadline in ms */
    uint32 maxDeadline;                     /* Maximum deadline in ms */
    boolean isInternalStart;                /* TRUE if start checkpoint is internal */
    boolean isInternalEnd;                  /* TRUE if end checkpoint is internal */
} WdgM_DeadlineSupervisionConfigType;

/******************************************************************************
 * Logical Supervision Configuration
 ******************************************************************************/
typedef struct {
    WdgM_SupervisedEntityIdType seId;       /* SE identifier */
    WdgM_CheckpointIdType fromCheckpoint;   /* Source checkpoint */
    WdgM_CheckpointIdType toCheckpoint;     /* Destination checkpoint */
    boolean isInternalFrom;                 /* TRUE if source is internal */
    boolean isInternalTo;                   /* TRUE if destination is internal */
} WdgM_TransitionConfigType;

typedef struct {
    WdgM_SupervisedEntityIdType seId;       /* SE identifier */
    const WdgM_TransitionConfigType *transitions;
    uint8 numTransitions;
    boolean isGraphActive;
} WdgM_LogicalSupervisionConfigType;

/******************************************************************************
 * Supervised Entity Configuration
 ******************************************************************************/
typedef struct {
    WdgM_SupervisedEntityIdType seId;               /* SE identifier */
    uint8 failureTolerance;                         /* Number of failures before expired */
    boolean isActive;                               /* SE active in this mode */
    uint8 numAliveSupervisions;                     /* Number of alive supervisions */
    const WdgM_AliveSupervisionConfigType *aliveSupervisions;
    uint8 numDeadlineSupervisions;                  /* Number of deadline supervisions */
    const WdgM_DeadlineSupervisionConfigType *deadlineSupervisions;
    const WdgM_LogicalSupervisionConfigType *logicalSupervision;
} WdgM_SupervisedEntityConfigType;

/******************************************************************************
 * Mode Configuration
 ******************************************************************************/
typedef struct {
    WdgM_ModeType modeId;                           /* Mode identifier */
    uint16 expirationTime;                          /* Expiration time in ms */
    uint8 triggerMode;                              /* WdgIf trigger mode (Off/Slow/Fast) */
    const WdgM_SupervisedEntityConfigType *seConfigs; /* SE configurations for this mode */
    uint8 numSEs;                                   /* Number of SEs in this mode */
} WdgM_ModeConfigType;

/******************************************************************************
 * WdgM Configuration
 ******************************************************************************/
typedef struct {
    /* General configuration */
    uint16 initialMode;                             /* Initial mode after Init */
    uint16 mainFunctionPeriod;                      /* WdgM_MainFunction period in ms */
    
    /* Mode configurations */
    const WdgM_ModeConfigType *modeConfigs;
    uint8 numModes;
    
    /* Callback functions */
    void (*globalStatusCallback)(WdgM_GlobalStatusType newStatus, WdgM_GlobalStatusType oldStatus);
    void (*localStatusCallback)(WdgM_SupervisedEntityIdType seId, WdgM_LocalStatusType newStatus, WdgM_LocalStatusType oldStatus);
    void (*errorCallback)(uint8 errorCode, WdgM_SupervisedEntityIdType seId);
    
    /* Safety configuration */
    boolean supervisionEnabled;                     /* Global supervision enable */
    boolean immediateResetOnFailure;                /* Immediate reset on failure */
    boolean demReportingEnabled;                    /* DEM error reporting enabled */
    uint8 maxFailedCyclesBeforeReset;               /* Max failed cycles before reset */
} WdgM_ConfigType;

/******************************************************************************
 * Runtime Status Types
 ******************************************************************************/
typedef struct {
    WdgM_CycleCounterType cycleCounter;             /* Current cycle counter */
    uint16 aliveIndications;                        /* Current alive indication count */
    uint16 expectedAliveIndications;                /* Expected count */
    uint16 minMargin;                               /* Min margin */
    uint16 maxMargin;                               /* Max margin */
    boolean checkpointReachedThisCycle;             /* Checkpoint reached flag */
} WdgM_AliveSupervisionStatusType;

typedef struct {
    uint32 startTime;                               /* Deadline start timestamp */
    boolean startReached;                           /* Start checkpoint reached */
    boolean endReached;                             /* End checkpoint reached */
} WdgM_DeadlineSupervisionStatusType;

typedef struct {
    WdgM_CheckpointIdType lastCheckpoint;           /* Last reached checkpoint */
    boolean lastCheckpointValid;                    /* Last checkpoint valid flag */
} WdgM_LogicalSupervisionStatusType;

typedef struct {
    WdgM_LocalStatusType localStatus;               /* Current local status */
    uint8 failureCounter;                           /* Current failure counter */
    uint8 consecutiveFailures;                      /* Consecutive failures */
    boolean isActive;                               /* SE active flag */
    
    /* Supervision statuses */
    WdgM_AliveSupervisionStatusType *aliveStatus;
    WdgM_DeadlineSupervisionStatusType *deadlineStatus;
    WdgM_LogicalSupervisionStatusType logicalStatus;
} WdgM_SupervisedEntityStatusType;

typedef struct {
    boolean initialized;                            /* Module initialized flag */
    WdgM_ModeType currentMode;                      /* Current mode */
    WdgM_GlobalStatusType globalStatus;             /* Global supervision status */
    uint16 failedCyclesCounter;                     /* Failed cycles counter */
    WdgM_SupervisedEntityIdType firstExpiredSEId;   /* First expired SE ID */
    uint32 cycleCounter;                            /* Global cycle counter */
} WdgM_ModuleStatusType;

/******************************************************************************
 * External Variables
 ******************************************************************************/
extern const WdgM_ConfigType *WdgM_ConfigPtr;
extern WdgM_ModuleStatusType WdgM_ModuleStatus;
extern WdgM_SupervisedEntityStatusType WdgM_SEStatuses[WDGM_MAX_SUPERVISED_ENTITIES];

/******************************************************************************
 * WdgM Core API Functions
 ******************************************************************************/

/**
 * @brief Initialize WdgM module
 * @param configPtr Pointer to WdgM configuration
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgM_Init(const WdgM_ConfigType *configPtr);

/**
 * @brief Deinitialize WdgM module
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgM_DeInit(void);

/**
 * @brief Set WdgM mode
 * @param mode Mode to set
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgM_SetMode(WdgM_ModeType mode);

/**
 * @brief Get current WdgM mode
 * @param mode Pointer to store current mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgM_GetMode(WdgM_ModeType *mode);

/**
 * @brief Get current global supervision status
 * @return Global status
 */
WdgM_GlobalStatusType WdgM_GetGlobalStatus(void);

/**
 * @brief Get local supervision status of an SE
 * @param seId Supervised entity ID
 * @param status Pointer to store local status
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgM_GetLocalStatus(WdgM_SupervisedEntityIdType seId, WdgM_LocalStatusType *status);

/**
 * @brief Report checkpoint reached (AUTOSAR internal)
 * @param seId Supervised entity ID
 * @param checkpointId Checkpoint ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgM_CheckpointReached(
    WdgM_SupervisedEntityIdType seId,
    WdgM_CheckpointIdType checkpointId
);

/**
 * @brief Report checkpoint reached (external/custom)
 * @param seId Supervised entity ID
 * @param checkpointId Checkpoint ID
 * @param source Source of checkpoint (internal or external)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgM_CheckpointReachedExtended(
    WdgM_SupervisedEntityIdType seId,
    WdgM_CheckpointIdType checkpointId,
    WdgM_CheckpointSourceType source
);

/**
 * @brief Main function - called cyclically
 */
void WdgM_MainFunction(void);

/******************************************************************************
 * Alive Supervision Functions
 ******************************************************************************/

/**
 * @brief Update alive counter for an SE
 * @param seId Supervised entity ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgM_UpdateAliveCounter(WdgM_SupervisedEntityIdType seId);

/******************************************************************************
 * Status and Error Functions
 ******************************************************************************/

/**
 * @brief Get first expired SE ID
 * @param seId Pointer to store first expired SE ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgM_GetFirstExpiredSEID(WdgM_SupervisedEntityIdType *seId);

/**
 * @brief Perform reset (called on supervision failure)
 */
void WdgM_PerformReset(void);

/******************************************************************************
 * BswM Integration Functions
 ******************************************************************************/

/**
 * @brief Request mode change from BswM
 * @param mode Requested mode
 */
void WdgM_RequestMode(WdgM_ModeType mode);

/**
 * @brief Get current mode for BswM
 * @return Current mode
 */
WdgM_ModeType WdgM_GetCurrentMode(void);

/******************************************************************************
 * EcuM Integration Functions
 ******************************************************************************/

/**
 * @brief Set mode during startup/shutdown (called by EcuM)
 * @param mode Mode to set
 */
void WdgM_SetModeForEcuM(WdgM_ModeType mode);

/******************************************************************************
 * Application Callbacks (to be implemented by application)
 ******************************************************************************/

/**
 * @brief Called when global status changes
 * @param newStatus New global status
 * @param oldStatus Old global status
 */
extern void WdgM_GlobalStatusCallback(WdgM_GlobalStatusType newStatus, WdgM_GlobalStatusType oldStatus);

/**
 * @brief Called when local status of an SE changes
 * @param seId Supervised entity ID
 * @param newStatus New local status
 * @param oldStatus Old local status
 */
extern void WdgM_LocalStatusCallback(WdgM_SupervisedEntityIdType seId, WdgM_LocalStatusType newStatus, WdgM_LocalStatusType oldStatus);

#ifdef __cplusplus
}
#endif

#endif /* WDGM_H */
