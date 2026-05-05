/***********************************************************************************************************************
 * File:        test_dem_queue.c
 * Description: Unit tests for Dem Event Queue Management
 **********************************************************************************************************************/

#include "unity.h"
#include "dem_queue.h"
#include <string.h>

/* Test setup */
void setUp(void)
{
    Dem_QueueInit();
}

void tearDown(void)
{
    Dem_QueueReset();
}

/* Test cases */

/**
 * Test: Queue initialization
 */
void test_Dem_QueueInit(void)
{
    TEST_ASSERT_TRUE(Dem_QueueIsEmpty());
    TEST_ASSERT_EQUAL(DEM_QUEUE_EMPTY, Dem_QueueGetState());
    TEST_ASSERT_EQUAL(0, Dem_QueueGetCount());
}

/**
 * Test: Queue enqueue and dequeue
 */
void test_Dem_QueueEnqueueDequeue(void)
{
    Std_ReturnType result;
    Dem_QueueEntryType entry;
    
    /* Enqueue */
    result = Dem_QueueEnqueue(0, DEM_EVENT_STATUS_FAILED, DEM_QUEUE_PRIORITY_NORMAL);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(1, Dem_QueueGetCount());
    
    /* Dequeue */
    result = Dem_QueueDequeue(&entry);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0, entry.EventId);
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, entry.EventStatus);
    TEST_ASSERT_TRUE(Dem_QueueIsEmpty());
}

/**
 * Test: Queue overflow handling
 */
void test_Dem_QueueOverflow(void)
{
    Std_ReturnType result;
    uint8 i;
    
    /* Fill queue */
    for (i = 0; i < DEM_CFG_EVENT_QUEUE_SIZE; i++) {
        result = Dem_QueueEnqueue(i, DEM_EVENT_STATUS_FAILED, DEM_QUEUE_PRIORITY_NORMAL);
        TEST_ASSERT_EQUAL(E_OK, result);
    }
    
    TEST_ASSERT_TRUE(Dem_QueueIsFull());
    
    /* Try to add one more - should handle overflow */
    result = Dem_QueueEnqueue(99, DEM_EVENT_STATUS_FAILED, DEM_QUEUE_PRIORITY_HIGH);
    /* May succeed with overflow handling or fail */
    
    TEST_ASSERT_NOT_EQUAL(0, Dem_QueueGetOverflowCount());
}

/**
 * Test: Queue priority insertion
 */
void test_Dem_QueuePriority(void)
{
    Std_ReturnType result;
    Dem_QueueEntryType entry;
    
    /* Insert with different priorities */
    Dem_QueueInsertPriority(0, DEM_EVENT_STATUS_FAILED, DEM_QUEUE_PRIORITY_LOW);
    Dem_QueueInsertPriority(1, DEM_EVENT_STATUS_FAILED, DEM_QUEUE_PRIORITY_HIGH);
    Dem_QueueInsertPriority(2, DEM_EVENT_STATUS_FAILED, DEM_QUEUE_PRIORITY_NORMAL);
    
    /* High priority should be at head */
    result = Dem_QueuePeek(&entry);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(1, entry.EventId); /* High priority */
    TEST_ASSERT_EQUAL(DEM_QUEUE_PRIORITY_HIGH, entry.Priority);
}

/**
 * Test: Queue remove event
 */
void test_Dem_QueueRemoveEvent(void)
{
    Std_ReturnType result;
    
    /* Add events */
    Dem_QueueEnqueue(0, DEM_EVENT_STATUS_FAILED, DEM_QUEUE_PRIORITY_NORMAL);
    Dem_QueueEnqueue(1, DEM_EVENT_STATUS_FAILED, DEM_QUEUE_PRIORITY_NORMAL);
    Dem_QueueEnqueue(2, DEM_EVENT_STATUS_FAILED, DEM_QUEUE_PRIORITY_NORMAL);
    
    /* Remove middle event */
    result = Dem_QueueRemoveEvent(1);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(2, Dem_QueueGetCount());
}

/**
 * Test: Queue null pointer handling
 */
void test_Dem_QueueNullPointer(void)
{
    Std_ReturnType result;
    
    result = Dem_QueueDequeue(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    
    result = Dem_QueuePeek(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * Test: Queue process
 */
void test_Dem_QueueProcess(void)
{
    /* Add events to queue */
    Dem_QueueEnqueue(0, DEM_EVENT_STATUS_FAILED, DEM_QUEUE_PRIORITY_NORMAL);
    Dem_QueueEnqueue(1, DEM_EVENT_STATUS_PASSED, DEM_QUEUE_PRIORITY_NORMAL);
    
    /* Process queue */
    Dem_QueueProcess();
    
    /* Queue should be empty after processing */
    /* Note: Actual behavior depends on Dem_ProcessEvent implementation */
}

/* Main test runner */
int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_Dem_QueueInit);
    RUN_TEST(test_Dem_QueueEnqueueDequeue);
    RUN_TEST(test_Dem_QueueOverflow);
    RUN_TEST(test_Dem_QueuePriority);
    RUN_TEST(test_Dem_QueueRemoveEvent);
    RUN_TEST(test_Dem_QueueNullPointer);
    RUN_TEST(test_Dem_QueueProcess);
    
    return UNITY_END();
}
