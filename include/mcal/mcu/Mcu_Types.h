/**
 * @file Mcu_Types.h
 * @brief Mcu (Microcontroller Driver) Types Definition
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Mcu Module - Microcontroller Driver
 * Compliant with AUTOSAR R22-11 MCAL Specification
 * Module ID: 0x12
 */

#ifndef MCU_TYPES_H
#define MCU_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================*
 * Version Information
 *============================================================================*/
#define MCU_MAJOR_VERSION           1u
#define MCU_MINOR_VERSION           0u
#define MCU_PATCH_VERSION           0u

#define MCU_MODULE_NAME             "Mcu"
#define MCU_VENDOR_ID               0x00u
#define MCU_MODULE_ID               0x12u    /* MCAL Module ID */
#define MCU_AR_MAJOR_VERSION        4u
#define MCU_AR_MINOR_VERSION        4u
#define MCU_AR_PATCH_VERSION        0u

/*============================================================================*
 * Configuration Constants
 *============================================================================*/
#define MCU_MAX_CLOCK_SETTINGS      16u     /* Maximum clock configurations */
#define MCU_MAX_RAM_SECTIONS        32u     /* Maximum RAM sections */
#define MCU_MAX_RESET_REASONS       16u     /* Maximum reset reasons */
#define MCU_MAX_MODE_CONFIGS        8u      /* Maximum mode configurations */
#define MCU_MAX_CORES               8u      /* Maximum CPU cores */
#define MCU_MAX_PERIPH_CLOCKS       64u     /* Maximum peripheral clocks */

/* OSC frequency limits */
#define MCU_MIN_OSC_FREQ_HZ         4000000u    /* 4 MHz minimum */
#define MCU_MAX_OSC_FREQ_HZ         50000000u   /* 50 MHz maximum */

/* PLL frequency limits */
#define MCU_MIN_PLL_FREQ_HZ         16000000u   /* 16 MHz minimum */
#define MCU_MAX_PLL_FREQ_HZ         800000000u  /* 800 MHz maximum */

/* System clock limits */
#define MCU_MIN_SYSCLK_FREQ_HZ      1000000u    /* 1 MHz minimum */
#define MCU_MAX_SYSCLK_FREQ_HZ      600000000u  /* 600 MHz maximum */

/* Watchdog timeout */
#define MCU_WDG_TIMEOUT_MIN_MS      1u
#define MCU_WDG_TIMEOUT_MAX_MS      10000u

/*============================================================================*
 * Error Codes (AUTOSAR Standard)
 *============================================================================*/
typedef enum {
    MCU_OK                      = 0x00u,    /* Operation successful */
    MCU_E_NOT_OK                = 0x01u,    /* General error */
    MCU_E_TIMEOUT               = 0x02u,    /* Operation timeout */
    MCU_E_PARAM_CLOCK           = 0x03u,    /* Invalid clock setting */
    MCU_E_PARAM_MODE            = 0x04u,    /* Invalid mode */
    MCU_E_PARAM_RAM             = 0x05u,    /* Invalid RAM section */
    MCU_E_PLL_NOT_LOCKED        = 0x06u,    /* PLL not locked */
    MCU_E_CLOCK_FAILURE         = 0x07u,    /* Clock failure detected */
    MCU_E_RESET_FAILURE         = 0x08u,    /* Reset failure */
    MCU_E_UNINIT                = 0x09u,    /* Module not initialized */
    MCU_E_ALREADY_INITIALIZED   = 0x0Au,    /* Already initialized */
    MCU_E_POINTER               = 0x0Bu,    /* Invalid pointer */
    MCU_E_PARAM_POINTER         = MCU_E_POINTER,
    MCU_E_CLOCK_NOT_CONFIGURED  = 0x0Cu,    /* Clock not configured */
    MCU_E_PERIPH_NOT_ENABLED    = 0x0Du,    /* Peripheral not enabled */
    MCU_E_MODE_NOT_AVAILABLE    = 0x0Eu,    /* Mode not available */
    MCU_E_WAKEUP_NOT_AVAILABLE  = 0x0Fu,    /* Wake-up not available */
    MCU_E_HW_ERROR              = 0x10u     /* Hardware error */
} Mcu_ErrorCode_t;

/*============================================================================*
 * Clock Source Type
 *============================================================================*/
