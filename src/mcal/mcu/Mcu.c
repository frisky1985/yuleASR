/**
 * @file Mcu.c
 * @brief Mcu (Microcontroller Driver) Implementation
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Mcu Module - Microcontroller Driver
 * Compliant with AUTOSAR R22-11 MCAL Specification
 * Module ID: 0x12
 * MISRA C:2012 compliant
 */

#include "mcal/mcu/Mcu.h"
#include "mcal/mcu/Mcu_Cfg.h"
#include <string.h>

/*============================================================================*
 * Static Variables
 *============================================================================*/
static Mcu_ModuleStateType gMcu_ModuleState;
static const Mcu_HwInterfaceType* gMcu_HwInterface = NULL;

/*============================================================================*
 * Default Configuration
 *============================================================================*/
static const Mcu_GeneralConfigType gMcu_DefaultGeneral = {
    .hwType = MCU_CFG_HW_TYPE,
    .devErrorDetect = (MCU_CFG_DEV_ERROR_DETECT == STD_ON),
    .versionInfoApi = (MCU_CFG_VERSION_INFO_API == STD_ON),
    .ramInitEnabled = (MCU_CFG_RAM_INIT_ENABLED == STD_ON),
    .clockInitEnabled = (MCU_CFG_CLOCK_INIT_ENABLED == STD_ON),
    .noInitOnReset = (MCU_CFG_NO_INIT_ON_RESET == STD_ON),
    .performReset = (MCU_CFG_PERFORM_RESET_API == STD_ON),
    .resetReasonEnabled = (MCU_CFG_RESET_REASON_API == STD_ON)
};

