/************************************************************************************
 * File: LinSM.h
 * Description: LIN State Manager - Main Header File
 * AUTOSAR Version: 4.4.0
 *
 * Module: LinSM (LIN State Manager)
 * Purpose: Manages the state of LIN channels and schedule tables
 *
 * The LinSM (LIN State Manager) is responsible for:
 * - Managing communication modes (NO_COM, FULL_COM) per LIN channel
 * - Schedule table switching and management
 * - Sleep/wake-up handling for LIN channels
 * - Integration with ComM for communication mode management
 ************************************************************************************/

#ifndef LINSM_H
#define LINSM_H

#include "Std_Types.h"
#include "ComStack_Types.h"
#include "Lin_GeneralTypes.h"
#include "LinSM_Cfg.h"
#include "ComM.h"
#include "EcuM.h"

/*================================================================================
 * Module Version Information
 *===============================================================================*/
#define LINSM_VENDOR_ID               (1001U)
#define LINSM_MODULE_ID               (90U)

#define LINSM_SW_MAJOR_VERSION        (1U)
#define LINSM_SW_MINOR_VERSION        (0U)
#define LINSM_SW_PATCH_VERSION        (0U)

#define LINSM_AR_RELEASE_MAJOR_VERSION  (4U)
#define LINSM_AR_RELEASE_MINOR_VERSION  (4U)
#define LINSM_AR_RELEASE_REVISION_VERSION (0U)

/*================================================================================
 * Service IDs for Error Reporting
 *===============================================================================*/
#define LINSM_SID_INIT                     (0x00U)
#define LINSM_SID_DEINIT                   (0x01U)
#define LINSM_SID_REQUESTCOMMODE           (0x02U)
#define LINSM_SID_GETCURRENTCOMMODE        (0x03U)
#define LINSM_SID_SCHEDULEREQUEST          (0x04U)
#define LINSM_SID_GETVERSIONINFO           (0x05U)
#define LINSM_SID_MAINFUNCTION             (0x06U)
#define LINSM_SID_WAKEUPCONFIRMATION       (0x21U)
#define LINSM_SID_GOTOSLEEPCONFIRMATION    (0x22U)
#define LINSM_SID_SCHEDULETABLEREQUEST     (0x20U)

/*================================================================================
 * Development Error Codes
 *===============================================================================*/
#define LINSM_E_UNINIT                     (0x01U)
#define LINSM_E_NONEXISTENT_CHANNEL        (0x02U)
#define LINSM_E_PARAMETER_POINTER          (0x03U)
#define LINSM_E_INVALID_SCHEDULE           (0x04U)
#define LINSM_E_PARAMETER                  (0x05U)
#define LINSM_E_STATE_TRANSITION           (0x06U)
#define LINSM_E_INIT_FAILED                (0x07U)

/*================================================================================
 * Communication Mode Types
 *===============================================================================*/
typedef enum
{
    LINSM_NO_COM = 0,        /* No communication mode */
    LINSM_FULL_COM           /* Full communication mode */
} LinSM_ModeType;

/*================================================================================
 * Schedule Types
 *===============================================================================*/
typedef enum
{
    LINSM_RUN_CONTINUOUS = 0,    /* Run schedule continuously */
    LINSM_RUN_ONCE               /* Run schedule once */
} LinSM_ScheduleType;

/*================================================================================
 * Schedule Table Type
 *===============================================================================*/
typedef enum
{
    LINSM_NULL_SCHEDULE = 0,     /* No schedule selected */
    LINSM_DIAG_REQUEST,          /* Diagnostic request schedule */
    LINSM_DIAG_RESPONSE,         /* Diagnostic response schedule */
    LINSM_NORMAL_TABLE,          /* Normal communication schedule */
    LINSM_MASTER_TABLE,          /* Master command schedule */
    LINSM_SLAVE_RESPONSE_TABLE   /* Slave response schedule */
} LinSM_ScheduleTableType;

/*================================================================================
 * Schedule Table Info
 *===============================================================================*/
typedef struct
{
    uint8 Channel;               /* LIN channel identifier */
    uint8 Schedule;              /* Schedule table index */
    LinSM_ScheduleType RunMode;  /* Continuous or once */
} LinSM_ScheduleInfoType;

/*================================================================================
 * Wake-up Source Configuration
 *===============================================================================*/
typedef struct
{
    EcuM_WakeupSourceType WakeupSource;
    uint8                 ChannelId;
} LinSM_WakeupSourceConfigType;

