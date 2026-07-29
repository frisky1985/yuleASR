/**
 * @file FrIf.h
 * @brief FlexRay Interface module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: FlexRay Interface (FRIF)
 * Layer: ECU Abstraction Layer (ECUAL)
 */

#ifndef FRIF_H
#define FRIF_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "FrIf_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define FRIF_VENDOR_ID                  (0x01U) /* YuleTech Vendor ID */
#define FRIF_MODULE_ID                  (0x3FU) /* FRIF Module ID */
#define FRIF_AR_RELEASE_MAJOR_VERSION   (0x04U)
#define FRIF_AR_RELEASE_MINOR_VERSION   (0x04U)
#define FRIF_AR_RELEASE_REVISION_VERSION (0x00U)
#define FRIF_SW_MAJOR_VERSION           (0x01U)
#define FRIF_SW_MINOR_VERSION           (0x00U)
#define FRIF_SW_PATCH_VERSION           (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define FRIF_SID_INIT                   (0x01U)
#define FRIF_SID_CONTROLLERINIT         (0x02U)
#define FRIF_SID_SETABSOLUTETIMER       (0x03U)
#define FRIF_SID_SETRELATIVETIMER       (0x04U)
#define FRIF_SID_CANCELABSOLUTETIMER    (0x05U)
#define FRIF_SID_CANCELRELATIVETIMER    (0x06U)
#define FRIF_SID_GETCLOCKCORRECTION     (0x07U)
#define FRIF_SID_GETSYNCFRAMELIST       (0x08U)
#define FRIF_SID_GETWAKEUPRXSTATUS      (0x09U)
#define FRIF_SID_GETVERSIONINFO         (0x0AU)
#define FRIF_SID_TRANSMIT               (0x0BU)
#define FRIF_SID_SETTRANSCEIVERMODE     (0x0CU)
#define FRIF_SID_GETTRANSCEIVERMODE     (0x0DU)
#define FRIF_SID_GETTRANSCEIVERWUREASON (0x0EU)
#define FRIF_SID_ENABLETRANSCEIVERWAKEUP (0x0FU)
#define FRIF_SID_DISABLETRANSCEIVERWAKEUP (0x10U)
#define FRIF_SID_CLEARTRANSCEIVERWAKEUP (0x11U)
#define FRIF_SID_WAKEUPCTRL             (0x12U)
#define FRIF_SID_GETPOCSTATUS           (0x13U)
#define FRIF_SID_GETGLOBALTIME          (0x14U)
#define FRIF_SID_ALLOWCOLDSTART         (0x15U)
#define FRIF_SID_HALTCOMMUNICATION      (0x16U)
#define FRIF_SID_ABORTCOMMUNICATION     (0x17U)
#define FRIF_SID_SENDWUP                (0x18U)
#define FRIF_SID_SETWAKEUPCHANNEL       (0x19U)
#define FRIF_SID_GETMACROTICKSPERCYCLE  (0x1AU)
#define FRIF_SID_GETWUPRXSTATUS         (0x1BU)
#define FRIF_SID_GETCHANNELSTATUS       (0x1CU)
#define FRIF_SID_GETNUMOFSTARTUPFRAMES  (0x1DU)
#define FRIF_SID_GETWUPCHANNEL          (0x1EU)
#define FRIF_SID_MAINFUNCTION           (0x1FU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define FRIF_E_INV_CTRL_IDX             (0x01U)
#define FRIF_E_INV_CHNL_IDX             (0x02U)
#define FRIF_E_INV_TRCV_IDX             (0x03U)
#define FRIF_E_INV_TIMER_IDX            (0x04U)
#define FRIF_E_INV_LPDU_IDX             (0x05U)
#define FRIF_E_INV_CYCLE                (0x06U)
#define FRIF_E_INV_OFFSET               (0x07U)
#define FRIF_E_INV_PDU_MODE             (0x08U)
#define FRIF_E_INV_PDU_STATUS           (0x09U)
#define FRIF_E_INV_TRCV_MODE            (0x0AU)
#define FRIF_E_INV_TRCV_WAKEUP_MODE     (0x0BU)
#define FRIF_E_INV_TRCV_WAKEUP_REASON   (0x0CU)
#define FRIF_E_INV_WAKEUP_STATUS        (0x0DU)
#define FRIF_E_INV_POC_STATUS           (0x0EU)
#define FRIF_E_INV_POINTER              (0x0FU)
#define FRIF_E_INV_LENGTH               (0x10U)
#define FRIF_E_INV_CONFIG               (0x11U)
#define FRIF_E_INV_PARAM                (0x12U)
#define FRIF_E_INV_CHNL                 (0x13U)
#define FRIF_E_INV_FRAME_ID             (0x14U)
#define FRIF_E_INV_SLOT_ID              (0x15U)
#define FRIF_E_UNINIT                   (0x20U)
#define FRIF_E_ALREADY_INITIALIZED      (0x21U)

