/**
 * @file Eth.c
 * @brief Eth (Ethernet Driver) Implementation
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Eth Module - Ethernet Hardware Driver
 * Compliant with AUTOSAR R22-11 MCAL Specification
 * Module ID: 0x11
 * MISRA C:2012 compliant
 */

#include "mcal/eth/Eth.h"
#include "mcal/eth/Eth_Cfg.h"
#include <string.h>
#include <stdio.h>

/*============================================================================*
 * Static Variables
 *============================================================================*/
static Eth_ModuleStateType gEth_ModuleState;
static Eth_ControllerStateType gEth_CtrlState[ETH_CFG_CONTROLLER_COUNT];
static const Eth_HwInterfaceType* gEth_HwInterface = NULL;

/* Callback function pointers */
static Eth_TxConfirmationCallback_t gEth_TxCb = NULL;
static Eth_RxIndicationCallback_t gEth_RxCb = NULL;
static Eth_LinkStateChangeCallback_t gEth_LinkCb = NULL;
static Eth_ErrorCallback_t gEth_ErrorCb = NULL;

/*============================================================================*
 * Default Configuration
 *============================================================================*/
static const Eth_GeneralConfigType gEth_DefaultGeneral = {
    .maxControllers = ETH_CFG_CONTROLLER_COUNT,
    .mainFunctionPeriod = ETH_CFG_MAIN_FUNCTION_PERIOD,
    .devErrorDetect = (ETH_CFG_DEV_ERROR_DETECT == STD_ON),
    .versionInfoApi = (ETH_CFG_VERSION_INFO_API == STD_ON),
    .globalTimeSupport = (ETH_CFG_GLOBAL_TIME_SUPPORT == STD_ON),
    .timestampSupport = (ETH_CFG_TIMESTAMP_SUPPORT == STD_ON),
    .dmaSwBufferEnabled = (ETH_CFG_DMA_SW_BUFFER == STD_ON)
};

/* Controller configuration array */
static const Eth_ControllerConfigType gEth_DefaultControllers[ETH_CFG_CONTROLLER_COUNT] = {
    {
        .controllerIdx = 0U,
        .hwType = ETH_CFG_CTRL0_HW_TYPE,
        .baseAddress = ETH_CFG_CTRL0_BASE_ADDR,
        .dmaBaseAddress = ETH_CFG_CTRL0_DMA_BASE_ADDR,
        .macAddr = {.addr = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U}},
        .phyType = ETH_CFG_CTRL0_PHY_TYPE,
        .phyInterface = ETH_CFG_CTRL0_PHY_INTERFACE,
        .phyAddress = ETH_CFG_CTRL0_PHY_ADDRESS,
        .mdcClock = ETH_CFG_CTRL0_MDC_CLOCK,
        .mdioTimeout = ETH_CFG_CTRL0_MDIO_TIMEOUT,
        .speed = ETH_CFG_CTRL0_SPEED,
        .duplex = ETH_CFG_CTRL0_DUPLEX,
        .autoNegotiation = true,
        .loopback = false,
        .rxDescCount = ETH_CFG_CTRL0_RX_DESC_COUNT,
        .txDescCount = ETH_CFG_CTRL0_TX_DESC_COUNT,
        .rxBufferSize = ETH_CFG_CTRL0_RX_BUFFER_SIZE,
        .txBufferSize = ETH_CFG_CTRL0_TX_BUFFER_SIZE,
        .interruptMask = ETH_CFG_CTRL0_IRQ_MASK,
        .irqPriority = ETH_CFG_CTRL0_IRQ_PRIORITY,
        .irqVector = ETH_CFG_CTRL0_IRQ_VECTOR,
        .flowControlEnabled = true,
        .pauseTime = ETH_CFG_CTRL0_PAUSE_TIME,
        .hwConfig = NULL
    },
