/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : Ethernet Switch
* Dependencies         : Eth (MCAL), Det
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file EthSwt.c
 * @brief Ethernet Switch Driver Implementation
 * @req SHALL_ETHSWT - AUTOSAR ECUAL Ethernet Switch
 *
 * Manages multi-port Ethernet switch configuration: port enable/disable,
 * speed/duplex, VLAN member table with ingress/egress filtering, PVID and
 * VID-PCP mapping (aligned with B1 TcpIp_VlanConfigType), flow control
 * (pause frames, high/low watermarks), MAC filters, per-port statistics,
 * port mirroring, frame forwarding. Hardware abstraction + software
 * emulation fallback.
 */

#include "EthSwt.h"
#include "Det.h"
#include <string.h>

/*==================================================================================================
 *                                    LOCAL CONSTANTS
 *==================================================================================================*/
#define ETHSWT_STATE_UNINIT                     (0x00U)
#define ETHSWT_STATE_INIT                       (0x01U)

#define ETHSWT_VLAN_NONE                        (0U)
#define ETHSWT_MAX_PCP                          (7U)

#define ETHSWT_NO_VLAN_INDEX                    (ETHSWT_MAX_VLANS)

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    #define ETHSWT_DET_REPORT_ERROR(api, err) \
        Det_ReportError(ETHSWT_MODULE_ID, ETHSWT_INSTANCE_ID, (api), (err))
#else
    #define ETHSWT_DET_REPORT_ERROR(api, err)
#endif

#define ETHSWT_IS_VALID_PORT(port) \
    (((port) < ETHSWT_MAX_PORTS) ? TRUE : FALSE)

#define ETHSWT_IS_INIT() \
    (EthSwt_InternalState.State == ETHSWT_STATE_INIT)

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/

/** Per-port runtime state */
typedef struct {
    EthSwt_PortEnableType        Enable;
    EthSwt_LinkStateType         LinkState;
    EthSwt_SpeedType             Speed;
    EthSwt_DuplexType            Duplex;
    EthSwt_PortStatsType         Stats;
    EthSwt_MacAddrType           MacAddr;
    EthSwt_MacAddrType           FilterMac;
    boolean                      FilterEnabled;
    uint16                       Pvid;
    EthSwt_FlowControlConfigType FlowControl;
    uint16                       TxQueueDepth;
    boolean                      PauseActive;    /* TX pause in progress */
    boolean                      PauseReceived;  /* RX pause in progress */
} EthSwt_PortStateType;

/** Internal module state */
typedef struct {
    uint8                        State;
    const EthSwt_ConfigType*     ConfigPtr;
    EthSwt_PortStateType         Ports[ETHSWT_MAX_PORTS];
    EthSwt_VlanConfigType        Vlans[ETHSWT_MAX_VLANS];
    uint8                        NumVlans;
    EthSwt_MirrorConfigType      Mirror;
    uint32                       TickCounter;
} EthSwt_InternalStateType;

/*==================================================================================================
 *                                    LOCAL DATA
 *==================================================================================================*/
static EthSwt_InternalStateType EthSwt_InternalState;

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
static void EthSwt_LocalInitPorts(const EthSwt_ConfigType* Config);
static void EthSwt_LocalInitVlans(const EthSwt_ConfigType* Config);
static void EthSwt_LocalInitFlowControl(const EthSwt_ConfigType* Config);
static void EthSwt_LocalInitMirror(const EthSwt_ConfigType* Config);
static boolean EthSwt_LocalPortInVlan(uint8 PortMask, EthSwt_PortIdType PortId);
static uint8 EthSwt_LocalFindVlan(uint16 VlanId);
static boolean EthSwt_LocalIsPcpValid(uint8 Pcp);
static boolean EthSwt_LocalIsWatermarkValid(const EthSwt_FlowControlConfigType* Config);
static void EthSwt_LocalForwardToPort(EthSwt_PortIdType SrcPort, EthSwt_PortIdType DstPort,
                                      uint16 Length, boolean Tagged);
static void EthSwt_LocalAccountTx(EthSwt_PortIdType SrcPort, uint16 Length, boolean Tagged);

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initialise port states from configuration.
 */
