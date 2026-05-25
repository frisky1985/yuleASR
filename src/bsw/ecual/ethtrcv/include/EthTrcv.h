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
 * File: EthTrcv.h
 * Module: EthTrcv (Ethernet Transceiver Driver)
 * AUTOSAR Version: 4.4.0
 *==================================================================================================*/

#ifndef ETHTRCV_H
#define ETHTRCV_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                          INCLUDES
 *==================================================================================================*/
#include "EthTrcv_Cfg.h"
#include "Std_Types.h"
#include "Eth_GeneralTypes.h"

/*==================================================================================================
 *                                     VERSION INFORMATION
 *==================================================================================================*/
#define ETHTRCV_VENDOR_ID                    (30U)
#define ETHTRCV_MODULE_ID                    (73U)
#define ETHTRCV_AR_RELEASE_MAJOR_VERSION     (4U)
#define ETHTRCV_AR_RELEASE_MINOR_VERSION     (4U)
#define ETHTRCV_AR_RELEASE_REVISION_VERSION  (0U)
#define ETHTRCV_SW_MAJOR_VERSION             (1U)
#define ETHTRCV_SW_MINOR_VERSION             (0U)
#define ETHTRCV_SW_PATCH_VERSION             (0U)

/*==================================================================================================
 *                                     FILE VERSION CHECKS
 *==================================================================================================*/
#ifndef DISABLE_MCAL_INTERMODULE_ASR_CHECK
    #if ((ETHTRCV_AR_RELEASE_MAJOR_VERSION != STD_AR_RELEASE_MAJOR_VERSION) || \
         (ETHTRCV_AR_RELEASE_MINOR_VERSION != STD_AR_RELEASE_MINOR_VERSION))
        #error "AUTOSAR Version Numbers of EthTrcv.h and Std_Types.h are different"
    #endif
#endif

/*==================================================================================================
 *                                      DEFINES AND MACROS
 *==================================================================================================*/

/* Service IDs for Error Detection */
#define ETHTRCV_SID_INIT                     (0x01U)
#define ETHTRCV_SID_DEINIT                   (0x02U)
#define ETHTRCV_SID_SET_TRANSCEIVER_MODE     (0x03U)
#define ETHTRCV_SID_GET_TRANSCEIVER_MODE     (0x04U)
#define ETHTRCV_SID_GET_LINK_STATE           (0x05U)
#define ETHTRCV_SID_GET_BAUD_RATE            (0x06U)
#define ETHTRCV_SID_GET_DUPLEX_MODE          (0x07U)
#define ETHTRCV_SID_MAINFUNCTION             (0x08U)
#define ETHTRCV_SID_GET_VERSIONINFO          (0x09U)
#define ETHTRCV_SID_CHECK_WAKEUP             (0x0AU)
#define ETHTRCV_SID_PHY_REG_READ             (0x0BU)
#define ETHTRCV_SID_PHY_REG_WRITE            (0x0CU)
#define ETHTRCV_SID_GET_SIGNAL_QUALITY       (0x0DU)
#define ETHTRCV_SID_GET_CABLE_DIAGNOSTICS    (0x0EU)

/* Development Error Codes */
#define ETHTRCV_E_NO_ERROR                   (0x00U)
#define ETHTRCV_E_NOT_INITIALIZED            (0x01U)
#define ETHTRCV_E_INV_TRCV_IDX               (0x02U)
#define ETHTRCV_E_INV_TRCV_MODE              (0x03U)
#define ETHTRCV_E_INV_POINTER                (0x04U)
#define ETHTRCV_E_INVALID_PHY_ADDR           (0x05U)
#define ETHTRCV_E_INVALID_REG_IDX            (0x06U)
#define ETHTRCV_E_INVALID_REG_VAL            (0x07U)
#define ETHTRCV_E_TIMEOUT                    (0x08U)
#define ETHTRCV_E_INIT_FAILED                (0x09U)
#define ETHTRCV_E_ALREADY_INITIALIZED        (0x0AU)

/* Transceiver Mode Values */
#define ETHTRCV_MODE_DOWN                    (0x00U)
#define ETHTRCV_MODE_ACTIVE                  (0x01U)
#define ETHTRCV_MODE_STANDBY                 (0x02U)
#define ETHTRCV_MODE_SLEEP                   (0x03U)

/* Link State Values */
#define ETHTRCV_LINK_STATE_DOWN              (0x00U)
#define ETHTRCV_LINK_STATE_ACTIVE            (0x01U)

