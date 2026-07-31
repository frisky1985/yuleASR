/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*==================================================================================================
 * File: EthTrcv_Lcfg.c
 * Module: EthTrcv (Ethernet Transceiver Driver)
 * AUTOSAR Version: 4.4.0
 *==================================================================================================*/

/*==================================================================================================
 *                                          INCLUDES
 *==================================================================================================*/
#include "EthTrcv.h"
#include "EthTrcv_Cfg.h"

/*==================================================================================================
 *                                   INTERNAL DEFINES
 *==================================================================================================*/

/* Access Interface Types */
#define ETHTRCV_ACCESS_MII                   (0x00U)
#define ETHTRCV_ACCESS_RMII                  (0x01U)
#define ETHTRCV_ACCESS_RGMII                 (0x02U)
#define ETHTRCV_ACCESS_SPI                   (0x03U)
#define ETHTRCV_ACCESS_I2C                   (0x04U)

/*==================================================================================================
 *                                INTERNAL TYPE DEFINITIONS
 *==================================================================================================*/



/**
 * @brief TJA1100 Vendor-Specific Configuration
 */
typedef struct
{
    uint16 Config1Value;
    uint16 Config2Value;
    boolean EnableWakeOnLan;
    boolean EnableSqi;
    uint8 SlaveJitter;
} EthTrcv_TJA1100ConfigType;

/**
 * @brief RTL8211 Vendor-Specific Configuration
 */
typedef struct
{
    boolean EnableGreenEthernet;
    boolean EnableCrossoverDetection;
    boolean EnableAutoMdix;
    uint8 LedMode;
} EthTrcv_RTL8211ConfigType;

/**
 * @brief MII Interface Configuration
 */
typedef struct
{
    uint8 TxClockDelay;
    uint8 RxClockDelay;
    boolean EnableClockGating;
    uint8 ClockDivisor;
} EthTrcv_MiiConfigType;

/**
 * @brief RMII Interface Configuration
 */
typedef struct
{
    uint8 RefClockSource;   /* 0=External, 1=Internal */
    uint8 RefClockFreqMHz;
    boolean CrsDvRemap;
} EthTrcv_RmiiConfigType;

/**
 * @brief RGMII Interface Configuration
 */
typedef struct
{
    uint8 TxClockDelayNs;
    uint8 RxClockDelayNs;
    boolean EnableIdDelay;
    boolean TxClockInvert;
    boolean RxClockInvert;
} EthTrcv_RgmiiConfigType;

/**
 * @brief Interface Configuration Union
 */
typedef union
{
    EthTrcv_MiiConfigType Mii;
    EthTrcv_RmiiConfigType Rmii;
    EthTrcv_RgmiiConfigType Rgmii;
} EthTrcv_InterfaceConfigUnionType;

/*==================================================================================================
 *                                      LOCAL CONSTANTS
 *==================================================================================================*/

#define ETHTRCV_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "EthTrcv_MemMap.h"

/* TJA1100 Vendor-Specific Configuration */
static const EthTrcv_TJA1100ConfigType EthTrcv_TJA1100_Config =
{
    /* Config1Value */       0x4000U,  /* Enable master mode, 100BASE-T1 */
    /* Config2Value */       0x0000U,  /* Default configuration */
    /* EnableWakeOnLan */    TRUE,
    /* EnableSqi */          TRUE,
    /* SlaveJitter */        0U
};

/* RTL8211 Vendor-Specific Configuration */
static const EthTrcv_RTL8211ConfigType EthTrcv_RTL8211_Config =
{
    /* EnableGreenEthernet */      TRUE,
    /* EnableCrossoverDetection */ TRUE,
    /* EnableAutoMdix */           TRUE,
    /* LedMode */                  0U   /* LED mode 0: Link/Activity */
};

/* MII Interface Configuration for Transceiver 0 (if using MII) */
static const EthTrcv_MiiConfigType EthTrcv_MiiConfig_Trcv0 =
{
    /* TxClockDelay */       0U,
    /* RxClockDelay */       0U,
    /* EnableClockGating */  FALSE,
    /* ClockDivisor */       2U
};

/* RMII Interface Configuration for Transceiver 0 */
static const EthTrcv_RmiiConfigType EthTrcv_RmiiConfig_Trcv0 =
{
    /* RefClockSource */     0U,       /* External reference clock */
    /* RefClockFreqMHz */    50U,      /* 50 MHz reference clock */
    /* CrsDvRemap */         FALSE
};

/* RGMII Interface Configuration for Transceiver 1 */
static const EthTrcv_RgmiiConfigType EthTrcv_RgmiiConfig_Trcv1 =
{
    /* TxClockDelayNs */     2U,       /* 2ns TX clock delay */
    /* RxClockDelayNs */     2U,       /* 2ns RX clock delay */
    /* EnableIdDelay */      TRUE,     /* Enable internal delay */
    /* TxClockInvert */      FALSE,
    /* RxClockInvert */      FALSE
};

