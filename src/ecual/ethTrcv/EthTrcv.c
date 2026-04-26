/******************************************************************************
 * @file    EthTrcv.c
 * @brief   Ethernet Transceiver (EthTrcv) Implementation - AUTOSAR R22-11
 *
 * This module implements the EthTrcv API for controlling Ethernet PHY
 * transceivers, with support for TJA1101 100BASE-T1 Automotive Ethernet PHY.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x35 (EthTrcv)
 * ASIL-B Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ecual/ethTrcv/EthTrcv.h"
#include "ecual/ethTrcv/EthTrcv_Cfg.h"
#include <string.h>

/******************************************************************************
 * Module Local Definitions
 ******************************************************************************/

/* Module state */
#define ETHTRCV_STATE_MODULE_UNINIT     0x00U
#define ETHTRCV_STATE_MODULE_INIT       0x01U

/* Internal function return values */
#define ETHTRCV_RET_OK                  0x00U
#define ETHTRCV_RET_NOT_OK              0x01U
#define ETHTRCV_RET_TIMEOUT             0x02U

/******************************************************************************
 * Module Local Variables
 ******************************************************************************/

/* Module initialized flag */
static uint8 EthTrcv_ModuleState = ETHTRCV_STATE_MODULE_UNINIT;

/* Configuration pointer */
const EthTrcv_ConfigType *EthTrcv_ConfigPtr = NULL_PTR;

/* Transceiver status */
EthTrcv_TrcvStatusType EthTrcv_TrcvStatus[ETHTRCV_MAX_TRANSCEIVERS];

/* Link state history for debouncing */
static EthTrcv_LinkStateType EthTrcv_LinkStateHistory[ETHTRCV_MAX_TRANSCEIVERS];
static uint8 EthTrcv_LinkDebounceCount[ETHTRCV_MAX_TRANSCEIVERS];

/* PHY register cache */
static EthTrcv_PhyRegValType EthTrcv_PhyRegCache[ETHTRCV_MAX_TRANSCEIVERS][ETHTRCV_MAX_PHY_REGISTERS];

/******************************************************************************
 * Internal Function Prototypes
 ******************************************************************************/

static Std_ReturnType EthTrcv_InternalReadPhyRegister(
    EthTrcv_TrcvIdxType trcvIdx,
    uint8 regAddr,
    EthTrcv_PhyRegValType *regValPtr
);

static Std_ReturnType EthTrcv_InternalWritePhyRegister(
    EthTrcv_TrcvIdxType trcvIdx,
    uint8 regAddr,
    EthTrcv_PhyRegValType regVal
);

static Std_ReturnType EthTrcv_CheckLinkStatus(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_LinkStateType *linkStatePtr
);

static void EthTrcv_UpdateLinkState(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_LinkStateType newLinkState
);

static Std_ReturnType EthTrcv_PhyInit(EthTrcv_TrcvIdxType trcvIdx);

static boolean EthTrcv_IsTrcvIdxValid(EthTrcv_TrcvIdxType trcvIdx);

static boolean EthTrcv_IsPointerValid(const void *ptr);

/******************************************************************************
 * DET Helper Functions
 ******************************************************************************/

#if (ETHTRCV_DEV_ERROR_DETECT == STD_ON)
/**
 * @brief Report DET error
 */
static void EthTrcv_ReportDetError(uint8 apiId, uint8 errorId)
{
    (void)Det_ReportError(ETHTRCV_MODULE_ID, 0U, apiId, errorId);
}
#else
#define EthTrcv_ReportDetError(apiId, errorId) ((void)0)
#endif

/******************************************************************************
 * Validation Functions
 ******************************************************************************/

/**
 * @brief Check if transceiver index is valid
 */
static boolean EthTrcv_IsTrcvIdxValid(EthTrcv_TrcvIdxType trcvIdx)
{
    boolean isValid = FALSE;

    if (trcvIdx < ETHTRCV_MAX_TRANSCEIVERS) {
        if ((EthTrcv_ConfigPtr != NULL_PTR) &&
            (trcvIdx < EthTrcv_ConfigPtr->numTransceivers) &&
            (EthTrcv_ConfigPtr->trcvConfigs[trcvIdx] != NULL_PTR)) {
            isValid = EthTrcv_ConfigPtr->trcvConfigs[trcvIdx]->enabled;
        }
    }

    return isValid;
}

/**
 * @brief Check if pointer is valid (non-NULL)
 */
static boolean EthTrcv_IsPointerValid(const void *ptr)
{
    return (ptr != NULL_PTR) ? TRUE : FALSE;
}

/******************************************************************************
 * PHY Access Functions
 ******************************************************************************/

/**
 * @brief Internal function to read PHY register via MII/MDIO
 *
 * This is a platform-specific implementation. In a real system, this would
 * interface with the MAC's MII/MDIO controller.
 */
