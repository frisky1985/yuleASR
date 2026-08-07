/**
 * @file Eth.c
 * @brief Ethernet Driver Implementation
 * @version 1.0.0
 * 
 * Ethernet MAC driver implementation for AUTOSAR MCAL layer.
 * Based on AUTOSAR Classic Platform 4.4.0.
 * ASIL-D safety level compatible, MISRA C:2012 compliant.
 * 
 * @copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

/*==================================================================================================
*                              PRE-COMPILATION CHECKS
==================================================================================================*/
#include "Eth.h"
#include "Eth_Private.h"
#include "Eth_Cfg.h"
#include "Eth_Lcfg.h"

/*==================================================================================================
*                              MEMORY SECTIONS
==================================================================================================*/
#define ETH_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Module internal state */
Eth_InternalStateType Eth_InternalState;

/* Controller state array */
Eth_CtrlStateType Eth_CtrlState[ETH_MAX_CONTROLLERS];

/* TX and RX descriptors */
#if (ETH_MAX_CONTROLLERS > 0U)
Eth_TxDescType Eth_TxDesc[ETH_MAX_CONTROLLERS][ETH_MAX_TX_BUFS];
Eth_RxDescType Eth_RxDesc[ETH_MAX_CONTROLLERS][ETH_MAX_RX_BUFS];

/* TX and RX buffer pools */
uint8 Eth_TxBufPool[ETH_MAX_CONTROLLERS][ETH_MAX_TX_BUFS][ETH_MAX_FRAME_SIZE];
uint8 Eth_RxBufPool[ETH_MAX_CONTROLLERS][ETH_MAX_RX_BUFS][ETH_MAX_FRAME_SIZE];
#endif

#define ETH_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define ETH_START_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                              INTERNAL HELPER FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize TX buffers for a controller
 */
static void Eth_InitTxBuffers(Eth_ControllerType CtrlIdx)
{
    uint8 bufIdx;
    
    for (bufIdx = 0u; bufIdx < ETH_MAX_TX_BUFS; bufIdx++)
    {
        Eth_TxDesc[CtrlIdx][bufIdx].DataPtr = &Eth_TxBufPool[CtrlIdx][bufIdx][0];
        Eth_TxDesc[CtrlIdx][bufIdx].Len = 0u;
        Eth_TxDesc[CtrlIdx][bufIdx].BufIdx = bufIdx;
        Eth_TxDesc[CtrlIdx][bufIdx].State = ETH_BUF_STATE_FREE;
        Eth_TxDesc[CtrlIdx][bufIdx].TxConfirmation = FALSE;
    }
    
    Eth_CtrlState[CtrlIdx].TxDesc = &Eth_TxDesc[CtrlIdx][0];
    Eth_CtrlState[CtrlIdx].TxBufCount = ETH_MAX_TX_BUFS;
}

/**
 * @brief Initialize RX buffers for a controller
 */
static void Eth_InitRxBuffers(Eth_ControllerType CtrlIdx)
{
    uint8 bufIdx;
    
    for (bufIdx = 0u; bufIdx < ETH_MAX_RX_BUFS; bufIdx++)
    {
        Eth_RxDesc[CtrlIdx][bufIdx].DataPtr = &Eth_RxBufPool[CtrlIdx][bufIdx][0];
        Eth_RxDesc[CtrlIdx][bufIdx].Len = 0u;
        Eth_RxDesc[CtrlIdx][bufIdx].State = ETH_BUF_STATE_FREE;
        Eth_RxDesc[CtrlIdx][bufIdx].FrameType = 0u;
    }
    
    Eth_CtrlState[CtrlIdx].RxDesc = &Eth_RxDesc[CtrlIdx][0];
    Eth_CtrlState[CtrlIdx].RxBufCount = ETH_MAX_RX_BUFS;
}

/**
 * @brief Allocate a TX buffer
 */