/* Clock settings */
static const Mcu_ClockSettingConfigType gMcu_DefaultClockSettings[MCU_CFG_CLOCK_SETTING_COUNT] = {
    /* Clock Setting 0: 100MHz */
    {
        .settingId = MCU_CFG_CLK0_ID,
        .oscFreqHz = MCU_CFG_CLK0_OSC_FREQ_HZ,
        .oscEnabled = (MCU_CFG_CLK0_OSC_ENABLED == STD_ON),
        .oscBypass = (MCU_CFG_CLK0_OSC_BYPASS == STD_ON),
        .pll = {
            .inputFreqHz = MCU_CFG_CLK0_PLL_INPUT_FREQ_HZ,
            .outputFreqHz = MCU_CFG_CLK0_PLL_OUTPUT_FREQ_HZ,
            .multiplier = MCU_CFG_CLK0_PLL_MULTIPLIER,
            .predivider = MCU_CFG_CLK0_PLL_PREDIVIDER,
            .postdivider1 = MCU_CFG_CLK0_PLL_POSTDIVIDER1,
            .postdivider2 = MCU_CFG_CLK0_PLL_POSTDIVIDER2,
            .enabled = (MCU_CFG_CLK0_PLL_ENABLED == STD_ON),
            .lockTimeout = MCU_CFG_CLK0_PLL_LOCK_TIMEOUT
        },
        .sysClockFreqHz = MCU_CFG_CLK0_SYS_FREQ_HZ,
        .cpuClockFreqHz = MCU_CFG_CLK0_CPU_FREQ_HZ,
        .busClockFreqHz = MCU_CFG_CLK0_BUS_FREQ_HZ,
        .flashClockFreqHz = MCU_CFG_CLK0_FLASH_FREQ_HZ,
        .flashWaitStates = MCU_CFG_CLK0_FLASH_WAIT_STATES,
        .wdgTimeoutMs = MCU_CFG_CLK0_WDG_TIMEOUT_MS,
        .wdgEnabled = (MCU_CFG_CLK0_WDG_ENABLED == STD_ON),
        .periphClockCount = 0U
    },
    /* Clock Setting 1: 200MHz */
    {
        .settingId = MCU_CFG_CLK1_ID,
        .oscFreqHz = MCU_CFG_CLK1_OSC_FREQ_HZ,
        .oscEnabled = (MCU_CFG_CLK1_OSC_ENABLED == STD_ON),
        .oscBypass = (MCU_CFG_CLK1_OSC_BYPASS == STD_ON),
        .pll = {
            .inputFreqHz = MCU_CFG_CLK1_PLL_INPUT_FREQ_HZ,
            .outputFreqHz = MCU_CFG_CLK1_PLL_OUTPUT_FREQ_HZ,
            .multiplier = MCU_CFG_CLK1_PLL_MULTIPLIER,
            .predivider = MCU_CFG_CLK1_PLL_PREDIVIDER,
            .postdivider1 = MCU_CFG_CLK1_PLL_POSTDIVIDER1,
            .postdivider2 = MCU_CFG_CLK1_PLL_POSTDIVIDER2,
            .enabled = (MCU_CFG_CLK1_PLL_ENABLED == STD_ON),
            .lockTimeout = MCU_CFG_CLK1_PLL_LOCK_TIMEOUT
        },
        .sysClockFreqHz = MCU_CFG_CLK1_SYS_FREQ_HZ,
        .cpuClockFreqHz = MCU_CFG_CLK1_CPU_FREQ_HZ,
        .busClockFreqHz = MCU_CFG_CLK1_BUS_FREQ_HZ,
        .flashClockFreqHz = MCU_CFG_CLK1_FLASH_FREQ_HZ,
        .flashWaitStates = MCU_CFG_CLK1_FLASH_WAIT_STATES,
        .wdgTimeoutMs = MCU_CFG_CLK1_WDG_TIMEOUT_MS,
        .wdgEnabled = (MCU_CFG_CLK1_WDG_ENABLED == STD_ON),
        .periphClockCount = 0U
    },
    /* Clock Setting 2: 50MHz (Low Power) */
    {
        .settingId = MCU_CFG_CLK2_ID,
        .oscFreqHz = MCU_CFG_CLK2_OSC_FREQ_HZ,
        .oscEnabled = (MCU_CFG_CLK2_OSC_ENABLED == STD_ON),
        .oscBypass = (MCU_CFG_CLK2_OSC_BYPASS == STD_ON),
        .pll = {
            .inputFreqHz = MCU_CFG_CLK2_PLL_INPUT_FREQ_HZ,
            .outputFreqHz = MCU_CFG_CLK2_PLL_OUTPUT_FREQ_HZ,
            .multiplier = MCU_CFG_CLK2_PLL_MULTIPLIER,
            .predivider = MCU_CFG_CLK2_PLL_PREDIVIDER,
            .postdivider1 = MCU_CFG_CLK2_PLL_POSTDIVIDER1,
            .postdivider2 = MCU_CFG_CLK2_PLL_POSTDIVIDER2,
            .enabled = (MCU_CFG_CLK2_PLL_ENABLED == STD_ON),
            .lockTimeout = MCU_CFG_CLK2_PLL_LOCK_TIMEOUT
        },
        .sysClockFreqHz = MCU_CFG_CLK2_SYS_FREQ_HZ,
        .cpuClockFreqHz = MCU_CFG_CLK2_CPU_FREQ_HZ,
        .busClockFreqHz = MCU_CFG_CLK2_BUS_FREQ_HZ,
        .flashClockFreqHz = MCU_CFG_CLK2_FLASH_FREQ_HZ,
        .flashWaitStates = MCU_CFG_CLK2_FLASH_WAIT_STATES,
        .wdgTimeoutMs = MCU_CFG_CLK2_WDG_TIMEOUT_MS,
        .wdgEnabled = (MCU_CFG_CLK2_WDG_ENABLED == STD_ON),
        .periphClockCount = 0U
    }
};

