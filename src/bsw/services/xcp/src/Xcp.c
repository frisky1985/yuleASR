/**
 * @file Xcp.c
 * @brief XCP (Universal Measurement and Calibration Protocol) implementation
 *        following ASAM XCP 1.1 standard
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: XCP Protocol (ASAM XCP 1.1)
 * Layer: Service Layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Xcp.h"
#include "Det.h"
#include <string.h>

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define XCP_UNUSED(x)                       ((void)(x))
#define XCP_IS_INITIALIZED()                (Xcp_Initialized == TRUE)
#define XCP_IS_CHANNEL_VALID(ch)            ((ch) < Xcp_Config.NumChannels)

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
typedef struct {
    uint8 SequenceNumber;
    boolean Connected;
    Xcp_ConnectionStateType State;
    Xcp_SessionStatusType SessionStatus;
    Xcp_MtaType Mta;
    uint8 ResourceProtection;
    boolean ResourcesLocked[XCP_MAX_SEEDS];
    uint8 CurrentSeed[XCP_MAX_SEEDS][4];
    uint8 MaxCto;
    uint16 MaxDto;
    Xcp_CommModeType CommMode;
} Xcp_ChannelStateType;

typedef struct {
    uint8 DaqListNumber;
    uint8 OdtNumber;
    uint8 OdtEntryNumber;
} Xcp_DaqPtrType;

typedef struct {
    uint32 Address;
    uint8 Ext;
    uint8 AccessFlags;
    uint8 SectorType;
} Xcp_MemoryRangeType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define XCP_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

static boolean Xcp_Initialized = FALSE;

#define XCP_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

#define XCP_START_SEC_VAR_NOINIT_UNSPECIFIED
#include "MemMap.h"

static Xcp_ChannelStateType Xcp_ChannelState[XCP_NUMBER_OF_CHANNELS];
static Xcp_DaqPtrType Xcp_DaqPtr;
static Xcp_DaqListType Xcp_DaqLists[XCP_MAX_DAQ_LISTS];
static Xcp_OdtType Xcp_Odts[XCP_MAX_DAQ_LISTS][XCP_MAX_ODTS_PER_DAQ];
static Xcp_OdtEntryType Xcp_OdtEntries[XCP_MAX_DAQ_LISTS][XCP_MAX_ODTS_PER_DAQ][XCP_MAX_ODT_ENTRIES_PER_ODT];
static uint8 Xcp_TxBuffer[XCP_NUMBER_OF_CHANNELS][XCP_MAX_CTO_SIZE];
static uint8 Xcp_DaqBuffer[XCP_MAX_DAQ_LISTS][XCP_MAX_DTO_SIZE];
static uint8 Xcp_StimQueue[XCP_MAX_DAQ_LISTS][XCP_STIM_QUEUE_SIZE][XCP_MAX_DTO_SIZE];
static uint8 Xcp_StimQueueHead[XCP_MAX_DAQ_LISTS];
static uint8 Xcp_StimQueueTail[XCP_MAX_DAQ_LISTS];
static uint32 Xcp_EventCounter[XCP_MAX_EVENT_CHANNELS];
static Xcp_PgmStateType Xcp_PgmState;

#define XCP_STOP_SEC_VAR_NOINIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    CONSTANTS
==================================================================================================*/
#define XCP_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/* XCP Protocol Layer Version */
static const uint8 Xcp_ProtocolVersion[] = { 0x01U, 0x01U }; /* XCP 1.1 */

/* Memory ranges for access validation */
static const Xcp_MemoryRangeType Xcp_MemoryRanges[XCP_NUMBER_OF_MEMORY_RANGES] = {
    /* RAM range */
    { 0x20000000U, 0x00U, (XCP_MEMORY_ACCESS_READ | XCP_MEMORY_ACCESS_WRITE), XCP_MEMORY_SECTOR_TYPE_RAM },
    /* Flash range */
    { 0x08000000U, 0x00U, (XCP_MEMORY_ACCESS_READ | XCP_MEMORY_ACCESS_WRITE | XCP_MEMORY_ACCESS_ERASE), XCP_MEMORY_SECTOR_TYPE_FLASH },
    /* EEPROM range */
    { 0x08080000U, 0x00U, (XCP_MEMORY_ACCESS_READ | XCP_MEMORY_ACCESS_WRITE | XCP_MEMORY_ACCESS_ERASE), XCP_MEMORY_SECTOR_TYPE_EEPROM },
    /* Calibration RAM */
    { 0x20010000U, 0x00U, (XCP_MEMORY_ACCESS_READ | XCP_MEMORY_ACCESS_WRITE), XCP_MEMORY_SECTOR_TYPE_RAM }
};