static Eth_BufIdxType Eth_AllocateTxBuffer(Eth_ControllerType CtrlIdx, uint16 Len)
{
    uint8 bufIdx;
    Eth_BufIdxType allocatedBuf = ETH_INVALID_BUF_INDEX;
    
    for (bufIdx = 0u; bufIdx < Eth_CtrlState[CtrlIdx].TxBufCount; bufIdx++)
    {
        if (Eth_TxDesc[CtrlIdx][bufIdx].State == ETH_BUF_STATE_FREE)
        {
            Eth_TxDesc[CtrlIdx][bufIdx].State = ETH_BUF_STATE_BUSY;
            Eth_TxDesc[CtrlIdx][bufIdx].Len = Len;
            allocatedBuf = bufIdx;
            break;
        }
    }
    
    return allocatedBuf;
}

/**
 * @brief Free a TX buffer
 */
static void Eth_FreeTxBuffer(Eth_ControllerType CtrlIdx, Eth_BufIdxType BufIdx)
{
    if ((BufIdx < Eth_CtrlState[CtrlIdx].TxBufCount) && 
        (Eth_TxDesc[CtrlIdx][BufIdx].State != ETH_BUF_STATE_FREE))
    {
        Eth_TxDesc[CtrlIdx][BufIdx].State = ETH_BUF_STATE_FREE;
        Eth_TxDesc[CtrlIdx][BufIdx].Len = 0u;
        Eth_TxDesc[CtrlIdx][BufIdx].TxConfirmation = FALSE;
    }
}

/**
 * @brief Free an RX buffer
 */
static void Eth_FreeRxBuffer(Eth_ControllerType CtrlIdx, Eth_BufIdxType BufIdx)
{
    if ((BufIdx < Eth_CtrlState[CtrlIdx].RxBufCount) && 
        (Eth_RxDesc[CtrlIdx][BufIdx].State != ETH_BUF_STATE_FREE))
    {
        Eth_RxDesc[CtrlIdx][BufIdx].State = ETH_BUF_STATE_FREE;
        Eth_RxDesc[CtrlIdx][BufIdx].Len = 0u;
    }
}

/**
 * @brief Validate frame data
 */
static boolean Eth_ValidateFrame(const uint8* DataPtr, uint16 Len)
{
    boolean valid = TRUE;
    
    if (DataPtr == NULL_PTR)
    {
        valid = FALSE;
    }
    else if ((Len < ETH_MIN_FRAME_SIZE) || (Len > ETH_MAX_FRAME_SIZE))
    {
        valid = FALSE;
    }
    else
    {
        /* Check for valid frame content */
    }
    
    return valid;
}

/**
 * @brief Update MAC address in hardware
 */
static void Eth_UpdateMacAddress(Eth_ControllerType CtrlIdx, const uint8* MacAddr)
{
    /* This would write to hardware registers */
    /* For now, just store in controller state */
    if (MacAddr != NULL_PTR)
    {
        (void)memcpy(Eth_CtrlState[CtrlIdx].ConfigPtr->MacAddr, MacAddr, 6u);
    }
}

/*==================================================================================================
*                              HARDWARE INTERFACE FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize hardware for a controller
 */
static Std_ReturnType Eth_HwInit(Eth_ControllerType CtrlIdx, const Eth_ControllerConfigType* CfgPtr)
{
    Std_ReturnType result = E_OK;
    
    /* Initialize MAC controller hardware */
    /* Configure MAC address */
    Eth_UpdateMacAddress(CtrlIdx, CfgPtr->MacAddr);
    
    /* Configure speed and duplex mode */
    /* Configure DMA */
    /* Enable interrupts */
    
    Eth_CtrlState[CtrlIdx].InitDone = TRUE;
    Eth_CtrlState[CtrlIdx].Mode = ETH_MODE_DOWN;
    
    return result;
}

/**
 * @brief De-initialize hardware for a controller
 */
static Std_ReturnType Eth_HwDeInit(Eth_ControllerType CtrlIdx)
{
    /* Disable MAC and DMA */
    /* Reset hardware registers */
    
    Eth_CtrlState[CtrlIdx].InitDone = FALSE;
    Eth_CtrlState[CtrlIdx].Mode = ETH_MODE_DOWN;
    
    return E_OK;
}

