/******************************************************************************
 * @file    test_ram_partition.c
 * @brief   Unit tests for ram_partition.c (safe RAM partitioning)
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 ******************************************************************************/

#include "unity.h"
#include "ram_partition.h"
#include "safety_test_mem.h"
#include <string.h>

/* Guard zones need real memory 256 bytes below/above the partition, and the
 * guard check writes/reads through uint32_t pointers: the partition buffer
 * therefore lives inside the 32-bit addressable test pool. */
#define P0_START (ST_PART_BUF + 0x1000u)
#define P0_SIZE  0x0400u
#define P1_START (ST_PART_BUF + 0x2000u)
#define P1_SIZE  0x0400u

static uint8_t g_id0;
static uint8_t g_id1;
static uint32_t g_cb_violations;

static RamPartitionConfigType MakeCfg(const char *name, uint32_t start,
                                      uint32_t size, uint8_t perms,
                                      boolean guards)
{
    RamPartitionConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.name, name, RAM_PARTITION_NAME_MAX_LEN - 1u);
    cfg.type = RAM_PARTITION_TYPE_STATIC;
    cfg.attributes = RAM_PARTITION_ATTR_READ | RAM_PARTITION_ATTR_WRITE;
    cfg.safety_level = RAM_PARTITION_LEVEL_GREEN;
    cfg.start_addr = start;
    cfg.size = size;
    cfg.permissions = perms;
    cfg.enable_guard_zones = guards;
    return cfg;
}


/* Register via the E_OK path and return the assigned id. */
static uint8_t Reg(const char *name, uint32_t start, uint32_t size,
                   uint8_t perms, boolean guards)
{
    RamPartitionConfigType cfg = MakeCfg(name, start, size, perms, guards);
    uint8_t id = 0xEEu;
    TEST_ASSERT_EQUAL(E_OK, RamPartition_Register(&cfg, &id));
    return id;
}

void setUp(void)
{
    (void)st_pool_map();
    RamPartition_DeInit();
    (void)RamPartition_Init();
    g_id0 = 0xFFu;
    g_id1 = 0xFFu;
    g_cb_violations = 0u;
}

void tearDown(void)
{
    /* PRODUCT QUIRK (not fixed here): RamPartition_DeInit() loops
     * "while (count > 0) Unregister(partitions[0].id)".  Unregistering
     * slot 0 leaves an empty slot 0, so once two or more partitions are
     * registered the loop spins forever.  Drain every slot explicitly
     * (descending) so DeInit never enters the loop with count > 1. */
    {
        int i;
        for (i = RAM_PARTITION_MAX_COUNT - 1; i >= 0; i--) {
            (void)RamPartition_Unregister((uint8_t)i);
        }
    }
    RamPartition_DeInit();
}

static void cb_counter(const RamPartitionViolationType *v)
{
    (void)v;
    g_cb_violations++;
}

/* ---------------- lifecycle ---------------- */

void test_init_deinit_cycle(void)
{
    TEST_ASSERT_EQUAL(E_OK, RamPartition_Init());
    TEST_ASSERT_EQUAL_UINT8(0, RamPartition_GetCount());
    TEST_ASSERT_EQUAL(E_OK, RamPartition_DeInit());
    TEST_ASSERT_EQUAL_UINT8(0, RamPartition_GetCount());
    /* re-init after de-init works */
    TEST_ASSERT_EQUAL(E_OK, RamPartition_Init());
}

void test_api_rejected_before_init(void)
{
    TEST_ASSERT_EQUAL(E_OK, RamPartition_DeInit());
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Register(
        &(RamPartitionConfigType){0}, &g_id0));
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Unregister(0));
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_GetInfo(0, NULL));
}

/* ---------------- registration ---------------- */

