/*==================================================================================================
 * XCP 命令处理实现 — 被 Xcp.c 聚合
 *================================================================================================*/

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
}


