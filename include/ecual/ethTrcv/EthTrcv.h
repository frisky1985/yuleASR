/******************************************************************************
 * @file    EthTrcv.h
 * @brief   Ethernet Transceiver (EthTrcv) - AUTOSAR R22-11
 *
 * This module provides an interface to control Ethernet PHY transceivers.
 * It supports 100BASE-T1 Automotive Ethernet PHY (e.g., TJA1101).
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x35 (EthTrcv)
 * ASIL-B Safety Level
 * MISRA C:2012 compliant
 *
 * Supported PHY: TJA1101 (NXP 100BASE-T1 Automotive Ethernet PHY)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ETHTRCV_H
#define ETHTRCV_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "common/autosar_types.h"
#include "common/autosar_errors.h"
#include "autosar/service/Det/Det.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Module Version Information
 ******************************************************************************/
#define ETHTRCV_VENDOR_ID               0x01U
#define ETHTRCV_MODULE_ID               0x35U  /* EthTrcv module ID per AUTOSAR R22-11 */
#define ETHTRCV_SW_MAJOR_VERSION        1U
#define ETHTRCV_SW_MINOR_VERSION        0U
#define ETHTRCV_SW_PATCH_VERSION        0U

/******************************************************************************
 * API Service IDs
 ******************************************************************************/
#define ETHTRCV_SID_INIT                        0x01U
#define ETHTRCV_SID_DEINIT                      0x02U
#define ETHTRCV_SID_SETTRANSCEIVERMODE          0x03U
#define ETHTRCV_SID_GETTRANSCEIVERMODE          0x04U
#define ETHTRCV_SID_GETLINKSTATE                0x05U
#define ETHTRCV_SID_GETBAUDRATE                 0x06U
#define ETHTRCV_SID_SETPHYTESTMODE              0x07U
#define ETHTRCV_SID_SETPHYLOOPBACKMODE          0x08U
#define ETHTRCV_SID_SETPHYTXMODE                0x09U
#define ETHTRCV_SID_CHECKWAKEUP                 0x0AU
#define ETHTRCV_SID_GETTRANSCEIVERWAKEUPMODE    0x0BU
#define ETHTRCV_SID_SETTRANSCEIVERWAKEUPMODE    0x0CU
#define ETHTRCV_SID_GETVERSIONINFO              0x0DU
#define ETHTRCV_SID_MAINFUNCTION                0x0EU
#define ETHTRCV_SID_CBKLINKCHGINDICATION        0x0FU
#define ETHTRCV_SID_SETWAKEUPMODE               0x10U

/******************************************************************************
 * Error Codes
 ******************************************************************************/
#define ETHTRCV_E_NOT_INITIALIZED       0x01U
#define ETHTRCV_E_INVALID_TRCV_IDX      0x02U
#define ETHTRCV_E_INVALID_POINTER       0x03U
#define ETHTRCV_E_INVALID_MODE          0x04U
#define ETHTRCV_E_INVALID_PARAMETER     0x05U
#define ETHTRCV_E_PHY_ACCESS_FAILED     0x06U
#define ETHTRCV_E_TIMEOUT               0x07U
#define ETHTRCV_E_INIT_FAILED           0x08U
#define ETHTRCV_E_NOT_SUPPORTED         0x09U

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/
#define ETHTRCV_MAX_TRANSCEIVERS        8U
#define ETHTRCV_PHY_ADDR_LEN            5U     /* 5-bit PHY address */
#define ETHTRCV_MAX_PHY_REGISTERS       32U
#define ETHTRCV_LINK_CHECK_PERIOD_MS    100U   /* Link status polling period */
#define ETHTRCV_PHY_RESET_TIMEOUT_MS    1000U  /* PHY reset timeout */
#define ETHTRCV_LINK_UP_TIMEOUT_MS      5000U  /* Link up timeout */

/******************************************************************************
 * PHY Register Addresses (TJA1101 specific)
 ******************************************************************************/