/* RAM sections */
static const Mcu_RamSectionConfigType gMcu_DefaultRamSections[MCU_CFG_RAM_SECTION_COUNT] = {
    {
        .sectionId = MCU_CFG_RAM0_ID,
        .sectionType = MCU_CFG_RAM0_TYPE,
        .startAddress = MCU_CFG_RAM0_START_ADDR,
        .size = MCU_CFG_RAM0_SIZE,
        .initValue = MCU_CFG_RAM0_INIT_VALUE,
        .initEnabled = (MCU_CFG_RAM0_INIT_ENABLED == STD_ON),
        .eccEnabled = (MCU_CFG_RAM0_ECC_ENABLED == STD_ON),
        .retention = (MCU_CFG_RAM0_RETENTION == STD_ON)
    },
    {
        .sectionId = MCU_CFG_RAM1_ID,
        .sectionType = MCU_CFG_RAM1_TYPE,
        .startAddress = MCU_CFG_RAM1_START_ADDR,
        .size = MCU_CFG_RAM1_SIZE,
        .initValue = MCU_CFG_RAM1_INIT_VALUE,
        .initEnabled = (MCU_CFG_RAM1_INIT_ENABLED == STD_ON),
        .eccEnabled = (MCU_CFG_RAM1_ECC_ENABLED == STD_ON),
        .retention = (MCU_CFG_RAM1_RETENTION == STD_ON)
    },
    {
        .sectionId = MCU_CFG_RAM2_ID,
        .sectionType = MCU_CFG_RAM2_TYPE,
        .startAddress = MCU_CFG_RAM2_START_ADDR,
        .size = MCU_CFG_RAM2_SIZE,
        .initValue = MCU_CFG_RAM2_INIT_VALUE,
        .initEnabled = (MCU_CFG_RAM2_INIT_ENABLED == STD_ON),
        .eccEnabled = (MCU_CFG_RAM2_ECC_ENABLED == STD_ON),
        .retention = (MCU_CFG_RAM2_RETENTION == STD_ON)
    },
    {
        .sectionId = MCU_CFG_RAM3_ID,
        .sectionType = MCU_CFG_RAM3_TYPE,
        .startAddress = MCU_CFG_RAM3_START_ADDR,
        .size = MCU_CFG_RAM3_SIZE,
        .initValue = MCU_CFG_RAM3_INIT_VALUE,
        .initEnabled = (MCU_CFG_RAM3_INIT_ENABLED == STD_ON),
        .eccEnabled = (MCU_CFG_RAM3_ECC_ENABLED == STD_ON),
        .retention = (MCU_CFG_RAM3_RETENTION == STD_ON)
    }
};

/* Mode configurations */
static const Mcu_ModeConfigType gMcu_DefaultModeConfigs[MCU_CFG_MODE_CONFIG_COUNT] = {
    {
        .mode = MCU_CFG_MODE0_TYPE,
        .wakeupSources = MCU_CFG_MODE0_WAKEUP_SOURCES,
        .wakeupTimeout = MCU_CFG_MODE0_WAKEUP_TIMEOUT,
        .ramRetention = (MCU_CFG_MODE0_RAM_RETENTION == STD_ON),
        .clockRetention = (MCU_CFG_MODE0_CLK_RETENTION == STD_ON),
        .voltageRegulatorMode = MCU_CFG_MODE0_VREG_MODE
    },
    {
        .mode = MCU_CFG_MODE1_TYPE,
        .wakeupSources = MCU_CFG_MODE1_WAKEUP_SOURCES,
        .wakeupTimeout = MCU_CFG_MODE1_WAKEUP_TIMEOUT,
        .ramRetention = (MCU_CFG_MODE1_RAM_RETENTION == STD_ON),
        .clockRetention = (MCU_CFG_MODE1_CLK_RETENTION == STD_ON),
        .voltageRegulatorMode = MCU_CFG_MODE1_VREG_MODE
    },
    {
        .mode = MCU_CFG_MODE2_TYPE,
        .wakeupSources = MCU_CFG_MODE2_WAKEUP_SOURCES,
        .wakeupTimeout = MCU_CFG_MODE2_WAKEUP_TIMEOUT,
        .ramRetention = (MCU_CFG_MODE2_RAM_RETENTION == STD_ON),
        .clockRetention = (MCU_CFG_MODE2_CLK_RETENTION == STD_ON),
        .voltageRegulatorMode = MCU_CFG_MODE2_VREG_MODE
    },
    {
        .mode = MCU_CFG_MODE3_TYPE,
        .wakeupSources = MCU_CFG_MODE3_WAKEUP_SOURCES,
        .wakeupTimeout = MCU_CFG_MODE3_WAKEUP_TIMEOUT,
        .ramRetention = (MCU_CFG_MODE3_RAM_RETENTION == STD_ON),
        .clockRetention = (MCU_CFG_MODE3_CLK_RETENTION == STD_ON),
        .voltageRegulatorMode = MCU_CFG_MODE3_VREG_MODE
    }
};

