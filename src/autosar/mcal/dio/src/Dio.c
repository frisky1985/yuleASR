/**
 * @file Dio.c
 * @brief DIO Driver implementation for i.MX8M Mini
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "Dio.h"
#include "Dio_Cfg.h"
#include "Det.h"

#define DIO_GPIO1_BASE_ADDR             (0x30200000UL)
#define DIO_GPIO2_BASE_ADDR             (0x30210000UL)
#define DIO_GPIO3_BASE_ADDR             (0x30220000UL)
#define DIO_GPIO4_BASE_ADDR             (0x30230000UL)
#define DIO_GPIO5_BASE_ADDR             (0x30240000UL)

#define DIO_GPIO_DR                     (0x00)
#define DIO_GPIO_GDIR                   (0x04)
#define DIO_GPIO_PSR                    (0x08)
#define DIO_GPIO_ICR1                   (0x0C)
#define DIO_GPIO_ICR2                   (0x10)
#define DIO_GPIO_IMR                    (0x14)
#define DIO_GPIO_ISR                    (0x18)
#define DIO_GPIO_EDGE_SEL               (0x1C)

#define DIO_GET_PORT(channel)           ((uint8)((channel) >> 8))
#define DIO_GET_PIN(channel)            ((uint8)((channel) & 0xFF))

#define DIO_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Dio_DriverInitialized = FALSE;

#define DIO_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static uint32 Dio_GetGpioBaseAddr(uint8 port)
{
    uint32 baseAddr;
    switch (port) {
        case DIO_PORT_A: baseAddr = DIO_GPIO1_BASE_ADDR; break;
        case DIO_PORT_B: baseAddr = DIO_GPIO2_BASE_ADDR; break;
        case DIO_PORT_C: baseAddr = DIO_GPIO3_BASE_ADDR; break;
        case DIO_PORT_D: baseAddr = DIO_GPIO4_BASE_ADDR; break;
        case DIO_PORT_E: baseAddr = DIO_GPIO5_BASE_ADDR; break;
        default: baseAddr = 0U; break;
    }
    return baseAddr;
}

#define DIO_START_SEC_CODE
#include "MemMap.h"

void Dio_Init(const Dio_ConfigType* ConfigPtr)
{
    #if (DIO_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_INIT, DIO_E_PARAM_CONFIG);
        return;
    }
    #endif
    (void)ConfigPtr;
    Dio_DriverInitialized = TRUE;
}

Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId)
{
    Dio_LevelType level;
    #if (DIO_DEV_ERROR_DETECT == STD_ON)
    if (Dio_DriverInitialized == FALSE) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_READ_CHANNEL, DIO_E_UNINIT);
        return STD_LOW;
    }
    if (ChannelId >= (DIO_NUM_PORTS * DIO_NUM_CHANNELS_PER_PORT)) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_READ_CHANNEL, DIO_E_PARAM_INVALID_CHANNEL_ID);
        return STD_LOW;
    }
    #endif

    uint8 port = DIO_GET_PORT(ChannelId);
    uint8 pin = DIO_GET_PIN(ChannelId);
    uint32 gpioBase = Dio_GetGpioBaseAddr(port);
    uint32 psrValue = REG_READ32(gpioBase + DIO_GPIO_PSR);

    level = ((psrValue & (1U << pin)) != 0U) ? STD_HIGH : STD_LOW;
    return level;
}

void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level)
{
    #if (DIO_DEV_ERROR_DETECT == STD_ON)
    if (Dio_DriverInitialized == FALSE) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_WRITE_CHANNEL, DIO_E_UNINIT);
        return;
    }
    if (ChannelId >= (DIO_NUM_PORTS * DIO_NUM_CHANNELS_PER_PORT)) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_WRITE_CHANNEL, DIO_E_PARAM_INVALID_CHANNEL_ID);
        return;
    }
    #endif

    uint8 port = DIO_GET_PORT(ChannelId);
    uint8 pin = DIO_GET_PIN(ChannelId);
    uint32 gpioBase = Dio_GetGpioBaseAddr(port);
    uint32 drValue = REG_READ32(gpioBase + DIO_GPIO_DR);

    if (Level == STD_HIGH) {
        drValue |= (1U << pin);
    } else {
        drValue &= ~(1U << pin);
    }
    REG_WRITE32(gpioBase + DIO_GPIO_DR, drValue);
}

Dio_PortLevelType Dio_ReadPort(Dio_PortType PortId)
{
    Dio_PortLevelType level;
    #if (DIO_DEV_ERROR_DETECT == STD_ON)
    if (Dio_DriverInitialized == FALSE) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_READ_PORT, DIO_E_UNINIT);
        return 0U;
    }
    if (PortId >= DIO_NUM_PORTS) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_READ_PORT, DIO_E_PARAM_INVALID_PORT_ID);
        return 0U;
    }
    #endif

    uint32 gpioBase = Dio_GetGpioBaseAddr((uint8)PortId);
    level = (Dio_PortLevelType)REG_READ32(gpioBase + DIO_GPIO_PSR);
    return level;
}

void Dio_WritePort(Dio_PortType PortId, Dio_PortLevelType Level)
{
    #if (DIO_DEV_ERROR_DETECT == STD_ON)
    if (Dio_DriverInitialized == FALSE) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_WRITE_PORT, DIO_E_UNINIT);
        return;
    }
    if (PortId >= DIO_NUM_PORTS) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_WRITE_PORT, DIO_E_PARAM_INVALID_PORT_ID);
        return;
    }
    #endif

    uint32 gpioBase = Dio_GetGpioBaseAddr((uint8)PortId);
    REG_WRITE32(gpioBase + DIO_GPIO_DR, (uint32)Level);
}

Dio_PortLevelType Dio_ReadChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr)
{
    Dio_PortLevelType level;
    #if (DIO_DEV_ERROR_DETECT == STD_ON)
    if (Dio_DriverInitialized == FALSE) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_READ_CHANNEL_GROUP, DIO_E_UNINIT);
        return 0U;
    }
    if (ChannelGroupIdPtr == NULL_PTR) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_READ_CHANNEL_GROUP, DIO_E_PARAM_POINTER);
        return 0U;
    }
    #endif

    uint32 gpioBase = Dio_GetGpioBaseAddr((uint8)ChannelGroupIdPtr->port);
    uint32 portValue = REG_READ32(gpioBase + DIO_GPIO_PSR);
    level = (Dio_PortLevelType)((portValue & ChannelGroupIdPtr->mask) >> ChannelGroupIdPtr->offset);
    return level;
}

void Dio_WriteChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr, Dio_PortLevelType Level)
{
    #if (DIO_DEV_ERROR_DETECT == STD_ON)
    if (Dio_DriverInitialized == FALSE) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_WRITE_CHANNEL_GROUP, DIO_E_UNINIT);
        return;
    }
    if (ChannelGroupIdPtr == NULL_PTR) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_WRITE_CHANNEL_GROUP, DIO_E_PARAM_POINTER);
        return;
    }
    #endif

    uint32 gpioBase = Dio_GetGpioBaseAddr((uint8)ChannelGroupIdPtr->port);
    uint32 drValue = REG_READ32(gpioBase + DIO_GPIO_DR);
    drValue &= ~(ChannelGroupIdPtr->mask);
    drValue |= ((uint32)Level << ChannelGroupIdPtr->offset) & ChannelGroupIdPtr->mask;
    REG_WRITE32(gpioBase + DIO_GPIO_DR, drValue);
}

#if (DIO_VERSION_INFO_API == STD_ON)
void Dio_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (DIO_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_GET_VERSION_INFO, DIO_E_PARAM_POINTER);
        return;
    }
    #endif
    versioninfo->vendorID = DIO_VENDOR_ID;
    versioninfo->moduleID = DIO_MODULE_ID;
    versioninfo->sw_major_version = DIO_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = DIO_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = DIO_SW_PATCH_VERSION;
}
#endif

#if (DIO_FLIP_CHANNEL_API == STD_ON)
Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId)
{
    Dio_LevelType newLevel;
    #if (DIO_DEV_ERROR_DETECT == STD_ON)
    if (Dio_DriverInitialized == FALSE) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_FLIP_CHANNEL, DIO_E_UNINIT);
        return STD_LOW;
    }
    if (ChannelId >= (DIO_NUM_PORTS * DIO_NUM_CHANNELS_PER_PORT)) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_FLIP_CHANNEL, DIO_E_PARAM_INVALID_CHANNEL_ID);
        return STD_LOW;
    }
    #endif

    uint8 port = DIO_GET_PORT(ChannelId);
    uint8 pin = DIO_GET_PIN(ChannelId);
    uint32 gpioBase = Dio_GetGpioBaseAddr(port);
    uint32 drValue = REG_READ32(gpioBase + DIO_GPIO_DR);

    if ((drValue & (1U << pin)) != 0U) {
        drValue &= ~(1U << pin);
        newLevel = STD_LOW;
    } else {
        drValue |= (1U << pin);
        newLevel = STD_HIGH;
    }
    REG_WRITE32(gpioBase + DIO_GPIO_DR, drValue);
    return newLevel;
}
#endif

#if (DIO_MASKED_WRITE_PORT_API == STD_ON)
void Dio_MaskedWritePort(Dio_PortType PortId, Dio_PortLevelType Level, Dio_PortLevelType Mask)
{
    #if (DIO_DEV_ERROR_DETECT == STD_ON)
    if (Dio_DriverInitialized == FALSE) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_MASKED_WRITE_PORT, DIO_E_UNINIT);
        return;
    }
    if (PortId >= DIO_NUM_PORTS) {
        Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_MASKED_WRITE_PORT, DIO_E_PARAM_INVALID_PORT_ID);
        return;
    }
    #endif

    uint32 gpioBase = Dio_GetGpioBaseAddr((uint8)PortId);
    uint32 drValue = REG_READ32(gpioBase + DIO_GPIO_DR);
    drValue &= ~((uint32)Mask);
    drValue |= ((uint32)Level & (uint32)Mask);
    REG_WRITE32(gpioBase + DIO_GPIO_DR, drValue);
}
#endif

#define DIO_STOP_SEC_CODE
#include "MemMap.h"
