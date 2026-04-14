/**
 * @file EthIf.h
 * @brief Ethernet Interface module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Ethernet Interface (ETHIF)
 * Layer: ECU Abstraction Layer (ECUAL)
 */

#ifndef ETHIF_H
#define ETHIF_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "EthIf_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define ETHIF_VENDOR_ID                 (0x01U) /* YuleTech Vendor ID */
#define ETHIF_MODULE_ID                 (0x41U) /* ETHIF Module ID */
#define ETHIF_AR_RELEASE_MAJOR_VERSION  (0x04U)
#define ETHIF_AR_RELEASE_MINOR_VERSION  (0x04U)
#define ETHIF_AR_RELEASE_REVISION_VERSION (0x00U)
#define ETHIF_SW_MAJOR_VERSION          (0x01U)
#define ETHIF_SW_MINOR_VERSION          (0x00U)
#define ETHIF_SW_PATCH_VERSION          (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define ETHIF_SID_INIT                  (0x01U)
#define ETHIF_SID_CONTROLLERINIT        (0x02U)
#define ETHIF_SID_SETCONTROLLERMODE     (0x03U)
#define ETHIF_SID_GETCONTROLLERMODE     (0x04U)
#define ETHIF_SID_GETPHYSADDR           (0x05U)
#define ETHIF_SID_SETPHYSADDR           (0x06U)
#define ETHIF_SID_GETARLTABLE           (0x07U)
#define ETHIF_SID_GETCTRLIDX            (0x08U)
#define ETHIF_SID_GETVLANIDX            (0x09U)
#define ETHIF_SID_GETANDRESETMEASUREMENTDATA (0x0AU)
#define ETHIF_SID_GETVERSIONINFO        (0x0BU)
#define ETHIF_SID_TRANSMIT              (0x0CU)
#define ETHIF_SID_GETCURRENTTIME        (0x0DU)
#define ETHIF_SID_ENABLEEGRESSTIMESTAMP (0x0EU)
#define ETHIF_SID_GETEGRESSTIMESTAMP    (0x0FU)
#define ETHIF_SID_GETINGRESSTIMESTAMP   (0x10U)
#define ETHIF_SID_MAINFUNCTION          (0x11U)
#define ETHIF_SID_SWITCHPORTGROUPMODE   (0x12U)
#define ETHIF_SID_GETTRANSCEIVERWAKEUPMODE (0x13U)
#define ETHIF_SID_SETTRANSCEIVERWAKEUPMODE (0x14U)
#define ETHIF_SID_CHECKWAKEUP           (0x15U)
#define ETHIF_SID_STORECONFIGURATION    (0x16U)
#define ETHIF_SID_RESETCONFIGURATION    (0x17U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define ETHIF_E_INV_CTRL_IDX            (0x01U)
#define ETHIF_E_INV_TRCV_IDX            (0x02U)
#define ETHIF_E_INV_SWITCH_IDX          (0x03U)
#define ETHIF_E_INV_SWITCH_GRP_IDX      (0x04U)
#define ETHIF_E_INV_PARAM_POINTER       (0x05U)
#define ETHIF_E_INV_MODE                (0x06U)
#define ETHIF_E_INV_CONFIG              (0x07U)
#define ETHIF_E_INV_VLAN_IDX            (0x08U)
#define ETHIF_E_INV_MTU                 (0x09U)
#define ETHIF_E_INV_TIMER               (0x0AU)
#define ETHIF_E_INV_TIMESTAMP_TYPE      (0x0BU)
#define ETHIF_E_INV_WAKEUP_MODE         (0x0CU)
#define ETHIF_E_INV_PHY_ADDRESS         (0x0DU)
#define ETHIF_E_INV_FRAME_TYPE          (0x0EU)
#define ETHIF_E_INV_FRAME_ID            (0x0FU)
#define ETHIF_E_INV_CHANNEL             (0x10U)
#define ETHIF_E_INV_MAC_LAYER_TYPE      (0x11U)
#define ETHIF_E_INV_PRIORITY            (0x12U)
#define ETHIF_E_INV_ETHIF_SWT_PORT      (0x13U)
#define ETHIF_E_INV_ETHIF_SWT_PORT_MODE (0x14U)
#define ETHIF_E_INV_ETHIF_SWT_PORT_GRP_IDX (0x15U)
#define ETHIF_E_INV_ETHIF_SWT_PORT_GRP_MODE (0x16U)
#define ETHIF_E_INV_ETHIF_SWT_PORT_GRP_ACTIVE (0x17U)
#define ETHIF_E_INV_ETHIF_SWT_PORT_GRP_PASSIVE (0x18U)
#define ETHIF_E_UNINIT                  (0x20U)
#define ETHIF_E_ALREADY_INITIALIZED     (0x21U)

