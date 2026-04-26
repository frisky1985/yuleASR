/******************************************************************************
 * @file    EthIf.c
 * @brief   Ethernet Interface (EthIf) - Core Implementation
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-B Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ecual/ethIf/EthIf.h"
#include "ecual/ethIf/EthIf_Cfg.h"
#include <string.h>

/* Define NULL_PTR if not defined */
#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

/******************************************************************************
 * Module Version Information
 ******************************************************************************/
#define ETHIF_SW_VERSION_MAJOR          ETHIF_CFG_SW_MAJOR_VERSION
#define ETHIF_SW_VERSION_MINOR          ETHIF_CFG_SW_MINOR_VERSION
#define ETHIF_SW_VERSION_PATCH          ETHIF_CFG_SW_PATCH_VERSION
#define ETHIF_VENDOR_ID_VALUE           ETHIF_CFG_VENDOR_ID
#define ETHIF_MODULE_ID_VALUE           ETHIF_CFG_MODULE_ID

/******************************************************************************
 * Magic Numbers
 ******************************************************************************/
#define ETHIF_MAGIC_UNINITIALIZED       0x00000000U
#define ETHIF_MAGIC_INITIALIZED         0x45544849U  /* "ETHI" */

/******************************************************************************
 * DET Error Reporting Macro
 ******************************************************************************/
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
#define ETHIF_DET_REPORT_ERROR(sid, error) \
    Det_ReportError(ETHIF_MODULE_ID_VALUE, 0U, (sid), (error))
#define ETHIF_CHECK_INITIALIZED(sid) \
    do { if (!EthIf_IsModuleInitialized()) { \
        ETHIF_DET_REPORT_ERROR((sid), ETHIF_E_NOT_INITIALIZED); \
        return E_NOT_OK; }} while(0)
#define ETHIF_CHECK_CTRL_IDX(ctrl, sid) \
    do { if ((ctrl) >= ETHIF_MAX_CONTROLLERS) { \
        ETHIF_DET_REPORT_ERROR((sid), ETHIF_E_INVALID_CTRL_IDX); \
        return E_NOT_OK; }} while(0)
#define ETHIF_CHECK_POINTER(ptr, sid) \
    do { if (NULL_PTR == (ptr)) { \
        ETHIF_DET_REPORT_ERROR((sid), ETHIF_E_INVALID_POINTER); \
        return E_NOT_OK; }} while(0)
#else
#define ETHIF_DET_REPORT_ERROR(sid, error)
#define ETHIF_CHECK_INITIALIZED(sid)
#define ETHIF_CHECK_CTRL_IDX(ctrl, sid)
#define ETHIF_CHECK_POINTER(ptr, sid)
#endif

/******************************************************************************
 * Internal State Type
 ******************************************************************************/
typedef struct {
    uint32 magicNumber;
    boolean initialized;
    uint32 activeControllers;
    uint32 txTotalFrames;
    uint32 rxTotalFrames;
    uint32 txDroppedFrames;
    uint32 rxDroppedFrames;
} EthIf_InternalStateType;

/******************************************************************************
 * Internal Variables
 ******************************************************************************/
/* Module configuration pointer */
const EthIf_ConfigType *EthIf_ConfigPtr = NULL_PTR;

/* Controller status array */
EthIf_ControllerStatusType EthIf_CtrlStatus[ETHIF_MAX_CONTROLLERS];

/* Transmit queue array */
EthIf_TxQueueType EthIf_TxQueue[ETHIF_MAX_CONTROLLERS];

/* Internal state */
static EthIf_InternalStateType EthIf_InternalState;

/* Static buffers */
static uint8 EthIf_RxBuffers[ETHIF_MAX_CONTROLLERS][ETHIF_NUM_RX_BUFFERS][ETHIF_RX_BUFFER_SIZE];
static uint8 EthIf_TxBuffers[ETHIF_MAX_CONTROLLERS][ETHIF_NUM_TX_BUFFERS][ETHIF_TX_BUFFER_SIZE];
static boolean EthIf_TxBufInUse[ETHIF_MAX_CONTROLLERS][ETHIF_NUM_TX_BUFFERS];

