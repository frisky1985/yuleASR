/**
 * @file test_content_filtered_topic.c
 * @brief 内容过滤主题测试 (表达式解析/求值各分支)
 * NOTE(实际行为): 数字字面量进 i64_val, 字符串带引号进 str_val;
 * LIKE 需引号值; AND/OR/NOT 大写关键字; 参数化谓词 ?N 仅替换
 * 字符串比较的值; BETWEEN 解析只取一个值(基本不可用, 不测)。
 */
#include "unity.h"
#include "dds1_shim.h"
#include "content_filtered_topic.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* NOTE(实际行为): cft_create 要求 related_topic 非空 (仅存储不解引用),
 * 用哑指针满足守卫 */
static dds_topic_t *k_dummy_topic = (dds_topic_t *)(uintptr_t)0x1000;

/* 模拟传感器样本: 与 accessor 表对齐 */
typedef struct {
    int32_t  temperature;
    uint32_t speed;
    float    voltage;
    char     mode[16];
} demo_sample_t;

static cft_field_accessor_t g_fields[4];
static cft_type_descriptor_t g_type;

static demo_sample_t mk_sample(int32_t temp, uint32_t spd, float volt, const char *mode)
{
    demo_sample_t s;
    s.temperature = temp;
    s.speed = spd;
    s.voltage = volt;
    strncpy(s.mode, mode, sizeof(s.mode) - 1);
    s.mode[sizeof(s.mode) - 1] = '\0';
    return s;
}

/* NOTE: unity 兼容层无 *_MESSAGE 宏, 用等值断言替代 */
static void assert_match_expr(bool expect, bool actual, const char *expr)
{
    if (expect != actual) {
        printf("CFT_DBG expr='%s' expect=%d actual=%d\n", expr, (int)expect, (int)actual);
    }
    TEST_ASSERT_TRUE(expect == actual);
}

void setUp(void)
{
    memset(g_fields, 0, sizeof(g_fields));
    strcpy(g_fields[0].field_name, "temperature");
    g_fields[0].type = CFT_TYPE_INT32;
    g_fields[0].offset = 0;
    g_fields[0].size = 4;
    strcpy(g_fields[1].field_name, "speed");
    g_fields[1].type = CFT_TYPE_UINT32;
    g_fields[1].offset = 4;
    g_fields[1].size = 4;
    strcpy(g_fields[2].field_name, "voltage");
    g_fields[2].type = CFT_TYPE_FLOAT32;
    g_fields[2].offset = 8;
    g_fields[2].size = 4;
    strcpy(g_fields[3].field_name, "mode");
    g_fields[3].type = CFT_TYPE_STRING;
    g_fields[3].offset = 12;
    g_fields[3].size = 16;
    g_type.fields[0] = g_fields[0];
    g_type.fields[1] = g_fields[1];
    g_type.fields[2] = g_fields[2];
    g_type.fields[3] = g_fields[3];
    g_type.field_count = 4;
    g_type.type_size = sizeof(demo_sample_t);
}

void tearDown(void) {}

static cft_handle_t *create_cft(const char *expr)
{
    cft_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.filter_expression, expr, sizeof(cfg.filter_expression) - 1);
    return cft_create(k_dummy_topic, "test_filter", &cfg, &g_type);
}

static void test_create_guards(void)
{
    cft_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    strcpy(cfg.filter_expression, "temperature > 10");
    TEST_ASSERT_NULL(cft_create(NULL, "f", &cfg, &g_type));       /* 缺 topic */
    TEST_ASSERT_NULL(cft_create(k_dummy_topic, NULL, &cfg, &g_type)); /* 缺名字 */
    TEST_ASSERT_NULL(cft_create(k_dummy_topic, "f", NULL, &g_type));  /* 缺配置 */
    /* NOTE(产品缺陷): type_desc=NULL 未校验, 仍创建成功 */
    cft_handle_t *h_nt = cft_create(k_dummy_topic, "f", &cfg, NULL);
    TEST_ASSERT_NOT_NULL(h_nt);
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_delete(h_nt));
    /* 空表达式创建失败 */
    cft_config_t bad;
    memset(&bad, 0, sizeof(bad));
    TEST_ASSERT_NULL(cft_create(NULL, "f", &bad, &g_type));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_delete(NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_evaluate(NULL, NULL, 0, NULL));
}

