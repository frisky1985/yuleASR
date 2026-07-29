/**
 * @file Port.c
 * @brief PORT Driver implementation for i.MX8M Mini
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: PORT Driver
 * Layer: MCAL (Microcontroller Driver Layer)
 * Platform: i.MX8M Mini (NXP)
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Port.h"
#include "Port_Cfg.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define PORT_IOMUXC_BASE_ADDR           (0x30330000UL)
#define PORT_GPIO1_BASE_ADDR            (0x30200000UL)
#define PORT_GPIO2_BASE_ADDR            (0x30210000UL)
#define PORT_GPIO3_BASE_ADDR            (0x30220000UL)
#define PORT_GPIO4_BASE_ADDR            (0x30230000UL)
#define PORT_GPIO5_BASE_ADDR            (0x30240000UL)

#define PORT_IOMUXC_SW_MUX_CTL_PAD_OFFSET   (0x0000)
#define PORT_IOMUXC_SW_PAD_CTL_PAD_OFFSET   (0x0204)

#define PORT_GPIO_DR                    (0x00)
#define PORT_GPIO_GDIR                  (0x04)
#define PORT_GPIO_PSR                   (0x08)
#define PORT_GPIO_ICR1                  (0x0C)
#define PORT_GPIO_ICR2                  (0x10)
#define PORT_GPIO_IMR                   (0x14)
#define PORT_GPIO_ISR                   (0x18)
#define PORT_GPIO_EDGE_SEL              (0x1C)

#define PORT_MUX_MODE_GPIO              (5U)
#define PORT_MUX_MODE_ALT0              (0U)
#define PORT_MUX_MODE_ALT1              (1U)
#define PORT_MUX_MODE_ALT2              (2U)
#define PORT_MUX_MODE_ALT3              (3U)
#define PORT_MUX_MODE_ALT4              (4U)
#define PORT_MUX_MODE_ALT5              (5U)
#define PORT_MUX_MODE_ALT6              (6U)

#define PORT_SRE_SLOW                   (0U << 0)
#define PORT_SRE_FAST                   (1U << 0)
#define PORT_DSE_DISABLE                (0U << 1)
#define PORT_DSE_ENABLE                 (1U << 1)
#define PORT_HYS_DISABLE                (0U << 2)
#define PORT_HYS_ENABLE                 (1U << 2)
#define PORT_PUS_DOWN                   (0U << 3)
#define PORT_PUS_UP                     (1U << 3)
#define PORT_PUE_KEEPER                 (0U << 4)
#define PORT_PUE_PULL                   (1U << 4)
#define PORT_ODE_DISABLE                (0U << 5)
#define PORT_ODE_ENABLE                 (1U << 5)

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
typedef struct {
    boolean initialized;
    const Port_ConfigType* configPtr;
} Port_DriverStateType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define PORT_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static Port_DriverStateType Port_DriverState = {
    .initialized = FALSE,
    .configPtr = NULL_PTR
};

#define PORT_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static uint32 Port_GetGpioBaseAddr(uint8 port);
static uint32 Port_GetMuxRegisterAddr(Port_PinType pin);
static uint32 Port_GetPadRegisterAddr(Port_PinType pin);
static void Port_ConfigurePinMux(Port_PinType pin, Port_PinModeType mode);
static void Port_ConfigurePinPad(Port_PinType pin, const Port_PinConfigType* config);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Gets GPIO base address for a port
 */
static uint32 Port_GetGpioBaseAddr(uint8 port)
{
    uint32 baseAddr;

    switch (port) {
        case PORT_A:
            baseAddr = PORT_GPIO1_BASE_ADDR;
            break;
        case PORT_B:
            baseAddr = PORT_GPIO2_BASE_ADDR;
            break;
        case PORT_C:
            baseAddr = PORT_GPIO3_BASE_ADDR;
            break;
        case PORT_D:
            baseAddr = PORT_GPIO4_BASE_ADDR;
            break;
        case PORT_E:
            baseAddr = PORT_GPIO5_BASE_ADDR;
            break;
        default:
            baseAddr = 0U;
            break;
    }

    return baseAddr;
}