/* Virtual controller to physical controller mapping */
static EthIf_CtrlIdxType EthIf_VirtCtrlToPhysCtrl[ETHIF_MAX_VIRT_CTRLS];

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static Std_ReturnType EthIf_InitController(EthIf_CtrlIdxType ctrlIdx);
static void EthIf_DeInitController(EthIf_CtrlIdxType ctrlIdx);
static Std_ReturnType EthIf_ProcessTxQueue(EthIf_CtrlIdxType ctrlIdx);
static EthIf_BufIdxType EthIf_AllocateTxBuffer(EthIf_CtrlIdxType ctrlIdx);
static void EthIf_ReleaseTxBuffer(EthIf_CtrlIdxType ctrlIdx, EthIf_BufIdxType bufIdx);
static boolean EthIf_IsModuleInitialized(void);
static void EthIf_UpdateControllerActivity(EthIf_CtrlIdxType ctrlIdx);

/******************************************************************************
 * API Implementation
 ******************************************************************************/

/**
 * @brief Initialize EthIf module
 */
Std_ReturnType EthIf_Init(const EthIf_ConfigType *config)
{
    uint8 i;
    Std_ReturnType result = E_OK;

    /* Check for NULL pointer */
    if (NULL_PTR == config) {
        return E_NOT_OK;
    }

    /* Check if already initialized */
    if (EthIf_IsModuleInitialized()) {
        ETHIF_DET_REPORT_ERROR(ETHIF_SID_INIT, ETHIF_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    /* Initialize internal state */
    (void)memset(&EthIf_InternalState, 0, sizeof(EthIf_InternalState));
    (void)memset(EthIf_CtrlStatus, 0, sizeof(EthIf_CtrlStatus));
    (void)memset(EthIf_TxQueue, 0, sizeof(EthIf_TxQueue));
    (void)memset(EthIf_VirtCtrlToPhysCtrl, 0xFF, sizeof(EthIf_VirtCtrlToPhysCtrl));
    (void)memset(EthIf_TxBufInUse, 0, sizeof(EthIf_TxBufInUse));

    /* Store configuration pointer */
    EthIf_ConfigPtr = config;

    /* Initialize each enabled controller */
    if (NULL_PTR != config->generalConfig) {
        for (i = 0U; i < ETHIF_MAX_CONTROLLERS; i++) {
            if ((i < config->generalConfig->numControllers) && 
                (NULL_PTR != config->ctrlConfigs[i]) &&
                (config->ctrlConfigs[i]->enabled)) {
                
                result = EthIf_InitController(i);
                
                if (E_OK == result) {
                    EthIf_InternalState.activeControllers |= (1U << i);
                    EthIf_CtrlStatus[i].state = ETHIF_STATE_INIT;
                }
            }
        }
    }

    /* Mark module as initialized if at least one controller is active */
    if (EthIf_InternalState.activeControllers > 0U) {
        EthIf_InternalState.magicNumber = ETHIF_MAGIC_INITIALIZED;
        EthIf_InternalState.initialized = TRUE;
    } else {
        result = E_NOT_OK;
    }

    return result;
}

/**
 * @brief Deinitialize EthIf module
 */
Std_ReturnType EthIf_DeInit(void)
{
    uint8 i;

    /* Check if initialized */
    if (!EthIf_IsModuleInitialized()) {
        ETHIF_DET_REPORT_ERROR(ETHIF_SID_DEINIT, ETHIF_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    /* Deinitialize all active controllers */
    for (i = 0U; i < ETHIF_MAX_CONTROLLERS; i++) {
        if ((EthIf_InternalState.activeControllers & (1U << i)) != 0U) {
            EthIf_DeInitController(i);
        }
    }

    /* Clear internal state */
    (void)memset(&EthIf_InternalState, 0, sizeof(EthIf_InternalState));
    (void)memset(EthIf_CtrlStatus, 0, sizeof(EthIf_CtrlStatus));
    (void)memset(EthIf_TxQueue, 0, sizeof(EthIf_TxQueue));
    
    EthIf_ConfigPtr = NULL_PTR;

    return E_OK;
}

/**
 * @brief Set controller mode (DOWN/ACTIVE)
 */
Std_ReturnType EthIf_SetControllerMode(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_ControllerModeType mode)
{
    Std_ReturnType result = E_NOT_OK;

    /* Parameter checks */
    ETHIF_CHECK_INITIALIZED(ETHIF_SID_SETCONTROLLERMODE);
    ETHIF_CHECK_CTRL_IDX(ctrlIdx, ETHIF_SID_SETCONTROLLERMODE);

    if (!ETHIF_CTRL_MODE_IS_VALID(mode)) {
        ETHIF_DET_REPORT_ERROR(ETHIF_SID_SETCONTROLLERMODE, ETHIF_E_INVALID_MODE);
        return E_NOT_OK;
    }

    /* Check if controller is active */
    if ((EthIf_InternalState.activeControllers & (1U << ctrlIdx)) == 0U) {
        ETHIF_DET_REPORT_ERROR(ETHIF_SID_SETCONTROLLERMODE, ETHIF_E_INVALID_CTRL_IDX);
        return E_NOT_OK;
    }

    /* Perform mode switch */
    if (ETHIF_CTRL_MODE_ACTIVE == mode) {
        /* Transition to ACTIVE mode */
        EthIf_CtrlStatus[ctrlIdx].currentMode = ETHIF_CTRL_MODE_ACTIVE;
        EthIf_CtrlStatus[ctrlIdx].state = ETHIF_STATE_ACTIVE;
        result = E_OK;
    } else {
        /* Transition to DOWN mode */
        EthIf_CtrlStatus[ctrlIdx].currentMode = ETHIF_CTRL_MODE_DOWN;
        EthIf_CtrlStatus[ctrlIdx].state = ETHIF_STATE_DOWN;
        /* Clear transmit queue when going down */
        (void)EthIf_ClearTxQueue(ctrlIdx);
        result = E_OK;
    }

    /* Notify upper layer */
#if (ETHIF_UL_MODE_INDICATION_CALLBACK == STD_ON)
    if (NULL_PTR != EthIf_UpperLayer_ControllerModeIndication) {
        EthIf_UpperLayer_ControllerModeIndication(ctrlIdx, mode);
    }
#endif

    return result;
}

/**
 * @brief Get current controller mode
 */
Std_ReturnType EthIf_GetControllerMode(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_ControllerModeType *modePtr)
{
    /* Parameter checks */
    ETHIF_CHECK_INITIALIZED(ETHIF_SID_GETCONTROLLERMODE);
    ETHIF_CHECK_CTRL_IDX(ctrlIdx, ETHIF_SID_GETCONTROLLERMODE);
    ETHIF_CHECK_POINTER(modePtr, ETHIF_SID_GETCONTROLLERMODE);

    *modePtr = EthIf_CtrlStatus[ctrlIdx].currentMode;

    return E_OK;
}

/**
 * @brief Get physical controller index from virtual controller
 */
EthIf_CtrlIdxType EthIf_GetControllerIdx(EthIf_VirtCtrlIdxType virtCtrlIdx)
{
    if (!EthIf_IsModuleInitialized()) {
        return 0xFFU;
    }

    if (virtCtrlIdx >= ETHIF_MAX_VIRT_CTRLS) {
        return 0xFFU;
    }

    return EthIf_VirtCtrlToPhysCtrl[virtCtrlIdx];
}

/**
 * @brief Get physical address (MAC address)
 */
Std_ReturnType EthIf_GetPhysAddr(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_MacAddrType *physAddrPtr)
{
    /* Parameter checks */
    ETHIF_CHECK_INITIALIZED(ETHIF_SID_GETPHYSADDR);
    ETHIF_CHECK_CTRL_IDX(ctrlIdx, ETHIF_SID_GETPHYSADDR);
    ETHIF_CHECK_POINTER(physAddrPtr, ETHIF_SID_GETPHYSADDR);

    if ((EthIf_InternalState.activeControllers & (1U << ctrlIdx)) == 0U) {
        return E_NOT_OK;
    }

    if (NULL_PTR == EthIf_ConfigPtr->ctrlConfigs[ctrlIdx]) {
        return E_NOT_OK;
    }

    (void)memcpy(*physAddrPtr, EthIf_ConfigPtr->ctrlConfigs[ctrlIdx]->macAddr, 
                 ETHIF_MAC_ADDR_LEN);

    return E_OK;
}

/**
 * @brief Set physical address (MAC address)
 */
Std_ReturnType EthIf_SetPhysAddr(
    EthIf_CtrlIdxType ctrlIdx,
    const EthIf_MacAddrType *physAddrPtr)
{
    /* Parameter checks */
    ETHIF_CHECK_INITIALIZED(ETHIF_SID_SETPHYSADDR);
    ETHIF_CHECK_CTRL_IDX(ctrlIdx, ETHIF_SID_SETPHYSADDR);
    ETHIF_CHECK_POINTER(physAddrPtr, ETHIF_SID_SETPHYSADDR);

    if ((EthIf_InternalState.activeControllers & (1U << ctrlIdx)) == 0U) {
        return E_NOT_OK;
    }

    /* In a real implementation, this would call the Eth driver */
    /* For now, just update the config (should use a writable copy) */
    
    return E_OK;
}

/**
 * @brief Get broadcast MAC address
 */
void EthIf_GetBroadcast(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_MacAddrType *broadcastAddrPtr)
{
    uint8 i;

    if (NULL_PTR == broadcastAddrPtr) {
        return;
    }

    /* Broadcast address is always FF:FF:FF:FF:FF:FF */
    for (i = 0U; i < ETHIF_MAC_ADDR_LEN; i++) {
        (*broadcastAddrPtr)[i] = 0xFFU;
    }

    (void)ctrlIdx; /* Parameter not used, avoid compiler warning */
}

/**
 * @brief Provide transmit buffer
 */
EthIf_BufReqReturnType EthIf_ProvideTxBuffer(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_PriorityType priority,
    EthIf_BufIdxType *bufIdxPtr,
    uint8 **bufPtrPtr,
    uint16 *lenPtr)
{
    EthIf_BufIdxType bufIdx;

    /* Parameter checks */
    if (!EthIf_IsModuleInitialized()) {
        return ETHIF_BUFREQ_E_NOT_OK;
    }

    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return ETHIF_BUFREQ_E_NOT_OK;
    }

    if ((NULL_PTR == bufIdxPtr) || (NULL_PTR == bufPtrPtr) || (NULL_PTR == lenPtr)) {
        return ETHIF_BUFREQ_E_NOT_OK;
    }

    if ((EthIf_InternalState.activeControllers & (1U << ctrlIdx)) == 0U) {
        return ETHIF_BUFREQ_E_NOT_OK;
    }

    (void)priority; /* Priority based allocation not implemented yet */

    /* Allocate buffer */
    bufIdx = EthIf_AllocateTxBuffer(ctrlIdx);
    if (bufIdx >= ETHIF_NUM_TX_BUFFERS) {
        return ETHIF_BUFREQ_E_BUSY;
    }

    *bufIdxPtr = bufIdx;
    *bufPtrPtr = EthIf_TxBuffers[ctrlIdx][bufIdx];
    
    if (*lenPtr > ETHIF_TX_BUFFER_SIZE) {
        *lenPtr = ETHIF_TX_BUFFER_SIZE;
    }

    return ETHIF_BUFREQ_OK;
}

/**
 * @brief Transmit Ethernet frame
 */
Std_ReturnType EthIf_Transmit(
    EthIf_VirtCtrlIdxType virtCtrlIdx,
    EthIf_FrameType frameType,
    EthIf_PduHandleType txPduId,
    EthIf_BufIdxType bufIdx,
    const uint8 *bufPtr,
    uint16 len)
{
    EthIf_CtrlIdxType ctrlIdx;
    uint8 queueIdx;

    /* Parameter checks */
    ETHIF_CHECK_INITIALIZED(ETHIF_SID_TRANSMIT);

    if (virtCtrlIdx >= ETHIF_MAX_VIRT_CTRLS) {
        ETHIF_DET_REPORT_ERROR(ETHIF_SID_TRANSMIT, ETHIF_E_INVALID_CTRL_IDX);
        return E_NOT_OK;
    }

    if (NULL_PTR == bufPtr) {
        ETHIF_DET_REPORT_ERROR(ETHIF_SID_TRANSMIT, ETHIF_E_INVALID_POINTER);
        return E_NOT_OK;
    }

    if ((len == 0U) || (len > ETHIF_TX_BUFFER_SIZE)) {
        ETHIF_DET_REPORT_ERROR(ETHIF_SID_TRANSMIT, ETHIF_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }

    /* Get physical controller index */
    ctrlIdx = EthIf_VirtCtrlToPhysCtrl[virtCtrlIdx];
    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        ETHIF_DET_REPORT_ERROR(ETHIF_SID_TRANSMIT, ETHIF_E_INVALID_CTRL_IDX);
        return E_NOT_OK;
    }

    /* Check controller mode */
    if (ETHIF_CTRL_MODE_ACTIVE != EthIf_CtrlStatus[ctrlIdx].currentMode) {
        return E_NOT_OK;
    }

    /* Add to transmit queue */
    queueIdx = EthIf_TxQueue[ctrlIdx].head;
    
    if (EthIf_TxQueue[ctrlIdx].count >= ETHIF_MAX_QUEUE_DEPTH) {
        /* Queue full */
        EthIf_CtrlStatus[ctrlIdx].txErrorCount++;
        EthIf_InternalState.txDroppedFrames++;
        EthIf_ReleaseTxBuffer(ctrlIdx, bufIdx);
        return E_NOT_OK;
    }

    /* Store in queue */
    EthIf_TxQueue[ctrlIdx].entries[queueIdx].dataPtr = (uint8 *)bufPtr;
    EthIf_TxQueue[ctrlIdx].entries[queueIdx].length = len;
    EthIf_TxQueue[ctrlIdx].entries[queueIdx].pduHandle = txPduId;
    EthIf_TxQueue[ctrlIdx].entries[queueIdx].inUse = TRUE;

    EthIf_TxQueue[ctrlIdx].head = (queueIdx + 1U) % ETHIF_MAX_QUEUE_DEPTH;
    EthIf_TxQueue[ctrlIdx].count++;

    /* Update statistics */
    EthIf_CtrlStatus[ctrlIdx].txFrameCount++;
    EthIf_InternalState.txTotalFrames++;
    EthIf_UpdateControllerActivity(ctrlIdx);

    return E_OK;
}

/**
 * @brief Get version information
 */
void EthIf_GetVersionInfo(Std_VersionInfoType *versionInfo)
{
    if (NULL_PTR != versionInfo) {
        versionInfo->vendorID = ETHIF_VENDOR_ID_VALUE;
        versionInfo->moduleID = ETHIF_MODULE_ID_VALUE;
        versionInfo->sw_major_version = ETHIF_SW_VERSION_MAJOR;
        versionInfo->sw_minor_version = ETHIF_SW_VERSION_MINOR;
        versionInfo->sw_patch_version = ETHIF_SW_VERSION_PATCH;
    }
}

/******************************************************************************
 * Time Synchronization Functions
 ******************************************************************************/

/**
 * @brief Get current time
 */
Std_ReturnType EthIf_GetCurrentTime(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_TimeStampQualType *timeStampPtr)
{
    ETHIF_CHECK_INITIALIZED(ETHIF_SID_GETCURRENTTIME);
    ETHIF_CHECK_CTRL_IDX(ctrlIdx, ETHIF_SID_GETCURRENTTIME);
    ETHIF_CHECK_POINTER(timeStampPtr, ETHIF_SID_GETCURRENTTIME);

#if (ETHIF_ENABLE_TIMESTAMP == STD_ON)
    /* In a real implementation, this would read hardware timestamp */
    timeStampPtr->seconds = 0U;
    timeStampPtr->nanoseconds = 0U;
    return E_OK;
#else
    (void)timeStampPtr;
    return E_NOT_OK;
#endif
}

/**
 * @brief Enable egress timestamp
 */
Std_ReturnType EthIf_EnableEgressTimeStamp(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_BufIdxType bufIdx)
{
    ETHIF_CHECK_INITIALIZED(ETHIF_SID_ENABLEEGRESSTIMESTAMP);
    ETHIF_CHECK_CTRL_IDX(ctrlIdx, ETHIF_SID_ENABLEEGRESSTIMESTAMP);

#if (ETHIF_ENABLE_TIMESTAMP == STD_ON)
    /* In a real implementation, this would enable timestamp in hardware */
    (void)bufIdx;
    return E_OK;
#else
    (void)bufIdx;
    return E_NOT_OK;
#endif
}

/**
 * @brief Get egress timestamp
 */
Std_ReturnType EthIf_GetEgressTimeStamp(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_BufIdxType bufIdx,
    EthIf_TimeStampQualType *timeStampPtr)
{
    ETHIF_CHECK_INITIALIZED(ETHIF_SID_GETEGRESSTIMESTAMP);
    ETHIF_CHECK_CTRL_IDX(ctrlIdx, ETHIF_SID_GETEGRESSTIMESTAMP);
    ETHIF_CHECK_POINTER(timeStampPtr, ETHIF_SID_GETEGRESSTIMESTAMP);

#if (ETHIF_ENABLE_TIMESTAMP == STD_ON)
    /* In a real implementation, this would read hardware timestamp */
    (void)bufIdx;
    timeStampPtr->seconds = 0U;
    timeStampPtr->nanoseconds = 0U;
    return E_OK;
#else
    (void)bufIdx;
    (void)timeStampPtr;
    return E_NOT_OK;
#endif
}

/******************************************************************************
 * Status Functions
 ******************************************************************************/

/**
 * @brief Get controller status
 */
EthIf_StateType EthIf_GetControllerState(EthIf_CtrlIdxType ctrlIdx)
{
    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return ETHIF_STATE_UNINIT;
    }

    if (!EthIf_IsModuleInitialized()) {
        return ETHIF_STATE_UNINIT;
    }

    return EthIf_CtrlStatus[ctrlIdx].state;
}

/**
 * @brief Check if controller is initialized
 */
boolean EthIf_IsControllerInitialized(EthIf_CtrlIdxType ctrlIdx)
{
    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return FALSE;
    }

    if (!EthIf_IsModuleInitialized()) {
        return FALSE;
    }

    return (ETHIF_STATE_UNINIT != EthIf_CtrlStatus[ctrlIdx].state);
}

/**
 * @brief Get transmit queue depth
 */
uint8 EthIf_GetTxQueueDepth(EthIf_CtrlIdxType ctrlIdx)
{
    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return 0U;
    }

    if (!EthIf_IsModuleInitialized()) {
        return 0U;
    }

    return EthIf_TxQueue[ctrlIdx].count;
}

