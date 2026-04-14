/**
 * @file Mcu.c
 * @brief MCU Driver implementation for i.MX8M Mini
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: MCU Driver
 * Layer: MCAL (Microcontroller Driver Layer)
 * Platform: i.MX8M Mini (NXP)
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Mcu.h"
#include "Mcu_Cfg.h"
#include "Mcu_Reg.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define MCU_GPC_BASE_ADDR               (0x303A0000UL)
#define MCU_CCM_BASE_ADDR               (0x30380000UL)
#define MCU_SRC_BASE_ADDR               (0x30390000UL)

#define MCU_GPC_PGC_CPU_MAPPING         (MCU_GPC_BASE_ADDR + 0x0EC)
#define MCU_GPC_PU_PGC_SW_PUP_REQ       (MCU_GPC_BASE_ADDR + 0x0F8)
#define MCU_GPC_PU_PGC_SW_PDN_REQ       (MCU_GPC_BASE_ADDR + 0x104)

#define MCU_CCM_CCR                     (MCU_CCM_BASE_ADDR + 0x0000)
#define MCU_CCM_CSR                     (MCU_CCM_BASE_ADDR + 0x0008)
#define MCU_CCM_CCSR                    (MCU_CCM_BASE_ADDR + 0x000C)
#define MCU_CCM_CACRR                   (MCU_CCM_BASE_ADDR + 0x0010)
#define MCU_CCM_CBCDR                   (MCU_CCM_BASE_ADDR + 0x0014)
#define MCU_CCM_CBCMR                   (MCU_CCM_BASE_ADDR + 0x0018)

#define MCU_SRC_SCR                     (MCU_SRC_BASE_ADDR + 0x0000)
#define MCU_SRC_SRSR                    (MCU_SRC_BASE_ADDR + 0x0004)
#define MCU_SRC_SBMR1                   (MCU_SRC_BASE_ADDR + 0x0008)
#define MCU_SRC_SBMR2                   (MCU_SRC_BASE_ADDR + 0x001C)

#define MCU_PLL_LOCK_TIMEOUT            (10000U)
#define MCU_CLOCK_SWITCH_TIMEOUT        (10000U)

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
typedef struct {
    boolean initialized;
    Mcu_ClockType currentClock;
    Mcu_ModeType currentMode;
    Mcu_RamStateType ramState;
} Mcu_DriverStateType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define MCU_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static Mcu_DriverStateType Mcu_DriverState = {
    .initialized = FALSE,
    .currentClock = 0U,
    .currentMode = MCU_MODE_RUN,
    .ramState = MCU_RAMSTATE_INVALID
};

static const Mcu_ConfigType* Mcu_ConfigPtr = NULL_PTR;

#define MCU_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static Std_ReturnType Mcu_ConfigureClock(const Mcu_ClockConfigType* clockConfig);
static Std_ReturnType Mcu_ConfigurePLL(uint32 pllBaseAddr, const Mcu_PllConfigType* pllConfig);
static Std_ReturnType Mcu_WaitForPLLLock(uint32 pllBaseAddr);
static void Mcu_SetClock dividers(const Mcu_ClockConfigType* clockConfig);
static Mcu_ResetType Mcu_GetResetReasonFromRegister(void);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Configures the system clock
 */
static Std_ReturnType Mcu_ConfigureClock(const Mcu_ClockConfigType* clockConfig)
{
    Std_ReturnType status = E_OK;
    uint32 timeout;
    
    /* Configure ARM PLL if needed */
    if (clockConfig->PllConfigs != NULL_PTR) {
        for (uint8 i = 0U; i < clockConfig->NumPllConfigs; i++) {
            status = Mcu_ConfigurePLL(clockConfig->PllConfigs[i].PllBaseAddr,
                                       &clockConfig->PllConfigs[i]);
            if (status != E_OK) {
                break;
            }
        }
    }
    
    if (status == E_OK) {
        /* Set clock dividers */
        Mcu_SetClockDividers(clockConfig);
        
        /* Switch to target clock source */
        timeout = MCU_CLOCK_SWITCH_TIMEOUT;
        while ((REG_READ32(MCU_CCM_CCSR) & 0x01U) != clockConfig->ClockSource) {
            if (timeout == 0U) {
                status = E_NOT_OK;
                break;
            }
            timeout--;
        }
    }
    
    return status;
}

/**
 * @brief Configures a PLL
 */
