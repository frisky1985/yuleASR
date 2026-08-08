/*==================================================================================================
 * XCP 命令处理实现 — 被 Xcp.c 聚合
 *================================================================================================*/

void Xcp_CmdConnect(uint8 ChannelId, const uint8* Data, uint8 Length)
{
    uint8 response[7];
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
    if ((Xcp_DaqPtr.DaqListNumber >= XCP_MAX_DAQ_LISTS) ||
        ((Xcp_DaqPtr.OdtNumber >= XCP_MAX_ODTS_PER_DAQ)) ||
        (Xcp_DaqPtr.OdtEntryNumber >= XCP_MAX_ODT_ENTRIES_PER_ODT)) {
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
}