#define ETHTRCV_PHY_REG_BASIC_CTRL      0x00U  /* Basic Control Register */
#define ETHTRCV_PHY_REG_BASIC_STATUS    0x01U  /* Basic Status Register */
#define ETHTRCV_PHY_REG_ID1             0x02U  /* PHY Identifier 1 */
#define ETHTRCV_PHY_REG_ID2             0x03U  /* PHY Identifier 2 */
#define ETHTRCV_PHY_REG_EXTENDED_STATUS 0x0FU  /* Extended Status Register */
#define ETHTRCV_PHY_REG_EXTENDED_CTRL   0x11U  /* Extended Control Register (TJA1101) */
#define ETHTRCV_PHY_REG_CONFIG1         0x12U  /* Configuration Register 1 (TJA1101) */
#define ETHTRCV_PHY_REG_CONFIG2         0x13U  /* Configuration Register 2 (TJA1101) */
#define ETHTRCV_PHY_REG_SYM_ERR_COUNTER 0x14U  /* Symbol Error Counter */
#define ETHTRCV_PHY_REG_INT_SOURCE      0x15U  /* Interrupt Source Register */
#define ETHTRCV_PHY_REG_INT_ENABLE      0x16U  /* Interrupt Enable Register */
#define ETHTRCV_PHY_REG_COMM_STATUS     0x17U  /* Communication Status Register */
#define ETHTRCV_PHY_REG_GENERAL_STATUS  0x18U  /* General Status Register */

/* Basic Control Register bits */
#define ETHTRCV_PHY_BCTRL_RESET         0x8000U
#define ETHTRCV_PHY_BCTRL_LOOPBACK      0x4000U
#define ETHTRCV_PHY_BCTRL_SPEED_SEL     0x2000U
#define ETHTRCV_PHY_BCTRL_AUTONEG       0x1000U
#define ETHTRCV_PHY_BCTRL_POWER_DOWN    0x0800U
#define ETHTRCV_PHY_BCTRL_ISOLATE       0x0400U
#define ETHTRCV_PHY_BCTRL_RESTART_AUTONEG   0x0200U
#define ETHTRCV_PHY_BCTRL_DUPLEX_MODE   0x0100U

/* Basic Status Register bits */
#define ETHTRCV_PHY_BSTATUS_100BASE_T4      0x8000U
#define ETHTRCV_PHY_BSTATUS_100BASE_TX_FD   0x4000U
#define ETHTRCV_PHY_BSTATUS_100BASE_TX_HD   0x2000U
#define ETHTRCV_BSTATUS_10BASE_T_FD         0x1000U
#define ETHTRCV_BSTATUS_10BASE_T_HD         0x0800U
#define ETHTRCV_PHY_BSTATUS_AUTONEG_COMP    0x0020U
#define ETHTRCV_PHY_BSTATUS_REMOTE_FAULT    0x0010U
#define ETHTRCV_PHY_BSTATUS_AUTONEG_ABLE    0x0008U
#define ETHTRCV_PHY_BSTATUS_LINK_STATUS     0x0004U
#define ETHTRCV_PHY_BSTATUS_JABBER_DETECT   0x0002U
#define ETHTRCV_PHY_BSTATUS_EXT_CAPABLE     0x0001U

/* TJA1101 Extended Control Register bits */
#define ETHTRCV_PHY_ECTRL_LINK_CONTROL      0x8000U
#define ETHTRCV_PHY_ECTRL_POWER_MODE_MASK   0x0007U
#define ETHTRCV_PHY_ECTRL_POWER_MODE_NORMAL 0x0000U
#define ETHTRCV_PHY_ECTRL_POWER_MODE_STANDBY    0x0001U
#define ETHTRCV_PHY_ECTRL_POWER_MODE_SLEEP  0x0002U

/******************************************************************************
 * Transceiver State Type
 ******************************************************************************/
typedef enum {
    ETHTRCV_STATE_UNINIT = 0,
    ETHTRCV_STATE_INIT,
    ETHTRCV_STATE_ACTIVE,
    ETHTRCV_STATE_DOWN,
    ETHTRCV_STATE_STANDBY
} EthTrcv_StateType;