static void EthSwt_LocalInitPorts(const EthSwt_ConfigType* Config)
{
    uint8 i;
    if ((Config == NULL_PTR) || (Config->PortConfigs == NULL_PTR))
    {
        return;
    }

    for (i = 0U; i < ETHSWT_MAX_PORTS; i++)
    {
        EthSwt_InternalState.Ports[i].Enable  = ETHSWT_PORT_DISABLED;
        EthSwt_InternalState.Ports[i].LinkState = ETHSWT_LINK_DOWN;
        EthSwt_InternalState.Ports[i].Speed   = ETHSWT_SPEED_100MBPS;
        EthSwt_InternalState.Ports[i].Duplex  = ETHSWT_DUPLEX_FULL;
        EthSwt_InternalState.Ports[i].FilterEnabled = FALSE;
        EthSwt_InternalState.Ports[i].Pvid    = 0U;
        EthSwt_InternalState.Ports[i].TxQueueDepth = 0U;
        EthSwt_InternalState.Ports[i].PauseActive = FALSE;
        EthSwt_InternalState.Ports[i].PauseReceived = FALSE;
        EthSwt_InternalState.Ports[i].FlowControl.TxPauseEnable = FALSE;
        EthSwt_InternalState.Ports[i].FlowControl.RxPauseEnable = FALSE;
        EthSwt_InternalState.Ports[i].FlowControl.HighWatermark = (uint16)ETHSWT_DEFAULT_HIGH_WATERMARK;
        EthSwt_InternalState.Ports[i].FlowControl.LowWatermark  = (uint16)ETHSWT_DEFAULT_LOW_WATERMARK;
        EthSwt_InternalState.Ports[i].FlowControl.PauseTime     = (uint16)ETHSWT_DEFAULT_PAUSE_TIME;
        (void)memset(&EthSwt_InternalState.Ports[i].Stats, 0, sizeof(EthSwt_PortStatsType));
    }

    for (i = 0U; i < Config->NumPorts; i++)
    {
        if ((ETHSWT_IS_VALID_PORT(Config->PortConfigs[i].PortId)) != 0U)
        {
            EthSwt_PortIdType pid = Config->PortConfigs[i].PortId;
            EthSwt_InternalState.Ports[pid].Enable = Config->PortConfigs[i].Enable;
            EthSwt_InternalState.Ports[pid].Speed  = Config->PortConfigs[i].Speed;
            EthSwt_InternalState.Ports[pid].Duplex = Config->PortConfigs[i].Duplex;
            EthSwt_InternalState.Ports[pid].MacAddr = Config->PortConfigs[i].MacAddress;
            EthSwt_InternalState.Ports[pid].Pvid   = Config->PortConfigs[i].Pvid;
        }
    }
}

/**
 * @brief Initialise VLAN table from configuration (sanitised copy).
 */
static void EthSwt_LocalInitVlans(const EthSwt_ConfigType* Config)
{
    uint8 i;
    EthSwt_InternalState.NumVlans = 0U;

    if ((Config == NULL_PTR) || (Config->VlanConfigs == NULL_PTR))
    {
        return;
    }

    for (i = 0U; (i < Config->NumVlans) && (EthSwt_InternalState.NumVlans < ETHSWT_MAX_VLANS); i++)
    {
        EthSwt_InternalState.Vlans[EthSwt_InternalState.NumVlans] = Config->VlanConfigs[i];
        /* Sanitise: PCP limited to 0..7, boolean fields normalised */
        if ((EthSwt_LocalIsPcpValid(EthSwt_InternalState.Vlans[EthSwt_InternalState.NumVlans].VlanPriority)) == FALSE)
        {
            EthSwt_InternalState.Vlans[EthSwt_InternalState.NumVlans].VlanPriority = 0U;
        }
        if ((EthSwt_InternalState.Vlans[EthSwt_InternalState.NumVlans].DropUntagged) != FALSE)
        {
            EthSwt_InternalState.Vlans[EthSwt_InternalState.NumVlans].DropUntagged = TRUE;
        }
        if ((EthSwt_InternalState.Vlans[EthSwt_InternalState.NumVlans].Tagged) != FALSE)
        {
            EthSwt_InternalState.Vlans[EthSwt_InternalState.NumVlans].Tagged = TRUE;
        }
        EthSwt_InternalState.NumVlans++;
    }
}

/**
 * @brief Initialise per-port flow control from configuration (optional).
 */
static void EthSwt_LocalInitFlowControl(const EthSwt_ConfigType* Config)
{
    uint8 i;
    if ((Config == NULL_PTR) || (Config->FlowControlConfigs == NULL_PTR))
    {
        return;
    }

    for (i = 0U; i < ETHSWT_MAX_PORTS; i++)
    {
        if ((Config->FlowControlConfigs[i].TxPauseEnable) != FALSE)
        {
            EthSwt_InternalState.Ports[i].FlowControl.TxPauseEnable = TRUE;
        }
        if ((Config->FlowControlConfigs[i].RxPauseEnable) != FALSE)
        {
            EthSwt_InternalState.Ports[i].FlowControl.RxPauseEnable = TRUE;
        }
        if ((EthSwt_LocalIsWatermarkValid(&Config->FlowControlConfigs[i])) != FALSE)
        {
            EthSwt_InternalState.Ports[i].FlowControl.HighWatermark = Config->FlowControlConfigs[i].HighWatermark;
            EthSwt_InternalState.Ports[i].FlowControl.LowWatermark  = Config->FlowControlConfigs[i].LowWatermark;
        }
        EthSwt_InternalState.Ports[i].FlowControl.PauseTime = Config->FlowControlConfigs[i].PauseTime;
    }
}

/**
 * @brief Initialise mirror configuration (optional).
 */
