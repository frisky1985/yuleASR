/**
 * @file LinM.h
 * @brief LIN Master Management module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: LIN Master Management (LinM)
 * Layer: Service Layer
 * Module ID: 0x8E (LINM_MODULE_ID)
 */

#ifndef LINM_H
#define LINM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "LinM_Cfg.h"
#include "ComStack_Types.h"
#include "Lin_GeneralTypes.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define LINM_VENDOR_ID                      (0x01U) /* YuleTech Vendor ID */
#define LINM_INSTANCE_ID           0U
#define LINM_MODULE_ID                      (0x8EU) /* LinM Module ID */
#define LINM_AR_RELEASE_MAJOR_VERSION       (0x04U)
#define LINM_AR_RELEASE_MINOR_VERSION       (0x04U)
#define LINM_AR_RELEASE_REVISION_VERSION    (0x00U)
#define LINM_SW_MAJOR_VERSION               (0x01U)
#define LINM_SW_MINOR_VERSION               (0x00U)
#define LINM_SW_PATCH_VERSION               (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define LINM_SID_INIT                       (0x01U)
#define LINM_SID_DEINIT                     (0x02U)
#define LINM_SID_GET_VERSION_INFO           (0x03U)
#define LINM_SID_INIT_SCHEDULE              (0x04U)
#define LINM_SID_START_SCHEDULE             (0x05U)
#define LINM_SID_STOP_SCHEDULE              (0x06U)
#define LINM_SID_SET_SCHEDULE_MODE          (0x07U)
#define LINM_SID_GET_SCHEDULE_STATUS        (0x08U)
#define LINM_SID_MAIN_FUNCTION              (0x09U)
#define LINM_SID_WAKEUP                     (0x0AU)
#define LINM_SID_GOTOSLEEP                  (0x0BU)
#define LINM_SID_GET_SLAVE_RESPONSE         (0x0CU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define LINM_E_NOT_INITIALIZED              (0x01U)
#define LINM_E_INVALID_PARAMETER            (0x02U)
#define LINM_E_INVALID_POINTER              (0x03U)
#define LINM_E_INVALID_SCHEDULE             (0x04U)
#define LINM_E_SCHEDULE_NOT_RUNNING         (0x05U)
#define LINM_E_INIT_FAILED                  (0x06U)

/*==================================================================================================
*                                    TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief LIN Master Channel type
 */
typedef uint8 LinM_ChannelType;

/**
 * @brief LIN Schedule type
 */
typedef uint8 LinM_ScheduleType;

/**
 * @brief LIN Schedule Entry type
 */
typedef uint8 LinM_ScheduleEntryType;

/**
 * @brief LIN Slave Response Status type
 */
typedef enum {
    LINM_SLAVE_RESPONSE_OK = 0,
    LINM_SLAVE_RESPONSE_ERROR,
    LINM_SLAVE_RESPONSE_TIMEOUT,
    LINM_SLAVE_RESPONSE_INVALID
} LinM_SlaveResponseStatusType;

/**
 * @brief LIN Schedule Mode type
 */
typedef enum {
    LINM_SCHEDULE_MODE_STOPPED = 0,
    LINM_SCHEDULE_MODE_STARTED,
    LINM_SCHEDULE_MODE_CONTINUE,
    LINM_SCHEDULE_MODE_ONCE
} LinM_ScheduleModeType;

/**
 * @brief LIN Schedule Status type
 */
typedef enum {
    LINM_SCHEDULE_IDLE = 0,
    LINM_SCHEDULE_RUNNING,
    LINM_SCHEDULE_WAITING,
    LINM_SCHEDULE_PAUSED
} LinM_ScheduleStatusType;

/**
 * @brief LIN Schedule Entry structure
 */
typedef struct {
    uint16 Delay;                   /*!< Entry delay in ms */
    uint8  FrameIndex;              /*!< Frame index or 0xFF for empty */
    uint8  FrameType;               /*!< Frame type (unconditional/event triggered/etc) */
} LinM_ScheduleEntryConfigType;

/**
 * @brief LIN Schedule Configuration type
 */
typedef struct {
    LinM_ScheduleType ScheduleId;                   /*!< Schedule ID */
    const LinM_ScheduleEntryConfigType* Entries;    /*!< Schedule entries */
    uint8 NumEntries;                               /*!< Number of entries */
    uint8 Priority;                                 /*!< Schedule priority */
    boolean IsEventTriggered;                       /*!< Event triggered flag */
} LinM_ScheduleConfigType;