/* Reset configuration */
static const Mcu_ResetConfigType gMcu_DefaultResetConfig = {
    .swResetEnabled = (MCU_CFG_SW_RESET_ENABLED == STD_ON),
    .wdgResetEnabled = (MCU_CFG_WDG_RESET_ENABLED == STD_ON),
    .extResetEnabled = (MCU_CFG_EXT_RESET_ENABLED == STD_ON),
    .jtagResetEnabled = (MCU_CFG_JTAG_RESET_ENABLED == STD_ON),
    .resetDelay = MCU_CFG_RESET_DELAY_US
};

/* Complete configuration */
const Mcu_ConfigType Mcu_Config = {
    .general = &gMcu_DefaultGeneral,
    .clockSettings = gMcu_DefaultClockSettings,
    .ramSections = gMcu_DefaultRamSections,
    .modeConfigs = gMcu_DefaultModeConfigs,
    .resetConfig = &gMcu_DefaultResetConfig,
    .clockSettingCount = MCU_CFG_CLOCK_SETTING_COUNT,
    .ramSectionCount = MCU_CFG_RAM_SECTION_COUNT,
    .modeConfigCount = MCU_CFG_MODE_CONFIG_COUNT,
    .defaultClockSetting = MCU_CFG_DEFAULT_CLOCK_SETTING
};

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Validate clock setting ID
 */
static bool Mcu_ValidateClockSettingId(uint8_t id)
{
    return (id < MCU_CFG_CLOCK_SETTING_COUNT);
}

/**
 * @brief Validate RAM section ID
 */
static bool Mcu_ValidateRamSectionId(uint8_t id)
{
    return (id < MCU_CFG_RAM_SECTION_COUNT);
}

/**
 * @brief Get clock setting by ID
 */
static const Mcu_ClockSettingConfigType* Mcu_GetClockSettingById(uint8_t id)
{
    if (!Mcu_ValidateClockSettingId(id)) {
        return NULL;
    }
    return &gMcu_DefaultClockSettings[id];
}

/**
 * @brief Get RAM section by ID
 */
static const Mcu_RamSectionConfigType* Mcu_GetRamSectionById(uint8_t id)
{
    if (!Mcu_ValidateRamSectionId(id)) {
        return NULL;
    }
    return &gMcu_DefaultRamSections[id];
}

/*============================================================================*
 * Initialization API
 *============================================================================*/

Mcu_ErrorCode_t Mcu_Init(const Mcu_ConfigType* config)
{
    Mcu_ErrorCode_t result = MCU_OK;

    if (gMcu_ModuleState.initialized) {
        return MCU_E_ALREADY_INITIALIZED;
    }

    /* Initialize module state */
    (void)memset(&gMcu_ModuleState, 0, sizeof(Mcu_ModuleStateType));

    if (config == NULL) {
        gMcu_ModuleState.config = &Mcu_Config;
    } else {
        gMcu_ModuleState.config = config;
    }

    /* Get reset reason */
    gMcu_ModuleState.lastReset = Mcu_GetResetReason();

    /* Initialize hardware */
    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->Init != NULL)) {
        result = gMcu_HwInterface->Init(gMcu_ModuleState.config);
        if (result != MCU_OK) {
            return result;
        }
    }

    /* Initialize RAM sections if enabled */
    if (gMcu_ModuleState.config->general->ramInitEnabled) {
        result = Mcu_InitRam();
        if (result != MCU_OK) {
            return result;
        }
    }

    /* Initialize clock if enabled */
    if (gMcu_ModuleState.config->general->clockInitEnabled) {
        result = Mcu_InitClockById(gMcu_ModuleState.config->defaultClockSetting);
        if (result != MCU_OK) {
            return result;
        }
    }

    gMcu_ModuleState.initialized = true;
    gMcu_ModuleState.currentMode = MCU_MODE_NORMAL;

    return MCU_OK;
}

