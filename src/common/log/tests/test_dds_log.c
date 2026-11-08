/*******************************************************************************
 * @file test_dds_log.c
 * @brief dds_log 模块宿主机单元测试 (P3 覆盖率提升)
 *
 * 仅使用 dds_log.h 的公开 API, 不修改生产代码。
 ******************************************************************************/

#include "dds_log.h"

#include <stdio.h>
#include <string.h>

static int g_failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                   \
        }                                                                   \
    } while (0)

/*=========================== 回调桩 ===========================*/

static unsigned g_output_cb1_calls = 0;
static unsigned g_output_cb2_calls = 0;
static unsigned g_audit_cb_calls = 0;
static unsigned g_ota_cb_calls = 0;
static bool g_ota_cb_success = false;
static char g_last_module[DDS_LOG_MAX_MODULE_LEN];

static void output_cb1(const dds_log_entry_t* entry, void* user_data)
{
    (void)user_data;
    if (entry != NULL) {
        ++g_output_cb1_calls;
        (void)strncpy(g_last_module, entry->module, sizeof(g_last_module) - 1U);
    }
}

static void output_cb2(const dds_log_entry_t* entry, void* user_data)
{
    (void)entry;
    (void)user_data;
    ++g_output_cb2_calls;
}

static void audit_cb(const dds_audit_event_t* event, void* user_data)
{
    (void)event;
    (void)user_data;
    ++g_audit_cb_calls;
}

static void ota_cb(const dds_ota_log_entry_t* entry, bool success, void* user_data)
{
    (void)entry;
    (void)user_data;
    ++g_ota_cb_calls;
    g_ota_cb_success = success;
}

/*=========================== 配置辅助 ===========================*/

static void config_basic(dds_log_config_t* cfg)
{
    (void)memset(cfg, 0, sizeof(*cfg));
    cfg->level = DDS_LOG_LEVEL_DEBUG;
    cfg->enable_async = false;
    cfg->enable_console = false;
    cfg->enable_file = false;
    cfg->enable_rotation = false;
    cfg->enable_color = true;
    cfg->enable_timestamp = true;
    cfg->enable_location = true;
    cfg->enable_thread_id = true;
    cfg->enable_audit = true;
    cfg->enable_uds_trace = true;
    cfg->max_file_size = DDS_LOG_FILE_SIZE_MAX;
    cfg->max_files = DDS_LOG_MAX_FILES;
    cfg->queue_size = DDS_LOG_QUEUE_SIZE;
    cfg->flush_interval_ms = 5U;
    (void)strncpy(cfg->log_name, "unit", sizeof(cfg->log_name) - 1U);
    (void)strncpy(cfg->ecu_id, "ECU1", sizeof(cfg->ecu_id) - 1U);
}

/*=========================== 用例 ===========================*/

static void test_lifecycle(void)
{
    dds_log_config_t cfg;

    CHECK(dds_log_is_initialized() == false);
    dds_log_deinit(); /* 未初始化时为空操作 */
    dds_log_flush();  /* 未初始化时为空操作 */

    /* 默认配置 (config == NULL) */
    CHECK(dds_log_init(NULL) == 0);
    CHECK(dds_log_is_initialized() == true);
    CHECK(dds_log_init(NULL) == -1); /* 重复初始化 */
    dds_log_deinit();
    CHECK(dds_log_is_initialized() == false);
    dds_log_deinit(); /* 再次反初始化 */

    /* 显式配置 */
    config_basic(&cfg);
    CHECK(dds_log_init(&cfg) == 0);
    CHECK(dds_log_is_initialized() == true);
    dds_log_deinit();
    CHECK(dds_log_is_initialized() == false);
}