#if (ETH_CFG_CONTROLLER_COUNT > 1U)
    {
        .controllerIdx = 1U,
        .hwType = ETH_CFG_CTRL1_HW_TYPE,
        .baseAddress = ETH_CFG_CTRL1_BASE_ADDR,
        .dmaBaseAddress = ETH_CFG_CTRL1_DMA_BASE_ADDR,
        .macAddr = {.addr = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U}},
        .phyType = ETH_CFG_CTRL1_PHY_TYPE,
        .phyInterface = ETH_CFG_CTRL1_PHY_INTERFACE,
        .phyAddress = ETH_CFG_CTRL1_PHY_ADDRESS,
        .mdcClock = ETH_CFG_CTRL1_MDC_CLOCK,
        .mdioTimeout = ETH_CFG_CTRL1_MDIO_TIMEOUT,
        .speed = ETH_CFG_CTRL1_SPEED,
        .duplex = ETH_CFG_CTRL1_DUPLEX,
        .autoNegotiation = true,
        .loopback = false,
        .rxDescCount = ETH_CFG_CTRL1_RX_DESC_COUNT,
        .txDescCount = ETH_CFG_CTRL1_TX_DESC_COUNT,
        .rxBufferSize = ETH_CFG_CTRL1_RX_BUFFER_SIZE,
        .txBufferSize = ETH_CFG_CTRL1_TX_BUFFER_SIZE,
        .interruptMask = ETH_CFG_CTRL1_IRQ_MASK,
        .irqPriority = ETH_CFG_CTRL1_IRQ_PRIORITY,
        .irqVector = ETH_CFG_CTRL1_IRQ_VECTOR,
        .flowControlEnabled = true,
        .pauseTime = ETH_CFG_CTRL1_PAUSE_TIME,
        .hwConfig = NULL
    }
#endif
};

/* Complete configuration */
const Eth_ConfigType Eth_Config = {
    .general = &gEth_DefaultGeneral,
    .controllers = gEth_DefaultControllers,
    .controllerCount = ETH_CFG_CONTROLLER_COUNT
};

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Validate controller index
 */
static bool Eth_ValidateCtrlIdx(uint8_t ctrlIdx)
{
    return (ctrlIdx < ETH_CFG_CONTROLLER_COUNT);
}

/**
 * @brief Get controller configuration
 */
static const Eth_ControllerConfigType* Eth_GetControllerConfig(uint8_t ctrlIdx)
{
    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return NULL;
    }
    return &gEth_DefaultControllers[ctrlIdx];
}

/**
 * @brief Get controller state
 */
static Eth_ControllerStateType* Eth_GetControllerState(uint8_t ctrlIdx)
{
    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return NULL;
    }
    return &gEth_CtrlState[ctrlIdx];
}

/**
 * @brief Check if controller is active
 */
static bool Eth_IsControllerActive(uint8_t ctrlIdx)
{
    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state == NULL) {
        return false;
    }
    return (state->mode == ETH_MODE_ACTIVE);
}

/**
 * @brief Update link state
 */
static void Eth_UpdateLinkState(uint8_t ctrlIdx)
{
    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state == NULL) {
        return;
    }

    if (gEth_HwInterface == NULL) {
        return;
    }

    if (gEth_HwInterface->GetLinkState != NULL) {
        bool newLinkState = gEth_HwInterface->GetLinkState(ctrlIdx);
        if (newLinkState != state->linkUp) {
            state->linkUp = newLinkState;
            if (gEth_LinkCb != NULL) {
                gEth_LinkCb(ctrlIdx, newLinkState);
            }
            if (newLinkState) {
                /* Link up - update PHY config */
                state->currentSpeed = gEth_HwInterface->GetBaudRate(ctrlIdx);
                state->currentDuplex = gEth_HwInterface->GetDuplexMode(ctrlIdx);
            }
        }
    }
}

/*============================================================================*
 * Initialization API
 *============================================================================*/

Eth_ErrorCode_t Eth_Init(const Eth_ConfigType* config)
{
    uint8_t i;

    if (gEth_ModuleState.initialized) {
        return ETH_E_ALREADY_INITIALIZED;
    }

    /* Initialize module state */
    (void)memset(&gEth_ModuleState, 0, sizeof(Eth_ModuleStateType));
    (void)memset(&gEth_CtrlState, 0, sizeof(gEth_CtrlState));

    if (config == NULL) {
        gEth_ModuleState.config = &Eth_Config;
    } else {
        gEth_ModuleState.config = config;
    }

    gEth_ModuleState.ctrlState = gEth_CtrlState;
    gEth_ModuleState.moduleState = ETH_MODE_DOWN;
    gEth_ModuleState.initialized = true;

    /* Initialize each controller */
    for (i = 0U; i < ETH_CFG_CONTROLLER_COUNT; i++) {
        Eth_ErrorCode_t result = Eth_ControllerInit(i);
        if (result != ETH_OK) {
            /* Continue with other controllers even if one fails */
            /* Application can check individual controller status */
        }
    }

    return ETH_OK;
}