static Std_ReturnType Mcu_ConfigurePLL(uint32 pllBaseAddr, const Mcu_PllConfigType* pllConfig)
{
    Std_ReturnType status = E_OK;
    uint32 regValue;
    
    /* Bypass PLL during configuration */
    regValue = REG_READ32(pllBaseAddr + 0x00);
    regValue |= 0x1000U; /* Set BYPASS bit */
    REG_WRITE32(pllBaseAddr + 0x00, regValue);
    
    /* Configure PLL parameters */
    regValue = ((pllConfig->Prediv - 1U) & 0x07U) << 12;
    regValue |= (pllConfig->Multiplier & 0x3FFU) << 0;
    REG_WRITE32(pllBaseAddr + 0x04, regValue);
    
    /* Configure post dividers */
    regValue = ((pllConfig->Postdiv1 - 1U) & 0x07U) << 4;
    regValue |= ((pllConfig->Postdiv2 - 1U) & 0x07U) << 0;
    REG_WRITE32(pllBaseAddr + 0x08, regValue);
    
    /* Enable PLL */
    regValue = REG_READ32(pllBaseAddr + 0x00);
    regValue |= 0x2000U; /* Set ENABLE bit */
    REG_WRITE32(pllBaseAddr + 0x00, regValue);
    
    /* Wait for PLL lock */
    status = Mcu_WaitForPLLLock(pllBaseAddr);
    
    if (status == E_OK) {
        /* Disable bypass */
        regValue = REG_READ32(pllBaseAddr + 0x00);
        regValue &= ~0x1000U; /* Clear BYPASS bit */
        REG_WRITE32(pllBaseAddr + 0x00, regValue);
    }
    
    return status;
}

/**
 * @brief Waits for PLL to lock
 */
static Std_ReturnType Mcu_WaitForPLLLock(uint32 pllBaseAddr)
{
    Std_ReturnType status = E_NOT_OK;
    uint32 timeout = MCU_PLL_LOCK_TIMEOUT;
    uint32 regValue;
    
    do {
        regValue = REG_READ32(pllBaseAddr + 0x00);
        if ((regValue & 0x80000000U) != 0U) { /* LOCK bit */
            status = E_OK;
            break;
        }
        timeout--;
    } while (timeout > 0U);
    
    return status;
}

/**
 * @brief Sets clock dividers
 */
static void Mcu_SetClockDividers(const Mcu_ClockConfigType* clockConfig)
{
    uint32 regValue;
    
    /* Configure ARM clock divider */
    regValue = REG_READ32(MCU_CCM_CACRR);
    regValue &= ~0x07U;
    regValue |= (clockConfig->ArmDiv - 1U) & 0x07U;
    REG_WRITE32(MCU_CCM_CACRR, regValue);
    
    /* Configure AXI clock divider */
    regValue = REG_READ32(MCU_CCM_CBCDR);
    regValue &= ~(0x07U << 16);
    regValue |= ((clockConfig->AxiDiv - 1U) & 0x07U) << 16;
    REG_WRITE32(MCU_CCM_CBCDR, regValue);
    
    /* Configure AHB clock divider */
    regValue = REG_READ32(MCU_CCM_CBCMR);
    regValue &= ~(0x07U << 18);
    regValue |= ((clockConfig->AhbDiv - 1U) & 0x07U) << 18;
    REG_WRITE32(MCU_CCM_CBCMR, regValue);
}

/**
 * @brief Gets reset reason from hardware register
 */
static Mcu_ResetType Mcu_GetResetReasonFromRegister(void)
{
    Mcu_ResetType resetReason;
    uint32 srsrValue;
    
    srsrValue = REG_READ32(MCU_SRC_SRSR);
    
    if ((srsrValue & 0x01U) != 0U) {
        resetReason = MCU_POWER_ON_RESET;
    }
    else if ((srsrValue & 0x02U) != 0U) {
        resetReason = MCU_WATCHDOG_RESET;
    }
    else if ((srsrValue & 0x04U) != 0U) {
        resetReason = MCU_SW_RESET;
    }
    else if ((srsrValue & 0x08U) != 0U) {
        resetReason = MCU_EXTERNAL_RESET;
    }
    else {
        resetReason = MCU_RESET_UNDEFINED;
    }
    
    return resetReason;
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
#define MCU_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the MCU driver
 */
void Mcu_Init(const Mcu_ConfigType* ConfigPtr)
{
    #if (MCU_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_INIT, MCU_E_PARAM_CONFIG);
        return;
    }
    
    if (Mcu_DriverState.initialized == TRUE) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_INIT, MCU_E_ALREADY_INITIALIZED);
        return;
    }
    #endif
    
    Mcu_ConfigPtr = ConfigPtr;
    Mcu_DriverState.initialized = TRUE;
    Mcu_DriverState.currentMode = MCU_MODE_RUN;
    
    /* Initialize RAM sections if configured */
    if (ConfigPtr->RamSections != NULL_PTR) {
        for (uint8 i = 0U; i < ConfigPtr->NumRamSections; i++) {
            /* Initialize RAM section */
            uint8* ramPtr = (uint8*)ConfigPtr->RamSections[i].RamBaseAddr;
            for (uint32 j = 0U; j < ConfigPtr->RamSections[i].RamSize; j++) {
                ramPtr[j] = ConfigPtr->RamSections[i].RamDefaultValue;
            }
        }
    }
}

