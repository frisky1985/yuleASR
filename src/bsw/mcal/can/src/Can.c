/**
 * @file Can.c
 * @brief CAN Driver implementation for i.MX8M Mini (FlexCAN)
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "Can.h"
#include "Can_Cfg.h"
#include "Det.h"

#define CAN_FLEXCAN1_BASE_ADDR          (0x308C0000UL)
#define CAN_FLEXCAN2_BASE_ADDR          (0x308D0000UL)

#define CAN_MCR                         (0x00)
#define CAN_CTRL1                       (0x04)
#define CAN_TIMER                       (0x08)
#define CAN_RXMGMASK                    (0x10)
#define CAN_RX14MASK                    (0x14)
#define CAN_RX15MASK                    (0x18)
#define CAN_ECR                         (0x1C)
#define CAN_ESR1                        (0x20)
#define CAN_IMASK2                      (0x24)
#define CAN_IMASK1                      (0x28)
#define CAN_IFLAG2                      (0x2C)
#define CAN_IFLAG1                      (0x30)
#define CAN_CTRL2                       (0x34)
#define CAN_ESR2                        (0x38)
#define CAN_CRCR                        (0x44)
#define CAN_RXFGMASK                    (0x48)
#define CAN_RXFIR                       (0x4C)
#define CAN_CBT                         (0x50)
#define CAN_MB_BASE                     (0x80)

#define CAN_MCR_MDIS                    (0x80000000U)
#define CAN_MCR_FRZ                     (0x40000000U)
#define CAN_MCR_FEN                     (0x20000000U)
#define CAN_MCR_HALT                    (0x10000000U)
#define CAN_MCR_NOT_RDY                 (0x08000000U)
#define CAN_MCR_SOFTRST                 (0x02000000U)
#define CAN_MCR_SUPV                    (0x00800000U)
#define CAN_MCR_MAXMB_MASK              (0x0000007FU)

#define CAN_CTRL1_PRESDIV_MASK          (0xFF000000U)
#define CAN_CTRL1_RJW_MASK              (0x00C00000U)
#define CAN_CTRL1_PSEG1_MASK            (0x00380000U)
#define CAN_CTRL1_PSEG2_MASK            (0x00070000U)
#define CAN_CTRL1_BOFFMSK               (0x00008000U)
#define CAN_CTRL1_ERRMSK                (0x00004000U)
#define CAN_CTRL1_CLKSRC                (0x00002000U)
#define CAN_CTRL1_LPB                   (0x00001000U)
#define CAN_CTRL1_TWRNMSK               (0x00000800U)
#define CAN_CTRL1_RWRNMSK               (0x00000400U)
#define CAN_CTRL1_SMP                   (0x00000080U)
#define CAN_CTRL1_BOFFREC               (0x00000040U)
#define CAN_CTRL1_TSYN                  (0x00000020U)
#define CAN_CTRL1_LBUF                  (0x00000010U)
#define CAN_CTRL1_LOM                   (0x00000008U)
#define CAN_CTRL1_PROPSEG_MASK          (0x00000007U)

#define CAN_MB_CODE_MASK                (0x0F000000U)
#define CAN_MB_CODE_TX_INACTIVE         (0x08000000U)
#define CAN_MB_CODE_TX_ACTIVE           (0x00000000U)
#define CAN_MB_CODE_RX_EMPTY            (0x04000000U)
#define CAN_MB_CODE_RX_FULL             (0x02000000U)

#define CAN_ESR1_BOFFINT                (0x00000004U)
#define CAN_ESR1_ERRINT                 (0x00000002U)
#define CAN_ESR1_WAKINT                 (0x00000001U)

#define CAN_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Can_DriverInitialized = FALSE;
static Can_ControllerStateType Can_ControllerState[CAN_NUM_CONTROLLERS];
static const Can_ConfigType* Can_ConfigPtr = NULL_PTR;

#define CAN_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static uint32 Can_GetBaseAddr(uint8 controller)
{
    uint32 baseAddr;
    switch (controller) {
        case CAN_CONTROLLER_0: baseAddr = CAN_FLEXCAN1_BASE_ADDR; break;
        case CAN_CONTROLLER_1: baseAddr = CAN_FLEXCAN2_BASE_ADDR; break;
        default: baseAddr = 0U; break;
    }
    return baseAddr;
}

static void Can_EnableClock(uint8 controller)
{
    (void)controller;
}

static void Can_DisableClock(uint8 controller)
{
    (void)controller;
}

static Std_ReturnType Can_WaitForFreezeAck(uint32 baseAddr)
{
    uint32 timeout = 10000U;
    while ((REG_READ32(baseAddr + CAN_MCR) & CAN_MCR_FRZ) == 0U) {
        if (timeout == 0U) return E_NOT_OK;
        timeout--;
    }
    return E_OK;
}

static Std_ReturnType Can_WaitForNotReady(uint32 baseAddr)
{
    uint32 timeout = 10000U;
    while ((REG_READ32(baseAddr + CAN_MCR) & CAN_MCR_NOT_RDY) != 0U) {
        if (timeout == 0U) return E_NOT_OK;
        timeout--;
    }
    return E_OK;
}

#define CAN_START_SEC_CODE
#include "MemMap.h"

void Can_Init(const Can_ConfigType* Config)
{
    #if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (Config == NULL_PTR) {
        Det_ReportError(CAN_MODULE_ID, 0U, CAN_SID_INIT, CAN_E_PARAM_POINTER);
        return;
    }
    if (Can_DriverInitialized == TRUE) {
        Det_ReportError(CAN_MODULE_ID, 0U, CAN_SID_INIT, CAN_E_TRANSITION);
        return;
    }
    #endif

    Can_ConfigPtr = Config;

    for (uint8 i = 0U; i < CAN_NUM_CONTROLLERS; i++) {
        Can_ControllerState[i] = CAN_CS_UNINIT;

        uint32 baseAddr = Can_GetBaseAddr(i);
        if (baseAddr == 0U) continue;

        Can_EnableClock(i);

        /* Enable module */
        REG_WRITE32(baseAddr + CAN_MCR, 0U);

        /* Enter freeze mode */
        uint32 mcrValue = REG_READ32(baseAddr + CAN_MCR);
        mcrValue |= CAN_MCR_HALT | CAN_MCR_FRZ;
        REG_WRITE32(baseAddr + CAN_MCR, mcrValue);

        if (Can_WaitForFreezeAck(baseAddr) != E_OK) {
            continue;
        }

        /* Configure maximum message buffers */
        mcrValue = REG_READ32(baseAddr + CAN_MCR);
        mcrValue &= ~CAN_MCR_MAXMB_MASK;
        mcrValue |= (CAN_NUM_HOH - 1U) & CAN_MCR_MAXMB_MASK;
        REG_WRITE32(baseAddr + CAN_MCR, mcrValue);

        /* Configure bit timing */
        const Can_BaudrateConfigType* baudrate = &Config->Controllers[i].BaudrateConfigs[0];
        uint32 ctrl1Value = 0U;
        ctrl1Value |= ((baudrate->Prescaler - 1U) << 24) & CAN_CTRL1_PRESDIV_MASK;
        ctrl1Value |= ((baudrate->SyncJumpWidth - 1U) << 22) & CAN_CTRL1_RJW_MASK;
        ctrl1Value |= ((baudrate->PhaseSeg1 - 1U) << 19) & CAN_CTRL1_PSEG1_MASK;
        ctrl1Value |= ((baudrate->PhaseSeg2 - 1U) << 16) & CAN_CTRL1_PSEG2_MASK;
        ctrl1Value |= ((baudrate->PropSeg - 1U) << 0) & CAN_CTRL1_PROPSEG_MASK;
        REG_WRITE32(baseAddr + CAN_CTRL1, ctrl1Value);

        /* Configure message buffers */
        for (uint8 j = 0U; j < CAN_NUM_HOH; j++) {
            uint32 mbAddr = baseAddr + CAN_MB_BASE + (j * 16U);
            REG_WRITE32(mbAddr + 0, CAN_MB_CODE_TX_INACTIVE);
            REG_WRITE32(mbAddr + 4, 0U);
            REG_WRITE32(mbAddr + 8, 0U);
            REG_WRITE32(mbAddr + 12, 0U);
        }

        /* Enable interrupts if needed */
        if (Config->Controllers[i].BusOffProcessing ||
            Config->Controllers[i].WakeupProcessing) {
            uint32 imaskValue = 0U;
            if (Config->Controllers[i].BusOffProcessing) {
                imaskValue |= CAN_ESR1_BOFFINT;
            }
            REG_WRITE32(baseAddr + CAN_IMASK1, imaskValue);
        }

        Can_ControllerState[i] = CAN_CS_STOPPED;
    }

    Can_DriverInitialized = TRUE;
}