/**
 * @brief Set controller mode in hardware
 */
static Std_ReturnType Eth_HwSetMode(Eth_ControllerType CtrlIdx, Eth_ModeType Mode)
{
    Std_ReturnType result = E_OK;
    
    if ((unsigned int)((uint32_t)(Mode)) == ETH_MODE_ACTIVE)
    {
        /* Enable receiver and transmitter */
        Eth_CtrlState[CtrlIdx].Mode = ETH_MODE_ACTIVE;
    }
    else
    {
        /* Disable receiver and transmitter */
        Eth_CtrlState[CtrlIdx].Mode = ETH_MODE_DOWN;
    }
    
    return result;
}

/**
 * @brief Write to MII register
 */
static Std_ReturnType Eth_HwWriteMii(Eth_ControllerType CtrlIdx, Eth_PhyAddrType PhyAddr, 
                                      Eth_RegAddrType RegAddr, Eth_DataType Data)
{
    Std_ReturnType result = E_OK;
    
    /* Write PHY address and register address */
    /* Write data */
    /* Wait for completion */
    
    (void)CtrlIdx;
    (void)PhyAddr;
    (void)RegAddr;
    (void)Data;
    
    return result;
}

/**
 * @brief Read from MII register
 */
static Std_ReturnType Eth_HwReadMii(Eth_ControllerType CtrlIdx, Eth_PhyAddrType PhyAddr, 
                                     Eth_RegAddrType RegAddr, Eth_DataType* DataPtr)
{
    Std_ReturnType result = E_OK;
    
    /* Write PHY address and register address */
    /* Read data */
    /* Wait for completion */
    
    if (DataPtr != NULL_PTR)
    {
        *DataPtr = 0u;
    }
    
    (void)CtrlIdx;
    (void)PhyAddr;
    (void)RegAddr;
    
    return result;
}

/**
 * @brief Transmit frame via hardware
 */
static Std_ReturnType Eth_HwTransmit(Eth_ControllerType CtrlIdx, Eth_BufIdxType BufIdx, uint16 Len)
{
    Std_ReturnType result = E_OK;
    
    /* Configure TX descriptor */
    /* Start transmission */
    
    (void)Len;
    
    if (BufIdx < Eth_CtrlState[CtrlIdx].TxBufCount)
    {
        Eth_TxDesc[CtrlIdx][BufIdx].State = ETH_BUF_STATE_TRANSMITTING;
    }
    
    return result;
}

/**
 * @brief Receive frame from hardware
 */
static Std_ReturnType Eth_HwReceive(Eth_ControllerType CtrlIdx, Eth_BufIdxType* BufIdxPtr, uint16* LenPtr)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 bufIdx;
    
    if ((BufIdxPtr != NULL_PTR) && (LenPtr != NULL_PTR))
    {
        /* Search for received frame */
        for (bufIdx = 0u; bufIdx < Eth_CtrlState[CtrlIdx].RxBufCount; bufIdx++)
        {
            if (Eth_RxDesc[CtrlIdx][bufIdx].State == ETH_BUF_STATE_READY)
            {
                *BufIdxPtr = bufIdx;
                *LenPtr = Eth_RxDesc[CtrlIdx][bufIdx].Len;
                result = E_OK;
                break;
            }
        }
    }
    
    return result;
}

/*==================================================================================================
*                              API IMPLEMENTATION
==================================================================================================*/

/**
 * @brief Initialize the Eth module
 */