static void test_levels(void)
{
    dds_log_config_t cfg;

    dds_log_set_level(DDS_LOG_LEVEL_WARN);
    CHECK(dds_log_get_level() == DDS_LOG_LEVEL_WARN);
    dds_log_set_level(DDS_LOG_LEVEL_MAX); /* 非法, 应忽略 */
    CHECK(dds_log_get_level() == DDS_LOG_LEVEL_WARN);
    dds_log_set_level(DDS_LOG_LEVEL_NONE);
    CHECK(dds_log_get_level() == DDS_LOG_LEVEL_NONE);

    config_basic(&cfg);
    cfg.level = DDS_LOG_LEVEL_INFO;
    CHECK(dds_log_init(&cfg) == 0);

    /* 模块级别: 新增 / 更新 / 查询 / 缺省回退 */
    dds_log_set_module_level("MODA", DDS_LOG_LEVEL_ERROR);
    CHECK(dds_log_get_module_level("MODA") == DDS_LOG_LEVEL_ERROR);
    dds_log_set_module_level("MODA", DDS_LOG_LEVEL_DEBUG);
    CHECK(dds_log_get_module_level("MODA") == DDS_LOG_LEVEL_DEBUG);
    CHECK(dds_log_get_module_level("UNKNOWN_MOD") == DDS_LOG_LEVEL_INFO);
    CHECK(dds_log_get_module_level(NULL) == DDS_LOG_LEVEL_INFO);
    dds_log_set_module_level(NULL, DDS_LOG_LEVEL_DEBUG);      /* 忽略 */
    dds_log_set_module_level("MODB", DDS_LOG_LEVEL_MAX);      /* 忽略 */
    CHECK(dds_log_get_module_level("MODB") == DDS_LOG_LEVEL_INFO);

    /* 模块级别过滤: MODA 限制为 ERROR 后 INFO 应被丢弃 */
    dds_log_set_module_level("MODA", DDS_LOG_LEVEL_ERROR);
    dds_log_write(DDS_LOG_LEVEL_INFO, DDS_LOG_TYPE_RUNTIME, "MODA", "T",
                  __FILE__, __LINE__, "filtered %d", 1);
    dds_log_write(DDS_LOG_LEVEL_ERROR, DDS_LOG_TYPE_RUNTIME, "MODA", "T",
                  __FILE__, __LINE__, "kept %d", 1);

    /* 全局级别过滤 */
    dds_log_set_level(DDS_LOG_LEVEL_NONE);
    dds_log_write(DDS_LOG_LEVEL_DEBUG, DDS_LOG_TYPE_RUNTIME, "MODA", "T",
                  __FILE__, __LINE__, "dropped by level");

    /* level = NONE 时走 "UNKNOWN" 名称 / 默认颜色分支 */
    dds_log_write(DDS_LOG_LEVEL_NONE, (dds_log_type_t)99, "MODX", "T",
                  __FILE__, __LINE__, "unknown level/type");

    dds_log_deinit();
}