static void EthSwt_LocalInitMirror(const EthSwt_ConfigType* Config)
{
    EthSwt_InternalState.Mirror.MirrorEnabled = FALSE;
    EthSwt_InternalState.Mirror.MirrorSourcePortMask = 0U;
    EthSwt_InternalState.Mirror.MirrorDestinationPort = 0U;

    if ((Config == NULL_PTR) || (Config->MirrorConfig == NULL_PTR))
    {
        return;
    }

    if ((Config->MirrorConfig->MirrorEnabled) != FALSE)
    {
        EthSwt_InternalState.Mirror.MirrorEnabled = TRUE;
    }
    EthSwt_InternalState.Mirror.MirrorSourcePortMask = Config->MirrorConfig->MirrorSourcePortMask;
    EthSwt_InternalState.Mirror.MirrorDestinationPort = Config->MirrorConfig->MirrorDestinationPort;
}

/**
 * @brief Check if a port is a member of a VLAN (port mask bit test).
 */
static boolean EthSwt_LocalPortInVlan(uint8 PortMask, EthSwt_PortIdType PortId)
{
    return ((PortMask & (uint8)(1U << PortId)) != 0U) ? TRUE : FALSE;
}

/**
 * @brief Find VLAN table index by VlanId; returns ETHSWT_NO_VLAN_INDEX if absent.
 */
static uint8 EthSwt_LocalFindVlan(uint16 VlanId)
{
    uint8 i;
    uint8 found = ETHSWT_NO_VLAN_INDEX;

    for (i = 0U; i < EthSwt_InternalState.NumVlans; i++)
    {
        if (EthSwt_InternalState.Vlans[i].VlanId == VlanId)
        {
            found = i;
            break;
        }
    }
    return found;
}

/**
 * @brief Validate 802.1p PCP value (0..7).
 */
static boolean EthSwt_LocalIsPcpValid(uint8 Pcp)
{
    return (Pcp <= ETHSWT_MAX_PCP) ? TRUE : FALSE;
}

/**
 * @brief Validate flow control watermarks (high > low).
 */
static boolean EthSwt_LocalIsWatermarkValid(const EthSwt_FlowControlConfigType* Config)
{
    boolean valid = FALSE;

    if (Config != NULL_PTR)
    {
        if (Config->HighWatermark > Config->LowWatermark)
        {
            valid = TRUE;
        }
    }
    return valid;
}

/**
 * @brief Account a TX on the source port (frames/bytes/VLAN counters).
 */
static void EthSwt_LocalAccountTx(EthSwt_PortIdType SrcPort, uint16 Length, boolean Tagged)
{
    EthSwt_InternalState.Ports[SrcPort].Stats.TxFrames++;
    EthSwt_InternalState.Ports[SrcPort].Stats.TxBytes += Length;
    if (Tagged != FALSE)
    {
        EthSwt_InternalState.Ports[SrcPort].Stats.TxVlanFrames++;
    }
}

/**
 * @brief Deliver one frame copy to a destination port (RX accounting).
 */
static void EthSwt_LocalForwardToPort(EthSwt_PortIdType SrcPort, EthSwt_PortIdType DstPort,
                                      uint16 Length, boolean Tagged)
{
    if ((EthSwt_InternalState.Ports[DstPort].Enable == ETHSWT_PORT_ENABLED) &&
        (EthSwt_InternalState.Ports[DstPort].PauseReceived == FALSE))
    {
        EthSwt_InternalState.Ports[DstPort].Stats.RxFrames++;
        EthSwt_InternalState.Ports[DstPort].Stats.RxBytes += Length;
        if (Tagged != FALSE)
        {
            EthSwt_InternalState.Ports[DstPort].Stats.RxVlanFrames++;
        }
    }
    else
    {
        EthSwt_InternalState.Ports[SrcPort].Stats.DroppedFrames++;
        EthSwt_InternalState.Ports[DstPort].Stats.DroppedFrames++;
    }
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initialise the Ethernet Switch driver.
 */
/** @req SWS_EthSwt_00001 */
void EthSwt_Init(const EthSwt_ConfigType* ConfigPtr)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (EthSwt_InternalState.State == ETHSWT_STATE_INIT)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_INIT, ETHSWT_E_ALREADY_INITIALIZED);
        return;
    }
    if (ConfigPtr == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_INIT, ETHSWT_E_PARAM_POINTER);
        return;
    }
#endif

    EthSwt_InternalState.ConfigPtr   = ConfigPtr;
    EthSwt_InternalState.State       = ETHSWT_STATE_INIT;
    EthSwt_InternalState.TickCounter = 0U;

    EthSwt_LocalInitPorts(ConfigPtr);
    EthSwt_LocalInitVlans(ConfigPtr);
    EthSwt_LocalInitFlowControl(ConfigPtr);
    EthSwt_LocalInitMirror(ConfigPtr);
}

/**
 * @brief De-initialise the Ethernet Switch driver.
 */
/** @req SWS_EthSwt_00002 */
void EthSwt_DeInit(void)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (EthSwt_InternalState.State != ETHSWT_STATE_INIT)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_DEINIT, ETHSWT_E_UNINIT);
        return;
    }
#endif

    /* Disable all ports */
    {
        uint8 i;
        for (i = 0U; i < ETHSWT_MAX_PORTS; i++)
        {
            EthSwt_InternalState.Ports[i].Enable = ETHSWT_PORT_DISABLED;
            EthSwt_InternalState.Ports[i].PauseActive = FALSE;
            EthSwt_InternalState.Ports[i].PauseReceived = FALSE;
        }
    }

    EthSwt_InternalState.State     = ETHSWT_STATE_UNINIT;
    EthSwt_InternalState.ConfigPtr = NULL_PTR;
    EthSwt_InternalState.NumVlans  = 0U;
    EthSwt_InternalState.Mirror.MirrorEnabled = FALSE;
}