/* Baud Rate Values */
#define ETHTRCV_BAUD_RATE_10MBIT             (0x00U)
#define ETHTRCV_BAUD_RATE_100MBIT            (0x01U)
#define ETHTRCV_BAUD_RATE_1000MBIT           (0x02U)

/* Duplex Mode Values */
#define ETHTRCV_DUPLEX_MODE_HALF             (0x00U)
#define ETHTRCV_DUPLEX_MODE_FULL             (0x01U)

/* Transceiver Types */
#define ETHTRCV_TYPE_GENERIC                 (0x00U)
#define ETHTRCV_TYPE_TJA1100                 (0x01U)
#define ETHTRCV_TYPE_TJA1101                 (0x02U)
#define ETHTRCV_TYPE_RTL8211                 (0x03U)
#define ETHTRCV_TYPE_RTL8211E                (0x04U)
#define ETHTRCV_TYPE_KSZ8081                 (0x05U)
#define ETHTRCV_TYPE_KSZ9031                 (0x06U)
#define ETHTRCV_TYPE_LAN8720                 (0x07U)
#define ETHTRCV_TYPE_LAN8742                 (0x08U)
#define ETHTRCV_TYPE_DP83848                 (0x09U)
#define ETHTRCV_TYPE_DP83867                 (0x0AU)

/* Interface Types */
#define ETHTRCV_INTERFACE_MII                (0x00U)
#define ETHTRCV_INTERFACE_RMII               (0x01U)
#define ETHTRCV_INTERFACE_RGMII              (0x02U)
#define ETHTRCV_INTERFACE_GMII               (0x03U)
#define ETHTRCV_INTERFACE_SGMII              (0x04U)
#define ETHTRCV_INTERFACE_SMI                (0x05U)

/* PHY Register Addresses (Common) */
#define ETHTRCV_PHY_REG_BMCR                 (0x00U)    /* Basic Mode Control Register */
#define ETHTRCV_PHY_REG_BMSR                 (0x01U)    /* Basic Mode Status Register */
#define ETHTRCV_PHY_REG_PHYIDR1              (0x02U)    /* PHY Identifier Register 1 */
#define ETHTRCV_PHY_REG_PHYIDR2              (0x03U)    /* PHY Identifier Register 2 */
#define ETHTRCV_PHY_REG_ANAR                 (0x04U)    /* Auto-Negotiation Advertisement Register */
#define ETHTRCV_PHY_REG_ANLPAR               (0x05U)    /* Auto-Negotiation Link Partner Ability */
#define ETHTRCV_PHY_REG_ANER                 (0x06U)    /* Auto-Negotiation Expansion Register */
#define ETHTRCV_PHY_REG_MMD_ACCESS           (0x0DU)    /* MMD Access Control */
#define ETHTRCV_PHY_REG_MMD_DATA             (0x0EU)    /* MMD Access Data */

/* BMCR Register Bits */
#define ETHTRCV_BMCR_RESET                   (0x8000U)
#define ETHTRCV_BMCR_LOOPBACK                (0x4000U)
#define ETHTRCV_BMCR_SPEED100                (0x2000U)
#define ETHTRCV_BMCR_ANEG_ENABLE             (0x1000U)
#define ETHTRCV_BMCR_POWER_DOWN              (0x0800U)
#define ETHTRCV_BMCR_ISOLATE                 (0x0400U)
#define ETHTRCV_BMCR_RESTART_ANEG            (0x0200U)
#define ETHTRCV_BMCR_DUPLEX_FULL             (0x0100U)
#define ETHTRCV_BMCR_SPEED1000               (0x0040U)

/* BMSR Register Bits */
#define ETHTRCV_BMSR_100BASET4               (0x8000U)
#define ETHTRCV_BMSR_100BASETX_FULL          (0x4000U)
#define ETHTRCV_BMSR_100BASETX_HALF          (0x2000U)
#define ETHTRCV_BMSR_10BASET_FULL            (0x1000U)
#define ETHTRCV_BMSR_10BASET_HALF            (0x0800U)
#define ETHTRCV_BMSR_100BASET2_FULL          (0x0400U)
#define ETHTRCV_BMSR_100BASET2_HALF          (0x0200U)
#define ETHTRCV_BMSR_EXT_STATUS              (0x0100U)
#define ETHTRCV_BMSR_ANEG_COMPLETE           (0x0020U)
#define ETHTRCV_BMSR_REMOTE_FAULT            (0x0010U)
#define ETHTRCV_BMSR_ANEG_ABILITY            (0x0008U)
#define ETHTRCV_BMSR_LINK_STATUS             (0x0004U)
#define ETHTRCV_BMSR_JABBER_DETECT           (0x0002U)
#define ETHTRCV_BMSR_EXT_CAPABILITY          (0x0001U)

