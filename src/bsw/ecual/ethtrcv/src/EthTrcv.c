/*==================================================================================================* Project : YuleTech
 * AutoSAR BSW* Platform             : NXP i.MX8M Mini* Dependencies         : ...** Copyright (c) 2026 Shanghai Yule
 * Electronics Technology Co., Ltd.* All rights reserved.** SPDX-License-Identifier:
 * MIT**================================================================================================*/
/*================================================================================================== * File: EthTrcv.c *
 * Module: EthTrcv (Ethernet Transceiver Driver) * AUTOSAR Version: 4.4.0
 * *==================================================================================================*/
/*================================================================================================== * INCLUDES
 * *==================================================================================================*/
#include "EthTrcv.h"
#include "EthTrcv_Cfg.h"
#include "Det.h"
#include "Eth.h"
#include "Spi.h"
#include "I2c.h"
#include "Dem.h"
#include "Mcal.h"
/*================================================================================================== * INTERNAL DEFINES
 * & MACROS *==================================================================================================*/
/* Module state definitions */
#define ETHTRCV_STATE_UNINIT (0x00U)
#define ETHTRCV_STATE_INIT (0x01U)
/* PHY access states */
#define ETHTRCV_PHY_STATE_IDLE (0x00U)
#define ETHTRCV_PHY_STATE_READ_PENDING (0x01U)
#define ETHTRCV_PHY_STATE_WRITE_PENDING (0x02U)
/* Link state machine */
#define ETHTRCV_LINK_SM_DOWN (0x00U)
#define ETHTRCV_LINK_SM_WAIT_UP (0x01U)
#define ETHTRCV_LINK_SM_UP (0x02U)
#define ETHTRCV_LINK_SM_WAIT_DOWN (0x03U)
/* Vendor ID for DET */
#define ETHTRCV_VENDOR_ID_VALUE (30U)
/*================================================================================================== * INTERNAL TYPE
 * DEFINITIONS *==================================================================================================*/
typedef struct
{
    EthTrcv_ModeType CurrentMode;
    EthTrcv_LinkStateType LinkState;
    EthTrcv_BaudRateType BaudRate;
    EthTrcv_DuplexModeType DuplexMode;
    uint8 LinkStateMachine;
    uint8 LinkDebounceCounter;
    uint8 PhyAccessState;
    uint16 PhyRegCache[32];
    /* Cache for PHY registers 0-31 */
    boolean IsInitialized;
    boolean LinkChangePending;
    uint16 LinkChangeCounter;
#if (ETHTRCV_STATS_ENABLE == STD_ON)
    uint32 LinkUpCounter;
    uint32 LinkDownCounter;
    uint32 WakeupCounter;
#endif
} EthTrcv_TrcvRuntimeType;
typedef struct
{
    uint8 ModuleState;
    EthTrcv_TrcvRuntimeType Trcv[ETHTRCV_NUMBER_OF_TRCVS];
    const EthTrcv_ConfigType* ConfigPtr;
} EthTrcv_GlobalType;
/*================================================================================================== * LOCAL CONSTANTS
 * *==================================================================================================*/
/* PHY ID to Type Mapping */
static const uint16 EthTrcv_PhyIdToType[] = {
    /* 0x0180 */
    ETHTRCV_TYPE_TJA1100, /* NXP TJA1100 */
    /* 0x001C */
    ETHTRCV_TYPE_LAN8720, /* SMSC/Microchip LAN8720 */
    /* 0x0007 */
    ETHTRCV_TYPE_DP83848, /* TI DP83848 */
    /* 0x001C */
    ETHTRCV_TYPE_KSZ8081, /* Microchip KSZ8081 */
};
/*================================================================================================== * GLOBAL VARIABLES
 * *==================================================================================================*/
#define ETHTRCV_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "EthTrcv_MemMap.h"
static EthTrcv_GlobalType EthTrcv_Global;
#define ETHTRCV_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "EthTrcv_MemMap.h"
#define ETHTRCV_START_SEC_VAR_INIT_UNSPECIFIED
#include "EthTrcv_MemMap.h"
/* Default configuration pointer */
static const EthTrcv_ConfigType* EthTrcv_CfgPtr = NULL_PTR;
#define ETHTRCV_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "EthTrcv_MemMap.h"
/*================================================================================================== * EXTERNAL
 * DECLARATIONS *==================================================================================================*/
extern const EthTrcv_TrcvConfigType EthTrcv_TrcvConfig[ETHTRCV_NUMBER_OF_TRCVS];
extern const EthTrcv_LinkStateChgCbkType EthTrcv_LinkStateChgCallback;
extern const EthTrcv_WakeupIndicationCbkType EthTrcv_WakeupIndicationCallback;
/*================================================================================================== * LOCAL FUNCTION
 * PROTOTYPES *==================================================================================================*/
static Std_ReturnType EthTrcv_DetectTransceiver(uint8 TrcvIdx);
static Std_ReturnType EthTrcv_InitTransceiver(uint8 TrcvIdx);
static Std_ReturnType EthTrcv_ReadPhyRegister(uint8 TrcvIdx, uint8 RegIdx, uint16* RegVal);
static Std_ReturnType EthTrcv_WritePhyRegister(uint8 TrcvIdx, uint8 RegIdx, uint16 RegVal);
static Std_ReturnType EthTrcv_UpdateLinkState(uint8 TrcvIdx);
static void EthTrcv_ProcessLinkStateMachine(uint8 TrcvIdx);
static Std_ReturnType EthTrcv_SetModeTJA1100(uint8 TrcvIdx, EthTrcv_ModeType Mode);
static Std_ReturnType EthTrcv_SetModeRTL8211(uint8 TrcvIdx, EthTrcv_ModeType Mode);
static Std_ReturnType EthTrcv_SetModeGeneric(uint8 TrcvIdx, EthTrcv_ModeType Mode);
static void EthTrcv_ReportDetError(uint8 ApiId, uint8 ErrorId);
/*================================================================================================== * LOCAL FUNCTIONS
 * *==================================================================================================*/
