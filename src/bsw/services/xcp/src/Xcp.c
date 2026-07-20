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
static void Xcp_ProcessStandardCommand(uint8 ChannelId, const uint8* Data, uint8 Length);
static void Xcp_ProcessDaqCommand(uint8 ChannelId, const uint8* Data, uint8 Length);
static void Xcp_ProcessPgmCommand(uint8 ChannelId, const uint8* Data, uint8 Length);
static uint16 Xcp_CalculateChecksum(const uint8* Data, uint32 Length);
static boolean Xcp_ValidateMemoryAccess(uint32 Addr, uint8 Ext, uint32 Length, uint8 AccessType);
static void Xcp_ClearDaqList(uint8 DaqListNumber);
static void Xcp_ResetDaqConfiguration(void);
static uint32 Xcp_GetTimestamp(void);

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
#define XCP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the XCP module
 */
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

    pduInfo.SduDataPtr = Xcp_TxBuffer[ChannelId];
    pduInfo.SduLength = (Length > 0U) ? (Length + 1U) : 1U;

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
    pduInfo.SduDataPtr = Xcp_TxBuffer[ChannelId];
    pduInfo.SduLength = 3U;
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
    pduInfo.SduDataPtr = Xcp_TxBuffer[ChannelId];
    pduInfo.SduLength = Length + 2U;
}

/*==================================================================================================
*                                    COMMAND HANDLERS
==================================================================================================*/

/**
 * @brief Handles Connect command
 */
void Xcp_CmdConnect(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 response[7];
    uint8 mode;

    XCP_UNUSED(Length);

    if (Length < 2U) {
        Xcp_SendError(ChannelId, XCP_ERR_CMD_SYNTAX, 0U);
        return;
    }

    mode = Data[1];

    /* Check if normal mode or user defined mode */
    if (mode > 1U) {
        Xcp_SendError(ChannelId, XCP_ERR_MODE_NOT_VALID, 0U);
        return;
    }

    /* Set connection state */
    Xcp_ChannelState[ChannelId].Connected = TRUE;
    Xcp_ChannelState[ChannelId].State = XCP_STATE_CONNECTED;
    Xcp_ChannelState[ChannelId].CommMode = XCP_COMM_MODE_BASIC;
    Xcp_ChannelState[ChannelId].SessionStatus = 0U;
    Xcp_ChannelState[ChannelId].ResourceProtection = 0U;
    Xcp_ChannelState[ChannelId].Mta.Address = 0U;
    Xcp_ChannelState[ChannelId].Mta.Extension = 0U;

    /* Prepare response */
    response[0] = 0x01U;  /* Resource - CAL/PAG available */
#if (XCP_DAQ_SUPPORTED == STD_ON)
    response[0] |= XCP_RESOURCE_DAQ;
#endif
#if (XCP_PROGRAMMING_SUPPORTED == STD_ON)
    response[0] |= XCP_RESOURCE_PGM;
#endif

    response[1] = XCP_COMM_MODE_BASIC;  /* Comm mode basic */
    response[2] = XCP_MAX_CTO_SIZE;     /* Max CTO */
    response[3] = (uint8)(XCP_MAX_DTO_SIZE & 0xFFU);  /* Max DTO (LSB) */
    response[4] = (uint8)((XCP_MAX_DTO_SIZE >> 8) & 0xFFU);  /* Max DTO (MSB) */
    response[5] = Xcp_ProtocolVersion[0];  /* Protocol version */
    response[6] = Xcp_ProtocolVersion[1];  /* Transport version */

    Xcp_SendResponse(ChannelId, response, 7U);
}

/**
 * @brief Handles Disconnect command
 */