typedef enum {
    MCU_CLOCK_SRC_OSC           = 0x00u,    /* External oscillator */
    MCU_CLOCK_SRC_OSC32K        = 0x01u,    /* 32.768 kHz oscillator */
    MCU_CLOCK_SRC_IRC           = 0x02u,    /* Internal RC oscillator */
    MCU_CLOCK_SRC_PLL           = 0x03u,    /* PLL output */
    MCU_CLOCK_SRC_BACKUP        = 0x04u,    /* Backup clock */
    MCU_CLOCK_SRC_NONE          = 0xFFu     /* No clock source */
} Mcu_ClockSourceType;

/*============================================================================*
 * Clock Type
 *============================================================================*/
typedef enum {
    MCU_CLOCK_SYS               = 0x00u,    /* System clock */
    MCU_CLOCK_CPU               = 0x01u,    /* CPU clock */
    MCU_CLOCK_BUS               = 0x02u,    /* Bus clock (AHB/APB) */
    MCU_CLOCK_PERIPH            = 0x03u,    /* Peripheral clock */
    MCU_CLOCK_FLASH             = 0x04u,    /* Flash clock */
    MCU_CLOCK_RTC               = 0x05u,    /* RTC clock */
    MCU_CLOCK_WDG               = 0x06u,    /* Watchdog clock */
    MCU_CLOCK_ADC               = 0x07u,    /* ADC clock */
    MCU_CLOCK_ETH               = 0x08u,    /* Ethernet clock */
    MCU_CLOCK_CAN               = 0x09u,    /* CAN clock */
    MCU_CLOCK_QSPI              = 0x0Au,    /* QSPI clock */
    MCU_CLOCK_SPI               = 0x0Bu,    /* SPI clock */
    MCU_CLOCK_UART              = 0x0Cu,    /* UART clock */
    MCU_CLOCK_I2C               = 0x0Du,    /* I2C clock */
    MCU_CLOCK_PWM               = 0x0Eu,    /* PWM clock */
    MCU_CLOCK_GPT               = 0x0Fu     /* GPT clock */
} Mcu_ClockType;

/*============================================================================*
 * Reset Type
 *============================================================================*/
typedef enum {
    MCU_RESET_POWER_ON          = 0x00u,    /* Power-on reset */
    MCU_RESET_WATCHDOG          = 0x01u,    /* Watchdog reset */
    MCU_RESET_SW                = 0x02u,    /* Software reset */
    MCU_RESET_EXTERNAL          = 0x03u,    /* External reset pin */
    MCU_RESET_OSC_FAILURE       = 0x04u,    /* Oscillator failure */
    MCU_RESET_PLL_FAILURE       = 0x05u,    /* PLL failure */
    MCU_RESET_CPU_LOCKUP        = 0x06u,    /* CPU lockup */
    MCU_RESET_DEBUG             = 0x07u,    /* Debug reset */
    MCU_RESET_LOW_VOLTAGE       = 0x08u,    /* Low voltage reset */
    MCU_RESET_WAKEUP            = 0x09u,    /* Wake-up from low power */
    MCU_RESET_BROWNOUT          = 0x0Au,    /* Brown-out reset */
    MCU_RESET_JTAG              = 0x0Bu,    /* JTAG reset */
    MCU_RESET_UNKNOWN           = 0xFFu     /* Unknown reset */
} Mcu_ResetType;

/*============================================================================*
 * Mode Type
 *============================================================================*/
typedef enum {
    MCU_MODE_NORMAL             = 0x00u,    /* Normal operating mode */
    MCU_MODE_RUN                = MCU_MODE_NORMAL,
    MCU_MODE_SLEEP              = 0x01u,    /* Sleep mode */
    MCU_MODE_DEEP_SLEEP         = 0x02u,    /* Deep sleep mode */
    MCU_MODE_POWER_DOWN         = 0x03u,    /* Power down mode */
    MCU_MODE_STANDBY            = 0x04u,    /* Standby mode */
    MCU_MODE_STOP               = 0x05u,    /* Stop mode */
    MCU_MODE_HALT               = 0x06u     /* Halt mode */
} Mcu_ModeType;

/*============================================================================*
 * RAM Section Type
 *============================================================================*/
typedef enum {
    MCU_RAM_SECTION_DATA        = 0x00u,    /* Data RAM */
    MCU_RAM_SECTION_BSS         = 0x01u,    /* BSS RAM */
    MCU_RAM_SECTION_STACK       = 0x02u,    /* Stack RAM */
    MCU_RAM_SECTION_HEAP        = 0x03u,    /* Heap RAM */
    MCU_RAM_SECTION_CACHE       = 0x04u,    /* Cache RAM */
    MCU_RAM_SECTION_TCM         = 0x05u,    /* Tightly Coupled Memory */
    MCU_RAM_SECTION_ECC         = 0x06u,    /* ECC-protected RAM */
    MCU_RAM_SECTION_RETENTION   = 0x07u     /* Retention RAM */
} Mcu_RamSectionType;