void test_register_valid_and_info(void)
{
    RamPartitionConfigType cfg = MakeCfg("P0", P0_START, P0_SIZE,
                                         RAM_PERM_READ | RAM_PERM_WRITE, TRUE);
    RamPartitionInfoType info;

    TEST_ASSERT_EQUAL(E_OK, RamPartition_Register(&cfg, &g_id0));
    TEST_ASSERT_EQUAL_UINT8(0, g_id0);
    TEST_ASSERT_EQUAL_UINT8(1, RamPartition_GetCount());

    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetInfo(g_id0, &info));
    TEST_ASSERT_EQUAL_STRING("P0", info.name);
    TEST_ASSERT_EQUAL_UINT32(P0_START, info.start_addr);
    TEST_ASSERT_EQUAL_UINT32(P0_START + P0_SIZE - 1u, info.end_addr);
    TEST_ASSERT_EQUAL_UINT32(P0_SIZE, info.size);
    TEST_ASSERT_EQUAL(PARTITION_STATE_ACTIVE, info.state);
    /* guard zones frame the partition by exactly RAM_GUARD_ZONE_SIZE */
    TEST_ASSERT_EQUAL_UINT32(P0_START - RAM_GUARD_ZONE_SIZE, info.guard_bottom);
    TEST_ASSERT_EQUAL_UINT32(P0_START + P0_SIZE, info.guard_top);
}

void test_register_null_guards(void)
{
    uint8_t id = 0xEEu;
    RamPartitionConfigType cfg = MakeCfg("P0", P0_START, P0_SIZE,
                                         RAM_PERM_READ | RAM_PERM_WRITE, TRUE);
    TEST_ASSERT_EQUAL(E_OK, RamPartition_Register(&cfg, &g_id0));

    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Register(NULL, &id));
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Register(&cfg, NULL));
    TEST_ASSERT_EQUAL_UINT8(1, RamPartition_GetCount());
}

void test_register_invalid_config(void)
{
    uint8_t id;
    RamPartitionConfigType cfg;

    /* empty name */
    cfg = MakeCfg("", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Register(&cfg, &id));

    /* zero size */
    cfg = MakeCfg("P0", P0_START, 0u, RAM_PERM_READ, FALSE);
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Register(&cfg, &id));

    /* null start address */
    cfg = MakeCfg("P0", 0u, P0_SIZE, RAM_PERM_READ, FALSE);
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Register(&cfg, &id));

    /* unknown safety level */
    cfg = MakeCfg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);
    cfg.safety_level = 3u;
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Register(&cfg, &id));

    TEST_ASSERT_EQUAL_UINT8(0, RamPartition_GetCount());
}

void test_validate_config_util(void)
{
    RamPartitionConfigType cfg = MakeCfg("P0", P0_START, P0_SIZE,
                                         RAM_PERM_READ, FALSE);
    TEST_ASSERT_EQUAL(E_OK, RamPartition_ValidateConfig(&cfg));
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_ValidateConfig(NULL));
    cfg.size = 0u;
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_ValidateConfig(&cfg));
}

void test_register_overlap_rejected(void)
{
    uint8_t id;
    RamPartitionConfigType ov;
    /* P1 overlaps the tail of P0 */
    g_id0 = Reg("P0", P0_START, 0x1000u, RAM_PERM_READ, FALSE);
    ov = MakeCfg("P1", P0_START + 0x0F00u, P1_SIZE, RAM_PERM_READ, FALSE);
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Register(&ov, &id));
    TEST_ASSERT_EQUAL_UINT8(1, RamPartition_GetCount());
}

void test_register_adjacent_ok(void)
{
    /* P1 starts exactly at P0 end + 1: no overlap */
    g_id0 = Reg("P0", P0_START, 0x1000u, RAM_PERM_READ, FALSE);
    g_id1 = Reg("P1", P0_START + 0x1000u, P1_SIZE, RAM_PERM_READ, FALSE);
    TEST_ASSERT_EQUAL_UINT8(2, RamPartition_GetCount());
}

void test_unregister(void)
{
    RamPartitionInfoType info;
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);

    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Unregister(0xF0u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_Unregister(g_id0));
    TEST_ASSERT_EQUAL_UINT8(0, RamPartition_GetCount());
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_GetInfo(g_id0, &info));
    /* double unregister */
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Unregister(g_id0));
}

void test_get_info_invalid_id(void)
{
    RamPartitionInfoType info;
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_GetInfo(0, &info));
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_GetInfo(0xF0u, &info));
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_GetInfo(g_id0, NULL));
}

void test_get_table(void)
{
    RamPartitionTableType table;
    memset(&table, 0, sizeof(table));
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_GetTable(NULL));
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetTable(&table));
    TEST_ASSERT_EQUAL_UINT8(1, table.count);
    TEST_ASSERT_EQUAL(TRUE, table.initialized);
    TEST_ASSERT_EQUAL_STRING("P0", table.partitions[0].name);
}