Eth_ErrorCode_t Eth_Deinit(void)
{
    uint8_t i;
    Eth_ErrorCode_t result = ETH_OK;

    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    /* Deinitialize all controllers */
    for (i = 0U; i < ETH_CFG_CONTROLLER_COUNT; i++) {
        (void)Eth_ControllerDeinit(i);
    }

    /* Deinitialize hardware interface if available */
    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->Deinit != NULL)) {
        for (i = 0U; i < ETH_CFG_CONTROLLER_COUNT; i++) {
            (void)gEth_HwInterface->Deinit(i);
        }
    }

    gEth_ModuleState.initialized = false;
    gEth_ModuleState.moduleState = ETH_MODE_UNINIT;
    gEth_ModuleState.initializedCtrlCount = 0U;

    return result;
}

Eth_ErrorCode_t Eth_ControllerInit(uint8_t ctrlIdx)
{
    Eth_ControllerStateType* state;
    const Eth_ControllerConfigType* cfg;
    Eth_ErrorCode_t result = ETH_OK;

    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    state = Eth_GetControllerState(ctrlIdx);
    cfg = Eth_GetControllerConfig(ctrlIdx);

    if ((state == NULL) || (cfg == NULL)) {
        return ETH_E_INV_CONFIG;
    }

    /* Initialize controller state */
    (void)memset(state, 0, sizeof(Eth_ControllerStateType));
    state->ctrlIdx = ctrlIdx;
    state->mode = ETH_MODE_DOWN;
    state->linkUp = false;
    state->currentSpeed = cfg->speed;
    state->currentDuplex = cfg->duplex;

    /* Initialize hardware through hardware interface */
    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->Init != NULL)) {
        result = gEth_HwInterface->Init(ctrlIdx, cfg);
        if (result != ETH_OK) {
            return result;
        }
    }

    /* Set default MAC address */
    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->SetMacAddr != NULL)) {
        (void)gEth_HwInterface->SetMacAddr(ctrlIdx, &cfg->macAddr);
    }

    state->mode = ETH_MODE_DOWN;
    gEth_ModuleState.initializedCtrlCount++;

    return ETH_OK;
}

Eth_ErrorCode_t Eth_ControllerDeinit(uint8_t ctrlIdx)
{
    Eth_ControllerStateType* state;
    Eth_ErrorCode_t result = ETH_OK;

    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    state = Eth_GetControllerState(ctrlIdx);
    if (state == NULL) {
        return ETH_E_INV_CTRL;
    }

    /* Disable controller */
    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->SetMode != NULL)) {
        (void)gEth_HwInterface->SetMode(ctrlIdx, ETH_MODE_DOWN);
    }

    /* Deinitialize hardware */
    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->Deinit != NULL)) {
        result = gEth_HwInterface->Deinit(ctrlIdx);
    }

    state->mode = ETH_MODE_UNINIT;
    state->linkUp = false;

    if (gEth_ModuleState.initializedCtrlCount > 0U) {
        gEth_ModuleState.initializedCtrlCount--;
    }

    return result;
}

Eth_ErrorCode_t Eth_SetControllerMode(uint8_t ctrlIdx, Eth_ModeType mode)
{
    Eth_ControllerStateType* state;
    Eth_ErrorCode_t result = ETH_OK;

    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    state = Eth_GetControllerState(ctrlIdx);
    if (state == NULL) {
        return ETH_E_INV_CTRL;
    }

    if (mode == state->mode) {
        return ETH_OK;  /* Already in requested mode */
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->SetMode != NULL)) {
        result = gEth_HwInterface->SetMode(ctrlIdx, mode);
        if (result == ETH_OK) {
            state->mode = mode;
        }
    } else {
        state->mode = mode;
    }

    return result;
}