/** * @brief Reports DET error if development error detection is enabled */
static void EthTrcv_ReportDetError(uint8 ApiId, uint8 ErrorId)
{
#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    (void)Det_ReportError(ETHTRCV_MODULE_ID, ETHTRCV_INSTANCE_ID, ApiId, ErrorId);
#else
    (void)ApiId;
    (void)ErrorId;
#endif
}
/** * @brief Reads a PHY register via MII/SMI interface */
static Std_ReturnType EthTrcv_ReadPhyRegister(uint8 TrcvIdx, uint8 RegIdx, uint16* RegVal)
{
    Std_ReturnType Status = E_NOT_OK;
    const EthTrcv_TrcvConfigType* TrcvCfg = &EthTrcv_TrcvConfig[TrcvIdx];
    if (RegVal == NULL_PTR)
    {
        return E_NOT_OK;
    }
    switch (TrcvCfg->AccessInterface)
    {
        case ETHTRCV_ACCESS_MII: /* Use Eth_ReadMii for SMI access */
            Status = Eth_ReadMii(TrcvCfg->CtrlIdx, TrcvCfg->PhyAddress, RegIdx, RegVal);
            break;
        case ETHTRCV_ACCESS_SPI: /* SPI-based PHY access (TJA1100) */
            /* Implementation depends on SPI driver */
            Status = Spi_ReadPhyRegister(TrcvCfg->PhyAddress, RegIdx, RegVal);
            break;
        case ETHTRCV_ACCESS_I2C: /* I2C-based PHY access */
            Status = I2c_ReadPhyRegister(TrcvCfg->PhyAddress, RegIdx, RegVal);
            break;
        default:
            Status = E_NOT_OK;
            break;
    }
    if (Status == E_OK)
    {
        /* Update register cache */
        if (RegIdx < 32U)
        {
            EthTrcv_Global.Trcv[TrcvIdx].PhyRegCache[RegIdx] = *RegVal;
        }
    }
    return Status;
}
/** * @brief Writes a PHY register via MII/SMI interface */
static Std_ReturnType EthTrcv_WritePhyRegister(uint8 TrcvIdx, uint8 RegIdx, uint16 RegVal)
{
    Std_ReturnType Status = E_NOT_OK;
    const EthTrcv_TrcvConfigType* TrcvCfg = &EthTrcv_TrcvConfig[TrcvIdx];
    switch (TrcvCfg->AccessInterface)
    {
        case ETHTRCV_ACCESS_MII:
            Status = Eth_WriteMii(TrcvCfg->CtrlIdx, TrcvCfg->PhyAddress, RegIdx, RegVal);
            break;
        case ETHTRCV_ACCESS_SPI:
            Status = Spi_WritePhyRegister(TrcvCfg->PhyAddress, RegIdx, RegVal);
            break;
        case ETHTRCV_ACCESS_I2C:
            Status = I2c_WritePhyRegister(TrcvCfg->PhyAddress, RegIdx, RegVal);
            break;
        default:
            Status = E_NOT_OK;
            break;
    }
    if (Status == E_OK)
    {
        /* Update register cache */
        if (RegIdx < 32U)
        {
            EthTrcv_Global.Trcv[TrcvIdx].PhyRegCache[RegIdx] = RegVal;
        }
    }
    return Status;
}
/** * @brief Detects transceiver type by reading PHY ID registers */
static Std_ReturnType EthTrcv_DetectTransceiver(uint8 TrcvIdx)
{
    Std_ReturnType Status;
    uint16 PhyId1, PhyId2;
    uint32 Oui;
    uint8 Model;
    /* Read PHY ID registers */
    Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_PHY_REG_PHYIDR1, &PhyId1);
    if (Status != E_OK)
    {
        return E_NOT_OK;
    }
    Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_PHY_REG_PHYIDR2, &PhyId2);
    if (Status != E_OK)
    {
        return E_NOT_OK;
    }
    /* Calculate OUI and Model number */
    Oui = ((uint32)(PhyId1 & 0xFFFFU) << 6) | ((PhyId2 >> 10) & 0x3FU);
    Model = (uint8)((PhyId2 >> 4) & 0x3FU);
    /* Detect type based on OUI and Model */
    switch (Oui)
    {
        case 0x0001C1U: /* NXP */
            if (Model == 0x04U)
            {
                EthTrcv_TrcvConfig[TrcvIdx].DetectedType = ETHTRCV_TYPE_TJA1100;
            }
            else if (Model == 0x05U)
            {
                EthTrcv_TrcvConfig[TrcvIdx].DetectedType = ETHTRCV_TYPE_TJA1101;
            }
            break;
        case 0x0010A0U: /* Realtek */
            if ((Model & 0x38U) == 0x00U)
            {
                EthTrcv_TrcvConfig[TrcvIdx].DetectedType = ETHTRCV_TYPE_RTL8211;
            }
            break;
        case 0x00005CU: /* Microchip/SMSC */
            if (Model == 0x00U)
            {
                EthTrcv_TrcvConfig[TrcvIdx].DetectedType = ETHTRCV_TYPE_LAN8720;
            }
            else if (Model == 0x08U)
            {
                EthTrcv_TrcvConfig[TrcvIdx].DetectedType = ETHTRCV_TYPE_KSZ8081;
            }
            break;
        case 0x080017U: /* Texas Instruments */
            if (Model == 0x09U)
            {
                EthTrcv_TrcvConfig[TrcvIdx].DetectedType = ETHTRCV_TYPE_DP83848;
            }
            break;
        default:
            EthTrcv_TrcvConfig[TrcvIdx].DetectedType = ETHTRCV_TYPE_GENERIC;
            break;
    }
    return E_OK;
}
/** * @brief Initializes a specific transceiver */
static Std_ReturnType EthTrcv_InitTransceiver(uint8 TrcvIdx)
{
    Std_ReturnType Status = E_OK;
    const EthTrcv_TrcvConfigType* TrcvCfg = &EthTrcv_TrcvConfig[TrcvIdx];
    uint16 RegVal;
    /* Reset PHY */
    Status = EthTrcv_WritePhyRegister(TrcvIdx, ETHTRCV_PHY_REG_BMCR, ETHTRCV_BMCR_RESET);
    if (Status != E_OK)
    {
        return E_NOT_OK;
    }
    /* Wait for reset completion */
    do
    {
        Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_PHY_REG_BMCR, &RegVal);
    } while ((Status == E_OK) && ((RegVal & ETHTRCV_BMCR_RESET) != 0U));
    /* Configure based on transceiver type */
    switch (TrcvCfg->DetectedType)
    {
        case ETHTRCV_TYPE_TJA1100:
        case ETHTRCV_TYPE_TJA1101:
            Status = EthTrcv_SetModeTJA1100(TrcvIdx, TrcvCfg->DefaultMode);
            break;
        case ETHTRCV_TYPE_RTL8211:
        case ETHTRCV_TYPE_RTL8211E:
            Status = EthTrcv_SetModeRTL8211(TrcvIdx, TrcvCfg->DefaultMode);
            break;
        default:
            Status = EthTrcv_SetModeGeneric(TrcvIdx, TrcvCfg->DefaultMode);
            break;
    }
    if (Status == E_OK)
    {
        EthTrcv_Global.Trcv[TrcvIdx].IsInitialized = TRUE;
        EthTrcv_Global.Trcv[TrcvIdx].CurrentMode = TrcvCfg->DefaultMode;
    }
    return Status;
}
/** * @brief Sets mode for TJA1100/TJA1101 transceivers */
static Std_ReturnType EthTrcv_SetModeTJA1100(uint8 TrcvIdx, EthTrcv_ModeType Mode)
{
    Std_ReturnType Status;
    uint16 ExtCtrl = 0U;
    switch (Mode)
    {
        case ETHTRCV_MODE_DOWN:
            ExtCtrl = ETHTRCV_TJA1100_EXT_CTRL_PWR_DISABLE;
            break;
        case ETHTRCV_MODE_ACTIVE:
            ExtCtrl = ETHTRCV_TJA1100_EXT_CTRL_PWR_NORMAL;
            break;
        case ETHTRCV_MODE_STANDBY:
            ExtCtrl = ETHTRCV_TJA1100_EXT_CTRL_PWR_STANDBY;
            break;
        case ETHTRCV_MODE_SLEEP:
            ExtCtrl = ETHTRCV_TJA1100_EXT_CTRL_PWR_SLEEP;
            break;
        default:
            return E_NOT_OK;
    }
    Status = EthTrcv_WritePhyRegister(TrcvIdx, ETHTRCV_TJA1100_REG_EXTENDED_CTRL, ExtCtrl);
    return Status;
}
/** * @brief Sets mode for RTL8211 transceivers */
static Std_ReturnType EthTrcv_SetModeRTL8211(uint8 TrcvIdx, EthTrcv_ModeType Mode)
{
    Std_ReturnType Status;
    uint16 Bmcr = 0U;
    /* Read current BMCR */
    Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_PHY_REG_BMCR, &Bmcr);
    if (Status != E_OK)
    {
        return E_NOT_OK;
    }
    switch (Mode)
    {
        case ETHTRCV_MODE_DOWN:
            Bmcr |= ETHTRCV_BMCR_POWER_DOWN;
            break;
        case ETHTRCV_MODE_ACTIVE:
            Bmcr &= ~ETHTRCV_BMCR_POWER_DOWN;
            Bmcr |= ETHTRCV_BMCR_ANEG_ENABLE;
            break;
        default: /* RTL8211 doesn't support STANDBY/SLEEP modes */
            return E_NOT_OK;
    }
    Status = EthTrcv_WritePhyRegister(TrcvIdx, ETHTRCV_PHY_REG_BMCR, Bmcr);
    return Status;
}
/** * @brief Sets mode for generic PHY transceivers */
static Std_ReturnType EthTrcv_SetModeGeneric(uint8 TrcvIdx, EthTrcv_ModeType Mode)
{
    Std_ReturnType Status;
    uint16 Bmcr = 0U;
    const EthTrcv_TrcvConfigType* TrcvCfg = &EthTrcv_TrcvConfig[TrcvIdx];
    /* Read current BMCR */
    Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_PHY_REG_BMCR, &Bmcr);
    if (Status != E_OK)
    {
        return E_NOT_OK;
    }
    switch (Mode)
    {
        case ETHTRCV_MODE_DOWN:
            Bmcr |= ETHTRCV_BMCR_POWER_DOWN;
            break;
        case ETHTRCV_MODE_ACTIVE:
            Bmcr &= ~ETHTRCV_BMCR_POWER_DOWN;
            /* Configure auto-negotiation or fixed speed */
            if (TrcvCfg->AutoNegotiationEnable == TRUE)
            {
                Bmcr |= ETHTRCV_BMCR_ANEG_ENABLE;
                Bmcr |= ETHTRCV_BMCR_RESTART_ANEG;
            }
            else
            {
                Bmcr &= ~ETHTRCV_BMCR_ANEG_ENABLE;
                /* Set fixed speed */
                switch (TrcvCfg->FixedSpeed)
                {
                    case ETHTRCV_BAUD_RATE_10MBIT:
                        Bmcr &= ~ETHTRCV_BMCR_SPEED100;
                        Bmcr &= ~ETHTRCV_BMCR_SPEED1000;
                        break;
                    case ETHTRCV_BAUD_RATE_100MBIT:
                        Bmcr |= ETHTRCV_BMCR_SPEED100;
                        Bmcr &= ~ETHTRCV_BMCR_SPEED1000;
                        break;
                    case ETHTRCV_BAUD_RATE_1000MBIT:
                        Bmcr |= ETHTRCV_BMCR_SPEED100;
                        Bmcr |= ETHTRCV_BMCR_SPEED1000;
                        break;
                    default:
                        break;
                }
                /* Set duplex mode */
                if (TrcvCfg->FixedDuplexMode == ETHTRCV_DUPLEX_MODE_FULL)
                {
                    Bmcr |= ETHTRCV_BMCR_DUPLEX_FULL;
                }
                else
                {
                    Bmcr &= ~ETHTRCV_BMCR_DUPLEX_FULL;
                }
            }
            break;
        default:
            return E_NOT_OK;
    }
    Status = EthTrcv_WritePhyRegister(TrcvIdx, ETHTRCV_PHY_REG_BMCR, Bmcr);
    return Status;
}
/** * @brief Updates the link state for a transceiver */
static Std_ReturnType EthTrcv_UpdateLinkState(uint8 TrcvIdx)
{
    Std_ReturnType Status;
    uint16 Bmsr = 0U;
    uint16 Physr = 0U;
    const EthTrcv_TrcvConfigType* TrcvCfg = &EthTrcv_TrcvConfig[TrcvIdx];
    EthTrcv_TrcvRuntimeType* TrcvRuntime = &EthTrcv_Global.Trcv[TrcvIdx];
    /* Read Basic Status Register */
    Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_PHY_REG_BMSR, &Bmsr);
    if (Status != E_OK)
    {
        return E_NOT_OK;
    }
    /* Determine link state */
    if ((Bmsr & ETHTRCV_BMSR_LINK_STATUS) != 0U)
    {
        /* Link is up - apply debounce */
        if (TrcvRuntime->LinkDebounceCounter < ETHTRCV_LINK_DEBOUNCE_COUNT)
        {
            TrcvRuntime->LinkDebounceCounter++;
        }
        if (TrcvRuntime->LinkDebounceCounter >= ETHTRCV_LINK_DEBOUNCE_COUNT)
        {
            if (TrcvRuntime->LinkState != ETHTRCV_LINK_STATE_ACTIVE)
            {
                TrcvRuntime->LinkState = ETHTRCV_LINK_STATE_ACTIVE;
                TrcvRuntime->LinkChangePending = TRUE;
#if (ETHTRCV_STATS_ENABLE == STD_ON)
                TrcvRuntime->LinkUpCounter++;
#endif
            }
            /* Read speed and duplex from PHY-specific registers */
            switch (TrcvCfg->DetectedType)
            {
                case ETHTRCV_TYPE_RTL8211:
                case ETHTRCV_TYPE_RTL8211E:
                    Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_RTL8211_REG_PHYSR, &Physr);
                    if (Status == E_OK)
                    {
                        /* Parse speed from PHYSR */
                        switch (Physr & ETHTRCV_RTL8211_PHYSR_SPEED_MASK)
                        {
                            case ETHTRCV_RTL8211_PHYSR_SPEED_10:
                                TrcvRuntime->BaudRate = ETHTRCV_BAUD_RATE_10MBIT;
                                break;
                            case ETHTRCV_RTL8211_PHYSR_SPEED_100:
                                TrcvRuntime->BaudRate = ETHTRCV_BAUD_RATE_100MBIT;
                                break;
                            case ETHTRCV_RTL8211_PHYSR_SPEED_1000:
                                TrcvRuntime->BaudRate = ETHTRCV_BAUD_RATE_1000MBIT;
                                break;
                            default:
                                TrcvRuntime->BaudRate = ETHTRCV_BAUD_RATE_10MBIT;
                                break;
                        }
                        /* Parse duplex mode */
                        TrcvRuntime->DuplexMode = (Physr & ETHTRCV_RTL8211_PHYSR_DUPLEX) ? ETHTRCV_DUPLEX_MODE_FULL
                                                                                         : ETHTRCV_DUPLEX_MODE_HALF;
                    }
                    break;
                case ETHTRCV_TYPE_LAN8720:
                case ETHTRCV_TYPE_LAN8742:
                    Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_LAN8720_REG_SCSR, &Physr);
                    if (Status == E_OK)
                    {
                        TrcvRuntime->BaudRate =
                            (Physr & ETHTRCV_LAN8720_SCSR_SPEED) ? ETHTRCV_BAUD_RATE_100MBIT : ETHTRCV_BAUD_RATE_10MBIT;
                        TrcvRuntime->DuplexMode =
                            (Physr & ETHTRCV_LAN8720_SCSR_DUPLEX) ? ETHTRCV_DUPLEX_MODE_FULL : ETHTRCV_DUPLEX_MODE_HALF;
                    }
                    break;
                case ETHTRCV_TYPE_TJA1100:
                case ETHTRCV_TYPE_TJA1101:
                    Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_TJA1100_REG_COMM_STATUS, &Physr);
                    if (Status == E_OK)
                    {
                        /* TJA1100 is always 100Mbps full duplex for 100BASE-T1 */
                        TrcvRuntime->BaudRate = ETHTRCV_BAUD_RATE_100MBIT;
                        TrcvRuntime->DuplexMode = ETHTRCV_DUPLEX_MODE_FULL;
                    }
                    break;
                default: /* Generic PHY - try to parse from BMSR */
                    if ((Bmsr & ETHTRCV_BMSR_ANEG_COMPLETE) != 0U)
                    {
                        /* Auto-negotiation completed - read ANLPAR for details */
                        Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_PHY_REG_ANLPAR, &Physr);
                        if (Status == E_OK)
                        {
                            /* Determine speed from negotiated capabilities */
                            if ((Physr & (ETHTRCV_BMSR_100BASETX_FULL | ETHTRCV_BMSR_100BASETX_HALF)) != 0U)
                            {
                                TrcvRuntime->BaudRate = ETHTRCV_BAUD_RATE_100MBIT;
                            }
                            else if ((Physr & (ETHTRCV_BMSR_10BASET_FULL | ETHTRCV_BMSR_10BASET_HALF)) != 0U)
                            {
                                TrcvRuntime->BaudRate = ETHTRCV_BAUD_RATE_10MBIT;
                            }
                            else
                            {
                                TrcvRuntime->BaudRate = ETHTRCV_BAUD_RATE_10MBIT;
                            }
                            /* Determine duplex */
                            if ((Physr & (ETHTRCV_BMSR_100BASETX_FULL | ETHTRCV_BMSR_10BASET_FULL)) != 0U)
                            {
                                TrcvRuntime->DuplexMode = ETHTRCV_DUPLEX_MODE_FULL;
                            }
                            else
                            {
                                TrcvRuntime->DuplexMode = ETHTRCV_DUPLEX_MODE_HALF;
                            }
                        }
                    }
                    break;
            }
        }
    }
    else
    {
        /* Link is down - reset debounce */
        TrcvRuntime->LinkDebounceCounter = 0U;
        if (TrcvRuntime->LinkState != ETHTRCV_LINK_STATE_DOWN)
        {
            TrcvRuntime->LinkState = ETHTRCV_LINK_STATE_DOWN;
            TrcvRuntime->LinkChangePending = TRUE;
#if (ETHTRCV_STATS_ENABLE == STD_ON)
            TrcvRuntime->LinkDownCounter++;
#endif
        }
    }
    return E_OK;
}
/** * @brief Processes the link state machine */
static void EthTrcv_ProcessLinkStateMachine(uint8 TrcvIdx)
{
    EthTrcv_TrcvRuntimeType* TrcvRuntime = &EthTrcv_Global.Trcv[TrcvIdx];
    if (TrcvRuntime->LinkChangePending)
    {
        TrcvRuntime->LinkChangeCounter++;
        if (TrcvRuntime->LinkChangeCounter >= 2U) /* Delay callback by 2 main function cycles */
        {
            TrcvRuntime->LinkChangePending = FALSE;
            TrcvRuntime->LinkChangeCounter = 0U;
#if (ETHTRCV_LINK_STATE_CHG_CALLBACK == STD_ON)
            if (EthTrcv_LinkStateChgCallback != NULL_PTR)
            {
                EthTrcv_LinkStateChgCallback(EthTrcv_TrcvConfig[TrcvIdx].CtrlIdx, TrcvRuntime->LinkState);
            }
#endif
        }
    }
}
/*================================================================================================== * GLOBAL FUNCTIONS
 * *==================================================================================================*/
