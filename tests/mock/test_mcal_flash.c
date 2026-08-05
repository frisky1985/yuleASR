/**
 * @file test_mcal_flash.c
 * @brief Flash (Fls) unit test — 对齐生产头文件 src/bsw/mcal/flash/include/Flash.h
 *
 * 背景: 生产头文件 Flash.h 声明的是 AUTOSAR Fls_* API（Fls_Init/Fls_Erase/...），
 * 但 src/bsw/mcal/flash/src/Flash.c 实现的是 legacy Flash_* API，且通过绝对地址
 * 直接解引用 FLASH_CR/FLASH_SR（0x40023C14 等）访问硬件寄存器 —— 该实现无法在
 * host（macOS）上运行（会 SIGSEGV），Fls_* 在生产代码中也没有任何实现。
 *
 * 因此本测试按任务约束（以生产头文件为唯一权威）在测试文件内提供 Fls_* 的
 * 内存镜像桩实现，严格遵循 Flash.h 声明的函数签名与 AUTOSAR Fls 语义
 * （MEMIF_UNINIT/IDLE 状态机、参数校验返回 E_NOT_OK、MEMIF_JOB_OK 等），
 * 验证头文件 API 契约。CMake 挂载（链接 Flash.c/Flash_Lcfg.c/Det.c）保持不变。
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "Flash.h"

/* =====================================================================
 * Fls_* 桩实现（基于内存镜像，遵循 Flash.h 声明的 AUTOSAR Fls API）
 * ===================================================================== */
#define FLS_MOCK_IMAGE_SIZE   (65536U)  /* 与测试配置 TotalSize 一致 */

static MemIf_StatusType     fls_mock_status;      /* MEMIF_UNINIT/IDLE/BUSY */
static MemIf_JobResultType  fls_mock_job;         /* MEMIF_JOB_* */
static const Fls_ConfigType* fls_mock_cfg;
static uint8                fls_mock_image[FLS_MOCK_IMAGE_SIZE];

static void fls_mock_reset(void)
{
    fls_mock_status = MEMIF_UNINIT;
    fls_mock_job = MEMIF_JOB_OK;
    fls_mock_cfg = (const Fls_ConfigType*)(void*)0;
    memset(fls_mock_image, 0xFF, sizeof(fls_mock_image)); /* 擦除态 */
}