Mcu_ErrorCode_t Mcu_Deinit(void)
{
    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    /* Deinitialize hardware */
    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->Deinit != NULL)) {
        (void)gMcu_HwInterface->Deinit();
    }

    gMcu_ModuleState.initialized = false;
    gMcu_ModuleState.currentMode = MCU_MODE_NORMAL;

    return MCU_OK;
}

bool Mcu_IsInitialized(void)
{
    return gMcu_ModuleState.initialized;
}

/*============================================================================*
 * Clock Initialization API
 *============================================================================*/

Mcu_ErrorCode_t Mcu_InitClock(const Mcu_ClockSettingConfigType* clockSetting)
{
    Mcu_ErrorCode_t result;

    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    if (clockSetting == NULL) {
        return MCU_E_POINTER;
    }

    /* Initialize through hardware interface */
    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->InitClock != NULL)) {
        result = gMcu_HwInterface->InitClock(clockSetting);
        if (result != MCU_OK) {
            return result;
        }
    }

    /* Update state */
    gMcu_ModuleState.currentClockSetting = clockSetting->settingId;
    gMcu_ModuleState.sysClockFreqHz = clockSetting->sysClockFreqHz;
    gMcu_ModuleState.cpuClockFreqHz = clockSetting->cpuClockFreqHz;
    gMcu_ModuleState.busClockFreqHz = clockSetting->busClockFreqHz;

    /* If PLL is enabled and not bypassed, wait for lock and distribute */
    if (clockSetting->pll.enabled) {
        result = Mcu_WaitPllLock(clockSetting->pll.lockTimeout);
        if (result != MCU_OK) {
            return result;
        }

        result = Mcu_DistributePllClock();
        if (result != MCU_OK) {
            return result;
        }
    }

    return MCU_OK;
}

Mcu_ErrorCode_t Mcu_InitClockById(uint8_t clockSettingId)
{
    const Mcu_ClockSettingConfigType* clockSetting;

    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    clockSetting = Mcu_GetClockSettingById(clockSettingId);
    if (clockSetting == NULL) {
        return MCU_E_PARAM_CLOCK;
    }

    return Mcu_InitClock(clockSetting);
}

Mcu_ErrorCode_t Mcu_DistributePllClock(void)
{
    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->DistributePllClock != NULL)) {
        return gMcu_HwInterface->DistributePllClock();
    }

    return MCU_OK;
}

bool Mcu_GetPllStatus(void)
{
    if (!gMcu_ModuleState.initialized) {
        return false;
    }

    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->GetPllStatus != NULL)) {
        return gMcu_HwInterface->GetPllStatus();
    }

    /* Assume locked if no hardware interface */
    return true;
}

Mcu_ErrorCode_t Mcu_WaitPllLock(uint32_t timeout)
{
    uint32_t elapsed = 0U;

    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    while (!Mcu_GetPllStatus()) {
        if ((timeout > 0U) && (elapsed >= timeout)) {
            return MCU_E_TIMEOUT;
        }
        /* Small delay - in real implementation, use precise timing */
        Mcu_DelayUs(1U);
        elapsed++;
    }

    return MCU_OK;
}

uint8_t Mcu_GetClockSetting(void)
{
    return gMcu_ModuleState.currentClockSetting;
}

/*============================================================================*
 * Clock Frequency API
 *============================================================================*/

uint32_t Mcu_GetSysClockFreq(void)
{
    return gMcu_ModuleState.sysClockFreqHz;
}

uint32_t Mcu_GetCpuClockFreq(void)
{
    return gMcu_ModuleState.cpuClockFreqHz;
}

