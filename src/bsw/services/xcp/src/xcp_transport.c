/*==================================================================================================
 * XCP 传输层/初始化实现
 * 自动拆分自 Xcp.c
 *================================================================================================*/
#define XCP_START_SEC_CODE
#include "MemMap.h"

/* Forward declarations for cross-file functions */
uint32 Xcp_GetTimestamp(void);
boolean Xcp_ValidateMemoryAccess(uint32 Addr, uint8 Ext, uint32 Length, uint8 AccessType);

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

/*     pduInfo.SduDataPtr = Xcp_DaqBuffer[DaqListIdx]; */
/*     pduInfo.SduLength = XCP_MAX_DTO_SIZE; */

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
    volatile const uint8* memPtr;

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
#define XCP_STOP_SEC_CODE
#include "MemMap.h"
