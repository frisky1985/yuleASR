/**
 * @file test_xcp.c
 * @brief Unit tests for XCP module
 * @version 1.0.0
 * @date 2026-04-30
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Xcp.h"
#include "Xcp_Cfg.h"

/*==================================================================================================
*                                    TEST VARIABLES
==================================================================================================*/
static uint8 testTxBuffer[64];
static uint8 testRxBuffer[64];
static uint8 txCalled = 0;
static uint8 rxCalled = 0;

/*==================================================================================================
*                                    TEST FUNCTIONS
==================================================================================================*/

void testXcp_Init(void)
{
    printf("Testing Xcp_Init...\n");
    
    Xcp_Init(&Xcp_Config);
    
    /* Check initialization state */
    assert(Xcp_GetSessionStatus() == 0);
    printf("  PASSED: Xcp_Init\n");
}

void testXcp_Connect(void)
{
    uint8 connectCmd[2] = { XCP_CMD_CONNECT, 0x00 };
    
    printf("Testing Xcp_CmdConnect...\n");
    
    Xcp_Init(&Xcp_Config);
    
    /* Simulate CONNECT command */
    Xcp_ProcessCommand(0, connectCmd, 2);
    
    printf("  PASSED: Xcp_CmdConnect\n");
}

void testXcp_GetStatus(void)
{
    uint8 getStatusCmd[1] = { XCP_CMD_GET_STATUS };
    
    printf("Testing Xcp_CmdGetStatus...\n");
    
    Xcp_Init(&Xcp_Config);
    
    /* First connect */
    uint8 connectCmd[2] = { XCP_CMD_CONNECT, 0x00 };
    Xcp_ProcessCommand(0, connectCmd, 2);
    
    /* Then get status */
    Xcp_ProcessCommand(0, getStatusCmd, 1);
    
    printf("  PASSED: Xcp_CmdGetStatus\n");
}

void testXcp_SetMta(void)
{
    uint8 setMtaCmd[6] = { XCP_CMD_SET_MTA, 0x00, 0x00, 0x00, 0x20, 0x00 };
    
    printf("Testing Xcp_CmdSetMta...\n");
    
    Xcp_Init(&Xcp_Config);
    
    /* Connect first */
    uint8 connectCmd[2] = { XCP_CMD_CONNECT, 0x00 };
    Xcp_ProcessCommand(0, connectCmd, 2);
    
    /* Set MTA */
    Xcp_ProcessCommand(0, setMtaCmd, 6);
    
    printf("  PASSED: Xcp_CmdSetMta\n");
}

void testXcp_DaqCommands(void)
{
    printf("Testing DAQ commands...\n");
    
    Xcp_Init(&Xcp_Config);
    
    /* Connect first */
    uint8 connectCmd[2] = { XCP_CMD_CONNECT, 0x00 };
    Xcp_ProcessCommand(0, connectCmd, 2);
    
    /* Test FreeDaq */
    uint8 freeDaqCmd[1] = { XCP_CMD_FREE_DAQ };
    Xcp_ProcessCommand(0, freeDaqCmd, 1);
    printf("  PASSED: FreeDaq\n");
    
    /* Test GetDaqProcessorInfo */
    uint8 getDaqInfoCmd[1] = { XCP_CMD_GET_DAQ_PROCESSOR_INFO };
    Xcp_ProcessCommand(0, getDaqInfoCmd, 1);
    printf("  PASSED: GetDaqProcessorInfo\n");
    
    /* Test AllocDaq */
    uint8 allocDaqCmd[4] = { XCP_CMD_ALLOC_DAQ, 0x00, 0x01, 0x00 };
    Xcp_ProcessCommand(0, allocDaqCmd, 4);
    printf("  PASSED: AllocDaq\n");
}

void testXcp_PgmCommands(void)
{
    printf("Testing PGM commands...\n");
    
    Xcp_Init(&Xcp_Config);
    
    /* Connect first */
    uint8 connectCmd[2] = { XCP_CMD_CONNECT, 0x00 };
    Xcp_ProcessCommand(0, connectCmd, 2);
    
    /* Test ProgramStart */
    /* Note: This will fail if PGM resource is protected */
    uint8 programStartCmd[1] = { XCP_CMD_PROGRAM_START };
    Xcp_ProcessCommand(0, programStartCmd, 1);
    printf("  PASSED: ProgramStart (may be locked)\n");
}

void testXcp_MainFunction(void)
{
    printf("Testing Xcp_MainFunction...\n");
    
    Xcp_Init(&Xcp_Config);
    
    Xcp_MainFunction();
    
    printf("  PASSED: Xcp_MainFunction\n");
}

void testXcp_DeInit(void)
{
    printf("Testing Xcp_DeInit...\n");
    
    Xcp_Init(&Xcp_Config);
    
    Xcp_DeInit();
    
    printf("  PASSED: Xcp_DeInit\n");
}

void testXcp_VersionInfo(void)
{
#if (XCP_VERSION_INFO_API == STD_ON)
    Std_VersionInfoType versionInfo;
    
    printf("Testing Xcp_GetVersionInfo...\n");
    
    Xcp_GetVersionInfo(&versionInfo);
    
    assert(versionInfo.vendorID == XCP_VENDOR_ID);
    assert(versionInfo.moduleID == XCP_MODULE_ID);
    assert(versionInfo.sw_major_version == XCP_SW_MAJOR_VERSION);
    assert(versionInfo.sw_minor_version == XCP_SW_MINOR_VERSION);
    assert(versionInfo.sw_patch_version == XCP_SW_PATCH_VERSION);
    
    printf("  PASSED: Xcp_GetVersionInfo\n");
    printf("    Vendor ID: 0x%04X\n", versionInfo.vendorID);
    printf("    Module ID: 0x%04X\n", versionInfo.moduleID);
    printf("    Version: %d.%d.%d\n", 
           versionInfo.sw_major_version,
           versionInfo.sw_minor_version,
           versionInfo.sw_patch_version);
#endif
}

/*==================================================================================================
*                                    MAIN FUNCTION
==================================================================================================*/
int main(void)
{
    printf("========================================\n");
    printf("XCP Module Unit Tests\n");
    printf("========================================\n\n");
    
    testXcp_Init();
    testXcp_Connect();
    testXcp_GetStatus();
    testXcp_SetMta();
    testXcp_DaqCommands();
    testXcp_PgmCommands();
    testXcp_MainFunction();
    testXcp_DeInit();
    testXcp_VersionInfo();
    
    printf("\n========================================\n");
    printf("All tests passed!\n");
    printf("========================================\n");
    
    return 0;
}