/*==================================================================================================
 *                                      TYPE DEFINITIONS
 *==================================================================================================*/

/* Forward declaration of configuration structure */
struct EthTrcv_ConfigType_s;
typedef struct EthTrcv_ConfigType_s EthTrcv_ConfigType;

/* Transceiver Mode Type */
typedef uint8 EthTrcv_ModeType;

/* Link State Type */
typedef uint8 EthTrcv_LinkStateType;

/* Baud Rate Type */
typedef uint8 EthTrcv_BaudRateType;

/* Duplex Mode Type */
typedef uint8 EthTrcv_DuplexModeType;

/* Transceiver Index Type */
typedef uint8 EthTrcv_TrcvIdxType;

/* Transceiver Type */
typedef uint8 EthTrcv_TypeType;

/* Interface Type */
typedef uint8 EthTrcv_InterfaceType;

/* PHY Register Type */
typedef uint16 EthTrcv_PhyRegType;

/* Wake-up Reason Type */
typedef enum
{
    ETHTRCV_WUR_NONE = 0,
    ETHTRCV_WUR_POWER_ON = 1,
    ETHTRCV_WUR_RESET = 2,
    ETHTRCV_WUR_PIN = 3,
    ETHTRCV_WUR_SYSERR = 4,
    ETHTRCV_WUR_WAKEUP = 5,
    ETHTRCV_WUR_LINK_STATE_CHANGED = 6
} EthTrcv_WakeupReasonType;

/* Signal Quality Type */
typedef enum
{
    ETHTRCV_SIGNAL_QUALITY_NO_CONNECTION = 0,
    ETHTRCV_SIGNAL_QUALITY_INVALID = 1,
    ETHTRCV_SIGNAL_QUALITY_EXCELLENT = 2,
    ETHTRCV_SIGNAL_QUALITY_GOOD = 3,
    ETHTRCV_SIGNAL_QUALITY_WEAK = 4,
    ETHTRCV_SIGNAL_QUALITY_POOR = 5,
    ETHTRCV_SIGNAL_QUALITY_ERROR = 6
} EthTrcv_SignalQualityType;

/* Cable Diagnostics Result Type */
typedef enum
{
    ETHTRCV_CABLE_DIAGNOSTICS_OK = 0,
    ETHTRCV_CABLE_DIAGNOSTICS_FAILED = 1,
    ETHTRCV_CABLE_DIAGNOSTICS_SHORT_CIRCUIT = 2,
    ETHTRCV_CABLE_DIAGNOSTICS_OPEN_CIRCUIT = 3,
    ETHTRCV_CABLE_DIAGNOSTICS_WRONG_CABLE_PAIR = 4
} EthTrcv_CableDiagnosticsResultType;

/*==================================================================================================
 *                                 CALLBACK TYPE DEFINITIONS
 *==================================================================================================*/

/* PHY Register Read Completion Callback */
typedef void (*EthTrcv_PhyRegReadCbkType)(
    uint8 TrcvIdx,
    uint8 RegIdx,
    uint16 RegVal
);

/* PHY Register Write Completion Callback */
typedef void (*EthTrcv_PhyRegWriteCbkType)(
    uint8 TrcvIdx,
    uint8 RegIdx
);

/* Wake-up Indication Callback */
typedef void (*EthTrcv_WakeupIndicationCbkType)(
    uint8 TrcvIdx
);

/* Link State Change Callback */
typedef void (*EthTrcv_LinkStateChgCbkType)(
    uint8 CtrlIdx,
    EthTrcv_LinkStateType LinkState
);

/*==================================================================================================
 *                                      GLOBAL CONSTANTS
 *==================================================================================================*/
#define ETHTRCV_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "EthTrcv_MemMap.h"

extern const EthTrcv_ConfigType EthTrcv_Config;

#define ETHTRCV_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "EthTrcv_MemMap.h"

/*==================================================================================================
 *                                      GLOBAL FUNCTIONS
 *==================================================================================================*/
#define ETHTRCV_START_SEC_CODE
#include "EthTrcv_MemMap.h"

/**
 * @brief Initializes the Ethernet Transceiver driver.
 * @param CfgPtr Pointer to configuration structure
 */
extern void EthTrcv_Init(
    const EthTrcv_ConfigType* CfgPtr
);