#define XCP_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
void Xcp_Init(const Xcp_ConfigType* ConfigPtr)
{
    uint8 ch;
    uint16 daq;
    uint8 odt;
    uint8 entry;

#if (XCP_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_INIT, XCP_E_PARAM_POINTER);
        return;
    }
#endif

    /* Initialize channel states */
    for (ch = 0U; ch < XCP_NUMBER_OF_CHANNELS; ch++) {
        Xcp_ChannelState[ch].SequenceNumber = 0U;
        Xcp_ChannelState[ch].Connected = FALSE;
        Xcp_ChannelState[ch].State = XCP_STATE_DISCONNECTED;
        Xcp_ChannelState[ch].SessionStatus = 0U;
        Xcp_ChannelState[ch].Mta.Address = 0U;
        Xcp_ChannelState[ch].Mta.Extension = 0U;
        Xcp_ChannelState[ch].ResourceProtection = 0U;
        Xcp_ChannelState[ch].MaxCto = XCP_MAX_CTO_SIZE;
        Xcp_ChannelState[ch].MaxDto = XCP_MAX_DTO_SIZE;
        Xcp_ChannelState[ch].CommMode = XCP_COMM_MODE_BASIC;
        
        for (daq = 0U; daq < XCP_MAX_SEEDS; daq++) {
            Xcp_ChannelState[ch].ResourcesLocked[daq] = FALSE;
        }
    }

    /* Initialize DAQ lists */
    for (daq = 0U; daq < XCP_MAX_DAQ_LISTS; daq++) {
        Xcp_DaqLists[daq].ListNumber = daq;
        Xcp_DaqLists[daq].OdtList = Xcp_Odts[daq];
        Xcp_DaqLists[daq].NumOdts = 0U;
        Xcp_DaqLists[daq].Mode = 0U;
        Xcp_DaqLists[daq].Prescaler = 1U;
        Xcp_DaqLists[daq].EventChannel = 0U;
        Xcp_DaqLists[daq].Priority = 0U;
        Xcp_DaqLists[daq].State = XCP_DAQ_STATE_STOPPED;
        Xcp_DaqLists[daq].IsAllocated = FALSE;
        Xcp_DaqLists[daq].CurrentTimestamp = 0U;
        Xcp_DaqLists[daq].CurrentOdt = 0U;

        for (odt = 0U; odt < XCP_MAX_ODTS_PER_DAQ; odt++) {
            Xcp_Odts[daq][odt].Entries = Xcp_OdtEntries[daq][odt];
            Xcp_Odts[daq][odt].NumEntries = 0U;
            Xcp_Odts[daq][odt].NextOdt = 0U;
            Xcp_Odts[daq][odt].FirstPid = 0U;

            for (entry = 0U; entry < XCP_MAX_ODT_ENTRIES_PER_ODT; entry++) {
                Xcp_OdtEntries[daq][odt][entry].BitOffset = 0U;
                Xcp_OdtEntries[daq][odt][entry].EleLength = 0U;
                Xcp_OdtEntries[daq][odt][entry].AddrExt = 0U;
                Xcp_OdtEntries[daq][odt][entry].Addr = 0U;
                Xcp_OdtEntries[daq][odt][entry].IsValid = FALSE;
            }
        }
    }

    /* Initialize DAQ pointer */
    Xcp_DaqPtr.DaqListNumber = 0U;
    Xcp_DaqPtr.OdtNumber = 0U;
    Xcp_DaqPtr.OdtEntryNumber = 0U;

    /* Initialize event counters */
    for (ch = 0U; ch < XCP_MAX_EVENT_CHANNELS; ch++) {
        Xcp_EventCounter[ch] = 0U;
    }

    /* Initialize STIM queues */
    for (daq = 0U; daq < XCP_MAX_DAQ_LISTS; daq++) {
        Xcp_StimQueueHead[daq] = 0U;
        Xcp_StimQueueTail[daq] = 0U;
    }

    /* Initialize PGM state */
    Xcp_PgmState = XCP_PGM_STATE_IDLE;

    Xcp_Initialized = TRUE;
}