uint32_t Mcu_GetBusClockFreq(void)
{
    return gMcu_ModuleState.busClockFreqHz;
}

uint32_t Mcu_GetPeriphClockFreq(uint8_t periphId)
{
    (void)periphId;
    /* In a real implementation, would return periph-specific clock */
    return gMcu_ModuleState.busClockFreqHz;
}

uint32_t Mcu_GetClockFrequency(Mcu_ClockType clockType)
{
    uint32_t freq = 0U;

    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->GetClockFrequency != NULL)) {
        freq = gMcu_HwInterface->GetClockFrequency(clockType);
    } else {
        /* Default implementation based on clock type */
        switch (clockType) {
            case MCU_CLOCK_SYS:
                freq = gMcu_ModuleState.sysClockFreqHz;
                break;
            case MCU_CLOCK_CPU:
                freq = gMcu_ModuleState.cpuClockFreqHz;
                break;
            case MCU_CLOCK_BUS:
                freq = gMcu_ModuleState.busClockFreqHz;
                break;
            default:
                freq = 0U;
                break;
        }
    }

    return freq;
}

/*============================================================================*
 * Peripheral Clock API
 *============================================================================*/

Mcu_ErrorCode_t Mcu_EnablePeriphClock(uint8_t periphId)
{
    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->EnablePeriphClock != NULL)) {
        return gMcu_HwInterface->EnablePeriphClock(periphId);
    }

    return MCU_OK;
}

Mcu_ErrorCode_t Mcu_DisablePeriphClock(uint8_t periphId)
{
    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->DisablePeriphClock != NULL)) {
        return gMcu_HwInterface->DisablePeriphClock(periphId);
    }

    return MCU_OK;
}

bool Mcu_IsPeriphClockEnabled(uint8_t periphId)
{
    if (!gMcu_ModuleState.initialized) {
        return false;
    }

    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->IsPeriphClockEnabled != NULL)) {
        return gMcu_HwInterface->IsPeriphClockEnabled(periphId);
    }

    return true;
}

void Mcu_EnablePeriphClocks(uint64_t periphMask)
{
    uint8_t i;

    for (i = 0U; i < 64U; i++) {
        if ((periphMask & ((uint64_t)1U << i)) != 0U) {
            (void)Mcu_EnablePeriphClock(i);
        }
    }
}

void Mcu_DisablePeriphClocks(uint64_t periphMask)
{
    uint8_t i;

    for (i = 0U; i < 64U; i++) {
        if ((periphMask & ((uint64_t)1U << i)) != 0U) {
            (void)Mcu_DisablePeriphClock(i);
        }
    }
}

/*============================================================================*
 * RAM Initialization API
 *============================================================================*/

Mcu_ErrorCode_t Mcu_InitRam(void)
{
    uint8_t i;
    Mcu_ErrorCode_t result;

    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    for (i = 0U; i < MCU_CFG_RAM_SECTION_COUNT; i++) {
        result = Mcu_InitRamSection(i);
        if (result != MCU_OK) {
            return result;
        }
    }

    return MCU_OK;
}

Mcu_ErrorCode_t Mcu_InitRamSection(uint8_t sectionId)
{
    const Mcu_RamSectionConfigType* ramSection;
    Mcu_ErrorCode_t result;

    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    ramSection = Mcu_GetRamSectionById(sectionId);
    if (ramSection == NULL) {
        return MCU_E_PARAM_RAM;
    }

    if (!ramSection->initEnabled) {
        return MCU_OK;
    }

    /* Use hardware interface if available */
    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->InitRamSection != NULL)) {
        result = gMcu_HwInterface->InitRamSection(ramSection);
        if (result != MCU_OK) {
            return result;
        }
    } else {
        /* Software initialization */
        result = Mcu_InitRamRange(
            ramSection->startAddress,
            ramSection->size,
            ramSection->initValue
        );
        if (result != MCU_OK) {
            return result;
        }
    }

    return MCU_OK;
}