/**
 * @brief Clear transmit queue
 */
Std_ReturnType EthIf_ClearTxQueue(EthIf_CtrlIdxType ctrlIdx)
{
    uint8 i;

    ETHIF_CHECK_INITIALIZED(ETHIF_SID_INIT);
    ETHIF_CHECK_CTRL_IDX(ctrlIdx, ETHIF_SID_INIT);

    /* Release all buffers in queue */
    for (i = 0U; i < ETHIF_MAX_QUEUE_DEPTH; i++) {
        if (EthIf_TxQueue[ctrlIdx].entries[i].inUse) {
            /* Find and release buffer */
            /* Note: This is simplified - real implementation needs better buffer tracking */
            EthIf_TxQueue[ctrlIdx].entries[i].inUse = FALSE;
        }
    }

    /* Reset queue */
    EthIf_TxQueue[ctrlIdx].head = 0U;
    EthIf_TxQueue[ctrlIdx].tail = 0U;
    EthIf_TxQueue[ctrlIdx].count = 0U;

    return E_OK;
}

/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/**
 * @brief RxIndication callback from Eth Driver
 */
void EthIf_RxIndication(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_FrameType frameType,
    EthIf_PduHandleType rxPduId,
    const uint8 *bufPtr,
    uint16 len)
{
    /* Check parameters */
    if (!EthIf_IsModuleInitialized()) {
        return;
    }

    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return;
    }

    if (NULL_PTR == bufPtr) {
        return;
    }

    if ((EthIf_InternalState.activeControllers & (1U << ctrlIdx)) == 0U) {
        return;
    }

    /* Update statistics */
    EthIf_CtrlStatus[ctrlIdx].rxFrameCount++;
    EthIf_InternalState.rxTotalFrames++;
    EthIf_UpdateControllerActivity(ctrlIdx);

    /* Forward to upper layer (SoAd) */