/*==================================================================================================
 *                                      GLOBAL CONSTANTS
 *==================================================================================================*/

/**
 * @brief Transceiver Configuration Table
 */
const EthTrcv_TrcvConfigType EthTrcv_TrcvConfig[ETHTRCV_NUMBER_OF_TRCVS] =
{
    /* Transceiver 0 - TJA1100 on RMII */
    {
        /* TrcvIdx */                0U,
        /* CtrlIdx */                0U,                    /* Associated with Eth controller 0 */
        /* PhyAddress */             ETHTRCV_TRCV0_PHY_ADDRESS,
        /* TrcvType */               ETHTRCV_TRCV0_TYPE,
        /* DetectedType */           ETHTRCV_TYPE_GENERIC,  /* Will be detected at runtime */
        /* InterfaceType */          ETHTRCV_TRCV0_INTERFACE,
        /* AccessInterface */        ETHTRCV_ACCESS_MII,    /* TJA1100 uses SMI (MII management) */
        /* DefaultMode */            ETHTRCV_TRCV0_DEFAULT_MODE,
        /* AutoNegotiationEnable */  (boolean)ETHTRCV_TRCV0_AUTO_NEG_ENABLE,
        /* FixedSpeed */             ETHTRCV_TRCV0_FIXED_SPEED,
        /* FixedDuplexMode */        ETHTRCV_TRCV0_FIXED_DUPLEX,
        /* WakeupSupport */          (boolean)ETHTRCV_TRCV0_WAKEUP_SUPPORT,
        /* WakeupMode */             ETHTRCV_TRCV0_WAKEUP_MODE,
        /* WakeupSource */           0x00000001UL,          /* EcuM Wakeup Source 0 */
        /* CableDiagnosticsSupport */ (boolean)ETHTRCV_TRCV0_CABLE_DIAG_ENABLE,
        /* SignalQualitySupport */   (boolean)ETHTRCV_TRCV0_SIGNAL_QUALITY_ENABLE,
        /* ResetDelayUs */           1000U,                 /* 1ms reset delay */
        /* LinkUpDelayMs */          100U,                  /* 100ms link up delay */
        /* VendorSpecificConfig */   &EthTrcv_TJA1100_Config
    },
    
    /* Transceiver 1 - RTL8211 on RGMII */
    {
        /* TrcvIdx */                1U,
        /* CtrlIdx */                1U,                    /* Associated with Eth controller 1 */
        /* PhyAddress */             ETHTRCV_TRCV1_PHY_ADDRESS,
        /* TrcvType */               ETHTRCV_TRCV1_TYPE,
        /* DetectedType */           ETHTRCV_TYPE_GENERIC,
        /* InterfaceType */          ETHTRCV_TRCV1_INTERFACE,
        /* AccessInterface */        ETHTRCV_ACCESS_MII,    /* RTL8211 uses SMI */
        /* DefaultMode */            ETHTRCV_TRCV1_DEFAULT_MODE,
        /* AutoNegotiationEnable */  (boolean)ETHTRCV_TRCV1_AUTO_NEG_ENABLE,
        /* FixedSpeed */             ETHTRCV_TRCV1_FIXED_SPEED,
        /* FixedDuplexMode */        ETHTRCV_TRCV1_FIXED_DUPLEX,
        /* WakeupSupport */          (boolean)ETHTRCV_TRCV1_WAKEUP_SUPPORT,
        /* WakeupMode */             ETHTRCV_TRCV1_WAKEUP_MODE,
        /* WakeupSource */           0x00000002UL,          /* EcuM Wakeup Source 1 */
        /* CableDiagnosticsSupport */ (boolean)ETHTRCV_TRCV1_CABLE_DIAG_ENABLE,
        /* SignalQualitySupport */   (boolean)ETHTRCV_TRCV1_SIGNAL_QUALITY_ENABLE,
        /* ResetDelayUs */           500U,
        /* LinkUpDelayMs */          2000U,                 /* 2s for 1000BASE-T */
        /* VendorSpecificConfig */   &EthTrcv_RTL8211_Config
    }
};

/**
 * @brief Interface Configuration Table
 */
static const EthTrcv_InterfaceConfigType EthTrcv_InterfaceConfig[ETHTRCV_NUMBER_OF_TRCVS] =
{
    /* Transceiver 0 - RMII */
    {
        .InterfaceType  = ETHTRCV_ACCESS_RMII,
        .MaxFrameSize   = 1518U,
        .ClockGating    = FALSE,
        .ClockDivisor   = 1U
    },

    /* Transceiver 1 - RGMII */
    {
        .InterfaceType  = ETHTRCV_ACCESS_RGMII,
        .MaxFrameSize   = 1518U,
        .ClockGating    = FALSE,
        .ClockDivisor   = 1U
    }
};

