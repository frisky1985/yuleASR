/**
 * @file Eth_Private.h
 * @brief Ethernet Driver Private Header
 * @version 1.0.0
 * 
 * Private definitions for Eth module internal use.
 * 
 * @copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef ETH_PRIVATE_H
#define ETH_PRIVATE_H

#include "Eth.h"
#include "Eth_Cfg.h"
#include "Eth_Lcfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                              DEBUGGING AND ERROR HANDLING
==================================================================================================*/

#if (ETH_DEV_ERROR_DETECT == STD_ON)
    #include "Det.h"
    #define ETH_REPORT_ERROR(ApiId, ErrorId) \
        (void)Det_ReportError(ETH_MODULE_ID, ETH_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define ETH_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                              STATE CHECK MACROS
==================================================================================================*/
#define ETH_CHECK_STATE_INIT(ApiId) \
    do { \
        if (Eth_InternalState.ModuleState == ETH_STATE_UNINIT) { \
            ETH_REPORT_ERROR((ApiId), ETH_E_NOT_INITIALIZED); \
            return E_NOT_OK; \
        } \
    } while(0)

#define ETH_CHECK_STATE_INIT_VOID(ApiId) \
    do { \
        if (Eth_InternalState.ModuleState == ETH_STATE_UNINIT) { \
            ETH_REPORT_ERROR((ApiId), ETH_E_NOT_INITIALIZED); \
            return; \
        } \
    } while(0)

#define ETH_CHECK_CONTROLLER_VALID(CtrlIdx, ApiId) \
    do { \
        if ((CtrlIdx) >= ETH_MAX_CONTROLLERS) { \
            ETH_REPORT_ERROR((ApiId), ETH_E_INV_CTRL_INDEX); \
            return E_NOT_OK; \
        } \
    } while(0)

#define ETH_CHECK_CONTROLLER_VALID_VOID(CtrlIdx, ApiId) \
    do { \
        if ((CtrlIdx) >= ETH_MAX_CONTROLLERS) { \
            ETH_REPORT_ERROR((ApiId), ETH_E_INV_CTRL_INDEX); \
            return; \
        } \
    } while(0)

#define ETH_CHECK_POINTER_VALID(Ptr, ApiId) \
    do { \
        if ((Ptr) == NULL_PTR) { \
            ETH_REPORT_ERROR((ApiId), ETH_E_INV_POINTER); \
            return E_NOT_OK; \
        } \
    } while(0)

#define ETH_CHECK_POINTER_VALID_VOID(Ptr, ApiId) \
    do { \
        if ((Ptr) == NULL_PTR) { \
            ETH_REPORT_ERROR((ApiId), ETH_E_INV_POINTER); \
            return; \
        } \
    } while(0)

/*==================================================================================================
*                              BUFFER STATES
==================================================================================================*/
typedef uint8 Eth_BufStateType;
#define ETH_BUF_STATE_FREE                 0x00u
#define ETH_BUF_STATE_BUSY                 0x01u
#define ETH_BUF_STATE_READY                0x02u
#define ETH_BUF_STATE_TRANSMITTING         0x03u

/*==================================================================================================
/* TX BUFFER DESCRIPTOR
==================================================================================================*/
typedef struct {
    uint8* DataPtr;                    /* Pointer to buffer data */
    uint16 Len;                        /* Data length */
    Eth_BufIdxType BufIdx;             /* Buffer index */
    Eth_BufStateType State;            /* Buffer state */
    boolean TxConfirmation;            /* Tx confirmation required */
    Eth_FrameIdType FrameType;         /* Frame type for this buffer */
    Eth_TimeStampType TimeStamp;       /* Transmission timestamp */
} Eth_TxDescType;

/*==================================================================================================
*                              RX BUFFER DESCRIPTOR
==================================================================================================*/
typedef struct {
    uint8* DataPtr;                    /* Pointer to buffer data */
    uint16 Len;                        /* Data length */
    Eth_BufStateType State;            /* Buffer state */
    Eth_FrameIdType FrameType;           /* Received frame type */
    Eth_TimeStampType TimeStamp;       /* Reception timestamp */
    Eth_MacAddrType SrcMacAddr;        /* Source MAC address */
    Eth_MacAddrType DestMacAddr;       /* Destination MAC address */
} Eth_RxDescType;

/*==================================================================================================
*                              CONTROLLER STATE STRUCTURE
==================================================================================================*/
typedef struct {
    Eth_StateType State;               /* Module state */
    Eth_ModeType Mode;                 /* Current mode */
    uint8 CtrlIdx;                     /* Controller index */
    boolean InitDone;                  /* Initialization complete flag */
    const Eth_ControllerConfigType* ConfigPtr;  /* Pointer to configuration */
    Eth_TxDescType* TxDesc;            /* TX descriptor array */
    Eth_RxDescType* RxDesc;            /* RX descriptor array */
    uint8 TxBufCount;                  /* Number of TX buffers */
    uint8 RxBufCount;                  /* Number of RX buffers */
    uint32 TxBufSize;                  /* Size of each TX buffer */
    uint32 RxBufSize;                  /* Size of each RX buffer */
    volatile uint32 TxPendingCount;    /* Pending TX frames */
    volatile uint32 RxPendingCount;    /* Pending RX frames */
    boolean InterruptsEnabled;         /* Interrupt state */
} Eth_CtrlStateType;

/*==================================================================================================
*                              MODULE INTERNAL STATE
==================================================================================================*/
typedef struct {
    Eth_StateType ModuleState;         /* Global module state */
    boolean Initialized;               /* Global initialized flag */
    uint8 NumControllers;              /* Number of configured controllers */
    Eth_CtrlStateType CtrlState[ETH_MAX_CONTROLLERS];  /* Per-controller state */
} Eth_InternalStateType;

/*==================================================================================================
*                              HARDWARE REGISTER DEFINITIONS
==================================================================================================*/

/* MAC Control Register Definitions */
#define ETH_MAC_CR                         0x0000u  /* MAC Configuration */
#define ETH_MAC_FFR                        0x0004u  /* MAC Frame Filter */
#define ETH_MAC_HTHR                       0x0008u  /* MAC Hash Table High */
#define ETH_MAC_HTLR                       0x000Cu  /* MAC Hash Table Low */
#define ETH_MAC_MIIAR                      0x0010u  /* MII Address */
#define ETH_MAC_MIIDR                      0x0014u  /* MII Data */
#define ETH_MAC_FCR                        0x0018u  /* Flow Control */
#define ETH_MAC_VTR                        0x001Cu  /* VLAN Tag */

/* DMA Register Definitions */
#define ETH_DMA_BMR                        0x1000u  /* Bus Mode */
#define ETH_DMA_TPDR                       0x1004u  /* Transmit Poll Demand */
#define ETH_DMA_RPDR                       0x1008u  /* Receive Poll Demand */
#define ETH_DMA_RDLAR                      0x100Cu  /* Receive Descriptor List Address */
#define ETH_DMA_TDLAR                      0x1010u  /* Transmit Descriptor List Address */
#define ETH_DMA_SR                         0x1014u  /* Status */
#define ETH_DMA_OMR                        0x1018u  /* Operation Mode */
#define ETH_DMA_IER                        0x101Cu  /* Interrupt Enable */

/*==================================================================================================
*                              MAC CONTROL BITS
==================================================================================================*/
#define ETH_MAC_CR_RE                      0x00000004u  /* Receiver Enable */
#define ETH_MAC_CR_TE                      0x00000008u  /* Transmitter Enable */
#define ETH_MAC_CR_DM                      0x00008000u  /* Duplex Mode */
#define ETH_MAC_CR_LM                      0x00001000u  /* Loopback Mode */
#define ETH_MAC_CR_FES                     0x00004000u  /* Fast Ethernet Speed (100Mbps) */

/*==================================================================================================
*                              DMA STATUS BITS
==================================================================================================*/
#define ETH_DMA_SR_TS                      0x00600000u  /* Transmit Process State */
#define ETH_DMA_SR_RS                      0x000E0000u  /* Receive Process State */
#define ETH_DMA_SR_NIS                     0x00010000u  /* Normal Interrupt Summary */
#define ETH_DMA_SR_AIS                     0x00008000u  /* Abnormal Interrupt Summary */
#define ETH_DMA_SR_ERI                     0x00004000u  /* Early Receive Interrupt */
#define ETH_DMA_SR_FBE                     0x00002000u  /* Fatal Bus Error */
#define ETH_DMA_SR_ETI                     0x00000400u  /* Early Transmit Interrupt */
#define ETH_DMA_SR_RWT                     0x00000200u  /* Receive Watchdog Timeout */
#define ETH_DMA_SR_RPS                     0x00000100u  /* Receive Process Stopped */
#define ETH_DMA_SR_RU                      0x00000080u  /* Receive Buffer Unavailable */
#define ETH_DMA_SR_RI                      0x00000040u  /* Receive Interrupt */
#define ETH_DMA_SR_UNF                     0x00000020u  /* Transmit Underflow */
#define ETH_DMA_SR_OVF                     0x00000010u  /* Receive Overflow */
#define ETH_DMA_SR_TJT                     0x00000008u  /* Transmit Jabber Timeout */
#define ETH_DMA_SR_TU                      0x00000004u  /* Transmit Buffer Unavailable */
#define ETH_DMA_SR_TPS                     0x00000002u  /* Transmit Process Stopped */
#define ETH_DMA_SR_TI                      0x00000001u  /* Transmit Interrupt */

/*==================================================================================================
*                              EXTERNAL VARIABLES
==================================================================================================*/

#define ETH_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

extern Eth_InternalStateType Eth_InternalState;
extern Eth_CtrlStateType Eth_CtrlState[ETH_MAX_CONTROLLERS];
extern Eth_TxDescType Eth_TxDesc[ETH_MAX_CONTROLLERS][ETH_MAX_TX_BUFS];
extern Eth_RxDescType Eth_RxDesc[ETH_MAX_CONTROLLERS][ETH_MAX_RX_BUFS];

#define ETH_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                              INTERNAL FUNCTION PROTOTYPES
==================================================================================================*/

/* Hardware interface functions */
static Std_ReturnType Eth_HwInit(Eth_ControllerType CtrlIdx, const Eth_ControllerConfigType* CfgPtr);
static Std_ReturnType Eth_HwDeInit(Eth_ControllerType CtrlIdx);
static Std_ReturnType Eth_HwSetMode(Eth_ControllerType CtrlIdx, Eth_ModeType Mode);
static Std_ReturnType Eth_HwWriteMii(Eth_ControllerType CtrlIdx, Eth_PhyAddrType PhyAddr, Eth_RegAddrType RegAddr, Eth_DataType Data);
static Std_ReturnType Eth_HwReadMii(Eth_ControllerType CtrlIdx, Eth_PhyAddrType PhyAddr, Eth_RegAddrType RegAddr, Eth_DataType* DataPtr);
static Std_ReturnType Eth_HwTransmit(Eth_ControllerType CtrlIdx, Eth_BufIdxType BufIdx, uint16 Len);
static Std_ReturnType Eth_HwReceive(Eth_ControllerType CtrlIdx, Eth_BufIdxType* BufIdxPtr, uint16* LenPtr);

/* Buffer management functions */
static void Eth_InitTxBuffers(Eth_ControllerType CtrlIdx);
static void Eth_InitRxBuffers(Eth_ControllerType CtrlIdx);
static Eth_BufIdxType Eth_AllocateTxBuffer(Eth_ControllerType CtrlIdx, uint16 Len);
static void Eth_FreeTxBuffer(Eth_ControllerType CtrlIdx, Eth_BufIdxType BufIdx);
static void Eth_FreeRxBuffer(Eth_ControllerType CtrlIdx, Eth_BufIdxType BufIdx);

/* Utility functions */
static void Eth_UpdateMacAddress(Eth_ControllerType CtrlIdx, const uint8* MacAddr);
static boolean Eth_ValidateFrame(const uint8* DataPtr, uint16 Len);

#ifdef __cplusplus
}
#endif

#endif /* ETH_PRIVATE_H */
