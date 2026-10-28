/**
 * @file fee_test_support.c
 * @brief Host-side support for the Fee unit tests.
 *
 * Provides:
 *  - the Fee_Config instance referenced as `extern const Fee_ConfigType Fee_Config`
 *    by the test cases (the production tree only generates it at integration time);
 *  - Det_ReportError capturing for DET assertions;
 *  - host stubs for the Fls driver (the real Fls driver needs S32K312 registers).
 */

#include "Fee.h"
#include "Fls.h"
#include "Det.h"
#include <string.h>

/*==================================================================================================
*                                  Fls stub behaviour hooks
*================================================================================================*/
Fls_JobResultType  FeeTest_FlsJobResult = MEMIF_JOB_OK;
Std_ReturnType     FeeTest_FlsEraseResult = E_OK;
Std_ReturnType     FeeTest_FlsWriteResult = E_OK;
uint8              FeeTest_FlsReadFillByte = 0xFFu;
uint16             FeeTest_DetCallCount = 0u;
uint8              FeeTest_DetLastApiId = 0xFFu;
uint8              FeeTest_DetLastErrorId = 0xFFu;

/*==================================================================================================
*                                  Fee configuration
*================================================================================================*/
#define FEE_TEST_BLOCK_COUNT   (16u)
#define FEE_TEST_SECTOR_COUNT  (4u)

static const Fee_BlockConfigType FeeTest_BlockConfig[FEE_TEST_BLOCK_COUNT] = {
    { 0u,   64u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 1u,  256u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 2u,  512u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 3u,   17u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 4u,    8u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 5u,  128u, 64u, 1000u, TRUE,  FALSE, TRUE,  NULL_PTR },
    { 6u,  128u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 7u,  128u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 8u,  256u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 9u,   64u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 10u,  64u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 11u,  64u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 12u,  64u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 13u,  64u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 14u,  64u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR },
    { 15u,  64u,  0u, 1000u, FALSE, FALSE, FALSE, NULL_PTR }
};

static const Fee_SectorConfigType FeeTest_SectorConfig[FEE_TEST_SECTOR_COUNT] = {
    { 0x10000000u, 65536u, 0u, TRUE  },
    { 0x10010000u, 65536u, 0u, TRUE  },
    { 0x10020000u, 65536u, 0u, FALSE },
    { 0x10030000u, 65536u, 0u, FALSE }
};

const Fee_ConfigType Fee_Config = {
    FeeTest_BlockConfig,
    FeeTest_SectorConfig,
    FEE_TEST_BLOCK_COUNT,
    FEE_TEST_SECTOR_COUNT,
    8u,
    10u,
    10000u,
    100000u,
    100000u,
    FALSE,
    FALSE,
    FALSE,
    TRUE,
    TRUE,
    TRUE,
    TRUE
};

/*==================================================================================================
*                                  Det stub
*================================================================================================*/
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    FeeTest_DetCallCount++;
    FeeTest_DetLastApiId = ApiId;
    FeeTest_DetLastErrorId = ErrorId;
    return E_OK;
}

/*==================================================================================================
*                                  Fls stubs
*================================================================================================*/
void Fls_Init(const Fls_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
}

Std_ReturnType Fls_Erase(Fls_AddressType TargetAddress, Fls_LengthType Length)
{
    (void)TargetAddress;
    (void)Length;
    return FeeTest_FlsEraseResult;
}

Std_ReturnType Fls_Write(Fls_AddressType TargetAddress, const uint8* SourceAddress, Fls_LengthType Length)
{
    (void)TargetAddress;
    (void)SourceAddress;
    (void)Length;
    return FeeTest_FlsWriteResult;
}

void Fls_Read(Fls_AddressType SourceAddress, uint8* TargetAddressPtr, Fls_LengthType Length)
{
    (void)SourceAddress;

    if (TargetAddressPtr != NULL_PTR)
    {
        (void)memset(TargetAddressPtr, FeeTest_FlsReadFillByte, (size_t)Length);
    }
}

void Fls_Compare(Fls_AddressType SourceAddress, const uint8* TargetAddressPtr, Fls_LengthType Length)
{
    (void)SourceAddress;
    (void)TargetAddressPtr;
    (void)Length;
}

void Fls_SetMode(MemIf_ModeType Mode)
{
    (void)Mode;
}

Fls_JobResultType Fls_GetJobResult(void)
{
    return FeeTest_FlsJobResult;
}

void Fls_Cancel(void)
{
    FeeTest_FlsJobResult = MEMIF_JOB_CANCELED;
}

void Fls_MainFunction(void)
{
}

void Fls_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo != NULL_PTR)
    {
        (void)memset(versioninfo, 0, sizeof(*versioninfo));
    }
}

/* NvM completion callbacks invoked by Fee_JobEndNotification/ErrorNotification.
 * (Fls_JobEndNotification/Fls_JobErrorNotification are defined by Fee.c itself.) */
void NvM_JobEndNotification(void)
{
}

void NvM_JobErrorNotification(void)
{
}