/**
 * @brief Deinitializes the XCP module
 */
void Xcp_DeInit(void)
{
    uint8 ch;

    if (Xcp_Initialized == 0U) {
        return;
    }

    /* Stop all DAQ lists */
    for (ch = 0U; ch < XCP_MAX_DAQ_LISTS; ch++) {
        Xcp_DaqLists[ch].State = XCP_DAQ_STATE_STOPPED;
    }

    /* Reset PGM state */
    Xcp_PgmState = XCP_PGM_STATE_IDLE;

    /* Reset initialization flag */
    Xcp_Initialized = FALSE;
}

#if (XCP_VERSION_INFO_API == STD_ON)
/**
 * @brief Gets version information
 */
void Xcp_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (XCP_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_GETVERSIONINFO, XCP_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = XCP_VENDOR_ID;
    versioninfo->moduleID = XCP_MODULE_ID;
    versioninfo->sw_major_version = XCP_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = XCP_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = XCP_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Main function for cyclic processing
 */
void Xcp_MainFunction(void)
{
    uint8 ch;

    if (Xcp_Initialized == 0U) {
        return;
    }

    /* Process DAQ lists */
    Xcp_DaqProcessor();

    /* Update event counters */
    for (ch = 0U; ch < XCP_MAX_EVENT_CHANNELS; ch++) {
        Xcp_EventCounter[ch]++;
    }
}

/**
 * @brief Rx Indication callback from lower layer
 */
void Xcp_RxIndication(uint8 XcpChannelId, PduIdType XcpPduId, const PduInfoType* XcpRxPduPtr)
{
#if (XCP_DEV_ERROR_DETECT == STD_ON)
    if (Xcp_Initialized == 0U) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_RXINDICATION, XCP_E_NOT_INITIALIZED);
        return;
    }
    if (XcpRxPduPtr == NULL_PTR || XcpRxPduPtr->SduDataPtr == NULL_PTR) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_RXINDICATION, XCP_E_PARAM_POINTER);
        return;
    }
    if (!XCP_IS_CHANNEL_VALID(XcpChannelId)) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_RXINDICATION, XCP_E_PARAM_CHANNEL);
        return;
    }
#endif

    XCP_UNUSED(XcpPduId);

    /* Process received XCP command */
    if (XcpRxPduPtr->SduLength > 0U) {
        Xcp_ProcessCommand(XcpChannelId, XcpRxPduPtr->SduDataPtr, (uint8)XcpRxPduPtr->SduLength);
    }
}

/**
 * @brief Tx Confirmation callback from lower layer
 */
void Xcp_TxConfirmation(uint8 XcpChannelId, PduIdType XcpTxPduId)
{
#if (XCP_DEV_ERROR_DETECT == STD_ON)
    if (Xcp_Initialized == 0U) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_TXCONFIRMATION, XCP_E_NOT_INITIALIZED);
        return;
    }
    if (!XCP_IS_CHANNEL_VALID(XcpChannelId)) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_TXCONFIRMATION, XCP_E_PARAM_CHANNEL);
        return;
    }
#endif

    XCP_UNUSED(XcpTxPduId);

    /* Transmission completed - can be used for flow control */
}

/**
 * @brief Trigger transmit callback from lower layer
 */
Std_ReturnType Xcp_TriggerTransmit(uint8 XcpChannelId, PduIdType XcpTxPduId, PduInfoType* PduInfoPtr)
{
#if (XCP_DEV_ERROR_DETECT == STD_ON)
    if (Xcp_Initialized == 0U) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_TRIGGERTRANSMIT, XCP_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (PduInfoPtr == NULL_PTR) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_TRIGGERTRANSMIT, XCP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (!XCP_IS_CHANNEL_VALID(XcpChannelId)) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_TRIGGERTRANSMIT, XCP_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }
