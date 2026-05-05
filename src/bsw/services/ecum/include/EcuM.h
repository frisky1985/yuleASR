/**
 * @file EcuM.h
 * @brief ECU State Manager - Multi-Phase Startup/Shutdown Implementation
 * @version 2.0.0
 * @implements AUTOSAR Classic Platform EcuM SWS
 */

#ifndef ECUM_H
#define ECUM_H

#include "Std_Types.h"

/* AUTOSAR Version */
#define ECUM_AR_RELEASE_MAJOR_VERSION       4
#define ECUM_AR_RELEASE_MINOR_VERSION       0
#define ECUM_AR_RELEASE_REVISION_VERSION    3

/* Module Version */
#define ECUM_SW_MAJOR_VERSION               2
#define ECUM_SW_MINOR_VERSION               0
#define ECUM_SW_PATCH_VERSION               0

/* Service IDs */
#define ECUM_INIT_SID                       0x00
#define ECUM_SHUTDOWN_SID                   0x01
#define ECUM_REQUESTRUN_SID                 0x02
#define ECUM_RELEASERUN_SID                 0x03
#define ECUM_SELECTSHUTDOWNTARGET_SID       0x04
#define ECUM_GETSTATE_SID                   0x05
#define ECUM_COMMODEREQUEST_SID             0x06
#define ECUM_SETWAKEUPEVENT_SID             0x07
#define ECUM_VALIDATEMCUWAKEUPEVENT_SID     0x08
#define ECUM_GETSHUTDOWNTARGET_SID          0x09
#define ECUM_COMMODERERELEASE_SID           0x0A
#define ECUM_SELECTBOOTTARGET_SID           0x0B
#define ECUM_GETBOOTTARGET_SID              0x0C
#define ECUM_SELECTAPPMODE_SID              0x0D
#define ECUM_GETAPPMODE_SID                 0x0E
#define ECUM_STARTUPONE_SID                 0x0F
#define ECUM_STARTUPTWO_SID                 0x10
#define ECUM_SLEEP_SID                      0x11
#define ECUM_HALT_SID                       0x12
#define ECUM_POLL_SID                       0x13
#define ECUM_WAKEUPRESTART_SID              0x14
#define ECUM_CLEARWAKEUPEVENT_SID           0x15
#define ECUM_CHECKWAKEUP_SID                0x16
#define ECUM_ENABLEWAKEUPSOURCES_SID        0x17
#define ECUM_DISABLEWAKEUPSOURCES_SID       0x18
#define ECUM_GETSTATUSOFWAKEUPSOURCE_SID    0x19
#define ECUM_GETWAKEUPSOURCES_SID           0x1A
#define ECUM_MAINFUNCTION_SID               0x1B
#define ECUM_KILLALLRUNREQUESTS_SID         0x1C
#define ECUM_GETLASTSHUTDOWNTARGET_SID      0x1D
#define ECUM_GETSHUTDOWNCAUSE_SID           0x1E
#define ECUM_SELECTSHUTDOWNCAUSE_SID        0x1F

/* Error Codes */
#define ECUM_E_NOT_INITIALIZED              0x10
#define ECUM_E_INVALID_PAR                  0x11
#define ECUM_E_NULL_POINTER                 0x12
#define ECUM_E_STATE_CHANGE_FAILED          0x13
#define ECUM_E_ARC_RELEASE_VERSION          0x14
#define ECUM_E_MODULE_NOT_CONFIGURED        0x15
#define ECUM_E_WRONG_API_ORDER              0x16
#define ECUM_E_MULTIPLE_RAM_BLOCKS          0x17
#define ECUM_E_INVALID_SDUR                 0x18
#define ECUM_E_ALL_CORES_NOT_SYNCHRONIZED   0x19
#define ECUM_E_MULTIPLE_MODULES             0x1A

/*******************************************************************************
 *                                State Definitions                            *
 ******************************************************************************/

