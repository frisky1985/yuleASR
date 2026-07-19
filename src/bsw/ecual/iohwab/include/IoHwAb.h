/**
 * @file IoHwAb.h
 * @brief I/O Hardware Abstraction - AUTOSAR ECUAL Module
 * @version 2.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @implements AUTOSAR_SWS_IOHardwareAbstraction.pdf
 */

#ifndef IOHWAB_H
#define IOHWAB_H

#include "Std_Types.h"
#include "IoHwAb_Cfg.h"

#define IOHWAB_AR_RELEASE_MAJOR_VERSION   4U
#define IOHWAB_AR_RELEASE_MINOR_VERSION   4U
#define IOHWAB_AR_RELEASE_REVISION_VERSION 0U
#define IOHWAB_SW_MAJOR_VERSION           1U
#define IOHWAB_SW_MINOR_VERSION           0U
#define IOHWAB_SW_PATCH_VERSION           0U
#define IOHWAB_MODULE_ID                0x7AU
#define IOHWAB_VENDOR_ID                0x0055U

#define IOHWAB_SID_INIT                 0x01U
#define IOHWAB_SID_DEINIT               0x02U
#define IOHWAB_SID_ANALOGREAD           0x03U
#define IOHWAB_SID_DIGITALREAD          0x04U
#define IOHWAB_SID_DIGITALWRITE         0x05U
#define IOHWAB_SID_MAINFUNCTION         0x0BU
#define IOHWAB_SID_GETVERSIONINFO       0x0AU

#define IOHWAB_E_PARAM_POINTER          0x01U
#define IOHWAB_E_PARAM_CHANNEL          0x02U
#define IOHWAB_E_PARAM_VALUE            0x03U
#define IOHWAB_E_UNINIT                 0x04U

typedef enum {
    IOHWAB_OK = 0,
    IOHWAB_NOT_OK,
    IOHWAB_BUSY,
    IOHWAB_TIMEOUT
} IoHwAb_ReturnType;

typedef uint16 IoHwAb_ChannelType;

typedef enum {
    IOHWAB_CHANNEL_DIGITAL = 0,
    IOHWAB_CHANNEL_ANALOG,
    IOHWAB_CHANNEL_PWM
} IoHwAb_ChannelTypeEnum;

typedef struct {
    IoHwAb_ChannelType    ChannelId;
    IoHwAb_ChannelTypeEnum Type;
    uint16                DioChannelId;
    uint8                 AdcChannelId;
    uint8                 PwmChannelId;
    boolean               Inverted;
    uint16                Resolution;
} IoHwAb_ChannelConfigType;

typedef struct {
    uint8 NumChannels;
    const IoHwAb_ChannelConfigType* Channels;
} IoHwAb_ConfigType;

void IoHwAb_Init(const IoHwAb_ConfigType* ConfigPtr);
void IoHwAb_DeInit(void);
IoHwAb_ReturnType IoHwAb_AnalogRead(IoHwAb_ChannelType Channel, uint16* Value);
IoHwAb_ReturnType IoHwAb_DigitalRead(IoHwAb_ChannelType Channel, uint8* Value);
IoHwAb_ReturnType IoHwAb_DigitalWrite(IoHwAb_ChannelType Channel, uint8 Value);
void IoHwAb_MainFunction(void);
void IoHwAb_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* IOHWAB_H */