/** * @brief Initializes the Ethernet Transceiver driver */
void EthTrcv_Init(const EthTrcv_ConfigType* CfgPtr)
{
    uint8 TrcvIdx;
#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    /* Check if already initialized */
    if (EthTrcv_Global.ModuleState == ETHTRCV_STATE_INIT)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_ALREADY_INITIALIZED);
        return;
    }
    /* Configuration pointer can be NULL_PTR if using post-build configuration */
    (void)CfgPtr;
#endif
    /* Initialize global state */
    EthTrcv_Global.ModuleState = ETHTRCV_STATE_INIT;
    EthTrcv_Global.ConfigPtr = (CfgPtr != NULL_PTR) ? CfgPtr : &EthTrcv_Config;
    /* Initialize all configured transceivers */
    for (TrcvIdx = 0U; TrcvIdx < ETHTRCV_NUMBER_OF_TRCVS; TrcvIdx++)
    {
        /* Clear runtime data */
        (void)MemSet(&EthTrcv_Global.Trcv[TrcvIdx], 0, sizeof(EthTrcv_TrcvRuntimeType));
        /* Detect transceiver type */
        (void)EthTrcv_DetectTransceiver(TrcvIdx);
        /* Initialize transceiver */
        (void)EthTrcv_InitTransceiver(TrcvIdx);
    }
}
/** * @brief Deinitializes the Ethernet Transceiver driver */
void EthTrcv_DeInit(void)
{
    uint8 TrcvIdx;
#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (EthTrcv_Global.ModuleState != ETHTRCV_STATE_INIT)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_DEINIT, ETHTRCV_E_NOT_INITIALIZED);
        return;
    }