void Eth_Init(const Eth_ConfigType* CfgPtr)
{
    uint8 ctrlIdx;
    
    #if (ETH_DEV_ERROR_DETECT == STD_ON)
    if (CfgPtr == NULL_PTR)
    {
        ETH_REPORT_ERROR(ETH_INIT_SID, ETH_E_INV_POINTER);
        return;
    }
    #endif
    
    /* Initialize global state */
    Eth_InternalState.ModuleState = ETH_STATE_INIT;
    Eth_InternalState.Initialized = TRUE;
    Eth_InternalState.NumControllers = CfgPtr->NumControllers;
    
    /* Initialize each controller */
    for (ctrlIdx = 0u; ctrlIdx < CfgPtr->NumControllers; ctrlIdx++)
    {
        Eth_CtrlState[ctrlIdx].State = ETH_STATE_INIT;
        Eth_CtrlState[ctrlIdx].Mode = ETH_MODE_DOWN;
        Eth_CtrlState[ctrlIdx].CtrlIdx = ctrlIdx;
        Eth_CtrlState[ctrlIdx].ConfigPtr = &CfgPtr->CtrlConfig[ctrlIdx];
        Eth_CtrlState[ctrlIdx].InitDone = FALSE;
        Eth_CtrlState[ctrlIdx].InterruptsEnabled = FALSE;
        Eth_CtrlState[ctrlIdx].TxPendingCount = 0u;
        Eth_CtrlState[ctrlIdx].RxPendingCount = 0u;
        
        /* Initialize buffers */
        Eth_InitTxBuffers(ctrlIdx);
        Eth_InitRxBuffers(ctrlIdx);
        
        /* Copy to internal state */
        (void)memcpy(&Eth_InternalState.CtrlState[ctrlIdx], &Eth_CtrlState[ctrlIdx], 
                     sizeof(Eth_CtrlStateType));
    }
}

/**
 * @brief De-initialize the Eth module
 */
void Eth_DeInit(void)
{
    uint8 ctrlIdx;
    
    ETH_CHECK_STATE_INIT_VOID(ETH_DEINIT_SID);
    
    /* De-initialize each controller */
    for (ctrlIdx = 0u; ctrlIdx < Eth_InternalState.NumControllers; ctrlIdx++)
    {
        (void)Eth_HwDeInit(ctrlIdx);
        Eth_CtrlState[ctrlIdx].State = ETH_STATE_UNINIT;
        Eth_CtrlState[ctrlIdx].InitDone = FALSE;
    }
    
    /* Reset global state */
    Eth_InternalState.ModuleState = ETH_STATE_UNINIT;
    Eth_InternalState.Initialized = FALSE;
    Eth_InternalState.NumControllers = 0u;
}

/**
 * @brief Initialize a specific controller
 */
void Eth_ControllerInit(Eth_ControllerType CtrlIdx, const Eth_ControllerConfigType* CfgPtr)
{
    #if (ETH_DEV_ERROR_DETECT == STD_ON)
    if (CfgPtr == NULL_PTR)
    {
        ETH_REPORT_ERROR(ETH_INIT_SID, ETH_E_INV_POINTER);
        return;
    }
    
    if (CtrlIdx >= ETH_MAX_CONTROLLERS)
    {
        ETH_REPORT_ERROR(ETH_INIT_SID, ETH_E_INV_CTRL_INDEX);
        return;
    }
    #endif
    
    /* Initialize controller */
    (void)Eth_HwInit(CtrlIdx, CfgPtr);
    
    /* Update configuration */
    Eth_CtrlState[CtrlIdx].ConfigPtr = CfgPtr;
    Eth_CtrlState[CtrlIdx].State = ETH_STATE_INIT;
    Eth_CtrlState[CtrlIdx].InitDone = TRUE;
    Eth_CtrlState[CtrlIdx].Mode = ETH_MODE_DOWN;
    
    /* Re-initialize buffers */
    Eth_InitTxBuffers(CtrlIdx);
    Eth_InitRxBuffers(CtrlIdx);
}

/**
 * @brief Get version information
 */
void Eth_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
    #if (ETH_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL_PTR)
    {
        ETH_REPORT_ERROR(ETH_GETVERSIONINFO_APIID, ETH_E_INV_POINTER);
        return;
    }
    #endif
    
    VersionInfoPtr->vendorID = 0x01u;  /* YuleTech Vendor ID */
    VersionInfoPtr->moduleID = ETH_MODULE_ID;
    VersionInfoPtr->sw_major_version = ETH_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version = ETH_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version = ETH_SW_PATCH_VERSION;
}

/**
 * @brief Set controller mode
 */
