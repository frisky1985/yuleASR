/**
 * @file Eth.h
 * @brief Ethernet Driver
 * @version 1.0.0
 * 
 * Ethernet MAC driver for AUTOSAR MCAL layer.
 * Based on AUTOSAR Classic Platform 4.4.0, Eth driver specification.
 * Supports 10/100/1000 Mbps operation with ASIL-D safety level compatibility.
 * 
 * @copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef ETH_H
#define ETH_H

#include "Std_Types.h"
#include "ComStack_Types.h"
#include "Eth_GeneralTypes.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                              AUTOSAR VERSION
==================================================================================================*/
#define ETH_AR_RELEASE_MAJOR_VERSION       4
#define ETH_AR_RELEASE_MINOR_VERSION       4
#define ETH_AR_RELEASE_REVISION_VERSION    0

/*==================================================================================================
*                              MODULE VERSION
==================================================================================================*/
#define ETH_SW_MAJOR_VERSION               1
#define ETH_SW_MINOR_VERSION               0
#define ETH_SW_PATCH_VERSION               0

/*==================================================================================================
*                              MODULE AND INSTANCE ID
==================================================================================================*/
#define ETH_MODULE_ID                      0x53
#define ETH_INSTANCE_ID                    0x00

/*==================================================================================================
*                              SERVICE IDs (APIs)
==================================================================================================*/
#define ETH_INIT_SID                       0x01
#define ETH_DEINIT_SID                     0x02
#define ETH_GETVERSIONINFO_SID             0x03
#define ETH_SETCONTROLLERMODE_SID          0x04
#define ETH_GETCONTROLLERMODE_SID          0x05
#define ETH_GETCONTROLLERIDX_SID           0x06
#define ETH_WRITEMII_SID                   0x07
#define ETH_READMII_SID                    0x08
#define ETH_GETPHYSADDR_SID                0x09
#define ETH_SETPHYSADDR_SID                0x0A
#define ETH_UPDATEADDRFILTER_SID           0x0B
#define ETH_SETFORWARDINGMODE_SID          0x0C
#define ETH_GETCTRLIDX_SID                 0x0D
#define ETH_PROVIDETXBUFFER_SID            0x0E
#define ETH_TRANSMIT_SID                   0x0F
#define ETH_RECEIVE_SID                    0x10
#define ETH_TXCONFIRMATION_SID             0x11
#define ETH_GETVERSIONINFO_APIID           0x12
#define ETH_ENABLEIRQ_SID                  0x13
#define ETH_DISABLEIRQ_SID                 0x14
#define ETH_INITBUFFERS_SID                0x15

/*==================================================================================================
*                              ERROR CODES
==================================================================================================*/
#define ETH_E_NOT_INITIALIZED              0x01
#define ETH_E_INV_CTRL_INDEX               0x02
#define ETH_E_INV_POINTER                  0x03
#define ETH_E_INV_PARAM                    0x04
#define ETH_E_INV_CONFIG                   0x05
#define ETH_E_INV_MODE                     0x06
#define ETH_E_INV_FRAME_LENGTH             0x07
#define ETH_E_INV_MAC_ADDR                 0x08
#define ETH_E_INV_BUF_INDEX                0x09
#define ETH_E_TIMEOUT                      0x0A
#define ETH_E_BUSY                         0x0B

/*==================================================================================================
*                              CONTROLLER STATE
==================================================================================================*/
typedef uint8 Eth_StateType;
#define ETH_STATE_UNINIT                   0x00U
#define ETH_STATE_INIT                     0x01

/*==================================================================================================
*                              CONTROLLER MODE
==================================================================================================*/
typedef uint8 Eth_ModeType;
#define ETH_MODE_DOWN                      0x00U
#define ETH_MODE_ACTIVE                    0x01U

/*==================================================================================================
*                              FRAME TYPE IDENTIFIER
==================================================================================================*/
typedef uint16 Eth_FrameIdType;