#endif
    /* Put all transceivers in DOWN mode */
    for (TrcvIdx = 0U; TrcvIdx < ETHTRCV_NUMBER_OF_TRCVS; TrcvIdx++)
    {
        if (EthTrcv_Global.Trcv[TrcvIdx].IsInitialized == TRUE)
        {
            (void)EthTrcv_SetTransceiverMode(TrcvIdx, EthTrcv_TrcvConfig[TrcvIdx].CtrlIdx, ETHTRCV_MODE_DOWN);
            EthTrcv_Global.Trcv[TrcvIdx].IsInitialized = FALSE;
        }
    }
    EthTrcv_Global.ModuleState = ETHTRCV_STATE_UNINIT;
    EthTrcv_Global.ConfigPtr = NULL_PTR;
}
/** * @brief Sets the transceiver mode */
Std_ReturnType EthTrcv_SetTransceiverMode(uint8 TrcvIdx, uint8 CtrlIdx, EthTrcv_ModeType Mode)
{
    Std_ReturnType Status = E_OK;
    const EthTrcv_TrcvConfigType* TrcvCfg;
#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (EthTrcv_Global.ModuleState != ETHTRCV_STATE_INIT)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_SET_TRANSCEIVER_MODE, ETHTRCV_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (TrcvIdx >= ETHTRCV_NUMBER_OF_TRCVS)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_SET_TRANSCEIVER_MODE, ETHTRCV_E_INV_TRCV_IDX);
        return E_NOT_OK;
    }
    if ((Mode != ETHTRCV_MODE_DOWN) && (Mode != ETHTRCV_MODE_ACTIVE) && (Mode != ETHTRCV_MODE_STANDBY)
        && (Mode != ETHTRCV_MODE_SLEEP))
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_SET_TRANSCEIVER_MODE, ETHTRCV_E_INV_TRCV_MODE);
        return E_NOT_OK;
    }