/* ---------------- permissions / locking ---------------- */

void test_set_permissions(void)
{
    RamPartitionInfoType info;
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_SetPermissions(0xF0u, RAM_PERM_READ));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_SetPermissions(
        g_id0, RAM_PERM_READ | RAM_PERM_WRITE));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetInfo(g_id0, &info));
    TEST_ASSERT_EQUAL_UINT8(RAM_PERM_READ | RAM_PERM_WRITE, info.permissions);

    /* locked partition refuses permission changes */
    TEST_ASSERT_EQUAL(E_OK, RamPartition_Lock(g_id0));
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_SetPermissions(g_id0, RAM_PERM_READ));
}

void test_lock_unlock(void)
{
    RamPartitionInfoType info;
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);

    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Lock(0xF0u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_Lock(g_id0));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetInfo(g_id0, &info));
    TEST_ASSERT_EQUAL(PARTITION_STATE_LOCKED, info.state);

    TEST_ASSERT_EQUAL(E_OK, RamPartition_Unlock(g_id0));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetInfo(g_id0, &info));
    TEST_ASSERT_EQUAL(PARTITION_STATE_ACTIVE, info.state);
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Unlock(0xF0u));
}

/* ---------------- usage / bounds ---------------- */

void test_update_usage_and_peak(void)
{
    RamPartitionInfoType info;
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);

    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_UpdateUsage(0xF0u, 10u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_UpdateUsage(g_id0, 0x0100u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_UpdateUsage(g_id0, 0x0080u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetInfo(g_id0, &info));
    TEST_ASSERT_EQUAL_UINT32(0x0080u, info.used_size);
    TEST_ASSERT_EQUAL_UINT32(0x0100u, info.peak_usage);

    /* NOTE(actual behaviour): UpdateUsage() accepts over-use silently -
     * it writes used_size, bumps the peak and returns E_OK without any
     * violation.  Overflow detection is CheckBounds()'s job only. */
    TEST_ASSERT_EQUAL(E_OK, RamPartition_UpdateUsage(g_id0, P0_SIZE + 1u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetInfo(g_id0, &info));
    TEST_ASSERT_EQUAL_UINT32(P0_SIZE + 1u, info.used_size);
    TEST_ASSERT_EQUAL(PARTITION_STATE_ACTIVE, info.state);
    TEST_ASSERT_EQUAL_UINT32(0u, info.violation_count);
}

void test_check_bounds(void)
{
    boolean ok = TRUE;
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_CheckBounds(0xF0u, &ok));
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);
    /* NOTE: a NULL result pointer is rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_CheckBounds(g_id0, NULL));

    TEST_ASSERT_EQUAL(E_OK, RamPartition_UpdateUsage(g_id0, 0x0010u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckBounds(g_id0, &ok));
    TEST_ASSERT_EQUAL(TRUE, ok);

    TEST_ASSERT_EQUAL(E_OK, RamPartition_UpdateUsage(g_id0, P0_SIZE + 4u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckBounds(g_id0, &ok));
    TEST_ASSERT_EQUAL(FALSE, ok);
}

/* ---------------- guard zones (real memory in the 32-bit pool) ---------- */

void test_guard_zones_intact(void)
{
    boolean ok = FALSE;
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, TRUE);

    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_CheckGuardZones(0xF0u, &ok));
    /* NOTE: a NULL result pointer is rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_CheckGuardZones(g_id0, NULL));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckGuardZones(g_id0, &ok));
    TEST_ASSERT_EQUAL(TRUE, ok);

    /* manual re-initialisation of the guard pattern keeps them intact */
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_InitGuardZones(0xF0u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_InitGuardZones(g_id0));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckGuardZones(g_id0, &ok));
    TEST_ASSERT_EQUAL(TRUE, ok);
}