/*================================================================================
 * Channel Configuration
 *===============================================================================*/
typedef struct
{
    uint8                    ChannelId;           /* LIN channel ID */
    uint8                    ComMChannelId;       /* ComM channel mapping */
    uint16                   ConfirmationTimeout; /* Confirmation timeout in ms */
    uint16                   ModeRequestRepetitionTime; /* Repetition time */
    uint8                    ScheduleCount;       /* Number of schedules */
    const uint8             *ScheduleIdList;      /* Schedule table IDs */
    boolean                  WakeupSupport;       /* Wake-up supported */
    EcuM_WakeupSourceType    WakeupSource;        /* Wake-up source */
} LinSM_ChannelConfigType;

/*================================================================================
 * Global Configuration
 *===============================================================================*/
typedef struct
{
    uint8                       ChannelCount;
    const LinSM_ChannelConfigType  *ChannelConfig;
    const LinSM_WakeupSourceConfigType *WakeupSources;
    uint8                       WakeupSourceCount;
} LinSM_ConfigType;

/*================================================================================
 * Function Prototypes
 *===============================================================================*/

/**
 * @brief Initializes the LIN State Manager module.
 * @param ConfigPtr - Pointer to configuration structure
 * @return void
 * @pre None
 * @post Module initialized and ready for operation
 */
extern void LinSM_Init(const LinSM_ConfigType *ConfigPtr);

/**
 * @brief De-initializes the LIN State Manager module.
 * @param void
 * @return void
 * @pre Module must be initialized
 * @post Module de-initialized
 */
extern void LinSM_DeInit(void);

/**
 * @brief Requests a communication mode for a LIN channel.
 * @param Channel - LIN channel handle
 * @param Mode - Requested communication mode (NO_COM or FULL_COM)
 * @return Std_ReturnType - E_OK if request accepted, E_NOT_OK otherwise
 * @pre Module must be initialized
 * @post Communication mode transition initiated
 */
extern Std_ReturnType LinSM_RequestComMode(
    uint8 Channel,
    ComM_ModeType Mode
);

/**
 * @brief Gets the current communication mode of a LIN channel.
 * @param Channel - LIN channel handle
 * @param Mode - Pointer to store current mode
 * @return Std_ReturnType - E_OK if successful, E_NOT_OK otherwise
 * @pre Module must be initialized
 * @post Current mode stored in Mode parameter
 */
extern Std_ReturnType LinSM_GetCurrentComMode(
    uint8 Channel,
    ComM_ModeType *Mode
);

/**
 * @brief Requests a schedule table for a LIN channel.
 * @param Channel - LIN channel handle
 * @param Schedule - Schedule table index
 * @return Std_ReturnType - E_OK if request accepted, E_NOT_OK otherwise
 * @pre Module must be initialized and in FULL_COM mode
 * @post Schedule table request initiated
 */
extern Std_ReturnType LinSM_ScheduleRequest(
    uint8 Channel,
    uint8 Schedule
);

/**
 * @brief Gets the version information of the LinSM module.
 * @param VersionInfo - Pointer to store version information
 * @return void
 * @pre None
 * @post Version information stored in VersionInfo
 */
#if (LINSM_VERSION_INFO_API == STD_ON)
extern void LinSM_GetVersionInfo(Std_VersionInfoType *VersionInfo);
#endif

/**
 * @brief Main function - must be called cyclically.
 * @param void
 * @return void
 * @pre Module must be initialized
 * @post State machine transitions processed
 */
extern void LinSM_MainFunction(void);

/**
 * @brief Callback from LinIf for schedule table request confirmation.
 * @param Channel - LIN channel handle
 * @param Schedule - Schedule table index
 * @return void
 */
extern void LinSM_ScheduleTableRequest(
    uint8 Channel,
    uint8 Schedule
);

/**
 * @brief Callback from LinIf for wake-up confirmation.
 * @param Channel - LIN channel handle
 * @param Success - TRUE if wake-up successful
 * @return void
 */
extern void LinSM_WakeUpConfirmation(uint8 Channel, boolean Success);

/**
 * @brief Callback from LinIf for go-to-sleep confirmation.
 * @param Channel - LIN channel handle
 * @param Success - TRUE if sleep successful
 * @return void
 */
extern void LinSM_GoToSleepConfirmation(uint8 Channel, boolean Success);

#endif /* LINSM_H */
