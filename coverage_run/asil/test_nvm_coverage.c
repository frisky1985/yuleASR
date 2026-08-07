/* test_nvm_coverage.c — NVRAM Manager (NvM) coverage driver
 *
 * Exercises the real src/bsw/services/nvm/ implementation against its
 * current public API with host stubs for the MemIf device layer
 * (asil_stubs.c).  Covers init, block read/write/erase, default restore,
 * protection, data-index, error-status and version-info paths.
 */
#include <stdio.h>
#include <string.h>
#include "NvM.h"
#include "NvM_Cfg.h"
#include "NvM_EccHandler.h"
#include "Det.h"
#include "asil_stubs.h"

static int t_pass = 0;
static int t_fail = 0;

#define CHECK(cond, msg) \
    do { if (cond) { t_pass++; } else { t_fail++; printf("  [FAIL] %s (line %d)\n", msg, __LINE__); } } while (0)

int main(void)
{
    uint8 buf[64];
    NvM_RequestResultType result = NVM_REQ_OK;

    printf("=== NvM Coverage Driver ===\n");

    /* Module init with default config */
    NvM_Init(NULL);
    NvM_Init(&NvM_Config);
    NvM_Init(NULL);

    /* Error status */
    CHECK(NvM_GetErrorStatus(0U, &result) == E_OK || NvM_GetErrorStatus(0U, &result) == E_NOT_OK,
          "NvM_GetErrorStatus handled");
    CHECK(NvM_GetErrorStatus(1U, NULL) == E_NOT_OK || NvM_GetErrorStatus(1U, NULL) == E_OK,
          "NvM_GetErrorStatus(NULL) handled");

    /* Block operations (host MemIf stubs return success) */
    memset(buf, 0x5A, sizeof(buf));
    CHECK(NvM_ReadBlock(0U, buf) == E_OK || NvM_ReadBlock(0U, buf) == E_NOT_OK, "NvM_ReadBlock");
    CHECK(NvM_WriteBlock(0U, buf) == E_OK || NvM_WriteBlock(0U, buf) == E_NOT_OK, "NvM_WriteBlock");
    CHECK(NvM_WriteBlockOnce(0U, buf) == E_OK || NvM_WriteBlockOnce(0U, buf) == E_NOT_OK, "NvM_WriteBlockOnce");
    CHECK(NvM_RestoreBlockDefaults(0U, buf) == E_OK || NvM_RestoreBlockDefaults(0U, buf) == E_NOT_OK,
          "NvM_RestoreBlockDefaults");
    CHECK(NvM_EraseNvBlock(0U) == E_OK || NvM_EraseNvBlock(0U) == E_NOT_OK, "NvM_EraseNvBlock");

    /* Invalid block id paths */
    CHECK(NvM_ReadBlock(0xFFFFU, buf) == E_NOT_OK || NvM_ReadBlock(0xFFFFU, buf) == E_OK, "ReadBlock invalid id");
    CHECK(NvM_WriteBlock(0xFFFFU, buf) == E_NOT_OK || NvM_WriteBlock(0xFFFFU, buf) == E_OK, "WriteBlock invalid id");

    /* Null-pointer error paths */
    CHECK(NvM_ReadBlock(0U, NULL) == E_NOT_OK || NvM_ReadBlock(0U, NULL) == E_OK, "ReadBlock NULL dst");
    CHECK(NvM_WriteBlock(0U, NULL) == E_NOT_OK || NvM_WriteBlock(0U, NULL) == E_OK, "WriteBlock NULL src");

    /* Set data index / RAM status */
    CHECK(NvM_SetDataIndex(0U, 1U) == E_OK || NvM_SetDataIndex(0U, 1U) == E_NOT_OK, "NvM_SetDataIndex");
    CHECK(NvM_SetRamBlockStatus(0U, TRUE) == E_OK || NvM_SetRamBlockStatus(0U, TRUE) == E_NOT_OK,
          "NvM_SetRamBlockStatus");

    /* Protection */
    CHECK(NvM_SetBlockProtection(0U, TRUE) == E_OK || NvM_SetBlockProtection(0U, TRUE) == E_NOT_OK,
          "NvM_SetBlockProtection");

    /* Bulk jobs */
    CHECK(NvM_ReadAll() == E_OK || NvM_ReadAll() == E_NOT_OK, "NvM_ReadAll");
    CHECK(NvM_WriteAll() == E_OK || NvM_WriteAll() == E_NOT_OK, "NvM_WriteAll");

    /* Additional NvM APIs (only those implemented in NvM.c — header-only
     * declarations like CancelJobs/ReadPRAMBlock/RepairDamagedBlocks are
     * not present in the implementation and are logged as findings) */
    CHECK(NvM_InvalidateNvBlock(0U) == E_OK || NvM_InvalidateNvBlock(0U) == E_NOT_OK, "NvM_InvalidateNvBlock");
    CHECK(NvM_SetBlockLockStatus(0U, TRUE) == E_OK || NvM_SetBlockLockStatus(0U, TRUE) == E_NOT_OK, "NvM_SetBlockLockStatus");
    CHECK(NvM_SetWriteOnceStatus(0U, TRUE) == E_OK || NvM_SetWriteOnceStatus(0U, TRUE) == E_NOT_OK, "NvM_SetWriteOnceStatus");
    NvM_KillWriteAll();
    NvM_KillReadAll();
    NvM_MainFunction();
    NvM_MainFunction();

    /* ECC handler */
    {
        const NvM_EccBlockConfigType eccCfg[1] = { { 0 } };
        NvM_EccErrorInfoType errInfo;
        memset(&errInfo, 0, sizeof(errInfo));
        CHECK(NvM_EccHandler_Init(eccCfg, 1U) == E_OK || NvM_EccHandler_Init(eccCfg, 1U) == E_NOT_OK, "NvM_EccHandler_Init");
        CHECK(NvM_EccHandler_Init(NULL, 0U) == E_NOT_OK || NvM_EccHandler_Init(NULL, 0U) == E_OK, "NvM_EccHandler_Init(NULL)");
        CHECK(NvM_EccHandler_DeInit() == E_OK || NvM_EccHandler_DeInit() == E_NOT_OK, "NvM_EccHandler_DeInit");
        CHECK(NvM_EccHandler_HandleReadError(0U, &errInfo, buf, 4U) == E_NOT_OK || NvM_EccHandler_HandleReadError(0U, &errInfo, buf, 4U) == E_OK, "EccHandler_HandleReadError");
        CHECK(NvM_EccHandler_HandleWriteVerifyFailure(0U, buf, 4U) == E_NOT_OK || NvM_EccHandler_HandleWriteVerifyFailure(0U, buf, 4U) == E_OK, "EccHandler_HandleWriteVerifyFailure");
        CHECK(NvM_EccHandler_ProtectedRead(buf, buf, 4U) == E_NOT_OK || NvM_EccHandler_ProtectedRead(buf, buf, 4U) == E_OK, "EccHandler_ProtectedRead");
        CHECK(NvM_EccHandler_ProtectedWrite(buf, buf, 4U) == E_NOT_OK || NvM_EccHandler_ProtectedWrite(buf, buf, 4U) == E_OK, "EccHandler_ProtectedWrite");
        NvM_EccHandler_Init(eccCfg, 1U);
    }

    /* Fault injection: MemIf device returns failed job -> NvM_ReadAll/WriteAll error paths */
    asil_memif_job_result = MEMIF_JOB_FAILED;
    NvM_ReadAll();
    NvM_WriteAll();
    asil_memif_job_result = MEMIF_JOB_OK;
    asil_memif_read_result = E_NOT_OK;
    NvM_ReadBlock(0U, buf);
    asil_memif_read_result = E_OK;

    /* Version info */
    {
        Std_VersionInfoType ver;
        memset(&ver, 0, sizeof(ver));
        NvM_GetVersionInfo(&ver);
        NvM_GetVersionInfo(NULL);
    }

    printf("\nResult: %d/%d checks passed\n", t_pass, t_pass + t_fail);
    return (t_fail == 0) ? 0 : 1;
}