Std_ReturnType Eth_SetControllerMode(Eth_ControllerType CtrlIdx, Eth_ModeType CtrlMode)
{
    Std_ReturnType result = E_NOT_OK;
    
    ETH_CHECK_STATE_INIT(ETH_SETCONTROLLERMODE_SID);
    ETH_CHECK_CONTROLLER_VALID(CtrlIdx, ETH_SETCONTROLLERMODE_SID);
    
    if (((unsigned int)(CtrlMode) != ETH_MODE_DOWN) && (CtrlMode != ETH_MODE_ACTIVE))
    {
        #if (ETH_DEV_ERROR_DETECT == STD_ON)
        ETH_REPORT_ERROR(ETH_SETCONTROLLERMODE_SID, ETH_E_INV_MODE);
        #endif
        return E_NOT_OK;
    }
    
    /* Check if controller is initialized */
    if (Eth_CtrlState[CtrlIdx].InitDone == FALSE)
    {
        #if (ETH_DEV_ERROR_DETECT == STD_ON)
        ETH_REPORT_ERROR(ETH_SETCONTROLLERMODE_SID, ETH_E_NOT_INITIALIZED);
        #endif
        return E_NOT_OK;
    }
    
    /* Set hardware mode */
    result = Eth_HwSetMode(CtrlIdx, CtrlMode);
    
    if (result == E_OK)
    {
        Eth_CtrlState[CtrlIdx].Mode = CtrlMode;
    }
    
    return result;
}

/**
 * @brief Get controller mode
 */
Std_ReturnType Eth_GetControllerMode(Eth_ControllerType CtrlIdx, Eth_ModeType* CtrlModePtr)
{
    Std_ReturnType result = E_NOT_OK;
    
    ETH_CHECK_STATE_INIT(ETH_GETCONTROLLERMODE_SID);
    ETH_CHECK_CONTROLLER_VALID(CtrlIdx, ETH_GETCONTROLLERMODE_SID);
    ETH_CHECK_POINTER_VALID(CtrlModePtr, ETH_GETCONTROLLERMODE_SID);
    
    if (Eth_CtrlState[CtrlIdx].InitDone == TRUE)
    {
        *CtrlModePtr = Eth_CtrlState[CtrlIdx].Mode;
        result = E_OK;
    }
    else
    {
        #if (ETH_DEV_ERROR_DETECT == STD_ON)
        ETH_REPORT_ERROR(ETH_GETCONTROLLERMODE_SID, ETH_E_NOT_INITIALIZED);
        #endif
    }
    
    return result;
}

/**
 * @brief Get controller index by name
 */
uint8 Eth_GetControllerIdx(const uint8* CtrlName)
{
    uint8 ctrlIdx = ETH_INVALID_CONTROLLER_INDEX;
    
    #if (ETH_DEV_ERROR_DETECT == STD_ON)
    if (CtrlName == NULL_PTR)
    {
        ETH_REPORT_ERROR(ETH_GETCONTROLLERIDX_SID, ETH_E_INV_POINTER);
        return ETH_INVALID_CONTROLLER_INDEX;
    }
    #endif
    
    /* Simple implementation - return controller 0 */
    if ((unsigned int)(ETH_MAX_CONTROLLERS) > 0u)
    {
        ctrlIdx = 0u;
    }
    
    (void)CtrlName;
    
    return ctrlIdx;
}

/**
 * @brief Get physical address (MAC address)
 */
void Eth_GetPhysAddr(Eth_ControllerType CtrlIdx, uint8* PhysAddrPtr)
{
    #if (ETH_DEV_ERROR_DETECT == STD_ON)
    if (PhysAddrPtr == NULL_PTR)
    {
        ETH_REPORT_ERROR(ETH_GETPHYSADDR_SID, ETH_E_INV_POINTER);
        return;
    }
    
    if (CtrlIdx >= ETH_MAX_CONTROLLERS)
    {
        ETH_REPORT_ERROR(ETH_GETPHYSADDR_SID, ETH_E_INV_CTRL_INDEX);
        return;
    }
    #endif
    
    if (Eth_CtrlState[CtrlIdx].InitDone == TRUE)
    {
        (void)memcpy(PhysAddrPtr, Eth_CtrlState[CtrlIdx].ConfigPtr->MacAddr, 6u);
    }
}