void test_guard_zone_corruption_detected(void)
{
    boolean ok = TRUE;
    RamPartitionInfoType info;

    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, TRUE);

    /* flip one word in the bottom guard zone (start - 256) */
    *(volatile uint32_t *)(uintptr_t)(P0_START - 256u) = 0x12345678u;
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckGuardZones(g_id0, &ok));
    TEST_ASSERT_EQUAL(FALSE, ok);
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetInfo(g_id0, &info));
    TEST_ASSERT_EQUAL(PARTITION_STATE_VIOLATED, info.state);
    TEST_ASSERT_EQUAL_UINT32(1u, info.violation_count);

    /* restore and re-init: clean again */
    TEST_ASSERT_EQUAL(E_OK, RamPartition_InitGuardZones(g_id0));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckGuardZones(g_id0, &ok));
    TEST_ASSERT_EQUAL(TRUE, ok);
}

void test_check_all_guard_zones(void)
{
    uint8_t id2;

    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, TRUE);
    id2 = Reg("P1", P1_START, P1_SIZE, RAM_PERM_READ, TRUE);

    TEST_ASSERT_EQUAL_UINT32(0u, RamPartition_CheckAllGuardZones());

    *(volatile uint32_t *)(uintptr_t)(P1_START + P1_SIZE) = 0xCCCCCCCCu; /* top guard of P1 */
    TEST_ASSERT_EQUAL_UINT32(1u, RamPartition_CheckAllGuardZones());
    (void)id2;
}

void test_verify_integrity(void)
{
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, TRUE);
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_VerifyIntegrity(0xF0u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_VerifyIntegrity(g_id0));

    /* smash inside the bottom guard zone ([start-256, start)) */
    *(volatile uint32_t *)(uintptr_t)(P0_START - 128u) = 0xBAD0BAD0u;
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_VerifyIntegrity(g_id0));
}

void test_guard_disabled_partition(void)
{
    /* without guard zones nothing is written outside [start, end] */
    boolean ok = FALSE;
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckGuardZones(g_id0, &ok));
    TEST_ASSERT_EQUAL(TRUE, ok);
    /* guard pointers stay 0 when disabled */
    TEST_ASSERT_EQUAL(E_OK, RamPartition_VerifyIntegrity(g_id0));
}

/* ---------------- access control ---------------- */

void test_check_access_permissions(void)
{
    RamAccessCheckResultType res;

    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_CheckAccess(P0_START, 4u,
                                                         RAM_PERM_READ, NULL));
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_CheckAccess(P0_START, 0u,
                                                         RAM_PERM_READ, &res));

    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ | RAM_PERM_WRITE, FALSE);

    memset(&res, 0, sizeof(res));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckAccess(P0_START, 4u,
                                                     RAM_PERM_READ, &res));
    TEST_ASSERT_EQUAL(TRUE, res.allowed);
    TEST_ASSERT_EQUAL_UINT8(g_id0, res.partition_id);

    /* exec denied */
    memset(&res, 0, sizeof(res));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckAccess(P0_START, 4u,
                                                     RAM_PERM_EXEC, &res));
    TEST_ASSERT_EQUAL(FALSE, res.allowed);

    /* range partially outside the partition is not "found" */
    memset(&res, 0, sizeof(res));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckAccess(P0_START + P0_SIZE - 2u, 4u,
                                                     RAM_PERM_READ, &res));
    TEST_ASSERT_EQUAL(FALSE, res.allowed);
    TEST_ASSERT_EQUAL_UINT8(0xFFu, res.partition_id);
}

void test_check_access_unknown_address(void)
{
    RamAccessCheckResultType res;
    memset(&res, 0, sizeof(res));
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ | RAM_PERM_WRITE, FALSE);

    /* NOTE(actual behaviour): the header comment says "allowed by default",
     * but the implementation leaves result->allowed == FALSE for addresses
     * outside every partition.  Asserting the real behaviour here. */
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckAccess(0x7FFF0000u, 4u,
                                                     RAM_PERM_READ, &res));
    TEST_ASSERT_EQUAL(FALSE, res.allowed);
    TEST_ASSERT_EQUAL_UINT8(0xFFu, res.partition_id);
}

void test_find_by_address(void)
{
    uint8_t found = 0xFFu;
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_FindByAddress(P0_START, NULL));
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);

    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_FindByAddress(0x7FFF0000u, &found));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_FindByAddress(P0_START + 0x10u, &found));
    TEST_ASSERT_EQUAL_UINT8(g_id0, found);
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_FindByAddress(P0_START + P0_SIZE, &found));
}