/**
 * @brief Link State Change Callback
 * @implements EthTrcv_LinkStateChgCallback
 */
#if (ETHTRCV_LINK_STATE_CHG_CALLBACK == STD_ON)
extern void EthIf_LinkStateChg(uint8 CtrlIdx, EthTrcv_LinkStateType LinkState);

const EthTrcv_LinkStateChgCbkType EthTrcv_LinkStateChgCallback = EthIf_LinkStateChg;
#else
const EthTrcv_LinkStateChgCbkType EthTrcv_LinkStateChgCallback = NULL_PTR;
#endif

/**
 * @brief Wake-up Indication Callback
 * @implements EthTrcv_WakeupIndicationCallback
 */
#if (ETHTRCV_WAKEUP_IND_CALLBACK == STD_ON)
extern void EcuM_CheckWakeup(EcuM_WakeupSourceType WakeupSource);
extern void EthTrcv_WakeupIndication(uint8 TrcvIdx);

static void EthTrcv_WakeupInd(uint8 TrcvIdx)
{
    (void)TrcvIdx;
#if (ETHTRCV_WAKEUP_SUPPORT == STD_ON)
    EcuM_CheckWakeup(EthTrcv_TrcvConfig[TrcvIdx].WakeupSource);
#endif
}

const EthTrcv_WakeupIndicationCbkType EthTrcv_WakeupIndicationCallback = EthTrcv_WakeupInd;
#else
const EthTrcv_WakeupIndicationCbkType EthTrcv_WakeupIndicationCallback = NULL_PTR;
#endif

/**
 * @brief General EthTrcv Configuration
 */
const EthTrcv_ConfigType EthTrcv_Config =
{
    /* NumberOfTransceivers */   ETHTRCV_NUMBER_OF_TRCVS,
    /* TransceiverConfig */      EthTrcv_TrcvConfig,
    /* InterfaceConfig */        EthTrcv_InterfaceConfig,
    /* MainFunctionPeriodMs */   ETHTRCV_MAIN_FUNCTION_PERIOD_MS,
    /* PhyAccessTimeoutMs */     ETHTRCV_PHY_ACCESS_TIMEOUT_MS,
    /* LinkDebounceCount */      ETHTRCV_LINK_DEBOUNCE_COUNT,
    /* DevErrorDetect */         (boolean)ETHTRCV_DEV_ERROR_DETECT,
    /* VersionInfoApi */         (boolean)ETHTRCV_VERSION_INFO_API,
    /* WakeupSupport */          (boolean)ETHTRCV_WAKEUP_SUPPORT,
    /* CableDiagnosticsSupport */ (boolean)ETHTRCV_CABLE_DIAGNOSTICS_SUPPORT,
    /* SignalQualitySupport */   (boolean)ETHTRCV_SIGNAL_QUALITY_SUPPORT
};

#define ETHTRCV_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "EthTrcv_MemMap.h"

/*==================================================================================================
 *                                      GLOBAL FUNCTIONS
 *==================================================================================================*/

#define ETHTRCV_START_SEC_CODE
#include "EthTrcv_MemMap.h"

/**
 * @brief Gets the interface configuration for a transceiver
 */
const EthTrcv_InterfaceConfigType* EthTrcv_GetInterfaceConfig(uint8 TrcvIdx)
{
    if (TrcvIdx < ETHTRCV_NUMBER_OF_TRCVS)
    {
        return &EthTrcv_InterfaceConfig[TrcvIdx];
    }
    return NULL_PTR;
}

/**
 * @brief Gets the transceiver configuration
 */
const EthTrcv_TrcvConfigType* EthTrcv_GetTransceiverConfig(uint8 TrcvIdx)
{
    if (TrcvIdx < ETHTRCV_NUMBER_OF_TRCVS)
    {
        return &EthTrcv_TrcvConfig[TrcvIdx];
    }
    return NULL_PTR;
}

/**
 * @brief Gets the PHY address for a transceiver
 */
uint8 EthTrcv_GetPhyAddress(uint8 TrcvIdx)
{
    if (TrcvIdx < ETHTRCV_NUMBER_OF_TRCVS)
    {
        return EthTrcv_TrcvConfig[TrcvIdx].PhyAddress;
    }
    return 0xFFU;  /* Invalid address */
}

/**
 * @brief Gets the interface type for a transceiver
 */
uint8 EthTrcv_GetInterfaceType(uint8 TrcvIdx)
{
    if (TrcvIdx < ETHTRCV_NUMBER_OF_TRCVS)
    {
        return EthTrcv_TrcvConfig[TrcvIdx].InterfaceType;
    }
    return 0xFFU;
}

#define ETHTRCV_STOP_SEC_CODE
#include "EthTrcv_MemMap.h"