/**
 * @brief Deinitializes the Ethernet Transceiver driver.
 */
extern void EthTrcv_DeInit(
    void
);

/**
 * @brief Sets the transceiver mode.
 * @param TrcvIdx Index of the transceiver
 * @param CtrlIdx Index of the controller
 * @param Mode Desired mode (DOWN, ACTIVE, STANDBY, SLEEP)
 */
extern Std_ReturnType EthTrcv_SetTransceiverMode(
    uint8 TrcvIdx,
    uint8 CtrlIdx,
    EthTrcv_ModeType Mode
);

/**
 * @brief Gets the current transceiver mode.
 * @param TrcvIdx Index of the transceiver
 * @param CtrlIdx Index of the controller
 * @param Mode Pointer to store the current mode
 */
extern Std_ReturnType EthTrcv_GetTransceiverMode(
    uint8 TrcvIdx,
    uint8 CtrlIdx,
    EthTrcv_ModeType* Mode
);

/**
 * @brief Gets the link state of the transceiver.
 * @param TrcvIdx Index of the transceiver
 * @param CtrlIdx Index of the controller
 * @param LinkStatePtr Pointer to store the link state
 */
extern Std_ReturnType EthTrcv_GetLinkState(
    uint8 TrcvIdx,
    uint8 CtrlIdx,
    EthTrcv_LinkStateType* LinkStatePtr
);

/**
 * @brief Gets the baud rate of the transceiver.
 * @param TrcvIdx Index of the transceiver
 * @param CtrlIdx Index of the controller
 * @param BaudRatePtr Pointer to store the baud rate
 */
extern Std_ReturnType EthTrcv_GetBaudRate(
    uint8 TrcvIdx,
    uint8 CtrlIdx,
    EthTrcv_BaudRateType* BaudRatePtr
);

/**
 * @brief Gets the duplex mode of the transceiver.
 * @param TrcvIdx Index of the transceiver
 * @param CtrlIdx Index of the controller
 * @param DuplexModePtr Pointer to store the duplex mode
 */
extern Std_ReturnType EthTrcv_GetDuplexMode(
    uint8 TrcvIdx,
    uint8 CtrlIdx,
    EthTrcv_DuplexModeType* DuplexModePtr
);

/**
 * @brief Main function for cyclic processing.
 */
extern void EthTrcv_MainFunction(
    void
);

#if (ETHTRCV_VERSION_INFO_API == STD_ON)
/**
 * @brief Gets version information of the EthTrcv module.
 * @param VersionInfoPtr Pointer to version info structure
 */
extern void EthTrcv_GetVersionInfo(
    Std_VersionInfoType* VersionInfoPtr
);
#endif /* ETHTRCV_VERSION_INFO_API */

/**
 * @brief Checks for wake-up events.
 * @param WakeupSource Wake-up source to check
 */
extern Std_ReturnType EthTrcv_CheckWakeup(
    EcuM_WakeupSourceType WakeupSource
);

/**
 * @brief Reads a PHY register.
 * @param TrcvIdx Index of the transceiver
 * @param RegIdx Register index to read
 * @param RegValPtr Pointer to store the register value
 */
extern Std_ReturnType EthTrcv_ReadMiiIndication(
    uint8 TrcvIdx,
    uint8 RegIdx,
    uint16* RegValPtr
);

/**
 * @brief Writes a PHY register.
 * @param TrcvIdx Index of the transceiver
 * @param RegIdx Register index to write
 * @param RegVal Value to write
 */
extern Std_ReturnType EthTrcv_WriteMiiIndication(
    uint8 TrcvIdx,
    uint8 RegIdx,
    uint16 RegVal
);

/**
 * @brief Gets signal quality information.
 * @param TrcvIdx Index of the transceiver
 * @param SignalQualityPtr Pointer to store signal quality
 */
extern Std_ReturnType EthTrcv_GetSignalQuality(
    uint8 TrcvIdx,
    EthTrcv_SignalQualityType* SignalQualityPtr
);

/**
 * @brief Runs cable diagnostics.
 * @param TrcvIdx Index of the transceiver
 * @param ResultPtr Pointer to store diagnostics result
 */
extern Std_ReturnType EthTrcv_GetCableDiagnosticsResult(
    uint8 TrcvIdx,
    EthTrcv_CableDiagnosticsResultType* ResultPtr
);

#define ETHTRCV_STOP_SEC_CODE
#include "EthTrcv_MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* ETHTRCV_H */