Eth_ModeType Eth_GetControllerMode(uint8_t ctrlIdx)
{
    Eth_ControllerStateType* state;

    if (!gEth_ModuleState.initialized) {
        return ETH_MODE_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_MODE_UNINIT;
    }

    state = Eth_GetControllerState(ctrlIdx);
    if (state == NULL) {
        return ETH_MODE_UNINIT;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->GetMode != NULL)) {
        return gEth_HwInterface->GetMode(ctrlIdx);
    }

    return state->mode;
}

bool Eth_IsInitialized(void)
{
    return gEth_ModuleState.initialized;
}

bool Eth_IsControllerInitialized(uint8_t ctrlIdx)
{
    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state == NULL) {
        return false;
    }
    return (state->mode != ETH_MODE_UNINIT);
}

/*============================================================================*
 * MAC Address API
 *============================================================================*/

Eth_ErrorCode_t Eth_SetMacAddr(uint8_t ctrlIdx, const Eth_MacAddrType* macAddr)
{
    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    if (macAddr == NULL) {
        return ETH_E_INV_POINTER;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->SetMacAddr != NULL)) {
        return gEth_HwInterface->SetMacAddr(ctrlIdx, macAddr);
    }

    return ETH_E_NOT_OK;
}

Eth_ErrorCode_t Eth_GetMacAddr(uint8_t ctrlIdx, Eth_MacAddrType* macAddr)
{
    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    if (macAddr == NULL) {
        return ETH_E_INV_POINTER;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->GetMacAddr != NULL)) {
        return gEth_HwInterface->GetMacAddr(ctrlIdx, macAddr);
    }

    return ETH_E_NOT_OK;
}

Eth_ErrorCode_t Eth_AddMacFilter(uint8_t ctrlIdx, const Eth_MacAddrType* macAddr)
{
    /* MAC filtering would be implemented in hardware-specific layer */
    (void)ctrlIdx;
    (void)macAddr;
    return ETH_E_NOT_OK;
}

Eth_ErrorCode_t Eth_RemoveMacFilter(uint8_t ctrlIdx, const Eth_MacAddrType* macAddr)
{
    /* MAC filtering would be implemented in hardware-specific layer */
    (void)ctrlIdx;
    (void)macAddr;
    return ETH_E_NOT_OK;
}

Eth_ErrorCode_t Eth_ClearMacFilters(uint8_t ctrlIdx)
{
    (void)ctrlIdx;
    return ETH_E_NOT_OK;
}

/*============================================================================*
 * Transmission API
 *============================================================================*/

Eth_ErrorCode_t Eth_Transmit(uint8_t ctrlIdx, const uint8_t* data, uint16_t len, uint8_t* bufIdx)
{
    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    if (data == NULL) {
        return ETH_E_INV_POINTER;
    }

    if ((len < ETH_FRAME_HEADER_SIZE) || (len > ETH_MAX_PAYLOAD_SIZE)) {
        return ETH_E_INV_LEN;
    }

    if (!Eth_IsControllerActive(ctrlIdx)) {
        return ETH_E_INV_MODE;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->Transmit != NULL)) {
        Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
        Eth_ErrorCode_t result = gEth_HwInterface->Transmit(ctrlIdx, data, len, bufIdx);
        if (result == ETH_OK) {
            state->txFrames++;
        } else {
            state->txErrors++;
        }
        return result;
    }

    return ETH_E_NOT_OK;
}

Eth_ErrorCode_t Eth_GetTxBuffer(uint8_t ctrlIdx, uint16_t len, uint8_t** buf, uint8_t* bufIdx)
{
    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    if ((buf == NULL) || (bufIdx == NULL)) {
        return ETH_E_INV_POINTER;
    }

    if ((len < ETH_FRAME_HEADER_SIZE) || (len > ETH_MAX_PAYLOAD_SIZE)) {
        return ETH_E_INV_LEN;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->GetTxBuffer != NULL)) {
        return gEth_HwInterface->GetTxBuffer(ctrlIdx, len, buf, bufIdx);
    }

    return ETH_E_NOT_OK;
}