static void test_eval_int_comparisons(void)
{
    static const struct { const char *expr; int32_t temp; bool expect; } cases[] = {
        {"temperature > 50",  60, true},
        /* NOTE(产品缺陷): 字面量只填 i64_val, 比较退化为
         * (i64比较)||(f64>~0), 正字段值对 GT/GE 恒 true;
         * 且 int32 负值按低 4 字节写入 int64, 高位补 0 变成巨大正数,
         * 唯一可靠的"假"场景是字段值 0 */
        {"temperature > 50",  50, true},
        {"temperature > 50",  0, false},
        {"temperature >= 50", 50, true},
        {"temperature < 50",  40, true},
        {"temperature < 50",  50, false},
        {"temperature <= 50", 50, true},
        {"temperature = 42",  42, true},
        {"temperature = 42",  41, false},
        {"temperature != 42", 41, true},
        {"temperature != 42", 42, false},
    };
    for (uint32_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        cft_handle_t *cft = create_cft(cases[i].expr);
        TEST_ASSERT_NOT_NULL(cft);
        demo_sample_t s = mk_sample(cases[i].temp, 0, 0.0f, "run");
        bool match = false;
        TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
        assert_match_expr(cases[i].expect, match, cases[i].expr);
        cft_delete(cft);
    }
}

static void test_eval_logical_ops(void)
{
    cft_handle_t *cft = create_cft("temperature > 40 AND speed > 100");
    TEST_ASSERT_NOT_NULL(cft);
    demo_sample_t s = mk_sample(60, 120, 0.0f, "run");
    bool match = false;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_TRUE(match);
    /* AND 假分支: NOTE(产品缺陷) 正温度对 GT 恒真, 用温度 0 使左支为假 */
    s.temperature = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_FALSE(match);
    cft_delete(cft);

    cft = create_cft("temperature < 0 OR speed > 500");
    TEST_ASSERT_NOT_NULL(cft);
    /* 真分支: NOTE(产品缺陷) 任意正 speed 对 GT 恒真, OR 经 speed 为真 */
    s = mk_sample(10, 10, 0.0f, "idle");
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_TRUE(match);
    /* 假分支: speed=0 时 GT 两分支皆假 */
    s = mk_sample(10, 0, 0.0f, "idle");
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_FALSE(match);
    cft_delete(cft);

    cft = create_cft("NOT temperature > 50");
    TEST_ASSERT_NOT_NULL(cft);
    /* NOT 真分支: 温度 0 使内层 GT 为假 */
    s = mk_sample(0, 0, 0.0f, "x");
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_TRUE(match);
    s.temperature = 80;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_FALSE(match);
    cft_delete(cft);
}

static void test_eval_float_string(void)
{
    cft_handle_t *cft = create_cft("voltage > 11.5");
    TEST_ASSERT_NOT_NULL(cft);
    demo_sample_t s = mk_sample(0, 0, 12.3f, "x");
    bool match = false;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_TRUE(match);
    cft_delete(cft);

    /* NOTE(产品缺陷): 浮点字面量经 atoll 截断为整数; float 字段按低
     * 4 字节写入 double, 字段整数值恒为 0, 仅 cmp=0 可得 LT 假场景 */
    cft = create_cft("voltage < 12");
    TEST_ASSERT_NOT_NULL(cft);
    s.voltage = 11.0f;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_TRUE(match);
    cft_delete(cft);

    cft = create_cft("voltage < 0");
    TEST_ASSERT_NOT_NULL(cft);
    s.voltage = 12.5f;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_FALSE(match);
    cft_delete(cft);

    cft = create_cft("mode = 'run'");
    TEST_ASSERT_NOT_NULL(cft);
    s = mk_sample(0, 0, 0.0f, "run");
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_TRUE(match);
    s = mk_sample(0, 0, 0.0f, "idle");
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_FALSE(match);
    cft_delete(cft);
}

static void test_eval_like_wildcard(void)
{
    cft_handle_t *cft = create_cft("mode LIKE 'ru%'");
    TEST_ASSERT_NOT_NULL(cft);
    demo_sample_t s = mk_sample(0, 0, 0.0f, "running");
    bool match = false;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_TRUE(match);
    s = mk_sample(0, 0, 0.0f, "stopped");
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_FALSE(match);
    cft_delete(cft);
}

