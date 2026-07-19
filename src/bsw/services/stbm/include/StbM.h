/**
 * @file StbM.h
 * @brief Synchronized Time-base Manager - AutoSAR R22-11 Service Layer
 * @version 4.7.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Synchronized Time-base Manager (STBM)
 * Module ID: 0xA2U
 * Layer: Service Layer
 */

#ifndef STBM_H
#define STBM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "StbM_Cfg.h"
#include "Eth.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define STBM_VENDOR_ID                          (0x01U) /* YuleTech Vendor ID */
#define STBM_MODULE_ID                          (0xA2U) /* STBM Module ID */
#define STBM_INSTANCE_ID                        (0x00U)

#define STBM_AR_RELEASE_MAJOR_VERSION           (0x22U)
#define STBM_AR_RELEASE_MINOR_VERSION           (0x11U)
#define STBM_AR_RELEASE_REVISION_VERSION        (0x00U)

#define STBM_SW_MAJOR_VERSION                   (0x04U)
#define STBM_SW_MINOR_VERSION                   (0x07U)
#define STBM_SW_PATCH_VERSION                   (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define STBM_SID_INIT                           (0x01U)
#define STBM_SID_DEINIT                         (0x02U)
#define STBM_SID_GETVERSIONINFO                 (0x03U)
#define STBM_SID_GETCURRENTTIME                 (0x04U)
#define STBM_SID_GETCURRENTVIRTUALTIME          (0x05U)
#define STBM_SID_SETGLOBALTIME                  (0x06U)
#define STBM_SID_BUSSETGLOBALTIME               (0x07U)
#define STBM_SID_GETTIMEBASESTATUS              (0x08U)
#define STBM_SID_GETMASTERCONFIG                (0x09U)
#define STBM_SID_SETRATECORRECTION              (0x0AU)
#define STBM_SID_GETTIMEBASEUPDATECOUNTER       (0x0BU)
#define STBM_SID_GETCURRENTTIMEDIFF             (0x0CU)
#define STBM_SID_SETUSERDATA                    (0x0DU)
#define STBM_SID_MAINFUNCTION                   (0x0EU)
#define STBM_SID_TIMESTAMPCHANGED               (0x0FU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define STBM_E_PARAM_POINTER                    (0x01U)
#define STBM_E_PARAM_CONFIG                     (0x02U)
#define STBM_E_UNINIT                           (0x03U)
#define STBM_E_ALREADY_INITIALIZED              (0x04U)
#define STBM_E_INVALID_TIMEBASE_ID              (0x05U)
#define STBM_E_PARAM                            (0x06U)
#define STBM_E_NOT_SUPPORTED                    (0x07U)
#define STBM_E_SYNC_FAILED                      (0x08U)
#define STBM_E_TIMEOUT                          (0x09U)

/*==================================================================================================
*                                    TIME BASE TYPES
==================================================================================================*/
#define STBM_TIMEBASE_LOCAL                     (0x00U)
#define STBM_TIMEBASE_GLOBAL                    (0x01U)

/*==================================================================================================
*                                    TIME BASE STATUS
==================================================================================================*/
#define STBM_SYNC_STATUS_UNKNOWN                (0x00U)
#define STBM_SYNC_STATUS_SYNC                   (0x01U)
#define STBM_SYNC_STATUS_SYNC_LOST              (0x02U)
#define STBM_SYNC_STATUS_SYNC_DEGRADED          (0x03U)

#define STBM_TIMEBASE_STATUS_OK                 (0x00U)
#define STBM_TIMEBASE_STATUS_PENDING            (0x01U)
#define STBM_TIMEBASE_STATUS_TIMEOUT            (0x02U)

/*==================================================================================================
*                                    MASTER/SLAVE CONFIG
==================================================================================================*/
typedef enum {
    STBM_MASTER_CONFIG_NONE = 0,
    STBM_MASTER_CONFIG_SLAVE,
    STBM_MASTER_CONFIG_MASTER,
    STBM_MASTER_CONFIG_MASTER_ACTIVE_PASSIVE
} StbM_MasterConfigType;

/*==================================================================================================
*                                    TIME STAMP TYPE
==================================================================================================*/
typedef struct {
    uint32 nanoseconds;
    uint32 seconds;
    uint16 secondsHi;
    uint8  timeBaseStatus;      /* Time base status flags */
} StbM_TimeStampType;

/*==================================================================================================
*                                    USER DATA TYPE
==================================================================================================*/
typedef struct {
    uint8 userByte0;
    uint8 userByte1;
    uint8 userByte2;
    uint8 userData[8];          /* User data bytes */
    uint8 userByteCount;        /* Number of valid user bytes */
} StbM_UserDataType;

/*==================================================================================================
*                                    SYNCHRONIZED TIME BASE TYPE
==================================================================================================*/
typedef struct {
    StbM_TimeStampType timeStamp;
    StbM_UserDataType userData;
} StbM_SynchronizedTimeBaseType;

/*==================================================================================================
*                                    VIRTUAL LOCAL TIME TYPE
==================================================================================================*/
typedef uint64 StbM_VirtualLocalTimeType;

/*==================================================================================================
*                                    RATE DEVIATION TYPE
==================================================================================================*/
typedef sint32 StbM_RateDeviationType;

/*==================================================================================================
*                                    TIME BASE STATUS TYPE
==================================================================================================*/
typedef struct {
    uint8 syncStatus;
    uint8 timeBaseStatus;
} StbM_TimeBaseStatusType;