/*==================================================================================================
*                                    FRIF CHANNEL TYPE
==================================================================================================*/
typedef enum {
    FRIF_CHANNEL_A = 0,
    FRIF_CHANNEL_B,
    FRIF_CHANNEL_AB
} FrIf_ChannelType;

/*==================================================================================================
*                                    FRIF CTRL MODE TYPE
==================================================================================================*/
typedef enum {
    FRIF_MODE_COLDSTART = 0,
    FRIF_MODE_ALL_SLOTS,
    FRIF_MODE_SINGLE_SLOT,
    FRIF_MODE_WAKEUP,
    FRIF_MODE_HALT,
    FRIF_MODE_READY,
    FRIF_MODE_STARTUP,
    FRIF_MODE_NORMAL_ACTIVE,
    FRIF_MODE_NORMAL_PASSIVE,
    FRIF_MODE_HALT_REQ,
    FRIF_MODE_POSSIBLE_FAIL,
    FRIF_MODE_STANDBY
} FrIf_ControllerModeType;

/*==================================================================================================
*                                    FRIF TRCV MODE TYPE
==================================================================================================*/
typedef enum {
    FRIF_TRCV_MODE_NORMAL = 0,
    FRIF_TRCV_MODE_STANDBY,
    FRIF_TRCV_MODE_SLEEP,
    FRIF_TRCV_MODE_RECEIVEONLY
} FrIf_TransceiverModeType;

/*==================================================================================================
*                                    FRIF TRCV WAKEUP MODE TYPE
==================================================================================================*/
typedef enum {
    FRIF_TRCV_WAKEUP_ENABLE = 0,
    FRIF_TRCV_WAKEUP_DISABLE,
    FRIF_TRCV_WAKEUP_CLEAR
} FrIf_TransceiverWakeupModeType;

/*==================================================================================================
*                                    FRIF TRCV WAKEUP REASON TYPE
==================================================================================================*/
typedef enum {
    FRIF_TRCV_WAKEUP_ERROR = 0,
    FRIF_TRCV_WAKEUP_BY_BUS,
    FRIF_TRCV_WAKEUP_BY_PIN,
    FRIF_TRCV_WAKEUP_INTERNAL,
    FRIF_TRCV_WAKEUP_NOT_SUPPORTED,
    FRIF_TRCV_WAKEUP_POWER_ON,
    FRIF_TRCV_WAKEUP_RESET,
    FRIF_TRCV_WAKEUP_BY_SYSERR
} FrIf_TransceiverWakeupReasonType;

/*==================================================================================================
*                                    FRIF POC STATUS TYPE
==================================================================================================*/
typedef struct {
    uint8 State;
    uint8 SubState;
    uint8 ColdstartNoise;
    uint8 WakeupStatus;
} FrIf_POCStatusType;

/*==================================================================================================
*                                    FRIF GLOBAL TIME TYPE
==================================================================================================*/
typedef struct {
    uint32 Macrotick;
    uint8 Cycle;
} FrIf_GlobalTimeType;

/*==================================================================================================
*                                    FRIF CTRL CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8 CtrlIdx;
    uint8 FrCtrlIdx;
    uint8 FrTrcvIdx;
    uint8 ClusterIdx;
    boolean WakeupSupport;
    boolean ColdstartSupport;
} FrIf_ControllerConfigType;

/*==================================================================================================
*                                    FRIF LPDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint16 LpduIdx;
    uint8 CtrlIdx;
    uint16 SlotId;
    uint8 Cycle;
    uint8 CycleRepetition;
    uint8 CycleOffset;
    uint8 Channel;
    uint8 PayloadLength;
    boolean DynamicSegment;
} FrIf_LpduConfigType;

/*==================================================================================================
*                                    FRIF CONFIG TYPE
==================================================================================================*/
typedef struct {
    const FrIf_ControllerConfigType* Controllers;
    uint8 NumControllers;
    const FrIf_LpduConfigType* Lpdus;
    uint16 NumLpdus;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean ClstStartupActive;
    boolean ClstWakeupActive;
    boolean FrIfGetWupRxStatusSupport;
    boolean FrIfGetSyncFrameListSupport;
    boolean FrIfGetClockCorrectionSupport;
} FrIf_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define FRIF_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const FrIf_ConfigType FrIf_Config;