static void test_output_and_callbacks(void)
{
    dds_log_config_t cfg;
    dds_log_stats_t stats;

    config_basic(&cfg);
    cfg.enable_console = true;   /* 覆盖控制台/颜色/位置/TID/序列号格式化 */
    CHECK(dds_log_init(&cfg) == 0);

    dds_log_write(DDS_LOG_LEVEL_INFO, DDS_LOG_TYPE_RUNTIME, "MODC", "TAG",
                  "src/some/file.c", 42, "console message %d", 7);
    dds_log_write(DDS_LOG_LEVEL_ERROR, DDS_LOG_TYPE_AUDIT, "AUDIT", "",
                  "audit.c", 9, "audit message");
    dds_log_write(DDS_LOG_LEVEL_WARN, DDS_LOG_TYPE_SECURITY, "SEC", "S",
                  "no_slash.c", 3, "security message");
    dds_log_write(DDS_LOG_LEVEL_DEBUG, DDS_LOG_TYPE_DIAGNOSTIC, "DIAG", "D",
                  "diag.c", 4, "diag message");
    dds_log_write(DDS_LOG_LEVEL_DEBUG, DDS_LOG_TYPE_OTA, "OTA", "O",
                  "ota.c", 5, "ota message");

    CHECK(dds_log_register_output_callback(NULL, NULL) == -1);
    CHECK(dds_log_register_output_callback(output_cb1, NULL) == 0);
    CHECK(dds_log_register_output_callback(output_cb2, (void*)0x1) == 0);

    dds_log_write(DDS_LOG_LEVEL_INFO, DDS_LOG_TYPE_RUNTIME, "MODCALL", "T",
                  __FILE__, __LINE__, "callback %d", 1);
    CHECK(g_output_cb1_calls == 1U);
    CHECK(g_output_cb2_calls == 1U);
    CHECK(strcmp(g_last_module, "MODCALL") == 0);

    dds_log_unregister_output_callback(output_cb1);
    dds_log_write(DDS_LOG_LEVEL_INFO, DDS_LOG_TYPE_RUNTIME, "MODCALL", "T",
                  __FILE__, __LINE__, "callback %d", 2);
    CHECK(g_output_cb1_calls == 1U);
    CHECK(g_output_cb2_calls == 2U);
    dds_log_unregister_output_callback(output_cb1); /* 不存在, 安全 */

    dds_log_get_stats(&stats);
    CHECK(stats.total_logs >= 6U);
    dds_log_get_stats(NULL); /* 空指针安全 */

    /* 审计注册/注销 (audit_mask 默认 0, 事件分发路径不可达) */
    CHECK(dds_log_register_audit_callback(NULL, NULL) == -1);
    CHECK(dds_log_register_audit_callback(audit_cb, NULL) == 0);
    dds_log_audit(DDS_AUDIT_EVENT_LOGIN, __FILE__, __LINE__, "login %s", "user");
    CHECK(g_audit_cb_calls == 0U);
    dds_log_unregister_audit_callback(audit_cb);
    dds_log_unregister_audit_callback(audit_cb); /* 不存在, 安全 */

    /* UDS */
    dds_log_uds_event(0x22U, 0x62U, "read data");
    dds_log_uds_event(0x2EU, 0x7FU, NULL);
    dds_log_clear_uds_session(); /* 会话为空, 不打日志 */
    /* 会话非空时 clear/结束日志需被级别过滤, 否则 uds_mutex 会被递归加锁 */
    dds_log_set_level(DDS_LOG_LEVEL_NONE);
    dds_log_set_uds_session("SESSION-1");
    dds_log_clear_uds_session();
    dds_log_set_uds_session(NULL);
    dds_log_set_level(DDS_LOG_LEVEL_DEBUG);

    /* 未初始化配置开关 */
    dds_log_enable_console(false);
    dds_log_enable_file(false);

    dds_log_deinit();
}

static void test_ota(void)
{
    dds_log_config_t cfg;
    uint8_t buffer[16];
    uint32_t actual = 0xFFFFFFFFU;

    CHECK(dds_log_trigger_ota_upload("sess", true) == -1); /* 未初始化 */

    config_basic(&cfg);
    CHECK(dds_log_init(&cfg) == 0);

    CHECK(dds_log_get_ota_buffer(NULL, sizeof(buffer), &actual) == -1);
    CHECK(dds_log_get_ota_buffer(buffer, sizeof(buffer), NULL) == -1);
    CHECK(dds_log_get_ota_buffer(buffer, 0U, &actual) == -1);
    CHECK(dds_log_get_ota_buffer(buffer, sizeof(buffer), &actual) == 0);
    CHECK(actual == 0U);

    dds_log_set_ota_callback(ota_cb, NULL);
    CHECK(dds_log_trigger_ota_upload("session-1", true) == 0);
    CHECK(g_ota_cb_calls == 1U);
    CHECK(g_ota_cb_success == true);
    CHECK(dds_log_trigger_ota_upload(NULL, false) == 0);
    CHECK(g_ota_cb_calls == 2U);

    dds_log_set_ota_callback(NULL, NULL);
    CHECK(dds_log_trigger_ota_upload(NULL, false) == 0);

    dds_log_deinit();
}