/******************************************************************************
 * Transceiver Mode Type
 ******************************************************************************/
typedef uint8 EthTrcv_ModeType;

#define ETHTRCV_MODE_DOWN               0x00U
#define ETHTRCV_MODE_STANDBY            0x01U
#define ETHTRCV_MODE_ACTIVE             0x02U

#define ETHTRCV_MODE_IS_VALID(mode)     \
    (((mode) == ETHTRCV_MODE_DOWN) || \
     ((mode) == ETHTRCV_MODE_STANDBY) || \
     ((mode) == ETHTRCV_MODE_ACTIVE))

/******************************************************************************
 * Link State Type
 ******************************************************************************/
typedef uint8 EthTrcv_LinkStateType;

#define ETHTRCV_LINK_DOWN               0x00U
#define ETHTRCV_LINK_UP                 0x01U

/******************************************************************************
 * Baud Rate Type
 ******************************************************************************/
typedef enum {
    ETHTRCV_BAUD_RATE_10MBIT = 0,
    ETHTRCV_BAUD_RATE_100MBIT,
    ETHTRCV_BAUD_RATE_1000MBIT,
    ETHTRCV_BAUD_RATE_2500MBIT,
    ETHTRCV_BAUD_RATE_10GBIT,
    ETHTRCV_BAUD_RATE_INVALID
} EthTrcv_BaudRateType;

/******************************************************************************
 * PHY Test Mode Type
 ******************************************************************************/
typedef enum {
    ETHTRCV_PHYTESTMODE_NONE = 0,
    ETHTRCV_PHYTESTMODE_1,      /* Test mode 1 - Waveform test */
    ETHTRCV_PHYTESTMODE_2,      /* Test mode 2 - Jitter master */
    ETHTRCV_PHYTESTMODE_3,      /* Test mode 3 - Jitter slave */
    ETHTRCV_PHYTESTMODE_4,      /* Test mode 4 - Transmitter distortion */
    ETHTRCV_PHYTESTMODE_5       /* Test mode 5 - Power spectral density */
} EthTrcv_PhyTestModeType;

/******************************************************************************
 * PHY Loopback Mode Type
 ******************************************************************************/
typedef enum {
    ETHTRCV_PHYLOOPBACK_NONE = 0,
    ETHTRCV_PHYLOOPBACK_INTERNAL,
    ETHTRCV_PHYLOOPBACK_EXTERNAL,
    ETHTRCV_PHYLOOPBACK_REMOTE
} EthTrcv_PhyLoopbackModeType;

/******************************************************************************
 * PHY Tx Mode Type
 ******************************************************************************/
typedef enum {
    ETHTRCV_PHYTXMODE_NORMAL = 0,
    ETHTRCV_PHYTXMODE_TX_OFF,
    ETHTRCV_PHYTXMODE_SCRAMBLER_OFF,
    ETHTRCV_PHYTXMODE_4B5B_OFF,
    ETHTRCV_PHYTXMODE_TRAINING_OFF
} EthTrcv_PhyTxModeType;

/******************************************************************************
 * Wake-up Mode Type
 ******************************************************************************/
typedef uint8 EthTrcv_WakeupModeType;

#define ETHTRCV_WUM_DISABLE             0x00U
#define ETHTRCV_WUM_ENABLE              0x01U
#define ETHTRCV_WUM_POLLING             0x02U

/******************************************************************************
 * Wake-up Source Type
 ******************************************************************************/
typedef uint8 EthTrcv_WakeupSourceType;

#define ETH_WKSRC_NONE                  0x00U
#define ETH_WKSRC_MASK                  0x01U
#define ETH_WKSRC_POWER_ON              0x02U
#define ETH_WKSRC_WAKEUP_PIN            0x04U
#define ETH_WKSRC_MAGIC_PACKET          0x08U
#define ETH_WKSRC_WAKEUP_FRAME          0x10U
#define ETH_WKSRC_AUTONEG               0x20U

