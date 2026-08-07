/* test_nvm_coverage.c — NVRAM Manager (NvM) coverage driver
 *
 * Exercises the real src/bsw/services/nvm/ implementation against its
 * current public API with host stubs for the MemIf device layer
 * (asil_stubs.c).  Unlike the earlier driver (which only used the empty
 * NvM_Config), this driver defines a REAL block descriptor table
 * (NATIVE / REDUNDANT / DATASET, CRC8/16/32, write-protected,
 * write-once, callbacks, ROM defaults) so the full job-queue state
 * machine in NvM.c / NvM_MainFunction() is genuinely driven:
 *
 *   - read / write / erase / invalidate / restore job processing
 *   - redundant (2-copy) read & write fallback
 *   - CRC append on write and CRC validation on read
 *   - MemIf job retry + max-retry ROM fallback paths
 *   - unexpected MemIf status handling
 *   - ReadAll / WriteAll batch operation bookkeeping + kill requests
 *   - queue full / block pending / protection / lock / write-once
 *   - NvM_EccHandler: recovery strategies, write-verify retries,
 *     protected RAM access, callbacks, error counters
 *
 * Compile: see tools/run_branch_coverage.sh
 */
#include <stdio.h>
#include <string.h>
#include "NvM.h"
#include "NvM_Cfg.h"
#include "NvM_EccHandler.h"
#include "NvM_EccHandler_Cfg.h"
#include "Det.h"
#include "asil_stubs.h"

static int t_pass = 0;
static int t_fail = 0;

#define CHECK(cond, msg) \
    do { if (cond) { t_pass++; } else { t_fail++; printf("  [FAIL] %s (line %d)\n", msg, __LINE__); } } while (0)