static Std_ReturnType EthTrcv_InternalReadPhyRegister(
    EthTrcv_TrcvIdxType trcvIdx,
    uint8 regAddr,
    EthTrcv_PhyRegValType *regValPtr
)
{
    Std_ReturnType retVal = E_NOT_OK;

    /* Parameter validation */
    if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_TRCV_IDX);
    }
    else if (!EthTrcv_IsPointerValid(regValPtr)) {
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_POINTER);
    }
    else if (regAddr < ETHTRCV_MAX_PHY_REGISTERS) {
        /* Platform-specific: Read PHY register via MII/MDIO */
        /* This is a stub - actual implementation would use MAC MDIO interface */
        
        /* For simulation/testing, return cached value */
        *regValPtr = EthTrcv_PhyRegCache[trcvIdx][regAddr];
        
        /* Simulate successful read */
        retVal = E_OK;
    }
    else {
        /* Register address out of range */
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_PARAMETER);
    }

    return retVal;
}

/**
 * @brief Internal function to write PHY register via MII/MDIO
 */
static Std_ReturnType EthTrcv_InternalWritePhyRegister(
    EthTrcv_TrcvIdxType trcvIdx,
    uint8 regAddr,
    EthTrcv_PhyRegValType regVal
)
{
    Std_ReturnType retVal = E_NOT_OK;

    /* Parameter validation */
    if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_TRCV_IDX);
    }
    else if (regAddr < ETHTRCV_MAX_PHY_REGISTERS) {
        /* Platform-specific: Write PHY register via MII/MDIO */
        /* This is a stub - actual implementation would use MAC MDIO interface */
        
        /* Update cache */
        EthTrcv_PhyRegCache[trcvIdx][regAddr] = regVal;
        
        /* Simulate successful write */
        retVal = E_OK;
    }
    else {
        /* Register address out of range */
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_PARAMETER);
    }

    return retVal;
}

/**
 * @brief Initialize PHY for specific transceiver
 */
static Std_ReturnType EthTrcv_PhyInit(EthTrcv_TrcvIdxType trcvIdx)
{
    Std_ReturnType retVal = E_OK;
    const EthTrcv_TrcvConfigType *trcvConfig;
    EthTrcv_PhyRegValType regVal;

    trcvConfig = EthTrcv_ConfigPtr->trcvConfigs[trcvIdx];

    /* Reset PHY */
    retVal = EthTrcv_ResetPhy(trcvIdx);
    
    if (retVal == E_OK) {
        /* Verify PHY ID (TJA1101 specific) */
        if (trcvConfig->phyType == ETHTRCV_PHY_TYPE_TJA1101) {
            (void)EthTrcv_InternalReadPhyRegister(trcvIdx, ETHTRCV_PHY_REG_ID1, &regVal);
            if (regVal != ETHTRCV_CFG_TJA1101_ID1) {
                /* PHY ID mismatch */
                retVal = E_NOT_OK;
            }
        }

        if (retVal == E_OK) {
            /* Configure Extended Control Register */
            if (trcvConfig->phyType == ETHTRCV_PHY_TYPE_TJA1101) {
                regVal = ETHTRCV_CFG_TJA1101_ECTRL_DEFAULT;
                retVal = EthTrcv_InternalWritePhyRegister(trcvIdx, 
                                                         ETHTRCV_PHY_REG_EXTENDED_CTRL, 
                                                         regVal);
            }

            if (retVal == E_OK) {
                /* Configure Configuration Register 1 */
                if (trcvConfig->phyType == ETHTRCV_PHY_TYPE_TJA1101) {
                    regVal = ETHTRCV_CFG_TJA1101_CONFIG1_DEFAULT;
                    retVal = EthTrcv_InternalWritePhyRegister(trcvIdx,
                                                             ETHTRCV_PHY_REG_CONFIG1,
                                                             regVal);
                }

                /* Configure Interrupt Enable Register */
                if (retVal == E_OK) {
                    regVal = ETHTRCV_CFG_TJA1101_INT_ENABLE_DEFAULT;
                    retVal = EthTrcv_InternalWritePhyRegister(trcvIdx,
                                                             ETHTRCV_PHY_REG_INT_ENABLE,
                                                             regVal);
                }
            }
        }
    }

    return retVal;
}

/******************************************************************************
 * Link State Functions
 ******************************************************************************/

/**
 * @brief Check current link status from PHY
 */
static Std_ReturnType EthTrcv_CheckLinkStatus(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_LinkStateType *linkStatePtr
)
{
    Std_ReturnType retVal = E_NOT_OK;
    EthTrcv_PhyRegValType regVal;

    if (EthTrcv_IsPointerValid(linkStatePtr)) {
        /* Read Basic Status Register */
        retVal = EthTrcv_InternalReadPhyRegister(trcvIdx, 
                                                 ETHTRCV_PHY_REG_BASIC_STATUS, 
                                                 &regVal);
        
        if (retVal == E_OK) {
            /* Check link status bit (bit 2) */
            if ((regVal & ETHTRCV_PHY_BSTATUS_LINK_STATUS) != 0U) {
                *linkStatePtr = ETHTRCV_LINK_UP;
            }
            else {
                *linkStatePtr = ETHTRCV_LINK_DOWN;
            }
        }
    }

    return retVal;
}