static void test_file_output(void)
{
    dds_log_config_t cfg;
    char path[DDS_LOG_MAX_PATH_LEN];
    char small[8];
    char* files[4];
    uint32_t count = 0U;

    /* 目录创建失败 + 文件打开失败分支 */
    config_basic(&cfg);
    cfg.enable_file = true;
    (void)strncpy(cfg.log_dir, "/nonexistent_yule_dir_xyz/sub",
                  sizeof(cfg.log_dir) - 1U);
    CHECK(dds_log_init(&cfg) == 0);
    CHECK(dds_log_get_current_file(path, sizeof(path)) == -1);
    dds_log_write(DDS_LOG_LEVEL_INFO, DDS_LOG_TYPE_RUNTIME, "M", "T",
                  __FILE__, __LINE__, "no file");
    dds_log_deinit();

    /* 正常文件输出 */
    config_basic(&cfg);
    cfg.enable_file = true;
    (void)strncpy(cfg.log_dir, "/tmp/yule_dds_log_test", sizeof(cfg.log_dir) - 1U);
    CHECK(dds_log_init(&cfg) == 0);

    dds_log_write(DDS_LOG_LEVEL_INFO, DDS_LOG_TYPE_RUNTIME, "M", "T",
                  __FILE__, __LINE__, "file message %d", 1);
    dds_log_flush();

    CHECK(dds_log_get_current_file(NULL, sizeof(path)) == -1);
    CHECK(dds_log_get_current_file(path, 0U) == -1);
    CHECK(dds_log_get_current_file(path, sizeof(path)) == 0);
    CHECK(strstr(path, "/tmp/yule_dds_log_test/") != NULL);
    CHECK(dds_log_get_current_file(small, sizeof(small)) == 0); /* 截断路径 */

    CHECK(dds_log_get_file_list(NULL, 4U, &count) == -1);
    CHECK(dds_log_get_file_list(files, 0U, &count) == -1);
    CHECK(dds_log_get_file_list(files, 4U, NULL) == -1);
    CHECK(dds_log_get_file_list(files, 4U, &count) == 0);
    CHECK(count >= 1U);
    CHECK(count <= 4U);
    CHECK(dds_log_get_file_list(files, 128U, &count) == 0);
    CHECK(count <= (uint32_t)DDS_LOG_MAX_FILES);

    dds_log_deinit();

    /* 轮转: 每次写入都超过 max_file_size, 触发多次 open_new_log_file */
    config_basic(&cfg);
    cfg.enable_file = true;
    cfg.enable_rotation = true;
    cfg.max_file_size = 1U;
    (void)strncpy(cfg.log_dir, "/tmp/yule_dds_log_test", sizeof(cfg.log_dir) - 1U);
    CHECK(dds_log_init(&cfg) == 0);
    for (int i = 0; i < 3; ++i) {
        dds_log_write(DDS_LOG_LEVEL_INFO, DDS_LOG_TYPE_RUNTIME, "M", "T",
                      __FILE__, __LINE__, "rotate %d", i);
    }
    CHECK(dds_log_get_file_list(files, 4U, &count) == 0);
    CHECK(count >= 2U);
    dds_log_deinit();

    /* 不存在的目录 -> opendir 失败 */
    config_basic(&cfg);
    cfg.enable_file = false;
    (void)strncpy(cfg.log_dir, "/nonexistent_yule_dir_abc",
                  sizeof(cfg.log_dir) - 1U);
    CHECK(dds_log_init(&cfg) == 0);
    CHECK(dds_log_get_file_list(files, 4U, &count) == -1);
    CHECK(dds_log_export_to_pcap("in.log", "out.pcap") == 0);
    dds_log_deinit();
}

int main(void)
{
    test_lifecycle();
    test_levels();
    test_output_and_callbacks();
    test_ota();
    test_file_output();

    if (g_failures != 0) {
        fprintf(stderr, "dds_log tests failed: %d\n", g_failures);
        return 1;
    }
    printf("dds_log tests passed\n");
    return 0;
}