/*==================================================================================================
*                                    ETHIF MAC ADDRESS TYPE
==================================================================================================*/
typedef uint8 EthIf_MacAddrType[6];

/*==================================================================================================
*                                    ETHIF FRAME TYPE
==================================================================================================*/
typedef uint16 EthIf_FrameType;

/*==================================================================================================
*                                    ETHIF CTRL MODE TYPE
==================================================================================================*/
typedef enum {
    ETHIF_MODE_DOWN = 0,
    ETHIF_MODE_ACTIVE
} EthIf_ControllerModeType;

/*==================================================================================================
*                                    ETHIF SPEED TYPE
==================================================================================================*/
typedef enum {
    ETHIF_SPEED_10MBPS = 0,
    ETHIF_SPEED_100MBPS,
    ETHIF_SPEED_1GBPS,
    ETHIF_SPEED_2_5GBPS,
    ETHIF_SPEED_10GBPS
} EthIf_SpeedType;

/*==================================================================================================
*                                    ETHIF DUPLEX TYPE
==================================================================================================*/
typedef enum {
    ETHIF_DUPLEX_HALF = 0,
    ETHIF_DUPLEX_FULL
} EthIf_DuplexType;

/*==================================================================================================
*                                    ETHIF LINK STATE TYPE
==================================================================================================*/
typedef enum {
    ETHIF_LINK_STATE_DOWN = 0,
    ETHIF_LINK_STATE_ACTIVE
} EthIf_LinkStateType;

/*==================================================================================================
*                                    ETHIF TIMESTAMP TYPE
==================================================================================================*/
typedef struct {
    uint32 seconds;
    uint32 nanoseconds;
} EthIf_TimestampType;

/*==================================================================================================
*                                    ETHIF QUALITY TYPE
==================================================================================================*/
typedef enum {
    ETHIF_TIMESTAMP_VALID = 0,
    ETHIF_TIMESTAMP_INVALID,
    ETHIF_TIMESTAMP_NOT_SUPPORTED
} EthIf_TimestampQualityType;

/*==================================================================================================
*                                    ETHIF TRANSCEIVER WAKEUP MODE
==================================================================================================*/
typedef enum {
    ETHIF_TRCV_WU_ENABLE = 0,
    ETHIF_TRCV_WU_DISABLE,
    ETHIF_TRCV_WU_CLEAR
} EthIf_TransceiverWakeupModeType;

/*==================================================================================================
*                                    ETHIF CTRL CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint8 CtrlIdx;
    uint8 EthCtrlIdx;
    uint8 EthTrcvIdx;
    EthIf_MacAddrType PhysAddr;
    uint16 Mtu;
    boolean CtrlEnableWakeup;
    boolean CtrlIdxApiActive;
} EthIf_ControllerConfigType;

/*==================================================================================================
*                                    ETHIF FRAME OWNER CONFIG TYPE
==================================================================================================*/
typedef struct {
    EthIf_FrameType FrameType;
    uint8 OwnerIdx;
    boolean HeaderByteOffsetApi;
} EthIf_FrameOwnerConfigType;

/*==================================================================================================
*                                    ETHIF VLAN CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint16 VlanId;
    uint8 CtrlIdx;
    uint8 Priority;
} EthIf_VlanConfigType;

/*==================================================================================================
*                                    ETHIF CONFIG TYPE
==================================================================================================*/
typedef struct {
    const EthIf_ControllerConfigType* Controllers;
    uint8 NumControllers;
    const EthIf_FrameOwnerConfigType* FrameOwners;
    uint8 NumFrameOwners;
    const EthIf_VlanConfigType* Vlans;
    uint8 NumVlans;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean EthIfEnableWakeupModeApi;
    boolean EthIfGetWakeupModeApi;
    boolean EthIfGetCtrlIdxApi;
    boolean EthIfGetVlanIdxApi;
    boolean EthIfGetAndResetMeasurementDataApi;
    boolean EthIfGetCurrentTimeApi;
    boolean EthIfEnableEgressTimestampApi;
    boolean EthIfGetEgressTimestampApi;
    boolean EthIfGetIngressTimestampApi;
} EthIf_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define ETHIF_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const EthIf_ConfigType EthIf_Config;