/**
 * @brief Get version information.
 */
#if (ETHSWT_VERSION_INFO_API == STD_ON)
/** @req SWS_EthSwt_00003 */
void EthSwt_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETVERSIONINFO, ETHSWT_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID         = ETHSWT_VENDOR_ID;
    versioninfo->moduleID         = ETHSWT_MODULE_ID;
    versioninfo->sw_major_version = ETHSWT_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = ETHSWT_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = ETHSWT_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Enable or disable a switch port.
 */
/** @req SWS_EthSwt_00004 */
Std_ReturnType EthSwt_SetPortEnable(EthSwt_PortIdType PortId, EthSwt_PortEnableType Enable)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETPORTENABLE, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETPORTENABLE, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
#endif

    EthSwt_InternalState.Ports[PortId].Enable = Enable;

    /* Reset link state when disabling */
    if (Enable == ETHSWT_PORT_DISABLED)
    {
        EthSwt_InternalState.Ports[PortId].LinkState = ETHSWT_LINK_DOWN;
    }

    return E_OK;
}

/**
 * @brief Get current enable state of a port.
 */
/** @req SWS_EthSwt_00005 */
Std_ReturnType EthSwt_GetPortEnable(EthSwt_PortIdType PortId, EthSwt_PortEnableType* Enable)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPORTENABLE, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPORTENABLE, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if (Enable == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPORTENABLE, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *Enable = EthSwt_InternalState.Ports[PortId].Enable;
    return E_OK;
}

/**
 * @brief Set port speed and duplex.
 */
/** @req SWS_EthSwt_00006 */
Std_ReturnType EthSwt_SetSpeed(EthSwt_PortIdType PortId, EthSwt_SpeedType Speed, EthSwt_DuplexType Duplex)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETSPEED, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETSPEED, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
#endif

    /* Check port is enabled */
    if (EthSwt_InternalState.Ports[PortId].Enable == ETHSWT_PORT_DISABLED)
    {
        return E_NOT_OK;
    }

    EthSwt_InternalState.Ports[PortId].Speed  = Speed;
    EthSwt_InternalState.Ports[PortId].Duplex = Duplex;

    return E_OK;
}

/**
 * @brief Get current speed and duplex of a port.
 */
/** @req SWS_EthSwt_00007 */
Std_ReturnType EthSwt_GetSpeed(EthSwt_PortIdType PortId, EthSwt_SpeedType* Speed, EthSwt_DuplexType* Duplex)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETSPEED, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETSPEED, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if ((Speed == NULL_PTR) || (Duplex == NULL_PTR))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETSPEED, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *Speed  = EthSwt_InternalState.Ports[PortId].Speed;
    *Duplex = EthSwt_InternalState.Ports[PortId].Duplex;
    return E_OK;
}

/**
 * @brief Get link state of a port.
 */
/** @req SWS_EthSwt_00008 */
Std_ReturnType EthSwt_GetLinkState(EthSwt_PortIdType PortId, EthSwt_LinkStateType* LinkState)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETLINKSTATE, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETLINKSTATE, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if (LinkState == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETLINKSTATE, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *LinkState = EthSwt_InternalState.Ports[PortId].LinkState;
    return E_OK;
}

/**
 * @brief Configure a VLAN entry (append to member table).
 */
/** @req SWS_EthSwt_00009 */
Std_ReturnType EthSwt_ConfigVlan(const EthSwt_VlanConfigType* VlanConfig)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_CONFIGVLAN, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (VlanConfig == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_CONFIGVLAN, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if (EthSwt_InternalState.NumVlans >= ETHSWT_MAX_VLANS)
    {
        return E_NOT_OK;
    }

    EthSwt_InternalState.Vlans[EthSwt_InternalState.NumVlans] = *VlanConfig;
    if ((EthSwt_LocalIsPcpValid(EthSwt_InternalState.Vlans[EthSwt_InternalState.NumVlans].VlanPriority)) == FALSE)
    {
        EthSwt_InternalState.Vlans[EthSwt_InternalState.NumVlans].VlanPriority = 0U;
    }
    EthSwt_InternalState.NumVlans++;
    return E_OK;
}

/**
 * @brief Set/upsert a VLAN entry (member table + PCP + drop-untagged).
 */
/** @req SWS_EthSwt_00010 */
Std_ReturnType EthSwt_SetVlanConfig(const EthSwt_VlanConfigType* VlanConfig)
{
    uint8 idx;
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETVLANCONFIG, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (VlanConfig == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETVLANCONFIG, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    if ((EthSwt_LocalIsPcpValid(VlanConfig->VlanPriority)) == FALSE)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETVLANCONFIG, ETHSWT_E_INVALID_PCP);
        return E_NOT_OK;
    }

    idx = EthSwt_LocalFindVlan(VlanConfig->VlanId);
    if (idx == ETHSWT_NO_VLAN_INDEX)
    {
        if (EthSwt_InternalState.NumVlans >= ETHSWT_MAX_VLANS)
        {
            return E_NOT_OK;
        }
        idx = EthSwt_InternalState.NumVlans;
        EthSwt_InternalState.NumVlans++;
    }

    EthSwt_InternalState.Vlans[idx] = *VlanConfig;
    return E_OK;
}