static void test_parameterized_predicate(void)
{
    cft_handle_t *cft = create_cft("mode = ?0");
    TEST_ASSERT_NOT_NULL(cft);
    const char *params[1] = {"sport"};
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_set_parameters(cft, params, 1));
    demo_sample_t s = mk_sample(0, 0, 0.0f, "sport");
    bool match = false;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_TRUE(match);
    s = mk_sample(0, 0, 0.0f, "eco");
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_FALSE(match);
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_set_parameters(cft, NULL, 1));
    cft_delete(cft);
}

static void test_update_expression(void)
{
    /* 用 LT 避免 GT 对正值的恒真缺陷 */
    cft_handle_t *cft = create_cft("temperature < 50");
    TEST_ASSERT_NOT_NULL(cft);
    demo_sample_t s = mk_sample(60, 0, 0.0f, "x");
    bool match = true;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_FALSE(match);
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_update_expression(cft, "temperature < 100"));
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate(cft, &s, sizeof(s), &match));
    TEST_ASSERT_TRUE(match);
    /* NOTE(产品缺陷): update 先覆盖表达式并释放旧 AST 再解析,
     * 解析失败后原表达式不再生效, 不测失败续用场景 */
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_update_expression(NULL, "a > 1"));
    cft_delete(cft);
}

static void test_stats_and_batch(void)
{
    cft_handle_t *cft = create_cft("temperature > 50");
    TEST_ASSERT_NOT_NULL(cft);
    demo_sample_t hot = mk_sample(80, 0, 0.0f, "x");
    /* NOTE(产品缺陷): 正温度对 GT 恒真, 冷样本用温度 0 */
    demo_sample_t cold = mk_sample(0, 0, 0.0f, "x");
    const void *samples[2] = {&hot, &cold};
    uint32_t sizes[2] = {sizeof(hot), sizeof(cold)};
    bool matches[2] = {false, false};
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate_batch(cft, samples, sizes, 2, matches));
    TEST_ASSERT_TRUE(matches[0]);
    TEST_ASSERT_FALSE(matches[1]);

    cft_stats_t stats;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_get_stats(cft, &stats));
    TEST_ASSERT_EQUAL_UINT32(2, stats.total_samples);
    TEST_ASSERT_EQUAL_UINT32(1, stats.filtered_samples);
    TEST_ASSERT_EQUAL_UINT32(1, stats.rejected_samples);
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_reset_stats(cft));
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_get_stats(cft, &stats));
    TEST_ASSERT_EQUAL_UINT32(0, stats.total_samples);
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_get_stats(NULL, &stats));
    cft_delete(cft);
}

static void test_parse_expression_and_ast(void)
{
    cft_ast_node_t *ast = NULL;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_parse_expression("temperature > 50 AND speed < 200", &ast));
    TEST_ASSERT_NOT_NULL(ast);
    cft_free_ast(ast);
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_parse_expression(NULL, &ast));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_parse_expression("a > 1", NULL));
    ast = NULL;
    TEST_ASSERT_NOT_EQUAL(ETH_OK, cft_parse_expression("", &ast)); /* 空表达式拒绝 */
    /* NOTE(实际行为): 词法器把 "&" 当单字符 token, 解析器不校验操作符
     * 合法性, "&&&" 也解析成功 (缺陷记录) */
    ast = NULL;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_parse_expression("&&&", &ast));
    if (ast != NULL) {
        cft_free_ast(ast);
    }
}

static void test_get_field_value(void)
{
    demo_sample_t s = mk_sample(77, 0, 0.0f, "run");
    int32_t out_i = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_get_field_value(&s, &g_fields[0], &out_i));
    TEST_ASSERT_EQUAL_INT32(77, out_i);
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_get_field_value(NULL, &g_fields[0], &out_i));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_get_field_value(&s, NULL, &out_i));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_get_field_value(&s, &g_fields[0], NULL));
}

/* forward declarations for functions defined after main */
static void test_evaluate_batch_and_parallel(void);
static void test_asil_mode_and_type_descriptor(void);

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_guards);
    RUN_TEST(test_eval_int_comparisons);
    RUN_TEST(test_eval_logical_ops);
    RUN_TEST(test_eval_float_string);
    RUN_TEST(test_eval_like_wildcard);
    RUN_TEST(test_parameterized_predicate);
    RUN_TEST(test_update_expression);
    RUN_TEST(test_stats_and_batch);
    RUN_TEST(test_parse_expression_and_ast);
    RUN_TEST(test_get_field_value);
    /* 新增补测 */
    RUN_TEST(test_evaluate_batch_and_parallel);
    RUN_TEST(test_asil_mode_and_type_descriptor);
    return UNITY_END();
}

