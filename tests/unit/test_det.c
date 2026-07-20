/**
 * @file test_det_coverage.c
 * @brief Det (Development Error Tracer) unit test
 *
 * Tests the real Det.c module with actual assertions.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "unity.h"

/* Include the real Det header */
#include "Det.h"

void test_Det_InitAndStart(void)
{
    Det_Init(NULL);
    Det_Start();
    TEST_ASSERT_TRUE(1);  /* no crash = pass */
}

void test_Det_ReportError(void)
{
    Det_Init(NULL);
    Det_Start();
    Std_ReturnType r = Det_ReportError(15, 0, 1, 2);
    TEST_ASSERT_EQUAL(E_OK, r);
}

void test_Det_ReportRuntimeError(void)
{
    Std_ReturnType r = Det_ReportRuntimeError(100, 1, 5, 9);
    TEST_ASSERT_EQUAL(E_OK, r);
}

void test_Det_ReportTransientFault(void)
{
    Std_ReturnType r = Det_ReportTransientFault(200, 2, 7, 3);
    TEST_ASSERT_EQUAL(E_OK, r);
}

void test_Det_GetVersionInfo(void)
{
    Std_VersionInfoType vi;
    Det_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(100U, vi.vendorID);
    TEST_ASSERT_EQUAL(15U, vi.moduleID);
    TEST_ASSERT_EQUAL(1U, vi.sw_major_version);
    TEST_ASSERT_EQUAL(0U, vi.sw_minor_version);
    TEST_ASSERT_EQUAL(0U, vi.sw_patch_version);
}

void test_Det_ReportErrorBoundary(void)
{
    Det_Init(NULL);
    Det_Start();
    Std_ReturnType r = Det_ReportError(0xFFFF, 0xFF, 0xFF, 0xFF);
    TEST_ASSERT_EQUAL(E_OK, r);
}

void test_Det_MultipleModules(void)
{
    Det_Init(NULL);
    Det_Start();
    uint16 modules[] = {0, 1, 255, 1024, 65535};
    for (uint8 i = 0; i < 5; i++) {
        Std_ReturnType r = Det_ReportError(modules[i], i, i, i);
        TEST_ASSERT_EQUAL(E_OK, r);
    }
}

int main(void)
{
    UnityBegin();

    UnityRunTest(test_Det_InitAndStart, "Det Init+Start", __LINE__);
    UnityRunTest(test_Det_ReportError, "Det ReportError", __LINE__);
    UnityRunTest(test_Det_ReportRuntimeError, "Det ReportRuntimeError", __LINE__);
    UnityRunTest(test_Det_ReportTransientFault, "Det ReportTransientFault", __LINE__);
    UnityRunTest(test_Det_GetVersionInfo, "Det GetVersionInfo", __LINE__);
    UnityRunTest(test_Det_ReportErrorBoundary, "Det ReportError boundary", __LINE__);
    UnityRunTest(test_Det_MultipleModules, "Det multiple modules", __LINE__);

    return UnityEnd();
}