/**
 * @brief Update link state with debouncing
 */
static void EthTrcv_UpdateLinkState(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_LinkStateType newLinkState
)
{
    if (newLinkState == EthTrcv_LinkStateHistory[trcvIdx]) {
        /* Same state, increment debounce counter */
        if (EthTrcv_LinkDebounceCount[trcvIdx] < 0xFFU) {
            EthTrcv_LinkDebounceCount[trcvIdx]++;
        }
    }
    else {
        /* State changed, reset counter */
        EthTrcv_LinkStateHistory[trcvIdx] = newLinkState;
        EthTrcv_LinkDebounceCount[trcvIdx] = 0U;
    }

    /* Check if debounce threshold reached */
    if (EthTrcv_LinkDebounceCount[trcvIdx] >= ETHTRCV_CFG_LINK_UP_THRESHOLD) {
        if (newLinkState != EthTrcv_TrcvStatus[trcvIdx].linkState) {
            /* Link state has changed */
            EthTrcv_TrcvStatus[trcvIdx].linkState = newLinkState;
            EthTrcv_TrcvStatus[trcvIdx].linkStatusChanged = TRUE;

            /* Update timestamps */
            if (newLinkState == ETHTRCV_LINK_UP) {
                /* Link went up */
            }
            else {
                /* Link went down */
            }

            /* Notify upper layer if callback is configured */
            EthTrcv_Cbk_LinkChgIndication(trcvIdx, newLinkState);
        }
    }
}

/******************************************************************************
 * Public API Functions - Core
 ******************************************************************************/

/**
 * @brief Initialize EthTrcv module
 */
Std_ReturnType EthTrcv_Init(const EthTrcv_ConfigType *config)
{
    Std_ReturnType retVal = E_OK;
    uint8 trcvIdx;

    /* Check if already initialized */
    if (EthTrcv_ModuleState == ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_NOT_SUPPORTED);
    }
    /* Parameter validation */
    else if (!EthTrcv_IsPointerValid(config)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_POINTER);
    }
    else if (config->numTransceivers == 0U) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_PARAMETER);
    }
    else if (config->numTransceivers > ETHTRCV_MAX_TRANSCEIVERS) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_PARAMETER);
    }
    else {
        /* Store configuration pointer */
        EthTrcv_ConfigPtr = config;

        /* Initialize all transceivers */
        for (trcvIdx = 0U; trcvIdx < config->numTransceivers; trcvIdx++) {
            if (config->trcvConfigs[trcvIdx] != NULL_PTR) {
                if (config->trcvConfigs[trcvIdx]->enabled) {
                    /* Initialize status */
                    EthTrcv_TrcvStatus[trcvIdx].state = ETHTRCV_STATE_INIT;
                    EthTrcv_TrcvStatus[trcvIdx].currentMode = ETHTRCV_MODE_DOWN;
                    EthTrcv_TrcvStatus[trcvIdx].linkState = ETHTRCV_LINK_DOWN;
                    EthTrcv_TrcvStatus[trcvIdx].baudRate = ETHTRCV_BAUD_RATE_100MBIT;
                    EthTrcv_TrcvStatus[trcvIdx].linkStatusChanged = FALSE;
                    EthTrcv_TrcvStatus[trcvIdx].linkUpTimeMs = 0U;
                    EthTrcv_TrcvStatus[trcvIdx].linkDownTimeMs = 0U;
                    EthTrcv_TrcvStatus[trcvIdx].errorCount = 0U;

                    /* Initialize link debounce */
                    EthTrcv_LinkStateHistory[trcvIdx] = ETHTRCV_LINK_DOWN;
                    EthTrcv_LinkDebounceCount[trcvIdx] = 0U;

                    /* Initialize PHY cache */
                    (void)memset(&EthTrcv_PhyRegCache[trcvIdx][0], 0, 
                                sizeof(EthTrcv_PhyRegValType) * ETHTRCV_MAX_PHY_REGISTERS);

                    /* Initialize PHY hardware */
                    if (EthTrcv_PhyInit(trcvIdx) != E_OK) {
                        EthTrcv_TrcvStatus[trcvIdx].state = ETHTRCV_STATE_DOWN;
                        EthTrcv_TrcvStatus[trcvIdx].errorCount++;
                        retVal = E_NOT_OK;
                    }
                }
            }
        }

        if (retVal == E_OK) {
            /* Mark module as initialized */
            EthTrcv_ModuleState = ETHTRCV_STATE_MODULE_INIT;
        }
    }

    return retVal;
}