/* Main ECU States */
typedef uint8 EcuM_StateType;
#define ECUM_STATE_OFF                      0x00u
#define ECUM_STATE_STARTUP                  0x01u
#define ECUM_STATE_RUN                      0x10u
#define ECUM_STATE_POST_RUN                 0x20u
#define ECUM_STATE_SHUTDOWN                 0x40u
#define ECUM_STATE_SLEEP                    0x30u
#define ECUM_STATE_WAKE_SLEEP               0x31u
#define ECUM_STATE_APP_RUN                  0x11u
#define ECUM_STATE_APP_POST_RUN             0x21u

/*******************************************************************************
 *                              Sub-State Definitions                          *
 ******************************************************************************/

typedef uint8 EcuM_SubStateType;

/* Startup Phase Sub-States */
#define ECUM_SUBSTATE_STARTUP_ONE           0x11u   /* First startup phase - MCU init */
#define ECUM_SUBSTATE_STARTUP_TWO           0x12u   /* Second startup phase - BSW init */
#define ECUM_SUBSTATE_STARTUP_THREE         0x13u   /* Third startup phase - SWC init */

/* Run Phase Sub-States */
#define ECUM_SUBSTATE_RUN                   0x21u   /* Normal RUN mode */
#define ECUM_SUBSTATE_POST_RUN              0x22u   /* Post RUN - deinitializing */

/* Sleep Phase Sub-States */
#define ECUM_SUBSTATE_GO_SLEEP              0x31u   /* Prepare to enter sleep */
#define ECUM_SUBSTATE_SLEEP                 0x32u   /* In sleep mode */
#define ECUM_SUBSTATE_WAKEUP_ONE            0x33u   /* First wakeup phase */
#define ECUM_SUBSTATE_WAKEUP_TWO            0x34u   /* Second wakeup phase */

/* Shutdown Phase Sub-States */
#define ECUM_SUBSTATE_GO_OFF_ONE            0x41u   /* First go off phase - write NV */
#define ECUM_SUBSTATE_GO_OFF_TWO            0x42u   /* Second go off phase - shutdown OS */
#define ECUM_SUBSTATE_RESET                 0x43u   /* Reset state */

/* Special Sub-States */
#define ECUM_SUBSTATE_HALT                  0x50u   /* Halt mode (stopped clock) */
#define ECUM_SUBSTATE_POLL                  0x51u   /* Poll mode (active wait) */

/*******************************************************************************
 *                            Shutdown Target Types                            *
 ******************************************************************************/

typedef uint8 EcuM_ShutdownTargetType;
#define ECUM_SHUTDOWN_TARGET_OFF            0x00u
#define ECUM_SHUTDOWN_TARGET_RESET          0x01u
#define ECUM_SHUTDOWN_TARGET_SLEEP          0x02u

/*******************************************************************************
 *                              Shutdown Cause Types                           *
 ******************************************************************************/

typedef uint8 EcuM_ShutdownCauseType;
#define ECUM_CAUSE_ECU_STATE                0x00u
#define ECUM_CAUSE_WATCHDOG                 0x01u
#define ECUM_CAUSE_HARDWARE                 0x02u
#define ECUM_CAUSE_SOFTWARE                 0x03u
#define ECUM_CAUSE_FATAL_ERROR              0x04u
#define ECUM_CAUSE_DCM                      0x05u
#define ECUM_CAUSE_UNDEFINED                0xFFu

/*******************************************************************************
 *                              Boot Target Types                              *
 ******************************************************************************/

typedef uint8 EcuM_BootTargetType;
#define ECUM_BOOT_TARGET_OEM_BOOTLOADER     0x00u
#define ECUM_BOOT_TARGET_SYS_BOOTLOADER     0x01u
#define ECUM_BOOT_TARGET_APPLICATION        0x02u

/*******************************************************************************
 *                              Wakeup Types                                   *
 ******************************************************************************/