Eth_ErrorCode_t Eth_TransmitTxBuffer(uint8_t ctrlIdx, uint8_t bufIdx, uint16_t len)
{
    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    if (!Eth_IsControllerActive(ctrlIdx)) {
        return ETH_E_INV_MODE;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->TransmitTxBuffer != NULL)) {
        Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
        Eth_ErrorCode_t result = gEth_HwInterface->TransmitTxBuffer(ctrlIdx, bufIdx, len);
        if (result == ETH_OK) {
            state->txFrames++;
        } else {
            state->txErrors++;
        }
        return result;
    }

    return ETH_E_NOT_OK;
}

bool Eth_IsTxReady(uint8_t ctrlIdx)
{
    if (!gEth_ModuleState.initialized) {
        return false;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return false;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->IsTxReady != NULL)) {
        return gEth_HwInterface->IsTxReady(ctrlIdx);
    }

    return true;
}

Eth_ErrorCode_t Eth_WaitTxComplete(uint8_t ctrlIdx, uint32_t timeout)
{
    uint32_t elapsed = 0U;

    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    while (!Eth_IsTxReady(ctrlIdx)) {
        if ((timeout > 0U) && (elapsed >= timeout)) {
            return ETH_E_TIMEOUT;
        }
        /* Small delay would be added here in real implementation */
        elapsed++;
    }

    return ETH_OK;
}

/*============================================================================*
 * Reception API
 *============================================================================*/

Eth_ErrorCode_t Eth_Receive(uint8_t ctrlIdx, uint8_t* data, uint16_t* len, Eth_FrameInfoType* frameInfo)
{
    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    if ((data == NULL) || (len == NULL)) {
        return ETH_E_INV_POINTER;
    }

    if (!Eth_IsControllerActive(ctrlIdx)) {
        return ETH_E_INV_MODE;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->Receive != NULL)) {
        Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
        Eth_ErrorCode_t result = gEth_HwInterface->Receive(ctrlIdx, data, len, frameInfo);
        if (result == ETH_OK) {
            state->rxFrames++;
        } else if (result != ETH_E_NO_CTRL) {
            /* ETH_E_NO_CTRL indicates no frame available, not an error */
            state->rxErrors++;
        }
        return result;
    }

    return ETH_E_NOT_OK;
}

uint16_t Eth_GetRxPendingCount(uint8_t ctrlIdx)
{
    if (!gEth_ModuleState.initialized) {
        return 0U;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return 0U;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->GetRxPendingCount != NULL)) {
        return gEth_HwInterface->GetRxPendingCount(ctrlIdx);
    }

    return 0U;
}

bool Eth_IsRxAvailable(uint8_t ctrlIdx)
{
    return (Eth_GetRxPendingCount(ctrlIdx) > 0U);
}

Eth_ErrorCode_t Eth_FlushRxBuffers(uint8_t ctrlIdx)
{
    /* Flush would be implemented in hardware-specific layer */
    (void)ctrlIdx;
    return ETH_OK;
}

/*============================================================================*
 * PHY Management API
 *============================================================================*/

Eth_ErrorCode_t Eth_ReadPhy(uint8_t ctrlIdx, uint8_t regAddr, uint16_t* regVal)
{
    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    if (regVal == NULL) {
        return ETH_E_INV_POINTER;
    }

    if (regAddr > 31U) {
        return ETH_E_INV_PARAM;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->ReadPhy != NULL)) {
        return gEth_HwInterface->ReadPhy(ctrlIdx, regAddr, regVal);
    }

    return ETH_E_NOT_OK;
}

Eth_ErrorCode_t Eth_WritePhy(uint8_t ctrlIdx, uint8_t regAddr, uint16_t regVal)
{
    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    if (regAddr > 31U) {
        return ETH_E_INV_PARAM;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->WritePhy != NULL)) {
        return gEth_HwInterface->WritePhy(ctrlIdx, regAddr, regVal);
    }

    return ETH_E_NOT_OK;
}

Eth_ErrorCode_t Eth_GetPhyId(uint8_t ctrlIdx, uint16_t* phyId1, uint16_t* phyId2)
{
    Eth_ErrorCode_t result;

    result = Eth_ReadPhy(ctrlIdx, ETH_PHY_REG_PHYID1, phyId1);
    if (result != ETH_OK) {
        return result;
    }

    result = Eth_ReadPhy(ctrlIdx, ETH_PHY_REG_PHYID2, phyId2);
    return result;
}

