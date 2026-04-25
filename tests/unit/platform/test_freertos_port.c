/**
 * @file test_freertos_port.c
 * @brief FreeRTOS移植层单元测试
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unity.h"
#include "../../../tests/mocks/mock_freertos.h"

/*==============================================================================
 * 全局状态
 *==============================================================================*/

static volatile int g_task_ran = 0;
static volatile int g_queue_received = 0;
static volatile int g_semaphore_taken = 0;

/*==============================================================================
 * 任务函数
 *==============================================================================*/

static void test_task(void* params) {
    (void)params;
    g_task_ran = 1;
    vTaskDelete(NULL);
}

static void delayed_task(void* params) {
    (void)params;
    vTaskDelay(pdMS_TO_TICKS(50));
    g_task_ran = 1;
    vTaskDelete(NULL);
}

/*==============================================================================
 * Unity Setup/Teardown
 *==============================================================================*/

void setUp(void) {
    g_task_ran = 0;
    g_queue_received = 0;
    g_semaphore_taken = 0;
    mock_freertos_reset();
}

void tearDown(void) {
    mock_freertos_reset();
}

/*==============================================================================
 * 测试用例 - 任务管理
 *==============================================================================*/

/* Test 1: 创建任务 */
void test_freertos_create_task(void) {
    TaskHandle_t task;
    BaseType_t result = xTaskCreate(test_task, "TestTask", 1024, NULL, 1, &task);
    TEST_ASSERT_EQUAL(pdPASS, result);
    TEST_ASSERT_NOT_NULL(task);
    
    /* 等待任务执行 */
    usleep(100000);
    TEST_ASSERT_TRUE(g_task_ran);
}

/* Test 2: 删除任务 */
void test_freertos_delete_task(void) {
    TaskHandle_t task;
    xTaskCreate(test_task, "DeleteTask", 1024, NULL, 1, &task);
    
    vTaskDelete(task);
    
    uint32_t count = mock_freertos_get_task_count();
    TEST_ASSERT_EQUAL(0, count);
}

/* Test 3: 任务延迟 */
void test_freertos_task_delay(void) {
    g_task_ran = 0;
    
    TickType_t start = xTaskGetTickCount();
    vTaskDelay(pdMS_TO_TICKS(100));
    TickType_t end = xTaskGetTickCount();
    
    /* 验证延迟大约100ms */
    TEST_ASSERT_GREATER_OR_EQUAL(pdMS_TO_TICKS(90), end - start);
}

/* Test 4: 任务优先级 */
void test_freertos_task_priority(void) {
    TaskHandle_t task;
    xTaskCreate(test_task, "PriorityTask", 1024, NULL, 5, &task);
    
    UBaseType_t priority = uxTaskPriorityGet(task);
    TEST_ASSERT_EQUAL(5, priority);
    
    vTaskPrioritySet(task, 10);
    priority = uxTaskPriorityGet(task);
    TEST_ASSERT_EQUAL(10, priority);
    
    vTaskDelete(task);
}

/* Test 5: 任务挂起和恢复 */
void test_freertos_suspend_resume(void) {
    TaskHandle_t task;
    g_task_ran = 0;
    
    xTaskCreate(test_task, "SuspendTask", 1024, NULL, 1, &task);
    
    vTaskSuspend(task);
    usleep(50000);
    /* 任务应该没有执行完成 */
    
    vTaskResume(task);
    usleep(100000);
    TEST_ASSERT_TRUE(g_task_ran);
}

/*==============================================================================
 * 测试用例 - 队列管理
 *==============================================================================*/