/*============================================================================*
 * Hardware Type
 *============================================================================*/
typedef enum {
    MCU_HW_GENERIC              = 0x00u,    /* Generic/Emulator */
    MCU_HW_AURIX_TC3XX          = 0x01u,    /* Infineon Aurix TC3xx */
    MCU_HW_AURIX_TC4XX          = 0x02u,    /* Infineon Aurix TC4xx */
    MCU_HW_S32G3                = 0x03u,    /* NXP S32G3 */
    MCU_HW_S32K3                = 0x04u,    /* NXP S32K3 */
    MCU_HW_S32Z                 = 0x05u,    /* NXP S32Z/E */
    MCU_HW_STM32H7              = 0x06u,    /* STM32 H7 series */
    MCU_HW_STM32MP1             = 0x07u,    /* STM32MP1 series */
    MCU_HW_RH850                = 0x08u,    /* Renesas RH850 */
    MCU_HW_RCAR                 = 0x09u,    /* Renesas R-Car */
    MCU_HW_TMS570               = 0x0Au,    /* TI TMS570 */
    MCU_HW_POSIX                = 0xFFu     /* POSIX Simulation */
} Mcu_HardwareType;

/*============================================================================*
 * PLL Configuration
 *============================================================================*/
typedef struct {
    uint32_t inputFreqHz;                   /* Input frequency */
    uint32_t outputFreqHz;                  /* Output frequency */
    uint16_t multiplier;                    /* PLL multiplier (N) */
    uint16_t predivider;                    /* PLL predivider (P) */
    uint16_t postdivider1;                  /* PLL postdivider 1 (R) */
    uint16_t postdivider2;                  /* PLL postdivider 2 (optional) */
    bool enabled;                           /* PLL enabled */
    uint32_t lockTimeout;                   /* Lock timeout (us) */
} Mcu_PllConfigType;

/*============================================================================*
 * Clock Configuration
 *============================================================================*/
typedef struct {
    Mcu_ClockSourceType source;             /* Clock source */
    uint32_t freqHz;                        /* Clock frequency */
    uint16_t divider;                       /* Clock divider */
    bool enabled;                           /* Clock enabled */
} Mcu_ClockConfigType;

/*============================================================================*
 * Clock Setting Configuration
 *============================================================================*/
typedef struct {
    uint8_t settingId;                      /* Clock setting ID */
    
    /* OSC configuration */
    uint32_t oscFreqHz;                     /* OSC frequency */
    bool oscEnabled;                        /* OSC enabled */
    bool oscBypass;                         /* OSC bypass (external clock) */
    
    /* PLL configuration */
    Mcu_PllConfigType pll;                  /* PLL configuration */
    
    /* Clock outputs */
    uint32_t sysClockFreqHz;                /* System clock frequency */
    uint32_t cpuClockFreqHz;                /* CPU clock frequency */
    uint32_t busClockFreqHz;                /* Bus clock frequency */
    uint32_t flashClockFreqHz;              /* Flash clock frequency */
    
    /* Peripheral clocks */
    uint32_t periphClocks[MCU_MAX_PERIPH_CLOCKS];
    uint8_t periphClockCount;
    
    /* Flash wait states */
    uint8_t flashWaitStates;                /* Flash wait states */
    
    /* Watchdog configuration */
    uint32_t wdgTimeoutMs;                  /* Watchdog timeout */
    bool wdgEnabled;                        /* Watchdog enabled */
} Mcu_ClockSettingConfigType;

/*============================================================================*
 * RAM Section Configuration
 *============================================================================*/
typedef struct {
    uint8_t sectionId;                      /* Section ID */
    Mcu_RamSectionType sectionType;         /* Section type */
    uint32_t startAddress;                  /* Start address */
    uint32_t size;                          /* Section size (bytes) */
    uint8_t initValue;                      /* Initialization value */
    bool initEnabled;                       /* Initialize on startup */
    bool eccEnabled;                        /* ECC enabled */
    bool retention;                         /* Retention in low power */
} Mcu_RamSectionConfigType;

/*============================================================================*
 * Mode Configuration
 *============================================================================*/
typedef struct {
    Mcu_ModeType mode;                      /* Power mode */
    uint32_t wakeupSources;                 /* Wake-up sources bitmap */
    uint32_t wakeupTimeout;                 /* Wake-up timeout (us) */
    bool ramRetention;                      /* RAM retention enabled */
    bool clockRetention;                    /* Clock retention enabled */
    uint8_t voltageRegulatorMode;           /* Voltage regulator mode */
} Mcu_ModeConfigType;