void Can_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(CAN_MODULE_ID, 0U, CAN_SID_GETVERSIONINFO, CAN_E_PARAM_POINTER);
        return;
    }
    #endif
    versioninfo->vendorID = CAN_VENDOR_ID;
    versioninfo->moduleID = CAN_MODULE_ID;
    versioninfo->sw_major_version = CAN_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CAN_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CAN_SW_PATCH_VERSION;
}

Can_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition)
{
    #if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (Can_DriverInitialized == FALSE) {
        Det_ReportError(CAN_MODULE_ID, Controller, CAN_SID_SETCONTROLLERMODE, CAN_E_UNINIT);
        return CAN_NOT_OK;
    }
    if (Controller >= CAN_NUM_CONTROLLERS) {
        Det_ReportError(CAN_MODULE_ID, Controller, CAN_SID_SETCONTROLLERMODE, CAN_E_PARAM_CONTROLLER);
        return CAN_NOT_OK;
    }
    #endif

    uint32 baseAddr = Can_GetBaseAddr(Controller);
    uint32 mcrValue;

    switch (Transition) {
        case CAN_CS_STARTED:
            if (Can_ControllerState[Controller] != CAN_CS_STOPPED) {
                return CAN_NOT_OK;
            }
            /* Exit freeze mode */
            mcrValue = REG_READ32(baseAddr + CAN_MCR);
            mcrValue &= ~CAN_MCR_HALT;
            REG_WRITE32(baseAddr + CAN_MCR, mcrValue);
            if (Can_WaitForNotReady(baseAddr) != E_OK) {
                return CAN_NOT_OK;
            }
            Can_ControllerState[Controller] = CAN_CS_STARTED;
            break;

        case CAN_CS_STOPPED:
            if (Can_ControllerState[Controller] != CAN_CS_STARTED) {
                return CAN_NOT_OK;
            }
            /* Enter freeze mode */
            mcrValue = REG_READ32(baseAddr + CAN_MCR);
            mcrValue |= CAN_MCR_HALT | CAN_MCR_FRZ;
            REG_WRITE32(baseAddr + CAN_MCR, mcrValue);
            if (Can_WaitForFreezeAck(baseAddr) != E_OK) {
                return CAN_NOT_OK;
            }
            Can_ControllerState[Controller] = CAN_CS_STOPPED;
            break;

        case CAN_CS_SLEEP:
            /* Not supported in this implementation */
            return CAN_NOT_OK;

        default:
            return CAN_NOT_OK;
    }

    return CAN_OK;
}

