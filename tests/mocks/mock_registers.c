/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Hardware Register Mock
*
* SW Version           : 1.0.0
* Build Date           : 2026-05-15
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* @file mock_registers.c
* @brief Hardware register mock implementation for unit testing
==================================================================================================*/

#include "mock_registers.h"
#include <string.h>

/*==================================================================================================
*                                      MAX REGISTERS
==================================================================================================*/
#define MOCK_MAX_REGISTERS              (256U)

/*==================================================================================================
*                                      REGISTER STORAGE
==================================================================================================*/
typedef struct {
    uint32 address;
    uint32 value;
    boolean used;
} MockRegisterEntryType;

static MockRegisterEntryType MockRegisters[MOCK_MAX_REGISTERS];
static uint32 MockRegisterCount = 0;

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Find register index by address
 */
static sint32 FindRegisterIndex(uint32 address)
{
    for (uint32 i = 0; i < MockRegisterCount; i++)
    {
        if (MockRegisters[i].address == address && MockRegisters[i].used)
        {
            return (sint32)i;
        }
    }
    return -1;  /* Not found */
}

/**
 * @brief Add new register entry
 */
static sint32 AddRegisterEntry(uint32 address)
{
    /* Check if already exists */
    sint32 index = FindRegisterIndex(address);
    if (index >= 0)
    {
        return index;
    }

    /* Add new entry if space available */
    if (MockRegisterCount < MOCK_MAX_REGISTERS)
    {
        MockRegisters[MockRegisterCount].address = address;
        MockRegisters[MockRegisterCount].value = 0;
        MockRegisters[MockRegisterCount].used = TRUE;
        return (sint32)MockRegisterCount++;
    }

    /* Table full - reuse an entry (simple approach) */
    return -1;
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

void MockRegisters_Reset(void)
{
    memset(MockRegisters, 0, sizeof(MockRegisters));
    MockRegisterCount = 0;
}

uint32 MockRegisters_Read32(uint32 address)
{
    sint32 index = FindRegisterIndex(address);
    if (index >= 0)
    {
        return MockRegisters[index].value;
    }

    /* Return 0 for uninitialized registers */
    return 0U;
}

void MockRegisters_Write32(uint32 address, uint32 value)
{
    sint32 index = AddRegisterEntry(address);
    if (index >= 0)
    {
        MockRegisters[index].value = value;
    }
}

uint16 MockRegisters_Read16(uint32 address)
{
    return (uint16)(MockRegisters_Read32(address) & 0xFFFFU);
}

void MockRegisters_Write16(uint32 address, uint16 value)
{
    uint32 currentValue = MockRegisters_Read32(address);
    MockRegisters_Write32(address, (currentValue & 0xFFFF0000U) | value);
}

uint8 MockRegisters_Read8(uint32 address)
{
    return (uint8)(MockRegisters_Read32(address) & 0xFFU);
}

void MockRegisters_Write8(uint32 address, uint8 value)
{
    uint32 currentValue = MockRegisters_Read32(address);
    MockRegisters_Write32(address, (currentValue & 0xFFFFFF00U) | value);
}