/**
 * @brief Deinitialize EthTrcv module
 */
Std_ReturnType EthTrcv_DeInit(void)
{
    Std_ReturnType retVal = E_OK;
    uint8 trcvIdx;

    /* Check if initialized */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_DEINIT, ETHTRCV_E_NOT_INITIALIZED);
    }
    else {
        /* Put all transceivers in DOWN mode */
        if (EthTrcv_ConfigPtr != NULL_PTR) {
            for (trcvIdx = 0U; trcvIdx < EthTrcv_ConfigPtr->numTransceivers; trcvIdx++) {
                if (EthTrcv_TrcvStatus[trcvIdx].state != ETHTRCV_STATE_UNINIT) {
                    (void)EthTrcv_SetTransceiverMode(trcvIdx, ETHTRCV_MODE_DOWN);
                    EthTrcv_TrcvStatus[trcvIdx].state = ETHTRCV_STATE_UNINIT;
                }
            }
        }

        /* Clear configuration pointer */
        EthTrcv_ConfigPtr = NULL_PTR;

        /* Mark module as uninitialized */
        EthTrcv_ModuleState = ETHTRCV_STATE_MODULE_UNINIT;
    }

    return retVal;
}

/**
 * @brief Set transceiver mode
 */
Std_ReturnType EthTrcv_SetTransceiverMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_ModeType mode
)
{
    Std_ReturnType retVal = E_OK;
    EthTrcv_PhyRegValType regVal;

    /* Check module state */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_SETTRANSCEIVERMODE, 
                              ETHTRCV_E_NOT_INITIALIZED);
    }
    /* Validate transceiver index */
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_SETTRANSCEIVERMODE, 
                              ETHTRCV_E_INVALID_TRCV_IDX);
    }
    /* Validate mode */
    else if (!ETHTRCV_MODE_IS_VALID(mode)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_SETTRANSCEIVERMODE, 
                              ETHTRCV_E_INVALID_MODE);
    }
    else {
        /* Set PHY mode based on target mode */
        switch (mode) {
            case ETHTRCV_MODE_DOWN:
                /* Power down the PHY */
                retVal = EthTrcv_InternalReadPhyRegister(trcvIdx,
                                                         ETHTRCV_PHY_REG_BASIC_CTRL,
                                                         &regVal);
                if (retVal == E_OK) {
                    regVal |= ETHTRCV_PHY_BCTRL_POWER_DOWN;
                    retVal = EthTrcv_InternalWritePhyRegister(trcvIdx,
                                                              ETHTRCV_PHY_REG_BASIC_CTRL,
                                                              regVal);
                }
                break;

            case ETHTRCV_MODE_STANDBY:
                /* TJA1101 specific: Set sleep/standby mode */
                if (EthTrcv_ConfigPtr->trcvConfigs[trcvIdx]->phyType == 
                    ETHTRCV_PHY_TYPE_TJA1101) {
                    retVal = EthTrcv_InternalReadPhyRegister(trcvIdx,
                                                             ETHTRCV_PHY_REG_EXTENDED_CTRL,
                                                             &regVal);
                    if (retVal == E_OK) {
                        regVal &= ~ETHTRCV_PHY_ECTRL_POWER_MODE_MASK;
                        regVal |= ETHTRCV_PHY_ECTRL_POWER_MODE_STANDBY;
                        retVal = EthTrcv_InternalWritePhyRegister(trcvIdx,
                                                                  ETHTRCV_PHY_REG_EXTENDED_CTRL,
                                                                  regVal);
                    }
                }
                break;

            case ETHTRCV_MODE_ACTIVE:
                /* Power up the PHY and enable normal operation */
                retVal = EthTrcv_InternalReadPhyRegister(trcvIdx,
                                                         ETHTRCV_PHY_REG_BASIC_CTRL,
                                                         &regVal);
                if (retVal == E_OK) {
                    /* Clear power down bit */
                    regVal &= ~ETHTRCV_PHY_BCTRL_POWER_DOWN;
                    retVal = EthTrcv_InternalWritePhyRegister(trcvIdx,
                                                              ETHTRCV_PHY_REG_BASIC_CTRL,
                                                              regVal);
                }

                if (retVal == E_OK) {
                    /* TJA1101 specific: Set normal power mode */
                    if (EthTrcv_ConfigPtr->trcvConfigs[trcvIdx]->phyType == 
                        ETHTRCV_PHY_TYPE_TJA1101) {
                        retVal = EthTrcv_InternalReadPhyRegister(trcvIdx,
                                                                 ETHTRCV_PHY_REG_EXTENDED_CTRL,
                                                                 &regVal);
                        if (retVal == E_OK) {
                            regVal &= ~ETHTRCV_PHY_ECTRL_POWER_MODE_MASK;
                            regVal |= ETHTRCV_PHY_ECTRL_POWER_MODE_NORMAL;
                            retVal = EthTrcv_InternalWritePhyRegister(trcvIdx,
                                                                      ETHTRCV_PHY_REG_EXTENDED_CTRL,
                                                                      regVal);
                        }
                    }
                }
                break;

            default:
                retVal = E_NOT_OK;
                break;
        }

        if (retVal == E_OK) {
            /* Update status */
            EthTrcv_TrcvStatus[trcvIdx].currentMode = mode;
            
            if (mode == ETHTRCV_MODE_ACTIVE) {
                EthTrcv_TrcvStatus[trcvIdx].state = ETHTRCV_STATE_ACTIVE;
            }
            else if (mode == ETHTRCV_MODE_DOWN) {
                EthTrcv_TrcvStatus[trcvIdx].state = ETHTRCV_STATE_DOWN;
            }
            else {
                EthTrcv_TrcvStatus[trcvIdx].state = ETHTRCV_STATE_STANDBY;
            }
        }
    }

    return retVal;
}