/**
 * @brief Get a VLAN entry by VlanId.
 */
/** @req SWS_EthSwt_00011 */
Std_ReturnType EthSwt_GetVlanConfig(uint16 VlanId, EthSwt_VlanConfigType* VlanConfig)
{
    uint8 idx;
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETVLANCONFIG, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (VlanConfig == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETVLANCONFIG, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    idx = EthSwt_LocalFindVlan(VlanId);
    if (idx == ETHSWT_NO_VLAN_INDEX)
    {
        return E_NOT_OK;
    }

    *VlanConfig = EthSwt_InternalState.Vlans[idx];
    return E_OK;
}

/**
 * @brief Add a port to a VLAN member table.
 */
/** @req SWS_EthSwt_00012 */
Std_ReturnType EthSwt_AddVlanMember(uint16 VlanId, EthSwt_PortIdType PortId, boolean Tagged)
{
    uint8 idx;
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_ADDVLANMEMBER, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_ADDVLANMEMBER, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
#endif

    idx = EthSwt_LocalFindVlan(VlanId);
    if (idx == ETHSWT_NO_VLAN_INDEX)
    {
        return E_NOT_OK;
    }

    EthSwt_InternalState.Vlans[idx].PortMask =
        (uint8)(EthSwt_InternalState.Vlans[idx].PortMask | (uint8)(1U << PortId));
    if (Tagged != FALSE)
    {
        EthSwt_InternalState.Vlans[idx].Tagged = TRUE;
    }
    return E_OK;
}

/**
 * @brief Remove a port from a VLAN member table.
 */
/** @req SWS_EthSwt_00013 */
Std_ReturnType EthSwt_RemoveVlanMember(uint16 VlanId, EthSwt_PortIdType PortId)
{
    uint8 idx;
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_REMOVEVLANMEMBER, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_REMOVEVLANMEMBER, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
#endif

    idx = EthSwt_LocalFindVlan(VlanId);
    if (idx == ETHSWT_NO_VLAN_INDEX)
    {
        return E_NOT_OK;
    }

    EthSwt_InternalState.Vlans[idx].PortMask =
        (uint8)(EthSwt_InternalState.Vlans[idx].PortMask & (uint8)(~(uint8)(1U << PortId)));
    return E_OK;
}

/**
 * @brief Set the port VLAN ID (PVID) of a port.
 */
/** @req SWS_EthSwt_00014 */
Std_ReturnType EthSwt_SetPvid(EthSwt_PortIdType PortId, uint16 VlanId)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETPVID, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETPVID, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
#endif

    EthSwt_InternalState.Ports[PortId].Pvid = VlanId;
    return E_OK;
}

/**
 * @brief Get the port VLAN ID (PVID) of a port.
 */
/** @req SWS_EthSwt_00015 */
Std_ReturnType EthSwt_GetPvid(EthSwt_PortIdType PortId, uint16* VlanId)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPVID, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPVID, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if (VlanId == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPVID, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *VlanId = EthSwt_InternalState.Ports[PortId].Pvid;
    return E_OK;
}

/**
 * @brief Set VID-PCP mapping (802.1p priority) for a VLAN.
 */
/** @req SWS_EthSwt_00016 */
Std_ReturnType EthSwt_SetVidPcpMap(uint16 VlanId, uint8 Pcp)
{
    uint8 idx;
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETVIDPCP, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    if ((EthSwt_LocalIsPcpValid(Pcp)) == FALSE)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETVIDPCP, ETHSWT_E_INVALID_PCP);
        return E_NOT_OK;
    }

    idx = EthSwt_LocalFindVlan(VlanId);
    if (idx == ETHSWT_NO_VLAN_INDEX)
    {
        return E_NOT_OK;
    }

    EthSwt_InternalState.Vlans[idx].VlanPriority = Pcp;
    return E_OK;
}

/**
 * @brief Get VID-PCP mapping (802.1p priority) of a VLAN.
 */
/** @req SWS_EthSwt_00017 */
Std_ReturnType EthSwt_GetVidPcpMap(uint16 VlanId, uint8* Pcp)
{
    uint8 idx;
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETVIDPCP, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (Pcp == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETVIDPCP, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    idx = EthSwt_LocalFindVlan(VlanId);
    if (idx == ETHSWT_NO_VLAN_INDEX)
    {
        return E_NOT_OK;
    }

    *Pcp = EthSwt_InternalState.Vlans[idx].VlanPriority;
    return E_OK;
}

/**
 * @brief Forward a frame from source port to destination port(s).
 *        Untagged ingress: effective VLAN = source port PVID; ingress and
 *        egress member filtering applied when the VLAN exists.
 */
/** @req SWS_EthSwt_00018 */
Std_ReturnType EthSwt_ForwardFrame(EthSwt_PortIdType SrcPort, uint8 DstPortMask,
                                   const uint8* FrameData, uint16 Length)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_FORWARDFRAME, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(SrcPort))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_FORWARDFRAME, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if (FrameData == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_FORWARDFRAME, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Untagged frame (VlanId 0): member filtering follows the PVID entry */
    return EthSwt_ForwardFrameVlan(SrcPort, ETHSWT_VLAN_NONE, DstPortMask, FrameData, Length);
}

