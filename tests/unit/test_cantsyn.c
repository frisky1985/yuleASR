/*
 * @file test_cantsyn.c
 * @brief CanTSyn (时间同步) 单元测试
 * 
 * 测试范围:
 * - SYNC报文发送和接收
 * - Follow-up报文处理
 * - 时间戳计算
 * - 时间偏移补偿
 * - 时间同步精度
 * - 超时处理
 *
 * AUTOSAR Standard: R22-11
 * ASIL Level: B
 */

// @tests src/bsw/services/cantsyn/src/CanTSyn.c  @tests src/bsw/services/cantsyn/include/CanTSyn.h

#include <unity.h>
#include "CanTSyn.h"
#include "StbM.h"
#include "CanIf.h"
#include <string.h>

/* ================================ 测试数据 ================================ */

static CanTSyn_TimeStampType masterTime;
static CanTSyn_TimeStampType slaveTime;
static boolean syncReceived;
static boolean followUpReceived;
static sint64 timeOffset;

void setUp(void) {
    memset(&masterTime, 0, sizeof(masterTime));
    memset(&slaveTime, 0, sizeof(slaveTime));
    syncReceived = FALSE;
    followUpReceived = FALSE;
    timeOffset = 0;
    
    CanTSyn_Init(NULL_PTR);
}

void tearDown(void) {
    CanTSyn_DeInit();
}

/* ================================ SYNC 报文测试 ================================ */

/**
 * @brief 测试SYNC报文发送
 * @test CANTSYN_SYNC_001
 */