typedef uint32 EcuM_WakeupSourceType;
#define ECUM_WKSOURCE_NONE                  0x00000000u
#define ECUM_WKSOURCE_POWER                 0x00000001u
#define ECUM_WKSOURCE_RESET                 0x00000002u
#define ECUM_WKSOURCE_INTERNAL_RESET        0x00000004u
#define ECUM_WKSOURCE_INTERNAL_WDG          0x00000008u
#define ECUM_WKSOURCE_EXTERNAL_WDG          0x00000010u
#define ECUM_WKSOURCE_TIMER                 0x00000020u
#define ECUM_WKSOURCE_CAN                   0x00000040u
#define ECUM_WKSOURCE_CAN0                  0x00000040u
#define ECUM_WKSOURCE_CAN1                  0x00000080u
#define ECUM_WKSOURCE_CAN2                  0x00000100u
#define ECUM_WKSOURCE_CAN3                  0x00000200u
#define ECUM_WKSOURCE_CAN4                  0x00000400u
#define ECUM_WKSOURCE_LIN                   0x00000800u
#define ECUM_WKSOURCE_LIN0                  0x00000800u
#define ECUM_WKSOURCE_LIN1                  0x00001000u
#define ECUM_WKSOURCE_LIN2                  0x00002000u
#define ECUM_WKSOURCE_LIN3                  0x00004000u
#define ECUM_WKSOURCE_ETH                   0x00008000u
#define ECUM_WKSOURCE_ETH0                  0x00008000u
#define ECUM_WKSOURCE_ETH1                  0x00010000u
#define ECUM_WKSOURCE_FLEXRAY               0x00020000u
#define ECUM_WKSOURCE_FLEXRAY0              0x00020000u
#define ECUM_WKSOURCE_FLEXRAY1              0x00040000u
#define ECUM_WKSOURCE_FR                    0x00020000u
#define ECUM_WKSOURCE_SPI                   0x00080000u
#define ECUM_WKSOURCE_I2C                   0x00100000u
#define ECUM_WKSOURCE_GPIO                  0x00200000u
#define ECUM_WKSOURCE_ADC                   0x00400000u
#define ECUM_WKSOURCE_KEY                   0x00800000u
#define ECUM_WKSOURCE_NVM                   0x01000000u
#define ECUM_WKSOURCE_COMM                  0x02000000u
#define ECUM_WKSOURCE_DCM                   0x04000000u
#define ECUM_WKSOURCE_ALL_SOURCES           0xFFFFFFFFu

/*******************************************************************************
 *                            Wakeup Status Types                              *
 ******************************************************************************/

typedef uint8 EcuM_WakeupStatusType;
#define ECUM_WKSTATUS_NONE                  0x00u
#define ECUM_WKSTATUS_PENDING               0x01u
#define ECUM_WKSTATUS_VALIDATED             0x02u
#define ECUM_WKSTATUS_EXPIRED               0x03u
#define ECUM_WKSTATUS_DISABLED              0x04u

/*******************************************************************************
 *                              User & Mode Types                              *
 ******************************************************************************/

typedef uint8 EcuM_UserType;
#define ECUM_USER_END_OF_LIST               0xFFu

typedef uint8 EcuM_ModeType;

typedef uint8 EcuM_AppModeType;
#define ECUM_APPMODE_DEFAULT                0x00u

/*******************************************************************************
 *                             BSW Mode Types                                  *
 ******************************************************************************/

typedef uint8 EcuM_BswModeType;
#define ECUM_BSWSTARTUP_MODE                0x00u
#define ECUM_BSWSTARTUP_TWO_MODE            0x01u
#define ECUM_BSWPREP_SHUTDOWN_MODE          0x02u
#define ECUM_BSWGO_OFF_ONE_MODE             0x03u
#define ECUM_BSWGO_OFF_TWO_MODE             0x04u

/*******************************************************************************
 *                              Configuration Types                            *
 ******************************************************************************/

typedef struct {
    EcuM_WakeupSourceType WakeupSource;
    uint32 ValidationTimeout;
    boolean CheckWakeupTimeEnabled;
    uint32 CheckWakeupTime;
} EcuM_WakeupSourceConfigType;

typedef struct {
    const EcuM_WakeupSourceConfigType* WakeupSources;
    uint8 NumWakeupSources;
    boolean ComMConfigEnabled;
    boolean NvmConfigEnabled;
    boolean WdgMConfigEnabled;
} EcuM_ConfigType;

/*******************************************************************************
 *                           Function Prototypes                               *
 ******************************************************************************/

/* Initialization Functions */
extern void EcuM_Init(void);
extern void EcuM_StartupOne(void);
extern void EcuM_StartupTwo(void);

