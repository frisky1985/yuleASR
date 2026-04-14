/**
 * @file LinIf.h
 * @brief LIN Interface module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: LIN Interface (LINIF)
 * Layer: ECU Abstraction Layer (ECUAL)
 */

#ifndef LINIF_H
#define LINIF_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "LinIf_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define LINIF_VENDOR_ID                 (0x01U) /* YuleTech Vendor ID */
#define LINIF_MODULE_ID                 (0x40U) /* LINIF Module ID */
#define LINIF_AR_RELEASE_MAJOR_VERSION  (0x04U)
#define LINIF_AR_RELEASE_MINOR_VERSION  (0x04U)
#define LINIF_AR_RELEASE_REVISION_VERSION (0x00U)
#define LINIF_SW_MAJOR_VERSION          (0x01U)
#define LINIF_SW_MINOR_VERSION          (0x00U)
#define LINIF_SW_PATCH_VERSION          (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define LINIF_SID_INIT                  (0x01U)
#define LINIF_SID_INITCHNL              (0x02U)
#define LINIF_SID_TRANSMIT              (0x03U)
#define LINIF_SID_SCHEDULEREQUEST       (0x04U)
#define LINIF_SID_GOTOSLEEP             (0x05U)
#define LINIF_SID_WAKEUP                (0x06U)
#define LINIF_SID_SETTRCVMODE           (0x07U)
#define LINIF_SID_GETTRCVMODE           (0x08U)
#define LINIF_SID_GETVERSIONINFO        (0x09U)
#define LINIF_SID_CHECKWAKEUP           (0x0AU)
#define LINIF_SID_DISABLEWAKEUP         (0x0BU)
#define LINIF_SID_ENABLEWAKEUP          (0x0CU)
#define LINIF_SID_CANCELTRANSMIT        (0x0DU)
#define LINIF_SID_CHANGEPARAM           (0x0EU)
#define LINIF_SID_CHECKWAKEUPINTERNAL   (0x0FU)
#define LINIF_SID_MAINFUNCTION          (0x10U)
#define LINIF_SID_WAKEUPCONFIRMATION    (0x11U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define LINIF_E_UNINIT                  (0x01U)
#define LINIF_E_ALREADY_INITIALIZED     (0x02U)
#define LINIF_E_NONEXISTENT_CHANNEL     (0x03U)
#define LINIF_E_PARAMETER               (0x04U)
#define LINIF_E_PARAMETER_POINTER       (0x05U)
#define LINIF_E_SCHEDULE_REQUEST_ERROR  (0x06U)
#define LINIF_E_RESPONSE                (0x07U)
#define LINIF_E_INVALID_SCHEDULE_INDEX  (0x08U)
#define LINIF_E_SCHEDULE_OVERFLOW       (0x09U)
#define LINIF_E_SCHEDULE_REQUEST        (0x0AU)
#define LINIF_E_TRCV_INV_MODE           (0x0BU)
#define LINIF_E_TRCV_NOT_SUPPORTED      (0x0CU)
#define LINIF_E_WAKEUP_SOURCE           (0x0DU)
#define LINIF_E_CHANNEL_NOT_ACTIVE      (0x0EU)
#define LINIF_E_CHANNEL_NOT_SLEEP       (0x0FU)
#define LINIF_E_CHANNEL_NOT_OPERATIONAL (0x10U)
#define LINIF_E_SCHEDULE_NOT_REQUESTED  (0x11U)
#define LINIF_E_SCHEDULE_REQUESTED      (0x12U)
#define LINIF_E_CONFIG                  (0x13U)

/*==================================================================================================
*                                    LINIF STATUS TYPE
==================================================================================================*/
typedef enum {
    LINIF_UNINIT = 0,
    LINIF_INIT
} LinIf_StatusType;

/*==================================================================================================
*                                    LINIF CHNL STATUS TYPE
==================================================================================================*/
typedef enum {
    LINIF_CHNL_SLEEP = 0,
    LINIF_CHNL_OPERATIONAL
} LinIf_ChannelStatusType;

/*==================================================================================================
*                                    LINIF TRCV MODE TYPE
==================================================================================================*/
typedef enum {
    LINIF_TRCV_MODE_NORMAL = 0,
    LINIF_TRCV_MODE_STANDBY,
    LINIF_TRCV_MODE_SLEEP
} LinIf_TransceiverModeType;

/*==================================================================================================
*                                    LINIF SCHEDULE TYPE
==================================================================================================*/
typedef enum {
    LINIF_SCHEDULE_NULL = 0,
    LINIF_SCHEDULE_DIAGNOSTIC,
    LINIF_SCHEDULE_NORMAL,
    LINIF_SCHEDULE_MRF,
    LINIF_SCHEDULE_SRF
} LinIf_ScheduleType;

