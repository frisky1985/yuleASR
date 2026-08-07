/** @file IoHwAb.c
 *  @brief I/O Hardware Abstraction implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_IOHardwareAbstraction.pdf
 */

#include "IoHwAb.h"
#include "IoHwAb_Cfg.h"
#include "Det.h"

/* Version check */
#if defined(IOHWAB_AR_RELEASE_MAJOR_VERSION) && (IOHWAB_AR_RELEASE_MAJOR_VERSION != 4u)
#error "IoHwAb: AR major mismatch"
#endif
#if defined(IOHWAB_AR_RELEASE_MINOR_VERSION) && (IOHWAB_AR_RELEASE_MINOR_VERSION != 4u)
#error "IoHwAb: AR minor mismatch"
#endif

#define IOHWAB_MAX_CHANNELS             64U

typedef enum {
    IOHWAB_INTERNAL_UNINIT = 0,
    IOHWAB_INTERNAL_INIT,
    IOHWAB_INTERNAL_READY
} IoHwAb_InternalStateType;

typedef struct {
    uint16                     id;
    uint8                      value;
    IoHwAb_ChannelTypeEnum     type;
    uint16                     dioChannel;
    uint8                      adcChannel;
    uint8                      pwmChannel;
    boolean                    inverted;
} IoHwAb_ChannelEntryType;

typedef struct {
    IoHwAb_InternalStateType   state;
    IoHwAb_ChannelEntryType    channels[IOHWAB_MAX_CHANNELS];
    uint8                      channelCount;
    const IoHwAb_ConfigType*   configPtr;
} IoHwAb_InternalType;

static IoHwAb_InternalType IoHwAb_State;

static IoHwAb_ChannelEntryType* IoHwAb_FindChannel(uint16 ChannelId)
{
    for (uint8 i = 0U; i < IoHwAb_State.channelCount; i++) {
        if (IoHwAb_State.channels[i].id == ChannelId) {
            return &IoHwAb_State.channels[i];
        }
    }
    return NULL_PTR;
}

void IoHwAb_Init(const IoHwAb_ConfigType* ConfigPtr)
{
#if (IOHWAB_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(IOHWAB_MODULE_ID, 0U, IOHWAB_SID_INIT, IOHWAB_E_PARAM_POINTER);
        return;
    }
#endif
    IoHwAb_State.state = IOHWAB_INTERNAL_UNINIT;
    IoHwAb_State.channelCount = 0U;
    IoHwAb_State.configPtr = ConfigPtr;

    if (ConfigPtr->NumChannels > IOHWAB_MAX_CHANNELS) return;

    for (uint8 i = 0U; i < ConfigPtr->NumChannels; i++) {
        const IoHwAb_ChannelConfigType* cfg = &ConfigPtr->Channels[i];
        IoHwAb_State.channels[i].id = cfg->ChannelId;
        IoHwAb_State.channels[i].type = cfg->Type;
        IoHwAb_State.channels[i].dioChannel = cfg->DioChannelId;
        IoHwAb_State.channels[i].adcChannel = cfg->AdcChannelId;
        IoHwAb_State.channels[i].pwmChannel = cfg->PwmChannelId;
        IoHwAb_State.channels[i].inverted = cfg->Inverted;
        IoHwAb_State.channels[i].value = 0U;
    }
    IoHwAb_State.channelCount = ConfigPtr->NumChannels;
    IoHwAb_State.state = IOHWAB_INTERNAL_INIT;
}

void IoHwAb_DeInit(void)
{
    IoHwAb_State.state = IOHWAB_INTERNAL_UNINIT;
    IoHwAb_State.channelCount = 0U;
}

IoHwAb_ReturnType IoHwAb_AnalogRead(IoHwAb_ChannelType Channel, uint16* Value)
{
    if (IoHwAb_State.state < IOHWAB_INTERNAL_INIT) return IOHWAB_NOT_OK;
    if (NULL_PTR == Value) return IOHWAB_NOT_OK;

    IoHwAb_ChannelEntryType* ch = IoHwAb_FindChannel(Channel);
    if ((ch == NULL_PTR) || (ch->type != IOHWAB_CHANNEL_ANALOG)) return IOHWAB_NOT_OK;

    *Value = ch->value;
    return IOHWAB_OK;
}

IoHwAb_ReturnType IoHwAb_DigitalRead(IoHwAb_ChannelType Channel, uint8* Value)
{
    if (IoHwAb_State.state < IOHWAB_INTERNAL_INIT) return IOHWAB_NOT_OK;
    if (NULL_PTR == Value) return IOHWAB_NOT_OK;

    IoHwAb_ChannelEntryType* ch = IoHwAb_FindChannel(Channel);
    if ((ch == NULL_PTR) || (ch->type != IOHWAB_CHANNEL_DIGITAL)) return IOHWAB_NOT_OK;

    *Value = ch->inverted ? (uint8)(1U - ch->value) : ch->value;
    return IOHWAB_OK;
}

IoHwAb_ReturnType IoHwAb_DigitalWrite(IoHwAb_ChannelType Channel, uint8 Value)
{
    if (IoHwAb_State.state < IOHWAB_INTERNAL_INIT) return IOHWAB_NOT_OK;

    IoHwAb_ChannelEntryType* ch = IoHwAb_FindChannel(Channel);
    if ((ch == NULL_PTR) || (ch->type != IOHWAB_CHANNEL_DIGITAL)) return IOHWAB_NOT_OK;

    ch->value = ch->inverted ? (uint8)(1U - Value) : Value;
    return IOHWAB_OK;
}

void IoHwAb_MainFunction(void)
{
    if (IoHwAb_State.state < IOHWAB_INTERNAL_INIT) return;
}

void IoHwAb_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (NULL_PTR == versioninfo) return;
    versioninfo->vendorID = IOHWAB_VENDOR_ID;
    versioninfo->moduleID = IOHWAB_MODULE_ID;
    versioninfo->sw_major_version = 1U;
    versioninfo->sw_minor_version = 0U;
    versioninfo->sw_patch_version = 0U;
}