#define ETHIF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define ETHIF_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Ethernet Interface
 * @param CfgPtr Pointer to configuration structure
 */
void EthIf_Init(const EthIf_ConfigType* CfgPtr);

/**
 * @brief Initializes a controller
 * @param CtrlIdx Controller index
 * @param CfgIdx Configuration index
 */
void EthIf_ControllerInit(uint8 CtrlIdx, uint8 CfgIdx);

/**
 * @brief Sets controller mode
 * @param CtrlIdx Controller index
 * @param CtrlMode Mode to set
 * @return Result of operation
 */
Std_ReturnType EthIf_SetControllerMode(uint8 CtrlIdx, EthIf_ControllerModeType CtrlMode);

/**
 * @brief Gets controller mode
 * @param CtrlIdx Controller index
 * @param CtrlModePtr Pointer to store mode
 * @return Result of operation
 */
Std_ReturnType EthIf_GetControllerMode(uint8 CtrlIdx, EthIf_ControllerModeType* CtrlModePtr);

/**
 * @brief Gets physical address
 * @param CtrlIdx Controller index
 * @param PhysAddrPtr Pointer to store address
 */
void EthIf_GetPhysAddr(uint8 CtrlIdx, uint8* PhysAddrPtr);

/**
 * @brief Sets physical address
 * @param CtrlIdx Controller index
 * @param PhysAddr Pointer to address
 */
void EthIf_SetPhysAddr(uint8 CtrlIdx, const uint8* PhysAddr);

/**
 * @brief Transmits an Ethernet frame
 * @param CtrlIdx Controller index
 * @param FrameType Frame type
 * @param TxBufferPtrEq Pointer to Tx buffer
 * @param LenByte Frame length
 * @return Result of operation
 */
Std_ReturnType EthIf_Transmit(uint8 CtrlIdx,
                               EthIf_FrameType FrameType,
                               const uint8* TxBufferPtrEq,
                               uint16 LenByte);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void EthIf_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Gets current time
 * @param CtrlIdx Controller index
 * @param timeStampPtr Pointer to store timestamp
 * @return Result of operation
 */
Std_ReturnType EthIf_GetCurrentTime(uint8 CtrlIdx, EthIf_TimestampType* timeStampPtr);

/**
 * @brief Enables egress timestamp
 * @param CtrlIdx Controller index
 * @param BufIdx Buffer index
 */
void EthIf_EnableEgressTimeStamp(uint8 CtrlIdx, uint8 BufIdx);

/**
 * @brief Gets egress timestamp
 * @param CtrlIdx Controller index
 * @param BufIdx Buffer index
 * @param timeStampPtr Pointer to store timestamp
 * @param timeStampQualityPtr Pointer to store quality
 */
void EthIf_GetEgressTimeStamp(uint8 CtrlIdx,
                               uint8 BufIdx,
                               EthIf_TimestampType* timeStampPtr,
                               EthIf_TimestampQualityType* timeStampQualityPtr);

/**
 * @brief Gets ingress timestamp
 * @param CtrlIdx Controller index
 * @param DataPtr Pointer to data
 * @param timeStampPtr Pointer to store timestamp
 * @param timeStampQualityPtr Pointer to store quality
 */
void EthIf_GetIngressTimeStamp(uint8 CtrlIdx,
                                const uint8* DataPtr,
                                EthIf_TimestampType* timeStampPtr,
                                EthIf_TimestampQualityType* timeStampQualityPtr);

/**
 * @brief Main function for periodic processing
 */
void EthIf_MainFunction(void);

/**
 * @brief Rx indication callback
 * @param CtrlIdx Controller index
 * @param FrameType Frame type
 * @param IsBroadcast Broadcast flag
 * @param PhysAddrPtr Physical address pointer
 * @param DataPtr Data pointer
 * @param LenByte Data length
 */
void EthIf_RxIndication(uint8 CtrlIdx,
                         EthIf_FrameType FrameType,
                         boolean IsBroadcast,
                         const uint8* PhysAddrPtr,
                         const uint8* DataPtr,
                         uint16 LenByte);

/**
 * @brief Tx confirmation callback
 * @param CtrlIdx Controller index
 * @param BufIdx Buffer index
 */
void EthIf_TxConfirmation(uint8 CtrlIdx, uint8 BufIdx);

#define ETHIF_STOP_SEC_CODE
#include "MemMap.h"

#endif /* ETHIF_H */