/*==================================================================================================
*                              BUFFER INDEX TYPE
==================================================================================================*/
typedef uint8 Eth_BufIdxType;
#define ETH_INVALID_BUF_INDEX              0xFFu

/*==================================================================================================
*                              DATA LENGTH TYPE
==================================================================================================*/
typedef uint16 Eth_DataLenType;
#define ETH_MIN_FRAME_SIZE                 14u   /* MAC header size */
#define ETH_MAX_FRAME_SIZE                 1522u /* Max Ethernet frame (including FCS) */
#define ETH_DEFAULT_FRAME_SIZE             1536u /* Default buffer size */

/*==================================================================================================
*                              MAC ADDRESS TYPE
==================================================================================================*/
typedef uint8 Eth_MacAddrType[6];

/*==================================================================================================
*                              ETHERNET RATE TYPE
==================================================================================================*/
typedef uint8 Eth_RateType;
#define ETH_RATE_10MBPS                    0x00
#define ETH_RATE_100MBPS                   0x01
#define ETH_RATE_1000MBPS                  0x02

/*==================================================================================================
*                              CONTROLLER INDEX TYPE
==================================================================================================*/
typedef uint8 Eth_ControllerType;
#define ETH_INVALID_CONTROLLER_INDEX       0xFFu

/*==================================================================================================
*                              MII DATA TYPE
==================================================================================================*/
typedef uint16 Eth_DataType;

/*==================================================================================================
*                              PHY ADDRESS TYPE
==================================================================================================*/
typedef uint8 Eth_PhyAddrType;
#define ETH_PHY_ADDR_MIN                   0x00u
#define ETH_PHY_ADDR_MAX                   0x1Fu

/*==================================================================================================
*                              MII REG ADDRESS TYPE
==================================================================================================*/
typedef uint8 Eth_RegAddrType;
#define ETH_MII_REG_BMCR                   0x00u  /* Basic Mode Control Register */
#define ETH_MII_REG_BMSR                   0x01u  /* Basic Mode Status Register */
#define ETH_MII_REG_PHYIDR1                0x02u  /* PHY Identifier 1 */
#define ETH_MII_REG_PHYIDR2                0x03u  /* PHY Identifier 2 */
#define ETH_MII_REG_ANAR                   0x04u  /* Auto-Negotiation Advertisement Register */
#define ETH_MII_REG_ANLPAR                 0x05u  /* Auto-Negotiation Link Partner Ability Register */

/*==================================================================================================
*                              FILTER ACTION TYPE
==================================================================================================*/
typedef uint8 Eth_FilterActionType;
#define ETH_FILTER_ACTION_ADD              0x00u
#define ETH_FILTER_ACTION_REMOVE           0x01u

/*==================================================================================================
*                              TIME STAMP TYPE
==================================================================================================*/
typedef struct {
    uint32 seconds;
    uint32 nanoseconds;
} Eth_TimeStampType;

/*==================================================================================================
*                              FRAME STRUCTURE
==================================================================================================*/
typedef struct {
    Eth_MacAddrType DestMacAddr;
    Eth_MacAddrType SrcMacAddr;
    Eth_FrameIdType FrameType;
    uint8* Payload;
    Eth_DataLenType PayloadLen;
} Eth_FrameStructType;

/*==================================================================================================
*                              CONTROLLER CONFIGURATION TYPE
==================================================================================================*/
typedef struct {
    uint32 CtrlIdx;
    Eth_MacAddrType MacAddr;
    Eth_RateType Speed;
    boolean FullDuplex;
    boolean RxChecksumOffload;
    boolean TxChecksumOffload;
    Eth_PhyAddrType PhyAddress;
    uint32 TxBufCount;
    uint32 RxBufCount;
    uint32 BufSize;
} Eth_ControllerConfigType;