void Can_DisableControllerInterrupts(uint8 Controller)
{
    #if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (Can_DriverInitialized == FALSE) {
        Det_ReportError(CAN_MODULE_ID, Controller, CAN_SID_DISABLECONTROLLERINTERRUPTS, CAN_E_UNINIT);
        return;
    }
    if (Controller >= CAN_NUM_CONTROLLERS) {
        Det_ReportError(CAN_MODULE_ID, Controller, CAN_SID_DISABLECONTROLLERINTERRUPTS, CAN_E_PARAM_CONTROLLER);
        return;
    }
    #endif

    uint32 baseAddr = Can_GetBaseAddr(Controller);
    REG_WRITE32(baseAddr + CAN_IMASK1, 0U);
    REG_WRITE32(baseAddr + CAN_IMASK2, 0U);
}

void Can_EnableControllerInterrupts(uint8 Controller)
{
    #if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (Can_DriverInitialized == FALSE) {
        Det_ReportError(CAN_MODULE_ID, Controller, CAN_SID_ENABLECONTROLLERINTERRUPTS, CAN_E_UNINIT);
        return;
    }
    if (Controller >= CAN_NUM_CONTROLLERS) {
        Det_ReportError(CAN_MODULE_ID, Controller, CAN_SID_ENABLECONTROLLERINTERRUPTS, CAN_E_PARAM_CONTROLLER);
        return;
    }
    #endif

    uint32 baseAddr = Can_GetBaseAddr(Controller);
    uint32 imaskValue = 0U;

    if (Can_ConfigPtr->Controllers[Controller].BusOffProcessing) {
        imaskValue |= CAN_ESR1_BOFFINT | CAN_ESR1_ERRINT;
    }

    REG_WRITE32(baseAddr + CAN_IMASK1, imaskValue);
}

Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo)
{
    #if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (Can_DriverInitialized == FALSE) {
        Det_ReportError(CAN_MODULE_ID, 0U, CAN_SID_WRITECANID, CAN_E_UNINIT);
        return CAN_NOT_OK;
    }
    if (PduInfo == NULL_PTR) {
        Det_ReportError(CAN_MODULE_ID, 0U, CAN_SID_WRITECANID, CAN_E_PARAM_POINTER);
        return CAN_NOT_OK;
    }
    if (Hth >= CAN_NUM_HOH) {
        Det_ReportError(CAN_MODULE_ID, 0U, CAN_SID_WRITECANID, CAN_E_PARAM_HANDLE);
        return CAN_NOT_OK;
    }
    #endif

    uint8 controller = Hth / (CAN_NUM_HOH / CAN_NUM_CONTROLLERS);
    uint8 mbIndex = Hth % (CAN_NUM_HOH / CAN_NUM_CONTROLLERS);
    uint32 baseAddr = Can_GetBaseAddr(controller);
    uint32 mbAddr = baseAddr + CAN_MB_BASE + (mbIndex * 16U);

    /* Check if mailbox is available */
    uint32 csValue = REG_READ32(mbAddr + 0);
    if ((csValue & CAN_MB_CODE_MASK) != CAN_MB_CODE_TX_INACTIVE) {
        return CAN_BUSY;
    }

    /* Write ID */
    uint32 idValue;
    if (PduInfo->idType == CAN_ID_TYPE_EXTENDED) {
        idValue = (PduInfo->CanId & 0x1FFFFFFFU) | 0x80000000U;
    } else {
        idValue = (PduInfo->CanId & 0x7FFU) << 18;
    }
    REG_WRITE32(mbAddr + 4, idValue);

    /* Write data */
    uint32 dataWord0 = 0U;
    uint32 dataWord1 = 0U;
    for (uint8 i = 0U; i < PduInfo->CanDlc && i < 4U; i++) {
        dataWord0 |= (uint32)(PduInfo->SduPtr[i]) << (i * 8);
    }
    for (uint8 i = 4U; i < PduInfo->CanDlc && i < 8U; i++) {
        dataWord1 |= (uint32)(PduInfo->SduPtr[i]) << ((i - 4U) * 8);
    }
    REG_WRITE32(mbAddr + 8, dataWord0);
    REG_WRITE32(mbAddr + 12, dataWord1);

    /* Write CS (code and DLC) */
    csValue = CAN_MB_CODE_TX_ACTIVE | ((uint32)(PduInfo->CanDlc & 0x0FU) << 16);
    REG_WRITE32(mbAddr + 0, csValue);

    return CAN_OK;
}