/**
 * @brief Initializes the MCU clock
 */
Std_ReturnType Mcu_InitClock(Mcu_ClockType ClockSetting)
{
    Std_ReturnType status = E_OK;
    
    #if (MCU_DEV_ERROR_DETECT == STD_ON)
    if (Mcu_DriverState.initialized == FALSE) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_INIT_CLOCK, MCU_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (ClockSetting >= Mcu_ConfigPtr->NumClockConfigs) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_INIT_CLOCK, MCU_E_PARAM_CLOCK);
        return E_NOT_OK;
    }
    #endif
    
    #if (MCU_NO_PLL == STD_OFF)
    status = Mcu_ConfigureClock(&Mcu_ConfigPtr->ClockConfigs[ClockSetting]);
    #endif
    
    if (status == E_OK) {
        Mcu_DriverState.currentClock = ClockSetting;
    }
    
    return status;
}

/**
 * @brief Distributes the PLL clock
 */
void Mcu_DistributePllClock(void)
{
    #if (MCU_DEV_ERROR_DETECT == STD_ON)
    if (Mcu_DriverState.initialized == FALSE) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_DISTRIBUTE_PLL_CLOCK, MCU_E_UNINIT);
        return;
    }
    
    if (Mcu_DriverState.currentClock == 0U) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_DISTRIBUTE_PLL_CLOCK, MCU_E_PLL_NOT_LOCKED);
        return;
    }
    #endif
    
    /* Enable clock distribution */
    uint32 regValue = REG_READ32(MCU_CCM_CCR);
    regValue |= 0x01U; /* Set CLK_ENABLE bit */
    REG_WRITE32(MCU_CCM_CCR, regValue);
}

/**
 * @brief Gets PLL lock status
 */
Mcu_PllStatusType Mcu_GetPllStatus(void)
{
    Mcu_PllStatusType pllStatus;
    
    #if (MCU_DEV_ERROR_DETECT == STD_ON)
    if (Mcu_DriverState.initialized == FALSE) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_GET_PLL_STATUS, MCU_E_UNINIT);
        return MCU_PLL_STATUS_UNDEFINED;
    }
    #endif
    
    #if (MCU_NO_PLL == STD_ON)
    pllStatus = MCU_PLL_STATUS_LOCKED;
    #else
    uint32 regValue = REG_READ32(MCU_CCM_CSR);
    if ((regValue & 0x01U) != 0U) {
        pllStatus = MCU_PLL_STATUS_LOCKED;
    }
    else {
        pllStatus = MCU_PLL_STATUS_UNLOCKED;
    }
    #endif
    
    return pllStatus;
}

/**
 * @brief Sets MCU mode
 */
void Mcu_SetMode(Mcu_ModeType McuMode)
{
    #if (MCU_DEV_ERROR_DETECT == STD_ON)
    if (Mcu_DriverState.initialized == FALSE) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_SET_MODE, MCU_E_UNINIT);
        return;
    }
    
    if (McuMode >= Mcu_ConfigPtr->NumModes) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_SET_MODE, MCU_E_PARAM_MODE);
        return;
    }
    #endif
    
    /* Configure mode based on target */
    switch (Mcu_ConfigPtr->ModeConfigs[McuMode].Mode) {
        case MCU_MODE_RUN:
            /* Set run mode */
            REG_WRITE32(MCU_GPC_PGC_CPU_MAPPING, 0x01U);
            break;
            
        case MCU_MODE_SLEEP:
            /* Set sleep mode */
            REG_WRITE32(MCU_GPC_PGC_CPU_MAPPING, 0x02U);
            REG_WRITE32(MCU_GPC_PU_PGC_SW_PDN_REQ, 0x01U);
            break;
            
        case MCU_MODE_DEEP_SLEEP:
            /* Set deep sleep mode */
            REG_WRITE32(MCU_GPC_PGC_CPU_MAPPING, 0x04U);
            REG_WRITE32(MCU_GPC_PU_PGC_SW_PDN_REQ, 0x01U);
            break;
            
        default:
            /* Do nothing */
            break;
    }
    
    Mcu_DriverState.currentMode = McuMode;
}

