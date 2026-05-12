/**
 * @file Lin.c
 * @brief LIN Driver Implementation
 */

#include "Lin.h"
#include "Lin_Cfg.h"
#include "Det.h"

/* Internal State */
static Lin_StatusType Lin_ChannelStatus[LIN_MAX_CHANNELS];
static boolean Lin_ChannelInitialized[LIN_MAX_CHANNELS];
static boolean Lin_ModuleInitialized = FALSE;
static const Lin_ConfigType* Lin_ConfigPtr = NULL_PTR;

/* Internal Buffers */
static uint8 Lin_TxBuffer[LIN_MAX_CHANNELS][LIN_MAX_FRAME_LENGTH];
static uint8 Lin_RxBuffer[LIN_MAX_CHANNELS][LIN_MAX_FRAME_LENGTH];

/* Version Info */
#define LIN_VENDOR_ID                      0x0001
#define LIN_INSTANCE_ID                    0x00

void Lin_Init(const Lin_ConfigType* Config)
{
#if (LIN_DEV_ERROR_DETECT == STD_ON)
    if (Config == NULL_PTR)
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_INIT_SID, LIN_E_INVALID_POINTER);
        return;
    }
#endif

    Lin_ConfigPtr = Config;
    
    /* Initialize all channels */
    for (uint8 i = 0; i < LIN_MAX_CHANNELS; i++)
    {
        if (i < Config->NumChannels)
        {
            Lin_ChannelStatus[i] = LIN_CH_SLEEP;
            Lin_ChannelInitialized[i] = TRUE;
        }
        else
        {
            Lin_ChannelInitialized[i] = FALSE;
        }
    }
    
    Lin_ModuleInitialized = TRUE;
    
    /* TODO: Initialize hardware */
    /* TODO: Configure baud rate */
    /* TODO: Set up interrupts */
}

void Lin_DeInit(void)
{
#if (LIN_DEV_ERROR_DETECT == STD_ON)
    if (!Lin_ModuleInitialized)
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_DEINIT_SID, LIN_E_UNINIT);
        return;
    }
#endif

    /* Deinitialize all channels */
    for (uint8 i = 0; i < LIN_MAX_CHANNELS; i++)
    {
        Lin_ChannelInitialized[i] = FALSE;
        Lin_ChannelStatus[i] = LIN_NOT_OK;
    }
    
    Lin_ConfigPtr = NULL_PTR;
    Lin_ModuleInitialized = FALSE;
}

void Lin_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (LIN_VERSION_INFO_API == STD_ON)
    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID = LIN_VENDOR_ID;
        versioninfo->moduleID = LIN_MODULE_ID;
        versioninfo->sw_major_version = LIN_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = LIN_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = LIN_SW_PATCH_VERSION;
    }
#endif
}

Std_ReturnType Lin_SendFrame(Lin_ChannelType Channel, const Lin_PduType* PduInfoPtr)
{
#if (LIN_DEV_ERROR_DETECT == STD_ON)
    if (!Lin_ModuleInitialized)
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_SENDFRAME_SID, LIN_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= LIN_MAX_CHANNELS)
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_SENDFRAME_SID, LIN_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    if (PduInfoPtr == NULL_PTR)
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_SENDFRAME_SID, LIN_E_INVALID_POINTER);
        return E_NOT_OK;
    }
    if (!Lin_ChannelInitialized[Channel])
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_SENDFRAME_SID, LIN_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif

    /* Check if channel is busy */
    if (Lin_ChannelStatus[Channel] == LIN_TX_BUSY || Lin_ChannelStatus[Channel] == LIN_RX_BUSY)
    {
        return E_NOT_OK;
    }

    /* Copy data to TX buffer */
    for (uint8 i = 0; i < PduInfoPtr->Length && i < LIN_MAX_FRAME_LENGTH; i++)
    {
        Lin_TxBuffer[Channel][i] = PduInfoPtr->SduPtr[i];
    }

    /* Set status to busy */
    Lin_ChannelStatus[Channel] = LIN_TX_BUSY;

    /* TODO: Start hardware transmission */
    /* TODO: Send header + data */

    return E_OK;
}

Std_ReturnType Lin_SendResponse(Lin_ChannelType Channel, const Lin_PduType* PduInfoPtr)
{
#if (LIN_DEV_ERROR_DETECT == STD_ON)
    if (!Lin_ModuleInitialized)
    {
        return E_NOT_OK;
    }
    if (Channel >= LIN_MAX_CHANNELS)
    {
        return E_NOT_OK;
    }
    if (PduInfoPtr == NULL_PTR)
    {
        return E_NOT_OK;
    }
#endif

    /* Copy response data to TX buffer */
    for (uint8 i = 0; i < PduInfoPtr->Length && i < LIN_MAX_FRAME_LENGTH; i++)
    {
        Lin_TxBuffer[Channel][i] = PduInfoPtr->SduPtr[i];
    }

    /* TODO: Send response */

    return E_OK;
}

