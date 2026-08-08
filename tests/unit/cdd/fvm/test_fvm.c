/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Cdd_Fvm (Flash Virtual Memory) Unit Tests
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* B3-1 (2026-08-09): compiles the real production sources
* Cdd_Fvm_1.0.0.c + Cdd_Fvm_Hw.c (RAM backend) with a Det_ReportError
* mock, covering:
*   - init lifecycle (default/custom config, double-init, de-init)
*   - bank registration / unregistration / queries
*   - active bank selection
*   - payload read/write with integrity finalization (magic + CRC32)
*   - erase and write protection
*   - bank-to-bank copy (migration) with post-copy verification
*   - corrupt-active failover (manual + automatic via MainFunction)
*   - DET error reporting
*
* Use: cmake -DBUILD_TESTING=ON .. && make CddFvm_UnitTest && ctest -R Fvm
==================================================================================================*/

#include "../test_framework.h"
#include "Cdd_Fvm.h"
#include "Cdd_Fvm_Hw.h"
#include <string.h>

/*==================================================================================================
*                                      DET API ID MIRRORS
*==================================================================================================*/
/* Mirror of the DET API ids used by Cdd_Fvm_1.0.0.c (kept local to the test) */
#define CDD_FVM_SID_REGISTERBANK        0x04u
#define CDD_FVM_SID_UNREGISTERBANK      0x05u
#define CDD_FVM_SID_SELECTACTIVEBANK    0x07u
#define CDD_FVM_SID_GETACTIVEBANK       0x08u
#define CDD_FVM_SID_GETSTATUS           0x0Au
#define CDD_FVM_SID_READ                0x0Bu
#define CDD_FVM_SID_WRITE               0x0Cu
#define CDD_FVM_SID_ERASEBANK           0x0Du
#define CDD_FVM_SID_COPYBANK            0x0Eu
#define CDD_FVM_SID_CHECKBANKINTEGRITY  0x11u
#define CDD_FVM_SID_FAILOVER            0x12u
#define CDD_FVM_SID_GETVERSIONINFO      0x13u

/*==================================================================================================
*                                      DET MOCK
*==================================================================================================*/
#define MOCK_MAX_DET_CALLS                      (128U)

static uint16 g_det_module[MOCK_MAX_DET_CALLS];
static uint8  g_det_api[MOCK_MAX_DET_CALLS];
static uint8  g_det_err[MOCK_MAX_DET_CALLS];
static uint16 g_det_count;

Std_ReturnType Det_ReportError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId)
{
    (void)InstanceId;
    if (g_det_count < (uint16)MOCK_MAX_DET_CALLS)
    {
        g_det_module[g_det_count] = ModuleId;
        g_det_api[g_det_count] = ApiId;
        g_det_err[g_det_count] = ErrorId;
        g_det_count++;
    }
    return E_OK;
}

static void mock_det_reset(void)
{
    g_det_count = 0U;
    (void)memset(g_det_module, 0, sizeof(g_det_module));
    (void)memset(g_det_api, 0, sizeof(g_det_api));
    (void)memset(g_det_err, 0, sizeof(g_det_err));
}

static uint16 mock_det_count_for(uint8 ApiId)
{
    uint16 i;
    uint16 count = 0U;
    for (i = 0U; i < g_det_count; i++)
    {
        if (g_det_api[i] == ApiId)
        {
            count++;
        }
    }
    return count;
}

/*==================================================================================================
*                                      TEST FIXTURE
*==================================================================================================*/

/* Small custom banks (4096 B, sector-aligned, inside the RAM mirror pool) */
#define TEST_BANK_0_ADDR                        (0x00400000u)
#define TEST_BANK_1_ADDR                        (0x00401000u)
#define TEST_BANK_2_ADDR                        (0x00402000u)
#define TEST_BANK_SIZE                          (0x00001000u)

/* Payload area of a bank: [4, size-4) */
#define TEST_PAYLOAD_OFFSET                     (4u)
#define TEST_PAYLOAD_MAX                        (TEST_BANK_SIZE - 4u - 4u)

static const Cdd_Fvm_BankDescriptorType TestBankTable[2] = {
    { 0u, TEST_BANK_0_ADDR, TEST_BANK_SIZE },
    { 1u, TEST_BANK_1_ADDR, TEST_BANK_SIZE }
};

static const Cdd_Fvm_ConfigType TestConfig = {
    TestBankTable,
    2u
};

static uint8 g_payload[64];