#else
    (void)CtrlIdx;
#endif
    TrcvCfg = &EthTrcv_TrcvConfig[TrcvIdx];
    switch (TrcvCfg->DetectedType)
    {
        case ETHTRCV_TYPE_TJA1100:
        case ETHTRCV_TYPE_TJA1101:
            Status = EthTrcv_SetModeTJA1100(TrcvIdx, Mode);
            break;
        case ETHTRCV_TYPE_RTL8211:
        case ETHTRCV_TYPE_RTL8211E:
            Status = EthTrcv_SetModeRTL8211(TrcvIdx, Mode);
            break;
        default:
            Status = EthTrcv_SetModeGeneric(TrcvIdx, Mode);
            break;
    }
    if (Status == E_OK)
    {
        EthTrcv_Global.Trcv[TrcvIdx].CurrentMode = Mode;
    }
    return Status;
}
/** * @brief Gets the current transceiver mode */
Std_ReturnType EthTrcv_GetTransceiverMode(uint8 TrcvIdx, uint8 CtrlIdx, EthTrcv_ModeType* Mode)
{
#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (EthTrcv_Global.ModuleState != ETHTRCV_STATE_INIT)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_TRANSCEIVER_MODE, ETHTRCV_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (TrcvIdx >= ETHTRCV_NUMBER_OF_TRCVS)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_TRANSCEIVER_MODE, ETHTRCV_E_INV_TRCV_IDX);
        return E_NOT_OK;
    }
    if (Mode == NULL_PTR)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_TRANSCEIVER_MODE, ETHTRCV_E_INV_POINTER);
        return E_NOT_OK;
    }
    (void)CtrlIdx;