void test_get_safety_level(void)
{
    uint8_t level = 0xEEu;
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_GetSafetyLevel(0xF0u, &level));
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);
    /* NOTE: a NULL level pointer is rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_GetSafetyLevel(g_id0, NULL));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetSafetyLevel(g_id0, &level));
    TEST_ASSERT_EQUAL_UINT8(RAM_PARTITION_LEVEL_GREEN, level);
}

/* ---------------- MPU integration ---------------- */

void test_mpu_configure_enable_disable(void)
{
    RamMpuRegionConfigType mpu;
    memset(&mpu, 0, sizeof(mpu));
    mpu.base_addr = P0_START;
    mpu.size = P0_SIZE;
    mpu.attributes = RAM_PARTITION_ATTR_MPU;
    mpu.permissions = RAM_PERM_READ;
    mpu.region_num = 3u;
    mpu.enable = TRUE;

    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);

    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_ConfigureMpu(0xF0u, &mpu));
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_ConfigureMpu(g_id0, NULL));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_ConfigureMpu(g_id0, &mpu));

    /* enabling without prior configuration is rejected */
    g_id1 = Reg("P1", P1_START, P1_SIZE, RAM_PERM_READ, FALSE);
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_EnableMpuProtection(g_id1));

    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_EnableMpuProtection(0xF0u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_EnableMpuProtection(g_id0));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_DisableMpuProtection(g_id0));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_DisableMpuProtection(g_id1));
}

/* ---------------- violations ---------------- */

void test_violation_callback_and_last_violation(void)
{
    RamPartitionViolationType v;
    RamPartitionViolationType got;
    RamPartitionInfoType info;

    memset(&v, 0, sizeof(v));
    v.partition_id = 0u;
    v.violation_type = RAM_PARTITION_ERR_ACCESS_DENY;
    v.violation_addr = P0_START;
    v.access_type = RAM_PERM_WRITE;
    v.timestamp = 0x1234u;
    v.task_id = 7u;

    RamPartition_SetViolationCallback(cb_counter);
    RamPartition_SetViolationCallback(NULL); /* unregister is accepted */

    RamPartition_SetViolationCallback(cb_counter);
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_GetLastViolation(NULL));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetLastViolation(&got));
    TEST_ASSERT_EQUAL_UINT8(0, got.partition_id); /* nothing reported yet */

    /* partition must exist so the state flip is observable */
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, FALSE);

    RamPartition_HandleViolation(NULL); /* ignored */
    RamPartition_HandleViolation(&v);

    TEST_ASSERT_EQUAL_UINT32(1u, g_cb_violations);
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetLastViolation(&got));
    TEST_ASSERT_EQUAL_UINT8(0u, got.partition_id);
    TEST_ASSERT_EQUAL_UINT8(RAM_PARTITION_ERR_ACCESS_DENY, got.violation_type);
    TEST_ASSERT_EQUAL_UINT32(0x1234u, got.timestamp);

    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetInfo(g_id0, &info));
    /* NOTE(actual behaviour): HandleViolation() only flips the state to
     * VIOLATED, stores the last violation and calls the callback - it does
     * NOT bump partition->violation_count (only the check APIs count). */
    TEST_ASSERT_EQUAL_UINT32(0u, info.violation_count);
    TEST_ASSERT_EQUAL(PARTITION_STATE_VIOLATED, info.state);

    /* NOTE(actual behaviour): a VIOLATED partition stays VIOLATED -
     * Unlock() refuses to recover it. */
    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_Unlock(g_id0));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetInfo(g_id0, &info));
    TEST_ASSERT_EQUAL(PARTITION_STATE_VIOLATED, info.state);
}

/* ---------------- stats / utils ---------------- */