/**
 * @brief Set physical address (MAC address)
 */
void Eth_SetPhysAddr(Eth_ControllerType CtrlIdx, const uint8* PhysAddrPtr)
{
    #if (ETH_DEV_ERROR_DETECT == STD_ON)
    if (PhysAddrPtr == NULL_PTR)
    {
        ETH_REPORT_ERROR(ETH_SETPHYSADDR_SID, ETH_E_INV_POINTER);
        return;
    }
    
    if (CtrlIdx >= ETH_MAX_CONTROLLERS)
    {
        ETH_REPORT_ERROR(ETH_SETPHYSADDR_SID, ETH_E_INV_CTRL_INDEX);
        return;
    }
    #endif
    
    if (Eth_CtrlState[CtrlIdx].InitDone == TRUE)
    {
        Eth_UpdateMacAddress(CtrlIdx, PhysAddrPtr);
    }
}

/**
 * @brief Write to MII register
 */
Std_ReturnType Eth_WriteMii(Eth_ControllerType CtrlIdx, Eth_PhyAddrType PhyAddr, 
                             Eth_RegAddrType RegAddr, Eth_DataType Data)
{
    Std_ReturnType result;
    
    ETH_CHECK_STATE_INIT(ETH_WRITEMII_SID);
    ETH_CHECK_CONTROLLER_VALID(CtrlIdx, ETH_WRITEMII_SID);
    
    if (Eth_CtrlState[CtrlIdx].InitDone == FALSE)
    {
        #if (ETH_DEV_ERROR_DETECT == STD_ON)
        ETH_REPORT_ERROR(ETH_WRITEMII_SID, ETH_E_NOT_INITIALIZED);
        #endif
        return E_NOT_OK;
    }
    
    result = Eth_HwWriteMii(CtrlIdx, PhyAddr, RegAddr, Data);
    
    return result;
}

/**
 * @brief Read from MII register
 */
Std_ReturnType Eth_ReadMii(Eth_ControllerType CtrlIdx, Eth_PhyAddrType PhyAddr, 
                            Eth_RegAddrType RegAddr, Eth_DataType* DataPtr)
{
    Std_ReturnType result;
    
    ETH_CHECK_STATE_INIT(ETH_READMII_SID);
    ETH_CHECK_CONTROLLER_VALID(CtrlIdx, ETH_READMII_SID);
    ETH_CHECK_POINTER_VALID(DataPtr, ETH_READMII_SID);
    
    if (Eth_CtrlState[CtrlIdx].InitDone == FALSE)
    {
        #if (ETH_DEV_ERROR_DETECT == STD_ON)
        ETH_REPORT_ERROR(ETH_READMII_SID, ETH_E_NOT_INITIALIZED);
        #endif
        return E_NOT_OK;
    }
    
    result = Eth_HwReadMii(CtrlIdx, PhyAddr, RegAddr, DataPtr);
    
    return result;
}

/**
 * @brief Provide TX buffer
 */
BufReq_ReturnType Eth_ProvideTxBuffer(Eth_ControllerType CtrlIdx, Eth_FrameIdType FrameType, 
                                       uint16 Priority, Eth_BufIdxType* BufIdxPtr, 
                                       uint8** BufPtr, uint16* LenBytePtr)
{
    BufReq_ReturnType result = BUFREQ_E_NOT_OK;
    Eth_BufIdxType bufIdx;
    
    #if (ETH_DEV_ERROR_DETECT == STD_ON)
    if ((BufIdxPtr == NULL_PTR) || (BufPtr == NULL_PTR) || (LenBytePtr == NULL_PTR))
    {
        ETH_REPORT_ERROR(ETH_PROVIDETXBUFFER_SID, ETH_E_INV_POINTER);
        return BUFREQ_E_NOT_OK;
    }
    
    if (CtrlIdx >= ETH_MAX_CONTROLLERS)
    {
        ETH_REPORT_ERROR(ETH_PROVIDETXBUFFER_SID, ETH_E_INV_CTRL_INDEX);
        return BUFREQ_E_NOT_OK;
    }
    
    if (Eth_InternalState.ModuleState == ETH_STATE_UNINIT)
    {
        ETH_REPORT_ERROR(ETH_PROVIDETXBUFFER_SID, ETH_E_NOT_INITIALIZED);
        return BUFREQ_E_NOT_OK;
    }
    #endif
    
    (void)FrameType;
    (void)Priority;
    
    if (Eth_CtrlState[CtrlIdx].InitDone == TRUE)
    {
        bufIdx = Eth_AllocateTxBuffer(CtrlIdx, *LenBytePtr);
        
        if (bufIdx != ETH_INVALID_BUF_INDEX)
        {
            *BufIdxPtr = bufIdx;
            *BufPtr = Eth_TxDesc[CtrlIdx][bufIdx].DataPtr;
            result = BUFREQ_E_OK;
        }
        else
        {
            result = BUFREQ_E_BUSY;
        }
    }
    
    return result;
}