/**
 * @brief Gets reset reason
 */
Mcu_ResetType Mcu_GetResetReason(void)
{
    Mcu_ResetType resetReason;
    
    #if (MCU_DEV_ERROR_DETECT == STD_ON)
    if (Mcu_DriverState.initialized == FALSE) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_GET_RESET_REASON, MCU_E_UNINIT);
        return MCU_RESET_UNDEFINED;
    }
    #endif
    
    resetReason = Mcu_GetResetReasonFromRegister();
    
    return resetReason;
}

/**
 * @brief Gets reset raw value
 */
Mcu_RawResetType Mcu_GetResetRawValue(void)
{
    Mcu_RawResetType rawValue;
    
    #if (MCU_DEV_ERROR_DETECT == STD_ON)
    if (Mcu_DriverState.initialized == FALSE) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_GET_RESET_RAW_VALUE, MCU_E_UNINIT);
        return 0U;
    }
    #endif
    
    rawValue = REG_READ32(MCU_SRC_SRSR);
    
    return rawValue;
}

/**
 * @brief Performs MCU reset
 */
void Mcu_PerformReset(void)
{
    #if (MCU_DEV_ERROR_DETECT == STD_ON)
    if (Mcu_DriverState.initialized == FALSE) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_PERFORM_RESET, MCU_E_UNINIT);
        return;
    }
    #endif
    
    #if (MCU_PERFORM_RESET_API == STD_ON)
    /* Trigger software reset */
    REG_WRITE32(MCU_SRC_SCR, 0x01U);
    
    /* Wait for reset */
    while (1) {
        /* Infinite loop until reset occurs */
    }
    #endif
}

/**
 * @brief Gets version information
 */
void Mcu_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (MCU_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_GET_VERSION_INFO, MCU_E_PARAM_POINTER);
        return;
    }
    #endif
    
    #if (MCU_VERSION_INFO_API == STD_ON)
    versioninfo->vendorID = MCU_VENDOR_ID;
    versioninfo->moduleID = MCU_MODULE_ID;
    versioninfo->sw_major_version = MCU_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = MCU_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = MCU_SW_PATCH_VERSION;
    #endif
}

/**
 * @brief Initializes RAM section
 */
Std_ReturnType Mcu_InitRamSection(Mcu_RamSectionType RamSection)
{
    Std_ReturnType status = E_NOT_OK;
    
    #if (MCU_DEV_ERROR_DETECT == STD_ON)
    if (Mcu_DriverState.initialized == FALSE) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_INIT_RAM_SECTION, MCU_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (RamSection >= Mcu_ConfigPtr->NumRamSections) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_INIT_RAM_SECTION, MCU_E_PARAM_RAMSECTION);
        return E_NOT_OK;
    }
    #endif
    
    #if (MCU_GET_RAM_STATE_API == STD_ON)
    /* Initialize RAM section */
    uint8* ramPtr = (uint8*)Mcu_ConfigPtr->RamSections[RamSection].RamBaseAddr;
    for (uint32 i = 0U; i < Mcu_ConfigPtr->RamSections[RamSection].RamSize; i++) {
        ramPtr[i] = Mcu_ConfigPtr->RamSections[RamSection].RamDefaultValue;
    }
    status = E_OK;
    #endif
    
    return status;
}

/**
 * @brief Gets RAM state
 */
Mcu_RamStateType Mcu_GetRamState(void)
{
    Mcu_RamStateType ramState;
    
    #if (MCU_DEV_ERROR_DETECT == STD_ON)
    if (Mcu_DriverState.initialized == FALSE) {
        Det_ReportError(MCU_MODULE_ID, 0U, MCU_SID_GET_RAM_STATE, MCU_E_UNINIT);
        return MCU_RAMSTATE_INVALID;
    }
    #endif
    
    #if (MCU_GET_RAM_STATE_API == STD_ON)
    /* Check RAM integrity */
    ramState = MCU_RAMSTATE_VALID;
    #else
    ramState = MCU_RAMSTATE_INVALID;
    #endif
    
    return ramState;
}

#define MCU_STOP_SEC_CODE
#include "MemMap.h"
