/**
 * @file test_bl_partition.c
 * @brief bl_partition 默认分区表与 Flash 几何边界回归测试
 *
 * 背景（第 13 轮静态加固）:
 *   bl_partition_table_init_default() 接收 flash_size 参数，但固定布局
 *   (256KB boot + 2x6MB app + 1MB cal + OTA 元数据扇区) 从未与实际 Flash
 *   容量比较；同时 get_info() 返回的 sector_size 也未做合法性校验，
 *   0 或非 2 的幂会让 align_to_sector() 把所有分区起点压到 0。
 *   两者都会生成指向器件之外 / 相互重叠的分区表，并被当作成功保存。
 */

// @tests src/bootloader/bl_partition.c  @tests src/bootloader/bl_partition.h

#include "unity.h"
#include "bl_partition.h"
#include "dds_log.h"
#include <string.h>
#include <stdarg.h>

/* ============================================================================
 * 依赖桩: 日志与时间
 * ============================================================================ */
void dds_log_write(dds_log_level_t level, dds_log_type_t type,
                   const char* module, const char* tag,
                   const char* file, int line,
                   const char* fmt, ...)
{
    (void)level;
    (void)type;
    (void)module;
    (void)tag;
    (void)file;
    (void)line;
    (void)fmt;
}

bool bl_time_get_ms(uint64_t *out_ms)
{
    if (out_ms != NULL) {
        *out_ms = 0U;
    }
    return false;
}

/* ============================================================================
 * 可控 Flash 驱动桩
 * ============================================================================ */
static uint32_t g_flash_total_size = 16U * 1024U * 1024U;
static uint32_t g_flash_sector_size = BL_FLASH_SECTOR_SIZE;

static int32_t mock_flash_init(void)
{
    return 0;
}

static int32_t mock_flash_get_info(uint32_t *total_size, uint32_t *sector_size)
{
    if (total_size != NULL) {
        *total_size = g_flash_total_size;
    }
    if (sector_size != NULL) {
        *sector_size = g_flash_sector_size;
    }
    return 0;
}

static const bl_flash_driver_t mock_flash_driver = {
    .init = mock_flash_init,
    .get_info = mock_flash_get_info,
};

static bl_partition_manager_t g_mgr;

void setUp(void)
{
    (void)memset(&g_mgr, 0, sizeof(g_mgr));
    g_mgr.flash_driver = &mock_flash_driver;
    g_flash_total_size = 16U * 1024U * 1024U;
    g_flash_sector_size = BL_FLASH_SECTOR_SIZE;
}

void tearDown(void)
{
}

/* ============================================================================
 * 测试
 * ============================================================================ */

/** 16MB 器件: 固定布局可容纳, 应成功且所有分区落在器件内 */
void test_DefaultTable_Fits16MbFlash_ShouldSucceed(void)
{
    bl_partition_error_t rc = bl_partition_table_init_default(&g_mgr, g_flash_total_size);

    TEST_ASSERT_EQUAL(BL_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(BL_MAX_PARTITIONS, g_mgr.table.header.num_partitions);
    TEST_ASSERT_TRUE(g_mgr.table.partitions[3].end_address < g_flash_total_size);
    TEST_ASSERT_EQUAL_UINT32(0U, g_mgr.table.partitions[0].start_address);
    TEST_ASSERT_EQUAL_UINT32(256U * 1024U, g_mgr.table.partitions[1].start_address);
}

/** 8MB 器件: 固定布局 (约 13.25MB) 放不下, 必须在生成阶段拒绝 */
void test_DefaultTable_LargerThan8MbFlash_ShouldBeRejected(void)
{
    bl_partition_error_t rc = bl_partition_table_init_default(&g_mgr, 8U * 1024U * 1024U);

    TEST_ASSERT_EQUAL(BL_ERROR_INVALID_PARAM, rc);
}

/** get_info() 返回非法扇区大小 (0) 时不得把分区起点对齐到 0 */
void test_DefaultTable_ZeroSectorSize_ShouldFallBackToDefaultSector(void)
{
    g_flash_sector_size = 0U;

    bl_partition_error_t rc = bl_partition_table_init_default(&g_mgr, g_flash_total_size);

    TEST_ASSERT_EQUAL(BL_OK, rc);
    /* 默认 4KB 扇区: bootloader 之后 app_a 必须从 256KB 处开始, 而不是 0 */
    TEST_ASSERT_EQUAL_UINT32(256U * 1024U, g_mgr.table.partitions[1].start_address);
    TEST_ASSERT_TRUE(g_mgr.table.partitions[1].start_address >
                     g_mgr.table.partitions[0].end_address);
}

/** 非 2 的幂扇区大小同样不可用于对齐运算 */
void test_DefaultTable_NonPowerOfTwoSectorSize_ShouldFallBackToDefaultSector(void)
{
    g_flash_sector_size = 3000U;

    bl_partition_error_t rc = bl_partition_table_init_default(&g_mgr, g_flash_total_size);

    TEST_ASSERT_EQUAL(BL_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(256U * 1024U, g_mgr.table.partitions[1].start_address);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_DefaultTable_Fits16MbFlash_ShouldSucceed);
    RUN_TEST(test_DefaultTable_LargerThan8MbFlash_ShouldBeRejected);
    RUN_TEST(test_DefaultTable_ZeroSectorSize_ShouldFallBackToDefaultSector);
    RUN_TEST(test_DefaultTable_NonPowerOfTwoSectorSize_ShouldFallBackToDefaultSector);

    return UNITY_END();
}