/**
 * @brief Forward a frame with VLAN semantics.
 *        VlanId == ETHSWT_VLAN_NONE: untagged frame — effective VLAN for
 *        member filtering is the source port PVID; DropUntagged applies.
 *        VlanId != 0: tagged frame — member filtering by that VLAN, no
 *        DropUntagged check.  Unknown VLANs (not in the member table) are
 *        forwarded unfiltered (legacy behaviour).
 */
/** @req SWS_EthSwt_00019 */
Std_ReturnType EthSwt_ForwardFrameVlan(EthSwt_PortIdType SrcPort, uint16 VlanId, uint8 DstPortMask,
                                       const uint8* FrameData, uint16 Length)
{
    uint8  i;
    uint8  vlanIdx;
    uint16 effectiveVlan;
    boolean tagged;
    boolean srcMember;

#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_FORWARDFRAMEVLAN, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(SrcPort))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_FORWARDFRAMEVLAN, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if (FrameData == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_FORWARDFRAMEVLAN, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    (void)FrameData;

    if (VlanId == ETHSWT_VLAN_NONE)
    {
        effectiveVlan = EthSwt_InternalState.Ports[SrcPort].Pvid;
        tagged = FALSE;
    }
    else
    {
        effectiveVlan = VlanId;
        tagged = TRUE;
    }

    vlanIdx  = EthSwt_LocalFindVlan(effectiveVlan);
    srcMember = TRUE;

    /* Ingress filtering: source port must be a member of the VLAN */
    if (vlanIdx != ETHSWT_NO_VLAN_INDEX)
    {
        srcMember = EthSwt_LocalPortInVlan(EthSwt_InternalState.Vlans[vlanIdx].PortMask, SrcPort);
        if (srcMember == FALSE)
        {
            EthSwt_InternalState.Ports[SrcPort].Stats.RxFilteredFrames++;
            return E_OK;    /* frame consumed, filtered out */
        }
        /* Drop-untagged: untagged frames dropped on VLANs that require tags */
        if ((tagged == FALSE) && (EthSwt_InternalState.Vlans[vlanIdx].DropUntagged != FALSE))
        {
            EthSwt_InternalState.Ports[SrcPort].Stats.RxFilteredFrames++;
            return E_OK;
        }
    }

    /* TX accounting + flow control pause trigger */
    EthSwt_LocalAccountTx(SrcPort, Length, tagged);

    if ((EthSwt_InternalState.Ports[SrcPort].FlowControl.TxPauseEnable != FALSE) &&
        (EthSwt_InternalState.Ports[SrcPort].PauseActive == FALSE))
    {
        EthSwt_InternalState.Ports[SrcPort].TxQueueDepth++;
        if (EthSwt_InternalState.Ports[SrcPort].TxQueueDepth >=
            EthSwt_InternalState.Ports[SrcPort].FlowControl.HighWatermark)
        {
            EthSwt_InternalState.Ports[SrcPort].PauseActive = TRUE;
            EthSwt_InternalState.Ports[SrcPort].Stats.TxPauseFrames++;
        }
    }

    /* Mirroring: copy to mirror destination port */
    if ((EthSwt_InternalState.Mirror.MirrorEnabled != FALSE) &&
        (EthSwt_InternalState.Mirror.MirrorDestinationPort != SrcPort) &&
        (ETHSWT_IS_VALID_PORT(EthSwt_InternalState.Mirror.MirrorDestinationPort)) &&
        (EthSwt_LocalPortInVlan(EthSwt_InternalState.Mirror.MirrorSourcePortMask, SrcPort) != FALSE))
    {
        EthSwt_PortIdType mport = EthSwt_InternalState.Mirror.MirrorDestinationPort;
        if (EthSwt_InternalState.Ports[mport].Enable == ETHSWT_PORT_ENABLED)
        {
            EthSwt_InternalState.Ports[mport].Stats.RxFrames++;
            EthSwt_InternalState.Ports[mport].Stats.RxBytes += Length;
            EthSwt_InternalState.Ports[mport].Stats.MirroredFrames++;
        }
    }

    /* Egress: forward to destination ports with member filtering */
    for (i = 0U; i < ETHSWT_MAX_PORTS; i++)
    {
        if ((EthSwt_LocalPortInVlan(DstPortMask, i)) != 0U)
        {
            /* Egress filtering: destination port must be a member of the VLAN */
            if ((vlanIdx != ETHSWT_NO_VLAN_INDEX) &&
                (EthSwt_LocalPortInVlan(EthSwt_InternalState.Vlans[vlanIdx].PortMask, i) == FALSE))
            {
                EthSwt_InternalState.Ports[SrcPort].Stats.TxFilteredFrames++;
                continue;
            }
            EthSwt_LocalForwardToPort(SrcPort, i, Length, tagged);
        }
    }

    return E_OK;
}