static void payload_set_pattern(uint8* buf, uint32 len, uint8 seed)
{
    uint32 i;
    for (i = 0u; i < len; i++)
    {
        buf[i] = (uint8)(seed + i);
    }
}

static void assert_bytes_equal_to(uint8 seed, const uint8* buf, uint32 len)
{
    uint32 i;
    for (i = 0u; i < len; i++)
    {
        char msg[64];
        (void)snprintf(msg, sizeof(msg),
                       "byte %lu: expected 0x%02X was 0x%02X",
                       (unsigned long)i, (unsigned)(seed + i), (unsigned)buf[i]);
        UNITY_TEST_ASSERT(buf[i] == (uint8)(seed + i), msg, __LINE__, __FILE__);
    }
}

void setUp(void)
{
    mock_det_reset();
    (void)memset(g_payload, 0, sizeof(g_payload));
    /* Fresh module: default init with the custom small-bank config */
    (void)Cdd_Fvm_Init(&TestConfig);
}

void tearDown(void)
{
    (void)Cdd_Fvm_DeInit();
    mock_det_reset();
}

/*==================================================================================================
*                                      INIT LIFECYCLE
*==================================================================================================*/
void test_fvm_init_default_config(void)
{
    /* De-init first: setUp used the custom config */
    (void)Cdd_Fvm_DeInit();
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Init(NULL_PTR));

    /* Default table: 2 banks registered, all erased -> active = bank 0 */
    TEST_ASSERT_TRUE(Cdd_Fvm_IsBankRegistered(0u));
    TEST_ASSERT_TRUE(Cdd_Fvm_IsBankRegistered(1u));
    TEST_ASSERT_FALSE(Cdd_Fvm_IsBankRegistered(2u));

    {
        uint8 active = 0xFFu;
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetActiveBank(&active));
        TEST_ASSERT_EQUAL(0u, active);
    }
}

void test_fvm_init_custom_config(void)
{
    uint8 active = 0xFFu;
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetActiveBank(&active));
    TEST_ASSERT_EQUAL(0u, active);

    {
        Cdd_Fvm_BankInfoType info;
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetBankInfo(0u, &info));
        TEST_ASSERT_EQUAL(TEST_BANK_0_ADDR, info.startAddr);
        TEST_ASSERT_EQUAL(TEST_BANK_SIZE, info.size);
        TEST_ASSERT_EQUAL(CDD_FVM_BANK_STATE_ERASED, info.state);
        TEST_ASSERT_FALSE(info.writeProtected);
        TEST_ASSERT_TRUE(info.active);
    }
}

void test_fvm_init_double_init(void)
{
    /* setUp already initialized -> second Init must fail */
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_Init(&TestConfig));
}

void test_fvm_init_null_table_uses_defaults(void)
{
    Cdd_Fvm_ConfigType cfg;
    (void)Cdd_Fvm_DeInit();

    cfg.bankTable = NULL_PTR;
    cfg.numBanks = 0u;
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Init(&cfg));
    TEST_ASSERT_TRUE(Cdd_Fvm_IsBankRegistered(0u));
    TEST_ASSERT_TRUE(Cdd_Fvm_IsBankRegistered(1u));
}

void test_fvm_deinit(void)
{
    Cdd_Fvm_StatusType status;
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_DeInit());

    /* After de-init the module reports UNINIT */
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetStatus(&status));
    TEST_ASSERT_EQUAL(CDD_FVM_STATUS_UNINIT, status);

    /* API calls on uninitialized module -> DET + E_NOT_OK */
    {
        uint8 dummy = 0u;
        TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_GetActiveBank(&dummy));
        TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_GETACTIVEBANK));
    }
}

void test_fvm_status(void)
{
    Cdd_Fvm_StatusType status;
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetStatus(&status));
    TEST_ASSERT_EQUAL(CDD_FVM_STATUS_IDLE, status);

    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_GetStatus(NULL_PTR));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_GETSTATUS));
}

void test_fvm_version_info(void)
{
    Std_VersionInfoType versioninfo;
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetVersionInfo(&versioninfo));
    TEST_ASSERT_EQUAL(CDD_FVM_VENDOR_ID, versioninfo.vendorID);
    TEST_ASSERT_EQUAL(CDD_MODULE_ID_FVM, versioninfo.moduleID);

    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_GetVersionInfo(NULL_PTR));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_GETVERSIONINFO));
}

