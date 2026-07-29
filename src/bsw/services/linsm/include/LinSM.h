/**
 * @file LinSM.h
 * @brief LIN State Manager following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: LIN State Manager (LinSM)
 * Layer: Service Layer
 * Module ID: 0x8F (LINSM_MODULE_ID)
 */

#ifndef LINSM_H
#define LINSM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "LinSM_Cfg.h"
#include "ComStack_Types.h"
#include "Lin_GeneralTypes.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define LINSM_VENDOR_ID                     (0x01U) /* YuleTech Vendor ID */
#define LINSM_MODULE_ID                     (0x8FU) /* LinSM Module ID */
#define LINSM_AR_RELEASE_MAJOR_VERSION      (0x04U)
#define LINSM_AR_RELEASE_MINOR_VERSION      (0x04U)
#define LINSM_AR_RELEASE_REVISION_VERSION   (0x00U)
#define LINSM_SW_MAJOR_VERSION              (0x01U)
#define LINSM_SW_MINOR_VERSION              (0x00U)
#define LINSM_SW_PATCH_VERSION              (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define LINSM_SID_INIT                      (0x01U)
#define LINSM_SID_DEINIT                    (0x02U)
#define LINSM_SID_GET_VERSION_INFO          (0x03U)
#define LINSM_SID_SCHEDULE_REQUEST          (0x10U)
#define LINSM_SID_GET_CURRENT_SCHEDULE      (0x11U)
#define LINSM_SID_REQUEST_COM_MODE          (0x12U)
#define LINSM_SID_GET_CURRENT_COM_MODE      (0x13U)
#define LINSM_SID_MAIN_FUNCTION             (0x30U)
#define LINSM_SID_SCHEDULE_CONFIRMATION     (0x20U)
#define LINSM_SID_WAKEUP_CONFIRMATION       (0x21U)
#define LINSM_SID_GOTOSLEEP_CONFIRMATION    (0x22U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define LINSM_E_NOT_INITIALIZED             (0x01U)
#define LINSM_E_INVALID_PARAMETER           (0x02U)
#define LINSM_E_INVALID_POINTER             (0x03U)
#define LINSM_E_INVALID_SCHEDULE            (0x04U)
#define LINSM_E_STATE_NOT_VALID             (0x05U)
#define LINSM_E_INIT_FAILED                 (0x06U)

/*==================================================================================================
*                                    TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief LinSM Schedule Index type
 */
typedef uint8 LinSM_ScheduleType;

/**
 * @brief LinSM Channel type
 */
typedef uint8 LinSM_ChannelType;

/**
 * @brief LinSM Mode type (from ComM)
 */
typedef enum {
    LINSM_NO_COM = 0,           /*!< No communication */
    LINSM_FULL_COM,             /*!< Full communication */
    LINSM_SILENT_COM            /*!< Silent communication (not used for LIN) */
} LinSM_ModeType;

/**
 * @brief LinSM Schedule Status type
 */
typedef enum {
    LINSM_SCHEDULE_NULL = 0,    /*!< No schedule requested */
    LINSM_SCHEDULE_REQUESTED,   /*!< Schedule change requested */
    LINSM_SCHEDULE_RUNNING      /*!< Schedule change running */
} LinSM_ScheduleStatusType;

/**
 * @brief LinSM State type
 */
typedef enum {
    LINSM_STATE_UNINIT = 0,
    LINSM_STATE_INIT,
    LINSM_STATE_RUN,
    LINSM_STATE_WAKEUP,
    LINSM_STATE_GOTOSLEEP
} LinSM_StateType;

/**
 * @brief LinSM Configuration type
 */
typedef struct {
    LinSM_ChannelType ChannelId;        /*!< Channel ID */
    LinSM_ScheduleType InitialSchedule; /*!< Initial schedule */
    boolean WakeupSupport;              /*!< Wakeup support flag */
    uint16 RequestTimeout;              /*!< Request timeout in ms */
    uint8 MaxScheduleSwitches;          /*!< Max schedule switches */
} LinSM_ChannelConfigType;

/**
 * @brief LinSM Configuration type
 */
typedef struct {
    const LinSM_ChannelConfigType* ChannelConfig;
    uint8 NumChannels;
    uint8 MainFunctionPeriod;           /*!< Main function period in ms */
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean CommunicationControlSupport;
} LinSM_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define LINSM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const LinSM_ConfigType LinSM_Config;

#define LINSM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define LINSM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the LIN State Manager module
 * @param ConfigPtr Pointer to configuration structure
 */
extern void LinSM_Init(const LinSM_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the LIN State Manager module
 */
extern void LinSM_DeInit(void);

/**
 * @brief Gets version information
 * @param VersionInfo Pointer to version info structure
 */
#if (LINSM_VERSION_INFO_API == STD_ON)
extern void LinSM_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

/**
 * @brief Requests schedule change
 * @param Channel Channel ID
 * @param Schedule Schedule to request
 * @return Result of operation
 */
extern Std_ReturnType LinSM_ScheduleRequest(LinSM_ChannelType Channel, LinSM_ScheduleType Schedule);

/**
 * @brief Gets the current schedule for a channel
 * @param Channel Channel ID
 * @param Schedule Pointer to store current schedule
 * @return Result of operation
 */
extern Std_ReturnType LinSM_GetCurrentSchedule(LinSM_ChannelType Channel, LinSM_ScheduleType* Schedule);

/**
 * @brief Requests communication mode change
 * @param Channel Channel ID
 * @param Mode Communication mode to request
 * @return Result of operation
 */
extern Std_ReturnType LinSM_RequestComMode(LinSM_ChannelType Channel, LinSM_ModeType Mode);

/**
 * @brief Gets the current communication mode for a channel
 * @param Channel Channel ID
 * @param Mode Pointer to store current mode
 * @return Result of operation
 */
extern Std_ReturnType LinSM_GetCurrentComMode(LinSM_ChannelType Channel, LinSM_ModeType* Mode);

/**
 * @brief Main function for LinSM (to be called periodically)
 */
extern void LinSM_MainFunction(void);

/**
 * @brief Schedule confirmation callback from LinM
 * @param Channel Channel ID
 * @param Schedule Schedule that was confirmed
 */
extern void LinSM_ScheduleConfirmation(LinSM_ChannelType Channel, LinSM_ScheduleType Schedule);

/**
 * @brief Wakeup confirmation callback from LinM
 * @param Channel Channel ID
 * @param Success TRUE if wakeup succeeded, FALSE otherwise
 */
extern void LinSM_WakeUpConfirmation(LinSM_ChannelType Channel, boolean Success);

/**
 * @brief Go-to-sleep confirmation callback from LinM
 * @param Channel Channel ID
 * @param Success TRUE if go-to-sleep succeeded, FALSE otherwise
 */
extern void LinSM_GotoSleepConfirmation(LinSM_ChannelType Channel, boolean Success);

#define LINSM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* LINSM_H */
