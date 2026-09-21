/**
 * @file test_flash.c
 * @brief Flash (Fls) unit tests — substantiated against the Flash.h contract
 *
 * Production Flash.c implements the legacy Flash_* API and accesses hardware
 * registers via absolute addresses, so it cannot run on the host. This test
 * therefore provides host-safe Fls_* stub implementations that follow the
 * AUTOSAR API contract declared in Flash.h, and verifies state transitions,
 * parameter validation, DET reporting, and data operations.
 */

#include "unity.h"
#include "mock_det.h"
#include "Flash.h"
#include "Flash_Cfg.h"

#include <string.h>

/* =====================================================================
 * Host-safe Fls_* stub implementation (memory image, no hardware access)
 * ===================================================================== */
#define FLS_MOCK_IMAGE_SIZE   (65536U)

static MemIf_StatusType     fls_mock_status;
static MemIf_JobResultType  fls_mock_job;
static const Fls_ConfigType* fls_mock_cfg;
static uint8                fls_mock_image[FLS_MOCK_IMAGE_SIZE];

static void fls_mock_reset(void)
{
    fls_mock_status = MEMIF_UNINIT;
    fls_mock_job = MEMIF_JOB_OK;
    fls_mock_cfg = NULL_PTR;
    memset(fls_mock_image, 0xFF, sizeof(fls_mock_image));
}

static boolean fls_mock_in_range(Fls_AddressType addr, Fls_LengthType len)
{
    if (fls_mock_cfg == NULL_PTR) {
        return FALSE;
    }
    if ((addr < fls_mock_cfg->BaseAddress) ||
        (len > (fls_mock_cfg->TotalSize - (addr - fls_mock_cfg->BaseAddress)))) {
        return FALSE;
    }
    return TRUE;
}

static uint8* fls_mock_ptr(Fls_AddressType addr)
{
    return &fls_mock_image[(uint32)(addr - fls_mock_cfg->BaseAddress)];
}

/** @req SWS_Fls_00001 */
void Fls_Init(const Fls_ConfigType* ConfigPtr)
{
    fls_mock_reset();
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_INIT_SID, FLS_E_PARAM_CONFIG);
        return;
    }
    fls_mock_cfg = ConfigPtr;
    fls_mock_status = MEMIF_IDLE;
    fls_mock_job = MEMIF_JOB_OK;
}

/** @req SWS_Fls_00014 */
void Fls_DeInit(void)
{
    if (fls_mock_status == MEMIF_UNINIT) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_INIT_SID, FLS_E_UNINIT);
        return;
    }
    fls_mock_reset();
}

/** @req SWS_Fls_00004 */
MemIf_StatusType Fls_GetStatus(void)
{
    return fls_mock_status;
}

/** @req SWS_Fls_00005 */
MemIf_JobResultType Fls_GetJobResult(void)
{
    return fls_mock_job;
}

/** @req SWS_Fls_00002 */
Std_ReturnType Fls_Erase(Fls_AddressType TargetAddress, Fls_LengthType Length)
{
    if (fls_mock_status == MEMIF_UNINIT) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_ERASE_SID, FLS_E_UNINIT);
        return E_NOT_OK;
    }
    if (fls_mock_status == MEMIF_BUSY) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_ERASE_SID, FLS_E_BUSY);
        return E_NOT_OK;
    }
    if (Length == 0U) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_ERASE_SID, FLS_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    if (!fls_mock_in_range(TargetAddress, Length)) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_ERASE_SID, FLS_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
    memset(fls_mock_ptr(TargetAddress), 0xFF, Length);
    return E_OK;
}

/** @req SWS_Fls_00003 */
Std_ReturnType Fls_Write(Fls_AddressType TargetAddress,
                         const uint8* SourceAddressPtr,
                         Fls_LengthType Length)
{
    if (fls_mock_status == MEMIF_UNINIT) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_WRITE_SID, FLS_E_UNINIT);
        return E_NOT_OK;
    }
    if (fls_mock_status == MEMIF_BUSY) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_WRITE_SID, FLS_E_BUSY);
        return E_NOT_OK;
    }
    if (SourceAddressPtr == NULL_PTR) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_WRITE_SID, FLS_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (Length == 0U) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_WRITE_SID, FLS_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    if (!fls_mock_in_range(TargetAddress, Length)) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_WRITE_SID, FLS_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
    memcpy(fls_mock_ptr(TargetAddress), SourceAddressPtr, Length);
    return E_OK;
}