void Can_MainFunction_Write(void)
{
    /* Polling mode implementation - check TX completion */
    for (uint8 controller = 0U; controller < CAN_NUM_CONTROLLERS; controller++) {
        if (Can_ControllerState[controller] != CAN_CS_STARTED) continue;

        uint32 baseAddr = Can_GetBaseAddr(controller);
        uint32 iflagValue = REG_READ32(baseAddr + CAN_IFLAG1);

        for (uint8 i = 0U; i < (CAN_NUM_HOH / CAN_NUM_CONTROLLERS); i++) {
            if ((iflagValue & (1U << i)) != 0U) {
                /* Clear flag */
                REG_WRITE32(baseAddr + CAN_IFLAG1, (1U << i));
                /* Notify upper layer */
                /* CanIf_TxConfirmation(...); */
            }
        }
    }
}

void Can_MainFunction_Read(void)
{
    /* Polling mode implementation - check RX reception */
    for (uint8 controller = 0U; controller < CAN_NUM_CONTROLLERS; controller++) {
        if (Can_ControllerState[controller] != CAN_CS_STARTED) continue;

        uint32 baseAddr = Can_GetBaseAddr(controller);
        uint32 iflagValue = REG_READ32(baseAddr + CAN_IFLAG1);

        for (uint8 i = (CAN_NUM_HOH / CAN_NUM_CONTROLLERS); i < CAN_NUM_HOH; i++) {
            if ((iflagValue & (1U << i)) != 0U) {
                uint32 mbAddr = baseAddr + CAN_MB_BASE + (i * 16U);
                /* Read message and notify upper layer */
                /* CanIf_RxIndication(...); */
                /* Clear flag */
                REG_WRITE32(baseAddr + CAN_IFLAG1, (1U << i));
            }
        }
    }
}