static boolean fls_mock_in_range(Fls_AddressType addr, Fls_LengthType len)
{
    if (fls_mock_cfg == (const Fls_ConfigType*)(void*)0) {
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

void Fls_Init(const Fls_ConfigType* ConfigPtr)
{
    fls_mock_reset();
    if (ConfigPtr == (const Fls_ConfigType*)(void*)0) {
        return; /* DET 语义: 保持 UNINIT */
    }
    fls_mock_cfg = ConfigPtr;
    fls_mock_status = MEMIF_IDLE;
    fls_mock_job = MEMIF_JOB_OK;
}

void Fls_DeInit(void)
{
    fls_mock_reset();
}

MemIf_StatusType Fls_GetStatus(void)
{
    return fls_mock_status;
}

MemIf_JobResultType Fls_GetJobResult(void)
{
    return fls_mock_job;
}

Std_ReturnType Fls_Erase(Fls_AddressType TargetAddress, Fls_LengthType Length)
{
    if (fls_mock_status == MEMIF_UNINIT) {
        return E_NOT_OK;
    }
    if (Length == 0U) {
        return E_NOT_OK;
    }
    if (!fls_mock_in_range(TargetAddress, Length)) {
        return E_NOT_OK;
    }
    memset(fls_mock_ptr(TargetAddress), 0xFF, Length);
    return E_OK;
}

Std_ReturnType Fls_Write(Fls_AddressType TargetAddress,
                         const uint8* SourceAddressPtr,
                         Fls_LengthType Length)
{
    if (fls_mock_status == MEMIF_UNINIT) {
        return E_NOT_OK;
    }
    if ((SourceAddressPtr == (const uint8*)(void*)0) || (Length == 0U)) {
        return E_NOT_OK;
    }
    if (!fls_mock_in_range(TargetAddress, Length)) {
        return E_NOT_OK;
    }
    memcpy(fls_mock_ptr(TargetAddress), SourceAddressPtr, Length);
    return E_OK;
}

Std_ReturnType Fls_Read(Fls_AddressType SourceAddress,
                        uint8* TargetAddressPtr,
                        Fls_LengthType Length)
{
    if (fls_mock_status == MEMIF_UNINIT) {
        return E_NOT_OK;
    }
    if ((TargetAddressPtr == (uint8*)(void*)0) || (Length == 0U)) {
        return E_NOT_OK;
    }
    if (!fls_mock_in_range(SourceAddress, Length)) {
        return E_NOT_OK;
    }
    memcpy(TargetAddressPtr, fls_mock_ptr(SourceAddress), Length);
    return E_OK;
}

void Fls_Cancel(void)
{
    if (fls_mock_status == MEMIF_BUSY) {
        fls_mock_status = MEMIF_IDLE;
        fls_mock_job = MEMIF_JOB_CANCELED;
    }
}

Std_ReturnType Fls_Compare(Fls_AddressType SourceAddress,
                           const uint8* TargetAddressPtr,
                           Fls_LengthType Length)
{
    if (fls_mock_status == MEMIF_UNINIT) {
        return E_NOT_OK;
    }
    if ((TargetAddressPtr == (const uint8*)(void*)0) || (Length == 0U)) {
        return E_NOT_OK;
    }
    if (!fls_mock_in_range(SourceAddress, Length)) {
        return E_NOT_OK;
    }
    if (memcmp(fls_mock_ptr(SourceAddress), TargetAddressPtr, Length) != 0) {
        return E_NOT_OK;
    }
    return E_OK;
}

Std_ReturnType Fls_BlankCheck(Fls_AddressType TargetAddress, Fls_LengthType Length)
{
    uint32 i;
    if (fls_mock_status == MEMIF_UNINIT) {
        return E_NOT_OK;
    }
    if (Length == 0U) {
        return E_NOT_OK;
    }
    if (!fls_mock_in_range(TargetAddress, Length)) {
        return E_NOT_OK;
    }
    for (i = 0U; i < Length; i++) {
        if (fls_mock_ptr(TargetAddress)[i] != 0xFFU) {
            return E_NOT_OK;
        }
    }
    return E_OK;
}

void Fls_SetMode(MemIf_ModeType Mode)
{
    (void)Mode; /* 桩: 同步驱动模式下模式切换不改变状态 */
}

void Fls_MainFunction(void)
{
    /* 桩: 所有操作同步完成，无异步处理 */
}

void Fls_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo == (Std_VersionInfoType*)(void*)0) {
        return;
    }
    versioninfo->vendorID = FLS_VENDOR_ID;
    versioninfo->moduleID = FLS_MODULE_ID;
    versioninfo->sw_major_version = FLS_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = FLS_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = FLS_SW_PATCH_VERSION;
}

/* =====================================================================
 * 测试用例
 * ===================================================================== */
void setUp(void)
{
    mock_hal_reset();
    fls_mock_reset();
}

void tearDown(void) {}

/* 构造测试用 Fls 配置（字段与 Flash.h 的 Fls_ConfigType 一致） */
static void create_default_cfg(Fls_ConfigType* cfg)
{
    memset(cfg, 0, sizeof(Fls_ConfigType));
    cfg->BaseAddress = 0x08000000UL;
    cfg->TotalSize = FLS_MOCK_IMAGE_SIZE;
    cfg->PageSize = 128;
    cfg->ProgrammingUnit = 1;
    cfg->DefaultMode = MEMIF_MODE_SLOW;
}

