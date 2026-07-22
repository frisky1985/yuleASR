/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : Ethernet Switch (EthSwt)
* Dependencies         : Eth (MCAL), Det
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file EthSwt.h
 * @brief Ethernet Switch Driver — AUTOSAR ECUAL Layer
 * @version 1.0.0
 *
 * Provides Ethernet switch port configuration, frame forwarding,
 * VLAN filtering, and statistics for multi-port Ethernet switches.
 */

#ifndef ETHSWT_H
#define ETHSWT_H

#include "Std_Types.h"
#include "EthSwt_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define ETHSWT_VENDOR_ID                        (0x0001U)
#define ETHSWT_MODULE_ID                        (0x88U)
#define ETHSWT_INSTANCE_ID                      (0x00U)

#define ETHSWT_AR_RELEASE_MAJOR_VERSION         (0x04U)
#define ETHSWT_AR_RELEASE_MINOR_VERSION         (0x04U)
#define ETHSWT_AR_RELEASE_REVISION_VERSION      (0x00U)
#define ETHSWT_SW_MAJOR_VERSION                 (0x01U)
#define ETHSWT_SW_MINOR_VERSION                 (0x00U)
#define ETHSWT_SW_PATCH_VERSION                 (0x00U)

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define ETHSWT_SID_INIT                         (0x01U)
#define ETHSWT_SID_DEINIT                       (0x02U)
#define ETHSWT_SID_GETVERSIONINFO               (0x03U)
#define ETHSWT_SID_SETPORTENABLE                (0x10U)
#define ETHSWT_SID_SETSPEED                     (0x11U)
#define ETHSWT_SID_GETLINKSTATE                 (0x12U)
#define ETHSWT_SID_CONFIGVLAN                   (0x13U)
#define ETHSWT_SID_FORWARDFRAME                 (0x14U)
#define ETHSWT_SID_GETPORTSTATS                 (0x15U)
#define ETHSWT_SID_SETMACFILTER                 (0x16U)
#define ETHSWT_SID_MAINFUNCTION                 (0x17U)
#define ETHSWT_SID_RESET                        (0x18U)

/*==================================================================================================
 *                                    DET ERROR CODES
 *==================================================================================================*/
#define ETHSWT_E_PARAM_POINTER                  (0x01U)
#define ETHSWT_E_PARAM_CONFIG                   (0x02U)
#define ETHSWT_E_UNINIT                         (0x03U)
#define ETHSWT_E_ALREADY_INITIALIZED            (0x04U)
#define ETHSWT_E_INVALID_PORT                   (0x05U)
#define ETHSWT_E_INVALID_SPEED                  (0x06U)
#define ETHSWT_E_INVALID_VLAN                   (0x07U)
#define ETHSWT_E_INVALID_MAC                    (0x08U)
#define ETHSWT_E_PORT_DISABLED                  (0x09U)
#define ETHSWT_E_BUFFER_FULL                    (0x0AU)
#define ETHSWT_E_INIT_FAILED                    (0x0BU)
#define ETHSWT_E_NOT_SUPPORTED                  (0x0CU)

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/** Port identifier */
typedef uint8 EthSwt_PortIdType;

/** Port enable state */
typedef uint8 EthSwt_PortEnableType;
#define ETHSWT_PORT_DISABLED                    (0x00U)
#define ETHSWT_PORT_ENABLED                     (0x01U)

/** Port link state */
typedef uint8 EthSwt_LinkStateType;
#define ETHSWT_LINK_DOWN                        (0x00U)
#define ETHSWT_LINK_UP                          (0x01U)

/** Port speed */
typedef uint8 EthSwt_SpeedType;
#define ETHSWT_SPEED_AUTO                       (0x00U)
#define ETHSWT_SPEED_10MBPS                     (0x01U)
#define ETHSWT_SPEED_100MBPS                    (0x02U)
#define ETHSWT_SPEED_1000MBPS                   (0x03U)