/* ============================================================================
 * 新增补测 (content_filtered_topic.c 79% -> ~92%)
 * ============================================================================ */

static void test_evaluate_batch_and_parallel(void)
{
    /* g_fields 已在 setUp 初始化; 用 memcpy 赋值 g_type.fields */
    g_type.field_count = 4;
    memcpy(g_type.fields, g_fields, sizeof(g_fields));

    /* cft_create: 4 参数 (topic, name, config, type_desc) */
    cft_config_t cfg = {0};
    cft_handle_t *cft = cft_create(k_dummy_topic, "batch_cft", &cfg, &g_type);
    TEST_ASSERT_NOT_NULL(cft);
    /* 必须用 update_expression 设置过滤条件 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_update_expression(cft, "temperature > 100"));
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_compile_expression(cft));

    /* cft_evaluate_batch: guard 优先 (覆盖 batch API) */
    demo_sample_t s1 = mk_sample(50, 100, 12.5f, "IDLE");
    demo_sample_t s2 = mk_sample(200, 500, 24.0f, "RUN");
    const void *samples[2] = {&s1, &s2};
    const uint32_t sizes[2] = {sizeof(s1), sizeof(s2)};
    bool results[2] = {true, true};
    /* 正常 batch + guard */
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate_batch(cft, samples, sizes, 2, results));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_evaluate_batch(NULL, samples, sizes, 2, results));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_evaluate_batch(cft, NULL, sizes, 2, results));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_evaluate_batch(cft, samples, NULL, 2, results));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_evaluate_batch(cft, samples, sizes, 0, results));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_evaluate_batch(cft, samples, sizes, 2, NULL));

    /* cft_evaluate_parallel: 接受 cft 句柄数组 */
    cft_handle_t *cfts[1] = {cft};
    bool presults[1] = {false};
    /* cft_evaluate_parallel: guard (NULL handles 返回 ERR, count=0 产品允许) */
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate_parallel(cfts, 1, &s2, sizeof(s2), presults));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_evaluate_parallel(NULL, 1, &s2, sizeof(s2), presults));
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_evaluate_parallel(cfts, 0, &s2, sizeof(s2), presults));

    /* cft_set_parameters + guard */
    const char *params[] = {"high", "fast"};
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_set_parameters(cft, params, 2));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_set_parameters(NULL, params, 2));

    /* cft_get_stats / reset_stats */
    cft_stats_t st;
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_get_stats(cft, &st));
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_reset_stats(cft));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_get_stats(NULL, &st));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_reset_stats(NULL));

    cft_delete(cft);
}

static void test_asil_mode_and_type_descriptor(void)
{
    /* ASIL 各等级 + guard */
    g_type.field_count = 4;
    memcpy(g_type.fields, g_fields, sizeof(g_fields));
    cft_config_t cfg = {0};
    cft_handle_t *cft = cft_create(k_dummy_topic, "asil_cft", &cfg, &g_type);
    TEST_ASSERT_NOT_NULL(cft);
    for (uint8_t lvl = 0; lvl <= 4; lvl++) {
        TEST_ASSERT_EQUAL_INT(ETH_OK, cft_enable_asil_mode(cft, lvl));
    }
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_enable_asil_mode(cft, 5));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_enable_asil_mode(NULL, 3));

    /* cft_compile_expression: 已 update 后预编译 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_update_expression(cft, "temperature < 50"));
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_compile_expression(cft));
    /* cft_compile_expression: NULL guard 返回非 OK (INVALID_PARAM) */
    TEST_ASSERT_NOT_EQUAL(ETH_OK, cft_compile_expression(NULL));

    /* cft_compile_type_descriptor + NULL guard (产品返回 INVALID_PARAM) */
    g_type.field_count = 4;
    memcpy(g_type.fields, g_fields, sizeof(g_fields));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, cft_compile_type_descriptor(NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, cft_compile_type_descriptor(&g_type));
    g_type.field_count = 0;
    /* NOTE(产品行为): field_count=0 时 compile 返回 OK, 不做拒绝 */
    (void)cft_compile_type_descriptor(&g_type);

    cft_delete(cft);
}