/*==================================================================================================
*                                    LINIF CHNL CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8 LinIfChnlIdx;
    uint8 LinChannel;
    uint8 LinTrcv;
    uint8 ScheduleRequestQueueLength;
    boolean WakeupSupport;
    boolean WakeupNotification;
} LinIf_ChannelConfigType;

/*==================================================================================================
*                                    LINIF PDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType PduId;
    uint8 LinIfChnlIdx;
    uint8 LinPduId;
    uint8 PduDirection;
    uint8 PduType;
    uint8 Cs;
    uint8 Dl;
    uint8 Pid;
} LinIf_PduConfigType;

/*==================================================================================================
*                                    LINIF SCHEDULE ENTRY CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8 EntryIdx;
    uint8 Delay;
    uint8 EntryType;
    PduIdType PduRef;
} LinIf_ScheduleEntryConfigType;

/*==================================================================================================
*                                    LINIF SCHEDULE CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8 ScheduleIdx;
    uint8 LinIfChnlIdx;
    uint8 ScheduleType;
    uint8 NumEntries;
    boolean RunOnce;
    const LinIf_ScheduleEntryConfigType* Entries;
} LinIf_ScheduleConfigType;

/*==================================================================================================
*                                    LINIF CONFIG TYPE
==================================================================================================*/
typedef struct {
    const LinIf_ChannelConfigType* Channels;
    uint8 NumChannels;
    const LinIf_PduConfigType* Pdus;
    uint8 NumPdus;
    const LinIf_ScheduleConfigType* Schedules;
    uint8 NumSchedules;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean LinIfTrcvDriverSupported;
    boolean LinIfWakeupSupport;
    boolean LinIfCancelTransmitSupported;
} LinIf_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define LINIF_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const LinIf_ConfigType LinIf_Config;

#define LINIF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define LINIF_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the LIN Interface
 * @param Config Pointer to configuration structure
 */
void LinIf_Init(const LinIf_ConfigType* Config);

/**
 * @brief Initializes a channel
 * @param Channel Channel index
 * @return Result of operation
 */
Std_ReturnType LinIf_InitChannel(uint8 Channel);

/**
 * @brief Transmits a PDU
 * @param LinTxPduId PDU ID
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType LinIf_Transmit(PduIdType LinTxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Requests schedule change
 * @param Channel Channel index
 * @param Schedule Schedule to request
 * @return Result of operation
 */
Std_ReturnType LinIf_ScheduleRequest(uint8 Channel, uint8 Schedule);

/**
 * @brief Puts channel to sleep
 * @param Channel Channel index
 * @return Result of operation
 */
Std_ReturnType LinIf_GotoSleep(uint8 Channel);

/**
 * @brief Wakes up channel
 * @param Channel Channel index
 * @return Result of operation
 */
Std_ReturnType LinIf_WakeUp(uint8 Channel);

/**
 * @brief Sets transceiver mode
 * @param Channel Channel index
 * @param Mode Mode to set
 * @return Result of operation
 */
Std_ReturnType LinIf_SetTransceiverMode(uint8 Channel, LinIf_TransceiverModeType Mode);

/**
 * @brief Gets transceiver mode
 * @param Channel Channel index
 * @param Mode Pointer to store mode
 * @return Result of operation
 */
Std_ReturnType LinIf_GetTransceiverMode(uint8 Channel, LinIf_TransceiverModeType* Mode);

/**
 * @brief Checks for wakeup
 * @param WakeupSource Wakeup source
 * @return Result of operation
 */
Std_ReturnType LinIf_CheckWakeup(EcuM_WakeupSourceType WakeupSource);

/**
 * @brief Disables wakeup
 * @param Channel Channel index
 * @return Result of operation
 */
Std_ReturnType LinIf_DisableWakeup(uint8 Channel);

/**
 * @brief Enables wakeup
 * @param Channel Channel index
 * @return Result of operation
 */
Std_ReturnType LinIf_EnableWakeup(uint8 Channel);

/**
 * @brief Cancels transmit
 * @param LinTxPduId PDU ID
 * @return Result of operation
 */
Std_ReturnType LinIf_CancelTransmit(PduIdType LinTxPduId);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void LinIf_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Wakeup confirmation callback
 * @param Channel Channel index
 */
void LinIf_WakeUpConfirmation(uint8 Channel);

/**
 * @brief Main function for periodic processing
 */
void LinIf_MainFunction(void);

#define LINIF_STOP_SEC_CODE
#include "MemMap.h"

#endif /* LINIF_H */