/*============================================================================*
 * Reset Configuration
 *============================================================================*/
typedef struct {
    bool swResetEnabled;                    /* Software reset enabled */
    bool wdgResetEnabled;                   /* Watchdog reset enabled */
    bool extResetEnabled;                   /* External reset enabled */
    bool jtagResetEnabled;                  /* JTAG reset enabled */
    uint32_t resetDelay;                    /* Reset delay (us) */
} Mcu_ResetConfigType;

/*============================================================================*
 * General Configuration
 *============================================================================*/
typedef struct {
    Mcu_HardwareType hwType;                /* Hardware type */
    bool devErrorDetect;                    /* Development error detection */
    bool versionInfoApi;                    /* Version info API enable */
    bool ramInitEnabled;                    /* RAM initialization enabled */
    bool clockInitEnabled;                  /* Clock initialization enabled */
    bool noInitOnReset;                     /* Skip init after certain resets */
    bool performReset;                      /* Perform reset API enabled */
    bool resetReasonEnabled;                /* Reset reason API enabled */
} Mcu_GeneralConfigType;

/*============================================================================*
 * MCU Configuration
 *============================================================================*/
typedef struct {
    const Mcu_GeneralConfigType* general;           /* General configuration */
    const Mcu_ClockSettingConfigType* clockSettings; /* Clock settings array */
    const Mcu_RamSectionConfigType* ramSections;    /* RAM sections array */
    const Mcu_ModeConfigType* modeConfigs;          /* Mode configs array */
    const Mcu_ResetConfigType* resetConfig;         /* Reset configuration */
    uint8_t clockSettingCount;                      /* Number of clock settings */
    uint8_t ramSectionCount;                        /* Number of RAM sections */
    uint8_t modeConfigCount;                        /* Number of mode configs */
    uint8_t defaultClockSetting;                    /* Default clock setting ID */
} Mcu_ConfigType;

/*============================================================================*
 * Module State
 *============================================================================*/
typedef struct {
    bool initialized;                       /* Module initialized */
    Mcu_ResetType lastReset;                /* Last reset reason */
    uint8_t currentClockSetting;            /* Current clock setting ID */
    Mcu_ModeType currentMode;               /* Current power mode */
    uint32_t sysClockFreqHz;                /* Current system clock */
    uint32_t cpuClockFreqHz;                /* Current CPU clock */
    uint32_t busClockFreqHz;                /* Current bus clock */
    const Mcu_ConfigType* config;           /* Current configuration */
} Mcu_ModuleStateType;

/*============================================================================*
 * Hardware Interface (Abstract Layer)
 *============================================================================*/
typedef struct {
    /* Initialize hardware */
    Mcu_ErrorCode_t (*Init)(const Mcu_ConfigType* config);
    
    /* Deinitialize hardware */
    Mcu_ErrorCode_t (*Deinit)(void);
    
    /* Initialize RAM section */
    Mcu_ErrorCode_t (*InitRamSection)(
        const Mcu_RamSectionConfigType* ramSection
    );
    
    /* Initialize clock */
    Mcu_ErrorCode_t (*InitClock)(
        const Mcu_ClockSettingConfigType* clockSetting
    );
    
    /* Distribute PLL clock */
    Mcu_ErrorCode_t (*DistributePllClock)(void);
    
    /* Get PLL status */
    bool (*GetPllStatus)(void);
    
    /* Set mode */
    Mcu_ErrorCode_t (*SetMode)(Mcu_ModeType mode);
    
    /* Get mode */
    Mcu_ModeType (*GetMode)(void);
    
    /* Perform reset */
    Mcu_ErrorCode_t (*PerformReset)(void);
    
    /* Get reset reason */
    Mcu_ResetType (*GetResetReason)(void);
    
    /* Get reset raw value */
    uint32_t (*GetResetRawValue)(void);
    
    /* Get clock frequency */
    uint32_t (*GetClockFrequency)(Mcu_ClockType clockType);
    
    /* Enable peripheral clock */
    Mcu_ErrorCode_t (*EnablePeriphClock)(uint8_t periphId);
    
    /* Disable peripheral clock */
    Mcu_ErrorCode_t (*DisablePeriphClock)(uint8_t periphId);
    
    /* Get peripheral clock status */
    bool (*IsPeriphClockEnabled)(uint8_t periphId);
    
    /* Main function (cyclic processing) */
    void (*MainFunction)(void);
} Mcu_HwInterfaceType;

#ifdef __cplusplus
}
#endif

#endif /* MCU_TYPES_H */