/*==================================================================================================
*                                    TIME DIFFERENCE TYPE
==================================================================================================*/
typedef struct {
    sint32 diff;
} StbM_TimeDiffType;

/*==================================================================================================
*                                    TIME BASE CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8 timeBaseId;
    uint8 timeBaseType;
    StbM_MasterConfigType masterConfig;
    boolean enableTimeRecording;
    boolean enableRateCorrection;
    uint32 syncTimeout;
    uint32 updateFreq;
    uint32 allowedRateDeviation;
    uint32 ethControllerId;
} StbM_TimeBaseConfigType;

/*==================================================================================================
*                                    STBM CONFIG TYPE
==================================================================================================*/
typedef struct {
    const StbM_TimeBaseConfigType* timeBaseConfigs;
    uint8 numTimeBases;
    boolean devErrorDetect;
    boolean versionInfoApi;
    boolean enableGptp;
} StbM_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define STBM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const StbM_ConfigType StbM_Config;

#define STBM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define STBM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the StbM module
 * @param ConfigPtr Pointer to configuration structure
 */
void StbM_Init(const StbM_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the StbM module
 */
void StbM_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
#if (STBM_VERSION_INFO_API == STD_ON)
void StbM_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Gets current synchronized time
 * @param timeBaseId Time base ID
 * @param timeStampPtr Pointer to store time stamp
 * @param userDataPtr Pointer to store user data (NULL if not needed)
 * @return Result of operation
 */
Std_ReturnType StbM_GetCurrentTime(uint8 timeBaseId, 
                                    StbM_TimeStampType* timeStampPtr,
                                    StbM_UserDataType* userDataPtr);

/**
 * @brief Gets current virtual local time
 * @param timeBaseId Time base ID
 * @param virtualLocalTimePtr Pointer to store virtual local time
 * @return Result of operation
 */
Std_ReturnType StbM_GetCurrentVirtualTime(uint8 timeBaseId,
                                           StbM_VirtualLocalTimeType* virtualLocalTimePtr);

/**
 * @brief Sets global time
 * @param timeBaseId Time base ID
 * @param timeStampPtr Pointer to new time stamp
 * @param userDataPtr Pointer to user data (NULL if not needed)
 * @return Result of operation
 */
Std_ReturnType StbM_SetGlobalTime(uint8 timeBaseId,
                                   const StbM_TimeStampType* timeStampPtr,
                                   const StbM_UserDataType* userDataPtr);

/**
 * @brief Sets global time from bus (called by time sync protocol)
 * @param timeBaseId Time base ID
 * @param timeStampPtr Pointer to received time stamp
 * @param virtualLocalTime Virtual local time when received
 * @param userDataPtr Pointer to received user data
 * @return Result of operation
 */
Std_ReturnType StbM_BusSetGlobalTime(uint8 timeBaseId,
                                      const StbM_TimeStampType* timeStampPtr,
                                      const StbM_VirtualLocalTimeType* virtualLocalTimePtr,
                                      const StbM_UserDataType* userDataPtr);

/**
 * @brief Gets time base status
 * @param timeBaseId Time base ID
 * @param syncStatusPtr Pointer to store sync status
 * @param timeBaseStatusPtr Pointer to store time base status
 * @return Result of operation
 */
Std_ReturnType StbM_GetTimeBaseStatus(uint8 timeBaseId,
                                       uint8* syncStatusPtr,
                                       uint8* timeBaseStatusPtr);

/**
 * @brief Gets master configuration
 * @param timeBaseId Time base ID
 * @param masterConfigPtr Pointer to store master configuration
 * @return Result of operation
 */
Std_ReturnType StbM_GetMasterConfig(uint8 timeBaseId,
                                     StbM_MasterConfigType* masterConfigPtr);

/**
 * @brief Sets rate correction
 * @param timeBaseId Time base ID
 * @param rateDeviation Rate correction value
 * @return Result of operation
 */
Std_ReturnType StbM_SetRateCorrection(uint8 timeBaseId,
                                       StbM_RateDeviationType rateDeviation);

/**
 * @brief Gets time base update counter
 * @param timeBaseId Time base ID
 * @param updateCounterPtr Pointer to store update counter
 * @return Result of operation
 */
Std_ReturnType StbM_GetTimeBaseUpdateCounter(uint8 timeBaseId,
                                              uint32* updateCounterPtr);

/**
 * @brief Gets current time difference
 * @param timeBaseId Time base ID
 * @param timeDiffPtr Pointer to store time difference
 * @return Result of operation
 */
Std_ReturnType StbM_GetCurrentTimeDiff(uint8 timeBaseId,
                                        StbM_TimeDiffType* timeDiffPtr);

/**
 * @brief Sets user data
 * @param timeBaseId Time base ID
 * @param userDataPtr Pointer to user data
 * @return Result of operation
 */
Std_ReturnType StbM_SetUserData(uint8 timeBaseId,
                                 const StbM_UserDataType* userDataPtr);

/**
 * @brief Main function for periodic processing
 */
void StbM_MainFunction(void);

/**
 * @brief Called when timestamp is received from Ethernet (gPTP)
 * @param timeBaseId Time base ID
 * @param ethTimeStampPtr Pointer to Ethernet timestamp
 */
void StbM_TimeStampChanged(uint8 timeBaseId, const Eth_TimeStampType* ethTimeStampPtr);

#define STBM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* STBM_H */
