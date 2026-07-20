/**
 * @file test_buffer_pool_coverage.c
 * @brief Comprehensive buffer pool unit tests
 *
 * Tests all paths in src/micro-dds/src/utils/buffer_pool.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "unity.h"

/* Functions to test */
extern bool MicroDDS_BufferPool_Init(void);
extern void MicroDDS_BufferPool_Shutdown(void);
extern void* MicroDDS_BufferPool_Alloc(void);
extern void MicroDDS_BufferPool_Free(void* buffer);
extern uint16_t MicroDDS_BufferPool_GetBufferSize(void);
extern uint32_t MicroDDS_BufferPool_GetAvailableCount(void);

void setUp(void) { MicroDDS_BufferPool_Init(); }
void tearDown(void) { MicroDDS_BufferPool_Shutdown(); }

void test_Init(void)
{
    /* Already initialized by setUp */
    bool r = MicroDDS_BufferPool_Init();
    TEST_ASSERT_TRUE(r);
}

void test_GetBufferSize(void)
{
    TEST_ASSERT_EQUAL_UINT16(512, MicroDDS_BufferPool_GetBufferSize());
}

void test_AllocAndFree(void)
{
    void* b = MicroDDS_BufferPool_Alloc();
    TEST_ASSERT_NOT_NULL(b);

    /* Write some data to verify buffer is usable */
    memset(b, 0xAA, 100);
    uint8_t* p = (uint8_t*)b;
    TEST_ASSERT_EQUAL_UINT8(0xAA, p[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, p[99]);

    MicroDDS_BufferPool_Free(b);

    /* After free, should be allocatable again */
    void* b2 = MicroDDS_BufferPool_Alloc();
    TEST_ASSERT_NOT_NULL(b2);
    TEST_ASSERT_TRUE(b == b2);  /* Same address (first free slot) */
}

void test_MultipleAllocs(void)
{
    void* bufs[8];
    uint32_t count = 0;

    for (int i = 0; i < 8; i++) {
        bufs[i] = MicroDDS_BufferPool_Alloc();
        if (bufs[i] != NULL) count++;
    }
    TEST_ASSERT_EQUAL_UINT32(8, count);  /* Pool size is 8 */

    /* Freed in reverse */
    for (int i = 7; i >= 0; i--) {
        MicroDDS_BufferPool_Free(bufs[i]);
    }
}

void test_AllocExhaustion(void)
{
    void* bufs[10];
    int i;
    int allocated = 0;

    for (i = 0; i < 10; i++) {
        bufs[i] = MicroDDS_BufferPool_Alloc();
        if (bufs[i] != NULL) allocated++;
    }

    TEST_ASSERT(allocated >= 8);  /* At least pool size */
    TEST_ASSERT(allocated <= 10);

    /* After exhaustion, at least one alloc should fail */
    if (allocated == 8) {
        void* extra = MicroDDS_BufferPool_Alloc();
        TEST_ASSERT_NULL(extra);
    }

    /* Cleanup */
    for (i = 0; i < allocated; i++) {
        MicroDDS_BufferPool_Free(bufs[i]);
    }
}

void test_FreeNull(void)
{
    /* Should not crash */
    MicroDDS_BufferPool_Free(NULL);
    TEST_ASSERT_TRUE(1);
}

void test_FreeTwice(void)
{
    void* b = MicroDDS_BufferPool_Alloc();
    TEST_ASSERT_NOT_NULL(b);
    MicroDDS_BufferPool_Free(b);
    /* Second free should not crash */
    MicroDDS_BufferPool_Free(b);
    TEST_ASSERT_TRUE(1);
}

void test_AvailableCount(void)
{
    uint32_t avail = MicroDDS_BufferPool_GetAvailableCount();
    TEST_ASSERT(avail > 0);
    TEST_ASSERT(avail <= 8);

    void* b = MicroDDS_BufferPool_Alloc();
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_UINT32(avail - 1, MicroDDS_BufferPool_GetAvailableCount());

    MicroDDS_BufferPool_Free(b);
    TEST_ASSERT_EQUAL_UINT32(avail, MicroDDS_BufferPool_GetAvailableCount());
}

void test_AllocFillBuffer(void)
{
    void* b = MicroDDS_BufferPool_Alloc();
    TEST_ASSERT_NOT_NULL(b);

    uint16_t sz = MicroDDS_BufferPool_GetBufferSize();
    memset(b, 0x55, sz);

    uint8_t* p = (uint8_t*)b;
    TEST_ASSERT_EQUAL_UINT8(0x55, p[0]);
    TEST_ASSERT_EQUAL_UINT8(0x55, p[sz - 1]);

    MicroDDS_BufferPool_Free(b);
}

void test_ShutdownAndRestart(void)
{
    MicroDDS_BufferPool_Shutdown();
    bool r = MicroDDS_BufferPool_Init();
    TEST_ASSERT_TRUE(r);

    void* b = MicroDDS_BufferPool_Alloc();
    TEST_ASSERT_NOT_NULL(b);
    MicroDDS_BufferPool_Free(b);
}

void test_AvailableCount_AfterExhaustion(void)
{
    void* bufs[10];
    int i, n;

    for (n = 0; n < 10; n++) {
        bufs[n] = MicroDDS_BufferPool_Alloc();
        if (bufs[n] == NULL) break;
    }

    TEST_ASSERT_EQUAL_UINT32(0, MicroDDS_BufferPool_GetAvailableCount());

    for (i = n - 1; i >= 0; i--) {
        MicroDDS_BufferPool_Free(bufs[i]);
    }

    TEST_ASSERT(MicroDDS_BufferPool_GetAvailableCount() > 0);
}

int main(void)
{
    UnityBegin();

    UnityRunTest(test_Init, "Init", __LINE__);
    UnityRunTest(test_GetBufferSize, "GetBufferSize", __LINE__);
    UnityRunTest(test_AllocAndFree, "AllocAndFree", __LINE__);
    UnityRunTest(test_MultipleAllocs, "MultipleAllocs", __LINE__);
    UnityRunTest(test_AllocExhaustion, "AllocExhaustion", __LINE__);
    UnityRunTest(test_FreeNull, "FreeNull", __LINE__);
    UnityRunTest(test_FreeTwice, "FreeTwice", __LINE__);
    UnityRunTest(test_AvailableCount, "AvailableCount", __LINE__);
    UnityRunTest(test_AllocFillBuffer, "AllocFillBuffer", __LINE__);
    UnityRunTest(test_ShutdownAndRestart, "ShutdownRestart", __LINE__);
    UnityRunTest(test_AvailableCount_AfterExhaustion, "AvailCountExhaust", __LINE__);

    return UnityEnd();
}