/**
 * @brief Gets IOMUXC mux register address for a pin
 */
static uint32 Port_GetMuxRegisterAddr(Port_PinType pin)
{
    uint8 port = (uint8)(pin >> 8);
    uint8 pinNum = (uint8)(pin & 0xFF);
    uint32 regAddr;

    regAddr = PORT_IOMUXC_BASE_ADDR + PORT_IOMUXC_SW_MUX_CTL_PAD_OFFSET +
              ((port * 32U + pinNum) * 4U);

    return regAddr;
}

/**
 * @brief Gets IOMUXC pad register address for a pin
 */
static uint32 Port_GetPadRegisterAddr(Port_PinType pin)
{
    uint8 port = (uint8)(pin >> 8);
    uint8 pinNum = (uint8)(pin & 0xFF);
    uint32 regAddr;

    regAddr = PORT_IOMUXC_BASE_ADDR + PORT_IOMUXC_SW_PAD_CTL_PAD_OFFSET +
              ((port * 32U + pinNum) * 4U);

    return regAddr;
}

/**
 * @brief Configures pin mux mode
 */
static void Port_ConfigurePinMux(Port_PinType pin, Port_PinModeType mode)
{
    uint32 muxRegAddr = Port_GetMuxRegisterAddr(pin);
    uint32 muxValue;

    muxValue = REG_READ32(muxRegAddr);
    muxValue &= ~0x07U; /* Clear MUX_MODE bits */

    switch (mode) {
        case PORT_PIN_MODE_GPIO:
            muxValue |= PORT_MUX_MODE_GPIO;
            break;
        case PORT_PIN_MODE_CAN:
            muxValue |= PORT_MUX_MODE_ALT2;
            break;
        case PORT_PIN_MODE_SPI:
            muxValue |= PORT_MUX_MODE_ALT3;
            break;
        case PORT_PIN_MODE_UART:
            muxValue |= PORT_MUX_MODE_ALT1;
            break;
        case PORT_PIN_MODE_I2C:
            muxValue |= PORT_MUX_MODE_ALT0;
            break;
        case PORT_PIN_MODE_PWM:
            muxValue |= PORT_MUX_MODE_ALT4;
            break;
        case PORT_PIN_MODE_ADC:
            muxValue |= PORT_MUX_MODE_ALT5;
            break;
        case PORT_PIN_MODE_ETH:
            muxValue |= PORT_MUX_MODE_ALT6;
            break;
        default:
            muxValue |= PORT_MUX_MODE_GPIO;
            break;
    }

    REG_WRITE32(muxRegAddr, muxValue);
}

/**
 * @brief Configures pin pad settings
 */
