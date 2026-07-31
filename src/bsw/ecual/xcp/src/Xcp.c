/** @file Xcp.c
 * @brief AUTOSAR XCP module implementation
 * @details XCP protocol layer - command processing, DAQ/STIM, calibration
 * @copyright YuleTech AutoSAR BSW Platform
 */

#include "Xcp.h"
#include "Xcp_Cfg.h"
#include <string.h>

/*============================================================================
 *                          MODULE STATE VARIABLES
 *===========================================================================*/

/* Connection state */
uint8 Xcp_MtaWrite(const uint8 *buffer, uint8 count);
uint8 Xcp_MtaRead(uint8 *buffer, uint8 count);
void Xcp_MtaSet(uint32 address, uint8 extension);
void Xcp_SendDaqPacket(const Xcp_DaqListType *daqList, uint8 odtIndex);
void Xcp_DaqTrigger(uint8 eventChannel);
extern Xcp_SessionStatusType Xcp_SessionStatus;
extern Xcp_ConnectionStateType Xcp_ConnectionState;
Xcp_ConnectionStateType Xcp_ConnectionState = XCP_STATE_DISCONNECTED;
Xcp_SessionStatusType Xcp_SessionStatus = 0x00u;

/* Memory Transfer Address */
Xcp_MtaType Xcp_Mta = {0, 0};

/* Response buffer */
static uint8 Xcp_ResBuffer[XCP_CTO_SIZE];
static uint8 Xcp_ResLen = 0;

/* Command counter */
static uint16 Xcp_CommandCounter = 0;

/* Protection status */
static uint8 Xcp_ResourceProtection = XCP_RESOURCE_DEFAULT_LOCK;
static uint8 Xcp_SeedRequested = 0;
static uint8 Xcp_CurrentSeed[XCP_SEED_LENGTH] = {0};

/* DAQ state */
typedef struct {
    uint8 daqListNum;
    uint8 odtNum;
    uint8 odtEntryNum;
} Xcp_DaqPtrType;

static Xcp_DaqPtrType Xcp_DaqPtr = {0, 0, 0};
static Xcp_DaqListType Xcp_DaqListsRuntime[XCP_MAX_DAQ_LISTS];

/* Working buffers */
static uint8 Xcp_TxBuffer[XCP_DTO_SIZE];
static uint8 Xcp_DaqBuffer[XCP_DTO_SIZE];

/*============================================================================
 *                          STATIC FUNCTION PROTOTYPES
 *===========================================================================*/
static void Xcp_SendPacket(const uint8 *data, uint8 len);
static uint8 Xcp_CheckResourceAccess(uint8 resource);
static void Xcp_GetResourceSeed(uint8 resource);
static boolean Xcp_VerifyKey(const uint8 *key, uint8 len);
static void Xcp_ClearDaqList(uint8 daqListNum);
static void Xcp_ReadDaqEntry(const Xcp_DaqEntryType *entry, uint8 *data);
static void Xcp_WriteDaqEntry(Xcp_DaqEntryType *entry, const uint8 *data);
static uint16 Xcp_CalculateChecksum(const uint8 *data, uint32 size);
static void Xcp_SetResponseByte(uint8 idx, uint8 val);

/*============================================================================
 *                          INITIALIZATION
 *===========================================================================*/

/**
 * @brief XCP module initialization
 * @param config Pointer to configuration structure (NULL_PTR for link-time config)
 */
void Xcp_Init(const void *config)
{
    (void)config;

    /* Initialize state variables */
    Xcp_ConnectionState = XCP_STATE_DISCONNECTED;
    Xcp_SessionStatus = 0x00u;
    Xcp_Mta.address = 0u;
    Xcp_Mta.extension = 0u;
    Xcp_ResourceProtection = XCP_RESOURCE_DEFAULT_LOCK;
    Xcp_CommandCounter = 0;

    /* Clear response buffer */
    (void)memset(Xcp_ResBuffer, 0, sizeof(Xcp_ResBuffer));
    Xcp_ResLen = 0;

    /* Clear DAQ lists */
    (void)memset(Xcp_DaqListsRuntime, 0, sizeof(Xcp_DaqListsRuntime));
    for (uint8 i = 0; i < XCP_MAX_DAQ_LISTS; i++) {
        Xcp_DaqListsRuntime[i].state = XCP_DAQ_STATE_STOPPED;
        Xcp_DaqListsRuntime[i].selected = FALSE;
    }

    /* Clear DAQ pointer */
    Xcp_DaqPtr.daqListNum = 0;
    Xcp_DaqPtr.odtNum = 0;
    Xcp_DaqPtr.odtEntryNum = 0;
}