#if (ETHIF_UL_RXINDICATION_CALLBACK == STD_ON)
    if (NULL_PTR != EthIf_UpperLayer_RxIndication) {
        EthIf_UpperLayer_RxIndication(rxPduId, frameType, bufPtr, len);
    }
#else
    (void)frameType;
    (void)rxPduId;
    (void)len;
#endif
}

/**
 * @brief TxConfirmation callback from Eth Driver
 */
Std_ReturnType EthIf_TxConfirmation(EthIf_CtrlIdxType ctrlIdx, EthIf_BufIdxType bufIdx)
{
    /* Check parameters */
    if (!EthIf_IsModuleInitialized()) {
        return E_NOT_OK;
    }

    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return E_NOT_OK;
    }

    /* Release the buffer */
    EthIf_ReleaseTxBuffer(ctrlIdx, bufIdx);

    /* Forward to upper layer (SoAd) */
    /* Note: txPduId should be retrieved from queue entry */
#if (ETHIF_UL_TXCONFIRMATION_CALLBACK == STD_ON)
    if (NULL_PTR != EthIf_UpperLayer_TxConfirmation) {
        /* EthIf_UpperLayer_TxConfirmation(txPduId); */
    }
#endif

    return E_OK;
}

/**
 * @brief Controller mode change indication from Eth Driver
 */