/*==================================================================================================
*                                      BANK REGISTRATION
*==================================================================================================*/
void test_fvm_register_bank(void)
{
    TEST_ASSERT_FALSE(Cdd_Fvm_IsBankRegistered(2u));
    TEST_ASSERT_EQUAL(E_OK,
        Cdd_Fvm_RegisterBank(2u, TEST_BANK_2_ADDR, TEST_BANK_SIZE));
    TEST_ASSERT_TRUE(Cdd_Fvm_IsBankRegistered(2u));

    {
        Cdd_Fvm_BankInfoType info;
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetBankInfo(2u, &info));
        TEST_ASSERT_EQUAL(TEST_BANK_2_ADDR, info.startAddr);
        TEST_ASSERT_EQUAL(CDD_FVM_BANK_STATE_ERASED, info.state);
    }
}

void test_fvm_register_duplicate(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK,
        Cdd_Fvm_RegisterBank(0u, TEST_BANK_0_ADDR, TEST_BANK_SIZE));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_REGISTERBANK));
}

void test_fvm_register_invalid_id(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK,
        Cdd_Fvm_RegisterBank(CDD_FVM_MAX_BANKS, TEST_BANK_2_ADDR, TEST_BANK_SIZE));
    TEST_ASSERT_EQUAL(E_NOT_OK,
        Cdd_Fvm_RegisterBank(0xFFu, TEST_BANK_2_ADDR, TEST_BANK_SIZE));
}

void test_fvm_register_invalid_size(void)
{
    /* Not sector aligned */
    TEST_ASSERT_EQUAL(E_NOT_OK,
        Cdd_Fvm_RegisterBank(2u, TEST_BANK_2_ADDR, 0x0800u));
    /* Too small for magic + signature (size must be >= 8) */
    TEST_ASSERT_EQUAL(E_NOT_OK,
        Cdd_Fvm_RegisterBank(2u, TEST_BANK_2_ADDR, 0u));
}

void test_fvm_unregister_bank(void)
{
    TEST_ASSERT_EQUAL(E_OK,
        Cdd_Fvm_RegisterBank(2u, TEST_BANK_2_ADDR, TEST_BANK_SIZE));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_UnregisterBank(2u));
    TEST_ASSERT_FALSE(Cdd_Fvm_IsBankRegistered(2u));

    /* Unregistering an unregistered bank -> DET + E_NOT_OK */
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_UnregisterBank(2u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_UNREGISTERBANK));
}

void test_fvm_unregister_active_bank(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_UnregisterBank(0u));
}

/*==================================================================================================
*                                      ACTIVE BANK SELECTION
*==================================================================================================*/
void test_fvm_select_active_bank(void)
{
    uint8 active = 0xFFu;
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_SelectActiveBank(1u));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetActiveBank(&active));
    TEST_ASSERT_EQUAL(1u, active);

    {
        Cdd_Fvm_BankInfoType info;
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetBankInfo(0u, &info));
        TEST_ASSERT_FALSE(info.active);
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetBankInfo(1u, &info));
        TEST_ASSERT_TRUE(info.active);
    }
}

void test_fvm_select_invalid_bank(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_SelectActiveBank(7u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_SELECTACTIVEBANK));
}

void test_fvm_get_active_null(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_GetActiveBank(NULL_PTR));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_GETACTIVEBANK));
}

/*==================================================================================================
*                                      READ / WRITE / INTEGRITY
*==================================================================================================*/
void test_fvm_write_read_roundtrip(void)
{
    uint8 buf[16];

    payload_set_pattern(g_payload, 16u, 0x10u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 16u));

    (void)memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Read(0u, TEST_PAYLOAD_OFFSET, buf, 16u));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(g_payload, buf, 16);
}

void test_fvm_write_finalizes_bank(void)
{
    boolean valid = FALSE;

    /* Erased bank is not valid yet */
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CheckBankIntegrity(0u, &valid));
    TEST_ASSERT_FALSE(valid);

    /* First write finalizes: magic + CRC32 signature written */
    payload_set_pattern(g_payload, 8u, 0x20u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CheckBankIntegrity(0u, &valid));
    TEST_ASSERT_TRUE(valid);

    {
        Cdd_Fvm_BankInfoType info;
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetBankInfo(0u, &info));
        TEST_ASSERT_EQUAL(CDD_FVM_BANK_STATE_VALID, info.state);
    }
}

void test_fvm_write_updates_signature(void)
{
    boolean valid = FALSE;

    payload_set_pattern(g_payload, 8u, 0x30u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));

    /* Rewrite with different content: signature must be refreshed */
    payload_set_pattern(g_payload, 8u, 0x31u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CheckBankIntegrity(0u, &valid));
    TEST_ASSERT_TRUE(valid);
}