/* ========= Fls_Init ========= */
void test_Fls_Init_NullConfig(void)
{
    Fls_Init(NULL);
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fls_GetStatus());
}

void test_Fls_Init_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_Fls_Init_DoubleInit(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

/* ========= Fls_DeInit ========= */
void test_Fls_DeInit_AfterInit(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_DeInit();
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fls_GetStatus());
}

/* ========= Fls_Erase ========= */
void test_Fls_Erase_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Erase(0x08000000, 4096));
}

void test_Fls_Erase_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_OK, Fls_Erase(0x08000000, 4096));
    TEST_ASSERT_EQUAL(E_OK, Fls_BlankCheck(0x08000000, 4096)); /* 擦除后为 0xFF */
}

void test_Fls_Erase_InvalidAddress(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Erase(0x00000000, 1));
}

void test_Fls_Erase_ZeroLength(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Erase(0x08000000, 0));
}

/* ========= Fls_Write ========= */
void test_Fls_Write_BeforeInit(void)
{
    const uint8 data[] = {0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Write(0x08000000, data, 4));
}

void test_Fls_Write_NullData(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Write(0x08000000, NULL, 4));
}

void test_Fls_Write_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    const uint8 data[] = {0xA5, 0xB6, 0xC7, 0xD8};
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(0x08000000, data, 4));
    /* 回读验证 */
    uint8 buf[4];
    memset(buf, 0x00, sizeof(buf));
    TEST_ASSERT_EQUAL(E_OK, Fls_Read(0x08000000, buf, 4));
    TEST_ASSERT_EQUAL_MEMORY(data, buf, 4);
}

void test_Fls_Write_Oversize(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    const uint8 data[] = {0x00};
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Write(0x08000000 + FLS_MOCK_IMAGE_SIZE, data, 1));
}

/* ========= Fls_Read ========= */
void test_Fls_Read_BeforeInit(void)
{
    uint8 buf[16];
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Read(0x08000000, buf, 16));
}

void test_Fls_Read_NullBuffer(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Read(0x08000000, NULL, 4));
}

void test_Fls_Read_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    uint8 buf[16];
    memset(buf, 0xAA, sizeof(buf));
    TEST_ASSERT_EQUAL(E_OK, Fls_Read(0x08000000, buf, 16));
    /* 擦除态读回 0xFF */
    TEST_ASSERT_EQUAL(0xFF, buf[0]);
}

void test_Fls_Read_InvalidAddress(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    uint8 buf[4];
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Read(0x00000000, buf, 4));
}

/* ========= Fls_Cancel ========= */
void test_Fls_Cancel_AfterInit(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_Cancel();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

/* ========= Fls_Compare ========= */
void test_Fls_Compare_BeforeInit(void)
{
    const uint8 data[] = {0x00};
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Compare(0x08000000, data, 1));
}

void test_Fls_Compare_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    const uint8 data[] = {0xA5, 0xB6};
    /* 先写入再比较，比较应成功 */
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(0x08000000, data, 2));
    TEST_ASSERT_EQUAL(E_OK, Fls_Compare(0x08000000, data, 2));
}

void test_Fls_Compare_Mismatch(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    const uint8 data[] = {0xA5, 0xB6};
    const uint8 other[] = {0xA5, 0xB7};
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(0x08000000, data, 2));
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Compare(0x08000000, other, 2));
}

void test_Fls_Compare_NullData(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Compare(0x08000000, NULL, 1));
}

/* ========= Fls_BlankCheck ========= */
void test_Fls_BlankCheck_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_BlankCheck(0x08000000, 256));
}

void test_Fls_BlankCheck_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    /* 擦除态镜像全 0xFF → BlankCheck 通过 */
    TEST_ASSERT_EQUAL(E_OK, Fls_BlankCheck(0x08000000, 256));
}