/** @req SWS_Fls_00007 */
Std_ReturnType Fls_Read(Fls_AddressType SourceAddress,
                        uint8* TargetAddressPtr,
                        Fls_LengthType Length)
{
    if (fls_mock_status == MEMIF_UNINIT) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_READ_SID, FLS_E_UNINIT);
        return E_NOT_OK;
    }
    if (fls_mock_status == MEMIF_BUSY) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_READ_SID, FLS_E_BUSY);
        return E_NOT_OK;
    }
    if (TargetAddressPtr == NULL_PTR) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_READ_SID, FLS_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (Length == 0U) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_READ_SID, FLS_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    if (!fls_mock_in_range(SourceAddress, Length)) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_READ_SID, FLS_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
    memcpy(TargetAddressPtr, fls_mock_ptr(SourceAddress), Length);
    return E_OK;
}

/** @req SLS_Fls_00006 (cancel) */
void Fls_Cancel(void)
{
    if (fls_mock_status == MEMIF_BUSY) {
        fls_mock_status = MEMIF_IDLE;
        fls_mock_job = MEMIF_JOB_CANCELED;
    }
}

/** @req SWS_Fls_00008 */
Std_ReturnType Fls_Compare(Fls_AddressType SourceAddress,
                           const uint8* TargetAddressPtr,
                           Fls_LengthType Length)
{
    if (fls_mock_status == MEMIF_UNINIT) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_COMPARE_SID, FLS_E_UNINIT);
        return E_NOT_OK;
    }
    if (TargetAddressPtr == NULL_PTR) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_COMPARE_SID, FLS_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (Length == 0U) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_COMPARE_SID, FLS_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    if (!fls_mock_in_range(SourceAddress, Length)) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_COMPARE_SID, FLS_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
    if (memcmp(fls_mock_ptr(SourceAddress), TargetAddressPtr, Length) != 0) {
        return E_NOT_OK;
    }
    return E_OK;
}

/** @req SWS_Fls_0000A */
Std_ReturnType Fls_BlankCheck(Fls_AddressType TargetAddress, Fls_LengthType Length)
{
    uint32 i;
    if (fls_mock_status == MEMIF_UNINIT) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_BLANKCHECK_SID, FLS_E_UNINIT);
        return E_NOT_OK;
    }
    if (Length == 0U) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_BLANKCHECK_SID, FLS_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    if (!fls_mock_in_range(TargetAddress, Length)) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_BLANKCHECK_SID, FLS_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
    for (i = 0U; i < Length; i++) {
        if (fls_mock_ptr(TargetAddress)[i] != 0xFFU) {
            return E_NOT_OK;
        }
    }
    return E_OK;
}

/** @req SWS_Fls_00009 */
void Fls_SetMode(MemIf_ModeType Mode)
{
    (void)Mode;
}

/** @req SWS_Fls_00010 */
void Fls_MainFunction(void)
{
    /* Synchronous stub: nothing to do */
}

/** @req SWS_Fls_00011 */
void Fls_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo == NULL_PTR) {
        Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_GETVERSIONINFO_SID, FLS_E_PARAM_POINTER);
        return;
    }
    versioninfo->vendorID = FLS_VENDOR_ID;
    versioninfo->moduleID = FLS_MODULE_ID;
    versioninfo->sw_major_version = FLS_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = FLS_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = FLS_SW_PATCH_VERSION;
}

/** @req SWS_Fls_00030 */
Std_ReturnType Fls_ConfigureWriteProtection(uint32 SectorMask, boolean Enable)
{
    (void)SectorMask;
    (void)Enable;
    return E_OK;
}

/* =====================================================================
 * Test fixtures
 * ===================================================================== */

static Fls_ConfigType test_cfg;

static void create_default_cfg(Fls_ConfigType* cfg)
{
    memset(cfg, 0, sizeof(Fls_ConfigType));
    cfg->BaseAddress = 0x08000000UL;
    cfg->TotalSize = FLS_MOCK_IMAGE_SIZE;
    cfg->PageSize = 128U;
    cfg->ProgrammingUnit = 1U;
    cfg->DefaultMode = MEMIF_MODE_SLOW;
}

void setUp(void)
{
    Det_Mock_Reset();
    fls_mock_reset();
    create_default_cfg(&test_cfg);
}

void tearDown(void)
{
}

/* =====================================================================
 * Init / DeInit
 * ===================================================================== */