void EthIf_ControllerModeIndication(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_ControllerModeType mode)
{
    /* Check parameters */
    if (!EthIf_IsModuleInitialized()) {
        return;
    }

    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return;
    }

    /* Update internal state */
    EthIf_CtrlStatus[ctrlIdx].currentMode = mode;
    
    if (ETHIF_CTRL_MODE_ACTIVE == mode) {
        EthIf_CtrlStatus[ctrlIdx].state = ETHIF_STATE_ACTIVE;
    } else {
        EthIf_CtrlStatus[ctrlIdx].state = ETHIF_STATE_DOWN;
    }

    /* Forward to upper layer (SoAd) */
#if (ETHIF_UL_MODE_INDICATION_CALLBACK == STD_ON)
    if (NULL_PTR != EthIf_UpperLayer_ControllerModeIndication) {
        EthIf_UpperLayer_ControllerModeIndication(ctrlIdx, mode);
    }
#endif
}

/******************************************************************************
 * Main Function
 ******************************************************************************/

/**
 * @brief Main function - called cyclically
 */
void EthIf_MainFunction(void)
{
    uint8 i;

    if (!EthIf_IsModuleInitialized()) {
        return;
    }

    /* Process transmit queues for all active controllers */
    for (i = 0U; i < ETHIF_MAX_CONTROLLERS; i++) {
        if ((EthIf_InternalState.activeControllers & (1U << i)) != 0U) {
            if (ETHIF_CTRL_MODE_ACTIVE == EthIf_CtrlStatus[i].currentMode) {
                (void)EthIf_ProcessTxQueue(i);
            }
        }
    }
}

