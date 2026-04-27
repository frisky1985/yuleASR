/**
 * @file test_dlt_improvements.c
 * @brief Unit tests for DLT module improvements
 */

#include "unity/unity.h"
#include "dlt/dlt.h"
#include "dlt/dlt_payload.h"
#include "dlt/dlt_control.h"
#include "dlt/dlt_output.h"
#include "dlt/dlt_performance.h"
#include <string.h>

/* Test setup */
void setUp(void) {
}

void tearDown(void) {
}

/*===========================================================================*/
/* Payload Type Info Tests                                                   */
/*===========================================================================*/

void test_payload_type_info_log_message(void) {
    uint8_t msin = Dlt_BuildMsinLog(DLT_LOG_INFO);
    
    TEST_ASSERT_EQUAL(DLT_TYPE_LOG, Dlt_GetMessageTypeFromMsin(msin));
    TEST_ASSERT_EQUAL(DLT_LOG_INFO, Dlt_GetLogLevelFromMsin(msin));
    TEST_ASSERT_TRUE(Dlt_IsLogMessage(msin));
    TEST_ASSERT_FALSE(Dlt_IsControlMessage(msin));
}

void test_payload_type_info_control_message(void) {
    uint8_t msin = Dlt_BuildMsinControl(DLT_CONTROL_REQUEST);
    
    TEST_ASSERT_EQUAL(DLT_TYPE_CONTROL, Dlt_GetMessageTypeFromMsin(msin));
    TEST_ASSERT_TRUE(Dlt_IsControlMessage(msin));
    TEST_ASSERT_FALSE(Dlt_IsLogMessage(msin));
}

void test_payload_type_info_app_trace(void) {
    uint8_t msin = Dlt_BuildMsinAppTrace(DLT_TRACE_FUNCTION_IN);
    
    TEST_ASSERT_EQUAL(DLT_TYPE_APP_TRACE, Dlt_GetMessageTypeFromMsin(msin));
    TEST_ASSERT_EQUAL(DLT_TRACE_FUNCTION_IN, Dlt_GetSpecificInfoFromMsin(msin));
}

void test_payload_type_info_network_trace(void) {
    uint8_t msin = Dlt_BuildMsinNwTrace(DLT_NW_TRACE_CAN);
    
    TEST_ASSERT_EQUAL(DLT_TYPE_NW_TRACE, Dlt_GetMessageTypeFromMsin(msin));
    TEST_ASSERT_EQUAL(DLT_NW_TRACE_CAN, Dlt_GetSpecificInfoFromMsin(msin));
}

/*===========================================================================*/
/* Payload Builder Tests                                                     */
/*===========================================================================*/

void test_payload_builder_init(void) {
    uint8_t buffer[256];
    Dlt_PayloadBuilderType builder;
    
    Dlt_PayloadBuilder_Init(&builder, buffer, sizeof(buffer));
    
    TEST_ASSERT_EQUAL_PTR(buffer, builder.buffer);
    TEST_ASSERT_EQUAL(256, builder.size);
    TEST_ASSERT_EQUAL(0, builder.position);
    TEST_ASSERT_EQUAL(256, builder.remaining);
}

void test_payload_builder_add_string(void) {
    uint8_t buffer[256];
    Dlt_PayloadBuilderType builder;
    
    Dlt_PayloadBuilder_Init(&builder, buffer, sizeof(buffer));
    
    Dlt_ReturnType result = Dlt_PayloadBuilder_AddString(&builder, DLT_SCOD_ASCII, "Hello");
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, result);
    TEST_ASSERT_EQUAL(1 + 2 + 5 + 1, builder.position);
}

void test_payload_builder_add_variable_uint32(void) {
    uint8_t buffer[256];
    Dlt_PayloadBuilderType builder;
    
    Dlt_PayloadBuilder_Init(&builder, buffer, sizeof(buffer));
    
    uint32_t value = 0x12345678;
    Dlt_ReturnType result = Dlt_PayloadBuilder_AddVariable(&builder, DLT_TYPE_UINT32, &value, 0);
    
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, result);
    TEST_ASSERT_EQUAL(1 + 4, builder.position);
    TEST_ASSERT_EQUAL(DLT_TYPE_UINT32, buffer[0]);
    TEST_ASSERT_EQUAL(0x78, buffer[1]);
    TEST_ASSERT_EQUAL(0x56, buffer[2]);
    TEST_ASSERT_EQUAL(0x34, buffer[3]);
    TEST_ASSERT_EQUAL(0x12, buffer[4]);
}