/******************************************************************************
 * Transceiver Index Type
 ******************************************************************************/
typedef uint8 EthTrcv_TrcvIdxType;

/******************************************************************************
 * PHY Register Value Type
 ******************************************************************************/
typedef uint16 EthTrcv_PhyRegValType;

/******************************************************************************
 * PHY Access Return Type
 ******************************************************************************/
typedef enum {
    ETHTRCV_PHY_ACCESS_OK = 0,
    ETHTRCV_PHY_ACCESS_E_NOT_OK,
    ETHTRCV_PHY_ACCESS_E_TIMEOUT,
    ETHTRCV_PHY_ACCESS_E_BUSY
} EthTrcv_PhyAccessResultType;

/******************************************************************************
 * PHY Device Type
 ******************************************************************************/
typedef enum {
    ETHTRCV_PHY_TYPE_GENERIC = 0,
    ETHTRCV_PHY_TYPE_TJA1101,
    ETHTRCV_PHY_TYPE_TJA1102,
    ETHTRCV_PHY_TYPE_DP83825I,
    ETHTRCV_PHY_TYPE_KSZ8081,
    ETHTRCV_PHY_TYPE_CUSTOM
} EthTrcv_PhyType;

/******************************************************************************
 * Transceiver Configuration Type
 ******************************************************************************/
typedef struct {
    EthTrcv_TrcvIdxType trcvIdx;
    uint8 phyAddress;                           /* 5-bit PHY address (0-31) */
    EthTrcv_PhyType phyType;
    boolean enabled;
    boolean wakeupSupported;
    boolean autonegSupported;
    uint32 linkCheckPeriodMs;                   /* Link status polling period */
    uint32 resetTimeoutMs;                      /* PHY reset timeout */
} EthTrcv_TrcvConfigType;

/******************************************************************************
 * Module Configuration Type
 ******************************************************************************/
typedef struct {
    uint8 numTransceivers;
    boolean enableWakeup;
    boolean enableLinkMonitoring;
    uint32 mainFunctionPeriodMs;
    const EthTrcv_TrcvConfigType *trcvConfigs[ETHTRCV_MAX_TRANSCEIVERS];
} EthTrcv_ConfigType;

/******************************************************************************
 * Transceiver Status Type
 ******************************************************************************/
typedef struct {
    EthTrcv_StateType state;
    EthTrcv_ModeType currentMode;
    EthTrcv_LinkStateType linkState;
    EthTrcv_BaudRateType baudRate;
    boolean linkStatusChanged;
    uint32 linkUpTimeMs;
    uint32 linkDownTimeMs;
    uint32 errorCount;
} EthTrcv_TrcvStatusType;

/******************************************************************************
 * External Variables
 ******************************************************************************/
extern const EthTrcv_ConfigType *EthTrcv_ConfigPtr;
extern EthTrcv_TrcvStatusType EthTrcv_TrcvStatus[ETHTRCV_MAX_TRANSCEIVERS];

/******************************************************************************
 * Core API Functions
 ******************************************************************************/

/**
 * @brief Initialize EthTrcv module
 * @param config Pointer to module configuration
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_Init(const EthTrcv_ConfigType *config);

/**
 * @brief Deinitialize EthTrcv module
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_DeInit(void);

/**
 * @brief Set transceiver mode (DOWN/STANDBY/ACTIVE)
 * @param trcvIdx Transceiver index
 * @param mode Target mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_SetTransceiverMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_ModeType mode
);

/**
 * @brief Get current transceiver mode
 * @param trcvIdx Transceiver index
 * @param modePtr Pointer to store current mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_GetTransceiverMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_ModeType *modePtr
);

/**
 * @brief Get link state (DOWN/UP)
 * @param trcvIdx Transceiver index
 * @param linkStatePtr Pointer to store link state
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_GetLinkState(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_LinkStateType *linkStatePtr
);

/**
 * @brief Get current baud rate
 * @param trcvIdx Transceiver index
 * @param baudRatePtr Pointer to store baud rate
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_GetBaudRate(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_BaudRateType *baudRatePtr
);

/**
 * @brief Set PHY test mode
 * @param trcvIdx Transceiver index
 * @param mode Test mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_SetPhyTestMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_PhyTestModeType mode
);

/**
 * @brief Set PHY loopback mode
 * @param trcvIdx Transceiver index
 * @param mode Loopback mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_SetPhyLoopbackMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_PhyLoopbackModeType mode
);

/**
 * @brief Set PHY Tx mode
 * @param trcvIdx Transceiver index
 * @param mode Tx mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_SetPhyTxMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_PhyTxModeType mode
);

/******************************************************************************
 * Wake-up Functions
 ******************************************************************************/