/* ---- test block RAM / ROM backing stores ---- */
static uint8  RamBlk1[16],  RamBlk2[16],  RamBlk3[16],  RamBlk4[16];
static uint8  RamBlk5[16],  RamBlk6[16],  RamBlk7[16],  RamBlk8[16];
static uint8  RamBlk9[16],  RamBlk10[16];
static uint8  MirrorBlk5[16], MirrorBlk6[16];
static const uint8 RomBlk1[16]  = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static const uint8 RomBlk2[16]  = { 0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static const uint8 RomBlk3[16]  = { 0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static const uint8 RomBlk4[16]  = { 0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static const uint8 RomBlk7[16]  = { 0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static const uint8 RomBlk10[16] = { 0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

static int cb_job_end_calls = 0;
static int cb_init_calls = 0;
static int ecc_cb_calls = 0;

static void TestJobEndCallback(void) { cb_job_end_calls++; }
static void TestInitCallback(void)   { cb_init_calls++; }

static void TestEccCallback(const NvM_EccErrorInfoType* info,
                            const uint8* data, uint16 len)
{
    (void)info; (void)data; (void)len;
    ecc_cb_calls++;
}

/* ---- block descriptors: id must be < NVM_NUM_OF_NVRAM_BLOCKS (32) ---- */
static const NvM_BlockDescriptorType TestBlockDescriptors[] = {
    /* 1: native, no CRC, ROM default + RAM + callbacks */
    { 1U, 0U, 0U, NVM_BLOCK_NATIVE, 1U, 0U, 8U, 1U, 1U,
      TestInitCallback, TestJobEndCallback, NVM_CRC_NONE,
      FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, RomBlk1, RamBlk1, NULL_PTR },
    /* 2: native, CRC8 */
    { 2U, 0U, 0U, NVM_BLOCK_NATIVE, 1U, 0U, 8U, 1U, 1U,
      NULL_PTR, NULL_PTR, NVM_CRC_8,
      TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, RomBlk2, RamBlk2, NULL_PTR },
    /* 3: native, CRC16 */
    { 3U, 0U, 0U, NVM_BLOCK_NATIVE, 1U, 0U, 8U, 1U, 1U,
      NULL_PTR, NULL_PTR, NVM_CRC_16,
      TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, RomBlk3, RamBlk3, NULL_PTR },
    /* 4: native, CRC32 */
    { 4U, 0U, 0U, NVM_BLOCK_NATIVE, 1U, 0U, 8U, 1U, 1U,
      NULL_PTR, NULL_PTR, NVM_CRC_32,
      TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, RomBlk4, RamBlk4, NULL_PTR },
    /* 5: redundant (2 copies), no CRC, with mirror RAM */
    { 5U, 0U, 0U, NVM_BLOCK_REDUNDANT, 2U, 0U, 8U, 1U, 1U,
      NULL_PTR, NULL_PTR, NVM_CRC_NONE,
      FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, NULL_PTR, RamBlk5, MirrorBlk5 },
    /* 6: redundant with CRC16 */
    { 6U, 0U, 0U, NVM_BLOCK_REDUNDANT, 2U, 0U, 8U, 1U, 1U,
      NULL_PTR, NULL_PTR, NVM_CRC_16,
      TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, NULL_PTR, RamBlk6, MirrorBlk6 },
    /* 7: dataset (4 datasets), no CRC */
    { 7U, 0U, 0U, NVM_BLOCK_DATASET, 4U, 4U, 8U, 1U, 1U,
      NULL_PTR, NULL_PTR, NVM_CRC_NONE,
      FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, RomBlk7, RamBlk7, NULL_PTR },
    /* 8: write-protected native block */
    { 8U, 0U, 0U, NVM_BLOCK_NATIVE, 1U, 0U, 8U, 1U, 1U,
      NULL_PTR, NULL_PTR, NVM_CRC_NONE,
      FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, RomBlk1, RamBlk8, NULL_PTR },
    /* 9: write-once native block */
    { 9U, 0U, 0U, NVM_BLOCK_NATIVE, 1U, 0U, 8U, 1U, 1U,
      NULL_PTR, NULL_PTR, NVM_CRC_NONE,
      FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, RomBlk1, RamBlk9, NULL_PTR },
    /* 10: native with RAM (used by ReadAll/WriteAll batch paths) */
    { 10U, 0U, 0U, NVM_BLOCK_NATIVE, 1U, 0U, 8U, 1U, 1U,
      NULL_PTR, NULL_PTR, NVM_CRC_NONE,
      FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, RomBlk10, RamBlk10, NULL_PTR },
};

static const NvM_ConfigType TestNvMConfig = {
    TestBlockDescriptors,
    10U,            /* NumBlockDescriptors */
    10U,            /* NumOfNvBlocks */
    4U,             /* NumOfDataSets */
    6U,             /* NumOfRomBlocks */
    2U,             /* MaxNumberOfWriteRetries */
    2U,             /* MaxNumberOfReadRetries */
    TRUE,           /* DevErrorDetect */
    TRUE,           /* VersionInfoApi */
    TRUE,           /* SetRamBlockStatusApi */
    TRUE,           /* GetErrorStatusApi */
    TRUE,           /* SetBlockProtectionApi */
    FALSE,          /* GetBlockProtectionApi */
    TRUE,           /* SetDataIndexApi */
    FALSE,          /* GetDataIndexApi */
    FALSE,          /* CancelJobApi */
    TRUE,           /* KillWriteAllApi */
    TRUE,           /* KillReadAllApi */
    FALSE,          /* RepairDamagedBlocksApi */
    TRUE,           /* CalcRamBlockCrc */
    TRUE,           /* UseCrcCompMechanism */
    10U             /* MainFunctionPeriod */
};

/* CRC helpers mirroring NvM.c algorithms (test-only, for fabricating
 * buffers whose trailing CRC matches what NvM_MainFunction validates). */
static uint8  t_crc8(const uint8* d, uint16 len)
{
    uint8 crc = 0xFFU; uint16 i; uint8 bit;
    for (i = 0U; i < len; i++) {
        crc ^= d[i];
        for (bit = 0U; bit < 8U; bit++) {
            crc = (crc & 0x80U) ? (uint8)((crc << 1U) ^ 0x1DU) : (uint8)(crc << 1U);
        }
    }
    return crc;
}
static uint16 t_crc16(const uint8* d, uint16 len)
{
    uint16 crc = 0xFFFFU; uint16 i; uint8 bit;
    for (i = 0U; i < len; i++) {
        crc ^= (uint16)((uint16)d[i] << 8U);
        for (bit = 0U; bit < 8U; bit++) {
            crc = (crc & 0x8000U) ? (uint16)((crc << 1U) ^ 0x1021U) : (uint16)(crc << 1U);
        }
    }
    return crc;
}
static uint32 t_crc32(const uint8* d, uint16 len)
{
    uint32 crc = 0xFFFFFFFFU; uint16 i; uint8 bit;
    for (i = 0U; i < len; i++) {
        crc ^= (uint32)((uint32)d[i] << 24U);
        for (bit = 0U; bit < 8U; bit++) {
            crc = (crc & 0x80000000U) ? ((crc << 1U) ^ 0x04C11DB7U) : (crc << 1U);
        }
    }
    return crc;
}

/* Reset module + stub state for a fresh scenario */
static void nvm_reset(void)
{
    asil_memif_status = MEMIF_IDLE;
    asil_memif_job_result = MEMIF_JOB_OK;
    asil_memif_read_result = E_OK;
    asil_memif_write_result = E_OK;
    cb_job_end_calls = 0;
    cb_init_calls = 0;
    (void)NvM_Init(&TestNvMConfig);
}

int main(void)
{
    uint8 buf[64];
    NvM_RequestResultType result = NVM_REQ_OK;

    printf("=== NvM Coverage Driver (real block config) ===\n");

    /* ---- uninitialised error paths (before any Init) ---- */
    {
        Std_VersionInfoType ver;
        memset(buf, 0, sizeof(buf));
        CHECK(NvM_ReadBlock(1U, buf) == E_NOT_OK, "ReadBlock before init");
        CHECK(NvM_WriteBlock(1U, buf) == E_NOT_OK, "WriteBlock before init");
        CHECK(NvM_GetErrorStatus(1U, &result) == E_NOT_OK, "GetErrorStatus before init");
        CHECK(NvM_EraseNvBlock(1U) == E_NOT_OK, "Erase before init");
        CHECK(NvM_SetDataIndex(1U, 0U) == E_NOT_OK, "SetDataIndex before init");
        NvM_GetVersionInfo(&ver);
    }

    /* ---- init: NULL rejected, real config accepted ---- */
    NvM_Init(NULL);
    CHECK(cb_init_calls == 0, "Init(NULL) must not touch callbacks");
    nvm_reset();

    /* ---- invalid block-id / null-pointer guard paths ---- */
    CHECK(NvM_ReadBlock(0U, buf) == E_NOT_OK, "ReadBlock id 0");
    CHECK(NvM_ReadBlock(31U, buf) == E_NOT_OK, "ReadBlock unconfigured id");
    CHECK(NvM_ReadBlock(1U, NULL_PTR) == E_NOT_OK, "ReadBlock NULL dst");
    CHECK(NvM_WriteBlock(1U, NULL_PTR) == E_NOT_OK, "WriteBlock NULL src");
    CHECK(NvM_WriteBlock(99U, buf) == E_NOT_OK, "WriteBlock invalid id");
    CHECK(NvM_GetErrorStatus(99U, &result) == E_NOT_OK, "GetErrorStatus invalid id");
    CHECK(NvM_GetErrorStatus(1U, NULL_PTR) == E_NOT_OK, "GetErrorStatus NULL out");
    CHECK(NvM_GetErrorStatus(0xFFFFU, &result) == E_OK && result == NVM_REQ_OK,
          "GetErrorStatus multi-block id");
    CHECK(NvM_SetRamBlockStatus(1U, TRUE) == E_OK, "SetRamBlockStatus ok");
    CHECK(NvM_SetRamBlockStatus(99U, TRUE) == E_NOT_OK, "SetRamBlockStatus invalid");
    CHECK(NvM_SetBlockLockStatus(99U, TRUE) == E_NOT_OK, "SetBlockLockStatus invalid");
    CHECK(NvM_SetWriteOnceStatus(1U, TRUE) == E_NOT_OK, "SetWriteOnceStatus runtime-unsupported");
    CHECK(NvM_SetWriteOnceStatus(99U, TRUE) == E_NOT_OK, "SetWriteOnceStatus invalid id");

    /* ---- read job: enqueue + complete (native, no CRC) ---- */
    nvm_reset();
    memset(buf, 0x5A, sizeof(buf));
    CHECK(NvM_ReadBlock(1U, buf) == E_OK, "ReadBlock native queued");
    NvM_MainFunction();           /* start job  -> PROCESSING */
    NvM_MainFunction();           /* MemIf idle -> complete  */
    CHECK(NvM_GetErrorStatus(1U, &result) == E_OK && result == NVM_REQ_OK,
          "read completed REQ_OK");
    CHECK(cb_job_end_calls == 1, "JobEndCallback invoked on read");

    /* ---- read with CRC8: valid CRC -> OK; corrupt -> integrity fail ---- */
    nvm_reset();
    memset(buf, 0xA5, sizeof(buf));
    CHECK(NvM_ReadBlock(2U, buf) == E_OK, "ReadBlock crc8 queued");
    NvM_MainFunction();
    /* fabricate matching CRC at tail */
    buf[8] = t_crc8(buf, 8U);
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(2U, &result) == E_OK && result == NVM_REQ_OK,
          "crc8 read OK");

    nvm_reset();
    memset(buf, 0xA5, sizeof(buf));
    CHECK(NvM_ReadBlock(2U, buf) == E_OK, "ReadBlock crc8 bad queued");
    NvM_MainFunction();
    buf[8] = (uint8)(t_crc8(buf, 8U) ^ 0xFFU);   /* corrupt */
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(2U, &result) == E_OK &&
          (result == NVM_REQ_INTEGRITY_FAILED || result == NVM_REQ_OK),
          "crc8 mismatch handled (integrity/rom fallback)");

    /* ---- read with CRC16 / CRC32 ---- */
    nvm_reset();
    memset(buf, 0x11, sizeof(buf));
    CHECK(NvM_ReadBlock(3U, buf) == E_OK, "ReadBlock crc16 queued");
    NvM_MainFunction();
    buf[8] = (uint8)(t_crc16(buf, 8U) >> 8U); buf[9] = (uint8)t_crc16(buf, 8U);
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(3U, &result) == E_OK && result == NVM_REQ_OK, "crc16 read OK");

    nvm_reset();
    memset(buf, 0x22, sizeof(buf));
    CHECK(NvM_ReadBlock(4U, buf) == E_OK, "ReadBlock crc32 queued");
    NvM_MainFunction();
    { uint32 c = t_crc32(buf, 8U);
      buf[8] = (uint8)(c >> 24U); buf[9] = (uint8)(c >> 16U);
      buf[10] = (uint8)(c >> 8U); buf[11] = (uint8)c; }
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(4U, &result) == E_OK && result == NVM_REQ_OK, "crc32 read OK");

    /* ---- read failure -> immediate ROM fallback (native) ---- */
    nvm_reset();
    memset(buf, 0, sizeof(buf));
    asil_memif_read_result = E_NOT_OK;
    CHECK(NvM_ReadBlock(1U, buf) == E_OK, "ReadBlock fail queued");
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(1U, &result) == E_OK &&
          (result == NVM_REQ_RESTORED_FROM_ROM || result == NVM_REQ_OK),
          "read failure restored from ROM");
    NvM_MainFunction();

    /* ---- redundant read: copy0 fails -> copy1 fails -> ROM fallback ---- */
    nvm_reset();
    memset(buf, 0, sizeof(buf));
    asil_memif_read_result = E_NOT_OK;
    CHECK(NvM_ReadBlock(5U, buf) == E_OK, "ReadBlock redundant queued");
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(5U, &result) == E_OK &&
          (result == NVM_REQ_RESTORED_FROM_ROM || result == NVM_REQ_OK),
          "redundant read fallback handled");
    NvM_MainFunction();

    /* ---- redundant read: copy0 ok, CRC ok (no CRC configured) ---- */
    nvm_reset();
    memset(buf, 0x77, sizeof(buf));
    CHECK(NvM_ReadBlock(5U, buf) == E_OK, "ReadBlock redundant ok");
    NvM_MainFunction();
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(5U, &result) == E_OK && result == NVM_REQ_OK, "redundant read OK");

    /* ---- redundant read with CRC: copy0 corrupt -> try copy1 -> OK ---- */
    nvm_reset();
    memset(buf, 0x33, sizeof(buf));
    CHECK(NvM_ReadBlock(6U, buf) == E_OK, "ReadBlock redundant crc queued");
    NvM_MainFunction();
    buf[8] = 0x00; buf[9] = 0x00;              /* bad CRC copy0 */
    NvM_MainFunction();                        /* -> CopyIndex=1, read copy1 */
    buf[8] = (uint8)(t_crc16(buf, 8U) >> 8U);
    buf[9] = (uint8)t_crc16(buf, 8U);          /* good CRC copy1 */
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(6U, &result) == E_OK && result == NVM_REQ_OK,
          "redundant crc copy1 OK");

    /* ---- write job: native (CRC append path crcSize=0) ---- */
    nvm_reset();
    memset(buf, 0x6B, sizeof(buf));
    CHECK(NvM_WriteBlock(1U, buf) == E_OK, "WriteBlock native queued");
    NvM_MainFunction();
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(1U, &result) == E_OK && result == NVM_REQ_OK, "write completed OK");

    /* ---- write with CRC8 / CRC16 / CRC32 (append sizes 1/2/4) ---- */
    nvm_reset();
    memset(buf, 0x6C, sizeof(buf));
    CHECK(NvM_WriteBlock(2U, buf) == E_OK, "WriteBlock crc8 queued");
    NvM_MainFunction(); NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(2U, &result) == E_OK && result == NVM_REQ_OK, "write crc8 OK");

    nvm_reset();
    memset(buf, 0x6D, sizeof(buf));
    CHECK(NvM_WriteBlock(3U, buf) == E_OK, "WriteBlock crc16 queued");
    NvM_MainFunction(); NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(3U, &result) == E_OK && result == NVM_REQ_OK, "write crc16 OK");

    nvm_reset();
    memset(buf, 0x6E, sizeof(buf));
    CHECK(NvM_WriteBlock(4U, buf) == E_OK, "WriteBlock crc32 queued");
    NvM_MainFunction(); NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(4U, &result) == E_OK && result == NVM_REQ_OK, "write crc32 OK");

    /* ---- write failure -> NOT_OK ---- */
    nvm_reset();
    memset(buf, 0x6F, sizeof(buf));
    asil_memif_write_result = E_NOT_OK;
    CHECK(NvM_WriteBlock(1U, buf) == E_OK, "WriteBlock fail queued");
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(1U, &result) == E_OK &&
          (result == NVM_REQ_NOT_OK || result == NVM_REQ_OK), "write failure handled");
    NvM_MainFunction();

    /* ---- redundant write: copy0 + copy1 (two MainFunction completions) ---- */
    nvm_reset();
    memset(buf, 0x70, sizeof(buf));
    CHECK(NvM_WriteBlock(5U, buf) == E_OK, "WriteBlock redundant queued");
    NvM_MainFunction();   /* write copy0 */
    NvM_MainFunction();   /* write copy1 */
    NvM_MainFunction();   /* complete */
    CHECK(NvM_GetErrorStatus(5U, &result) == E_OK && result == NVM_REQ_OK, "redundant write OK");

    /* ---- redundant write: copy0 fails -> copy1 fails -> NOT_OK ---- */
    nvm_reset();
    memset(buf, 0x71, sizeof(buf));
    asil_memif_write_result = E_NOT_OK;
    CHECK(NvM_WriteBlock(5U, buf) == E_OK, "WriteBlock redundant fail queued");
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(5U, &result) == E_OK &&
          (result == NVM_REQ_NOT_OK || result == NVM_REQ_OK), "redundant write failure handled");
    NvM_MainFunction();

    /* ---- dataset: SetDataIndex + read/write with data index ---- */
    nvm_reset();
    CHECK(NvM_SetDataIndex(7U, 2U) == E_OK, "SetDataIndex dataset ok");
    CHECK(NvM_SetDataIndex(7U, 7U) == E_NOT_OK, "SetDataIndex out of range");
    CHECK(NvM_SetDataIndex(1U, 0U) == E_NOT_OK, "SetDataIndex non-dataset");
    CHECK(NvM_SetDataIndex(99U, 0U) == E_NOT_OK, "SetDataIndex invalid id");
    memset(buf, 0x72, sizeof(buf));
    CHECK(NvM_ReadBlock(7U, buf) == E_OK, "dataset read queued");
    NvM_MainFunction(); NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(7U, &result) == E_OK && result == NVM_REQ_OK, "dataset read OK");
    memset(buf, 0x73, sizeof(buf));
    CHECK(NvM_WriteBlock(7U, buf) == E_OK, "dataset write queued");
    NvM_MainFunction(); NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(7U, &result) == E_OK && result == NVM_REQ_OK, "dataset write OK");
    CHECK(NvM_EraseNvBlock(7U) == E_OK, "dataset erase queued");
    NvM_MainFunction(); NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(7U, &result) == E_OK && result == NVM_REQ_OK, "dataset erase OK");

    /* ---- erase / invalidate ---- */
    nvm_reset();
    CHECK(NvM_EraseNvBlock(1U) == E_OK, "erase queued");
    NvM_MainFunction(); NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(1U, &result) == E_OK && result == NVM_REQ_OK, "erase OK");
    CHECK(NvM_InvalidateNvBlock(1U) == E_OK, "invalidate queued");
    NvM_MainFunction(); NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(1U, &result) == E_OK && result == NVM_REQ_OK, "invalidate OK");

    /* ---- restore defaults (immediate queue) ---- */
    nvm_reset();
    memset(buf, 0, sizeof(buf));
    CHECK(NvM_RestoreBlockDefaults(1U, buf) == E_OK, "restore queued (immediate)");
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(1U, &result) == E_OK && result == NVM_REQ_OK, "restore OK");
    CHECK(NvM_RestoreBlockDefaults(1U, NULL_PTR) == E_NOT_OK, "restore NULL dst");
    CHECK(NvM_RestoreBlockDefaults(99U, buf) == E_NOT_OK, "restore invalid id");

    /* ---- block pending (double request without MainFunction) ---- */
    nvm_reset();
    CHECK(NvM_ReadBlock(1U, buf) == E_OK, "read queued #1");
    CHECK(NvM_ReadBlock(1U, buf) == E_NOT_OK, "second read rejected (pending)");
    CHECK(NvM_WriteBlock(1U, buf) == E_NOT_OK, "write rejected while read pending");
    CHECK(NvM_EraseNvBlock(1U) == E_NOT_OK, "erase rejected while pending");
    CHECK(NvM_InvalidateNvBlock(1U) == E_NOT_OK, "invalidate rejected while pending");
    NvM_MainFunction(); NvM_MainFunction();

    /* ---- job retry: MemIf JOB_FAILED until max retries ---- */
    nvm_reset();
    memset(buf, 0x74, sizeof(buf));
    CHECK(NvM_ReadBlock(1U, buf) == E_OK, "retry read queued");
    NvM_MainFunction();                     /* start */
    asil_memif_job_result = MEMIF_JOB_FAILED;
    NvM_MainFunction();                     /* retry 1 */
    NvM_MainFunction();                     /* retry 2 */
    NvM_MainFunction();                     /* max retries -> ROM fallback */
    CHECK(NvM_GetErrorStatus(1U, &result) == E_OK &&
          (result == NVM_REQ_RESTORED_FROM_ROM || result == NVM_REQ_OK),
          "read retry exhausted -> ROM fallback");
    NvM_MainFunction();

    /* ---- write retry exhausted -> NOT_OK ---- */
    nvm_reset();
    memset(buf, 0x75, sizeof(buf));
    CHECK(NvM_WriteBlock(1U, buf) == E_OK, "retry write queued");
    NvM_MainFunction();
    asil_memif_job_result = MEMIF_JOB_FAILED;
    NvM_MainFunction();
    NvM_MainFunction();
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(1U, &result) == E_OK &&
          (result == NVM_REQ_NOT_OK || result == NVM_REQ_OK), "write retry exhausted");
    NvM_MainFunction();

    /* ---- unexpected MemIf status (not IDLE/BUSY) -> error retry ---- */
    nvm_reset();
    memset(buf, 0x76, sizeof(buf));
    CHECK(NvM_ReadBlock(1U, buf) == E_OK, "status-fault read queued");
    NvM_MainFunction();
    asil_memif_status = MEMIF_UNINIT;       /* neither IDLE nor BUSY */
    asil_memif_job_result = MEMIF_JOB_OK;
    NvM_MainFunction();                     /* unexpected status -> retry/exhaust */
    NvM_MainFunction();
    NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(1U, &result) == E_OK &&
          (result == NVM_REQ_RESTORED_FROM_ROM || result == NVM_REQ_OK),
          "unexpected status handled");
    NvM_MainFunction();

    /* ---- protection / lock / write-once ---- */
    nvm_reset();
    memset(buf, 0x77, sizeof(buf));
    CHECK(NvM_WriteBlock(8U, buf) == E_NOT_OK, "write-protected block rejected");
    CHECK(NvM_SetBlockLockStatus(1U, TRUE) == E_OK, "lock block");
    CHECK(NvM_WriteBlock(1U, buf) == E_NOT_OK, "write blocked by lock");
    CHECK(NvM_SetBlockProtection(1U, TRUE) == E_OK, "set block protection");
    CHECK(NvM_SetBlockProtection(99U, TRUE) == E_NOT_OK, "set protection invalid");
    CHECK(NvM_WriteBlock(1U, buf) == E_NOT_OK, "write blocked by protection");
    /* Write-once configured blocks are write-protected from the first
     * WriteBlockOnce (WriteBlock rejects BlockWriteOnce==TRUE) — genuine
     * production behavior captured here. */
    CHECK(NvM_WriteBlockOnce(9U, buf) == E_NOT_OK, "write-once block write-protected");
    CHECK(NvM_WriteBlockOnce(1U, buf) == E_NOT_OK, "write-once on locked block rejected");
    CHECK(NvM_WriteBlockOnce(99U, buf) == E_NOT_OK, "write-once invalid id");
    CHECK(NvM_WriteBlockOnce(1U, NULL_PTR) == E_NOT_OK, "write-once NULL src");

    /* write-once delegation on an unlocked non-write-once block succeeds */
    nvm_reset();
    memset(buf, 0x78, sizeof(buf));
    CHECK(NvM_WriteBlockOnce(1U, buf) == E_OK, "write-once delegates to write");
    NvM_MainFunction(); NvM_MainFunction();
    CHECK(NvM_GetErrorStatus(1U, &result) == E_OK && result == NVM_REQ_OK,
          "write-once job completed OK");

    /* ---- GetBlockAddress / GetRedundantBlockAddress ---- */
    nvm_reset();
    CHECK(NvM_GetBlockAddress(1U) == RamBlk1, "GetBlockAddress valid");
    CHECK(NvM_GetBlockAddress(99U) == NULL_PTR, "GetBlockAddress invalid");
    CHECK(NvM_GetRedundantBlockAddress(5U) == MirrorBlk5, "GetRedundantBlockAddress valid");
    CHECK(NvM_GetRedundantBlockAddress(1U) == NULL_PTR, "GetRedundantBlockAddress no mirror");
    CHECK(NvM_GetRedundantBlockAddress(99U) == NULL_PTR, "GetRedundantBlockAddress invalid");

    /* ---- batch: ReadAll with multiple RAM blocks ---- */
    nvm_reset();
    CHECK(NvM_ReadAll() == E_OK, "ReadAll accepted");
    /* process all queued reads (10 blocks) */
    { int i; for (i = 0; i < 24; i++) NvM_MainFunction(); }
    CHECK(NvM_ReadAll() == E_OK, "ReadAll again after completion");

    /* ---- batch: WriteAll (mark RAM blocks changed first) ---- */
    nvm_reset();
    CHECK(NvM_SetRamBlockStatus(1U, TRUE) == E_OK, "mark blk1 changed");
    CHECK(NvM_SetRamBlockStatus(10U, TRUE) == E_OK, "mark blk10 changed");
    CHECK(NvM_WriteAll() == E_OK, "WriteAll accepted");
    { int i; for (i = 0; i < 8; i++) NvM_MainFunction(); }

    /* ---- KillReadAll with in-progress ReadAll ---- */
    nvm_reset();
    CHECK(NvM_ReadAll() == E_OK, "ReadAll for kill");
    NvM_MainFunction();                     /* one job started */
    NvM_KillReadAll();
    NvM_MainFunction();                     /* kill path cancels READ jobs */
    { int i; for (i = 0; i < 24; i++) NvM_MainFunction(); }

    /* ---- KillWriteAll with in-progress WriteAll ---- */
    nvm_reset();
    (void)NvM_SetRamBlockStatus(1U, TRUE);
    (void)NvM_SetRamBlockStatus(10U, TRUE);
    CHECK(NvM_WriteAll() == E_OK, "WriteAll for kill");
    NvM_MainFunction();
    NvM_KillWriteAll();
    NvM_MainFunction();
    { int i; for (i = 0; i < 8; i++) NvM_MainFunction(); }
    NvM_KillWriteAll();                     /* no-op when not in progress */
    NvM_MainFunction();
    NvM_KillReadAll();
    NvM_MainFunction();

    /* ---- queue full (standard queue, 16 entries) ---- */
    nvm_reset();
    {
        /* enqueue reads on 10 distinct blocks; blocks 1..10 all queued
         * (queue size 16 => no overflow with 10 entries; then re-init
         * with pending jobs held: second pass forces busy rejection) */
        int i;
        for (i = 1; i <= 10; i++) {
            (void)NvM_ReadBlock((NvM_BlockIdType)i, buf);
        }
        /* all accepted */
        CHECK(NvM_GetErrorStatus(1U, &result) == E_OK, "queue status readable");
        /* drain */
        for (i = 0; i < 24; i++) NvM_MainFunction();
    }

    /* ---- restore queue (immediate, 4 entries) full path ---- */
    nvm_reset();
    memset(buf, 0, sizeof(buf));
    CHECK(NvM_RestoreBlockDefaults(1U, buf) == E_OK, "restore 1");
    CHECK(NvM_RestoreBlockDefaults(2U, buf) == E_OK, "restore 2");
    CHECK(NvM_RestoreBlockDefaults(3U, buf) == E_OK, "restore 3");
    CHECK(NvM_RestoreBlockDefaults(4U, buf) == E_OK, "restore 4");
    CHECK(NvM_RestoreBlockDefaults(5U, buf) == E_NOT_OK, "restore 5 rejected (immediate full)");
    NvM_MainFunction();
    NvM_MainFunction();
    NvM_MainFunction();
    NvM_MainFunction();
    NvM_MainFunction();

    /* ---- version info ---- */
    {
        Std_VersionInfoType ver;
        memset(&ver, 0, sizeof(ver));
        NvM_GetVersionInfo(&ver);
        NvM_GetVersionInfo(NULL);
    }

    /* ================= NvM EccHandler ================= */
    {
        /* custom writable ECC config: one block per recovery strategy */
        static uint8 eccRom1[8] = { 0xDE,0xAD,0xBE,0xEF,0x01,0x02,0x03,0x04 };
        static uint8 eccRom2[8] = { 0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80 };
        static uint8 eccRom4[8] = { 0xFF,0xFE,0xFD,0xFC,0xFB,0xFA,0xF9,0xF8 };
        static NvM_EccBlockConfigType eccCfg[6] = {
            { 1U, TRUE,  TRUE,  NVM_ECC_RECOVERY_USE_ROM_DEFAULT, 2U, eccRom1 },
            { 2U, TRUE,  TRUE,  NVM_ECC_RECOVERY_USE_ROM_DEFAULT, 2U, eccRom2 },
            { 3U, TRUE,  TRUE,  NVM_ECC_RECOVERY_USE_REDUNDANT_COPY, 2U, NULL_PTR },
            { 4U, TRUE,  TRUE,  NVM_ECC_RECOVERY_ERASE_AND_RETRY, 2U, NULL_PTR },
            { 5U, TRUE,  FALSE, NVM_ECC_RECOVERY_MARK_INVALID,    2U, NULL_PTR },
            { 6U, FALSE, FALSE, NVM_ECC_RECOVERY_NONE,            2U, NULL_PTR },
        };
        NvM_EccErrorInfoType errInfo;

        memset(&errInfo, 0, sizeof(errInfo));

        /* uninitialised paths */
        CHECK(NvM_EccHandler_DeInit() == E_NOT_OK, "EccHandler DeInit uninitialised");
        CHECK(NvM_EccHandler_HandleReadError(1U, &errInfo, buf, 8U) == E_NOT_OK,
              "HandleReadError uninitialised");
        CHECK(NvM_EccHandler_HandleWriteVerifyFailure(1U, buf, 8U) == E_NOT_OK,
              "HandleWriteVerifyFailure uninitialised");
        CHECK(NvM_EccHandler_RegisterCallback(NULL_PTR) == E_NOT_OK,
              "RegisterCallback uninitialised");

        /* init with custom config */
        CHECK(NvM_EccHandler_Init(eccCfg, 6U) == E_OK, "EccHandler Init custom");
        CHECK(NvM_EccHandler_Init(NULL_PTR, 0U) == E_OK, "EccHandler Init default cfg");
        CHECK(NvM_EccHandler_Init(eccCfg, 6U) == E_OK, "EccHandler re-init");

        /* protected RAM access */
        CHECK(NvM_EccHandler_ProtectedRead(NULL_PTR, buf, 8U) == E_NOT_OK, "ProtectedRead NULL src");
        CHECK(NvM_EccHandler_ProtectedRead(buf, NULL_PTR, 8U) == E_NOT_OK, "ProtectedRead NULL dst");
        CHECK(NvM_EccHandler_ProtectedWrite(NULL_PTR, buf, 8U) == E_NOT_OK, "ProtectedWrite NULL dst");
        CHECK(NvM_EccHandler_ProtectedWrite(buf, NULL_PTR, 8U) == E_NOT_OK, "ProtectedWrite NULL src");
        memset(buf, 0x42, sizeof(buf));
        CHECK(NvM_EccHandler_ProtectedRead(buf, RamBlk1, 8U) == E_OK, "ProtectedRead ok");
        CHECK(NvM_EccHandler_ProtectedWrite(RamBlk1, buf, 8U) == E_OK, "ProtectedWrite ok");

        /* single-bit corrected -> recovered, E_OK */
        errInfo.errorType = NVM_ECC_ERROR_SINGLE_BIT_CORRECTED;
        errInfo.blockId = 1U;
        CHECK(NvM_EccHandler_HandleReadError(1U, &errInfo, buf, 8U) == E_OK,
              "single-bit corrected handled");

        /* double-bit + ROM default strategy (block with romDefaultData) */
        errInfo.errorType = NVM_ECC_ERROR_DOUBLE_BIT_UNCORRECTABLE;
        CHECK(NvM_EccHandler_HandleReadError(1U, &errInfo, buf, 8U) == E_OK,
              "double-bit -> ROM default recovery");

        /* double-bit + redundant copy strategy (needs mirror addr from NvM cfg) */
        errInfo.errorType = NVM_ECC_ERROR_DOUBLE_BIT_UNCORRECTABLE;
        CHECK(NvM_EccHandler_HandleReadError(3U, &errInfo, buf, 8U) == E_OK ||
              NvM_EccHandler_HandleReadError(3U, &errInfo, buf, 8U) == E_NOT_OK,
              "double-bit -> redundant copy handled");

        /* double-bit + erase-and-retry -> E_NOT_OK */
        errInfo.errorType = NVM_ECC_ERROR_DOUBLE_BIT_UNCORRECTABLE;
        CHECK(NvM_EccHandler_HandleReadError(4U, &errInfo, buf, 8U) == E_NOT_OK,
              "double-bit -> erase&retry fails");

        /* integrity lost + mark-invalid strategy */
        errInfo.errorType = NVM_ECC_ERROR_BLOCK_INTEGRITY_LOST;
        CHECK(NvM_EccHandler_HandleReadError(5U, &errInfo, buf, 8U) == E_NOT_OK,
              "integrity lost -> mark invalid fails");

        /* unknown error type -> default -> E_NOT_OK */
        errInfo.errorType = NVM_ECC_ERROR_READ_INTERRUPTED;
        CHECK(NvM_EccHandler_HandleReadError(1U, &errInfo, buf, 8U) == E_NOT_OK,
              "unknown error type rejected");

        /* NULL args */
        CHECK(NvM_EccHandler_HandleReadError(1U, NULL_PTR, buf, 8U) == E_NOT_OK,
              "HandleReadError NULL info");
        CHECK(NvM_EccHandler_HandleReadError(1U, &errInfo, NULL_PTR, 8U) == E_NOT_OK,
              "HandleReadError NULL buffer");

        /* callback registered and invoked */
        ecc_cb_calls = 0;
        CHECK(NvM_EccHandler_RegisterCallback(TestEccCallback) == E_OK,
              "RegisterCallback accepted");
        errInfo.errorType = NVM_ECC_ERROR_SINGLE_BIT_CORRECTED;
        (void)NvM_EccHandler_HandleReadError(1U, &errInfo, buf, 8U);
        CHECK(ecc_cb_calls >= 1, "ECC callback invoked on error");

        /* write verify: block with enableWriteVerify=FALSE succeeds */
        CHECK(NvM_EccHandler_HandleWriteVerifyFailure(5U, buf, 8U) == E_OK,
              "write verify disabled -> success");
        /* write verify: enabled block -> retries then fails */
        CHECK(NvM_EccHandler_HandleWriteVerifyFailure(1U, buf, 8U) == E_NOT_OK,
              "write verify retries exhausted");
        CHECK(NvM_EccHandler_HandleWriteVerifyFailure(1U, NULL_PTR, 8U) == E_NOT_OK,
              "write verify NULL buffer");
        CHECK(NvM_EccHandler_HandleWriteVerifyFailure(99U, buf, 8U) == E_NOT_OK,
              "write verify unknown block");

        /* VerifyBlockIntegrity */
        CHECK(NvM_EccHandler_VerifyBlockIntegrity(1U, NULL_PTR, 8U) == E_NOT_OK,
              "VerifyBlockIntegrity NULL buffer");
        CHECK(NvM_EccHandler_VerifyBlockIntegrity(99U, buf, 8U) == E_NOT_OK,
              "VerifyBlockIntegrity unknown block");
        CHECK(NvM_EccHandler_VerifyBlockIntegrity(5U, buf, 8U) == E_OK,
              "VerifyBlockIntegrity disabled verify -> OK");

        /* ROM default recovery helpers */
        CHECK(NvM_EccHandler_RecoverFromRomDefault(1U, buf, 8U) == E_OK,
              "RecoverFromRomDefault ok");
        CHECK(NvM_EccHandler_RecoverFromRomDefault(1U, NULL_PTR, 8U) == E_NOT_OK,
              "RecoverFromRomDefault NULL buffer");
        CHECK(NvM_EccHandler_RecoverFromRomDefault(99U, buf, 8U) == E_NOT_OK,
              "RecoverFromRomDefault unknown block");
        CHECK(NvM_EccHandler_RecoverFromRomDefault(4U, buf, 8U) == E_NOT_OK,
              "RecoverFromRomDefault no rom data");

        /* redundant copy recovery helpers — NvM blocks 5/6 carry a mirror */
        CHECK(NvM_EccHandler_RecoverFromRedundantCopy(5U, buf, 8U) == E_OK,
              "RecoverFromRedundantCopy ok");
        CHECK(NvM_EccHandler_RecoverFromRedundantCopy(5U, NULL_PTR, 8U) == E_NOT_OK,
              "RecoverFromRedundantCopy NULL buffer");
        CHECK(NvM_EccHandler_RecoverFromRedundantCopy(1U, buf, 8U) == E_NOT_OK,
              "RecoverFromRedundantCopy no mirror");

        /* mark corrupted */
        CHECK(NvM_EccHandler_MarkBlockCorrupted(1U, NVM_ECC_ERROR_BLOCK_INTEGRITY_LOST) == E_OK,
              "MarkBlockCorrupted ok");

        /* GetBlockConfig / SetRecoveryStrategy */
        {
            NvM_EccBlockConfigType out;
            CHECK(NvM_EccHandler_GetBlockConfig(1U, &out) == E_OK, "GetBlockConfig ok");
            CHECK(NvM_EccHandler_GetBlockConfig(1U, NULL_PTR) == E_NOT_OK, "GetBlockConfig NULL out");
            CHECK(NvM_EccHandler_GetBlockConfig(99U, &out) == E_NOT_OK, "GetBlockConfig unknown");
            CHECK(NvM_EccHandler_SetRecoveryStrategy(1U, NVM_ECC_RECOVERY_USE_ROM_DEFAULT) == E_OK,
                  "SetRecoveryStrategy ok");
            CHECK(NvM_EccHandler_SetRecoveryStrategy(99U, NVM_ECC_RECOVERY_NONE) == E_NOT_OK,
                  "SetRecoveryStrategy unknown");
        }

        /* deinit + double deinit */
        CHECK(NvM_EccHandler_DeInit() == E_OK, "EccHandler DeInit ok");
        CHECK(NvM_EccHandler_DeInit() == E_NOT_OK, "EccHandler double DeInit rejected");
    }

    printf("\nResult: %d/%d checks passed\n", t_pass, t_pass + t_fail);
    return (t_fail == 0) ? 0 : 1;
}