void test_payload_builder_add_raw_data(void) {
    uint8_t buffer[256];
    Dlt_PayloadBuilderType builder;
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    
    Dlt_PayloadBuilder_Init(&builder, buffer, sizeof(buffer));
    
    Dlt_ReturnType result = Dlt_PayloadBuilder_AddRawData(&builder, data, sizeof(data));
    
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, result);
    TEST_ASSERT_EQUAL(1 + 2 + 4, builder.position);
    TEST_ASSERT_EQUAL(DLT_TYPE_RAWD, buffer[0]);
    TEST_ASSERT_EQUAL(0x04, buffer[1]);
    TEST_ASSERT_EQUAL(0x00, buffer[2]);
    TEST_ASSERT_EQUAL(0x01, buffer[3]);
    TEST_ASSERT_EQUAL(0x02, buffer[4]);
}

/*===========================================================================*/
/* Control Message Tests                                                     */
/*===========================================================================*/

void test_control_init(void) {
    Dlt_ReturnType result = Dlt_Control_Init(NULL);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, result);
    
    const Dlt_ControlStateType *state = Dlt_Control_GetState();
    TEST_ASSERT_NOT_NULL(state);
    TEST_ASSERT_TRUE(state->verbose_mode);
    TEST_ASSERT_TRUE(state->message_filtering);
    TEST_ASSERT_EQUAL(DLT_LOG_INFO, state->default_log_level);
    
    Dlt_Control_DeInit();
}

void test_control_set_default_log_level(void) {
    Dlt_Control_Init(NULL);
    
    uint8_t payload = DLT_LOG_WARN;
    uint8_t response[16];
    uint16_t resp_len = sizeof(response);
    
    Dlt_ReturnType result = Dlt_Control_SetDefaultLogLevel(&payload, 1, response, &resp_len);
    
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, result);
    TEST_ASSERT_EQUAL(1, resp_len);
    TEST_ASSERT_EQUAL(DLT_RESPONSE_OK, response[0]);
    
    const Dlt_ControlStateType *state = Dlt_Control_GetState();
    TEST_ASSERT_EQUAL(DLT_LOG_WARN, state->default_log_level);
    
    Dlt_Control_DeInit();
}

void test_control_set_verbose_mode(void) {
    Dlt_Control_Init(NULL);
    
    uint8_t payload = 0; /* Disable verbose mode */
    uint8_t response[16];
    uint16_t resp_len = sizeof(response);
    
    Dlt_ReturnType result = Dlt_Control_SetVerboseMode(&payload, 1, response, &resp_len);
    
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, result);
    
    const Dlt_ControlStateType *state = Dlt_Control_GetState();
    TEST_ASSERT_FALSE(state->verbose_mode);
    
    Dlt_Control_DeInit();
}

void test_control_register_service(void) {
    Dlt_Control_Init(NULL);
    
    Dlt_ReturnType (*callback)(Dlt_ServiceIdType, const uint8_t*, uint16_t, 
                                uint8_t*, uint16_t*, void*) = NULL;
    
    Dlt_ReturnType result = Dlt_Control_RegisterService(DLT_SERVICE_ID_USER, callback, NULL);
    TEST_ASSERT_EQUAL(DLT_RETURN_ERROR, result); /* NULL callback not allowed */
    
    Dlt_Control_DeInit();
}

/*===========================================================================*/
/* Output Channel Tests                                                      */
/*===========================================================================*/

void test_output_init(void) {
    Dlt_OutputManagerConfigType config = {0};
    
    Dlt_ReturnType result = Dlt_Output_Init(&config);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, result);
    
    Dlt_Output_DeInit();
}

void test_output_add_channel(void) {
    Dlt_OutputManagerConfigType config = {0};
    Dlt_Output_Init(&config);
    
    Dlt_OutputChannelConfigType channel = {
        .type = DLT_OUTPUT_CALLBACK,
        .enabled = true,
        .priority = 1
    };
    
    Dlt_ReturnType result = Dlt_Output_AddChannel(&channel);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, result);
    TEST_ASSERT_TRUE(Dlt_Output_IsChannelEnabled(DLT_OUTPUT_CALLBACK));
    
    Dlt_Output_DeInit();
}

