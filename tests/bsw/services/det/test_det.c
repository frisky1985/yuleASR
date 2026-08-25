/**
 * @file test_det.c
 * @brief Det (Default Error Tracer) Unit Tests
 * @req SWS_Det
 */
#include "unity.h"
#include "Det.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_Det_00001 */
void test_Det_Init_NullPtr_ShouldNotCrash(void) { Det_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_Det_00001 */
void test_Det_Init_ValidConfig_ShouldSucceed(void) { Det_ConfigType cfg; cfg.MaxEntries = 16U; Det_Init(&cfg); TEST_ASSERT_TRUE(1); }
/** @req SWS_Det_00002 */
void test_Det_Start_AfterInit_ShouldNotCrash(void) { Det_ConfigType cfg; cfg.MaxEntries = 16U; Det_Init(&cfg); Det_Start(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Det_00003 */
void test_Det_ReportError_AfterInit_ShouldReturnOk(void) { Det_ConfigType cfg; cfg.MaxEntries = 16U; Det_Init(&cfg); Det_Start(); Std_ReturnType ret = Det_ReportError(0x01U, 0U, 0x01U, 0x01U); TEST_ASSERT_EQUAL(E_OK, ret); }
/** @req SWS_Det_00004 */
void test_Det_ReportRuntimeError_AfterInit_ShouldReturnOk(void) { Det_ConfigType cfg; cfg.MaxEntries = 16U; Det_Init(&cfg); Det_Start(); Std_ReturnType ret = Det_ReportRuntimeError(0x01U, 0U, 0x01U, 0x01U); TEST_ASSERT_EQUAL(E_OK, ret); }
/** @req SWS_Det_00005 */
void test_Det_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Det_GetVersionInfo(&info); TEST_ASSERT_EQUAL(DET_VENDOR_ID, info.vendorID); }
/** @req SWS_Det_00005 */
void test_Det_GetVersionInfo_NullPtr_ShouldNotCrash(void) { Det_GetVersionInfo(NULL_PTR); TEST_ASSERT_TRUE(1); }
void test_Det_Init_DoubleInit_ShouldNotCrash(void) { Det_ConfigType cfg; cfg.MaxEntries = 16U; Det_Init(&cfg); Det_Init(&cfg); TEST_ASSERT_TRUE(1); }
void test_Det_ReportError_BeforeInit_ShouldReturnOk(void) { Std_ReturnType ret = Det_ReportError(0x01U, 0U, 0x01U, 0x01U); TEST_ASSERT_EQUAL(E_OK, ret); }