/**
 * @brief LIN Channel Configuration type
 */
typedef struct {
    LinM_ChannelType ChannelId;                     /*!< Channel ID */
    const LinM_ScheduleConfigType* Schedules;       /*!< Schedule configurations */
    uint8 NumSchedules;                             /*!< Number of schedules */
    uint8 NumFrames;                                /*!< Number of frames */
    uint16 ScheduleTimerBase;                       /*!< Schedule timer base in ms */
    boolean WakeupSupport;                          /*!< Wakeup support flag */
    boolean SleepSupport;                           /*!< Sleep support flag */
} LinM_ChannelConfigType;

/**
 * @brief LIN Master Configuration type
 */
typedef struct {
    const LinM_ChannelConfigType* ChannelConfig;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} LinM_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define LINM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const LinM_ConfigType LinM_Config;

#define LINM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define LINM_START_SEC_CODE
#include "MemMap.h"

/** @req SWS_LinM_00001 */
/**
 * @brief Initializes the LIN Master Management module
 * @param ConfigPtr Pointer to configuration structure
 */
extern void LinM_Init(const LinM_ConfigType* ConfigPtr);

/** @req SWS_LinM_00002 */
/**
 * @brief Deinitializes the LIN Master Management module
 */
extern void LinM_DeInit(void);

/**
 * @brief Gets version information
 * @param VersionInfo Pointer to version info structure
 */
#if (LINM_VERSION_INFO_API == STD_ON)
/** @req SWS_LinM_00003 */
extern void LinM_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

/** @req SWS_LinM_00005 */
/**
 * @brief Initializes a schedule for a channel
 * @param Channel Channel ID
 * @param Schedule Schedule ID
 * @return Result of operation
 */
extern Std_ReturnType LinM_InitSchedule(LinM_ChannelType Channel, LinM_ScheduleType Schedule);

/** @req SWS_LinM_00006 */
/**
 * @brief Starts a schedule for a channel
 * @param Channel Channel ID
 * @param Schedule Schedule ID
 * @return Result of operation
 */
extern Std_ReturnType LinM_StartSchedule(LinM_ChannelType Channel, LinM_ScheduleType Schedule);

/** @req SWS_LinM_00007 */
/**
 * @brief Stops a schedule for a channel
 * @param Channel Channel ID
 * @return Result of operation
 */
extern Std_ReturnType LinM_StopSchedule(LinM_ChannelType Channel);

/** @req SWS_LinM_00008 */
/**
 * @brief Sets schedule mode for a channel
 * @param Channel Channel ID
 * @param Mode Schedule mode
 * @return Result of operation
 */
extern Std_ReturnType LinM_SetScheduleMode(LinM_ChannelType Channel, LinM_ScheduleModeType Mode);

/** @req SWS_LinM_00009 */
/**
 * @brief Gets schedule status for a channel
 * @param Channel Channel ID
 * @param Status Pointer to store status
 * @return Result of operation
 */
extern Std_ReturnType LinM_GetScheduleStatus(LinM_ChannelType Channel, LinM_ScheduleStatusType* Status);

/** @req SWS_LinM_00004 */
/**
 * @brief Main function for LinM (to be called periodically)
 */
extern void LinM_MainFunction(void);

/** @req SWS_LinM_00010 */
/**
 * @brief Sends wakeup signal on LIN bus
 * @param Channel Channel ID
 * @return Result of operation
 */
extern Std_ReturnType LinM_WakeUp(LinM_ChannelType Channel);

/** @req SWS_LinM_00011 */
/**
 * @brief Sends go-to-sleep command on LIN bus
 * @param Channel Channel ID
 * @return Result of operation
 */
extern Std_ReturnType LinM_GotoSleep(LinM_ChannelType Channel);

/** @req SWS_LinM_00012 */
/**
 * @brief Gets slave response status
 * @param Channel Channel ID
 * @param Status Pointer to store status
 * @return Result of operation
 */
extern Std_ReturnType LinM_GetSlaveResponse(LinM_ChannelType Channel, LinM_SlaveResponseStatusType* Status);

#define LINM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* LINM_H */
