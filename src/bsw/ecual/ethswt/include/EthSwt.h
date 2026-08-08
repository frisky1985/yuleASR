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
#define ETHSWT_SID_GETPORTENABLE                (0x19U)
#define ETHSWT_SID_GETSPEED                     (0x1AU)
#define ETHSWT_SID_GETMACFILTER                 (0x1BU)
#define ETHSWT_SID_SETVLANCONFIG                (0x1CU)
#define ETHSWT_SID_GETVLANCONFIG                (0x1DU)
#define ETHSWT_SID_ADDVLANMEMBER                (0x1EU)
#define ETHSWT_SID_REMOVEVLANMEMBER             (0x1FU)
#define ETHSWT_SID_SETPVID                      (0x20U)
#define ETHSWT_SID_GETPVID                      (0x21U)
#define ETHSWT_SID_SETVIDPCP                    (0x22U)
#define ETHSWT_SID_GETVIDPCP                    (0x23U)
#define ETHSWT_SID_FORWARDFRAMEVLAN             (0x24U)
#define ETHSWT_SID_SETFLOWCONTROL               (0x25U)
#define ETHSWT_SID_GETFLOWCONTROL               (0x26U)
#define ETHSWT_SID_SETPAUSETIME                 (0x27U)
#define ETHSWT_SID_GETPAUSETIME                 (0x28U)
#define ETHSWT_SID_INDICATEPAUSE                (0x29U)
#define ETHSWT_SID_GETSTATISTICS                (0x2AU)
#define ETHSWT_SID_RESETSTATISTICS              (0x2BU)
#define ETHSWT_SID_SETPORTMIRRORING             (0x2CU)
#define ETHSWT_SID_GETPORTMIRRORING             (0x2DU)

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
#define ETHSWT_E_INVALID_PCP                    (0x0DU)
#define ETHSWT_E_VLAN_NOT_FOUND                 (0x0EU)
#define ETHSWT_E_VLAN_FULL                      (0x0FU)
#define ETHSWT_E_INVALID_WATERMARK              (0x10U)
#define ETHSWT_E_MIRROR_INVALID                 (0x11U)
#define ETHSWT_E_INVALID_PAUSE                  (0x12U)

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

/** VLAN configuration — aligned with B1 TcpIp_VlanConfigType
 * (VlanEnabled/VlanId/VlanPriority/DropUntagged); VlanPriority carries
 * the 802.1p PCP (VID-PCP mapping), DropUntagged gates untagged ingress.
 * PortMask is the VLAN member table (bitmask of member ports). */
typedef struct {
    uint16  VlanId;             /* VLAN ID (0–4095); 0 = no VLAN */
    uint8   PortMask;           /* VLAN member table: bitmask of member ports */
    boolean Tagged;             /* Tagged (TRUE) or Untagged (FALSE) */
    uint8   VlanPriority;       /* 802.1p PCP (0–7): VID-PCP mapping */
    boolean DropUntagged;       /* Drop untagged ingress frames on member ports */
} EthSwt_VlanConfigType;

/** Flow control configuration (per port) */
typedef struct {
    boolean TxPauseEnable;      /* Generate pause frames when TX queue over HighWatermark */
    boolean RxPauseEnable;      /* Honor received pause frames (drop TX while paused) */
    uint16  HighWatermark;      /* TX queue depth that triggers pause emission */
    uint16  LowWatermark;       /* TX queue depth that releases pause */
    uint16  PauseTime;          /* Pause quanta advertised in pause frames */
} EthSwt_FlowControlConfigType;

/** Port mirroring configuration */
typedef struct {
    uint8             MirrorSourcePortMask;  /* Ports whose frames are mirrored */
    EthSwt_PortIdType MirrorDestinationPort; /* Port receiving the mirrored copy */
    boolean           MirrorEnabled;
} EthSwt_MirrorConfigType;

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
    uint64 RxPauseFrames;       /* Pause frames received (flow control) */
    uint64 TxPauseFrames;       /* Pause frames transmitted (flow control) */
    uint64 RxVlanFrames;        /* VLAN-tagged frames accepted (ingress) */
    uint64 TxVlanFrames;        /* VLAN-tagged frames forwarded (egress) */
    uint64 RxFilteredFrames;    /* Frames dropped by ingress VLAN/member filter */
    uint64 TxFilteredFrames;    /* Frames dropped by egress VLAN/member filter */
    uint64 MirroredFrames;      /* Frames copied to this port by mirroring */
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
    const EthSwt_FlowControlConfigType* FlowControlConfigs;  /* Per-port, may be NULL */
    const EthSwt_MirrorConfigType* MirrorConfig;             /* May be NULL */
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

/** @brief Get current enable state of a port */
Std_ReturnType EthSwt_GetPortEnable(EthSwt_PortIdType PortId, EthSwt_PortEnableType* Enable);

/** @brief Set port speed and duplex */
Std_ReturnType EthSwt_SetSpeed(EthSwt_PortIdType PortId, EthSwt_SpeedType Speed, EthSwt_DuplexType Duplex);