#else
    (void)CtrlIdx;
#endif
    *Mode = EthTrcv_Global.Trcv[TrcvIdx].CurrentMode;
    return E_OK;
}
/** * @brief Gets the link state */
Std_ReturnType EthTrcv_GetLinkState(uint8 TrcvIdx, uint8 CtrlIdx, EthTrcv_LinkStateType* LinkStatePtr)
{
#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (EthTrcv_Global.ModuleState != ETHTRCV_STATE_INIT)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_LINK_STATE, ETHTRCV_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (TrcvIdx >= ETHTRCV_NUMBER_OF_TRCVS)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_LINK_STATE, ETHTRCV_E_INV_TRCV_IDX);
        return E_NOT_OK;
    }
    if (LinkStatePtr == NULL_PTR)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_LINK_STATE, ETHTRCV_E_INV_POINTER);
        return E_NOT_OK;
    }
    (void)CtrlIdx;
#else
    (void)CtrlIdx;
#endif
    *LinkStatePtr = EthTrcv_Global.Trcv[TrcvIdx].LinkState;
    return E_OK;
}
/** * @brief Gets the baud rate */
Std_ReturnType EthTrcv_GetBaudRate(uint8 TrcvIdx, uint8 CtrlIdx, EthTrcv_BaudRateType* BaudRatePtr)
{
#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (EthTrcv_Global.ModuleState != ETHTRCV_STATE_INIT)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_BAUD_RATE, ETHTRCV_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (TrcvIdx >= ETHTRCV_NUMBER_OF_TRCVS)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_BAUD_RATE, ETHTRCV_E_INV_TRCV_IDX);
        return E_NOT_OK;
    }
    if (BaudRatePtr == NULL_PTR)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_BAUD_RATE, ETHTRCV_E_INV_POINTER);
        return E_NOT_OK;
    }
    (void)CtrlIdx;
#else
    (void)CtrlIdx;
#endif
    *BaudRatePtr = EthTrcv_Global.Trcv[TrcvIdx].BaudRate;
    return E_OK;
}
/** * @brief Gets the duplex mode */
Std_ReturnType EthTrcv_GetDuplexMode(uint8 TrcvIdx, uint8 CtrlIdx, EthTrcv_DuplexModeType* DuplexModePtr)
{
#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (EthTrcv_Global.ModuleState != ETHTRCV_STATE_INIT)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_DUPLEX_MODE, ETHTRCV_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (TrcvIdx >= ETHTRCV_NUMBER_OF_TRCVS)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_DUPLEX_MODE, ETHTRCV_E_INV_TRCV_IDX);
        return E_NOT_OK;
    }
    if (DuplexModePtr == NULL_PTR)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_DUPLEX_MODE, ETHTRCV_E_INV_POINTER);
        return E_NOT_OK;
    }
    (void)CtrlIdx;
#else
    (void)CtrlIdx;