Mcu_ErrorCode_t Mcu_InitRamRange(uint32_t startAddr, uint32_t size, uint8_t initValue)
{
    volatile uint8_t* ptr;
    uint32_t i;

    if (size == 0U) {
        return MCU_OK;
    }

    /* Validate address range */
    if (startAddr == 0U) {
        return MCU_E_PARAM_RAM;
    }

    ptr = (volatile uint8_t*)(uintptr_t)startAddr;

    /* Initialize memory */
    for (i = 0U; i < size; i++) {
        ptr[i] = initValue;
    }

    return MCU_OK;
}

Mcu_ErrorCode_t Mcu_ClearRamSection(uint8_t sectionId)
{
    const Mcu_RamSectionConfigType* ramSection;

    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    ramSection = Mcu_GetRamSectionById(sectionId);
    if (ramSection == NULL) {
        return MCU_E_PARAM_RAM;
    }

    return Mcu_InitRamRange(ramSection->startAddress, ramSection->size, 0U);
}

Mcu_ErrorCode_t Mcu_GetRamSectionInfo(uint8_t sectionId, uint32_t* startAddr, uint32_t* size)
{
    const Mcu_RamSectionConfigType* ramSection;

    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    if ((startAddr == NULL) || (size == NULL)) {
        return MCU_E_POINTER;
    }

    ramSection = Mcu_GetRamSectionById(sectionId);
    if (ramSection == NULL) {
        return MCU_E_PARAM_RAM;
    }

    *startAddr = ramSection->startAddress;
    *size = ramSection->size;

    return MCU_OK;
}

/*============================================================================*
 * Reset API
 *============================================================================*/

Mcu_ErrorCode_t Mcu_PerformReset(void)
{
    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->PerformReset != NULL)) {
        return gMcu_HwInterface->PerformReset();
    }

    /* Software reset via NVIC (ARM) or equivalent */
    /* This is platform-specific */
    return MCU_E_NOT_OK;
}

Mcu_ResetType Mcu_GetResetReason(void)
{
    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->GetResetReason != NULL)) {
        return gMcu_HwInterface->GetResetReason();
    }

    /* Default to unknown if no hardware interface */
    return MCU_RESET_UNKNOWN;
}

uint32_t Mcu_GetResetRawValue(void)
{
    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->GetResetRawValue != NULL)) {
        return gMcu_HwInterface->GetResetRawValue();
    }

    return 0U;
}

void Mcu_ClearResetReason(void)
{
    /* Platform-specific implementation would clear reset register */
}

bool Mcu_WasResetBy(Mcu_ResetType resetType)
{
    return (Mcu_GetResetReason() == resetType);
}

/*============================================================================*
 * Mode Management API
 *============================================================================*/

Mcu_ErrorCode_t Mcu_SetMode(Mcu_ModeType mode)
{
    Mcu_ErrorCode_t result;

    if (!gMcu_ModuleState.initialized) {
        return MCU_E_UNINIT;
    }

    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->SetMode != NULL)) {
        result = gMcu_HwInterface->SetMode(mode);
        if (result == MCU_OK) {
            gMcu_ModuleState.currentMode = mode;
        }
        return result;
    }

    gMcu_ModuleState.currentMode = mode;
    return MCU_OK;
}

Mcu_ModeType Mcu_GetMode(void)
{
    if (!gMcu_ModuleState.initialized) {
        return MCU_MODE_NORMAL;
    }

    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->GetMode != NULL)) {
        return gMcu_HwInterface->GetMode();
    }

    return gMcu_ModuleState.currentMode;
}

bool Mcu_IsModeAvailable(Mcu_ModeType mode)
{
    uint8_t i;

    if (gMcu_ModuleState.config == NULL) {
        return false;
    }

    for (i = 0U; i < gMcu_ModuleState.config->modeConfigCount; i++) {
        if (gMcu_ModuleState.config->modeConfigs[i].mode == mode) {
            return true;
        }
    }

    return false;
}

Mcu_ErrorCode_t Mcu_EnterSleepMode(void)
{
    return Mcu_SetMode(MCU_MODE_SLEEP);
}