/**
 * @brief Get current transceiver mode
 */
Std_ReturnType EthTrcv_GetTransceiverMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_ModeType *modePtr
)
{
    Std_ReturnType retVal = E_OK;

    /* Check module state */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETTRANSCEIVERMODE, 
                              ETHTRCV_E_NOT_INITIALIZED);
    }
    /* Validate transceiver index */
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETTRANSCEIVERMODE, 
                              ETHTRCV_E_INVALID_TRCV_IDX);
    }
    /* Validate pointer */
    else if (!EthTrcv_IsPointerValid(modePtr)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETTRANSCEIVERMODE, 
                              ETHTRCV_E_INVALID_POINTER);
    }
    else {
        *modePtr = EthTrcv_TrcvStatus[trcvIdx].currentMode;
    }

    return retVal;
}

/**
 * @brief Get link state
 */
Std_ReturnType EthTrcv_GetLinkState(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_LinkStateType *linkStatePtr
)
{
    Std_ReturnType retVal = E_OK;

    /* Check module state */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETLINKSTATE, 
                              ETHTRCV_E_NOT_INITIALIZED);
    }
    /* Validate transceiver index */
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETLINKSTATE, 
                              ETHTRCV_E_INVALID_TRCV_IDX);
    }
    /* Validate pointer */
    else if (!EthTrcv_IsPointerValid(linkStatePtr)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETLINKSTATE, 
                              ETHTRCV_E_INVALID_POINTER);
    }
    else {
        /* Return cached link state */
        *linkStatePtr = EthTrcv_TrcvStatus[trcvIdx].linkState;
        
        /* Clear change indication */
        EthTrcv_TrcvStatus[trcvIdx].linkStatusChanged = FALSE;
    }

    return retVal;
}

/**
 * @brief Get baud rate
 */
Std_ReturnType EthTrcv_GetBaudRate(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_BaudRateType *baudRatePtr
)
{
    Std_ReturnType retVal = E_OK;

    /* Check module state */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETBAUDRATE, 
                              ETHTRCV_E_NOT_INITIALIZED);
    }
    /* Validate transceiver index */
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETBAUDRATE, 
                              ETHTRCV_E_INVALID_TRCV_IDX);
    }
    /* Validate pointer */
    else if (!EthTrcv_IsPointerValid(baudRatePtr)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETBAUDRATE, 
                              ETHTRCV_E_INVALID_POINTER);
    }
    else {
        /* Return configured baud rate */
        *baudRatePtr = EthTrcv_TrcvStatus[trcvIdx].baudRate;
    }

    return retVal;
}

/******************************************************************************
 * Public API Functions - PHY Test/Loopback
 ******************************************************************************/

/**
 * @brief Set PHY test mode
 */
Std_ReturnType EthTrcv_SetPhyTestMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_PhyTestModeType mode
)
{
    Std_ReturnType retVal = E_OK;

#if (ETHTRCV_PHY_TEST_MODE_SUPPORT == STD_ON)
    /* Check module state */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_SETPHYTESTMODE, 
                              ETHTRCV_E_NOT_INITIALIZED);
    }
    /* Validate transceiver index */
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_SETPHYTESTMODE, 
                              ETHTRCV_E_INVALID_TRCV_IDX);
    }
    else {
        /* Platform-specific: Set PHY test mode */
        /* This is typically done via vendor-specific registers */
        /* For TJA1101, test modes are controlled in specific registers */
        
        /* Stub implementation - would need platform-specific code */
        (void)mode; /* Prevent unused parameter warning */
        retVal = E_OK;
    }
#else
    (void)trcvIdx;
    (void)mode;
    retVal = E_NOT_OK;
#endif

    return retVal;
}

/**
 * @brief Set PHY loopback mode
 */