bool Eth_GetLinkState(uint8_t ctrlIdx)
{
    if (!gEth_ModuleState.initialized) {
        return false;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return false;
    }

    Eth_UpdateLinkState(ctrlIdx);

    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state != NULL) {
        return state->linkUp;
    }

    return false;
}

Eth_ErrorCode_t Eth_WaitLinkUp(uint8_t ctrlIdx, uint32_t timeout)
{
    uint32_t elapsed = 0U;

    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    while (!Eth_GetLinkState(ctrlIdx)) {
        if ((timeout > 0U) && (elapsed >= timeout)) {
            return ETH_E_TIMEOUT;
        }
        /* Small delay would be added here in real implementation */
        /* Also update link state from PHY */
        Eth_MainFunction(ctrlIdx);
        elapsed += ETH_CFG_MAIN_FUNCTION_PERIOD;
    }

    return ETH_OK;
}

Eth_SpeedType Eth_GetBaudRate(uint8_t ctrlIdx)
{
    if (!gEth_ModuleState.initialized) {
        return ETH_SPEED_10M;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_SPEED_10M;
    }

    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state == NULL) {
        return ETH_SPEED_10M;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->GetBaudRate != NULL)) {
        state->currentSpeed = gEth_HwInterface->GetBaudRate(ctrlIdx);
    }

    return state->currentSpeed;
}

Eth_DuplexModeType Eth_GetDuplexMode(uint8_t ctrlIdx)
{
    if (!gEth_ModuleState.initialized) {
        return ETH_DUPLEX_HALF;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_DUPLEX_HALF;
    }

    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state == NULL) {
        return ETH_DUPLEX_HALF;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->GetDuplexMode != NULL)) {
        state->currentDuplex = gEth_HwInterface->GetDuplexMode(ctrlIdx);
    }

    return state->currentDuplex;
}

Eth_ErrorCode_t Eth_UpdatePhyConfig(uint8_t ctrlIdx)
{
    if (!gEth_ModuleState.initialized) {
        return ETH_E_UNINIT;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return ETH_E_INV_CTRL;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->UpdatePhyConfig != NULL)) {
        return gEth_HwInterface->UpdatePhyConfig(ctrlIdx);
    }

    return ETH_E_NOT_OK;
}

/*============================================================================*
 * Flow Control API
 *============================================================================*/

Eth_ErrorCode_t Eth_EnableFlowControl(uint8_t ctrlIdx, uint16_t pauseTime)
{
    (void)ctrlIdx;
    (void)pauseTime;
    return ETH_E_NOT_OK;
}

Eth_ErrorCode_t Eth_DisableFlowControl(uint8_t ctrlIdx)
{
    (void)ctrlIdx;
    return ETH_E_NOT_OK;
}

Eth_ErrorCode_t Eth_SendPauseFrame(uint8_t ctrlIdx, uint16_t pauseTime)
{
    (void)ctrlIdx;
    (void)pauseTime;
    return ETH_E_NOT_OK;
}

/*============================================================================*
 * Interrupt API
 *============================================================================*/

void Eth_EnableIrq(uint8_t ctrlIdx, uint32_t irqMask)
{
    if (!gEth_ModuleState.initialized) {
        return;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->EnableIrq != NULL)) {
        gEth_HwInterface->EnableIrq(ctrlIdx, irqMask);
    }
}

void Eth_DisableIrq(uint8_t ctrlIdx, uint32_t irqMask)
{
    if (!gEth_ModuleState.initialized) {
        return;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->DisableIrq != NULL)) {
        gEth_HwInterface->DisableIrq(ctrlIdx, irqMask);
    }
}

uint32_t Eth_GetPendingIrqs(uint8_t ctrlIdx)
{
    if (!gEth_ModuleState.initialized) {
        return 0U;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return 0U;
    }

    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state != NULL) {
        return state->pendingIrqs;
    }

    return 0U;
}

void Eth_ClearIrq(uint8_t ctrlIdx, Eth_IrqEventType event)
{
    if (!gEth_ModuleState.initialized) {
        return;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->ClearIrq != NULL)) {
        gEth_HwInterface->ClearIrq(ctrlIdx, event);
    }

    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state != NULL) {
        state->pendingIrqs &= ~(uint32_t)event;
    }
}

