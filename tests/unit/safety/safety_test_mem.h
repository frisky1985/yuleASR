/******************************************************************************
 * @file    safety_test_mem.h
 * @brief   Test helper: 32-bit addressable RAM pool for saferam unit tests
 *
 * The saferam modules address memory through 32-bit integers (guard zones,
 * stack canaries, heap pool, fault-injection targets).  On Apple Silicon the
 * default 4 GB __PAGEZERO makes every low address unmappable, so the test
 * executables are built as x86_64 with -Wl,-pagezero_size,0x1000 (see
 * CMakeLists.txt) and map a private RAM pool at 0x20000000 via mmap(MAP_FIXED).
 * All uint32_t addresses handed to the module under test must point into this
 * pool.
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 ******************************************************************************/

#ifndef SAFETY_TEST_MEM_H
#define SAFETY_TEST_MEM_H

#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* RAM pool: 16 MB, 4 KB aligned sub-regions */
#define ST_POOL_BASE    0x20000000u
#define ST_POOL_SIZE    0x01000000u

/* Sub-regions (all 4 KB aligned, well separated for guard zones) */
#define ST_PART_BUF     0x20040000u  /* partitions + their guard zones      */
#define ST_STACK_BUF    0x20100000u  /* stack canary / watermark memory     */
#define ST_HEAP_BUF     0x20200000u  /* heap pool (32 KB used)              */
#define ST_FI_BUF       0x20400000u  /* fault-injection target memory       */

static void *st_pool_map(void)
{
    void *p = mmap((void *)(uintptr_t)ST_POOL_BASE, ST_POOL_SIZE,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
    if (p == MAP_FAILED) {
        return NULL;
    }
    return p;
}

/* Helper: run at the top of main(); aborts the process if the pool cannot
 * be mapped (e.g. wrong arch/pagezero configuration). */
static void st_require_pool(void)
{
    if (st_pool_map() == NULL) {
        perror("safety_test_mem: cannot map 32-bit RAM pool at "
               "0x20000000 (build as x86_64 with -Wl,-pagezero_size,0x1000)");
        exit(1);
    }
}

#endif /* SAFETY_TEST_MEM_H */