#endif
    *DuplexModePtr = EthTrcv_Global.Trcv[TrcvIdx].DuplexMode;
    return E_OK;
}
/** * @brief Main function for cyclic processing */
void EthTrcv_MainFunction(void)
{
    uint8 TrcvIdx;
    if (EthTrcv_Global.ModuleState != ETHTRCV_STATE_INIT)
    {
        return;
    }
    for (TrcvIdx = 0U; TrcvIdx < ETHTRCV_NUMBER_OF_TRCVS; TrcvIdx++)
    {
        if (EthTrcv_Global.Trcv[TrcvIdx].IsInitialized == TRUE)
        {
            /* Update link state */
            (void)EthTrcv_UpdateLinkState(TrcvIdx);
            /* Process link state machine */
            EthTrcv_ProcessLinkStateMachine(TrcvIdx);
/* Check for wake-up events if supported */
#if (ETHTRCV_WAKEUP_SUPPORT == STD_ON)
            if (EthTrcv_TrcvConfig[TrcvIdx].WakeupSupport == TRUE)
            {
                /* Wake-up detection would be implemented here based on PHY type */
            }
#endif
        }
    }
}
/** * @brief Gets version information */
#if (ETHTRCV_VERSION_INFO_API == STD_ON)
void EthTrcv_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
#    if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL_PTR)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_VERSIONINFO, ETHTRCV_E_INV_POINTER);
        return;
    }
#    endif
    VersionInfoPtr->vendorID = ETHTRCV_VENDOR_ID;
    VersionInfoPtr->moduleID = ETHTRCV_MODULE_ID;
    VersionInfoPtr->sw_major_version = ETHTRCV_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version = ETHTRCV_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version = ETHTRCV_SW_PATCH_VERSION;
}
#endif
/** * @brief Checks for wake-up events */
Std_ReturnType EthTrcv_CheckWakeup(EcuM_WakeupSourceType WakeupSource)
{
    uint8 TrcvIdx;
    Std_ReturnType Status = E_NOT_OK;
#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (EthTrcv_Global.ModuleState != ETHTRCV_STATE_INIT)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_CHECK_WAKEUP, ETHTRCV_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    (void)WakeupSource;
#if (ETHTRCV_WAKEUP_SUPPORT == STD_ON)
    for (TrcvIdx = 0U; TrcvIdx < ETHTRCV_NUMBER_OF_TRCVS; TrcvIdx++)
    {
        if ((EthTrcv_TrcvConfig[TrcvIdx].WakeupSupport == TRUE)
            && (EthTrcv_TrcvConfig[TrcvIdx].WakeupSource == WakeupSource))
        {
            /* Check PHY-specific wake-up status */
            switch (EthTrcv_TrcvConfig[TrcvIdx].DetectedType)
            {
                case ETHTRCV_TYPE_TJA1100:
                case ETHTRCV_TYPE_TJA1101: {
                    uint16 CommStatus;
                    if (EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_TJA1100_REG_COMM_STATUS, &CommStatus) == E_OK)
                    {
                        if ((CommStatus & ETHTRCV_TJA1100_COMM_REM_WUR) != 0U)
                        {
#    if (ETHTRCV_WAKEUP_IND_CALLBACK == STD_ON)
                            if (EthTrcv_WakeupIndicationCallback != NULL_PTR)
                            {
                                EthTrcv_WakeupIndicationCallback(TrcvIdx);
                            }
#    endif
                            Status = E_OK;
                        }
                    }
                    break;
                }
                default: /* Check generic link status change as wake-up */
                    if (EthTrcv_Global.Trcv[TrcvIdx].LinkState == ETHTRCV_LINK_STATE_ACTIVE)
                    {
                        Status = E_OK;
                    }
                    break;
            }
        }
    }
#else
    (void)TrcvIdx;
#endif
    return Status;
}
/** * @brief PHY register read completion indication (callback from Eth or Spi) */
Std_ReturnType EthTrcv_ReadMiiIndication(uint8 TrcvIdx, uint8 RegIdx, const uint16* RegValPtr)
{
#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (TrcvIdx >= ETHTRCV_NUMBER_OF_TRCVS)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_PHY_REG_READ, ETHTRCV_E_INV_TRCV_IDX);
        return E_NOT_OK;
    }
    if (RegValPtr == NULL_PTR)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_PHY_REG_READ, ETHTRCV_E_INV_POINTER);
        return E_NOT_OK;
    }
#endif
    /* Update cache */
    if (RegIdx < 32U)
    {
        EthTrcv_Global.Trcv[TrcvIdx].PhyRegCache[RegIdx] = *RegValPtr;
    }
    EthTrcv_Global.Trcv[TrcvIdx].PhyAccessState = ETHTRCV_PHY_STATE_IDLE;
    return E_OK;
}
/** * @brief PHY register write completion indication (callback from Eth or Spi) */
Std_ReturnType EthTrcv_WriteMiiIndication(uint8 TrcvIdx, uint8 RegIdx, uint16 RegVal)
{
#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (TrcvIdx >= ETHTRCV_NUMBER_OF_TRCVS)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_PHY_REG_WRITE, ETHTRCV_E_INV_TRCV_IDX);
        return E_NOT_OK;
    }
#endif
    (void)RegVal;
    /* Update cache */
    if (RegIdx < 32U)
    {
        EthTrcv_Global.Trcv[TrcvIdx].PhyRegCache[RegIdx] = RegVal;
    }
    EthTrcv_Global.Trcv[TrcvIdx].PhyAccessState = ETHTRCV_PHY_STATE_IDLE;
    return E_OK;
}
/** * @brief Gets signal quality information */
#if (ETHTRCV_SIGNAL_QUALITY_SUPPORT == STD_ON)
Std_ReturnType EthTrcv_GetSignalQuality(uint8 TrcvIdx, EthTrcv_SignalQualityType* SignalQualityPtr)
{
    Std_ReturnType Status = E_NOT_OK;
#    if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (EthTrcv_Global.ModuleState != ETHTRCV_STATE_INIT)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_SIGNAL_QUALITY, ETHTRCV_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (TrcvIdx >= ETHTRCV_NUMBER_OF_TRCVS)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_SIGNAL_QUALITY, ETHTRCV_E_INV_TRCV_IDX);
        return E_NOT_OK;
    }
    if (SignalQualityPtr == NULL_PTR)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_SIGNAL_QUALITY, ETHTRCV_E_INV_POINTER);
        return E_NOT_OK;
    }