bool Eth_ProcessIrq(uint8_t ctrlIdx, Eth_IrqEventType* event)
{
    if (!gEth_ModuleState.initialized) {
        return false;
    }

    if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
        return false;
    }

    if (event == NULL) {
        return false;
    }

    if ((gEth_HwInterface != NULL) && (gEth_HwInterface->ProcessIrq != NULL)) {
        gEth_HwInterface->ProcessIrq(ctrlIdx, event);

        /* Call callbacks if registered */
        if (*event != ETH_IRQ_NONE) {
            Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
            if (state != NULL) {
                state->pendingIrqs |= (uint32_t)*event;
            }

            switch (*event) {
                case ETH_IRQ_TX_COMPLETE:
                    if (gEth_TxCb != NULL) {
                        gEth_TxCb(ctrlIdx, 0U, ETH_OK);
                    }
                    break;
                case ETH_IRQ_RX_COMPLETE:
                    if (gEth_RxCb != NULL) {
                        /* Frame info would be retrieved and passed */
                        gEth_RxCb(ctrlIdx, NULL);
                    }
                    break;
                case ETH_IRQ_PHY_EVENT:
                    Eth_UpdateLinkState(ctrlIdx);
                    break;
                case ETH_IRQ_TX_ERROR:
                case ETH_IRQ_RX_ERROR:
                case ETH_IRQ_DMA_ERROR:
                    if (gEth_ErrorCb != NULL) {
                        gEth_ErrorCb(ctrlIdx, *event, 0U);
                    }
                    break;
                default:
                    break;
            }
        }

        return (*event != ETH_IRQ_NONE);
    }

    return false;
}

/*============================================================================*
 * Main Function
 *============================================================================*/

void Eth_MainFunction(uint8_t ctrlIdx)
{
    uint8_t startIdx, endIdx, i;

    if (!gEth_ModuleState.initialized) {
        return;
    }

    if (ctrlIdx == 0xFFU) {
        /* Process all controllers */
        startIdx = 0U;
        endIdx = ETH_CFG_CONTROLLER_COUNT;
    } else {
        if (!Eth_ValidateCtrlIdx(ctrlIdx)) {
            return;
        }
        startIdx = ctrlIdx;
        endIdx = ctrlIdx + 1U;
    }

    for (i = startIdx; i < endIdx; i++) {
        Eth_ControllerStateType* state = Eth_GetControllerState(i);
        if (state == NULL) {
            continue;
        }

        if (state->mode != ETH_MODE_ACTIVE) {
            continue;
        }

        /* Update link state */
        Eth_UpdateLinkState(i);

        /* Call hardware main function if available */
        if ((gEth_HwInterface != NULL) && (gEth_HwInterface->MainFunction != NULL)) {
            gEth_HwInterface->MainFunction(i);
        }
    }
}

/*============================================================================*
 * Statistics API
 *============================================================================*/

uint32_t Eth_GetTxFrameCount(uint8_t ctrlIdx)
{
    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state != NULL) {
        return state->txFrames;
    }
    return 0U;
}

uint32_t Eth_GetRxFrameCount(uint8_t ctrlIdx)
{
    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state != NULL) {
        return state->rxFrames;
    }
    return 0U;
}

void Eth_GetErrorCounts(uint8_t ctrlIdx, uint32_t* txErrors, uint32_t* rxErrors, uint32_t* crcErrors)
{
    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state != NULL) {
        if (txErrors != NULL) {
            *txErrors = state->txErrors;
        }
        if (rxErrors != NULL) {
            *rxErrors = state->rxErrors;
        }
        if (crcErrors != NULL) {
            *crcErrors = state->crcErrors;
        }
    }
}

void Eth_ResetStatistics(uint8_t ctrlIdx)
{
    Eth_ControllerStateType* state = Eth_GetControllerState(ctrlIdx);
    if (state != NULL) {
        state->txFrames = 0U;
        state->rxFrames = 0U;
        state->txErrors = 0U;
        state->rxErrors = 0U;
        state->crcErrors = 0U;
        state->bufferOverflows = 0U;
    }
}

/*============================================================================*
 * Callback Registration
 *============================================================================*/

void Eth_SetTxConfirmationCallback(Eth_TxConfirmationCallback_t callback)
{
    gEth_TxCb = callback;
}