Std_ReturnType Lin_DisableResponse(Lin_ChannelType Channel)
{
#if (LIN_DEV_ERROR_DETECT == STD_ON)
    if (!Lin_ModuleInitialized)
    {
        return E_NOT_OK;
    }
    if (Channel >= LIN_MAX_CHANNELS)
    {
        return E_NOT_OK;
    }
#endif

    /* TODO: Disable response transmission */

    return E_OK;
}

Std_ReturnType Lin_WakeUp(Lin_ChannelType Channel)
{
#if (LIN_DEV_ERROR_DETECT == STD_ON)
    if (!Lin_ModuleInitialized)
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_WAKEUP_SID, LIN_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= LIN_MAX_CHANNELS)
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_WAKEUP_SID, LIN_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif

    /* Send wake-up signal */
    /* TODO: Generate dominant level for 250us */

    Lin_ChannelStatus[Channel] = LIN_OPERATIONAL;

    return E_OK;
}

Std_ReturnType Lin_WakeUpInternal(Lin_ChannelType Channel)
{
#if (LIN_DEV_ERROR_DETECT == STD_ON)
    if (!Lin_ModuleInitialized)
    {
        return E_NOT_OK;
    }
    if (Channel >= LIN_MAX_CHANNELS)
    {
        return E_NOT_OK;
    }
#endif

    /* Internal wakeup without signal */
    Lin_ChannelStatus[Channel] = LIN_OPERATIONAL;

    return E_OK;
}

Std_ReturnType Lin_CheckWakeup(Lin_ChannelType Channel)
{
#if (LIN_DEV_ERROR_DETECT == STD_ON)
    if (!Lin_ModuleInitialized)
    {
        return E_NOT_OK;
    }
    if (Channel >= LIN_MAX_CHANNELS)
    {
        return E_NOT_OK;
    }
#endif

    /* Check for wake-up event */
    /* TODO: Check if wake-up signal detected */

    return E_OK;
}

Lin_StatusType Lin_GetStatus(Lin_ChannelType Channel, uint8** Lin_SduPtr)
{
#if (LIN_DEV_ERROR_DETECT == STD_ON)
    if (!Lin_ModuleInitialized)
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_GETSTATUS_SID, LIN_E_UNINIT);
        return LIN_NOT_OK;
    }
    if (Channel >= LIN_MAX_CHANNELS)
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_GETSTATUS_SID, LIN_E_INVALID_CHANNEL);
        return LIN_NOT_OK;
    }
#endif

    if (Lin_SduPtr != NULL_PTR)
    {
        *Lin_SduPtr = Lin_RxBuffer[Channel];
    }

    return Lin_ChannelStatus[Channel];
}

Std_ReturnType Lin_GoToSleep(Lin_ChannelType Channel)
{
#if (LIN_DEV_ERROR_DETECT == STD_ON)
    if (!Lin_ModuleInitialized)
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_GOTOSLEEP_SID, LIN_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= LIN_MAX_CHANNELS)
    {
        Det_ReportError(LIN_MODULE_ID, LIN_INSTANCE_ID, LIN_GOTOSLEEP_SID, LIN_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif

    /* Send go-to-sleep command */
    /* TODO: Send diagnostic frame with sleep command */

    Lin_ChannelStatus[Channel] = LIN_CH_SLEEP;

    return E_OK;
}

Std_ReturnType Lin_GoToSleepInternal(Lin_ChannelType Channel)
{
#if (LIN_DEV_ERROR_DETECT == STD_ON)
    if (!Lin_ModuleInitialized)
    {
        return E_NOT_OK;
    }
    if (Channel >= LIN_MAX_CHANNELS)
    {
        return E_NOT_OK;
    }
#endif

    /* Internal sleep transition */
    Lin_ChannelStatus[Channel] = LIN_CH_SLEEP;

    return E_OK;
}

/* Callback Implementations */
void Lin_WakeUpConfirmation(Lin_ChannelType Channel)
{
    if (Channel < LIN_MAX_CHANNELS)
    {
        Lin_ChannelStatus[Channel] = LIN_OPERATIONAL;
    }
}

void Lin_WakeUpFrameIndication(void)
{
    /* Wake-up frame received */
}

/* ISR Implementations */
void Lin_IsrTx(Lin_ChannelType Channel)
{
    if (Channel < LIN_MAX_CHANNELS)
    {
        Lin_ChannelStatus[Channel] = LIN_TX_OK;
    }
}

void Lin_IsrRx(Lin_ChannelType Channel)
{
    if (Channel < LIN_MAX_CHANNELS)
    {
        Lin_ChannelStatus[Channel] = LIN_RX_OK;
    }
}

void Lin_IsrErr(Lin_ChannelType Channel)
{
    if (Channel < LIN_MAX_CHANNELS)
    {
        Lin_ChannelStatus[Channel] = LIN_TX_ERROR;
    }
}