Std_ReturnType EthTrcv_SetPhyLoopbackMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_PhyLoopbackModeType mode
)
{
    Std_ReturnType retVal = E_OK;
    EthTrcv_PhyRegValType regVal;

#if (ETHTRCV_PHY_LOOPBACK_SUPPORT == STD_ON)
    /* Check module state */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_SETPHYLOOPBACKMODE, 
                              ETHTRCV_E_NOT_INITIALIZED);
    }
    /* Validate transceiver index */
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_SETPHYLOOPBACKMODE, 
                              ETHTRCV_E_INVALID_TRCV_IDX);
    }
    else {
        /* Read Basic Control Register */
        retVal = EthTrcv_InternalReadPhyRegister(trcvIdx,
                                                 ETHTRCV_PHY_REG_BASIC_CTRL,
                                                 &regVal);
        
        if (retVal == E_OK) {
            /* Configure loopback mode */
            switch (mode) {
                case ETHTRCV_PHYLOOPBACK_NONE:
                    /* Disable loopback */
                    regVal &= ~ETHTRCV_PHY_BCTRL_LOOPBACK;
                    break;
                    
                case ETHTRCV_PHYLOOPBACK_INTERNAL:
                    /* Enable loopback in Basic Control Register */
                    regVal |= ETHTRCV_PHY_BCTRL_LOOPBACK;
                    break;
                    
                default:
                    /* External and remote loopback not supported in basic register */
                    retVal = E_NOT_OK;
                    break;
            }

            if (retVal == E_OK) {
                retVal = EthTrcv_InternalWritePhyRegister(trcvIdx,
                                                          ETHTRCV_PHY_REG_BASIC_CTRL,
                                                          regVal);
            }
        }
    }
#else
    (void)trcvIdx;
    (void)mode;
    retVal = E_NOT_OK;
#endif

    return retVal;
}

/**
 * @brief Set PHY Tx mode
 */
Std_ReturnType EthTrcv_SetPhyTxMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_PhyTxModeType mode
)
{
    Std_ReturnType retVal = E_OK;

    /* Check module state */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_SETPHYTXMODE, 
                              ETHTRCV_E_NOT_INITIALIZED);
    }
    /* Validate transceiver index */
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_SETPHYTXMODE, 
                              ETHTRCV_E_INVALID_TRCV_IDX);
    }
    else {
        /* Platform-specific: Set PHY Tx mode */
        /* This controls transmitter settings in vendor-specific registers */
        
        /* Stub implementation - would need platform-specific code */
        (void)mode; /* Prevent unused parameter warning */
        retVal = E_OK;
    }

    return retVal;
}

/******************************************************************************
 * Public API Functions - Wake-up
 ******************************************************************************/

/**
 * @brief Check for wake-up events
 */
Std_ReturnType EthTrcv_CheckWakeup(EthTrcv_TrcvIdxType trcvIdx)
{
    Std_ReturnType retVal = E_OK;

#if (ETHTRCV_WAKEUP_SUPPORT == STD_ON)
    /* Check module state */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_CHECKWAKEUP, 
                              ETHTRCV_E_NOT_INITIALIZED);
    }
    /* Validate transceiver index */
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_CHECKWAKEUP, 
                              ETHTRCV_E_INVALID_TRCV_IDX);
    }
    else {
        /* Check wake-up status from PHY */
        /* TJA1101 has specific wake-up detection in status registers */
        
        /* Stub implementation */
        retVal = E_OK;
    }
#else
    (void)trcvIdx;
    retVal = E_NOT_OK;
#endif

    return retVal;
}

/**
 * @brief Get transceiver wake-up mode
 */
Std_ReturnType EthTrcv_GetTransceiverWakeupMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_WakeupModeType *wakeupModePtr
)
{
    Std_ReturnType retVal = E_OK;

#if (ETHTRCV_WAKEUP_SUPPORT == STD_ON)
    /* Check module state */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETTRANSCEIVERWAKEUPMODE, 
                              ETHTRCV_E_NOT_INITIALIZED);
    }
    /* Validate transceiver index */
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETTRANSCEIVERWAKEUPMODE, 
                              ETHTRCV_E_INVALID_TRCV_IDX);
    }
    /* Validate pointer */
    else if (!EthTrcv_IsPointerValid(wakeupModePtr)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_GETTRANSCEIVERWAKEUPMODE, 
                              ETHTRCV_E_INVALID_POINTER);
    }
    else {
        /* Return current wake-up mode */
        /* Default to disabled if not configured */
        *wakeupModePtr = ETHTRCV_WUM_DISABLE;
    }
#else
    (void)trcvIdx;
    (void)wakeupModePtr;
    retVal = E_NOT_OK;
#endif

    return retVal;
}

/**
 * @brief Set transceiver wake-up mode
 */