void Eth_SetRxIndicationCallback(Eth_RxIndicationCallback_t callback)
{
    gEth_RxCb = callback;
}

void Eth_SetLinkStateChangeCallback(Eth_LinkStateChangeCallback_t callback)
{
    gEth_LinkCb = callback;
}

void Eth_SetErrorCallback(Eth_ErrorCallback_t callback)
{
    gEth_ErrorCb = callback;
}

/*============================================================================*
 * Version Info API
 *============================================================================*/

Eth_ErrorCode_t Eth_GetVersionInfo(uint8_t* major, uint8_t* minor, uint8_t* patch)
{
    if ((major == NULL) || (minor == NULL) || (patch == NULL)) {
        return ETH_E_INV_POINTER;
    }

    *major = ETH_MAJOR_VERSION;
    *minor = ETH_MINOR_VERSION;
    *patch = ETH_PATCH_VERSION;

    return ETH_OK;
}

/*============================================================================*
 * Hardware Interface Registration
 *============================================================================*/

Eth_ErrorCode_t Eth_RegisterHwInterface(const Eth_HwInterfaceType* hwInterface)
{
    if (hwInterface == NULL) {
        return ETH_E_INV_POINTER;
    }

    if (gEth_ModuleState.initialized) {
        return ETH_E_ALREADY_INITIALIZED;
    }

    gEth_HwInterface = hwInterface;
    return ETH_OK;
}

const Eth_HwInterfaceType* Eth_GetHwInterface(void)
{
    return gEth_HwInterface;
}

/*============================================================================*
 * Utility Functions
 *============================================================================*/

uint16_t Eth_SpeedToMbps(Eth_SpeedType speed)
{
    switch (speed) {
        case ETH_SPEED_10M:
            return 10U;
        case ETH_SPEED_100M:
            return 100U;
        case ETH_SPEED_1000M:
            return 1000U;
        case ETH_SPEED_2500M:
            return 2500U;
        case ETH_SPEED_5000M:
            return 5000U;
        case ETH_SPEED_10000M:
            return 10000U;
        default:
            return 10U;
    }
}

Eth_SpeedType Eth_MbpsToSpeed(uint16_t mbps)
{
    switch (mbps) {
        case 10U:
            return ETH_SPEED_10M;
        case 100U:
            return ETH_SPEED_100M;
        case 1000U:
            return ETH_SPEED_1000M;
        case 2500U:
            return ETH_SPEED_2500M;
        case 5000U:
            return ETH_SPEED_5000M;
        case 10000U:
            return ETH_SPEED_10000M;
        default:
            return ETH_SPEED_10M;
    }
}

bool Eth_MacAddrEqual(const Eth_MacAddrType* addr1, const Eth_MacAddrType* addr2)
{
    if ((addr1 == NULL) || (addr2 == NULL)) {
        return false;
    }

    return (memcmp(addr1->addr, addr2->addr, ETH_MAC_ADDR_SIZE) == 0);
}

void Eth_MacAddrCopy(Eth_MacAddrType* dest, const Eth_MacAddrType* src)
{
    if ((dest == NULL) || (src == NULL)) {
        return;
    }

    (void)memcpy(dest->addr, src->addr, ETH_MAC_ADDR_SIZE);
}

bool Eth_IsMulticastAddr(const Eth_MacAddrType* addr)
{
    if (addr == NULL) {
        return false;
    }

    /* Multicast addresses have the least significant bit of the first byte set */
    return ((addr->addr[0] & 0x01U) != 0U);
}

bool Eth_IsBroadcastAddr(const Eth_MacAddrType* addr)
{
    if (addr == NULL) {
        return false;
    }

    const uint8_t broadcast[ETH_MAC_ADDR_SIZE] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    return (memcmp(addr->addr, broadcast, ETH_MAC_ADDR_SIZE) == 0);
}

Eth_ErrorCode_t Eth_MacAddrToString(const Eth_MacAddrType* addr, char* str)
{
    if ((addr == NULL) || (str == NULL)) {
        return ETH_E_INV_POINTER;
    }

    (void)sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
                  addr->addr[0], addr->addr[1], addr->addr[2],
                  addr->addr[3], addr->addr[4], addr->addr[5]);

    return ETH_OK;
}