/** Duplex mode */
typedef uint8 EthSwt_DuplexType;
#define ETHSWT_DUPLEX_HALF                      (0x00U)
#define ETHSWT_DUPLEX_FULL                      (0x01U)

/** MAC address (6 bytes) */
typedef struct {
    uint8 octet[6];
} EthSwt_MacAddrType;

/** VLAN configuration */
typedef struct {
    uint16 VlanId;              /* VLAN ID (0–4095) */
    uint8  PortMask;            /* Bitmask of member ports */
    boolean Tagged;             /* Tagged (TRUE) or Untagged (FALSE) */
} EthSwt_VlanConfigType;

/** Port statistics */
typedef struct {
    uint64 TxFrames;
    uint64 RxFrames;
    uint64 TxBytes;
    uint64 RxBytes;
    uint64 TxErrors;
    uint64 RxErrors;
    uint64 Collisions;
    uint64 DroppedFrames;
} EthSwt_PortStatsType;

/** Per-port configuration */
typedef struct {
    EthSwt_PortIdType    PortId;
    EthSwt_SpeedType     Speed;
    EthSwt_DuplexType    Duplex;
    EthSwt_PortEnableType Enable;
    EthSwt_MacAddrType   MacAddress;
    uint16               Pvid;           /* Port VLAN ID */
} EthSwt_PortConfigType;

/** Global switch configuration */
typedef struct {
    uint8                    NumPorts;
    const EthSwt_PortConfigType* PortConfigs;
    uint8                    NumVlans;
    const EthSwt_VlanConfigType* VlanConfigs;
    boolean                  DevErrorDetect;
    boolean                  VersionInfoApi;
} EthSwt_ConfigType;

/** Switch hardware type abstraction */
typedef struct {
    uint32 BaseAddr;          /* MMIO base address (0 = software emulation) */
    uint8  NumPhysicalPorts;
    uint16 Mtu;
} EthSwt_HwConfigType;

/*==================================================================================================
 *                                    FUNCTION DECLARATIONS
 *==================================================================================================*/

/** @brief Initialise the Ethernet Switch driver */
void EthSwt_Init(const EthSwt_ConfigType* ConfigPtr);

/** @brief De-initialise the Ethernet Switch driver */
void EthSwt_DeInit(void);

/** @brief Get version information */
#if (ETHSWT_VERSION_INFO_API == STD_ON)
void EthSwt_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/** @brief Enable or disable a switch port */
Std_ReturnType EthSwt_SetPortEnable(EthSwt_PortIdType PortId, EthSwt_PortEnableType Enable);

/** @brief Set port speed and duplex */
Std_ReturnType EthSwt_SetSpeed(EthSwt_PortIdType PortId, EthSwt_SpeedType Speed, EthSwt_DuplexType Duplex);

/** @brief Get link state of a port */
Std_ReturnType EthSwt_GetLinkState(EthSwt_PortIdType PortId, EthSwt_LinkStateType* LinkState);

/** @brief Configure a VLAN entry */
Std_ReturnType EthSwt_ConfigVlan(const EthSwt_VlanConfigType* VlanConfig);

/** @brief Forward a frame from source port to destination port mask */
Std_ReturnType EthSwt_ForwardFrame(EthSwt_PortIdType SrcPort, uint8 DstPortMask, const uint8* FrameData, uint16 Length);

/** @brief Get statistics for a port */
Std_ReturnType EthSwt_GetPortStats(EthSwt_PortIdType PortId, EthSwt_PortStatsType* Stats);

/** @brief Set MAC address filter on a port */
Std_ReturnType EthSwt_SetMacFilter(EthSwt_PortIdType PortId, const EthSwt_MacAddrType* MacAddr, boolean Enable);

/** @brief Main function — periodic housekeeping */
void EthSwt_MainFunction(void);

/** @brief Reset the switch */
Std_ReturnType EthSwt_Reset(void);

#endif /* ETHSWT_H */