#endif

    XCP_UNUSED(XcpTxPduId);

    /* Prepare DTO data if available */
    /* This is typically used for DAQ data transmission */

    return E_OK;
}

/**
 * @brief Sets transmission mode
 */
void Xcp_SetTransmissionMode(uint8 XcpChannelId, uint8 Mode)
{
#if (XCP_DEV_ERROR_DETECT == STD_ON)
    if (Xcp_Initialized == 0U) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_SETTRANSMISSIONMODE, XCP_E_NOT_INITIALIZED);
        return;
    }
    if (!XCP_IS_CHANNEL_VALID(XcpChannelId)) {
        Det_ReportError(XCP_MODULE_ID, XCP_INSTANCE_ID, XCP_SID_SETTRANSMISSIONMODE, XCP_E_PARAM_CHANNEL);
        return;
    }
#endif

    XCP_UNUSED(Mode);
    /* Implementation depends on transport layer */
}

/**
 * @brief Gets current session status
 */
Xcp_SessionStatusType Xcp_GetSessionStatus(void)
{
    Xcp_SessionStatusType status = 0U;
    uint8 ch;

    if (Xcp_Initialized == 0U) {
        return 0U;
    }

    for (ch = 0U; ch < XCP_NUMBER_OF_CHANNELS; ch++) {
        status |= Xcp_ChannelState[ch].SessionStatus;
    }

    return status;
}

/**
 * @brief Processes received XCP command
 */

/* Forward declarations for functions defined in sub-files (called by main file) */
void Xcp_ProcessStandardCommand(uint8 ChannelId, const uint8* Data, uint8 Length);
void Xcp_ProcessDaqCommand(uint8 ChannelId, const uint8* Data, uint8 Length);
void Xcp_ProcessPgmCommand(uint8 ChannelId, const uint8* Data, uint8 Length);
void Xcp_SendResponse(uint8 ChannelId, const uint8* Data, uint8 Length);
void Xcp_SendError(uint8 ChannelId, uint8 ErrorCode, uint8 ErrorInfo);
void Xcp_SendEvent(uint8 ChannelId, uint8 EventCode, const uint8* Data, uint8 Length);
void Xcp_CmdConnect(uint8 ChannelId, const uint8* Data, uint8 Length);
void Xcp_CmdDisconnect(uint8 ChannelId);
void Xcp_CmdGetStatus(uint8 ChannelId);
void Xcp_CmdGetCommModeInfo(uint8 ChannelId);
void Xcp_CmdGetId(uint8 ChannelId, const uint8* Data, uint8 Length);

void Xcp_ProcessCommand(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 cmd;

    if ((Data == NULL_PTR) || (Length == 0U)) {
        return;
    }

    cmd = Data[0];

    /* Check if connected or if this is a CONNECT command */
    if ((!Xcp_ChannelState[ChannelId].Connected) && (cmd != XCP_CMD_CONNECT)) {
        /* Not connected and not a CONNECT command - ignore */
        return;
    }

    /* Route command to appropriate processor */
    if ((cmd >= 0xC0U) || (cmd == XCP_CMD_CONNECT) || (cmd == XCP_CMD_DISCONNECT) ||
        (cmd == XCP_CMD_GET_STATUS) || (cmd == XCP_CMD_SYNCH) ||
        (cmd == XCP_CMD_GET_COMM_MODE_INFO) || (cmd == XCP_CMD_GET_ID) ||
        (cmd == XCP_CMD_SET_REQUEST) || (cmd == XCP_CMD_GET_SEED) ||
        (cmd == XCP_CMD_UNLOCK) || (cmd == XCP_CMD_SET_MTA) ||
        (cmd == XCP_CMD_UPLOAD) || (cmd == XCP_CMD_SHORT_UPLOAD) ||
        (cmd == XCP_CMD_BUILD_CHECKSUM) || (cmd == XCP_CMD_TRANSPORT_LAYER_CMD) ||
        (cmd == XCP_CMD_USER_CMD)) {
        Xcp_ProcessStandardCommand(ChannelId, Data, Length);
    }
    else if ((cmd >= 0xC0U) && (cmd <= 0xE3U)) {
        Xcp_ProcessDaqCommand(ChannelId, Data, Length);
    }
    else if ((cmd >= 0xCCU) && (cmd <= 0xD2U)) {
        Xcp_ProcessPgmCommand(ChannelId, Data, Length);
    }
    else {
        /* Unknown command */
        Xcp_SendError(ChannelId, XCP_ERR_CMD_UNKNOWN, 0U);
    }
}