static void Port_ConfigurePinPad(Port_PinType pin, const Port_PinConfigType* config)
{
    uint32 padRegAddr = Port_GetPadRegisterAddr(pin);
    uint32 padValue = 0U;

    /* Configure slew rate */
    if (config->Mode == PORT_PIN_MODE_GPIO) {
        padValue |= PORT_SRE_SLOW;
    } else {
        padValue |= PORT_SRE_FAST;
    }

    /* Configure drive strength */
    padValue |= PORT_DSE_ENABLE;

    /* Configure hysteresis */
    padValue |= PORT_HYS_DISABLE;

    /* Configure pull up/down */
    if (config->PullUpEnable) {
        padValue |= PORT_PUS_UP;
        padValue |= PORT_PUE_PULL;
    } else if (config->PullDownEnable) {
        padValue |= PORT_PUS_DOWN;
        padValue |= PORT_PUE_PULL;
    } else {
        padValue |= PORT_PUE_KEEPER;
    }

    /* Configure open drain */
    padValue |= PORT_ODE_DISABLE;

    REG_WRITE32(padRegAddr, padValue);
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
#define PORT_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the PORT driver
 */
void Port_Init(const Port_ConfigType* ConfigPtr)
{
    #if (PORT_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(PORT_MODULE_ID, 0U, PORT_SID_INIT, PORT_E_PARAM_CONFIG);
        return;
    }

    if (Port_DriverState.initialized == TRUE) {
        Det_ReportError(PORT_MODULE_ID, 0U, PORT_SID_INIT, PORT_E_ALREADY_INITIALIZED);
        return;
    }
    #endif

    /* Configure all pins */
    for (uint16 i = 0U; i < ConfigPtr->NumPins; i++) {
        const Port_PinConfigType* pinConfig = &ConfigPtr->PinConfigs[i];

        /* Configure pin mux */
        Port_ConfigurePinMux(pinConfig->Pin, pinConfig->Mode);

        /* Configure pin pad settings */
        Port_ConfigurePinPad(pinConfig->Pin, pinConfig);

        /* Configure GPIO direction if GPIO mode */
        if (pinConfig->Mode == PORT_PIN_MODE_GPIO) {
            uint8 port = (uint8)(pinConfig->Pin >> 8);
            uint8 pinNum = (uint8)(pinConfig->Pin & 0xFF);
            uint32 gpioBase = Port_GetGpioBaseAddr(port);
            uint32 gdirValue;

            gdirValue = REG_READ32(gpioBase + PORT_GPIO_GDIR);

            if (pinConfig->Direction == PORT_PIN_OUT) {
                gdirValue |= (1U << pinNum);

                /* Set initial level */
                uint32 drValue = REG_READ32(gpioBase + PORT_GPIO_DR);
                if (pinConfig->InitialLevel == PORT_PIN_LEVEL_HIGH) {
                    drValue |= (1U << pinNum);
                } else {
                    drValue &= ~(1U << pinNum);
                }
                REG_WRITE32(gpioBase + PORT_GPIO_DR, drValue);
            } else {
                gdirValue &= ~(1U << pinNum);
            }

            REG_WRITE32(gpioBase + PORT_GPIO_GDIR, gdirValue);
        }
    }

    Port_DriverState.configPtr = ConfigPtr;
    Port_DriverState.initialized = TRUE;
}

/**
 * @brief Sets pin direction
 */
#if (PORT_SET_PIN_DIRECTION_API == STD_ON)
void Port_SetPinDirection(Port_PinType Pin, Port_PinDirectionType Direction)
{
    #if (PORT_DEV_ERROR_DETECT == STD_ON)
    if (Port_DriverState.initialized == FALSE) {
        Det_ReportError(PORT_MODULE_ID, 0U, PORT_SID_SET_PIN_DIRECTION, PORT_E_UNINIT);
        return;
    }

    if (Pin >= PORT_TOTAL_NUM_PINS) {
        Det_ReportError(PORT_MODULE_ID, 0U, PORT_SID_SET_PIN_DIRECTION, PORT_E_PARAM_PIN);
        return;
    }
    #endif

    /* Check if direction is changeable */
    boolean directionChangeable = FALSE;
    for (uint16 i = 0U; i < Port_DriverState.configPtr->NumPins; i++) {
        if (Port_DriverState.configPtr->PinConfigs[i].Pin == Pin) {
            directionChangeable = Port_DriverState.configPtr->PinConfigs[i].DirectionChangeable;
            break;
        }
    }

    #if (PORT_DEV_ERROR_DETECT == STD_ON)
    if (directionChangeable == FALSE) {
        Det_ReportError(PORT_MODULE_ID, 0U, PORT_SID_SET_PIN_DIRECTION, PORT_E_DIRECTION_UNCHANGEABLE);
        return;
    }
    #endif

    uint8 port = (uint8)(Pin >> 8);
    uint8 pinNum = (uint8)(Pin & 0xFF);
    uint32 gpioBase = Port_GetGpioBaseAddr(port);
    uint32 gdirValue;

    gdirValue = REG_READ32(gpioBase + PORT_GPIO_GDIR);

    if (Direction == PORT_PIN_OUT) {
        gdirValue |= (1U << pinNum);
    } else {
        gdirValue &= ~(1U << pinNum);
    }

    REG_WRITE32(gpioBase + PORT_GPIO_GDIR, gdirValue);
}
#endif