/* Test 6: 队列创建和删除 */
void test_freertos_queue_create_delete(void) {
    QueueHandle_t queue = xQueueCreate(10, sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(queue);
    
    vQueueDelete(queue);
}

/* Test 7: 队列发送和接收 */
void test_freertos_queue_send_receive(void) {
    QueueHandle_t queue = xQueueCreate(10, sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(queue);
    
    uint32_t send_data = 0x12345678;
    BaseType_t result = xQueueSend(queue, &send_data, 0);
    TEST_ASSERT_EQUAL(pdPASS, result);
    
    uint32_t recv_data = 0;
    result = xQueueReceive(queue, &recv_data, 0);
    TEST_ASSERT_EQUAL(pdPASS, result);
    TEST_ASSERT_EQUAL(send_data, recv_data);
    
    vQueueDelete(queue);
}

/* Test 8: 队列阻塞接收 */
void test_freertos_queue_blocking_receive(void) {
    QueueHandle_t queue = xQueueCreate(1, sizeof(uint32_t));
    
    uint32_t recv_data = 0;
    /* 没有数据时应该返回失败 */
    BaseType_t result = xQueueReceive(queue, &recv_data, 0);
    TEST_ASSERT_EQUAL(errQUEUE_EMPTY, result);
    
    vQueueDelete(queue);
}

/* Test 9: 队列消息计数 */
void test_freertos_queue_messages_waiting(void) {
    QueueHandle_t queue = xQueueCreate(10, sizeof(uint32_t));
    
    TEST_ASSERT_EQUAL(0, uxQueueMessagesWaiting(queue));
    
    uint32_t data = 1;
    xQueueSend(queue, &data, 0);
    TEST_ASSERT_EQUAL(1, uxQueueMessagesWaiting(queue));
    
    xQueueSend(queue, &data, 0);
    TEST_ASSERT_EQUAL(2, uxQueueMessagesWaiting(queue));
    
    vQueueDelete(queue);
}

/*==============================================================================
 * 测试用例 - 信号量管理
 *==============================================================================*/

/* Test 10: 二值信号量 */
void test_freertos_binary_semaphore(void) {
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    TEST_ASSERT_NOT_NULL(sem);
    
    /* 初始状态应该不可用 */
    BaseType_t result = xSemaphoreTake(sem, 0);
    TEST_ASSERT_EQUAL(pdFALSE, result);
    
    /* 给信号量 */
    result = xSemaphoreGive(sem);
    TEST_ASSERT_EQUAL(pdTRUE, result);
    
    /* 现在可以获取 */
    result = xSemaphoreTake(sem, 0);
    TEST_ASSERT_EQUAL(pdTRUE, result);
    
    vSemaphoreDelete(sem);
}

/* Test 11: 计数信号量 */
void test_freertos_counting_semaphore(void) {
    SemaphoreHandle_t sem = xSemaphoreCreateCounting(5, 0);
    TEST_ASSERT_NOT_NULL(sem);
    
    /* 给三次 */
    xSemaphoreGive(sem);
    xSemaphoreGive(sem);
    xSemaphoreGive(sem);
    
    /* 获取三次 */
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(sem, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(sem, 0));
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(sem, 0));
    
    /* 第四次应该失败 */
    TEST_ASSERT_EQUAL(pdFALSE, xSemaphoreTake(sem, 0));
    
    vSemaphoreDelete(sem);
}

/* Test 12: 互斥锁 */
void test_freertos_mutex(void) {
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    TEST_ASSERT_NOT_NULL(mutex);
    
    /* 获取互斥锁 */
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(mutex, portMAX_DELAY));
    
    /* 重复获取应该失败（非递归互斥锁） */
    TEST_ASSERT_EQUAL(pdFALSE, xSemaphoreTake(mutex, 0));
    
    /* 释放互斥锁 */
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreGive(mutex));
    
    vSemaphoreDelete(mutex);
}

/*==============================================================================
 * 测试用例 - 内存管理
 *==============================================================================*/

/* Test 13: 动态内存分配 */
void test_freertos_malloc_free(void) {
    void* ptr = pvPortMalloc(256);
    TEST_ASSERT_NOT_NULL(ptr);
    
    vPortFree(ptr);
}

/* Test 14: 内存清零分配 */
void test_freertos_calloc(void) {
    uint32_t* ptr = (uint32_t*)pvPortCalloc(10, sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(ptr);
    
    /* 验证已清零 */
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL(0, ptr[i]);
    }
    
    vPortFree(ptr);
}

/* Test 15: 获取堆大小 */
void test_freertos_heap_size(void) {
    size_t free1 = xPortGetFreeHeapSize();
    
    void* ptr = pvPortMalloc(1024);
    TEST_ASSERT_NOT_NULL(ptr);
    
    size_t free2 = xPortGetFreeHeapSize();
    TEST_ASSERT_LESS_THAN(free1, free2);
    
    vPortFree(ptr);
    
    size_t free3 = xPortGetFreeHeapSize();
    TEST_ASSERT_EQUAL(free1, free3);
}

/*==============================================================================
 * 主函数
 *==============================================================================*/

int main(void) {
    UNITY_BEGIN();
    
    /* 任务测试 */
    RUN_TEST(test_freertos_create_task);
    RUN_TEST(test_freertos_delete_task);
    RUN_TEST(test_freertos_task_delay);
    RUN_TEST(test_freertos_task_priority);
    RUN_TEST(test_freertos_suspend_resume);
    
    /* 队列测试 */
    RUN_TEST(test_freertos_queue_create_delete);
    RUN_TEST(test_freertos_queue_send_receive);
    RUN_TEST(test_freertos_queue_blocking_receive);
    RUN_TEST(test_freertos_queue_messages_waiting);
    
    /* 信号量测试 */
    RUN_TEST(test_freertos_binary_semaphore);
    RUN_TEST(test_freertos_counting_semaphore);
    RUN_TEST(test_freertos_mutex);
    
    /* 内存测试 */
    RUN_TEST(test_freertos_malloc_free);
    RUN_TEST(test_freertos_calloc);
    RUN_TEST(test_freertos_heap_size);
    
    return UNITY_END();
}