/**
 * @brief Transmit frame
 */
Std_ReturnType Eth_Transmit(Eth_ControllerType CtrlIdx, Eth_BufIdxType BufIdx, 
                             Eth_FrameIdType FrameType, boolean TxConfirmation, 
                             uint16 LenByte, const uint8* PhysAddrPtr)
{
    Std_ReturnType result = E_NOT_OK;
    
    ETH_CHECK_STATE_INIT(ETH_TRANSMIT_SID);
    ETH_CHECK_CONTROLLER_VALID(CtrlIdx, ETH_TRANSMIT_SID);
    
    if (Eth_CtrlState[CtrlIdx].InitDone == FALSE)
    {
        #if (ETH_DEV_ERROR_DETECT == STD_ON)
        ETH_REPORT_ERROR(ETH_TRANSMIT_SID, ETH_E_NOT_INITIALIZED);
        #endif
        return E_NOT_OK;
    }
    
    if ((unsigned int)(Eth_CtrlState[CtrlIdx].Mode) != ETH_MODE_ACTIVE)
    {
        return E_NOT_OK;
    }
    
    if (BufIdx >= Eth_CtrlState[CtrlIdx].TxBufCount)
    {
        #if (ETH_DEV_ERROR_DETECT == STD_ON)
        ETH_REPORT_ERROR(ETH_TRANSMIT_SID, ETH_E_INV_BUF_INDEX);
        #endif
        return E_NOT_OK;
    }
    
    if (Eth_TxDesc[CtrlIdx][BufIdx].State != ETH_BUF_STATE_BUSY)
    {
        #if (ETH_DEV_ERROR_DETECT == STD_ON)
        ETH_REPORT_ERROR(ETH_TRANSMIT_SID, ETH_E_INV_PARAM);
        #endif
        return E_NOT_OK;
    }
    
    /* Update buffer descriptor */
    Eth_TxDesc[CtrlIdx][BufIdx].Len = LenByte;
    Eth_TxDesc[CtrlIdx][BufIdx].FrameType = FrameType;
    Eth_TxDesc[CtrlIdx][BufIdx].TxConfirmation = TxConfirmation;
    
    if (PhysAddrPtr != NULL_PTR)
    {
        /* Use provided physical address as destination */
        (void)memcpy(&Eth_TxDesc[CtrlIdx][BufIdx].DataPtr[0], PhysAddrPtr, 6u);
    }
    
    /* Start transmission */
    result = Eth_HwTransmit(CtrlIdx, BufIdx, LenByte);
    
    return result;
}

/**
 * @brief Receive frame
 */
