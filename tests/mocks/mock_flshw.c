/*==================================================================================================
 *                                  MOCK FLASH HARDWARE LAYER
 *==================================================================================================
 * FILENAME: mock_flshw.c
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Mock implementation of Flash Hardware layer for integration testing
 *==================================================================================================
 */

#include "mock_flshw.h"
#include <string.h>
#include <stdio.h>

/*==================================================================================================
 *                                      GLOBAL VARIABLES
 *==================================================================================================*/
uint8 Mock_FlashMemory[MOCK_FLASH_SIZE];
boolean Mock_FlsHw_WriteFailInjected = FALSE;
boolean Mock_FlsHw_ReadFailInjected = FALSE;
boolean Mock_FlsHw_EraseFailInjected = FALSE;

static uint32 WriteOperationCount = 0u;
static uint32 ReadOperationCount = 0u;
static uint32 EraseOperationCount = 0u;

/*==================================================================================================
 *                                      INTERNAL FUNCTIONS
 *==================================================================================================*/
static boolean IsAddressValid(uint32 Address)
{
    return (Address < MOCK_FLASH_SIZE);
}

static boolean IsSectorAligned(uint32 Address)
{
    return ((Address % MOCK_FLASH_SECTOR_SIZE) == 0u);
}

/*==================================================================================================
 *                                      API IMPLEMENTATION
 *==================================================================================================*/
void Mock_FlsHw_Init(void)
{
    memset(Mock_FlashMemory, 0xFFu, MOCK_FLASH_SIZE);
    Mock_FlsHw_WriteFailInjected = FALSE;
    Mock_FlsHw_ReadFailInjected = FALSE;
    Mock_FlsHw_EraseFailInjected = FALSE;
    WriteOperationCount = 0u;
    ReadOperationCount = 0u;
    EraseOperationCount = 0u;
    printf("[MOCK] Flash Hardware Initialized\n");
}

void Mock_FlsHw_DeInit(void)
{
    printf("[MOCK] Flash Hardware Deinitialized\n");
}

Mock_FlsHw_StatusType Mock_FlsHw_Read(uint32 Address, uint8* DataPtr, uint32 Length)
{
    if (Mock_FlsHw_ReadFailInjected)
    {
        printf("[MOCK] Flash Read Failed (Injected)\n");
        return MOCK_FLSHW_NOT_OK;
    }
    
    if (!IsAddressValid(Address))
    {
        printf("[MOCK] Flash Read: Invalid Address 0x%08X\n", (unsigned int)Address);
        return MOCK_FLSHW_NOT_OK;
    }
    
    if ((Address + Length) > MOCK_FLASH_SIZE)
    {
        printf("[MOCK] Flash Read: Address out of range\n");
        return MOCK_FLSHW_NOT_OK;
    }
    
    if (DataPtr == NULL_PTR)
    {
        printf("[MOCK] Flash Read: NULL pointer\n");
        return MOCK_FLSHW_NOT_OK;
    }
    
    memcpy(DataPtr, &Mock_FlashMemory[Address], Length);
    ReadOperationCount++;
    
    return MOCK_FLSHW_OK;
}

Mock_FlsHw_StatusType Mock_FlsHw_Write(uint32 Address, const uint8* DataPtr, uint32 Length)
{
    uint32 i;
    
    if (Mock_FlsHw_WriteFailInjected)
    {
        printf("[MOCK] Flash Write Failed (Injected)\n");
        return MOCK_FLSHW_NOT_OK;
    }
    
    if (!IsAddressValid(Address))
    {
        printf("[MOCK] Flash Write: Invalid Address 0x%08X\n", (unsigned int)Address);
        return MOCK_FLSHW_NOT_OK;
    }
    
    if ((Address + Length) > MOCK_FLASH_SIZE)
    {
        printf("[MOCK] Flash Write: Address out of range\n");
        return MOCK_FLSHW_NOT_OK;
    }
    
    if (DataPtr == NULL_PTR)
    {
        printf("[MOCK] Flash Write: NULL pointer\n");
        return MOCK_FLSHW_NOT_OK;
    }
    
    /* Flash write can only change bits from 1 to 0 */
    for (i = 0u; i < Length; i++)
    {
        Mock_FlashMemory[Address + i] &= DataPtr[i];
    }
    
    WriteOperationCount++;
    printf("[MOCK] Flash Write: %u bytes at 0x%08X\n", (unsigned int)Length, (unsigned int)Address);
    
    return MOCK_FLSHW_OK;
}

Mock_FlsHw_StatusType Mock_FlsHw_Erase(uint32 Address, uint32 Size)
{
    if (Mock_FlsHw_EraseFailInjected)
    {
        printf("[MOCK] Flash Erase Failed (Injected)\n");
        return MOCK_FLSHW_NOT_OK;
    }
    
    if (!IsAddressValid(Address))
    {
        printf("[MOCK] Flash Erase: Invalid Address 0x%08X\n", (unsigned int)Address);
        return MOCK_FLSHW_NOT_OK;
    }
    
    if (!IsSectorAligned(Address))
    {
        printf("[MOCK] Flash Erase: Address not sector aligned\n");
        return MOCK_FLSHW_NOT_OK;
    }
    
    if (Size != MOCK_FLASH_SECTOR_SIZE)
    {
        printf("[MOCK] Flash Erase: Invalid size (must be sector size)\n");
        return MOCK_FLSHW_NOT_OK;
    }
    
    if ((Address + Size) > MOCK_FLASH_SIZE)
    {
        printf("[MOCK] Flash Erase: Address out of range\n");
        return MOCK_FLSHW_NOT_OK;
    }
    
    memset(&Mock_FlashMemory[Address], 0xFFu, Size);
    EraseOperationCount++;
    printf("[MOCK] Flash Erase: Sector at 0x%08X\n", (unsigned int)Address);
    
    return MOCK_FLSHW_OK;
}

void Mock_FlsHw_InjectWriteFail(boolean enable)
{
    Mock_FlsHw_WriteFailInjected = enable;
    if (enable)
    {
        printf("[MOCK] Write Failure Injection Enabled\n");
    }
}

void Mock_FlsHw_InjectReadFail(boolean enable)
{
    Mock_FlsHw_ReadFailInjected = enable;
    if (enable)
    {
        printf("[MOCK] Read Failure Injection Enabled\n");
    }
}

void Mock_FlsHw_InjectEraseFail(boolean enable)
{
    Mock_FlsHw_EraseFailInjected = enable;
    if (enable)
    {
        printf("[MOCK] Erase Failure Injection Enabled\n");
    }
}

void Mock_FlsHw_GetSectorInfo(uint32 Address, Mock_FlsHw_SectorInfoType* InfoPtr)
{
    if (InfoPtr == NULL_PTR)
    {
        return;
    }
    
    InfoPtr->Address = Address;
    InfoPtr->Size = MOCK_FLASH_SECTOR_SIZE;
    InfoPtr->EraseCount = EraseOperationCount;
    InfoPtr->IsErased = TRUE;
    InfoPtr->IsValid = IsSectorAligned(Address);
}

void Mock_FlsHw_Clear(void)
{
    memset(Mock_FlashMemory, 0xFFu, MOCK_FLASH_SIZE);
    WriteOperationCount = 0u;
    ReadOperationCount = 0u;
    EraseOperationCount = 0u;
    printf("[MOCK] Flash Memory Cleared\n");
}