Std_ReturnType EthTrcv_SetTransceiverWakeupMode(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_WakeupModeType wakeupMode
)
{
    Std_ReturnType retVal = E_OK;

#if (ETHTRCV_WAKEUP_SUPPORT == STD_ON)
    /* Check module state */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_SETTRANSCEIVERWAKEUPMODE, 
                              ETHTRCV_E_NOT_INITIALIZED);
    }
    /* Validate transceiver index */
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_SETTRANSCEIVERWAKEUPMODE, 
                              ETHTRCV_E_INVALID_TRCV_IDX);
    }
    else {
        /* Configure wake-up mode in PHY */
        /* This involves setting up interrupt masks and wake-up filters */
        
        (void)wakeupMode;
        retVal = E_OK;
    }
#else
    (void)trcvIdx;
    (void)wakeupMode;
    retVal = E_NOT_OK;
#endif

    return retVal;
}

/******************************************************************************
 * Public API Functions - Version Info
 ******************************************************************************/

/**
 * @brief Get version information
 */
void EthTrcv_GetVersionInfo(Std_VersionInfoType *versionInfo)
{
#if (ETHTRCV_VERSION_INFO_API == STD_ON)
    if (versionInfo != NULL_PTR) {
        versionInfo->vendorID = ETHTRCV_VENDOR_ID;
        versionInfo->moduleID = ETHTRCV_MODULE_ID;
        versionInfo->sw_major_version = ETHTRCV_SW_MAJOR_VERSION;
        versionInfo->sw_minor_version = ETHTRCV_SW_MINOR_VERSION;
        versionInfo->sw_patch_version = ETHTRCV_SW_PATCH_VERSION;
    }
#else
    (void)versionInfo;
#endif
}

/******************************************************************************
 * Public API Functions - Status
 ******************************************************************************/

/**
 * @brief Get transceiver state
 */
EthTrcv_StateType EthTrcv_GetState(EthTrcv_TrcvIdxType trcvIdx)
{
    EthTrcv_StateType state = ETHTRCV_STATE_UNINIT;

    if ((EthTrcv_ModuleState == ETHTRCV_STATE_MODULE_INIT) &&
        (trcvIdx < ETHTRCV_MAX_TRANSCEIVERS)) {
        state = EthTrcv_TrcvStatus[trcvIdx].state;
    }

    return state;
}

/**
 * @brief Check if transceiver is initialized
 */
boolean EthTrcv_IsInitialized(EthTrcv_TrcvIdxType trcvIdx)
{
    boolean initialized = FALSE;

    if ((EthTrcv_ModuleState == ETHTRCV_STATE_MODULE_INIT) &&
        (trcvIdx < ETHTRCV_MAX_TRANSCEIVERS)) {
        initialized = (EthTrcv_TrcvStatus[trcvIdx].state != ETHTRCV_STATE_UNINIT) ? 
                      TRUE : FALSE;
    }

    return initialized;
}

/**
 * @brief Check if link is up
 */
boolean EthTrcv_IsLinkUp(EthTrcv_TrcvIdxType trcvIdx)
{
    boolean linkUp = FALSE;

    if ((EthTrcv_ModuleState == ETHTRCV_STATE_MODULE_INIT) &&
        (trcvIdx < ETHTRCV_MAX_TRANSCEIVERS)) {
        linkUp = (EthTrcv_TrcvStatus[trcvIdx].linkState == ETHTRCV_LINK_UP) ? 
                 TRUE : FALSE;
    }

    return linkUp;
}

/******************************************************************************
 * Public API Functions - PHY Register Access
 ******************************************************************************/

/**
 * @brief Read PHY register via MII/MDIO
 */
EthTrcv_PhyAccessResultType EthTrcv_ReadPhyRegister(
    EthTrcv_TrcvIdxType trcvIdx,
    uint8 regAddr,
    EthTrcv_PhyRegValType *regValPtr
)
{
    EthTrcv_PhyAccessResultType result = ETHTRCV_PHY_ACCESS_E_NOT_OK;

#if (ETHTRCV_PHY_MII_MDIO_SUPPORT == STD_ON)
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_NOT_INITIALIZED);
    }
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_TRCV_IDX);
    }
    else if (!EthTrcv_IsPointerValid(regValPtr)) {
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_POINTER);
    }
    else if (regAddr >= ETHTRCV_MAX_PHY_REGISTERS) {
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_PARAMETER);
    }
    else {
        if (EthTrcv_InternalReadPhyRegister(trcvIdx, regAddr, regValPtr) == E_OK) {
            result = ETHTRCV_PHY_ACCESS_OK;
        }
    }
#else
    (void)trcvIdx;
    (void)regAddr;
    (void)regValPtr;
#endif

    return result;
}

/**
 * @brief Write PHY register via MII/MDIO
 */