/*==================================================================================================
*                              MODULE CONFIGURATION TYPE
==================================================================================================*/
typedef struct {
    const Eth_ControllerConfigType* CtrlConfig;
    uint8 NumControllers;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} Eth_ConfigType;

/*==================================================================================================
*                              FUNCTION PROTOTYPES
==================================================================================================*/

/* Initialization and De-initialization */
extern void Eth_Init(const Eth_ConfigType* CfgPtr);
extern void Eth_DeInit(void);
extern void Eth_ControllerInit(Eth_ControllerType CtrlIdx, const Eth_ControllerConfigType* CfgPtr);

/* Version Information */
#if (ETH_VERSION_INFO_API == STD_ON)
extern void Eth_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);
#endif

/* Controller Mode Management */
extern Std_ReturnType Eth_SetControllerMode(Eth_ControllerType CtrlIdx, Eth_ModeType CtrlMode);
extern Std_ReturnType Eth_GetControllerMode(Eth_ControllerType CtrlIdx, Eth_ModeType* CtrlModePtr);
extern uint8 Eth_GetControllerIdx(const uint8* CtrlName);

/* MAC Address Management */
extern void Eth_GetPhysAddr(Eth_ControllerType CtrlIdx, uint8* PhysAddrPtr);
extern void Eth_SetPhysAddr(Eth_ControllerType CtrlIdx, const uint8* PhysAddrPtr);
extern Std_ReturnType Eth_UpdatePhysAddrFilter(Eth_ControllerType CtrlIdx, const uint8* PhysAddrPtr, Eth_FilterActionType Action);

/* PHY MII Interface */
extern Std_ReturnType Eth_WriteMii(Eth_ControllerType CtrlIdx, Eth_PhyAddrType PhyAddr, Eth_RegAddrType RegAddr, Eth_DataType Data);
extern Std_ReturnType Eth_ReadMii(Eth_ControllerType CtrlIdx, Eth_PhyAddrType PhyAddr, Eth_RegAddrType RegAddr, Eth_DataType* DataPtr);

/* Buffer Management */
extern BufReq_ReturnType Eth_ProvideTxBuffer(Eth_ControllerType CtrlIdx, Eth_FrameIdType FrameType, uint16 Priority, Eth_BufIdxType* BufIdxPtr, uint8** BufPtr, uint16* LenBytePtr);
extern void Eth_TxConfirmation(Eth_ControllerType CtrlIdx, Eth_BufIdxType BufIdx);

/* Transmission */
extern Std_ReturnType Eth_Transmit(Eth_ControllerType CtrlIdx, Eth_BufIdxType BufIdx, Eth_FrameIdType FrameType, boolean TxConfirmation, uint16 LenByte, const uint8* PhysAddrPtr);

/* Reception */
extern Std_ReturnType Eth_Receive(Eth_ControllerType CtrlIdx, uint8* RxStatusPtr, Eth_BufIdxType* BufIdxPtr, Eth_FrameStructType** FramePtr);

/* Interrupt Control */
extern void Eth_EnableIrq(void);
extern void Eth_DisableIrq(void);

/* Buffer Initialization */
extern void Eth_InitBuffers(void);

/*==================================================================================================
*                              ISR DECLARATIONS
==================================================================================================*/
#define ETH_START_SEC_CODE
#include "MemMap.h"

extern void Eth_IsrTx(Eth_ControllerType CtrlIdx);
extern void Eth_IsrRx(Eth_ControllerType CtrlIdx);
extern void Eth_IsrError(Eth_ControllerType CtrlIdx);

#define ETH_STOP_SEC_CODE
#include "MemMap.h"

#ifdef __cplusplus
}
#endif


/* Ethernet hardware timestamp access (used by StbM) */
extern Std_ReturnType Eth_GetCurrentTime(uint8 ControllerId, Eth_TimeStampType* TimeStampPtr, Eth_RxStatusType* RxStatusPtr);

#endif /* ETH_H */