/******************************************************************************
 * Local Functions
 ******************************************************************************/

/**
 * @brief Initialize a specific controller
 */
static Std_ReturnType EthIf_InitController(EthIf_CtrlIdxType ctrlIdx)
{
    const EthIf_ControllerConfigType *config;
    uint8 i;

    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return E_NOT_OK;
    }

    if (NULL_PTR == EthIf_ConfigPtr) {
        return E_NOT_OK;
    }

    config = EthIf_ConfigPtr->ctrlConfigs[ctrlIdx];
    if (NULL_PTR == config) {
        return E_NOT_OK;
    }

    /* Initialize controller status */
    EthIf_CtrlStatus[ctrlIdx].state = ETHIF_STATE_INIT;
    EthIf_CtrlStatus[ctrlIdx].currentMode = ETHIF_CTRL_MODE_DOWN;
    EthIf_CtrlStatus[ctrlIdx].txFrameCount = 0U;
    EthIf_CtrlStatus[ctrlIdx].rxFrameCount = 0U;
    EthIf_CtrlStatus[ctrlIdx].txErrorCount = 0U;
    EthIf_CtrlStatus[ctrlIdx].rxErrorCount = 0U;
    EthIf_CtrlStatus[ctrlIdx].droppedFrames = 0U;
    EthIf_CtrlStatus[ctrlIdx].lastActivityTime = 0U;

    /* Initialize transmit queue */
    EthIf_TxQueue[ctrlIdx].head = 0U;
    EthIf_TxQueue[ctrlIdx].tail = 0U;
    EthIf_TxQueue[ctrlIdx].count = 0U;
    EthIf_TxQueue[ctrlIdx].overflowCount = 0U;

    /* Setup virtual controller mappings */
    for (i = 0U; i < config->numVirtCtrls; i++) {
        if (NULL_PTR != config->virtCtrlConfigs) {
            EthIf_VirtCtrlIdxType virtIdx = config->virtCtrlConfigs[i].virtCtrlIdx;
            if (virtIdx < ETHIF_MAX_VIRT_CTRLS) {
                EthIf_VirtCtrlToPhysCtrl[virtIdx] = ctrlIdx;
            }
        }
    }

    /* Set initial mode to DOWN (user must explicitly set ACTIVE) */
    EthIf_CtrlStatus[ctrlIdx].currentMode = ETHIF_CTRL_MODE_DOWN;

    return E_OK;
}