void test_reset_stats(void)
{
    RamPartitionInfoType info;
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ | RAM_PERM_WRITE, FALSE);
    (void)RamPartition_UpdateUsage(g_id0, 0x20u);
    TEST_ASSERT_EQUAL(E_OK, RamPartition_CheckAccess(P0_START, 4u,
                                                     RAM_PERM_READ, &(RamAccessCheckResultType){0}));

    TEST_ASSERT_EQUAL(E_NOT_OK, RamPartition_ResetStats(0xF0u));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_ResetStats(g_id0));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetInfo(g_id0, &info));
    TEST_ASSERT_EQUAL_UINT32(0u, info.access_count);
    TEST_ASSERT_EQUAL_UINT32(0u, info.violation_count);
    /* NOTE(actual behaviour): ResetStats() clears access/violation counters
     * only - peak_usage survives (single-id and 0xFF branches alike). */
    TEST_ASSERT_EQUAL_UINT32(0x20u, info.peak_usage);

    /* NOTE: partition_id == 0xFF resets all partitions at once, but
     * PRODUCT QUIRK: the 0xFF branch clears access/violation counters only,
     * peak_usage is left untouched (unlike the single-id branch). */
    (void)RamPartition_UpdateUsage(g_id0, 0x20u);
    TEST_ASSERT_EQUAL(E_OK, RamPartition_ResetStats(0xFFu));
    TEST_ASSERT_EQUAL(E_OK, RamPartition_GetInfo(g_id0, &info));
    TEST_ASSERT_EQUAL_UINT32(0u, info.access_count);
    TEST_ASSERT_EQUAL_UINT32(0x20u, info.peak_usage);
}

void test_check_overlap_util(void)
{
    /* identical range overlaps */
    TEST_ASSERT_EQUAL(TRUE, RamPartition_CheckOverlap(0x1000u, 0x100u,
                                                      0x1000u, 0x100u));
    /* partial overlap */
    TEST_ASSERT_EQUAL(TRUE, RamPartition_CheckOverlap(0x1000u, 0x100u,
                                                      0x1050u, 0x100u));
    /* containment */
    TEST_ASSERT_EQUAL(TRUE, RamPartition_CheckOverlap(0x1000u, 0x100u,
                                                      0x1080u, 0x10u));
    /* adjacent does NOT overlap ([0x1000,0x10FF] vs [0x1100,...]) */
    TEST_ASSERT_EQUAL(FALSE, RamPartition_CheckOverlap(0x1000u, 0x100u,
                                                       0x1100u, 0x100u));
    /* disjoint */
    TEST_ASSERT_EQUAL(FALSE, RamPartition_CheckOverlap(0x1000u, 0x100u,
                                                       0x9000u, 0x100u));
}

void test_full_check_clean(void)
{
    g_id0 = Reg("P0", P0_START, P0_SIZE, RAM_PERM_READ, TRUE);
    /* FullCheck() scans every partition and returns the violation count */
    TEST_ASSERT_EQUAL_UINT32(0u, RamPartition_FullCheck());
}

void test_version_info(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0xAA, sizeof(vi));
    RamPartition_GetVersionInfo(NULL); /* tolerated */
    RamPartition_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT16(0u, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT16(0u, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8(RAM_PARTITION_VERSION_MAJOR, vi.sw_major_version);
}

int main(void)
{
    st_require_pool();
    UNITY_BEGIN();
    RUN_TEST(test_init_deinit_cycle);
    RUN_TEST(test_api_rejected_before_init);
    RUN_TEST(test_register_valid_and_info);
    RUN_TEST(test_register_null_guards);
    RUN_TEST(test_register_invalid_config);
    RUN_TEST(test_validate_config_util);
    RUN_TEST(test_register_overlap_rejected);
    RUN_TEST(test_register_adjacent_ok);
    RUN_TEST(test_unregister);
    RUN_TEST(test_get_info_invalid_id);
    RUN_TEST(test_get_table);
    RUN_TEST(test_set_permissions);
    RUN_TEST(test_lock_unlock);
    RUN_TEST(test_update_usage_and_peak);
    RUN_TEST(test_check_bounds);
    RUN_TEST(test_guard_zones_intact);
    RUN_TEST(test_guard_zone_corruption_detected);
    RUN_TEST(test_check_all_guard_zones);
    RUN_TEST(test_verify_integrity);
    RUN_TEST(test_guard_disabled_partition);
    RUN_TEST(test_check_access_permissions);
    RUN_TEST(test_check_access_unknown_address);
    RUN_TEST(test_find_by_address);
    RUN_TEST(test_get_safety_level);
    RUN_TEST(test_mpu_configure_enable_disable);
    RUN_TEST(test_violation_callback_and_last_violation);
    RUN_TEST(test_reset_stats);
    RUN_TEST(test_check_overlap_util);
    RUN_TEST(test_full_check_clean);
    RUN_TEST(test_version_info);
    return UNITY_END();
}