/**
 * @brief XCP module deinitialization
 */
void Xcp_DeInit(void)
{
    /* Stop all DAQ lists */
    for (uint8 i = 0; i < XCP_MAX_DAQ_LISTS; i++) {
        Xcp_DaqListsRuntime[i].state = XCP_DAQ_STATE_STOPPED;
    }

    Xcp_ConnectionState = XCP_STATE_DISCONNECTED;
}

/*============================================================================
 *                          MAIN FUNCTION
 *===========================================================================*/

/**
 * @brief Cyclic XCP main function
 * @details Processes DAQ and handles timeouts
 */
void Xcp_MainFunction(void)
{
    /* Process DAQ if connected */
    if (Xcp_ConnectionState == XCP_STATE_CONNECTED) {
        Xcp_DaqProcessor();
    }
}

/*============================================================================
 *                          RECEPTION HANDLING
 *===========================================================================*/

/**
 * @brief Rx indication callback from transport layer
 * @param data Received data pointer
 * @param length Data length
 */
void Xcp_RxIndication(const uint8 *data, uint16 length)
{
    if ((data == NULL_PTR) || (length == 0U ) || (length > XCP_CTO_SIZE)) {
        return;
    }

    /* Process command */
    Xcp_ProcessCommand(data, (uint8)length);
}

/**
 * @brief Process received XCP command
 * @param cmd Command data
 * @param len Command length
 */
void Xcp_ProcessCommand(const uint8 *cmd, uint8 len)
{
    uint8 pid;

    if ((cmd == NULL_PTR) || (len == 0U )) {
        return;
    }

    pid = cmd[0];
    Xcp_CommandCounter++;

    /* Handle CONNECT regardless of state */
    if (pid == XCP_CMD_CONNECT) {
        Xcp_CmdConnect(cmd);
        return;
    }

    /* Check connection state for other commands */
    if (Xcp_ConnectionState == XCP_STATE_DISCONNECTED) {
        Xcp_SendError(E_ERR_CMD_UNKNOWN);
        return;
    }

    /* Dispatch command */
    switch (pid) {
        case XCP_CMD_DISCONNECT:
            Xcp_CmdDisconnect(cmd);
            break;
        case XCP_CMD_GET_STATUS:
            Xcp_CmdGetStatus(cmd);
            break;
        case XCP_CMD_SYNCH:
            Xcp_CmdSynch(cmd);
            break;
        case XCP_CMD_SET_MTA:
            Xcp_CmdSetMta(cmd);
            break;
        case XCP_CMD_UPLOAD:
            Xcp_CmdUpload(cmd);
            break;
        case XCP_CMD_SHORT_UPLOAD:
            Xcp_CmdShortUpload(cmd);
            break;
        case XCP_CMD_DOWNLOAD:
            Xcp_CmdDownload(cmd);
            break;
        case XCP_CMD_SET_CAL_PAGE:
            Xcp_CmdSetCalPage(cmd);
            break;
        case XCP_CMD_GET_CAL_PAGE:
            Xcp_CmdGetCalPage(cmd);
            break;
        case XCP_CMD_COPY_CAL_PAGE:
            Xcp_CmdCopyCalPage(cmd);
            break;
        case XCP_CMD_FREE_DAQ:
            Xcp_CmdFreeDaq(cmd);
            break;
        case XCP_CMD_ALLOC_DAQ:
            Xcp_CmdAllocDaq(cmd);
            break;
        case XCP_CMD_ALLOC_ODT:
            Xcp_CmdAllocOdt(cmd);
            break;
        case XCP_CMD_ALLOC_ODT_ENTRY:
            Xcp_CmdAllocOdtEntry(cmd);
            break;
        case XCP_CMD_SET_DAQ_PTR:
            Xcp_CmdSetDaqPtr(cmd);
            break;
        case XCP_CMD_WRITE_DAQ:
            Xcp_CmdWriteDaq(cmd);
            break;
        case XCP_CMD_SET_DAQ_LIST_MODE:
            Xcp_CmdSetDaqListMode(cmd);
            break;
        case XCP_CMD_START_STOP_DAQ_LIST:
            Xcp_CmdStartStopDaqList(cmd);
            break;
        case XCP_CMD_START_STOP_SYNCH:
            Xcp_CmdStartStopSynch(cmd);
            break;
        case XCP_CMD_GET_DAQ_PROCESSOR_INFO:
            Xcp_CmdGetDaqProcessorInfo(cmd);
            break;
        case XCP_CMD_BUILD_CHECKSUM:
            Xcp_CmdBuildChecksum(cmd);
            break;
        default:
            Xcp_SendError(E_ERR_CMD_UNKNOWN);
            break;
    }
}