Mcu_ErrorCode_t Mcu_EnterDeepSleepMode(void)
{
    return Mcu_SetMode(MCU_MODE_DEEP_SLEEP);
}

Mcu_ErrorCode_t Mcu_EnterStandbyMode(void)
{
    return Mcu_SetMode(MCU_MODE_STANDBY);
}

Mcu_ErrorCode_t Mcu_WakeUp(void)
{
    return Mcu_SetMode(MCU_MODE_NORMAL);
}

/*============================================================================*
 * Watchdog API
 *============================================================================*/

Mcu_ErrorCode_t Mcu_EnableWatchdog(uint32_t timeoutMs)
{
    (void)timeoutMs;
    /* Platform-specific watchdog enable */
    return MCU_OK;
}

Mcu_ErrorCode_t Mcu_DisableWatchdog(void)
{
    /* Platform-specific watchdog disable */
    return MCU_OK;
}

void Mcu_ServiceWatchdog(void)
{
    /* Platform-specific watchdog service/kick */
}

bool Mcu_WasWatchdogReset(void)
{
    return Mcu_WasResetBy(MCU_RESET_WATCHDOG);
}

/*============================================================================*
 * Main Function
 *============================================================================*/

void Mcu_MainFunction(void)
{
    if (!gMcu_ModuleState.initialized) {
        return;
    }

    /* Service watchdog if enabled */
    Mcu_ServiceWatchdog();

    /* Call hardware main function if available */
    if ((gMcu_HwInterface != NULL) && (gMcu_HwInterface->MainFunction != NULL)) {
        gMcu_HwInterface->MainFunction();
    }
}

/*============================================================================*
 * Version Info API
 *============================================================================*/

Mcu_ErrorCode_t Mcu_GetVersionInfo(uint8_t* major, uint8_t* minor, uint8_t* patch)
{
    if ((major == NULL) || (minor == NULL) || (patch == NULL)) {
        return MCU_E_POINTER;
    }

    *major = MCU_MAJOR_VERSION;
    *minor = MCU_MINOR_VERSION;
    *patch = MCU_PATCH_VERSION;

    return MCU_OK;
}

/*============================================================================*
 * Hardware Interface Registration
 *============================================================================*/

Mcu_ErrorCode_t Mcu_RegisterHwInterface(const Mcu_HwInterfaceType* hwInterface)
{
    if (hwInterface == NULL) {
        return MCU_E_POINTER;
    }

    if (gMcu_ModuleState.initialized) {
        return MCU_E_ALREADY_INITIALIZED;
    }

    gMcu_HwInterface = hwInterface;
    return MCU_OK;
}

const Mcu_HwInterfaceType* Mcu_GetHwInterface(void)
{
    return gMcu_HwInterface;
}

/*============================================================================*
 * Utility Functions
 *============================================================================*/

void Mcu_DelayUs(uint32_t us)
{
    /* Simple busy-wait delay */
    /* In a real implementation, use a hardware timer */
    volatile uint32_t i;
    /* Approximate - depends on CPU frequency */
    for (i = 0U; i < (us * 10U); i++) {
        __asm__ volatile ("nop");
    }
}

void Mcu_DelayMs(uint32_t ms)
{
    uint32_t i;
    for (i = 0U; i < ms; i++) {
        Mcu_DelayUs(1000U);
    }
}

Mcu_HardwareType Mcu_GetHardwareType(void)
{
    if ((gMcu_ModuleState.config != NULL) &&
        (gMcu_ModuleState.config->general != NULL)) {
        return gMcu_ModuleState.config->general->hwType;
    }
    return MCU_HW_GENERIC;
}

Mcu_ErrorCode_t Mcu_GetDeviceId(uint8_t* id, uint8_t* size)
{
    if ((id == NULL) || (size == NULL)) {
        return MCU_E_POINTER;
    }

    /* Platform-specific device ID retrieval */
    /* For generic implementation, return zeros */
    (void)memset(id, 0, *size);

    return MCU_OK;
}
