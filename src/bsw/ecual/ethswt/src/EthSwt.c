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
 * speed/duplex, VLAN filtering, MAC filters, frame forwarding, statistics.
 * Hardware abstraction + software emulation fallback.
 */

#include "EthSwt.h"
#include "Det.h"
#include <string.h>

/*==================================================================================================
 *                                    LOCAL CONSTANTS
 *==================================================================================================*/
#define ETHSWT_STATE_UNINIT                     (0x00U)
#define ETHSWT_STATE_INIT                       (0x01U)

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
    EthSwt_PortEnableType Enable;
    EthSwt_LinkStateType  LinkState;
    EthSwt_SpeedType      Speed;
    EthSwt_DuplexType     Duplex;
    EthSwt_PortStatsType  Stats;
    EthSwt_MacAddrType    MacAddr;
    EthSwt_MacAddrType    FilterMac;
    boolean               FilterEnabled;
    uint16                Pvid;
} EthSwt_PortStateType;

/** Internal module state */
typedef struct {
    uint8                  State;
    const EthSwt_ConfigType* ConfigPtr;
    EthSwt_PortStateType   Ports[ETHSWT_MAX_PORTS];
    EthSwt_VlanConfigType  Vlans[ETHSWT_MAX_VLANS];
    uint8                  NumVlans;
    uint32                 TickCounter;
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
static boolean EthSwt_LocalPortInVlan(uint8 PortMask, EthSwt_PortIdType PortId);

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
        (void)memset(&EthSwt_InternalState.Ports[i].Stats, 0, sizeof(EthSwt_PortStatsType));
    }

    for (i = 0U; i < Config->NumPorts; i++)
    {
        if (ETHSWT_IS_VALID_PORT(Config->PortConfigs[i].PortId))
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
 * @brief Initialise VLAN table from configuration.
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
        EthSwt_InternalState.NumVlans++;
    }
}

/**
 * @brief Check if a port is a member of a VLAN (port mask bit test).
 */
static boolean EthSwt_LocalPortInVlan(uint8 PortMask, EthSwt_PortIdType PortId)
{
    return ((PortMask & (uint8)(1U << PortId)) != 0U) ? TRUE : FALSE;
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initialise the Ethernet Switch driver.
 */
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
}

/**
 * @brief De-initialise the Ethernet Switch driver.
 */
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
        }
    }

    EthSwt_InternalState.State     = ETHSWT_STATE_UNINIT;
    EthSwt_InternalState.ConfigPtr = NULL_PTR;
    EthSwt_InternalState.NumVlans  = 0U;
}

/**
 * @brief Get version information.
 */
#if (ETHSWT_VERSION_INFO_API == STD_ON)
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
 * @brief Set port speed and duplex.
 */
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
 * @brief Get link state of a port.
 */
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
 * @brief Configure a VLAN entry.
 */
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
    EthSwt_InternalState.NumVlans++;
    return E_OK;
}

/**
 * @brief Forward a frame from source port to destination port(s).
 */
Std_ReturnType EthSwt_ForwardFrame(EthSwt_PortIdType SrcPort, uint8 DstPortMask,
                                   const uint8* FrameData, uint16 Length)
{
    uint8 i;

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

    /* Update source port TX stats */
    EthSwt_InternalState.Ports[SrcPort].Stats.TxFrames++;
    EthSwt_InternalState.Ports[SrcPort].Stats.TxBytes += Length;

    /* Forward to destination ports */
    for (i = 0U; i < ETHSWT_MAX_PORTS; i++)
    {
        if (EthSwt_LocalPortInVlan(DstPortMask, i))
        {
            if (EthSwt_InternalState.Ports[i].Enable == ETHSWT_PORT_ENABLED)
            {
                EthSwt_InternalState.Ports[i].Stats.RxFrames++;
                EthSwt_InternalState.Ports[i].Stats.RxBytes += Length;
            }
            else
            {
                EthSwt_InternalState.Ports[SrcPort].Stats.DroppedFrames++;
                EthSwt_InternalState.Ports[i].Stats.DroppedFrames++;
            }
        }
    }

    (void)FrameData;
    return E_OK;
}

/**
 * @brief Get port statistics.
 */
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
 * @brief Set MAC address filter on a port.
 */
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
 * @brief Main function — periodic polling of link states.
 */
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
}

/**
 * @brief Reset the switch (de-init then re-init with saved config).
 */
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