void test_output_statistics(void) {
    Dlt_OutputManagerConfigType config = {0};
    Dlt_Output_Init(&config);
    
    const Dlt_OutputStatisticsType *stats = Dlt_Output_GetStatistics();
    TEST_ASSERT_NOT_NULL(stats);
    TEST_ASSERT_EQUAL(0, stats->bytes_sent[DLT_OUTPUT_UDP]);
    TEST_ASSERT_EQUAL(0, stats->errors[DLT_OUTPUT_TCP]);
    
    Dlt_Output_ResetStatistics();
    
    Dlt_Output_DeInit();
}

/*===========================================================================*/
/* Performance Tests                                                         */
/*===========================================================================*/

void test_mempool_init(void) {
    Dlt_MemPoolType pool;
    
    Dlt_ReturnType result = Dlt_MemPool_Init(&pool);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, result);
    
    Dlt_MemPoolStatsType stats;
    Dlt_MemPool_GetStats(&pool, &stats);
    TEST_ASSERT_EQUAL(0, stats.used_small);
    TEST_ASSERT_EQUAL(0, stats.used_medium);
    
    Dlt_MemPool_DeInit(&pool);
}

void test_mempool_allocate_free(void) {
    Dlt_MemPoolType pool;
    Dlt_MemPool_Init(&pool);
    
    void *ptr = Dlt_MemPool_Allocate(&pool, 50);
    TEST_ASSERT_NOT_NULL(ptr);
    
    Dlt_MemPoolStatsType stats;
    Dlt_MemPool_GetStats(&pool, &stats);
    TEST_ASSERT_EQUAL(1, stats.used_small);
    
    Dlt_MemPool_Free(&pool, ptr, 50);
    
    Dlt_MemPool_GetStats(&pool, &stats);
    TEST_ASSERT_EQUAL(0, stats.used_small);
    
    Dlt_MemPool_DeInit(&pool);
}

void test_zerocopy_buffer_init(void) {
    uint8_t memory[256];
    Dlt_ZeroCopyBufferType buffer;
    
    Dlt_ReturnType result = Dlt_ZeroCopyBuffer_Init(&buffer, memory, sizeof(memory));
    
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, result);
    TEST_ASSERT_EQUAL_PTR(memory, buffer.base);
    TEST_ASSERT_EQUAL(256, buffer.capacity);
    TEST_ASSERT_EQUAL(0, buffer.head);
    TEST_ASSERT_EQUAL(0, buffer.tail);
    TEST_ASSERT_EQUAL(256, Dlt_ZeroCopyBuffer_Available(&buffer));
}

void test_zerocopy_buffer_allocate_commit(void) {
    uint8_t memory[256];
    Dlt_ZeroCopyBufferType buffer;
    Dlt_BufferSliceType slice;
    
    Dlt_ZeroCopyBuffer_Init(&buffer, memory, sizeof(memory));
    
    int16_t result = Dlt_ZeroCopyBuffer_Allocate(&buffer, 64, &slice);
    TEST_ASSERT_EQUAL(64, result);
    TEST_ASSERT_TRUE(slice.contiguous);
    
    Dlt_ZeroCopyBuffer_Commit(&buffer, 64);
    TEST_ASSERT_EQUAL(64, buffer.head);
    TEST_ASSERT_EQUAL(64, buffer.used);
    TEST_ASSERT_EQUAL(192, Dlt_ZeroCopyBuffer_Available(&buffer));
}

void test_lockfree_queue_init(void) {
    Dlt_LockFreeQueueType queue;
    
    Dlt_LockFreeQueue_Init(&queue);
    
    TEST_ASSERT_EQUAL(0, queue.head);
    TEST_ASSERT_EQUAL(0, queue.tail);
    TEST_ASSERT_TRUE(Dlt_LockFreeQueue_IsEmpty(&queue));
}