void Can_MainFunction_BusOff(void)
{
    for (uint8 controller = 0U; controller < CAN_NUM_CONTROLLERS; controller++) {
        if (Can_ControllerState[controller] != CAN_CS_STARTED) continue;

        uint32 baseAddr = Can_GetBaseAddr(controller);
        uint32 esrValue = REG_READ32(baseAddr + CAN_ESR1);

        if ((esrValue & CAN_ESR1_BOFFINT) != 0U) {
            /* Bus-off detected */
            /* CanIf_ControllerBusOff(controller); */
            /* Clear flag */
            REG_WRITE32(baseAddr + CAN_ESR1, CAN_ESR1_BOFFINT);
        }
    }
}

void Can_MainFunction_Wakeup(void)
{
    /* Wakeup handling */
}

void Can_MainFunction_Mode(void)
{
    /* Mode transition handling */
}

Std_ReturnType Can_CheckWakeup(uint8 Controller)
{
    #if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (Can_DriverInitialized == FALSE) {
        Det_ReportError(CAN_MODULE_ID, Controller, CAN_SID_CHECKWAKEUP, CAN_E_UNINIT);
        return E_NOT_OK;
    }
    if (Controller >= CAN_NUM_CONTROLLERS) {
        Det_ReportError(CAN_MODULE_ID, Controller, CAN_SID_CHECKWAKEUP, CAN_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    #endif

    uint32 baseAddr = Can_GetBaseAddr(Controller);
    uint32 esrValue = REG_READ32(baseAddr + CAN_ESR1);

    if ((esrValue & CAN_ESR1_WAKINT) != 0U) {
        return E_OK;
    }

    return E_NOT_OK;
}

#define CAN_STOP_SEC_CODE
#include "MemMap.h"