#    endif
    if (EthTrcv_TrcvConfig[TrcvIdx].SignalQualitySupport == FALSE)
    {
        *SignalQualityPtr = ETHTRCV_SIGNAL_QUALITY_INVALID;
        return E_NOT_OK;
    }
    switch (EthTrcv_TrcvConfig[TrcvIdx].DetectedType)
    {
        case ETHTRCV_TYPE_TJA1100:
        case ETHTRCV_TYPE_TJA1101: {
            uint16 CommStatus;
            uint16 LinkFailCounter;
            Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_TJA1100_REG_COMM_STATUS, &CommStatus);
            if (Status == E_OK)
            {
                Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_TJA1100_REG_LINK_FAIL_COUNTER, &LinkFailCounter);
            }
            if (Status == E_OK)
            {
                if ((CommStatus & ETHTRCV_TJA1100_COMM_LINK_UP) == 0U)
                {
                    *SignalQualityPtr = ETHTRCV_SIGNAL_QUALITY_NO_CONNECTION;
                }
                else if (LinkFailCounter == 0U)
                {
                    *SignalQualityPtr = ETHTRCV_SIGNAL_QUALITY_EXCELLENT;
                }
                else if (LinkFailCounter < 5U)
                {
                    *SignalQualityPtr = ETHTRCV_SIGNAL_QUALITY_GOOD;
                }
                else if (LinkFailCounter < 20U)
                {
                    *SignalQualityPtr = ETHTRCV_SIGNAL_QUALITY_WEAK;
                }
                else
                {
                    *SignalQualityPtr = ETHTRCV_SIGNAL_QUALITY_POOR;
                }
            }
            break;
        }
        default:
            *SignalQualityPtr = ETHTRCV_SIGNAL_QUALITY_INVALID;
            Status = E_NOT_OK;
            break;
    }
    return Status;
}
#endif
/** * @brief Gets cable diagnostics result */
#if (ETHTRCV_CABLE_DIAGNOSTICS_SUPPORT == STD_ON)
Std_ReturnType EthTrcv_GetCableDiagnosticsResult(uint8 TrcvIdx, EthTrcv_CableDiagnosticsResultType* ResultPtr)
{
    Std_ReturnType Status = E_NOT_OK;
#    if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
    if (EthTrcv_Global.ModuleState != ETHTRCV_STATE_INIT)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_CABLE_DIAGNOSTICS, ETHTRCV_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (TrcvIdx >= ETHTRCV_NUMBER_OF_TRCVS)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_CABLE_DIAGNOSTICS, ETHTRCV_E_INV_TRCV_IDX);
        return E_NOT_OK;
    }
    if (ResultPtr == NULL_PTR)
    {
        EthTrcv_ReportDetError(ETHTRCV_SID_GET_CABLE_DIAGNOSTICS, ETHTRCV_E_INV_POINTER);
        return E_NOT_OK;
    }
#    endif
    if (EthTrcv_TrcvConfig[TrcvIdx].CableDiagnosticsSupport == FALSE)
    {
        *ResultPtr = ETHTRCV_CABLE_DIAGNOSTICS_FAILED;
        return E_NOT_OK;
    }
    switch (EthTrcv_TrcvConfig[TrcvIdx].DetectedType)
    {
        case ETHTRCV_TYPE_TJA1100:
        case ETHTRCV_TYPE_TJA1101: {
            uint16 ExtCtrl;
            uint16 CommStatus;
            /* Start cable test */
            Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_TJA1100_REG_EXTENDED_CTRL, &ExtCtrl);
            if (Status == E_OK)
            {
                ExtCtrl |= ETHTRCV_TJA1100_EXT_CTRL_CABLE_TEST;
                Status = EthTrcv_WritePhyRegister(TrcvIdx, ETHTRCV_TJA1100_REG_EXTENDED_CTRL, ExtCtrl);
            }
            /* Poll for completion */
            if (Status == E_OK)
            {
                uint16 Timeout = 1000U;
                do
                {
                    Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_TJA1100_REG_EXTENDED_CTRL, &ExtCtrl);
                    Timeout--;
                } while ((Status == E_OK) && ((ExtCtrl & ETHTRCV_TJA1100_EXT_CTRL_CABLE_TEST) != 0U) && (Timeout > 0U));
                if (Timeout == 0U)
                {
                    Status = E_NOT_OK;
                }
            }
            /* Read result */
            if (Status == E_OK)
            {
                Status = EthTrcv_ReadPhyRegister(TrcvIdx, ETHTRCV_TJA1100_REG_COMM_STATUS, &CommStatus);
                if (Status == E_OK)
                {
                    /* Parse cable test results from PHY state */
                    uint8 PhyState = (uint8)((CommStatus & ETHTRCV_TJA1100_COMM_PHY_STATE_MASK) >> 7);
                    switch (PhyState)
                    {
                        case 0x05: /* Active state - good cable */
                            *ResultPtr = ETHTRCV_CABLE_DIAGNOSTICS_OK;
                            break;
                        case 0x00: /* Idle error - open circuit */
                            *ResultPtr = ETHTRCV_CABLE_DIAGNOSTICS_OPEN_CIRCUIT;
                            break;
                        default:
                            *ResultPtr = ETHTRCV_CABLE_DIAGNOSTICS_FAILED;
                            break;
                    }
                }
            }
            break;
        }
        default:
            *ResultPtr = ETHTRCV_CABLE_DIAGNOSTICS_FAILED;
            Status = E_NOT_OK;
            break;
    }
    return Status;
}
#endif