void test_CanTSyn_Sync_Transmit(void) {
    Std_ReturnType result = CanTSyn_TransmitSync(0);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief 测试SYNC报文接收
 * @test CANTSYN_SYNC_002
 */
void test_CanTSyn_Sync_Receive(void) {
    uint8 syncFrame[16];
    
    /* 构建SYNC报文 */
    memset(syncFrame, 0, sizeof(syncFrame));
    syncFrame[0] = 0x10;  /* Time Domain = 0, SGW = 1 */
    
    /* 模拟接收SYNC */
    CanIf_RxIndication(0, syncFrame, 16);
    
    /* 验证SYNC已接收 */
    TEST_ASSERT_TRUE(CanTSyn_IsSyncReceived(0));
}

/**
 * @brief 测试SYNC时间戳记录
 * @test CANTSYN_SYNC_003
 */
void test_CanTSyn_Sync_Timestamp(void) {
    StbM_TimeStampType timestamp;
    
    /* 记录SYNC发送时间戳 */
    timestamp.seconds = 1000;
    timestamp.nanoseconds = 500000000;
    timestamp.timeBaseStatus = 0;
    
    CanTSyn_SetSyncTxTimestamp(0, &timestamp);
    
    /* 验证时间戳已记录 */
    StbM_TimeStampType recorded;
    CanTSyn_GetSyncTxTimestamp(0, &recorded);
    
    TEST_ASSERT_EQUAL_UINT32(1000, recorded.seconds);
    TEST_ASSERT_EQUAL_UINT32(500000000, recorded.nanoseconds);
}

/* ================================ Follow-up 测试 ================================ */

/**
 * @brief 测试Follow-up报文发送
 * @test CANTSYN_FUP_001
 */
void test_CanTSyn_FollowUp_Transmit(void) {
    /* 先发送SYNC */
    CanTSyn_TransmitSync(0);
    
    /* 然后发送Follow-up */
    Std_ReturnType result = CanTSyn_TransmitFollowUp(0);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief 测试Follow-up时间戳传递
 * @test CANTSYN_FUP_002
 */
void test_CanTSyn_FollowUp_Timestamp(void) {
    uint8 fupFrame[16];
    
    /* 构建Follow-up报文包含精确时间戳 */
    memset(fupFrame, 0, sizeof(fupFrame));
    fupFrame[0] = 0x18;  /* Time Domain = 0, SGW = 1, Follow-up = 1 */
    
    /* 精确时间戳: seconds = 1000, nanoseconds = 500000000 */
    fupFrame[1] = 0x00;  /* secondsHigh = 0 */
    fupFrame[2] = 0x00;
    fupFrame[3] = 0x03;  /* seconds = 1000 = 0x3E8 */
    fupFrame[4] = 0xE8;
    fupFrame[5] = 0x1D;  /* nanoseconds = 500000000 = 0x1DCD6500 */
    fupFrame[6] = 0xCD;
    fupFrame[7] = 0x65;
    fupFrame[8] = 0x00;
    
    CanIf_RxIndication(0, fupFrame, 16);
    
    /* 验证Follow-up已接收 */
    TEST_ASSERT_TRUE(CanTSyn_IsFollowUpReceived(0));
}

/* ================================ 时间计算测试 ================================ */

/**
 * @brief 测试时间偏移计算
 * @test CANTSYN_OFFSET_001
 */
void test_CanTSyn_TimeOffset_Calculation(void) {
    /* 模拟Master和Slave时间 */
    CanTSyn_TimeStampType masterTx;
    masterTx.seconds = 1000;
    masterTx.nanoseconds = 0;
    
    CanTSyn_TimeStampType slaveRx;
    slaveRx.seconds = 1000;
    slaveRx.nanoseconds = 100000;  /* 100us偏移 */
    
    /* 计算偏移 */
    sint64 offset = CanTSyn_CalculateTimeOffset(&masterTx, &slaveRx);
    
    /* 验证偏移正确 */
    TEST_ASSERT_EQUAL(100000, offset);  /* 100us in nanoseconds */
}

/**
 * @brief 测试时间同步精度
 * @test CANTSYN_ACCURACY_001
 */
void test_CanTSyn_Sync_Accuracy(void) {
    /* 执行多次时间同步 */
    for (uint8 i = 0; i < 10; i++) {
        CanTSyn_MainFunction();
    }
    
    /* 获取当前时间同步精度 */
    uint32 accuracy = CanTSyn_GetCurrentSyncAccuracy(0);
    
    /* 验证精度在合理范围内 (< 1ms) */
    TEST_ASSERT_LESS_THAN(1000000, accuracy);  /* 1ms = 1000000ns */
}

/* ================================ 超时测试 ================================ */

/**
 * @brief 测试SYNC超时处理
 * @test CANTSYN_TIMEOUT_001
 */
void test_CanTSyn_Sync_Timeout(void) {
    /* 启动时间同步 */
    CanTSyn_StartTimeSync(0);
    
    /* 模拟超过最大超时时间 */
    for (uint16 i = 0; i < 1000; i++) {
        CanTSyn_MainFunction();
    }
    
    /* 验证时间同步状态 */
    TEST_ASSERT_EQUAL(CANTSYN_TIMEOUT, CanTSyn_GetTimeSyncStatus(0));
}

/**
 * @brief 测试重复SYNC快速跟踪
 * @test CANTSYN_TIMEOUT_002
 */
void test_CanTSyn_Rapid_Sync_Recovery(void) {
    /* 发送多个快速SYNC消息 */
    for (uint8 i = 0; i < 5; i++) {
        CanTSyn_TransmitSync(0);
        CanTSyn_MainFunction();
    }
    
    /* 验证同步状态正常 */
    TEST_ASSERT_EQUAL(CANTSYN_SYNCED, CanTSyn_GetTimeSyncStatus(0));
}

/* ================================ 主函数 ================================ */

int main(void) {
    UNITY_BEGIN();
    
    /* SYNC测试 */
    RUN_TEST(test_CanTSyn_Sync_Transmit);
    RUN_TEST(test_CanTSyn_Sync_Receive);
    RUN_TEST(test_CanTSyn_Sync_Timestamp);
    
    /* Follow-up测试 */
    RUN_TEST(test_CanTSyn_FollowUp_Transmit);
    RUN_TEST(test_CanTSyn_FollowUp_Timestamp);
    
    /* 时间计算测试 */
    RUN_TEST(test_CanTSyn_TimeOffset_Calculation);
    RUN_TEST(test_CanTSyn_Sync_Accuracy);
    
    /* 超时测试 */
    RUN_TEST(test_CanTSyn_Sync_Timeout);
    RUN_TEST(test_CanTSyn_Rapid_Sync_Recovery);
    
    return UNITY_END();
}