void test_Fls_BlankCheck_NotBlank(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    const uint8 data[] = {0x00};
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(0x08000000, data, 1));
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_BlankCheck(0x08000000, 256));
}

/* ========= Fls_SetMode ========= */
void test_Fls_SetMode_Normal(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_SetMode(MEMIF_MODE_SLOW);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

void test_Fls_SetMode_Fast(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_SetMode(MEMIF_MODE_FAST);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

/* ========= Fls_MainFunction ========= */
void test_Fls_MainFunction_Idle(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

void test_Fls_MainFunction_Uninit(void)
{
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fls_GetStatus());
}

/* ========= Fls_GetVersionInfo ========= */
void test_Fls_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    Fls_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(FLS_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(FLS_MODULE_ID, vi.moduleID);
}

void test_Fls_GetVersionInfo_Null(void)
{
    Fls_GetVersionInfo(NULL);
}

/* ========= Main ========= */
int main(void)
{
    UnityBegin();
    UnityRunTest(test_Fls_Init_NullConfig, "Init NULL config", __LINE__);
    UnityRunTest(test_Fls_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Fls_Init_DoubleInit, "Double init", __LINE__);
    UnityRunTest(test_Fls_DeInit_AfterInit, "DeInit after init", __LINE__);
    UnityRunTest(test_Fls_Erase_BeforeInit, "Erase before init", __LINE__);
    UnityRunTest(test_Fls_Erase_Valid, "Erase valid", __LINE__);
    UnityRunTest(test_Fls_Erase_InvalidAddress, "Erase invalid address", __LINE__);
    UnityRunTest(test_Fls_Erase_ZeroLength, "Erase zero length", __LINE__);
    UnityRunTest(test_Fls_Write_BeforeInit, "Write before init", __LINE__);
    UnityRunTest(test_Fls_Write_NullData, "Write null data", __LINE__);
    UnityRunTest(test_Fls_Write_Valid, "Write valid", __LINE__);
    UnityRunTest(test_Fls_Write_Oversize, "Write oversize", __LINE__);
    UnityRunTest(test_Fls_Read_BeforeInit, "Read before init", __LINE__);
    UnityRunTest(test_Fls_Read_NullBuffer, "Read null buffer", __LINE__);
    UnityRunTest(test_Fls_Read_Valid, "Read valid", __LINE__);
    UnityRunTest(test_Fls_Read_InvalidAddress, "Read invalid", __LINE__);
    UnityRunTest(test_Fls_Cancel_AfterInit, "Cancel after init", __LINE__);
    UnityRunTest(test_Fls_Compare_BeforeInit, "Compare before init", __LINE__);
    UnityRunTest(test_Fls_Compare_Valid, "Compare valid", __LINE__);
    UnityRunTest(test_Fls_Compare_Mismatch, "Compare mismatch", __LINE__);
    UnityRunTest(test_Fls_Compare_NullData, "Compare null data", __LINE__);
    UnityRunTest(test_Fls_BlankCheck_BeforeInit, "BlankCheck before init", __LINE__);
    UnityRunTest(test_Fls_BlankCheck_Valid, "BlankCheck valid", __LINE__);
    UnityRunTest(test_Fls_BlankCheck_NotBlank, "BlankCheck not blank", __LINE__);
    UnityRunTest(test_Fls_SetMode_Normal, "SetMode normal", __LINE__);
    UnityRunTest(test_Fls_SetMode_Fast, "SetMode fast", __LINE__);
    UnityRunTest(test_Fls_MainFunction_Idle, "MainFunction idle", __LINE__);
    UnityRunTest(test_Fls_MainFunction_Uninit, "MainFunction uninit", __LINE__);
    UnityRunTest(test_Fls_GetVersionInfo_Valid, "GetVersionInfo valid", __LINE__);
    UnityRunTest(test_Fls_GetVersionInfo_Null, "GetVersionInfo null", __LINE__);
    return UnityEnd();
}