/**
 * @brief Refreshes port direction
 */
void Port_RefreshPortDirection(void)
{
    #if (PORT_DEV_ERROR_DETECT == STD_ON)
    if (Port_DriverState.initialized == FALSE) {
        Det_ReportError(PORT_MODULE_ID, 0U, PORT_SID_REFRESH_PORT_DIRECTION, PORT_E_UNINIT);
        return;
    }
    #endif

    /* Refresh all pin directions from configuration */
    for (uint16 i = 0U; i < Port_DriverState.configPtr->NumPins; i++) {
        const Port_PinConfigType* pinConfig = &Port_DriverState.configPtr->PinConfigs[i];

        if (pinConfig->Mode == PORT_PIN_MODE_GPIO) {
            uint8 port = (uint8)(pinConfig->Pin >> 8);
            uint8 pinNum = (uint8)(pinConfig->Pin & 0xFF);
            uint32 gpioBase = Port_GetGpioBaseAddr(port);
            uint32 gdirValue;

            gdirValue = REG_READ32(gpioBase + PORT_GPIO_GDIR);

            if (pinConfig->Direction == PORT_PIN_OUT) {
                gdirValue |= (1U << pinNum);
            } else {
                gdirValue &= ~(1U << pinNum);
            }

            REG_WRITE32(gpioBase + PORT_GPIO_GDIR, gdirValue);
        }
    }
}

/**
 * @brief Gets version information
 */
#if (PORT_VERSION_INFO_API == STD_ON)
void Port_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (PORT_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(PORT_MODULE_ID, 0U, PORT_SID_GET_VERSION_INFO, PORT_E_PARAM_POINTER);
        return;
    }
    #endif

    versioninfo->vendorID = PORT_VENDOR_ID;
    versioninfo->moduleID = PORT_MODULE_ID;
    versioninfo->sw_major_version = PORT_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = PORT_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = PORT_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Sets pin mode
 */
#if (PORT_SET_PIN_MODE_API == STD_ON)
void Port_SetPinMode(Port_PinType Pin, Port_PinModeType Mode)
{
    #if (PORT_DEV_ERROR_DETECT == STD_ON)
    if (Port_DriverState.initialized == FALSE) {
        Det_ReportError(PORT_MODULE_ID, 0U, PORT_SID_SET_PIN_MODE, PORT_E_UNINIT);
        return;
    }

    if (Pin >= PORT_TOTAL_NUM_PINS) {
        Det_ReportError(PORT_MODULE_ID, 0U, PORT_SID_SET_PIN_MODE, PORT_E_PARAM_PIN);
        return;
    }

    if (Mode > PORT_PIN_MODE_DISABLED) {
        Det_ReportError(PORT_MODULE_ID, 0U, PORT_SID_SET_PIN_MODE, PORT_E_PARAM_INVALID_MODE);
        return;
    }
    #endif

    /* Check if mode is changeable */
    boolean modeChangeable = FALSE;
    for (uint16 i = 0U; i < Port_DriverState.configPtr->NumPins; i++) {
        if (Port_DriverState.configPtr->PinConfigs[i].Pin == Pin) {
            modeChangeable = Port_DriverState.configPtr->PinConfigs[i].ModeChangeable;
            break;
        }
    }

    #if (PORT_DEV_ERROR_DETECT == STD_ON)
    if (modeChangeable == FALSE) {
        Det_ReportError(PORT_MODULE_ID, 0U, PORT_SID_SET_PIN_MODE, PORT_E_MODE_UNCHANGEABLE);
        return;
    }
    #endif

    /* Configure pin mux */
    Port_ConfigurePinMux(Pin, Mode);
}
#endif

#define PORT_STOP_SEC_CODE
#include "MemMap.h"