/**
 * @brief Get port statistics.
 */
/** @req SWS_EthSwt_00020 */
Std_ReturnType EthSwt_GetPortStats(EthSwt_PortIdType PortId, EthSwt_PortStatsType* Stats)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPORTSTATS, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPORTSTATS, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if (Stats == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPORTSTATS, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *Stats = EthSwt_InternalState.Ports[PortId].Stats;
    return E_OK;
}

/**
 * @brief Get statistics for a port (AUTOSAR SWS name).
 */
/** @req SWS_EthSwt_00021 */
Std_ReturnType EthSwt_GetStatistics(EthSwt_PortIdType PortId, EthSwt_PortStatsType* Stats)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETSTATISTICS, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETSTATISTICS, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if (Stats == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETSTATISTICS, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *Stats = EthSwt_InternalState.Ports[PortId].Stats;
    return E_OK;
}

/**
 * @brief Reset statistics for a port (ETHSWT_ALL_PORTS resets every port).
 */
/** @req SWS_EthSwt_00022 */
Std_ReturnType EthSwt_ResetStatistics(EthSwt_PortIdType PortId)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_RESETSTATISTICS, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (PortId == ETHSWT_ALL_PORTS)
    {
        uint8 i;
        for (i = 0U; i < ETHSWT_MAX_PORTS; i++)
        {
            (void)memset(&EthSwt_InternalState.Ports[i].Stats, 0, sizeof(EthSwt_PortStatsType));
        }
    }
    else
    {
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
        if (!ETHSWT_IS_VALID_PORT(PortId))
        {
            ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_RESETSTATISTICS, ETHSWT_E_INVALID_PORT);
            return E_NOT_OK;
        }
#endif
        (void)memset(&EthSwt_InternalState.Ports[PortId].Stats, 0, sizeof(EthSwt_PortStatsType));
    }

    return E_OK;
}

/**
 * @brief Set MAC address filter on a port.
 */
/** @req SWS_EthSwt_00023 */
Std_ReturnType EthSwt_SetMacFilter(EthSwt_PortIdType PortId, const EthSwt_MacAddrType* MacAddr, boolean Enable)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETMACFILTER, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETMACFILTER, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if (MacAddr == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETMACFILTER, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    EthSwt_InternalState.Ports[PortId].FilterMac = *MacAddr;
    EthSwt_InternalState.Ports[PortId].FilterEnabled = Enable;

    return E_OK;
}

/**
 * @brief Get MAC address filter state of a port.
 */
/** @req SWS_EthSwt_00024 */
Std_ReturnType EthSwt_GetMacFilter(EthSwt_PortIdType PortId, EthSwt_MacAddrType* MacAddr, boolean* Enable)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETMACFILTER, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETMACFILTER, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if ((MacAddr == NULL_PTR) || (Enable == NULL_PTR))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETMACFILTER, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *MacAddr = EthSwt_InternalState.Ports[PortId].FilterMac;
    *Enable  = EthSwt_InternalState.Ports[PortId].FilterEnabled;
    return E_OK;
}

/**
 * @brief Configure flow control (pause frames, high/low watermarks) for a port.
 */
/** @req SWS_EthSwt_00025 */
Std_ReturnType EthSwt_SetFlowControl(EthSwt_PortIdType PortId, const EthSwt_FlowControlConfigType* Config)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETFLOWCONTROL, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETFLOWCONTROL, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if (Config == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETFLOWCONTROL, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    if ((EthSwt_LocalIsWatermarkValid(Config)) == FALSE)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETFLOWCONTROL, ETHSWT_E_INVALID_WATERMARK);
        return E_NOT_OK;
    }

    EthSwt_InternalState.Ports[PortId].FlowControl = *Config;
    /* Re-evaluate pause state against new watermarks */
    if (EthSwt_InternalState.Ports[PortId].TxQueueDepth <
        EthSwt_InternalState.Ports[PortId].FlowControl.LowWatermark)
    {
        EthSwt_InternalState.Ports[PortId].PauseActive = FALSE;
    }
    return E_OK;
}

/**
 * @brief Get flow control configuration of a port.
 */
/** @req SWS_EthSwt_00026 */
Std_ReturnType EthSwt_GetFlowControl(EthSwt_PortIdType PortId, EthSwt_FlowControlConfigType* Config)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETFLOWCONTROL, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETFLOWCONTROL, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if (Config == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETFLOWCONTROL, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *Config = EthSwt_InternalState.Ports[PortId].FlowControl;
    return E_OK;
}

/**
 * @brief Set pause time (quanta) advertised in pause frames.
 */
/** @req SWS_EthSwt_00027 */
Std_ReturnType EthSwt_SetPauseTime(EthSwt_PortIdType PortId, uint16 PauseTime)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETPAUSETIME, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETPAUSETIME, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
#endif

    EthSwt_InternalState.Ports[PortId].FlowControl.PauseTime = PauseTime;
    return E_OK;
}

