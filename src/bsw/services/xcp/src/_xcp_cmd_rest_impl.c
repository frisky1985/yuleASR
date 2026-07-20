/*==================================================================================================
 * XCP 命令处理实现 — 被 Xcp.c 聚合
 *================================================================================================*/

void Xcp_CmdConnect(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 response[7];
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
            Xcp_OdtEntryType const* odtEntry = &Xcp_OdtEntries[DaqListIdx][odt][entry];

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
    // cppcheck-suppress unusedVariable
    PduInfoType pduInfo;

    XCP_UNUSED(OdtIdx);

    if (DaqListIdx >= XCP_MAX_DAQ_LISTS) {
        return;
    }

    /* UNREAD: pduInfo.SduDataPtr = Xcp_DaqBuffer[DaqListIdx]; */
    /* UNREAD: pduInfo.SduLength = XCP_MAX_DTO_SIZE; */

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
    volatile uint8 const* memPtr;

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