void test_lockfree_queue_enqueue_dequeue(void) {
    Dlt_LockFreeQueueType queue;
    Dlt_LockFreeQueue_Init(&queue);
    
    uint8_t item1 = 1;
    uint8_t item2 = 2;
    
    bool result = Dlt_LockFreeQueue_Enqueue(&queue, &item1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(Dlt_LockFreeQueue_IsEmpty(&queue));
    TEST_ASSERT_EQUAL(1, Dlt_LockFreeQueue_Count(&queue));
    
    result = Dlt_LockFreeQueue_Enqueue(&queue, &item2);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, Dlt_LockFreeQueue_Count(&queue));
    
    void *dequeued = Dlt_LockFreeQueue_Dequeue(&queue);
    TEST_ASSERT_EQUAL_PTR(&item1, dequeued);
    TEST_ASSERT_EQUAL(1, Dlt_LockFreeQueue_Count(&queue));
}

void test_performance_stats(void) {
    Dlt_PerformanceStatsType stats;
    
    Dlt_Performance_ResetStats();
    Dlt_Performance_GetStats(&stats);
    
    TEST_ASSERT_EQUAL(0, stats.messages_logged);
    TEST_ASSERT_EQUAL(0, stats.bytes_written);
}

/*===========================================================================*/
/* Integration Tests                                                         */
/*===========================================================================*/

void test_full_control_flow(void) {
    Dlt_Control_Init(NULL);
    
    /* Test multiple control operations */
    uint8_t payload[16];
    uint8_t response[32];
    uint16_t resp_len;
    
    /* Set default log level to WARN */
    payload[0] = DLT_LOG_WARN;
    resp_len = sizeof(response);
    Dlt_Control_SetDefaultLogLevel(payload, 1, response, &resp_len);
    TEST_ASSERT_EQUAL(DLT_RESPONSE_OK, response[0]);
    
    /* Disable verbose mode */
    payload[0] = 0;
    resp_len = sizeof(response);
    Dlt_Control_SetVerboseMode(payload, 1, response, &resp_len);
    
    /* Enable message filtering */
    payload[0] = 1;
    resp_len = sizeof(response);
    Dlt_Control_SetMessageFiltering(payload, 1, response, &resp_len);
    
    /* Verify state */
    const Dlt_ControlStateType *state = Dlt_Control_GetState();
    TEST_ASSERT_EQUAL(DLT_LOG_WARN, state->default_log_level);
    TEST_ASSERT_FALSE(state->verbose_mode);
    TEST_ASSERT_TRUE(state->message_filtering);
    
    /* Reset to factory default */
    resp_len = sizeof(response);
    Dlt_Control_ResetToFactoryDefault(NULL, 0, response, &resp_len);
    
    state = Dlt_Control_GetState();
    TEST_ASSERT_EQUAL(DLT_LOG_INFO, state->default_log_level);
    TEST_ASSERT_TRUE(state->verbose_mode);
    TEST_ASSERT_TRUE(state->message_filtering);
    
    Dlt_Control_DeInit();
}

/*===========================================================================*/
/* Test Runner                                                               */
/*===========================================================================*/

int main(void) {
    UNITY_BEGIN();
    
    /* Payload Type Info Tests */
    RUN_TEST(test_payload_type_info_log_message);
    RUN_TEST(test_payload_type_info_control_message);
    RUN_TEST(test_payload_type_info_app_trace);
    RUN_TEST(test_payload_type_info_network_trace);
    
    /* Payload Builder Tests */
    RUN_TEST(test_payload_builder_init);
    RUN_TEST(test_payload_builder_add_string);
    RUN_TEST(test_payload_builder_add_variable_uint32);
    RUN_TEST(test_payload_builder_add_raw_data);
    
    /* Control Message Tests */
    RUN_TEST(test_control_init);
    RUN_TEST(test_control_set_default_log_level);
    RUN_TEST(test_control_set_verbose_mode);
    RUN_TEST(test_control_register_service);
    
    /* Output Channel Tests */
    RUN_TEST(test_output_init);
    RUN_TEST(test_output_add_channel);
    RUN_TEST(test_output_statistics);
    
    /* Performance Tests */
    RUN_TEST(test_mempool_init);
    RUN_TEST(test_mempool_allocate_free);
    RUN_TEST(test_zerocopy_buffer_init);
    RUN_TEST(test_zerocopy_buffer_allocate_commit);
    RUN_TEST(test_lockfree_queue_init);
    RUN_TEST(test_lockfree_queue_enqueue_dequeue);
    RUN_TEST(test_performance_stats);
    
    /* Integration Tests */
    RUN_TEST(test_full_control_flow);
    
    return UNITY_END();
}