/*============================================================================
 *                          COMMAND HANDLERS
 *===========================================================================*/

/**
 * @brief CONNECT command handler
 * @param cmd Command data
 */
void Xcp_CmdConnect(const uint8 *cmd)
{
    uint8 mode;

    if (cmd[1] != 0U ) { /* Length check */ }

    mode = cmd[1]; /* Mode: normal or user defined */
    (void)mode;

    /* Build positive response */
    (void)memset(Xcp_ResBuffer, 0, XCP_CTO_SIZE);
    Xcp_ResBuffer[0] = 0xFF; /* PID = OK */
    Xcp_ResBuffer[1] = XCP_RESOURCE_CAL_PAG | XCP_RESOURCE_DAQ; /* Resource */
    Xcp_ResBuffer[2] = XCP_CTO_SIZE; /* Comm mode basic */
    Xcp_ResBuffer[3] = XCP_MAX_DAQ_LISTS; /* Max DAQ lists (LSB) */
    Xcp_ResBuffer[4] = 0; /* Max DAQ lists (MSB) */
    Xcp_ResBuffer[5] = XCP_MAX_EVENT_CHANNELS; /* Max event channels */
    Xcp_ResBuffer[6] = XCP_PROTOCOL_LAYER_VERSION_MAJOR; /* XCP protocol version */
    Xcp_ResBuffer[7] = XCP_PROTOCOL_LAYER_VERSION_MINOR; /* XCP transport version */

    Xcp_ResLen = 8;
    Xcp_ConnectionState = XCP_STATE_CONNECTED;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief DISCONNECT command handler
 * @param cmd Command data
 */
void Xcp_CmdDisconnect(const uint8 *cmd)
{
    (void)cmd;

    /* Stop all DAQ lists */
    for (uint8 i = 0; i < XCP_MAX_DAQ_LISTS; i++) {
        Xcp_DaqListsRuntime[i].state = XCP_DAQ_STATE_STOPPED;
    }

    /* Build response */
    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResLen = 1;

    Xcp_ConnectionState = XCP_STATE_DISCONNECTED;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief GET_STATUS command handler
 * @param cmd Command data
 */
void Xcp_CmdGetStatus(const uint8 *cmd)
{
    (void)cmd;

    Xcp_ResBuffer[0] = 0xFF; /* PID = OK */
    Xcp_ResBuffer[1] = Xcp_SessionStatus; /* Session status */
    Xcp_ResBuffer[2] = Xcp_ResourceProtection; /* Resource protection status */
    Xcp_ResBuffer[3] = 0; /* Session configuration ID (LSB) */
    Xcp_ResBuffer[4] = 0; /* Session configuration ID */
    Xcp_ResBuffer[5] = 0; /* Session configuration ID */
    Xcp_ResBuffer[6] = 0; /* Session configuration ID (MSB) */

    /* Clear pending store/clear requests */
    Xcp_SessionStatus &= ~(XCP_SESSION_STORE_CAL_REQ | 
                           XCP_SESSION_STORE_DAQ_REQ | 
                           XCP_SESSION_CLEAR_DAQ_REQ);

    Xcp_ResLen = 7;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief SYNCH command handler
 * @param cmd Command data
 */
void Xcp_CmdSynch(const uint8 *cmd)
{
    (void)cmd;
    Xcp_SendError(E_ERR_CMD_SYNCH);
}

/**
 * @brief SET_MTA command handler
 * @param cmd Command data
 */
void Xcp_CmdSetMta(const uint8 *cmd)
{
    Xcp_Mta.address = ((uint32)cmd[4] << 24) | 
                      ((uint32)cmd[3] << 16) |
                      ((uint32)cmd[2] << 8)  |
                      ((uint32)cmd[1]);
    Xcp_Mta.extension = cmd[5];

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResLen = 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief UPLOAD command handler
 * @param cmd Command data
 */
void Xcp_CmdUpload(const uint8 *cmd)
{
    uint8 count = cmd[1];
    uint8 i;

    if (count > XCP_CTO_SIZE - 1) {
        count = XCP_CTO_SIZE - 1;
    }

    Xcp_ResBuffer[0] = 0xFF;

    /* Read data from MTA */
    for (i = 0; i < count; i++) {
        Xcp_ResBuffer[1 + i] = *((uint8 *)(uintptr)(Xcp_Mta.address + i));
        Xcp_Mta.address++;
    }

    Xcp_ResLen = count + 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief SHORT_UPLOAD command handler
 * @param cmd Command data
 */
void Xcp_CmdShortUpload(const uint8 *cmd)
{
    uint8 count = cmd[1] & 0x07;
    uint8 ext = cmd[2];
    uint32 addr = ((uint32)cmd[6] << 24) | 
                  ((uint32)cmd[5] << 16) |
                  ((uint32)cmd[4] << 8)  |
                  ((uint32)cmd[3]);
    uint8 i;

    (void)ext;

    if (count > XCP_CTO_SIZE - 1) {
        count = XCP_CTO_SIZE - 1;
    }

    Xcp_ResBuffer[0] = 0xFF;

    /* Read data from specified address */
    for (i = 0; i < count; i++) {
        Xcp_ResBuffer[1 + i] = *((uint8 *)(uintptr)(addr + i));
    }

    Xcp_ResLen = count + 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief DOWNLOAD command handler
 * @param cmd Command data
 */
void Xcp_CmdDownload(const uint8 *cmd)
{
    uint8 count = cmd[1];
    uint8 i;

    if (count > XCP_CTO_SIZE - 2) {
        Xcp_SendError(E_ERR_OUT_OF_RANGE);
        return;
    }

    /* Check calibration resource protection */
    if (Xcp_CheckResourceAccess(XCP_RESOURCE_CAL_PAG) != E_OK_OK) {
        return;
    }

    /* Write data to MTA */
    for (i = 0; i < count; i++) {
        *((uint8 *)(uintptr)(Xcp_Mta.address + i)) = cmd[2 + i];
    }

    Xcp_Mta.address += count;

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResLen = 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief SET_CAL_PAGE command handler
 * @param cmd Command data
 */
void Xcp_CmdSetCalPage(const uint8 *cmd)
{
    uint8 mode = cmd[1];
    uint8 seg = cmd[2];
    uint8 page = cmd[3];
    uint8 result;

    (void)seg;

    /* Check calibration resource protection */
    if (Xcp_CheckResourceAccess(XCP_RESOURCE_CAL_PAG) != E_OK_OK) {
        return;
    }

    result = Xcp_SetCalPage(seg, page, mode);

    if (result == E_OK_OK) {
        Xcp_ResBuffer[0] = 0xFF;
        Xcp_ResLen = 1;
        Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
    } else {
        Xcp_SendError(result);
    }
}

/**
 * @brief GET_CAL_PAGE command handler
 * @param cmd Command data
 */
void Xcp_CmdGetCalPage(const uint8 *cmd)
{
    uint8 mode = cmd[1];
    uint8 seg = cmd[2];
    uint8 page;

    page = Xcp_GetCalPage(seg, mode);

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResBuffer[1] = 0; /* Reserved */
    Xcp_ResBuffer[2] = page;
    Xcp_ResLen = 3;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief COPY_CAL_PAGE command handler
 * @param cmd Command data
 */
void Xcp_CmdCopyCalPage(const uint8 *cmd)
{
    uint8 srcSeg = cmd[1];
    uint8 srcPage = cmd[2];
    uint8 destSeg = cmd[3];
    uint8 destPage = cmd[4];
    uint8 result;

    result = Xcp_CopyCalPage(srcSeg, srcPage, destSeg, destPage);

    if (result == E_OK_OK) {
        Xcp_ResBuffer[0] = 0xFF;
        Xcp_ResLen = 1;
        Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
    } else {
        Xcp_SendError(result);
    }
}

/*============================================================================
 *                          DAQ COMMANDS
 *===========================================================================*/

/**
 * @brief FREE_DAQ command handler
 * @param cmd Command data
 */
void Xcp_CmdFreeDaq(const uint8 *cmd)
{
    (void)cmd;

    /* Check DAQ resource protection */
    if (Xcp_CheckResourceAccess(XCP_RESOURCE_DAQ) != E_OK_OK) {
        return;
    }

    /* Clear all DAQ lists */
    for (uint8 i = 0; i < XCP_MAX_DAQ_LISTS; i++) {
        Xcp_ClearDaqList(i);
    }

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResLen = 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief ALLOC_DAQ command handler
 * @param cmd Command data
 */
void Xcp_CmdAllocDaq(const uint8 *cmd)
{
    uint16 count = ((uint16)cmd[2] << 8) | cmd[1];

    (void)count;

    /* Check DAQ resource protection */
    if (Xcp_CheckResourceAccess(XCP_RESOURCE_DAQ) != E_OK_OK) {
        return;
    }

    /* In a full implementation, allocate memory for DAQ lists */
    /* For now, just acknowledge */

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResLen = 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief ALLOC_ODT command handler
 * @param cmd Command data
 */
void Xcp_CmdAllocOdt(const uint8 *cmd)
{
    uint8 daqList = cmd[1];
    uint8 count = cmd[2];

    (void)daqList;
    (void)count;

    /* Check DAQ resource protection */
    if (Xcp_CheckResourceAccess(XCP_RESOURCE_DAQ) != E_OK_OK) {
        return;
    }

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResLen = 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief ALLOC_ODT_ENTRY command handler
 * @param cmd Command data
 */
void Xcp_CmdAllocOdtEntry(const uint8 *cmd)
{
    uint8 daqList = cmd[1];
    uint8 odt = cmd[2];
    uint8 count = cmd[3];

    (void)daqList;
    (void)odt;
    (void)count;

    /* Check DAQ resource protection */
    if (Xcp_CheckResourceAccess(XCP_RESOURCE_DAQ) != E_OK_OK) {
        return;
    }

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResLen = 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief SET_DAQ_PTR command handler
 * @param cmd Command data
 */
void Xcp_CmdSetDaqPtr(const uint8 *cmd)
{
    Xcp_DaqPtr.daqListNum = cmd[2];
    Xcp_DaqPtr.odtNum = cmd[3];
    Xcp_DaqPtr.odtEntryNum = cmd[4];

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResLen = 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief WRITE_DAQ command handler
 * @param cmd Command data
 */
void Xcp_CmdWriteDaq(const uint8 *cmd)
{
    uint8 size = cmd[1];
    uint8 ext = cmd[2];
    uint32 addr = ((uint32)cmd[6] << 24) | 
                  ((uint32)cmd[5] << 16) |
                  ((uint32)cmd[4] << 8)  |
                  ((uint32)cmd[3]);

    (void)size;
    (void)ext;
    (void)addr;

    /* Check DAQ resource protection */
    if (Xcp_CheckResourceAccess(XCP_RESOURCE_DAQ) != E_OK_OK) {
        return;
    }

    /* Store DAQ entry at current pointer position */
    /* In a full implementation, this would store the entry */

    Xcp_DaqPtr.odtEntryNum++;

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResLen = 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief SET_DAQ_LIST_MODE command handler
 * @param cmd Command data
 */
void Xcp_CmdSetDaqListMode(const uint8 *cmd)
{
    uint8 daqList = cmd[2];
    Xcp_DaqListModeType mode = cmd[1];
    uint8 eventChannel = cmd[4];
    uint8 prescaler = cmd[6];

    if (daqList >= XCP_MAX_DAQ_LISTS) {
        Xcp_SendError(E_ERR_OUT_OF_RANGE);
        return;
    }

    Xcp_DaqListsRuntime[daqList].mode = mode;
    Xcp_DaqListsRuntime[daqList].eventChannel = eventChannel;
    Xcp_DaqListsRuntime[daqList].prescaler = prescaler;

    if (mode & XCP_DAQ_MODE_SELECTED) {
        Xcp_DaqListsRuntime[daqList].selected = TRUE;
    }

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResLen = 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief START_STOP_DAQ_LIST command handler
 * @param cmd Command data
 */
void Xcp_CmdStartStopDaqList(const uint8 *cmd)
{
    uint8 mode = cmd[1];
    uint8 daqList = cmd[2];

    if (daqList >= XCP_MAX_DAQ_LISTS) {
        Xcp_SendError(E_ERR_OUT_OF_RANGE);
        return;
    }

    switch (mode) {
        case 0: /* Stop */
            Xcp_DaqListsRuntime[daqList].state = XCP_DAQ_STATE_STOPPED;
            break;
        case 1: /* Start */
            Xcp_DaqListsRuntime[daqList].state = XCP_DAQ_STATE_RUNNING;
            break;
        case 2: /* Select (prepare for synchronized start) */
            Xcp_DaqListsRuntime[daqList].selected = TRUE;
            break;
        default:
            Xcp_SendError(E_ERR_OUT_OF_RANGE);
            return;
    }

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResBuffer[1] = 0; /* First PID */
    Xcp_ResLen = 2;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief START_STOP_SYNCH command handler
 * @param cmd Command data
 */
void Xcp_CmdStartStopSynch(const uint8 *cmd)
{
    uint8 mode = cmd[1];
    uint8 i;

    switch (mode) {
        case 0: /* Stop all */
            for (i = 0; i < XCP_MAX_DAQ_LISTS; i++) {
                Xcp_DaqListsRuntime[i].state = XCP_DAQ_STATE_STOPPED;
            }
            break;
        case 1: /* Start selected */
            for (i = 0; i < XCP_MAX_DAQ_LISTS; i++) {
                if (Xcp_DaqListsRuntime[i].selected) {
                    Xcp_DaqListsRuntime[i].state = XCP_DAQ_STATE_RUNNING;
                    Xcp_DaqListsRuntime[i].selected = FALSE;
                }
            }
            break;
        case 2: /* Stop selected */
            for (i = 0; i < XCP_MAX_DAQ_LISTS; i++) {
                if (Xcp_DaqListsRuntime[i].selected) {
                    Xcp_DaqListsRuntime[i].state = XCP_DAQ_STATE_STOPPED;
                    Xcp_DaqListsRuntime[i].selected = FALSE;
                }
            }
            break;
        default:
            Xcp_SendError(E_ERR_OUT_OF_RANGE);
            return;
    }

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResLen = 1;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief GET_DAQ_PROCESSOR_INFO command handler
 * @param cmd Command data
 */
void Xcp_CmdGetDaqProcessorInfo(const uint8 *cmd)
{
    (void)cmd;

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResBuffer[1] = 0xC3; /* DAQ properties: dynamic, timestamp, PID_off */
    Xcp_ResBuffer[2] = XCP_MAX_DAQ_LISTS; /* Max DAQ lists */
    Xcp_ResBuffer[3] = 0;
    Xcp_ResBuffer[4] = XCP_MAX_EVENT_CHANNELS; /* Max event channels */
    Xcp_ResBuffer[5] = XCP_MIN_ST; /* Min DAQ */
    Xcp_ResBuffer[6] = XCP_TIMESTAMP_SIZE; /* DAQ key byte */
    Xcp_ResBuffer[7] = 0;

    Xcp_ResLen = 8;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/**
 * @brief BUILD_CHECKSUM command handler
 * @param cmd Command data
 */
void Xcp_CmdBuildChecksum(const uint8 *cmd)
{
    uint32 size = ((uint32)cmd[4] << 24) | 
                  ((uint32)cmd[3] << 16) |
                  ((uint32)cmd[2] << 8)  |
                  ((uint32)cmd[1]);
    uint16 checksum;

    checksum = Xcp_CalculateChecksum((const uint8 *)(uintptr)Xcp_Mta.address, size);
    Xcp_Mta.address += size;

    Xcp_ResBuffer[0] = 0xFF;
    Xcp_ResBuffer[1] = XCP_CHECKSUM_TYPE;
    Xcp_ResBuffer[2] = 0;
    Xcp_ResBuffer[3] = 0;
    Xcp_ResBuffer[4] = (uint8)(checksum >> 8);
    Xcp_ResBuffer[5] = (uint8)checksum;

    Xcp_ResLen = 6;
    Xcp_SendPacket(Xcp_ResBuffer, Xcp_ResLen);
}

/*============================================================================
 *                          DAQ PROCESSING
 *===========================================================================*/

/**
 * @brief DAQ processor - cyclic function
 */
void Xcp_DaqProcessor(void)
{
    /* Process running DAQ lists */
    /* In a full implementation, check event channels and trigger DAQ */
}

/**
 * @brief Trigger DAQ for an event channel
 * @param eventChannel Event channel number
 */
void Xcp_DaqTrigger(uint8 eventChannel)
{
    uint8 i;

    for (i = 0; i < XCP_MAX_DAQ_LISTS; i++) {
        if ((Xcp_DaqListsRuntime[i].state == XCP_DAQ_STATE_RUNNING) &&
            (Xcp_DaqListsRuntime[i].eventChannel == eventChannel)) {
            /* Process DAQ list */
            /* Xcp_SendDaqPacket(&Xcp_DaqListsRuntime[i], 0); */
        }
    }
}

/**
 * @brief Send DAQ DTO packet
 * @param daqList DAQ list pointer
 * @param odtIndex ODT index
 */
void Xcp_SendDaqPacket(const Xcp_DaqListType *daqList, uint8 odtIndex)
{
    (void)daqList;
    (void)odtIndex;
    /* In a full implementation, construct and send DAQ DTO */
}

/*============================================================================
 *                          CALIBRATION PAGE SWITCHING
 *===========================================================================*/

/**
 * @brief Set calibration page
 * @param segment Segment number
 * @param page Page number
 * @param mode Page mode (ECU or XCP)
 * @return Error code
 */
uint8 Xcp_SetCalPage(uint8 segment, uint8 page, uint8 mode)
{
    (void)segment;
    (void)page;
    (void)mode;

    /* In a full implementation, switch calibration page */
    /* This would typically involve remapping memory */

    return E_OK_OK;
}

/**
 * @brief Get calibration page
 * @param segment Segment number
 * @param mode Page mode
 * @return Current page number
 */
uint8 Xcp_GetCalPage(uint8 segment, uint8 mode)
{
    (void)segment;
    (void)mode;

    /* In a full implementation, return active calibration page */
    return 0;
}

/**
 * @brief Copy calibration page
 * @param srcSeg Source segment
 * @param srcPage Source page
 * @param destSeg Destination segment
 * @param destPage Destination page
 * @return Error code
 */
uint8 Xcp_CopyCalPage(uint8 srcSeg, uint8 srcPage, uint8 destSeg, uint8 destPage)
{
    (void)srcSeg;
    (void)srcPage;
    (void)destSeg;
    (void)destPage;

    /* In a full implementation, copy calibration page */
    return E_OK_OK;
}

/*============================================================================
 *                          UTILITY FUNCTIONS
 *===========================================================================*/

/**
 * @brief Send positive response
 * @param data Response data
 * @param len Data length
 */
void Xcp_SendResponse(const uint8 *data, uint8 len)
{
    if ((data != NULL_PTR) && (len > 0U ) && (len <= XCP_CTO_SIZE)) {
        Xcp_SendPacket(data, len);
    }
}

/**
 * @brief Send error response
 * @param errorCode Error code
 */
void Xcp_SendError(uint8 errorCode)
{
    uint8 errPacket[2];
    errPacket[0] = 0xFE; /* Error packet ID */
    errPacket[1] = errorCode;
    Xcp_SendPacket(errPacket, 2);
}

/**
 * @brief Send packet to transport layer
 * @param data Data to send
 * @param len Data length
 */
static void Xcp_SendPacket(const uint8 *data, uint8 len)
{
    /* Copy to TX buffer */
    if ((data != NULL_PTR) && (len <= XCP_CTO_SIZE)) {
        (void)memcpy(Xcp_TxBuffer, data, len);
        /* In a full implementation, call transport layer TX function */
        /* e.g., CanIf_Transmit() for XCP on CAN */
        (void)Xcp_TxConfirmation;
    }
}

/**
 * @brief TX confirmation callback
 */
void Xcp_TxConfirmation(void)
{
    /* Transmission completed */
}

/**
 * @brief Check resource access
 * @param resource Resource to check
 * @return Error code
 */
static uint8 Xcp_CheckResourceAccess(uint8 resource)
{
    if (Xcp_ResourceProtection & resource) {
        Xcp_SendError(E_ERR_ACCESS_LOCKED);
        return E_ERR_ACCESS_LOCKED;
    }
    return E_OK_OK;
}

/**
 * @brief Clear DAQ list
 * @param daqListNum DAQ list number
 */
static void Xcp_ClearDaqList(uint8 daqListNum)
{
    if (daqListNum < XCP_MAX_DAQ_LISTS) {
        Xcp_DaqListsRuntime[daqListNum].state = XCP_DAQ_STATE_STOPPED;
        Xcp_DaqListsRuntime[daqListNum].odtCount = 0;
        Xcp_DaqListsRuntime[daqListNum].eventChannel = 0;
        Xcp_DaqListsRuntime[daqListNum].prescaler = 1;
        Xcp_DaqListsRuntime[daqListNum].mode = 0;
        Xcp_DaqListsRuntime[daqListNum].selected = FALSE;
    }
}

/**
 * @brief Calculate checksum
 * @param data Data pointer
 * @param size Data size
 * @return Checksum value
 */
static uint16 Xcp_CalculateChecksum(const uint8 *data, uint32 size)
{
    uint16 crc = 0xFFFF;
    uint32 i;

    for (i = 0; i < size; i++) {
        crc ^= data[i];
        crc = (crc >> 8) | (crc << 8);
        crc ^= (crc & 0xFF) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xFF) << 4) << 1;
    }

    return crc;
}

/**
 * @brief Set MTA
 * @param address Memory address
 * @param extension Address extension
 */
void Xcp_MtaSet(uint32 address, uint8 extension)
{
    Xcp_Mta.address = address;
    Xcp_Mta.extension = extension;
}

/**
 * @brief Read from MTA
 * @param buffer Output buffer
 * @param count Number of bytes
 * @return Error code
 */
uint8 Xcp_MtaRead(uint8 *buffer, uint8 count)
{
    uint8 i;

    if (buffer == NULL_PTR) {
        return E_ERR_OUT_OF_RANGE;
    }

    for (i = 0; i < count; i++) {
        buffer[i] = *((uint8 *)(uintptr)(Xcp_Mta.address + i));
    }

    Xcp_Mta.address += count;
    return E_OK_OK;
}

/**
 * @brief Write to MTA
 * @param buffer Input buffer
 * @param count Number of bytes
 * @return Error code
 */
uint8 Xcp_MtaWrite(const uint8 *buffer, uint8 count)
{
    uint8 i;

    if (buffer == NULL_PTR) {
        return E_ERR_OUT_OF_RANGE;
    }

    for (i = 0; i < count; i++) {
        *((uint8 *)(uintptr)(Xcp_Mta.address + i)) = buffer[i];
    }

    Xcp_Mta.address += count;
    return E_OK_OK;
}