#define FRIF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define FRIF_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the FlexRay Interface
 * @param ConfigPtr Pointer to configuration structure
 */
void FrIf_Init(const FrIf_ConfigType* ConfigPtr);

/**
 * @brief Initializes a controller
 * @param FrIf_CtrlIdx Controller index
 * @return Result of operation
 */
Std_ReturnType FrIf_ControllerInit(uint8 FrIf_CtrlIdx);

/**
 * @brief Sets absolute timer
 * @param FrIf_CtrlIdx Controller index
 * @param FrIf_AbsTimerIdx Timer index
 * @param FrIf_Cycle Cycle
 * @param FrIf_Offset Offset
 * @return Result of operation
 */
Std_ReturnType FrIf_SetAbsoluteTimer(uint8 FrIf_CtrlIdx,
                                      uint8 FrIf_AbsTimerIdx,
                                      uint8 FrIf_Cycle,
                                      uint16 FrIf_Offset);

/**
 * @brief Sets relative timer
 * @param FrIf_CtrlIdx Controller index
 * @param FrIf_RelTimerIdx Timer index
 * @param FrIf_Offset Offset
 * @return Result of operation
 */
Std_ReturnType FrIf_SetRelativeTimer(uint8 FrIf_CtrlIdx,
                                      uint8 FrIf_RelTimerIdx,
                                      uint16 FrIf_Offset);

/**
 * @brief Cancels absolute timer
 * @param FrIf_CtrlIdx Controller index
 * @param FrIf_AbsTimerIdx Timer index
 * @return Result of operation
 */
Std_ReturnType FrIf_CancelAbsoluteTimer(uint8 FrIf_CtrlIdx, uint8 FrIf_AbsTimerIdx);

/**
 * @brief Cancels relative timer
 * @param FrIf_CtrlIdx Controller index
 * @param FrIf_RelTimerIdx Timer index
 * @return Result of operation
 */
Std_ReturnType FrIf_CancelRelativeTimer(uint8 FrIf_CtrlIdx, uint8 FrIf_RelTimerIdx);

/**
 * @brief Transmits a PDU
 * @param FrIf_TxPduId PDU ID
 * @param FrIf_PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType FrIf_Transmit(PduIdType FrIf_TxPduId, const PduInfoType* FrIf_PduInfoPtr);

/**
 * @brief Gets POC status
 * @param FrIf_CtrlIdx Controller index
 * @param FrIf_POCStatusPtr Pointer to POC status
 * @return Result of operation
 */
Std_ReturnType FrIf_GetPOCStatus(uint8 FrIf_CtrlIdx, FrIf_POCStatusType* FrIf_POCStatusPtr);

/**
 * @brief Gets global time
 * @param FrIf_CtrlIdx Controller index
 * @param FrIf_CyclePtr Pointer to cycle
 * @param FrIf_MacrotickPtr Pointer to macrotick
 * @return Result of operation
 */
Std_ReturnType FrIf_GetGlobalTime(uint8 FrIf_CtrlIdx,
                                   uint8* FrIf_CyclePtr,
                                   uint16* FrIf_MacrotickPtr);

/**
 * @brief Allows coldstart
 * @param FrIf_CtrlIdx Controller index
 * @return Result of operation
 */
Std_ReturnType FrIf_AllowColdstart(uint8 FrIf_CtrlIdx);

/**
 * @brief Halts communication
 * @param FrIf_CtrlIdx Controller index
 * @return Result of operation
 */
Std_ReturnType FrIf_HaltCommunication(uint8 FrIf_CtrlIdx);

/**
 * @brief Aborts communication
 * @param FrIf_CtrlIdx Controller index
 * @return Result of operation
 */
Std_ReturnType FrIf_AbortCommunication(uint8 FrIf_CtrlIdx);

/**
 * @brief Sends wakeup pattern
 * @param FrIf_CtrlIdx Controller index
 * @return Result of operation
 */
Std_ReturnType FrIf_SendWUP(uint8 FrIf_CtrlIdx);

/**
 * @brief Sets wakeup channel
 * @param FrIf_CtrlIdx Controller index
 * @param FrIf_ChnlIdx Channel index
 * @return Result of operation
 */
Std_ReturnType FrIf_SetWakeupChannel(uint8 FrIf_CtrlIdx, FrIf_ChannelType FrIf_ChnlIdx);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void FrIf_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Main function for periodic processing
 */
void FrIf_MainFunction(void);

#define FRIF_STOP_SEC_CODE
#include "MemMap.h"

#endif /* FRIF_H */
