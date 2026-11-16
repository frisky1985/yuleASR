/**
 * @file test_ara_com_e2e.c
 * @brief ara::com DDS 绑定层 E2E 接收路径测试
 *
 * R11 回归: ara_com_GetNewSamples() 之前用
 *   (e2eStatus & E2E_ERROR_NONE) != 0
 * 判断 E2E 校验结果, 而 E2E_ERROR_NONE 为 0 — 条件恒假, 所有受 E2E 保护的
 * 样本都被静默丢弃 (numSamples 恒为 0)。
 *
 * 本测试只链接 ara_com_Init()/ara_com_GetNewSamples() 所需的最小 stub,
 * 通过 -ffunction-sections + --gc-sections 丢弃其余未用代码的外部引用。
 */

#include "unity.h"
#include "ara_com_dds.h"
#include "e2e_protection.h"
#include "dds_runtime.h"
#include <string.h>

/* ---- 依赖 stub ---- */
static bool g_take_valid = true;
static uint16_t g_e2e_status = E2E_ERROR_NONE;
static uint32_t g_e2e_header = 8U;

eth_status_t dds_runtime_init(const dds_runtime_config_t *config)
{
    (void)config;
    return ETH_OK;
}

Std_ReturnType E2E_Protection_Init(void)
{
    return E_OK;
}

uint32_t E2E_GetHeaderSize(uint8_t profile)
{
    (void)profile;
    return g_e2e_header;
}

Std_ReturnType E2E_Check(E2E_ContextType *context, const void *data,
                         uint32_t length, uint16_t *status, uint8_t profile)
{
    (void)context;
    (void)data;
    (void)length;
    (void)profile;
    if (status != NULL) {
        *status = g_e2e_status;
    }
    return E_OK;
}

dds_ReturnCode_t dds_take(dds_DataReaderHandleType reader_handle, void *data,
                          dds_SampleInfoType *sample_info, uint32_t max_samples,
                          uint32_t sample_states, uint32_t view_states,
                          uint32_t instance_states)
{
    (void)reader_handle;
    (void)sample_states;
    (void)view_states;
    (void)instance_states;

    if (max_samples == 0U) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    if (data != NULL) {
        (void)memset(data, 0x5A, 4096U);   /* 调用方 sampleData 容量 */
    }
    if (sample_info != NULL) {
        (void)memset(sample_info, 0, sizeof(*sample_info));
        sample_info->valid_data = g_take_valid;
    }
    return DDS_RETCODE_OK;
}

void setUp(void)
{
    g_take_valid = true;
    g_e2e_status = E2E_ERROR_NONE;
    g_e2e_header = 8U;
}

void tearDown(void) {}

static void fill_handle(ara_com_ServiceHandleType *svc,
                        ara_com_EventHandleType *ev,
                        E2E_ContextType *e2eCtx)
{
    (void)memset(svc, 0, sizeof(*svc));
    (void)memset(ev, 0, sizeof(*ev));
    (void)memset(e2eCtx, 0, sizeof(*e2eCtx));
    svc->e2eContext = e2eCtx;
    ev->service = svc;
    ev->readerHandle = (dds_DataReaderHandleType)0x1234U;
}

/** E2E 校验通过时样本必须交付 */
static void test_e2e_valid_sample_is_delivered(void)
{
    ara_com_ServiceHandleType svc;
    ara_com_EventHandleType ev;
    E2E_ContextType e2eCtx;
    uint8_t out[4096];
    uint32_t numSamples = 0U;

    (void)memset(out, 0, sizeof(out));
    fill_handle(&svc, &ev, &e2eCtx);
    TEST_ASSERT_EQUAL_INT(E_OK, ara_com_Init());

    TEST_ASSERT_EQUAL_INT(E_OK, ara_com_GetNewSamples(&ev, out, sizeof(out), &numSamples));
    TEST_ASSERT_EQUAL_UINT32(1U, numSamples);       /* 修复前恒为 0 */
}

/** E2E 校验失败时样本不得交付 */
static void test_e2e_invalid_sample_is_dropped(void)
{
    ara_com_ServiceHandleType svc;
    ara_com_EventHandleType ev;
    E2E_ContextType e2eCtx;
    uint8_t out[4096];
    uint32_t numSamples = 0U;

    fill_handle(&svc, &ev, &e2eCtx);
    TEST_ASSERT_EQUAL_INT(E_OK, ara_com_Init());

    g_e2e_status = 0x0001U;     /* 非 E2E_ERROR_NONE */
    TEST_ASSERT_EQUAL_INT(E_OK, ara_com_GetNewSamples(&ev, out, sizeof(out), &numSamples));
    TEST_ASSERT_EQUAL_UINT32(0U, numSamples);
}

/** 未配置 E2E 上下文时样本照常交付 (回归保护) */
static void test_no_e2e_sample_is_delivered(void)
{
    ara_com_ServiceHandleType svc;
    ara_com_EventHandleType ev;
    E2E_ContextType e2eCtx;
    uint8_t out[4096];
    uint32_t numSamples = 0U;

    fill_handle(&svc, &ev, &e2eCtx);
    svc.e2eContext = NULL;
    TEST_ASSERT_EQUAL_INT(E_OK, ara_com_Init());

    TEST_ASSERT_EQUAL_INT(E_OK, ara_com_GetNewSamples(&ev, out, sizeof(out), &numSamples));
    TEST_ASSERT_EQUAL_UINT32(1U, numSamples);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_e2e_valid_sample_is_delivered);
    RUN_TEST(test_e2e_invalid_sample_is_dropped);
    RUN_TEST(test_no_e2e_sample_is_delivered);
    return UNITY_END();
}
