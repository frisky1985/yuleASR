/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Mock HAL — Implementation
*
* SW Version           : 1.0.0
* Build Date           : 2026-07-26
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* @file mock_hal.c
* @brief Memory-backed register table implementation
*
* Design:
*   1M-entry flat array maps register addresses to stored values.
*   Index = (addr >> 2) & 0xFFFFF — gives 1M entries covering 4 MB of
*   address space with 4-byte granularity.
*
*   A separate expectations array tracks expected writes for test verification.
==================================================================================================*/

#include "mock_hal.h"
#include <string.h>
#include <stdio.h>

/*==================================================================================================
*                                      CONSTANTS
==================================================================================================*/
#define MOCK_HAL_EXPECT_MAX             256U

/*==================================================================================================
*                                      INTERNAL STATE
==================================================================================================*/

/* Main register table — 1M entries, initialized to 0 */
static uint32_t mock_hal_reg_table[MOCK_HAL_TABLE_SIZE];

/* Default values — set by mock_hal_set_default, preserved across reset */
static uint32_t mock_hal_default_table[MOCK_HAL_TABLE_SIZE];

/* Statistics per address */
static uint32_t mock_hal_stat_rcnt[MOCK_HAL_TABLE_SIZE];
static uint32_t mock_hal_stat_wcnt[MOCK_HAL_TABLE_SIZE];

/* Global statistics */
static uint32_t mock_hal_total_reads_val = 0;
static uint32_t mock_hal_total_writes_val = 0;

/* Expectations */
static struct {
    uintptr_t addr;
    uint32_t  val;
    bool      pending;
} mock_hal_expects[MOCK_HAL_EXPECT_MAX];
static uint32_t mock_hal_expect_count = 0;

/*==================================================================================================
*                                      INTERNAL HELPERS
==================================================================================================*/

/**
 * @brief Map register address to table index
 * Uses (addr >> 2) masked to table size.
 * 4-byte aligned, covers 4 MB address span.
 */
static inline uint32_t mock_hal_index(uintptr_t addr)
{
    return ((uint32_t)(addr >> 2U) & MOCK_HAL_ADDR_MASK);
}

/*==================================================================================================
*                                      PUBLIC API
==================================================================================================*/

void mock_hal_reset(void)
{
    /* Zero the register table */
    memset(mock_hal_reg_table, 0, sizeof(mock_hal_reg_table));

    /* Re-apply defaults */
    {
        uint32_t i;
        for (i = 0; i < MOCK_HAL_TABLE_SIZE; i++) {
            if (mock_hal_default_table[i] != 0U) {
                mock_hal_reg_table[i] = mock_hal_default_table[i];
            }
        }
    }

    /* Zero statistics */
    memset(mock_hal_stat_rcnt, 0, sizeof(mock_hal_stat_rcnt));
    memset(mock_hal_stat_wcnt, 0, sizeof(mock_hal_stat_wcnt));
    mock_hal_total_reads_val = 0;
    mock_hal_total_writes_val = 0;

    /* Clear expectations */
    memset(mock_hal_expects, 0, sizeof(mock_hal_expects));
    mock_hal_expect_count = 0;
}

void mock_hal_set_default(uintptr_t addr, uint32_t val)
{
    uint32_t idx = mock_hal_index(addr);
    mock_hal_default_table[idx] = val;
    /* Also preload the register table so reads return the default */
    mock_hal_reg_table[idx] = val;
}

void mock_hal_set_defaults(const uint32_t (*table)[2], uint32_t count)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        const uint32_t *entry = table[i];
        uintptr_t addr = (uintptr_t)entry[0];
        uint32_t val = entry[1];

        /* Stop at sentinel (addr=0, val=0) */
        if (addr == 0U && val == 0U) {
            break;
        }

        mock_hal_set_default(addr, val);
    }
}

uint32_t mock_hal_read32(uintptr_t addr)
{
    uint32_t idx = mock_hal_index(addr);

    /* Increment statistics */
    mock_hal_stat_rcnt[idx]++;
    mock_hal_total_reads_val++;

    return mock_hal_reg_table[idx];
}

void mock_hal_write32(uintptr_t addr, uint32_t val)
{
    uint32_t idx = mock_hal_index(addr);

    /* Increment statistics and store */
    mock_hal_stat_wcnt[idx]++;
    mock_hal_reg_table[idx] = val;
    mock_hal_total_writes_val++;

    /* Check expectations */
    uint32_t i;
    for (i = 0; i < mock_hal_expect_count; i++) {
        if (mock_hal_expects[i].pending &&
            mock_hal_expects[i].addr == addr &&
            mock_hal_expects[i].val == val) {
            mock_hal_expects[i].pending = false;
            break;
        }
    }
}

uint16_t mock_hal_read16(uintptr_t addr)
{
    uint32_t full = mock_hal_read32(addr);
    if (addr & 0x02U) {
        return (uint16_t)(full >> 16U);
    }
    return (uint16_t)(full & 0xFFFFU);
}

void mock_hal_write16(uintptr_t addr, uint16_t val)
{
    uint32_t idx = mock_hal_index(addr);
    uint32_t full = mock_hal_reg_table[idx];

    if (addr & 0x02U) {
        full = (full & 0x0000FFFFU) | ((uint32_t)val << 16U);
    } else {
        full = (full & 0xFFFF0000U) | (uint32_t)val;
    }
    mock_hal_reg_table[idx] = full;
    mock_hal_total_writes_val++;
}

uint8_t mock_hal_read8(uintptr_t addr)
{
    uint32_t full = mock_hal_read32(addr);
    uint8_t shift = (uint8_t)((addr & 0x03U) * 8U);
    return (uint8_t)((full >> shift) & 0xFFU);
}

void mock_hal_write8(uintptr_t addr, uint8_t val)
{
    uint32_t idx = mock_hal_index(addr);
    uint32_t full = mock_hal_reg_table[idx];
    uint8_t shift = (uint8_t)((addr & 0x03U) * 8U);
    uint32_t mask = ~((uint32_t)0xFFU << shift);

    full = (full & mask) | ((uint32_t)val << shift);
    mock_hal_reg_table[idx] = full;
    mock_hal_total_writes_val++;
}

void mock_hal_expect_write(uintptr_t addr, uint32_t val)
{
    if (mock_hal_expect_count < MOCK_HAL_EXPECT_MAX) {
        mock_hal_expects[mock_hal_expect_count].addr = addr;
        mock_hal_expects[mock_hal_expect_count].val = val;
        mock_hal_expects[mock_hal_expect_count].pending = true;
        mock_hal_expect_count++;
    }
}

bool mock_hal_verify(void)
{
    bool all_met = true;
    uint32_t i;

    for (i = 0; i < mock_hal_expect_count; i++) {
        if (mock_hal_expects[i].pending) {
            fprintf(stderr, "[MOCK_HAL] EXPECTATION FAILED: "
                    "addr=0x%08lX val=0x%08lX was never written\n",
                    (unsigned long)mock_hal_expects[i].addr,
                    (unsigned long)mock_hal_expects[i].val);
            all_met = false;
        }
    }

    return all_met;
}

uint32_t mock_hal_read_count(uintptr_t addr)
{
    return mock_hal_stat_rcnt[mock_hal_index(addr)];
}

uint32_t mock_hal_write_count(uintptr_t addr)
{
    return mock_hal_stat_wcnt[mock_hal_index(addr)];
}

uint32_t mock_hal_total_reads(void)
{
    return mock_hal_total_reads_val;
}

uint32_t mock_hal_total_writes(void)
{
    return mock_hal_total_writes_val;
}