/* Runtime Functions */
extern void EcuM_MainFunction(void);
extern Std_ReturnType EcuM_RequestRUN(EcuM_UserType user);
extern Std_ReturnType EcuM_ReleaseRUN(EcuM_UserType user);
extern Std_ReturnType EcuM_KillAllRUNRequests(void);

/* State Management */
extern Std_ReturnType EcuM_GetState(EcuM_StateType* state);
extern Std_ReturnType EcuM_GetSubState(EcuM_SubStateType* subState);

/* Shutdown Management */
extern void EcuM_Shutdown(void);
extern Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType target, uint8 mode);
extern Std_ReturnType EcuM_GetShutdownTarget(EcuM_ShutdownTargetType* target, uint8* mode);
extern Std_ReturnType EcuM_GetLastShutdownTarget(EcuM_ShutdownTargetType* target, uint8* mode);
extern Std_ReturnType EcuM_SelectShutdownCause(EcuM_ShutdownCauseType cause);
extern Std_ReturnType EcuM_GetShutdownCause(EcuM_ShutdownCauseType* cause);

/* Sleep Management */
extern void EcuM_GoSleep(void);
extern void EcuM_GoHalt(void);
extern void EcuM_GoPoll(void);
extern void EcuM_WakeupRestart(void);

/* Wakeup Source Management */
extern void EcuM_SetWakeupEvent(EcuM_WakeupSourceType sources);
extern void EcuM_ClearWakeupEvent(EcuM_WakeupSourceType sources);
extern void EcuM_CheckWakeup(EcuM_WakeupSourceType sources);
extern Std_ReturnType EcuM_EnableWakeupSources(EcuM_WakeupSourceType sources);
extern Std_ReturnType EcuM_DisableWakeupSources(EcuM_WakeupSourceType sources);
extern EcuM_WakeupStatusType EcuM_GetStatusOfWakeupSource(EcuM_WakeupSourceType sources);
extern Std_ReturnType EcuM_GetWakeupSources(EcuM_WakeupSourceType* sources);
extern Std_ReturnType EcuM_CheckValidation(EcuM_WakeupSourceType source);

/* Boot Target Management */
extern Std_ReturnType EcuM_SelectBootTarget(EcuM_BootTargetType target);
extern Std_ReturnType EcuM_GetBootTarget(EcuM_BootTargetType* target);

/* Application Mode */
extern Std_ReturnType EcuM_SelectApplicationMode(EcuM_AppModeType appMode);
extern Std_ReturnType EcuM_GetApplicationMode(EcuM_AppModeType* appMode);

/* BSW Mode Management */
extern void EcuM_StartBswMode(EcuM_BswModeType mode);
extern void EcuM_StopBswMode(EcuM_BswModeType mode);

/* Communication Mode */
extern Std_ReturnType EcuM_ComM_RequestComMode(uint8 channel, EcuM_ModeType mode);
extern Std_ReturnType EcuM_ComM_ReleaseComMode(uint8 channel);

/* Version Info */
extern void EcuM_GetVersionInfo(Std_VersionInfoType* versionInfo);

/* Callout Declarations - To be implemented by integrator */
extern void EcuM_DriverInitOne(const EcuM_ConfigType* config);
extern void EcuM_DriverInitTwo(const EcuM_ConfigType* config);
extern void EcuM_DriverInitThree(const EcuM_ConfigType* config);
extern void EcuM_DriverRestart(const EcuM_ConfigType* config);
extern void EcuM_AL_DriverInitOne(const EcuM_ConfigType* config);
extern void EcuM_AL_DriverInitTwo(const EcuM_ConfigType* config);
extern void EcuM_AL_DriverInitThree(const EcuM_ConfigType* config);
extern void EcuM_AL_DriverRestart(const EcuM_ConfigType* config);
extern void EcuM_AL_SwitchOff(void);
extern void EcuM_AL_Reset(EcuM_ResetType resetType);
extern void EcuM_AL_EnterSleep(void);
extern void EcuM_AL_WakeupCheck(void);
extern void EcuM_AL_WakeupValidation(void);
extern void EcuM_AL_WakeupReaction(void);

#endif /* ECUM_H */