/** @brief Get current speed and duplex of a port */
Std_ReturnType EthSwt_GetSpeed(EthSwt_PortIdType PortId, EthSwt_SpeedType* Speed, EthSwt_DuplexType* Duplex);

/** @brief Get link state of a port */
Std_ReturnType EthSwt_GetLinkState(EthSwt_PortIdType PortId, EthSwt_LinkStateType* LinkState);

/** @brief Configure a VLAN entry (append to member table) */
Std_ReturnType EthSwt_ConfigVlan(const EthSwt_VlanConfigType* VlanConfig);

/** @brief Set/upsert a VLAN entry (member table + PCP + drop-untagged) */
Std_ReturnType EthSwt_SetVlanConfig(const EthSwt_VlanConfigType* VlanConfig);

/** @brief Get a VLAN entry by VlanId */
Std_ReturnType EthSwt_GetVlanConfig(uint16 VlanId, EthSwt_VlanConfigType* VlanConfig);

/** @brief Add a port to a VLAN member table */
Std_ReturnType EthSwt_AddVlanMember(uint16 VlanId, EthSwt_PortIdType PortId, boolean Tagged);

/** @brief Remove a port from a VLAN member table */
Std_ReturnType EthSwt_RemoveVlanMember(uint16 VlanId, EthSwt_PortIdType PortId);

/** @brief Set the port VLAN ID (PVID) of a port */
Std_ReturnType EthSwt_SetPvid(EthSwt_PortIdType PortId, uint16 VlanId);

/** @brief Get the port VLAN ID (PVID) of a port */
Std_ReturnType EthSwt_GetPvid(EthSwt_PortIdType PortId, uint16* VlanId);

/** @brief Set VID-PCP mapping (802.1p priority) for a VLAN */
Std_ReturnType EthSwt_SetVidPcpMap(uint16 VlanId, uint8 Pcp);

/** @brief Get VID-PCP mapping (802.1p priority) of a VLAN */
Std_ReturnType EthSwt_GetVidPcpMap(uint16 VlanId, uint8* Pcp);

/** @brief Forward a frame from source port to destination port mask */
Std_ReturnType EthSwt_ForwardFrame(EthSwt_PortIdType SrcPort, uint8 DstPortMask, const uint8* FrameData, uint16 Length);

/** @brief Forward a VLAN-tagged frame (ingress/egress member filtering applied) */
Std_ReturnType EthSwt_ForwardFrameVlan(EthSwt_PortIdType SrcPort, uint16 VlanId, uint8 DstPortMask,
                                       const uint8* FrameData, uint16 Length);

/** @brief Get statistics for a port (AUTOSAR name) */
Std_ReturnType EthSwt_GetPortStats(EthSwt_PortIdType PortId, EthSwt_PortStatsType* Stats);

/** @brief Get statistics for a port (SWS EthSwt_GetStatistics) */
Std_ReturnType EthSwt_GetStatistics(EthSwt_PortIdType PortId, EthSwt_PortStatsType* Stats);

/** @brief Reset statistics for a port (ETHSWT_ALL_PORTS resets every port) */
Std_ReturnType EthSwt_ResetStatistics(EthSwt_PortIdType PortId);

/** @brief Set MAC address filter on a port */
Std_ReturnType EthSwt_SetMacFilter(EthSwt_PortIdType PortId, const EthSwt_MacAddrType* MacAddr, boolean Enable);

/** @brief Get MAC address filter state of a port */
Std_ReturnType EthSwt_GetMacFilter(EthSwt_PortIdType PortId, EthSwt_MacAddrType* MacAddr, boolean* Enable);

/** @brief Configure flow control (pause frames, high/low watermarks) for a port */
Std_ReturnType EthSwt_SetFlowControl(EthSwt_PortIdType PortId, const EthSwt_FlowControlConfigType* Config);

/** @brief Get flow control configuration of a port */
Std_ReturnType EthSwt_GetFlowControl(EthSwt_PortIdType PortId, EthSwt_FlowControlConfigType* Config);

/** @brief Set pause time (quanta) advertised in pause frames */
Std_ReturnType EthSwt_SetPauseTime(EthSwt_PortIdType PortId, uint16 PauseTime);

/** @brief Get pause time of a port */
Std_ReturnType EthSwt_GetPauseTime(EthSwt_PortIdType PortId, uint16* PauseTime);

/** @brief Indicate received pause state (HW hook; gated by RxPauseEnable) */
Std_ReturnType EthSwt_IndicatePause(EthSwt_PortIdType PortId, boolean Pause);

/** @brief Configure port mirroring (source ports -> destination port) */
Std_ReturnType EthSwt_SetPortMirroring(const EthSwt_MirrorConfigType* MirrorConfig);

/** @brief Get current port mirroring configuration */
Std_ReturnType EthSwt_GetPortMirroring(EthSwt_MirrorConfigType* MirrorConfig);

/** @brief Main function — periodic housekeeping */
void EthSwt_MainFunction(void);

/** @brief Reset the switch */
Std_ReturnType EthSwt_Reset(void);

#endif /* ETHSWT_H */