EthTrcv_PhyAccessResultType EthTrcv_WritePhyRegister(
    EthTrcv_TrcvIdxType trcvIdx,
    uint8 regAddr,
    EthTrcv_PhyRegValType regVal
)
{
    EthTrcv_PhyAccessResultType result = ETHTRCV_PHY_ACCESS_E_NOT_OK;

#if (ETHTRCV_PHY_MII_MDIO_SUPPORT == STD_ON)
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_NOT_INITIALIZED);
    }
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_TRCV_IDX);
    }
    else if (regAddr >= ETHTRCV_MAX_PHY_REGISTERS) {
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_PARAMETER);
    }
    else {
        if (EthTrcv_InternalWritePhyRegister(trcvIdx, regAddr, regVal) == E_OK) {
            result = ETHTRCV_PHY_ACCESS_OK;
        }
    }
#else
    (void)trcvIdx;
    (void)regAddr;
    (void)regVal;
#endif

    return result;
}

/**
 * @brief Reset PHY
 */
Std_ReturnType EthTrcv_ResetPhy(EthTrcv_TrcvIdxType trcvIdx)
{
    Std_ReturnType retVal = E_OK;
    EthTrcv_PhyRegValType regVal;

    /* Check module state */
    if (EthTrcv_ModuleState != ETHTRCV_STATE_MODULE_INIT) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_NOT_INITIALIZED);
    }
    /* Validate transceiver index */
    else if (!EthTrcv_IsTrcvIdxValid(trcvIdx)) {
        retVal = E_NOT_OK;
        EthTrcv_ReportDetError(ETHTRCV_SID_INIT, ETHTRCV_E_INVALID_TRCV_IDX);
    }
    else {
        /* Read Basic Control Register */
        retVal = EthTrcv_InternalReadPhyRegister(trcvIdx,
                                                 ETHTRCV_PHY_REG_BASIC_CTRL,
                                                 &regVal);
        
        if (retVal == E_OK) {
            /* Set reset bit */
            regVal |= ETHTRCV_PHY_BCTRL_RESET;
            retVal = EthTrcv_InternalWritePhyRegister(trcvIdx,
                                                      ETHTRCV_PHY_REG_BASIC_CTRL,
                                                      regVal);
            
            /* Wait for reset to complete */
            if (retVal == E_OK) {
                /* In a real implementation, poll the reset bit until it clears */
                /* For now, simulate with a delay or assume immediate completion */
                regVal &= ~ETHTRCV_PHY_BCTRL_RESET;
                EthTrcv_PhyRegCache[trcvIdx][ETHTRCV_PHY_REG_BASIC_CTRL] = regVal;
            }
        }
    }

    return retVal;
}

/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/**
 * @brief Link state change indication callback
 */
void EthTrcv_Cbk_LinkChgIndication(
    EthTrcv_TrcvIdxType trcvIdx,
    EthTrcv_LinkStateType linkState
)
{
    /* This callback is called when the link state changes */
    /* Upper layers (e.g., EthIf) can register to receive this notification */
    
    /* For now, this is a stub that can be extended with proper callback handling */
    (void)trcvIdx;
    (void)linkState;
}

/******************************************************************************
 * Main Function
 ******************************************************************************/

/**
 * @brief Main function for cyclic processing (link monitoring)
 */
void EthTrcv_MainFunction(void)
{
    uint8 trcvIdx;
    EthTrcv_LinkStateType currentLinkState;

    /* Check if module is initialized */
    if (EthTrcv_ModuleState == ETHTRCV_STATE_MODULE_INIT) {
        /* Process all configured transceivers */
        for (trcvIdx = 0U; trcvIdx < EthTrcv_ConfigPtr->numTransceivers; trcvIdx++) {
            if (EthTrcv_TrcvStatus[trcvIdx].state != ETHTRCV_STATE_UNINIT) {
                
#if (ETHTRCV_LINK_MONITORING_SUPPORT == STD_ON)
                /* Check link status if in ACTIVE mode */
                if (EthTrcv_TrcvStatus[trcvIdx].currentMode == ETHTRCV_MODE_ACTIVE) {
                    if (EthTrcv_CheckLinkStatus(trcvIdx, &currentLinkState) == E_OK) {
                        EthTrcv_UpdateLinkState(trcvIdx, currentLinkState);
                    }
                }
#endif

#if (ETHTRCV_WAKEUP_SUPPORT == STD_ON)
                /* Check for wake-up events */
                if (EthTrcv_TrcvStatus[trcvIdx].currentMode == ETHTRCV_MODE_STANDBY) {
                    (void)EthTrcv_CheckWakeup(trcvIdx);
                }
#endif

                /* Update statistics */
                if (EthTrcv_TrcvStatus[trcvIdx].linkState == ETHTRCV_LINK_UP) {
                    EthTrcv_TrcvStatus[trcvIdx].linkUpTimeMs += 
                        EthTrcv_ConfigPtr->mainFunctionPeriodMs;
                }
                else {
                    EthTrcv_TrcvStatus[trcvIdx].linkDownTimeMs += 
                        EthTrcv_ConfigPtr->mainFunctionPeriodMs;
                }
            }
        }
    }
}