/**
 * @brief Get pause time of a port.
 */
/** @req SWS_EthSwt_00028 */
Std_ReturnType EthSwt_GetPauseTime(EthSwt_PortIdType PortId, uint16* PauseTime)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPAUSETIME, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPAUSETIME, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
    if (PauseTime == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPAUSETIME, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *PauseTime = EthSwt_InternalState.Ports[PortId].FlowControl.PauseTime;
    return E_OK;
}

/**
 * @brief Indicate received pause state (HW hook; gated by RxPauseEnable).
 */
/** @req SWS_EthSwt_00029 */
Std_ReturnType EthSwt_IndicatePause(EthSwt_PortIdType PortId, boolean Pause)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_INDICATEPAUSE, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (!ETHSWT_IS_VALID_PORT(PortId))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_INDICATEPAUSE, ETHSWT_E_INVALID_PORT);
        return E_NOT_OK;
    }
#endif

    if ((Pause != FALSE) && (EthSwt_InternalState.Ports[PortId].FlowControl.RxPauseEnable != FALSE))
    {
        if (EthSwt_InternalState.Ports[PortId].PauseReceived == FALSE)
        {
            EthSwt_InternalState.Ports[PortId].Stats.RxPauseFrames++;
        }
        EthSwt_InternalState.Ports[PortId].PauseReceived = TRUE;
    }
    if (Pause == FALSE)
    {
        EthSwt_InternalState.Ports[PortId].PauseReceived = FALSE;
    }

    return E_OK;
}

/**
 * @brief Configure port mirroring (source ports -> destination port).
 */
/** @req SWS_EthSwt_00030 */
Std_ReturnType EthSwt_SetPortMirroring(const EthSwt_MirrorConfigType* MirrorConfig)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETPORTMIRRORING, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (MirrorConfig == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETPORTMIRRORING, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    if (!ETHSWT_IS_VALID_PORT(MirrorConfig->MirrorDestinationPort))
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_SETPORTMIRRORING, ETHSWT_E_MIRROR_INVALID);
        return E_NOT_OK;
    }

    EthSwt_InternalState.Mirror = *MirrorConfig;
    return E_OK;
}

/**
 * @brief Get current port mirroring configuration.
 */
/** @req SWS_EthSwt_00031 */
Std_ReturnType EthSwt_GetPortMirroring(EthSwt_MirrorConfigType* MirrorConfig)
{
#if (ETHSWT_DEV_ERROR_DETECT == STD_ON)
    if (!ETHSWT_IS_INIT())
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPORTMIRRORING, ETHSWT_E_UNINIT);
        return E_NOT_OK;
    }
    if (MirrorConfig == NULL_PTR)
    {
        ETHSWT_DET_REPORT_ERROR(ETHSWT_SID_GETPORTMIRRORING, ETHSWT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *MirrorConfig = EthSwt_InternalState.Mirror;
    return E_OK;
}

/**
 * @brief Main function — periodic housekeeping: link polling + queue drain.
 */
/** @req SWS_EthSwt_00032 */
void EthSwt_MainFunction(void)
{
    uint8 i;

    if (!ETHSWT_IS_INIT())
    {
        return;
    }

    EthSwt_InternalState.TickCounter++;

    /* Periodically poll link status */
    if ((EthSwt_InternalState.TickCounter %
         (ETHSWT_LINK_POLL_INTERVAL_MS / ETHSWT_MAIN_FUNCTION_PERIOD_MS)) == 0U)
    {
        for (i = 0U; i < ETHSWT_MAX_PORTS; i++)
        {
            if (EthSwt_InternalState.Ports[i].Enable == ETHSWT_PORT_ENABLED)
            {
                /* In a real implementation this would read PHY register.
                 * For simulation, report UP when enabled. */
                EthSwt_InternalState.Ports[i].LinkState = ETHSWT_LINK_UP;
            }
        }
    }

    /* Flow control: drain TX queues, release pause below low watermark */
    for (i = 0U; i < ETHSWT_MAX_PORTS; i++)
    {
        if (EthSwt_InternalState.Ports[i].TxQueueDepth > 0U)
        {
            EthSwt_InternalState.Ports[i].TxQueueDepth--;
        }
        if ((EthSwt_InternalState.Ports[i].PauseActive != FALSE) &&
            (EthSwt_InternalState.Ports[i].TxQueueDepth <=
             EthSwt_InternalState.Ports[i].FlowControl.LowWatermark))
        {
            EthSwt_InternalState.Ports[i].PauseActive = FALSE;
        }
    }
}

/**
 * @brief Reset the switch (de-init then re-init with saved config).
 */
/** @req SWS_EthSwt_00033 */
Std_ReturnType EthSwt_Reset(void)
{
    const EthSwt_ConfigType* savedConfig;

    if (EthSwt_InternalState.State == ETHSWT_STATE_INIT)
    {
        savedConfig = EthSwt_InternalState.ConfigPtr;
        EthSwt_DeInit();
        if (savedConfig != NULL_PTR)
        {
            EthSwt_Init(savedConfig);
        }
    }

    return E_OK;
}
