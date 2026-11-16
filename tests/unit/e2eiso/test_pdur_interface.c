/**
 * @file test_pdur_interface.c
 * @brief PduR LoTp 缓冲接口测试 (diagnostics/isotp/pdur_interface.c)
 *
 * R11 回归: PduR_LoTpCopyTxData 之前用 "buffer->used - buffer->used" 计算
 * 剩余长度, 结果恒为 0 — 任何 TP 发送都取不到数据。
 */

#include "unity.h"
#include "pdur_interface.h"
#include "isotp_core.h"
#include <string.h>

/* PduR_IsotpTransmit() 会引用该符号 (本测试不调用), 提供空实现避免链接失败 */
Isotp_ReturnType IsoTp_Transmit(uint8_t channel, const uint8_t* data, uint16_t length, void* userData)
{
    (void)channel;
    (void)data;
    (void)length;
    (void)userData;
    return (Isotp_ReturnType)0;
}

static const PduR_PduConfigType k_pdus[] = {
    { 0U, PDUR_PROT_ISOTP, 64U, true },
};

void setUp(void) {}
void tearDown(void) {}

/** 暂存 3 字节后, CopyTxData 必须按请求分块取出, 并报告剩余量 */
static void test_copytxdata_returns_staged_bytes(void)
{
    PduInfoType info;
    uint16_t bufferSize = 0U;
    uint16_t available = 0U;
    uint8_t staged[3] = {0xAAU, 0xBBU, 0xCCU};
    uint8_t out[8];

    TEST_ASSERT_EQUAL_INT(PDUR_OK, PduR_Init(k_pdus, 1U, NULL, 0U));

    /* 通过 RX 接口把 3 字节暂存进 PDU 0 的缓冲区 */
    info.sduDataPtr = staged;
    info.metaDataPtr = NULL;
    info.sduLength = 3U;
    TEST_ASSERT_EQUAL_INT(PDUR_OK, PduR_LoTpStartOfReception(0U, &info, 3U, &bufferSize));
    TEST_ASSERT_EQUAL_INT(PDUR_OK, PduR_LoTpCopyRxData(0U, &info, &bufferSize));

    /* 请求 2 字节: 必须真的复制 2 字节 (修复前恒为 0) */
    (void)memset(out, 0, sizeof(out));
    info.sduDataPtr = out;
    info.sduLength = 2U;
    TEST_ASSERT_EQUAL_INT(PDUR_OK, PduR_LoTpCopyTxData(0U, &info, &available));
    TEST_ASSERT_EQUAL_UINT16(2U, info.sduLength);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(staged, out, 2U);
    TEST_ASSERT_EQUAL_UINT16(1U, available);

    /* 剩余 1 字节: 应从缓冲区头部继续取出 */
    info.sduLength = 2U;
    TEST_ASSERT_EQUAL_INT(PDUR_OK, PduR_LoTpCopyTxData(0U, &info, &available));
    TEST_ASSERT_EQUAL_UINT16(1U, info.sduLength);
    TEST_ASSERT_EQUAL_UINT8(0xCCU, out[0]);
    TEST_ASSERT_EQUAL_UINT16(0U, available);

    PduR_DeInit();
}

/** 未分配缓冲区 / 空指针必须被拒绝 */
static void test_copytxdata_invalid_args(void)
{
    PduInfoType info;
    uint8_t out[4];

    TEST_ASSERT_EQUAL_INT(PDUR_INVALID_PARAMETER, PduR_LoTpCopyTxData(0U, NULL, NULL));

    (void)memset(&info, 0, sizeof(info));
    info.sduDataPtr = out;
    info.sduLength = 1U;

    TEST_ASSERT_EQUAL_INT(PDUR_OK, PduR_Init(k_pdus, 1U, NULL, 0U));

    /* 缓冲区未分配 -> PDUR_NO_BUFFER */
    TEST_ASSERT_EQUAL_INT(PDUR_NO_BUFFER, PduR_LoTpCopyTxData(0U, &info, NULL));

    /* 目标指针为空 -> PDUR_INVALID_PARAMETER */
    info.sduDataPtr = NULL;
    TEST_ASSERT_EQUAL_INT(PDUR_INVALID_PARAMETER, PduR_LoTpCopyTxData(0U, &info, NULL));

    PduR_DeInit();
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_copytxdata_returns_staged_bytes);
    RUN_TEST(test_copytxdata_invalid_args);
    return UNITY_END();
}
