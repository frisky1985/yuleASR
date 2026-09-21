/**
 * @file stubs.c
 * @brief MemIf driver stubs for the NvM unit tests.
 *
 * Records every call the SUT (NvM.c) makes into the Memory Interface and
 * lets each test control the returned status/job result so the synchronous
 * NvM_MainFunction() job lifecycle can be driven deterministically.
 */

#include "MemIf.h"

/* ---- call recorders ---- */
uint32 stub_MemIf_Read_calls = 0u;
uint32 stub_MemIf_Write_calls = 0u;
uint32 stub_MemIf_Erase_calls = 0u;
uint32 stub_MemIf_Invalidate_calls = 0u;

uint8  stub_last_Device = 0u;
uint16 stub_last_BlockNumber = 0u;
uint16 stub_last_Offset = 0u;
uint16 stub_last_Length = 0u;
uint8* stub_last_ReadPtr = (uint8*)0;
const uint8* stub_last_WritePtr = (const uint8*)0;

/* ---- controllable return values ---- */
Std_ReturnType stub_MemIf_Read_ret = E_OK;
Std_ReturnType stub_MemIf_Write_ret = E_OK;
Std_ReturnType stub_MemIf_Erase_ret = E_OK;
Std_ReturnType stub_MemIf_Invalidate_ret = E_OK;
MemIf_StatusType stub_MemIf_Status = MEMIF_IDLE;
MemIf_JobResultType stub_MemIf_JobResult = MEMIF_JOB_OK;

Std_ReturnType MemIf_Read(uint8 DeviceIndex,
                          uint16 BlockNumber,
                          uint16 BlockOffset,
                          uint8* DataPtr,
                          uint16 Length)
{
    stub_MemIf_Read_calls++;
    stub_last_Device = DeviceIndex;
    stub_last_BlockNumber = BlockNumber;
    stub_last_Offset = BlockOffset;
    stub_last_ReadPtr = DataPtr;
    stub_last_Length = Length;
    return stub_MemIf_Read_ret;
}

Std_ReturnType MemIf_Write(uint8 DeviceIndex,
                           uint16 BlockNumber,
                           const uint8* DataPtr)
{
    stub_MemIf_Write_calls++;
    stub_last_Device = DeviceIndex;
    stub_last_BlockNumber = BlockNumber;
    stub_last_WritePtr = DataPtr;
    return stub_MemIf_Write_ret;
}

Std_ReturnType MemIf_EraseImmediateBlock(uint8 DeviceIndex,
                                         uint16 BlockNumber)
{
    stub_MemIf_Erase_calls++;
    stub_last_Device = DeviceIndex;
    stub_last_BlockNumber = BlockNumber;
    return stub_MemIf_Erase_ret;
}

Std_ReturnType MemIf_InvalidateBlock(uint8 DeviceIndex,
                                     uint16 BlockNumber)
{
    stub_MemIf_Invalidate_calls++;
    stub_last_Device = DeviceIndex;
    stub_last_BlockNumber = BlockNumber;
    return stub_MemIf_Invalidate_ret;
}

MemIf_StatusType MemIf_GetStatus(uint8 DeviceIndex)
{
    stub_last_Device = DeviceIndex;
    return stub_MemIf_Status;
}

MemIf_JobResultType MemIf_GetJobResult(uint8 DeviceIndex)
{
    stub_last_Device = DeviceIndex;
    return stub_MemIf_JobResult;
}