/**
 * @brief Sends response packet
 */
void Xcp_SendResponse(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    PduInfoType pduInfo;

    if (!Xcp_Initialized || !XCP_IS_CHANNEL_VALID(ChannelId)) {
        return;
    }

    /* Prepare response */
    Xcp_TxBuffer[ChannelId][0] = XCP_PID_RES;
    if ((Data != NULL_PTR) && (Length > 0U)) {
        memcpy(&Xcp_TxBuffer[ChannelId][1], Data, Length);
    }

/*     pduInfo.SduDataPtr = Xcp_TxBuffer[ChannelId]; */
/*     pduInfo.SduLength = (Length > 0U) ? (Length + 1U) : 1U; */

    /* Send via transport layer - would call CanIf_Transmit, SoAd_Transmit, etc. */
    /* This is simplified - actual implementation depends on transport layer */
}

/**
 * @brief Sends error packet
 */
void Xcp_SendError(uint8 ChannelId, uint8 ErrorCode, uint8 ErrorInfo)
{
    uint8 errorData[2];

    if (!Xcp_Initialized || !XCP_IS_CHANNEL_VALID(ChannelId)) {
        return;
    }

    errorData[0] = ErrorCode;
    errorData[1] = ErrorInfo;

    Xcp_TxBuffer[ChannelId][0] = XCP_PID_ERR;
    memcpy(&Xcp_TxBuffer[ChannelId][1], errorData, 2U);

    /* Send error response */
    PduInfoType pduInfo;
/*     pduInfo.SduDataPtr = Xcp_TxBuffer[ChannelId]; */
/*     pduInfo.SduLength = 3U; */
}

/**
 * @brief Sends event packet
 */
void Xcp_SendEvent(uint8 ChannelId, uint8 EventCode, const uint8* Data, uint8 Length)
{
    if (!Xcp_Initialized || !XCP_IS_CHANNEL_VALID(ChannelId)) {
        return;
    }

    Xcp_TxBuffer[ChannelId][0] = XCP_PID_EV;
    Xcp_TxBuffer[ChannelId][1] = EventCode;
    if ((Data != NULL_PTR) && (Length > 0U)) {
        memcpy(&Xcp_TxBuffer[ChannelId][2], Data, Length);
    }

    PduInfoType pduInfo;
/*     pduInfo.SduDataPtr = Xcp_TxBuffer[ChannelId]; */
/*     pduInfo.SduLength = Length + 2U; */
}

/*==================================================================================================
*                                    COMMAND HANDLERS
==================================================================================================*/

/**
 * @brief Handles Connect command
 */

/*==================================================================================================
*                                    CONST CONFIGURATION
==================================================================================================*/
#define XCP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

const Xcp_ConfigType Xcp_Config = {
    NULL_PTR,  /* ChannelConfigs - would be actual configuration */
    XCP_NUMBER_OF_CHANNELS,
    NULL_PTR,  /* SessionConfig - would be actual configuration */
    Xcp_DaqLists,
    XCP_MAX_DAQ_LISTS,
    XCP_DEV_ERROR_DETECT,
    XCP_VERSION_INFO_API,
    XCP_BLOCK_DOWNLOAD_SUPPORTED,
    XCP_INTERLEAVED_MODE_SUPPORTED,
    10U  /* MainFunctionPeriod in ms */
};

#define XCP_STOP_SEC_CONFIG_DATA_UNSPECIFIED

/*==================================================================================================
 *  子文件包含 (批量拆分)
 *================================================================================================*/
#include "xcp_transport.c"
#include "xcp_cmd_daq_pgm.c"
#include "xcp_cmd_std.c"