void test_fvm_write_large_payload(void)
{
    static uint8 big[2048];
    static uint8 out[2048];
    uint32 i;

    for (i = 0u; i < 2048u; i++)
    {
        big[i] = (uint8)(i & 0xFFu);
    }
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, big, 2048u));

    (void)memset(out, 0, sizeof(out));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Read(0u, TEST_PAYLOAD_OFFSET, out, 2048u));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(big, out, 2048);
}

void test_fvm_write_metadata_area_rejected(void)
{
    /* offset 0..3 is the magic header -> out of payload range */
    payload_set_pattern(g_payload, 4u, 0x40u);
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_Write(0u, 0u, g_payload, 4u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_WRITE));

    /* Write reaching into the signature tail -> out of payload range */
    TEST_ASSERT_EQUAL(E_NOT_OK,
        Cdd_Fvm_Write(0u, TEST_PAYLOAD_MAX - 1u, g_payload, 8u));
}

void test_fvm_read_metadata_area_rejected(void)
{
    uint8 buf[4];
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_Read(0u, 0u, buf, 4u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_READ));
}

void test_fvm_write_null_pointer(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, NULL_PTR, 4u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_WRITE));
}

void test_fvm_read_null_pointer(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_Read(0u, TEST_PAYLOAD_OFFSET, NULL_PTR, 4u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_READ));
}

void test_fvm_write_unregistered_bank(void)
{
    payload_set_pattern(g_payload, 4u, 0x50u);
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_Write(3u, TEST_PAYLOAD_OFFSET, g_payload, 4u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_WRITE));
}

void test_fvm_integrity_null(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_CheckBankIntegrity(0u, NULL_PTR));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_CHECKBANKINTEGRITY));
}

/*==================================================================================================
*                                      ERASE
*==================================================================================================*/
void test_fvm_erase_bank(void)
{
    boolean valid = TRUE;
    Cdd_Fvm_BankInfoType info;

    payload_set_pattern(g_payload, 8u, 0x60u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CheckBankIntegrity(0u, &valid));
    TEST_ASSERT_TRUE(valid);

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_EraseBank(0u));

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CheckBankIntegrity(0u, &valid));
    TEST_ASSERT_FALSE(valid);

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetBankInfo(0u, &info));
    TEST_ASSERT_EQUAL(CDD_FVM_BANK_STATE_ERASED, info.state);
}

void test_fvm_erase_unregistered_bank(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_EraseBank(3u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_ERASEBANK));
}

/*==================================================================================================
*                                      WRITE PROTECTION
*==================================================================================================*/
void test_fvm_protect_blocks_write_erase(void)
{
    boolean protect = FALSE;
    uint8 buf[8];

    payload_set_pattern(g_payload, 8u, 0x70u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_ProtectBank(0u, TRUE));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_IsBankProtected(0u, &protect));
    TEST_ASSERT_TRUE(protect);

    /* Write rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_WRITE));

    /* Erase rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_EraseBank(0u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_ERASEBANK));

    /* Payload still readable while protected */
    (void)memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Read(0u, TEST_PAYLOAD_OFFSET, buf, 8u));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(g_payload, buf, 8);
}

void test_fvm_unprotect_restores_write(void)
{
    boolean protect = TRUE;
    uint8 buf[8];

    payload_set_pattern(g_payload, 8u, 0x71u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_ProtectBank(0u, TRUE));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_ProtectBank(0u, FALSE));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_IsBankProtected(0u, &protect));
    TEST_ASSERT_FALSE(protect);

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));

    (void)memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Read(0u, TEST_PAYLOAD_OFFSET, buf, 8u));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(g_payload, buf, 8);
}

void test_fvm_protect_invalid_bank(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_ProtectBank(3u, TRUE));
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_IsBankProtected(3u, NULL_PTR));
}

/*==================================================================================================
*                                      BANK COPY (MIGRATION)
*==================================================================================================*/
void test_fvm_copy_bank(void)
{
    uint8 buf[16];
    boolean valid = FALSE;

    payload_set_pattern(g_payload, 16u, 0x80u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 16u));

    /* Destination is currently erased */
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CheckBankIntegrity(1u, &valid));
    TEST_ASSERT_FALSE(valid);

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CopyBank(0u, 1u));

    /* Destination becomes valid with identical content */
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CheckBankIntegrity(1u, &valid));
    TEST_ASSERT_TRUE(valid);

    (void)memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Read(1u, TEST_PAYLOAD_OFFSET, buf, 16u));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(g_payload, buf, 16);
}