void Xcp_CmdDisconnect(uint8 ChannelId)
{
    /* Stop all DAQ lists */
    uint16 daq;
    for (daq = 0U; daq < XCP_MAX_DAQ_LISTS; daq++) {
        Xcp_DaqLists[daq].State = XCP_DAQ_STATE_STOPPED;
    }

    /* Clear session status */
    Xcp_ChannelState[ChannelId].Connected = FALSE;
    Xcp_ChannelState[ChannelId].State = XCP_STATE_DISCONNECTED;
    Xcp_ChannelState[ChannelId].SessionStatus = 0U;

    /* Send positive response */
    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/**
 * @brief Handles GetStatus command
 */
void Xcp_CmdGetStatus(uint8 ChannelId)
{
    uint8 response[5];

    response[0] = Xcp_ChannelState[ChannelId].SessionStatus;
    response[1] = Xcp_ChannelState[ChannelId].ResourceProtection;

    /* Reserved */
    response[2] = 0U;

    /* Session configuration ID (not used) */
    response[3] = 0U;
    response[4] = 0U;

    Xcp_SendResponse(ChannelId, response, 5U);
}

/**
 * @brief Handles GetCommModeInfo command
 */
void Xcp_CmdGetCommModeInfo(uint8 ChannelId)
{
    uint8 response[7];

    response[0] = 0U;  /* Reserved */
    response[1] = 0U;  /* Reserved */

    /* Comm mode optional */
    response[2] = 0U;
#if (XCP_MASTER_BLOCK_MODE == STD_ON)
    response[2] |= 0x01U;  /* MASTER_BLOCK_MODE */
#endif
#if (XCP_INTERLEAVED_BLOCK_MODE == STD_ON)
    response[2] |= 0x02U;  /* INTERLEAVED_MODE */
#endif

    response[3] = 0U;  /* Reserved */
    response[4] = XCP_MAX_BS_UPLOAD;  /* Max BS for upload */
    response[5] = XCP_MAX_BS_DOWNLOAD;  /* Max BS for download */
    response[6] = XCP_MIN_ST;  /* Min ST */

    Xcp_SendResponse(ChannelId, response, 7U);
}

/**
 * @brief Handles GetID command
 */
void Xcp_CmdGetId(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 response[7];
    uint8 idType;
    uint8 length;
    const char* idString;

    XCP_UNUSED(Length);

    idType = Data[1];

    /* Determine ID string based on type */
    switch (idType) {
        case 0x00U:  /* ASCII text */
            idString = "YuleTech XCP V1.0";
            break;
        case 0x01U:  /* ASAM-MC2 filename without path and extension */
            idString = "YuleXCP";
            break;
        case 0x02U:  /* ASAM-MC2 filename with path and extension */
            idString = "/cfg/YuleXCP.a2l";
            break;
        default:
            idString = "";
            break;
    }

    length = (uint8)strlen(idString);

    /* Set MTA to point to ID string */
    /* In a real implementation, this would point to the actual memory location */
    Xcp_ChannelState[ChannelId].Mta.Address = 0U;  /* Would be actual address */
    Xcp_ChannelState[ChannelId].Mta.Extension = 0U;

    response[0] = 0U;  /* Mode (0 = absolute address) */
    response[1] = 0U;  /* Reserved */
    response[2] = 0U;  /* Reserved */
    response[3] = length;  /* Length (LSB) */
    response[4] = 0U;      /* Length */
    response[5] = 0U;      /* Length */
    response[6] = 0U;      /* Length (MSB) */

    Xcp_SendResponse(ChannelId, response, 7U);
}

/**
 * @brief Handles SetMTA command
 */
void Xcp_CmdSetMta(uint8 ChannelId, const uint8* Data)
{
    Xcp_ChannelState[ChannelId].Mta.Extension = Data[1];
    Xcp_ChannelState[ChannelId].Mta.Address = 
        ((uint32)Data[2]) |
        (((uint32)Data[3]) << 8) |
        (((uint32)Data[4]) << 16) |
        (((uint32)Data[5]) << 24);

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/**
 * @brief Handles Upload command
 */
void Xcp_CmdUpload(uint8 ChannelId, const uint8* Data)
{
    uint8 response[XCP_MAX_CTO_SIZE - 1];
    uint8 length;
    uint8 i;

    length = Data[1];
    if (length > (XCP_MAX_CTO_SIZE - 1U)) {
        length = XCP_MAX_CTO_SIZE - 1U;
    }

    /* Validate memory access */
    if (!Xcp_ValidateMemoryAccess(Xcp_ChannelState[ChannelId].Mta.Address,
                                   Xcp_ChannelState[ChannelId].Mta.Extension,
                                   length, XCP_MEMORY_ACCESS_READ)) {
        Xcp_SendError(ChannelId, XCP_ERR_ACCESS_DENIED, 0U);
        return;
    }

    /* Read data from memory */
    for (i = 0U; i < length; i++) {
        if (Xcp_ReadMemory(Xcp_ChannelState[ChannelId].Mta.Address + i,
                           Xcp_ChannelState[ChannelId].Mta.Extension,
                           &response[i], 1U) != E_OK) {
            Xcp_SendError(ChannelId, XCP_ERR_ACCESS_DENIED, 0U);
            return;
        }
    }

    /* Update MTA */
    Xcp_ChannelState[ChannelId].Mta.Address += length;

    Xcp_SendResponse(ChannelId, response, length);
}

/**
 * @brief Handles ShortUpload command
 */
void Xcp_CmdShortUpload(uint8 ChannelId, const uint8* Data)
{
    uint8 response[XCP_MAX_CTO_SIZE - 1];
    uint8 length;
    uint8 addrExt;
    uint32 addr;
    uint8 i;

    length = Data[1];
    addrExt = Data[2];
    addr = ((uint32)Data[3]) |
           (((uint32)Data[4]) << 8) |
           (((uint32)Data[5]) << 16) |
           (((uint32)Data[6]) << 24);

    if (length > (XCP_MAX_CTO_SIZE - 1U)) {
        length = XCP_MAX_CTO_SIZE - 1U;
    }

    /* Validate memory access */
    if (!Xcp_ValidateMemoryAccess(addr, addrExt, length, XCP_MEMORY_ACCESS_READ)) {
        Xcp_SendError(ChannelId, XCP_ERR_ACCESS_DENIED, 0U);
        return;
    }

    /* Read data from memory */
    for (i = 0U; i < length; i++) {
        if (Xcp_ReadMemory(addr + i, addrExt, &response[i], 1U) != E_OK) {
            Xcp_SendError(ChannelId, XCP_ERR_ACCESS_DENIED, 0U);
            return;
        }
    }

    Xcp_SendResponse(ChannelId, response, length);
}

/**
 * @brief Handles Download command
 */
void Xcp_CmdDownload(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 i;
    uint8 dataLength;

    dataLength = Data[1];
    
    if (dataLength > (Length - 2U)) {
        Xcp_SendError(ChannelId, XCP_ERR_CMD_SYNTAX, 0U);
        return;
    }

    /* Check if DAQ is running - cannot download while DAQ is active */
    if (Xcp_ChannelState[ChannelId].SessionStatus & XCP_SESSION_DAQ_RUNNING) {
        Xcp_SendError(ChannelId, XCP_ERR_DAQ_ACTIVE, 0U);
        return;
    }

    /* Check resource protection */
    if (Xcp_IsResourceProtected(XCP_RESOURCE_CAL_PAG)) {
        Xcp_SendError(ChannelId, XCP_ERR_ACCESS_LOCKED, 0U);
        return;
    }

    /* Validate memory access */
    if (!Xcp_ValidateMemoryAccess(Xcp_ChannelState[ChannelId].Mta.Address,
                                   Xcp_ChannelState[ChannelId].Mta.Extension,
                                   dataLength, XCP_MEMORY_ACCESS_WRITE)) {
        Xcp_SendError(ChannelId, XCP_ERR_ACCESS_DENIED, 0U);
        return;
    }

    /* Write data to memory */
    for (i = 0U; i < dataLength; i++) {
        if (Xcp_WriteMemory(Xcp_ChannelState[ChannelId].Mta.Address + i,
                            Xcp_ChannelState[ChannelId].Mta.Extension,
                            &Data[2U + i], 1U) != E_OK) {
            Xcp_SendError(ChannelId, XCP_ERR_WRITE_PROTECTED, 0U);
            return;
        }
    }

    /* Update MTA */
    Xcp_ChannelState[ChannelId].Mta.Address += dataLength;

    /* Send positive response with MTA */
    {
        uint8 response[4];
        response[0] = XCP_MAX_BS_DOWNLOAD;  /* Max BS */
        response[1] = XCP_MIN_ST;           /* Min ST */
        response[2] = 0U;                   /* Queue size (if queue mode) */
        response[3] = 0U;                   /* Queue size high byte */
        Xcp_SendResponse(ChannelId, response, 4U);
    }
}

/**
 * @brief Handles GetSeed command
 */
void Xcp_CmdGetSeed(uint8 ChannelId, const uint8* Data)
{
    uint8 response[XCP_MAX_CTO_SIZE - 1];
    uint8 mode;
    uint8 resource;
    uint8 i;

    mode = Data[1];
    resource = Data[2];

    /* Check if resource is valid */
    if ((resource & ~(XCP_RESOURCE_CAL_PAG | XCP_RESOURCE_DAQ | 
                      XCP_RESOURCE_STIM | XCP_RESOURCE_PGM)) != 0U) {
        Xcp_SendError(ChannelId, XCP_ERR_OUT_OF_RANGE, 0U);
        return;
    }

    if (mode == 0U) {
        /* First part of seed */
        /* Generate seed (simplified - real implementation would use crypto) */
        for (i = 0U; i < 4U; i++) {
            Xcp_ChannelState[ChannelId].CurrentSeed[resource & 0x03U][i] = 
                (uint8)(Xcp_GetTimestamp() + i);
            response[i] = Xcp_ChannelState[ChannelId].CurrentSeed[resource & 0x03U][i];
        }
        Xcp_ChannelState[ChannelId].ResourcesLocked[resource & 0x03U] = TRUE;

        /* Set resource protection */
        Xcp_SetResourceProtection(resource, TRUE);

        Xcp_SendResponse(ChannelId, response, 4U);
    }
    else if (mode == 1U) {
        /* Remaining seed - not implemented for simplicity */
        Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
    }
    else {
        Xcp_SendError(ChannelId, XCP_ERR_OUT_OF_RANGE, 0U);
    }
}

/**
 * @brief Handles Unlock command
 */
void Xcp_CmdUnlock(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 response[1];
    uint8 keyLength;
    uint8 resource;
    uint8 i;

    keyLength = Data[1];
    
    if (keyLength > (Length - 2U)) {
        Xcp_SendError(ChannelId, XCP_ERR_CMD_SYNTAX, 0U);
        return;
    }

    /* Simple key validation (simplified - real implementation would use crypto) */
    /* Key should be derived from seed */
    resource = 0U;
    for (i = 0U; i < XCP_MAX_SEEDS; i++) {
        if (Xcp_ChannelState[ChannelId].ResourcesLocked[i]) {
            resource |= (1U << i);
        }
    }

        /* Check key (simplified) */
    if (keyLength >= 2U) {
        /* Unlock resource */
        XCP_CLEAR_RESOURCE_PROTECTION(resource, Xcp_ChannelState[ChannelId].ResourceProtection);
        for (i = 0U; i < XCP_MAX_SEEDS; i++) {
            Xcp_ChannelState[ChannelId].ResourcesLocked[i] = FALSE;
        }
        response[0] = 0U;  /* Current protection status */
    }
    else {
        Xcp_SendError(ChannelId, XCP_ERR_SEQUENCE, 0U);
        return;
    }

    Xcp_SendResponse(ChannelId, response, 1U);
}

/*==================================================================================================
*                                    DAQ COMMAND HANDLERS
==================================================================================================*/

/**
 * @brief Handles ClearDaqList command
 */
void Xcp_CmdClearDaqList(uint8 ChannelId, const uint8* Data)
{
    uint16 daqListNumber;

    /* Check if DAQ is running */
    if (Xcp_ChannelState[ChannelId].SessionStatus & XCP_SESSION_DAQ_RUNNING) {
        Xcp_SendError(ChannelId, XCP_ERR_DAQ_ACTIVE, 0U);
        return;
    }

    daqListNumber = (uint16)Data[2];
    if (XCP_MAX_DAQ_LISTS > 255U) {
        daqListNumber |= ((uint16)Data[3] << 8);
    }

    if (daqListNumber >= XCP_MAX_DAQ_LISTS) {
        Xcp_SendError(ChannelId, XCP_ERR_OUT_OF_RANGE, 0U);
        return;
    }

    Xcp_ClearDaqList((uint8)daqListNumber);

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/**
 * @brief Handles SetDaqPtr command
 */
void Xcp_CmdSetDaqPtr(uint8 ChannelId, const uint8* Data)
{
    Xcp_DaqPtr.DaqListNumber = Data[2];
    if (XCP_MAX_DAQ_LISTS > 255U) {
        Xcp_DaqPtr.DaqListNumber |= (Data[3] << 8);
    }
    Xcp_DaqPtr.OdtNumber = Data[4];
    Xcp_DaqPtr.OdtEntryNumber = Data[5];

    if (Xcp_DaqPtr.DaqListNumber >= XCP_MAX_DAQ_LISTS) {
        Xcp_SendError(ChannelId, XCP_ERR_OUT_OF_RANGE, 0U);
        return;
    }

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/**
 * @brief Handles WriteDaq command
 */
void Xcp_CmdWriteDaq(uint8 ChannelId, const uint8* Data)
{
    Xcp_OdtEntryType* entry;

    /* Check if DAQ is running */
    if (Xcp_ChannelState[ChannelId].SessionStatus & XCP_SESSION_DAQ_RUNNING) {
        Xcp_SendError(ChannelId, XCP_ERR_DAQ_ACTIVE, 0U);
        return;
    }

    /* Check DAQ pointer bounds */
    if (Xcp_DaqPtr.DaqListNumber >= XCP_MAX_DAQ_LISTS ||
        Xcp_DaqPtr.OdtNumber >= XCP_MAX_ODTS_PER_DAQ ||
        Xcp_DaqPtr.OdtEntryNumber >= XCP_MAX_ODT_ENTRIES_PER_ODT) {
        Xcp_SendError(ChannelId, XCP_ERR_OUT_OF_RANGE, 0U);
        return;
    }

    entry = &Xcp_OdtEntries[Xcp_DaqPtr.DaqListNumber][Xcp_DaqPtr.OdtNumber][Xcp_DaqPtr.OdtEntryNumber];

    entry->BitOffset = Data[1];
    entry->EleLength = Data[2];
    entry->AddrExt = Data[3];
    entry->Addr = ((uint32)Data[4]) |
                  (((uint32)Data[5]) << 8) |
                  (((uint32)Data[6]) << 16) |
                  (((uint32)Data[7]) << 24);
    entry->IsValid = TRUE;

    /* Advance to next ODT entry */
    Xcp_DaqPtr.OdtEntryNumber++;

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/**
 * @brief Handles SetDaqListMode command
 */
void Xcp_CmdSetDaqListMode(uint8 ChannelId, const uint8* Data)
{
    uint16 daqListNumber;
    uint16 eventChannel;
    uint8 mode;

    mode = Data[1];
    daqListNumber = (uint16)Data[2];
    if (XCP_MAX_DAQ_LISTS > 255U) {
        daqListNumber |= ((uint16)Data[3] << 8);
    }
    eventChannel = (uint16)Data[5];
    if (XCP_MAX_EVENT_CHANNELS > 255U) {
        eventChannel |= ((uint16)Data[6] << 8);
    }

    if (daqListNumber >= XCP_MAX_DAQ_LISTS) {
        Xcp_SendError(ChannelId, XCP_ERR_OUT_OF_RANGE, 0U);
        return;
    }

    Xcp_DaqLists[daqListNumber].Mode = mode;
    Xcp_DaqLists[daqListNumber].EventChannel = eventChannel;
    Xcp_DaqLists[daqListNumber].Prescaler = Data[7];
    if (Xcp_DaqLists[daqListNumber].Prescaler == 0U) {
        Xcp_DaqLists[daqListNumber].Prescaler = 1U;
    }

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/**
 * @brief Handles GetDaqListMode command
 */
void Xcp_CmdGetDaqListMode(uint8 ChannelId, const uint8* Data)
{
    uint8 response[7];
    uint16 daqListNumber;

    daqListNumber = (uint16)Data[2];
    if (XCP_MAX_DAQ_LISTS > 255U) {
        daqListNumber |= ((uint16)Data[3] << 8);
    }

    if (daqListNumber >= XCP_MAX_DAQ_LISTS) {
        Xcp_SendError(ChannelId, XCP_ERR_OUT_OF_RANGE, 0U);
        return;
    }

    response[0] = 0U;  /* Reserved */
    response[1] = Xcp_DaqLists[daqListNumber].Mode;
    response[2] = (uint8)(Xcp_DaqLists[daqListNumber].EventChannel & 0xFFU);
    response[3] = (uint8)((Xcp_DaqLists[daqListNumber].EventChannel >> 8) & 0xFFU);
    response[4] = 0U;  /* Prescaler (LSB) - would be actual value */
    response[5] = 0U;  /* Prescaler (MSB) */
    response[6] = Xcp_DaqLists[daqListNumber].Priority;

    Xcp_SendResponse(ChannelId, response, 7U);
}

/**
 * @brief Handles StartStopDaqList command
 */
void Xcp_CmdStartStopDaqList(uint8 ChannelId, const uint8* Data)
{
    uint8 response[1];
    uint8 mode;
    uint16 daqListNumber;

    mode = Data[1];
    daqListNumber = (uint16)Data[2];
    if (XCP_MAX_DAQ_LISTS > 255U) {
        daqListNumber |= ((uint16)Data[3] << 8);
    }

    if (daqListNumber >= XCP_MAX_DAQ_LISTS) {
        Xcp_SendError(ChannelId, XCP_ERR_OUT_OF_RANGE, 0U);
        return;
    }

    switch (mode) {
        case 0x00U:  /* Stop */
            Xcp_DaqLists[daqListNumber].State = XCP_DAQ_STATE_STOPPED;
            Xcp_ChannelState[ChannelId].SessionStatus &= ~XCP_SESSION_DAQ_RUNNING;
            break;
        case 0x01U:  /* Start */
            Xcp_DaqLists[daqListNumber].State = XCP_DAQ_STATE_RUNNING;
            Xcp_ChannelState[ChannelId].SessionStatus |= XCP_SESSION_DAQ_RUNNING;
            break;
        case 0x02U:  /* Select (prepare for synchronized start) */
            Xcp_DaqLists[daqListNumber].Mode |= XCP_DAQ_MODE_SELECTED;
            break;
        default:
            Xcp_SendError(ChannelId, XCP_ERR_MODE_NOT_VALID, 0U);
            return;
    }

    /* Return first PID */
    response[0] = Xcp_Odts[daqListNumber][0].FirstPid;

    Xcp_SendResponse(ChannelId, response, 1U);
}

/**
 * @brief Handles StartStopSynch command
 */
void Xcp_CmdStartStopSynch(uint8 ChannelId, const uint8* Data)
{
    uint8 mode;
    uint16 daq;

    mode = Data[1];

    switch (mode) {
        case 0x00U:  /* Stop all */
            for (daq = 0U; daq < XCP_MAX_DAQ_LISTS; daq++) {
                Xcp_DaqLists[daq].State = XCP_DAQ_STATE_STOPPED;
            }
            Xcp_ChannelState[ChannelId].SessionStatus &= ~XCP_SESSION_DAQ_RUNNING;
            break;
        case 0x01U:  /* Start selected */
            for (daq = 0U; daq < XCP_MAX_DAQ_LISTS; daq++) {
                if (Xcp_DaqLists[daq].Mode & XCP_DAQ_MODE_SELECTED) {
                    Xcp_DaqLists[daq].State = XCP_DAQ_STATE_RUNNING;
                    Xcp_DaqLists[daq].Mode &= ~XCP_DAQ_MODE_SELECTED;
                }
            }
            Xcp_ChannelState[ChannelId].SessionStatus |= XCP_SESSION_DAQ_RUNNING;
            break;
        case 0x02U:  /* Stop selected */
            for (daq = 0U; daq < XCP_MAX_DAQ_LISTS; daq++) {
                if (Xcp_DaqLists[daq].Mode & XCP_DAQ_MODE_SELECTED) {
                    Xcp_DaqLists[daq].State = XCP_DAQ_STATE_STOPPED;
                    Xcp_DaqLists[daq].Mode &= ~XCP_DAQ_MODE_SELECTED;
                }
            }
            break;
        default:
            Xcp_SendError(ChannelId, XCP_ERR_MODE_NOT_VALID, 0U);
            return;
    }

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/**
 * @brief Handles GetDaqProcessorInfo command
 */
void Xcp_CmdGetDaqProcessorInfo(uint8 ChannelId)
{
    uint8 response[7];

    response[0] = 0U;  /* DAQ properties */
#if (XCP_DAQ_DYNAMIC_ALLOCATION == STD_ON)
    response[0] |= 0x01U;  /* DAQ_CONFIG_MODE */
#endif
#if (XCP_DAQ_PRESCALER_SUPPORTED == STD_ON)
    response[0] |= 0x02U;  /* PRESCALER_SUPPORTED */
#endif
#if (XCP_DAQ_RESOLUTION_SUPPORTED == STD_ON)
    response[0] |= 0x04U;  /* RESOLUTION_DAQ */
#endif
#if (XCP_DAQ_PID_OFF_SUPPORTED == STD_ON)
    response[0] |= 0x08U;  /* PID_OFF_SUPPORTED */
#endif
#if (XCP_TIMESTAMP_SUPPORTED == STD_ON)
    response[0] |= 0x10U;  /* TIMESTAMP_SUPPORTED */
#endif
#if (XCP_DAQ_OVERLOAD_INDICATION == 1U)
    response[0] |= 0x20U;  /* OVERLOAD_INDICATION_PID */
#elif (XCP_DAQ_OVERLOAD_INDICATION == 2U)
    response[0] |= 0x40U;  /* OVERLOAD_INDICATION_EVENT */
#endif

    response[1] = XCP_MAX_DAQ_LISTS;  /* Max DAQ (LSB) */
    response[2] = 0U;                  /* Max DAQ (MSB) */
    response[3] = XCP_MAX_EVENT_CHANNELS;  /* Max event channels */
    response[4] = Xcp_Odts[0][0].FirstPid;  /* Min DAQ (first PID) */
    response[5] = XCP_DAQ_KEY_BYTE;  /* DAQ key byte */
    response[6] = 0U;  /* DAQ list properties */

    Xcp_SendResponse(ChannelId, response, 7U);
}

/**
 * @brief Handles GetDaqResolutionInfo command
 */
void Xcp_CmdGetDaqResolutionInfo(uint8 ChannelId)
{
    uint8 response[4];

    response[0] = XCP_GRANULARITY_ODT_ENTRY_SIZE_DAQ;  /* Granularity ODT entry size DAQ */
    response[1] = XCP_MAX_ODT_ENTRY_SIZE_DAQ;  /* Max ODT entry size DAQ */
    response[2] = XCP_GRANULARITY_ODT_ENTRY_SIZE_STIM;  /* Granularity ODT entry size STIM */
    response[3] = XCP_MAX_ODT_ENTRY_SIZE_STIM;  /* Max ODT entry size STIM */

    Xcp_SendResponse(ChannelId, response, 4U);
}

/**
 * @brief Handles GetDaqListInfo command
 */
void Xcp_CmdGetDaqListInfo(uint8 ChannelId, const uint8* Data)
{
    uint8 response[7];
    uint16 daqListNumber;

    daqListNumber = (uint16)Data[2];
    if (XCP_MAX_DAQ_LISTS > 255U) {
        daqListNumber |= ((uint16)Data[3] << 8);
    }

    if (daqListNumber >= XCP_MAX_DAQ_LISTS) {
        Xcp_SendError(ChannelId, XCP_ERR_OUT_OF_RANGE, 0U);
        return;
    }

    response[0] = 0U;  /* DAQ list properties */
    if (Xcp_DaqLists[daqListNumber].Mode & XCP_DAQ_MODE_STIM) {
        response[0] |= 0x01U;  /* STIM mode */
    }
    if (Xcp_DaqLists[daqListNumber].Mode & XCP_DAQ_MODE_DTO_CTR) {
        response[0] |= 0x02U;  /* DTO_CTR mode */
    }
    if (Xcp_DaqLists[daqListNumber].Mode & XCP_DAQ_MODE_PID_OFF) {
        response[0] |= 0x04U;  /* PID_OFF mode */
    }
    if (Xcp_DaqLists[daqListNumber].Mode & XCP_DAQ_MODE_TIMESTAMP) {
        response[0] |= 0x08U;  /* TIMESTAMP mode */
    }

    response[1] = Xcp_DaqLists[daqListNumber].EventChannel;  /* Fixed event */
    response[2] = 0U;  /* Event (MSB) */
    response[3] = XCP_MAX_ODTS_PER_DAQ;  /* Max ODT */
    response[4] = 0U;  /* Max ODT (MSB) */
    response[5] = XCP_MAX_ODT_ENTRIES_PER_ODT;  /* Max ODT entries */
    response[6] = 0U;  /* Max ODT entries (MSB) */

    Xcp_SendResponse(ChannelId, response, 7U);
}

/**
 * @brief Handles FreeDaq command
 */
void Xcp_CmdFreeDaq(uint8 ChannelId)
{
    /* Check if DAQ is running */
    if (Xcp_ChannelState[ChannelId].SessionStatus & XCP_SESSION_DAQ_RUNNING) {
        Xcp_SendError(ChannelId, XCP_ERR_DAQ_ACTIVE, 0U);
        return;
    }

    Xcp_ResetDaqConfiguration();

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/**
 * @brief Handles AllocDaq command
 */
void Xcp_CmdAllocDaq(uint8 ChannelId, const uint8* Data)
{
    uint16 daqCount;

    /* Check if DAQ is running */
    if (Xcp_ChannelState[ChannelId].SessionStatus & XCP_SESSION_DAQ_RUNNING) {
        Xcp_SendError(ChannelId, XCP_ERR_DAQ_ACTIVE, 0U);
        return;
    }

    daqCount = (uint16)Data[2] | ((uint16)Data[3] << 8);

    if (daqCount > XCP_MAX_DAQ_LISTS) {
        Xcp_SendError(ChannelId, XCP_ERR_MEMORY_OVERFLOW, 0U);
        return;
    }

    /* Mark DAQ lists as allocated */
    for (uint16 i = 0U; i < daqCount; i++) {
        Xcp_DaqLists[i].IsAllocated = TRUE;
    }

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/**
 * @brief Handles AllocOdt command
 */
void Xcp_CmdAllocOdt(uint8 ChannelId, const uint8* Data)
{
    uint16 daqListNumber;
    uint8 odtCount;

    /* Check if DAQ is running */
    if (Xcp_ChannelState[ChannelId].SessionStatus & XCP_SESSION_DAQ_RUNNING) {
        Xcp_SendError(ChannelId, XCP_ERR_DAQ_ACTIVE, 0U);
        return;
    }

    daqListNumber = (uint16)Data[2];
    if (XCP_MAX_DAQ_LISTS > 255U) {
        daqListNumber |= ((uint16)Data[3] << 8);
    }
    odtCount = Data[4];

    if (daqListNumber >= XCP_MAX_DAQ_LISTS) {
        Xcp_SendError(ChannelId, XCP_ERR_OUT_OF_RANGE, 0U);
        return;
    }

    if (odtCount > XCP_MAX_ODTS_PER_DAQ) {
        Xcp_SendError(ChannelId, XCP_ERR_MEMORY_OVERFLOW, 0U);
        return;
    }

    Xcp_DaqLists[daqListNumber].NumOdts = odtCount;

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/**
 * @brief Handles AllocOdtEntry command
 */
void Xcp_CmdAllocOdtEntry(uint8 ChannelId, const uint8* Data)
{
    uint16 daqListNumber;
    uint8 odtNumber;
    uint8 odtEntryCount;

    /* Check if DAQ is running */
    if (Xcp_ChannelState[ChannelId].SessionStatus & XCP_SESSION_DAQ_RUNNING) {
        Xcp_SendError(ChannelId, XCP_ERR_DAQ_ACTIVE, 0U);
        return;
    }

    daqListNumber = (uint16)Data[2];
    if (XCP_MAX_DAQ_LISTS > 255U) {
        daqListNumber |= ((uint16)Data[3] << 8);
    }
    odtNumber = Data[4];
    odtEntryCount = Data[5];

    if ((daqListNumber >= XCP_MAX_DAQ_LISTS) || (odtNumber >= XCP_MAX_ODTS_PER_DAQ)) {
        Xcp_SendError(ChannelId, XCP_ERR_OUT_OF_RANGE, 0U);
        return;
    }

    if (odtEntryCount > XCP_MAX_ODT_ENTRIES_PER_ODT) {
        Xcp_SendError(ChannelId, XCP_ERR_MEMORY_OVERFLOW, 0U);
        return;
    }

    Xcp_Odts[daqListNumber][odtNumber].NumEntries = odtEntryCount;

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/*==================================================================================================
*                                    PGM COMMAND HANDLERS
==================================================================================================*/

/**
 * @brief Handles ProgramStart command
 */
void Xcp_CmdProgramStart(uint8 ChannelId)
{
    uint8 response[7];

    /* Check if PGM is already running */
    if (Xcp_PgmState != XCP_PGM_STATE_IDLE) {
        Xcp_SendError(ChannelId, XCP_ERR_PGM_ACTIVE, 0U);
        return;
    }

    /* Check resource protection */
    if (Xcp_IsResourceProtected(XCP_RESOURCE_PGM)) {
        Xcp_SendError(ChannelId, XCP_ERR_ACCESS_LOCKED, 0U);
        return;
    }

    Xcp_PgmState = XCP_PGM_STATE_STARTED;
    Xcp_ChannelState[ChannelId].SessionStatus |= XCP_SESSION_PGM_RUNNING;

    response[0] = 0U;  /* Reserved */
    response[1] = 0U;  /* Comm mode PGM */
    response[2] = XCP_MAX_CTO_PGM;  /* Max CTO */
    response[3] = (uint8)(XCP_MAX_BS_PGM & 0xFFU);  /* Max BS (LSB) */
    response[4] = (uint8)((XCP_MAX_BS_PGM >> 8) & 0xFFU);  /* Max BS (MSB) */
    response[5] = XCP_MIN_ST_PGM;  /* Min ST */
    response[6] = XCP_QUEUE_SIZE_PGM;  /* Queue size */

    Xcp_SendResponse(ChannelId, response, 7U);
}

/**
 * @brief Handles ProgramClear command
 */
void Xcp_CmdProgramClear(uint8 ChannelId, const uint8* Data)
{
    uint32 clearRange;

    if (Xcp_PgmState == XCP_PGM_STATE_IDLE) {
        Xcp_SendError(ChannelId, XCP_ERR_SEQUENCE, 0U);
        return;
    }

    clearRange = ((uint32)Data[4]) |
                 (((uint32)Data[5]) << 8) |
                 (((uint32)Data[6]) << 16) |
                 (((uint32)Data[7]) << 24);

    /* In a real implementation, this would erase the flash sector */
    XCP_UNUSED(clearRange);

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/**
 * @brief Handles Program command
 */
void Xcp_CmdProgram(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 dataLength;
    uint8 i;

    if (Xcp_PgmState == XCP_PGM_STATE_IDLE) {
        Xcp_SendError(ChannelId, XCP_ERR_SEQUENCE, 0U);
        return;
    }

    dataLength = Data[1];
    if (dataLength > (Length - 2U)) {
        Xcp_SendError(ChannelId, XCP_ERR_CMD_SYNTAX, 0U);
        return;
    }

    Xcp_PgmState = XCP_PGM_STATE_PROGRAMMING;

    /* Validate memory access */
    if (!Xcp_ValidateMemoryAccess(Xcp_ChannelState[ChannelId].Mta.Address,
                                   Xcp_ChannelState[ChannelId].Mta.Extension,
                                   dataLength, XCP_MEMORY_ACCESS_WRITE)) {
        Xcp_SendError(ChannelId, XCP_ERR_ACCESS_DENIED, 0U);
        return;
    }

    /* Program memory (in real implementation, would call flash driver) */
    for (i = 0U; i < dataLength; i++) {
        if (Xcp_WriteMemory(Xcp_ChannelState[ChannelId].Mta.Address + i,
                            Xcp_ChannelState[ChannelId].Mta.Extension,
                            &Data[2U + i], 1U) != E_OK) {
            Xcp_SendError(ChannelId, XCP_ERR_GENERIC, 0U);
            return;
        }
    }

    /* Update MTA */
    Xcp_ChannelState[ChannelId].Mta.Address += dataLength;

    /* Send response */
    {
        uint8 response[5];
        response[0] = XCP_MAX_BS_PGM;  /* Max BS */
        response[1] = XCP_MIN_ST_PGM;  /* Min ST */
        response[2] = XCP_QUEUE_SIZE_PGM;  /* Queue size (LSB) */
        response[3] = 0U;  /* Queue size */
        response[4] = 0U;  /* Queue size (MSB) */
        Xcp_SendResponse(ChannelId, response, 5U);
    }
}

/**
 * @brief Handles ProgramReset command
 */
void Xcp_CmdProgramReset(uint8 ChannelId)
{
    /* Stop PGM session */
    Xcp_PgmState = XCP_PGM_STATE_IDLE;
    Xcp_ChannelState[ChannelId].SessionStatus &= ~XCP_SESSION_PGM_RUNNING;

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);

    /* In a real implementation, this might trigger ECU reset */
}

/**
 * @brief Handles ProgramVerify command
 */
void Xcp_CmdProgramVerify(uint8 ChannelId, const uint8* Data)
{
    uint16 verifyLength;
    uint8 verifyType;

    if (Xcp_PgmState == XCP_PGM_STATE_IDLE) {
        Xcp_SendError(ChannelId, XCP_ERR_SEQUENCE, 0U);
        return;
    }

    verifyType = Data[1];
    verifyLength = (uint16)Data[2] | ((uint16)Data[3] << 8);

    XCP_UNUSED(verifyType);
    XCP_UNUSED(verifyLength);

    /* In a real implementation, this would verify the programmed data */

    Xcp_SendResponse(ChannelId, NULL_PTR, 0U);
}

/*==================================================================================================
*                                    DAQ PROCESSING
==================================================================================================*/

/**
 * @brief DAQ processor - called periodically
 */
void Xcp_DaqProcessor(void)
{
    uint16 daq;
    uint32 currentTime;

    if (Xcp_Initialized == 0U) {
        return;
    }

    currentTime = Xcp_GetTimestamp();

    for (daq = 0U; daq < XCP_MAX_DAQ_LISTS; daq++) {
        if (Xcp_DaqLists[daq].State == XCP_DAQ_STATE_RUNNING) {
            /* Check if it's time to sample this DAQ list */
            if ((currentTime - Xcp_DaqLists[daq].CurrentTimestamp) >= Xcp_DaqLists[daq].Prescaler) {
                Xcp_DaqSample(daq);
                Xcp_DaqLists[daq].CurrentTimestamp = currentTime;
            }
        }
    }
}

/**
 * @brief Sample data for a DAQ list
 */
void Xcp_DaqSample(uint16 DaqListIdx)
{
    uint8 odt;
    uint8 entry;
    uint8* bufferPtr;
    uint32 addr;
    uint32 bitOffset;
    uint32 eleLength;

    if (DaqListIdx >= XCP_MAX_DAQ_LISTS) {
        return;
    }

    /* Process each ODT in the DAQ list */
    for (odt = 0U; odt < Xcp_DaqLists[DaqListIdx].NumOdts; odt++) {
        bufferPtr = Xcp_DaqBuffer[DaqListIdx];

        /* Add identification field if needed */
        if (!(Xcp_DaqLists[DaqListIdx].Mode & XCP_DAQ_MODE_PID_OFF)) {
            bufferPtr[0] = Xcp_Odts[DaqListIdx][odt].FirstPid + odt;
            bufferPtr++;
        }

        /* Add timestamp if enabled */
        if (Xcp_DaqLists[DaqListIdx].Mode & XCP_DAQ_MODE_TIMESTAMP) {
            uint32 timestamp = Xcp_GetTimestamp();
            memcpy(bufferPtr, &timestamp, XCP_TIMESTAMP_SIZE);
            bufferPtr += XCP_TIMESTAMP_SIZE;
        }

        /* Sample each ODT entry */
        for (entry = 0U; entry < Xcp_Odts[DaqListIdx][odt].NumEntries; entry++) {
            Xcp_OdtEntryType* odtEntry = &Xcp_OdtEntries[DaqListIdx][odt][entry];

            if (!odtEntry->IsValid) {
                continue;
            }

            addr = odtEntry->Addr;
            bitOffset = odtEntry->BitOffset;
            eleLength = odtEntry->EleLength;

            /* Read data from memory */
            if (bitOffset == 0U && (eleLength % 8U) == 0U) {
                /* Byte-aligned access */
                uint32 byteLength = eleLength / 8U;
                if (byteLength > (XCP_MAX_DTO_SIZE - (bufferPtr - Xcp_DaqBuffer[DaqListIdx]))) {
                    byteLength = XCP_MAX_DTO_SIZE - (bufferPtr - Xcp_DaqBuffer[DaqListIdx]);
                }
                Xcp_ReadMemory(addr, odtEntry->AddrExt, bufferPtr, byteLength);
                bufferPtr += byteLength;
            }
            else {
                /* Bit-aligned access - simplified */
                uint8 tempData;
                if (Xcp_ReadMemory(addr, odtEntry->AddrExt, &tempData, 1U) == E_OK) {
                    *bufferPtr = tempData;
                    bufferPtr++;
                }
            }
        }

        /* Transmit DTO */
        Xcp_DaqTransmit(DaqListIdx, odt);
    }
}

/**
 * @brief Transmit DTO packet
 */
void Xcp_DaqTransmit(uint16 DaqListIdx, uint8 OdtIdx)
{
    PduInfoType pduInfo;

    XCP_UNUSED(OdtIdx);

    if (DaqListIdx >= XCP_MAX_DAQ_LISTS) {
        return;
    }

    pduInfo.SduDataPtr = Xcp_DaqBuffer[DaqListIdx];
    pduInfo.SduLength = XCP_MAX_DTO_SIZE;

    /* Send via transport layer */
    /* In real implementation: CanIf_Transmit, SoAd_Transmit, etc. */
}

/**
 * @brief STIM processor
 */
void Xcp_StimProcessor(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 queueHead;

    XCP_UNUSED(ChannelId);

    /* Add STIM data to queue */
    queueHead = Xcp_StimQueueHead[0];  /* Simplified - should use actual DAQ list index */

    if (((queueHead + 1U) % XCP_STIM_QUEUE_SIZE) != Xcp_StimQueueTail[0]) {
        memcpy(Xcp_StimQueue[0][queueHead], Data, Length);
        Xcp_StimQueueHead[0] = (queueHead + 1U) % XCP_STIM_QUEUE_SIZE;
    }
    /* Queue overflow - drop data */
}

/*==================================================================================================
*                                    MEMORY ACCESS
==================================================================================================*/

/**
 * @brief Read memory
 */
Std_ReturnType Xcp_ReadMemory(uint32 Addr, uint8 Ext, uint8* Data, uint32 Length)
{
    uint32 i;
    volatile uint8* memPtr;

    XCP_UNUSED(Ext);

    if (Data == NULL_PTR) {
        return E_NOT_OK;
    }

    /* Validate memory access */
    if (!Xcp_ValidateMemoryAccess(Addr, Ext, Length, XCP_MEMORY_ACCESS_READ)) {
        return E_NOT_OK;
    }

    /* Read data */
    memPtr = (volatile uint8*)Addr;
    for (i = 0U; i < Length; i++) {
        Data[i] = memPtr[i];
    }

    return E_OK;
}

/**
 * @brief Write memory
 */
Std_ReturnType Xcp_WriteMemory(uint32 Addr, uint8 Ext, const uint8* Data, uint32 Length)
{
    uint32 i;
    volatile uint8* memPtr;

    XCP_UNUSED(Ext);

    if (Data == NULL_PTR) {
        return E_NOT_OK;
    }

    /* Validate memory access */
    if (!Xcp_ValidateMemoryAccess(Addr, Ext, Length, XCP_MEMORY_ACCESS_WRITE)) {
        return E_NOT_OK;
    }

    /* Write data */
    memPtr = (volatile uint8*)Addr;
    for (i = 0U; i < Length; i++) {
        memPtr[i] = Data[i];
    }

    return E_OK;
}

/*==================================================================================================
*                                    RESOURCE PROTECTION
==================================================================================================*/

/**
 * @brief Set resource protection
 */
void Xcp_SetResourceProtection(uint8 Resource, boolean Protected)
{
    uint8 ch;

    for (ch = 0U; ch < XCP_NUMBER_OF_CHANNELS; ch++) {
        if (Protected) {
            XCP_SET_RESOURCE_PROTECTION(Resource, Xcp_ChannelState[ch].ResourceProtection);
        }
        else {
            XCP_CLEAR_RESOURCE_PROTECTION(Resource, Xcp_ChannelState[ch].ResourceProtection);
        }
    }
}

/**
 * @brief Check if resource is protected
 */
boolean Xcp_IsResourceProtected(uint8 Resource)
{
    return XCP_IS_RESOURCE_PROTECTED(Resource, Xcp_ChannelState[0].ResourceProtection);
}

/**
 * @brief Unlock resource
 */
Std_ReturnType Xcp_UnlockResource(uint8 Resource)
{
    Xcp_SetResourceProtection(Resource, FALSE);
    return E_OK;
}

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Process standard commands
 */
static void Xcp_ProcessStandardCommand(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 cmd;

    cmd = Data[0];

    switch (cmd) {
        case XCP_CMD_CONNECT:
            Xcp_CmdConnect(ChannelId, Data, Length);
            break;
        case XCP_CMD_DISCONNECT:
            Xcp_CmdDisconnect(ChannelId);
            break;
        case XCP_CMD_GET_STATUS:
            Xcp_CmdGetStatus(ChannelId);
            break;
        case XCP_CMD_GET_COMM_MODE_INFO:
            Xcp_CmdGetCommModeInfo(ChannelId);
            break;
        case XCP_CMD_GET_ID:
            Xcp_CmdGetId(ChannelId, Data, Length);
            break;
        case XCP_CMD_SET_MTA:
            Xcp_CmdSetMta(ChannelId, Data);
            break;
        case XCP_CMD_UPLOAD:
            Xcp_CmdUpload(ChannelId, Data);
            break;
        case XCP_CMD_SHORT_UPLOAD:
            Xcp_CmdShortUpload(ChannelId, Data);
            break;
        case XCP_CMD_DOWNLOAD:
            Xcp_CmdDownload(ChannelId, Data, Length);
            break;
        case XCP_CMD_GET_SEED:
            Xcp_CmdGetSeed(ChannelId, Data);
            break;
        case XCP_CMD_UNLOCK:
            Xcp_CmdUnlock(ChannelId, Data, Length);
            break;
        default:
            Xcp_SendError(ChannelId, XCP_ERR_CMD_UNKNOWN, 0U);
            break;
    }
}

/**
 * @brief Process DAQ commands
 */
static void Xcp_ProcessDaqCommand(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 cmd;

    XCP_UNUSED(Length);

    cmd = Data[0];

    /* Check DAQ resource protection */
    if (Xcp_IsResourceProtected(XCP_RESOURCE_DAQ)) {
        Xcp_SendError(ChannelId, XCP_ERR_ACCESS_LOCKED, 0U);
        return;
    }

    switch (cmd) {
        case XCP_CMD_CLEAR_DAQ_LIST:
            Xcp_CmdClearDaqList(ChannelId, Data);
            break;
        case XCP_CMD_SET_DAQ_PTR:
            Xcp_CmdSetDaqPtr(ChannelId, Data);
            break;
        case XCP_CMD_WRITE_DAQ:
            Xcp_CmdWriteDaq(ChannelId, Data);
            break;
        case XCP_CMD_SET_DAQ_LIST_MODE:
            Xcp_CmdSetDaqListMode(ChannelId, Data);
            break;
        case XCP_CMD_GET_DAQ_LIST_MODE:
            Xcp_CmdGetDaqListMode(ChannelId, Data);
            break;
        case XCP_CMD_START_STOP_DAQ_LIST:
            Xcp_CmdStartStopDaqList(ChannelId, Data);
            break;
        case XCP_CMD_START_STOP_SYNCH:
            Xcp_CmdStartStopSynch(ChannelId, Data);
            break;
        case XCP_CMD_GET_DAQ_PROCESSOR_INFO:
            Xcp_CmdGetDaqProcessorInfo(ChannelId);
            break;
        case XCP_CMD_GET_DAQ_RESOLUTION_INFO:
            Xcp_CmdGetDaqResolutionInfo(ChannelId);
            break;
        case XCP_CMD_GET_DAQ_LIST_INFO:
            Xcp_CmdGetDaqListInfo(ChannelId, Data);
            break;
        case XCP_CMD_FREE_DAQ:
            Xcp_CmdFreeDaq(ChannelId);
            break;
        case XCP_CMD_ALLOC_DAQ:
            Xcp_CmdAllocDaq(ChannelId, Data);
            break;
        case XCP_CMD_ALLOC_ODT:
            Xcp_CmdAllocOdt(ChannelId, Data);
            break;
        case XCP_CMD_ALLOC_ODT_ENTRY:
            Xcp_CmdAllocOdtEntry(ChannelId, Data);
            break;
        default:
            Xcp_SendError(ChannelId, XCP_ERR_CMD_UNKNOWN, 0U);
            break;
    }
}

/**
 * @brief Process PGM commands
 */
static void Xcp_ProcessPgmCommand(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 cmd;

    cmd = Data[0];

    /* Check PGM resource protection */
    if (Xcp_IsResourceProtected(XCP_RESOURCE_PGM)) {
        Xcp_SendError(ChannelId, XCP_ERR_ACCESS_LOCKED, 0U);
        return;
    }

    switch (cmd) {
        case XCP_CMD_PROGRAM_START:
            Xcp_CmdProgramStart(ChannelId);
            break;
        case XCP_CMD_PROGRAM_CLEAR:
            Xcp_CmdProgramClear(ChannelId, Data);
            break;
        case XCP_CMD_PROGRAM:
            Xcp_CmdProgram(ChannelId, Data, Length);
            break;
        case XCP_CMD_PROGRAM_RESET:
            Xcp_CmdProgramReset(ChannelId);
            break;
        case XCP_CMD_PROGRAM_VERIFY:
            Xcp_CmdProgramVerify(ChannelId, Data);
            break;
        default:
            Xcp_SendError(ChannelId, XCP_ERR_CMD_UNKNOWN, 0U);
            break;
    }
}

/**
 * @brief Calculate checksum
 */
static uint16 Xcp_CalculateChecksum(const uint8* Data, uint32 Length)
{
    uint32 i;
    uint16 crc = 0xFFFFU;
    uint8 j;

    for (i = 0U; i < Length; i++) {
        crc ^= ((uint16)Data[i] << 8);
        for (j = 0U; j < 8U; j++) {
            if (crc & 0x8000U) {
                crc = (crc << 1) ^ 0x1021U;  /* CRC-16 CCITT polynomial */
            }
            else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief Validate memory access
 */
static boolean Xcp_ValidateMemoryAccess(uint32 Addr, uint8 Ext, uint32 Length, uint8 AccessType)
{
    uint32 endAddr;
    uint8 i;

    XCP_UNUSED(Ext);

    endAddr = Addr + Length;

    for (i = 0U; i < XCP_NUMBER_OF_MEMORY_RANGES; i++) {
        uint32 rangeStart = Xcp_MemoryRanges[i].Address;
        uint32 rangeEnd = rangeStart + XCP_MAX_PROGRAMMING_SECTOR_SIZE;  /* Simplified */

        if ((Addr >= rangeStart) && (endAddr <= rangeEnd)) {
            if ((Xcp_MemoryRanges[i].AccessFlags & AccessType) == AccessType) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/**
 * @brief Clear a DAQ list
 */
static void Xcp_ClearDaqList(uint8 DaqListNumber)
{
    uint8 odt;
    uint8 entry;

    if (DaqListNumber >= XCP_MAX_DAQ_LISTS) {
        return;
    }

    Xcp_DaqLists[DaqListNumber].State = XCP_DAQ_STATE_STOPPED;
    Xcp_DaqLists[DaqListNumber].Mode = 0U;
    Xcp_DaqLists[DaqListNumber].Prescaler = 1U;

    for (odt = 0U; odt < XCP_MAX_ODTS_PER_DAQ; odt++) {
        Xcp_Odts[DaqListNumber][odt].NumEntries = 0U;

        for (entry = 0U; entry < XCP_MAX_ODT_ENTRIES_PER_ODT; entry++) {
            Xcp_OdtEntries[DaqListNumber][odt][entry].IsValid = FALSE;
        }
    }
}

/**
 * @brief Reset DAQ configuration
 */
static void Xcp_ResetDaqConfiguration(void)
{
    uint8 daq;

    for (daq = 0U; daq < XCP_MAX_DAQ_LISTS; daq++) {
        Xcp_ClearDaqList(daq);
        Xcp_DaqLists[daq].NumOdts = 0U;
        Xcp_DaqLists[daq].IsAllocated = FALSE;
    }

    Xcp_DaqPtr.DaqListNumber = 0U;
    Xcp_DaqPtr.OdtNumber = 0U;
    Xcp_DaqPtr.OdtEntryNumber = 0U;
}

/**
 * @brief Get timestamp
 */
static uint32 Xcp_GetTimestamp(void)
{
    /* In a real implementation, this would read a hardware timer */
    /* For now, return a simple counter */
    static uint32 counter = 0U;
    return counter++;
}

#define XCP_STOP_SEC_CODE
#include "MemMap.h"

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
#include "MemMap.h"