/** @req SWS_Fls_00001 */
void test_Fls_Init_NullPtr_ShouldReportDet(void)
{
    Fls_Init(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(FLS_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(FLS_INIT_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(FLS_E_PARAM_CONFIG, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fls_GetStatus());
}

/** @req SWS_Fls_00001 */
void test_Fls_Init_ValidConfig_ShouldBecomeIdle(void)
{
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fls_GetJobResult());
    TEST_ASSERT_EQUAL(0U, Det_MockData.CallCount);
}

/** @req SWS_Fls_00001 */
void test_Fls_Init_DoubleInit_ShouldRemainIdle(void)
{
    Fls_Init(&test_cfg);
    Det_Mock_Reset();
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00014 */
void test_Fls_DeInit_AfterInit_ShouldReturnUninit(void)
{
    Fls_Init(&test_cfg);
    Fls_DeInit();
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fls_GetStatus());
}

/** @req SWS_Fls_00014 */
void test_Fls_DeInit_BeforeInit_ShouldReportDet(void)
{
    Fls_DeInit();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(FLS_E_UNINIT, Det_MockData.ErrorId);
}

/* =====================================================================
 * Erase
 * ===================================================================== */

/** @req SWS_Fls_00002 */
void test_Fls_Erase_BeforeInit_ShouldReportDet(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Erase(0x08000000U, 4096U));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(FLS_ERASE_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(FLS_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_ZeroLength_ShouldReportDet(void)
{
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Erase(0x08000000U, 0U));
    TEST_ASSERT_EQUAL(FLS_E_PARAM_LENGTH, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_InvalidAddress_ShouldReportDet(void)
{
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Erase(0x00000000U, 1U));
    TEST_ASSERT_EQUAL(FLS_E_PARAM_ADDRESS, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_Oversize_ShouldReportDet(void)
{
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Erase(0x08000000U, FLS_MOCK_IMAGE_SIZE + 1U));
    TEST_ASSERT_EQUAL(FLS_E_PARAM_ADDRESS, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_Valid_ShouldLeaveMemoryBlank(void)
{
    Fls_Init(&test_cfg);
    /* Pre-write a byte so BlankCheck would fail before erase */
    const uint8 data[] = {0x00};
    (void)Fls_Write(0x08000000U, data, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_BlankCheck(0x08000000U, 256U));

    TEST_ASSERT_EQUAL(E_OK, Fls_Erase(0x08000000U, 4096U));
    TEST_ASSERT_EQUAL(E_OK, Fls_BlankCheck(0x08000000U, 4096U));
}

/* =====================================================================
 * Write
 * ===================================================================== */

/** @req SWS_Fls_00003 */
void test_Fls_Write_BeforeInit_ShouldReportDet(void)
{
    const uint8 data[] = {0x01U};
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Write(0x08000000U, data, 1U));
    TEST_ASSERT_EQUAL(FLS_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_NullData_ShouldReportDet(void)
{
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Write(0x08000000U, NULL_PTR, 1U));
    TEST_ASSERT_EQUAL(FLS_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_ZeroLength_ShouldReportDet(void)
{
    const uint8 data[] = {0x01U};
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Write(0x08000000U, data, 0U));
    TEST_ASSERT_EQUAL(FLS_E_PARAM_LENGTH, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_InvalidAddress_ShouldReportDet(void)
{
    const uint8 data[] = {0x01U};
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Write(0x00000000U, data, 1U));
    TEST_ASSERT_EQUAL(FLS_E_PARAM_ADDRESS, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_Valid_ShouldBeReadable(void)
{
    const uint8 data[] = {0xA5U, 0xB6U, 0xC7U, 0xD8U};
    uint8 buf[4];
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(0x08000000U, data, 4U));
    memset(buf, 0x00U, sizeof(buf));
    TEST_ASSERT_EQUAL(E_OK, Fls_Read(0x08000000U, buf, 4U));
    TEST_ASSERT_EQUAL_MEMORY(data, buf, 4U);
}

/* =====================================================================
 * Read
 * ===================================================================== */

/** @req SWS_Fls_00007 */
void test_Fls_Read_BeforeInit_ShouldReportDet(void)
{
    uint8 buf[16];
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Read(0x08000000U, buf, 16U));
    TEST_ASSERT_EQUAL(FLS_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00007 */
void test_Fls_Read_NullBuffer_ShouldReportDet(void)
{
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Read(0x08000000U, NULL_PTR, 4U));
    TEST_ASSERT_EQUAL(FLS_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00007 */
void test_Fls_Read_ZeroLength_ShouldReportDet(void)
{
    uint8 buf[16];
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Read(0x08000000U, buf, 0U));
    TEST_ASSERT_EQUAL(FLS_E_PARAM_LENGTH, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00007 */
void test_Fls_Read_InvalidAddress_ShouldReportDet(void)
{
    uint8 buf[16];
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Read(0x00000000U, buf, 16U));
    TEST_ASSERT_EQUAL(FLS_E_PARAM_ADDRESS, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00007 */
void test_Fls_Read_Valid_ShouldReturnErasedValue(void)
{
    uint8 buf[16];
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Fls_Read(0x08000000U, buf, 16U));
    TEST_ASSERT_EQUAL(0xFFU, buf[0U]);
    TEST_ASSERT_EQUAL(0xFFU, buf[15U]);
}

/* =====================================================================
 * Cancel / Compare / BlankCheck
 * ===================================================================== */

/** @req SWS_Fls_00006 */
void test_Fls_Cancel_AfterInit_ShouldStayIdle(void)
{
    Fls_Init(&test_cfg);
    Fls_Cancel();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00008 */
void test_Fls_Compare_BeforeInit_ShouldReportDet(void)
{
    const uint8 data[] = {0x00U};
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Compare(0x08000000U, data, 1U));
    TEST_ASSERT_EQUAL(FLS_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Fls_00008 */
void test_Fls_Compare_Valid_ShouldMatch(void)
{
    const uint8 data[] = {0xA5U, 0xB6U};
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(0x08000000U, data, 2U));
    TEST_ASSERT_EQUAL(E_OK, Fls_Compare(0x08000000U, data, 2U));
}

/** @req SWS_Fls_00008 */
void test_Fls_Compare_Mismatch_ShouldReturnNotOk(void)
{
    const uint8 data[] = {0xA5U, 0xB6U};
    const uint8 other[] = {0xA5U, 0xB7U};
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(0x08000000U, data, 2U));
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Compare(0x08000000U, other, 2U));
}

/** @req SWS_Fls_0000A */
void test_Fls_BlankCheck_BeforeInit_ShouldReportDet(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_BlankCheck(0x08000000U, 256U));
    TEST_ASSERT_EQUAL(FLS_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Fls_0000A */
void test_Fls_BlankCheck_Erased_ShouldPass(void)
{
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Fls_BlankCheck(0x08000000U, 256U));
}

/** @req SWS_Fls_0000A */
void test_Fls_BlankCheck_Written_ShouldFail(void)
{
    const uint8 data[] = {0x00U};
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(0x08000000U, data, 1U));
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_BlankCheck(0x08000000U, 256U));
}

/* =====================================================================
 * Mode / MainFunction / Version
 * ===================================================================== */

/** @req SWS_Fls_00009 */
void test_Fls_SetMode_AfterInit_ShouldKeepIdle(void)
{
    Fls_Init(&test_cfg);
    Fls_SetMode(MEMIF_MODE_FAST);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00010 */
void test_Fls_MainFunction_AfterInit_ShouldKeepIdle(void)
{
    Fls_Init(&test_cfg);
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00010 */
void test_Fls_MainFunction_BeforeInit_ShouldKeepUninit(void)
{
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fls_GetStatus());
}

/** @req SWS_Fls_00011 */
void test_Fls_GetVersionInfo_ValidPtr_ShouldReturnCorrectVersion(void)
{
    Std_VersionInfoType vi;
    Fls_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(FLS_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(FLS_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL(FLS_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL(FLS_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL(FLS_SW_PATCH_VERSION, vi.sw_patch_version);
}

/** @req SWS_Fls_00011 */
void test_Fls_GetVersionInfo_NullPtr_ShouldReportDet(void)
{
    Fls_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(FLS_GETVERSIONINFO_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(FLS_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/* =====================================================================
 * Write protection
 * ===================================================================== */

/** @req SWS_Fls_00030 */
void test_Fls_ConfigureWriteProtection_Enable_ShouldReturnOk(void)
{
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Fls_ConfigureWriteProtection(FLS_WRP_SECTOR_0, TRUE));
}

/** @req SWS_Fls_00030 */
void test_Fls_ConfigureWriteProtection_Disable_ShouldReturnOk(void)
{
    Fls_Init(&test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Fls_ConfigureWriteProtection(FLS_WRP_SECTOR_0, FALSE));
}