void test_fvm_copy_source_later_modified(void)
{
    uint8 buf[8];

    payload_set_pattern(g_payload, 8u, 0x81u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CopyBank(0u, 1u));

    /* Modify source afterwards: destination stays at the copied snapshot */
    payload_set_pattern(g_payload, 8u, 0x82u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));

    (void)memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Read(1u, TEST_PAYLOAD_OFFSET, buf, 8u));
    assert_bytes_equal_to(0x81u, buf, 8u);
}

void test_fvm_copy_corrupt_source_rejected(void)
{
    uint8 garbage[4] = { 0xDEu, 0xADu, 0xBEu, 0xEFu };

    /* Bank 0 contains garbage (never finalized) */
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_HwWrite(TEST_BANK_0_ADDR, garbage, 4u));

    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_CopyBank(0u, 1u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_COPYBANK));

    /* Destination must stay untouched (erased) */
    {
        boolean valid = TRUE;
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CheckBankIntegrity(1u, &valid));
        TEST_ASSERT_FALSE(valid);
    }
}

void test_fvm_copy_same_bank_rejected(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_CopyBank(0u, 0u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_COPYBANK));
}

void test_fvm_copy_to_protected_rejected(void)
{
    payload_set_pattern(g_payload, 8u, 0x83u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_ProtectBank(1u, TRUE));
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_CopyBank(0u, 1u));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_COPYBANK));
}

void test_fvm_copy_unregistered_bank(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_CopyBank(0u, 3u));
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_CopyBank(3u, 0u));
}

/*==================================================================================================
*                                      FAILOVER
*==================================================================================================*/
static void prepare_two_valid_banks(void)
{
    /* Both banks valid */
    payload_set_pattern(g_payload, 8u, 0x90u);
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(0u, TEST_PAYLOAD_OFFSET, g_payload, 8u));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Write(1u, TEST_PAYLOAD_OFFSET, g_payload, 8u));
}

static void corrupt_bank0_magic(void)
{
    uint8 garbage[4] = { 0x11u, 0x22u, 0x33u, 0x44u };
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_HwWrite(TEST_BANK_0_ADDR, garbage, 4u));
}

void test_fvm_failover_switches_to_backup(void)
{
    uint8 active = 0xFFu;
    uint8 newBank = 0xFFu;

    prepare_two_valid_banks();
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_SelectActiveBank(0u));

    /* Corrupt the active bank */
    corrupt_bank0_magic();

    {
        boolean valid = TRUE;
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CheckBankIntegrity(0u, &valid));
        TEST_ASSERT_FALSE(valid);
    }

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Failover(&newBank));
    TEST_ASSERT_EQUAL(1u, newBank);

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetActiveBank(&active));
    TEST_ASSERT_EQUAL(1u, active);

    {
        Cdd_Fvm_BankInfoType info;
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetBankInfo(1u, &info));
        TEST_ASSERT_TRUE(info.active);
        TEST_ASSERT_EQUAL(CDD_FVM_BANK_STATE_VALID, info.state);
    }
}

void test_fvm_failover_healthy_active_noop(void)
{
    uint8 newBank = 0xFFu;

    prepare_two_valid_banks();
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_SelectActiveBank(0u));

    /* Active is healthy -> no failover */
    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_Failover(&newBank));
    TEST_ASSERT_EQUAL(0xFFu, newBank);

    {
        uint8 active = 0xFFu;
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetActiveBank(&active));
        TEST_ASSERT_EQUAL(0u, active);
    }
}

void test_fvm_failover_no_valid_backup(void)
{
    /* Bank 0 corrupted, bank 1 erased (no valid backup) */
    corrupt_bank0_magic();
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_SelectActiveBank(0u));

    TEST_ASSERT_EQUAL(E_NOT_OK, Cdd_Fvm_Failover(NULL_PTR));
    TEST_ASSERT_EQUAL(1u, mock_det_count_for(CDD_FVM_SID_FAILOVER));
}

void test_fvm_failover_out_param_optional(void)
{
    prepare_two_valid_banks();
    corrupt_bank0_magic();

    /* NULL out-param is allowed */
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Failover(NULL_PTR));
    {
        uint8 active = 0xFFu;
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetActiveBank(&active));
        TEST_ASSERT_EQUAL(1u, active);
    }
}