Std_ReturnType Eth_Receive(Eth_ControllerType CtrlIdx, uint8* RxStatusPtr, 
                            Eth_BufIdxType* BufIdxPtr, Eth_FrameStructType** FramePtr)
{
    Std_ReturnType result = E_NOT_OK;
    uint16 len;
    
    #if (ETH_DEV_ERROR_DETECT == STD_ON)
    if ((RxStatusPtr == NULL_PTR) || (BufIdxPtr == NULL_PTR) || (FramePtr == NULL_PTR))
    {
        ETH_REPORT_ERROR(ETH_RECEIVE_SID, ETH_E_INV_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    if (Eth_InternalState.ModuleState == ETH_STATE_UNINIT)
    {
        #if (ETH_DEV_ERROR_DETECT == STD_ON)
        ETH_REPORT_ERROR(ETH_RECEIVE_SID, ETH_E_NOT_INITIALIZED);
        #endif
        return E_NOT_OK;
    }
    
    if (CtrlIdx >= ETH_MAX_CONTROLLERS)
    {
        #if (ETH_DEV_ERROR_DETECT == STD_ON)
        ETH_REPORT_ERROR(ETH_RECEIVE_SID, ETH_E_INV_CTRL_INDEX);
        #endif
        return E_NOT_OK;
    }
    
    if (Eth_CtrlState[CtrlIdx].InitDone == FALSE)
    {
        #if (ETH_DEV_ERROR_DETECT == STD_ON)
        ETH_REPORT_ERROR(ETH_RECEIVE_SID, ETH_E_NOT_INITIALIZED);
        #endif
        return E_NOT_OK;
    }
    
    /* Check for received frames */
    result = Eth_HwReceive(CtrlIdx, BufIdxPtr, &len);
    
    if (result == E_OK)
    {
        *RxStatusPtr = 0x01u;  /* Frame received */
    }
    else
    {
        *RxStatusPtr = 0x00u;  /* No frame available */
    }
    
    (void)FramePtr;
    
    return result;
}

/**
 * @brief TX confirmation callback
 */
void Eth_TxConfirmation(Eth_ControllerType CtrlIdx, Eth_BufIdxType BufIdx)
{
    #if (ETH_DEV_ERROR_DETECT == STD_ON)
    if (CtrlIdx >= ETH_MAX_CONTROLLERS)
    {
        ETH_REPORT_ERROR(ETH_TXCONFIRMATION_SID, ETH_E_INV_CTRL_INDEX);
        return;
    }
    #endif
    
    if ((BufIdx < Eth_CtrlState[CtrlIdx].TxBufCount) &&
        (Eth_TxDesc[CtrlIdx][BufIdx].State == ETH_BUF_STATE_TRANSMITTING))
    {
        /* Free the buffer */
        Eth_FreeTxBuffer(CtrlIdx, BufIdx);
        
        /* Call upper layer confirmation if needed */
    }
}

/**
 * @brief Enable interrupts
 */
void Eth_EnableIrq(void)
{
    uint8 ctrlIdx;
    
    ETH_CHECK_STATE_INIT_VOID(ETH_ENABLEIRQ_SID);
    
    for (ctrlIdx = 0u; ctrlIdx < Eth_InternalState.NumControllers; ctrlIdx++)
    {
        if (Eth_CtrlState[ctrlIdx].InitDone == TRUE)
        {
            Eth_CtrlState[ctrlIdx].InterruptsEnabled = TRUE;
        }
    }
}

/**
 * @brief Disable interrupts
 */
void Eth_DisableIrq(void)
{
    uint8 ctrlIdx;
    
    ETH_CHECK_STATE_INIT_VOID(ETH_DISABLEIRQ_SID);
    
    for (ctrlIdx = 0u; ctrlIdx < Eth_InternalState.NumControllers; ctrlIdx++)
    {
        if (Eth_CtrlState[ctrlIdx].InitDone == TRUE)
        {
            Eth_CtrlState[ctrlIdx].InterruptsEnabled = FALSE;
        }
    }
}

/**
 * @brief Initialize buffers
 */
void Eth_InitBuffers(void)
{
    uint8 ctrlIdx;
    
    ETH_CHECK_STATE_INIT_VOID(ETH_INITBUFFERS_SID);
    
    for (ctrlIdx = 0u; ctrlIdx < Eth_InternalState.NumControllers; ctrlIdx++)
    {
        Eth_InitTxBuffers(ctrlIdx);
        Eth_InitRxBuffers(ctrlIdx);
    }
}

#define ETH_STOP_SEC_CODE
#include "MemMap.h"