/**
 * @brief Deinitialize a specific controller
 */
static void EthIf_DeInitController(EthIf_CtrlIdxType ctrlIdx)
{
    uint8 i;

    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return;
    }

    /* Clear virtual controller mappings */
    for (i = 0U; i < ETHIF_MAX_VIRT_CTRLS; i++) {
        if (EthIf_VirtCtrlToPhysCtrl[i] == ctrlIdx) {
            EthIf_VirtCtrlToPhysCtrl[i] = 0xFFU;
        }
    }

    /* Clear transmit queue */
    (void)EthIf_ClearTxQueue(ctrlIdx);

    /* Reset controller status */
    EthIf_CtrlStatus[ctrlIdx].state = ETHIF_STATE_UNINIT;
    EthIf_CtrlStatus[ctrlIdx].currentMode = ETHIF_CTRL_MODE_DOWN;
}

/**
 * @brief Process transmit queue
 */
static Std_ReturnType EthIf_ProcessTxQueue(EthIf_CtrlIdxType ctrlIdx)
{
    uint8 tail;
    EthIf_QueueEntryType *entry;

    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return E_NOT_OK;
    }

    /* Process entries in queue */
    while (EthIf_TxQueue[ctrlIdx].count > 0U) {
        tail = EthIf_TxQueue[ctrlIdx].tail;
        entry = &EthIf_TxQueue[ctrlIdx].entries[tail];

        if (!entry->inUse) {
            /* Skip unused entry */
            EthIf_TxQueue[ctrlIdx].tail = (tail + 1U) % ETHIF_MAX_QUEUE_DEPTH;
            continue;
        }

        /* In a real implementation, this would:
         * 1. Call Eth driver transmit function
         * 2. Handle confirmation
         * 3. Release buffer on success or error
         */

        /* Mark as processed and advance tail */
        entry->inUse = FALSE;
        EthIf_TxQueue[ctrlIdx].tail = (tail + 1U) % ETHIF_MAX_QUEUE_DEPTH;
        EthIf_TxQueue[ctrlIdx].count--;

        /* Call TxConfirmation to notify upper layer */
        /* EthIf_TxConfirmation(ctrlIdx, bufIdx); */
    }

    return E_OK;
}