void test_fvm_mainfunction_auto_failover(void)
{
    uint8 active = 0xFFu;

    prepare_two_valid_banks();
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_SelectActiveBank(0u));
    corrupt_bank0_magic();

    /* MainFunction detects the corrupt active bank and fails over */
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_MainFunction());
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetActiveBank(&active));
    TEST_ASSERT_EQUAL(1u, active);
}

void test_fvm_mainfunction_healthy_no_change(void)
{
    uint8 active = 0xFFu;

    prepare_two_valid_banks();
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_SelectActiveBank(0u));

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_MainFunction());
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_GetActiveBank(&active));
    TEST_ASSERT_EQUAL(0u, active);
}

/*==================================================================================================
*                                      RESTORE VIA COPY
*==================================================================================================*/
void test_fvm_restore_corrupt_active_from_backup(void)
{
    uint8 buf[8];

    prepare_two_valid_banks();
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_SelectActiveBank(0u));
    corrupt_bank0_magic();

    /* Failover to backup, then restore the corrupt primary from it */
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Failover(NULL_PTR));

    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_EraseBank(0u));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CopyBank(1u, 0u));

    {
        boolean valid = FALSE;
        TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_CheckBankIntegrity(0u, &valid));
        TEST_ASSERT_TRUE(valid);
    }

    (void)memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL(E_OK, Cdd_Fvm_Read(0u, TEST_PAYLOAD_OFFSET, buf, 8u));
    assert_bytes_equal_to(0x90u, buf, 8u);
}

/*==================================================================================================
*                                      RUNNER
*==================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    /* Init lifecycle */
    RUN_TEST(test_fvm_init_default_config);
    RUN_TEST(test_fvm_init_custom_config);
    RUN_TEST(test_fvm_init_double_init);
    RUN_TEST(test_fvm_init_null_table_uses_defaults);
    RUN_TEST(test_fvm_deinit);
    RUN_TEST(test_fvm_status);
    RUN_TEST(test_fvm_version_info);

    /* Bank registration */
    RUN_TEST(test_fvm_register_bank);
    RUN_TEST(test_fvm_register_duplicate);
    RUN_TEST(test_fvm_register_invalid_id);
    RUN_TEST(test_fvm_register_invalid_size);
    RUN_TEST(test_fvm_unregister_bank);
    RUN_TEST(test_fvm_unregister_active_bank);

    /* Active bank selection */
    RUN_TEST(test_fvm_select_active_bank);
    RUN_TEST(test_fvm_select_invalid_bank);
    RUN_TEST(test_fvm_get_active_null);

    /* Read / write / integrity */
    RUN_TEST(test_fvm_write_read_roundtrip);
    RUN_TEST(test_fvm_write_finalizes_bank);
    RUN_TEST(test_fvm_write_updates_signature);
    RUN_TEST(test_fvm_write_large_payload);
    RUN_TEST(test_fvm_write_metadata_area_rejected);
    RUN_TEST(test_fvm_read_metadata_area_rejected);
    RUN_TEST(test_fvm_write_null_pointer);
    RUN_TEST(test_fvm_read_null_pointer);
    RUN_TEST(test_fvm_write_unregistered_bank);
    RUN_TEST(test_fvm_integrity_null);

    /* Erase */
    RUN_TEST(test_fvm_erase_bank);
    RUN_TEST(test_fvm_erase_unregistered_bank);

    /* Write protection */
    RUN_TEST(test_fvm_protect_blocks_write_erase);
    RUN_TEST(test_fvm_unprotect_restores_write);
    RUN_TEST(test_fvm_protect_invalid_bank);

    /* Bank copy */
    RUN_TEST(test_fvm_copy_bank);
    RUN_TEST(test_fvm_copy_source_later_modified);
    RUN_TEST(test_fvm_copy_corrupt_source_rejected);
    RUN_TEST(test_fvm_copy_same_bank_rejected);
    RUN_TEST(test_fvm_copy_to_protected_rejected);
    RUN_TEST(test_fvm_copy_unregistered_bank);

    /* Failover */
    RUN_TEST(test_fvm_failover_switches_to_backup);
    RUN_TEST(test_fvm_failover_healthy_active_noop);
    RUN_TEST(test_fvm_failover_no_valid_backup);
    RUN_TEST(test_fvm_failover_out_param_optional);
    RUN_TEST(test_fvm_mainfunction_auto_failover);
    RUN_TEST(test_fvm_mainfunction_healthy_no_change);

    /* Restore */
    RUN_TEST(test_fvm_restore_corrupt_active_from_backup);

    return UNITY_END();
}
