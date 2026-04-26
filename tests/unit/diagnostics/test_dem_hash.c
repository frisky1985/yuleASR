/**
 * @file test_dem_hash.c
 * @brief Unit tests for DEM DTC Hash Table Implementation
 * @note MISRA C:2012 Compliant test code
 */

#include "unity.h"
#include "dem.h"
#include "dem_dtc_hash.c"  /* Include source for testing */

void setUp(void)
{
    /* Initialize hash table before each test */
    Dem_DtcHashInit();
}

void tearDown(void)
{
    /* Clean up after each test */
}

/* Test hash table initialization */
void test_Dem_DtcHashInit(void)
{
    TEST_ASSERT_TRUE(s_dtcHashTable.initialized);
    TEST_ASSERT_EQUAL_UINT16(0U, s_dtcHashTable.usedCount);
    TEST_ASSERT_EQUAL_UINT16(1U, s_dtcHashTable.freeListHead);
}

/* Test DTC insertion */
void test_Dem_DtcHashInsert(void)
{
    uint32_t dtcCode = 0x12345678U;
    Dem_EventStatusType status = DEM_EVENT_STATUS_FAILED;
    
    Std_ReturnType result = Dem_DtcHashInsert(dtcCode, status);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT16(1U, s_dtcHashTable.usedCount);
}

/* Test DTC find */
void test_Dem_DtcHashFind(void)
{
    uint32_t dtcCode = 0x12345678U;
    Dem_EventStatusType status = DEM_EVENT_STATUS_FAILED;
    
    /* Insert DTC */
    (void)Dem_DtcHashInsert(dtcCode, status);
    
    /* Find DTC */
    Dem_DtcHashEntryType *entry = Dem_DtcHashFind(dtcCode);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT32(dtcCode, entry->dtcCode);
    TEST_ASSERT_EQUAL(status, entry->status);
    
    /* Find non-existent DTC */
    Dem_DtcHashEntryType *notFound = Dem_DtcHashFind(0x87654321U);
    TEST_ASSERT_NULL(notFound);
}

/* Test DTC update */
void test_Dem_DtcHashUpdateStatus(void)
{
    uint32_t dtcCode = 0x12345678U;
    
    /* Insert with FAILED status */
    (void)Dem_DtcHashInsert(dtcCode, DEM_EVENT_STATUS_FAILED);
    
    /* Update to PASSED */
    Std_ReturnType result = Dem_DtcHashUpdateStatus(dtcCode, DEM_EVENT_STATUS_PASSED);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Verify update */
    Dem_DtcHashEntryType *entry = Dem_DtcHashFind(dtcCode);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_PASSED, entry->status);
}

/* Test DTC deletion */
void test_Dem_DtcHashDelete(void)
{
    uint32_t dtcCode = 0x12345678U;
    
    /* Insert and delete */
    (void)Dem_DtcHashInsert(dtcCode, DEM_EVENT_STATUS_FAILED);
    Std_ReturnType result = Dem_DtcHashDelete(dtcCode);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT16(0U, s_dtcHashTable.usedCount);
    
    /* Verify deletion */
    Dem_DtcHashEntryType *entry = Dem_DtcHashFind(dtcCode);
    TEST_ASSERT_NULL(entry);
    
    /* Delete non-existent DTC should fail */
    result = Dem_DtcHashDelete(0x87654321U);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/* Test multiple DTCs */
void test_Dem_DtcHash_MultipleDTCs(void)
{
    /* Insert multiple DTCs */
    for (uint32_t i = 0U; i < 10U; i++) {
        uint32_t dtcCode = 0x10000000U + i;
        Std_ReturnType result = Dem_DtcHashInsert(dtcCode, DEM_EVENT_STATUS_FAILED);
        TEST_ASSERT_EQUAL(E_OK, result);
    }
    
    TEST_ASSERT_EQUAL_UINT16(10U, s_dtcHashTable.usedCount);
    
    /* Verify all can be found */
    for (uint32_t i = 0U; i < 10U; i++) {
        uint32_t dtcCode = 0x10000000U + i;
        Dem_DtcHashEntryType *entry = Dem_DtcHashFind(dtcCode);
        TEST_ASSERT_NOT_NULL(entry);
        TEST_ASSERT_EQUAL_UINT32(dtcCode, entry->dtcCode);
    }
}

/* Test report DTC status optimized */
void test_Dem_ReportDTCStatus_Optimized(void)
{
    uint32_t dtcCode = 0x12345678U;
    
    /* Report FAILED */
    Std_ReturnType result = Dem_ReportDTCStatus_Optimized(dtcCode, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Verify inserted */
    Dem_DtcHashEntryType *entry = Dem_DtcHashFind(dtcCode);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, entry->status);
    
    /* Report PASSED */
    result = Dem_ReportDTCStatus_Optimized(dtcCode, DEM_EVENT_STATUS_PASSED);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Verify updated */
    entry = Dem_DtcHashFind(dtcCode);
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_PASSED, entry->status);
}

/* Test clear all DTCs */
void test_Dem_ClearAllDTCs(void)
{
    /* Insert some DTCs */
    (void)Dem_DtcHashInsert(0x11111111U, DEM_EVENT_STATUS_FAILED);
    (void)Dem_DtcHashInsert(0x22222222U, DEM_EVENT_STATUS_FAILED);
    (void)Dem_DtcHashInsert(0x33333333U, DEM_EVENT_STATUS_FAILED);
    
    TEST_ASSERT_EQUAL_UINT16(3U, s_dtcHashTable.usedCount);
    
    /* Clear all */
    Dem_ClearAllDTCs();
    
    TEST_ASSERT_EQUAL_UINT16(0U, s_dtcHashTable.usedCount);
    
    /* Verify all cleared */
    TEST_ASSERT_NULL(Dem_DtcHashFind(0x11111111U));
    TEST_ASSERT_NULL(Dem_DtcHashFind(0x22222222U));
    TEST_ASSERT_NULL(Dem_DtcHashFind(0x33333333U));
}

/* Test hash collision handling */
void test_Dem_DtcHash_Collisions(void)
{
    /* Insert DTCs that may hash to same bucket */
    uint32_t dtcCodes[5] = {0x00000001U, 0x00000041U, 0x00000081U, 0x000000C1U, 0x00000101U};
    
    for (uint8_t i = 0U; i < 5U; i++) {
        Std_ReturnType result = Dem_DtcHashInsert(dtcCodes[i], DEM_EVENT_STATUS_FAILED);
        TEST_ASSERT_EQUAL(E_OK, result);
    }
    
    /* Verify all can still be found despite collisions */
    for (uint8_t i = 0U; i < 5U; i++) {
        Dem_DtcHashEntryType *entry = Dem_DtcHashFind(dtcCodes[i]);
        TEST_ASSERT_NOT_NULL(entry);
        TEST_ASSERT_EQUAL_UINT32(dtcCodes[i], entry->dtcCode);
    }
}

/* Main test runner */
int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_Dem_DtcHashInit);
    RUN_TEST(test_Dem_DtcHashInsert);
    RUN_TEST(test_Dem_DtcHashFind);
    RUN_TEST(test_Dem_DtcHashUpdateStatus);
    RUN_TEST(test_Dem_DtcHashDelete);
    RUN_TEST(test_Dem_DtcHash_MultipleDTCs);
    RUN_TEST(test_Dem_ReportDTCStatus_Optimized);
    RUN_TEST(test_Dem_ClearAllDTCs);
    RUN_TEST(test_Dem_DtcHash_Collisions);
    
    return UNITY_END();
}