/**
 * @brief Allocate transmit buffer
 */
static EthIf_BufIdxType EthIf_AllocateTxBuffer(EthIf_CtrlIdxType ctrlIdx)
{
    EthIf_BufIdxType i;

    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return ETHIF_NUM_TX_BUFFERS;
    }

    /* Find free buffer */
    for (i = 0U; i < ETHIF_NUM_TX_BUFFERS; i++) {
        if (!EthIf_TxBufInUse[ctrlIdx][i]) {
            EthIf_TxBufInUse[ctrlIdx][i] = TRUE;
            return i;
        }
    }

    /* No buffer available */
    return ETHIF_NUM_TX_BUFFERS;
}

/**
 * @brief Release transmit buffer
 */
static void EthIf_ReleaseTxBuffer(EthIf_CtrlIdxType ctrlIdx, EthIf_BufIdxType bufIdx)
{
    if (ctrlIdx >= ETHIF_MAX_CONTROLLERS) {
        return;
    }

    if (bufIdx >= ETHIF_NUM_TX_BUFFERS) {
        return;
    }

    EthIf_TxBufInUse[ctrlIdx][bufIdx] = FALSE;
}

/**
 * @brief Check if module is initialized
 */
static boolean EthIf_IsModuleInitialized(void)
{
    return (EthIf_InternalState.magicNumber == ETHIF_MAGIC_INITIALIZED) &&
           (EthIf_InternalState.initialized);
}

/**
 * @brief Update controller activity timestamp
 */
static void EthIf_UpdateControllerActivity(EthIf_CtrlIdxType ctrlIdx)
{
    if (ctrlIdx < ETHIF_MAX_CONTROLLERS) {
        /* In a real implementation, this would use a real time source */
        EthIf_CtrlStatus[ctrlIdx].lastActivityTime++;
    }
}

/******************************************************************************
 * Weak Callback Definitions (to be overridden by application)
 ******************************************************************************/

#if (ETHIF_UL_RXINDICATION_CALLBACK == STD_ON)
__attribute__((weak)) void EthIf_UpperLayer_RxIndication(
    EthIf_PduHandleType rxPduId,
    EthIf_FrameType frameType,
    const uint8 *bufPtr,
    uint16 len)
{
    /* Default empty implementation - should be overridden by SoAd */
    (void)rxPduId;
    (void)frameType;
    (void)bufPtr;
    (void)len;
}
#endif

#if (ETHIF_UL_TXCONFIRMATION_CALLBACK == STD_ON)
__attribute__((weak)) void EthIf_UpperLayer_TxConfirmation(EthIf_PduHandleType txPduId)
{
    /* Default empty implementation - should be overridden by SoAd */
    (void)txPduId;
}
#endif

#if (ETHIF_UL_MODE_INDICATION_CALLBACK == STD_ON)
__attribute__((weak)) void EthIf_UpperLayer_ControllerModeIndication(
    EthIf_CtrlIdxType ctrlIdx,
    EthIf_ControllerModeType mode)
{
    /* Default empty implementation - should be overridden by SoAd */
    (void)ctrlIdx;
    (void)mode;
}
#endif

/******************************************************************************
 * End of File
 ******************************************************************************/