/**
 * @brief Check for wake-up events
 * @param trcvIdx Transceiver index
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_CheckWakeup(EthTrcv_TrcvIdxType trcvIdx);

/**
 * @brief Get transceiver wake-up mode
 * @param trcvIdx Transceiver index
 * @param wakeupModePtr Pointer to store wake-up mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_GetTransceiverWakeupMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_WakeupModeType *wakeupModePtr
);

/**
 * @brief Set transceiver wake-up mode
 * @param trcvIdx Transceiver index
 * @param wakeupMode Wake-up mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_SetTransceiverWakeupMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_WakeupModeType wakeupMode
);

/******************************************************************************
 * Version Information
 ******************************************************************************/

/**
 * @brief Get version information
 * @param versionInfo Pointer to store version info
 */
void EthTrcv_GetVersionInfo(Std_VersionInfoType *versionInfo);

/******************************************************************************
 * Status Functions
 ******************************************************************************/

/**
 * @brief Get transceiver state
 * @param trcvIdx Transceiver index
 * @return Transceiver state
 */
EthTrcv_StateType EthTrcv_GetState(EthTrcv_TrcvIdxType trcvIdx);

/**
 * @brief Check if transceiver is initialized
 * @param trcvIdx Transceiver index
 * @return TRUE if initialized, FALSE otherwise
 */
boolean EthTrcv_IsInitialized(EthTrcv_TrcvIdxType trcvIdx);

/**
 * @brief Check if link is up
 * @param trcvIdx Transceiver index
 * @return TRUE if link is up, FALSE otherwise
 */
boolean EthTrcv_IsLinkUp(EthTrcv_TrcvIdxType trcvIdx);

/******************************************************************************
 * PHY Access Functions
 ******************************************************************************/

/**
 * @brief Read PHY register via MII/MDIO
 * @param trcvIdx Transceiver index
 * @param regAddr Register address
 * @param regValPtr Pointer to store register value
 * @return PHY access result
 */
EthTrcv_PhyAccessResultType EthTrcv_ReadPhyRegister(
    EthTrcv_TrcvIdxType trcvIdx,
    uint8 regAddr,
    EthTrcv_PhyRegValType *regValPtr
);

/**
 * @brief Write PHY register via MII/MDIO
 * @param trcvIdx Transceiver index
 * @param regAddr Register address
 * @param regVal Register value to write
 * @return PHY access result
 */
EthTrcv_PhyAccessResultType EthTrcv_WritePhyRegister(
    EthTrcv_TrcvIdxType trcvIdx,
    uint8 regAddr,
    EthTrcv_PhyRegValType regVal
);

/**
 * @brief Reset PHY
 * @param trcvIdx Transceiver index
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EthTrcv_ResetPhy(EthTrcv_TrcvIdxType trcvIdx);

/******************************************************************************
 * Callback Functions (called by Eth Driver or interrupt)
 ******************************************************************************/

/**
 * @brief Link state change indication callback
 * @param trcvIdx Transceiver index
 * @param linkState New link state
 */
void EthTrcv_Cbk_LinkChgIndication(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_LinkStateType linkState
);

/******************************************************************************
 * Main Function
 ******************************************************************************/

/**
 * @brief Main function for cyclic processing (link monitoring)
 * Called periodically by the OS or scheduler
 */
void EthTrcv_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* ETHTRCV_H */
