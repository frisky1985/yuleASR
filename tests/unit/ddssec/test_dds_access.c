/**
 * @file test_dds_access.c
 * @brief DDS-Security 访问控制模块 (dds_access.c) 单元测试
 *
 * 链接真实产品源码, 断言真实行为:
 * - 权限/治理 XML 解析
 * - 参与者权限加载/卸载与默认策略决策
 * - 主题通配符匹配
 * - 治理保护开关与统计
 */
#include "unity.h"
#include "dds_access.h"
#include <string.h>
#include <stdio.h>

#define PERM_XML_PATH  "/tmp/ddssec_test_permissions.xml"
#define GOV_XML_PATH   "/tmp/ddssec_test_governance.xml"

static dds_access_context_t *g_acc;
static dds_security_config_t g_cfg;
/* NOTE: sizeof(dds_permissions_config_t)≈546MB, sizeof(dds_governance_config_t)
 * ≈2.2MB — 均不可放栈上, 统一静态分配 */
static dds_governance_config_t s_gov_cfg;

void setUp(void)
{
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_acc = dds_access_init(&g_cfg);
    TEST_ASSERT_NOT_NULL(g_acc);
}

void tearDown(void)
{
    if (g_acc) {
        dds_access_deinit(g_acc);
        g_acc = NULL;
    }
    remove(PERM_XML_PATH);
    remove(GOV_XML_PATH);
}

/* ── init / deinit ─────────────────────────────────────────────────────── */

static void test_init_null_config_returns_null(void)
{
    TEST_ASSERT_NULL(dds_access_init(NULL));
}

static void test_init_defaults(void)
{
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_MAX_SUBJECTS, (int)g_acc->max_subjects);
    TEST_ASSERT_FALSE(g_acc->default_policy.default_deny);
    TEST_ASSERT_EQUAL((int)DDS_PERM_SCOPE_DOMAIN, (int)g_acc->default_policy.scope);
    TEST_ASSERT_NOT_NULL(g_acc->permissions_db);
}

static void test_init_copies_paths(void)
{
    dds_security_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.permissions_ca_cert_path, "/tmp/ca.pem", sizeof(cfg.permissions_ca_cert_path) - 1);
    strncpy(cfg.permissions_xml_path, "/tmp/perm.xml", sizeof(cfg.permissions_xml_path) - 1);
    strncpy(cfg.governance_xml_path, "/tmp/gov.xml", sizeof(cfg.governance_xml_path) - 1);

    dds_access_context_t *ctx = dds_access_init(&cfg);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL_STRING("/tmp/ca.pem", ctx->permissions_ca_cert_path);
    TEST_ASSERT_EQUAL_STRING("/tmp/perm.xml", ctx->permissions_file_path);
    TEST_ASSERT_EQUAL_STRING("/tmp/gov.xml", ctx->governance_file_path);
    dds_access_deinit(ctx);
}

static void test_deinit_null_safe(void)
{
    dds_access_deinit(NULL);
}

/* ── 权限 XML 解析 ─────────────────────────────────────────────────────── */

static void test_parse_permissions_xml_guards(void)
{
    /* NOTE: sizeof(dds_permissions_config_t)≈546MB (产品结构超巨大),
     * 局部声明会栈溢出, 必须静态化 */
    static dds_permissions_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_CONFIG,
                      (int)dds_access_parse_permissions_xml(NULL, PERM_XML_PATH, &cfg));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_CONFIG,
                      (int)dds_access_parse_permissions_xml(g_acc, NULL, &cfg));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_CONFIG,
                      (int)dds_access_parse_permissions_xml(g_acc, PERM_XML_PATH, NULL));
    /* 文件不存在 */
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_CONFIG,
                      (int)dds_access_parse_permissions_xml(g_acc, "/tmp/ddssec_no_such.xml", &cfg));
}

static void write_permissions_xml(const char *path, int subject_count)
{
    FILE *fp = fopen(path, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("<?xml version=\"1.0\"?>\n<dds xmlns=\"ddssec\">\n", fp);
    for (int s = 0; s < subject_count; s++) {
        fprintf(fp,
            "  <subject>\n"
            "    <subject_name>CN=TestAlice%d,O=DDS</subject_name>\n"
            "    <validity>\n"
            "      <not_before>2026-01-01T00:00:00</not_before>\n"
            "      <not_after>2027-01-01T00:00:00</not_after>\n"
            "    </validity>\n"
            "    <domains>\n"
            "      <domain>\n"
            "        <id>0</id>\n"
            "        <publish><topic>Sensor.*</topic></publish>\n"
            "        <subscribe><topic>Actuator.*</topic></subscribe>\n"
            "      </domain>\n"
            "    </domains>\n"
            "  </subject>\n", s);
    }
    fputs("</dds>\n", fp);
    fclose(fp);
}

static void test_parse_permissions_xml_single_subject(void)
{
    static dds_permissions_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    write_permissions_xml(PERM_XML_PATH, 1);

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_parse_permissions_xml(g_acc, PERM_XML_PATH, &cfg));
    TEST_ASSERT_EQUAL(1, (int)cfg.subject_count);
    TEST_ASSERT_EQUAL(1, (int)cfg.version_major);
    TEST_ASSERT_EQUAL(0, (int)cfg.version_minor);
    TEST_ASSERT_EQUAL_STRING("CN=TestAlice0,O=DDS", cfg.subjects[0].subject_name);
    TEST_ASSERT_EQUAL_STRING("2026-01-01T00:00:00", cfg.subjects[0].validity_from);
    TEST_ASSERT_EQUAL_STRING("2027-01-01T00:00:00", cfg.subjects[0].validity_until);
    TEST_ASSERT_EQUAL(1, (int)cfg.subjects[0].domain_count);
    TEST_ASSERT_EQUAL(0, (int)cfg.subjects[0].domains[0].domain_id);
    TEST_ASSERT_EQUAL(1, (int)cfg.subjects[0].domains[0].publish_count);
    TEST_ASSERT_EQUAL_STRING("Sensor.*", cfg.subjects[0].domains[0].publish[0].topic_pattern);
    TEST_ASSERT_EQUAL(1, (int)cfg.subjects[0].domains[0].subscribe_count);
    TEST_ASSERT_EQUAL_STRING("Actuator.*", cfg.subjects[0].domains[0].subscribe[0].topic_pattern);
}

static void test_parse_permissions_xml_multi_subject(void)
{
    static dds_permissions_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    write_permissions_xml(PERM_XML_PATH, 3);

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_parse_permissions_xml(g_acc, PERM_XML_PATH, &cfg));
    TEST_ASSERT_EQUAL(3, (int)cfg.subject_count);
    TEST_ASSERT_EQUAL_STRING("CN=TestAlice1,O=DDS", cfg.subjects[1].subject_name);
    TEST_ASSERT_EQUAL_STRING("CN=TestAlice2,O=DDS", cfg.subjects[2].subject_name);
}

/* ── 治理 XML 解析 ─────────────────────────────────────────────────────── */

static void test_parse_governance_xml(void)
{
    static dds_governance_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_CONFIG,
                      (int)dds_access_parse_governance_xml(NULL, GOV_XML_PATH, &cfg));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_CONFIG,
                      (int)dds_access_parse_governance_xml(g_acc, NULL, &cfg));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_CONFIG,
                      (int)dds_access_parse_governance_xml(g_acc, GOV_XML_PATH, NULL));

    /* NOTE 实际行为: 治理文件不存在不报错, 返回默认版本 */
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_parse_governance_xml(g_acc, "/tmp/ddssec_no_gov.xml", &cfg));
    TEST_ASSERT_EQUAL(1, (int)cfg.version_major);
    TEST_ASSERT_EQUAL(0, (int)cfg.domain_count);

    /* 有效文件: 解析 domain id 与 allow_unauthenticated_participants */
    FILE *fp = fopen(GOV_XML_PATH, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("<dds>\n"
          "  <domain>\n"
          "    <id>7</id>\n"
          "    <allow_unauthenticated_participants>true</allow_unauthenticated_participants>\n"
          "  </domain>\n"
          "</dds>\n", fp);
    fclose(fp);

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_parse_governance_xml(g_acc, GOV_XML_PATH, &cfg));
    TEST_ASSERT_EQUAL(1, (int)cfg.domain_count);
    TEST_ASSERT_EQUAL(7, (int)cfg.domains[0].domain_id);
    TEST_ASSERT_TRUE(cfg.domains[0].allow_unauthenticated_participants);
}

static void test_validate_permissions_signature(void)
{
    uint8_t xml[16], sig[16];
    memset(xml, 1, sizeof(xml));
    memset(sig, 2, sizeof(sig));

    /* NOTE 实际行为: 简化实现, 内容无关恒通过 */
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_validate_permissions_signature(g_acc, xml, 16, sig, 16));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_validate_permissions_signature(NULL, xml, 16, sig, 16));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_validate_permissions_signature(g_acc, NULL, 16, sig, 16));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_validate_permissions_signature(g_acc, xml, 16, NULL, 16));
}

/* ── 参与者权限加载/卸载 ───────────────────────────────────────────────── */

static void test_load_unload_participant_permissions(void)
{
    uint8_t xml[8] = {0};

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_load_participant_permissions(NULL, "CN=A", xml, 8));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_load_participant_permissions(g_acc, NULL, xml, 8));
    /* xml 数据可为 NULL (简化实现不解析) */
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_load_participant_permissions(g_acc, "CN=Alice", NULL, 0));
    TEST_ASSERT_EQUAL(1, (int)g_acc->active_subjects);

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_SUBJECT_NOT_FOUND,
                      (int)dds_access_unload_participant_permissions(g_acc, "CN=Nobody"));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_unload_participant_permissions(NULL, "CN=Alice"));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_unload_participant_permissions(g_acc, NULL));

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_unload_participant_permissions(g_acc, "CN=Alice"));
    TEST_ASSERT_EQUAL(0, (int)g_acc->active_subjects);
    /* 二次卸载 */
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_SUBJECT_NOT_FOUND,
                      (int)dds_access_unload_participant_permissions(g_acc, "CN=Alice"));
}

static void test_reload_same_subject_leaks_slot(void)
{
    uint8_t xml[8] = {0};
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_load_participant_permissions(g_acc, "CN=Bob", xml, 8));
    TEST_ASSERT_EQUAL(1, (int)g_acc->active_subjects);

    /* NOTE 产品缺陷: 对已加载 subject 重复 load 会分配新静态槽而旧槽
     * 不回收 (subject_name 非空永不复用), active_subjects 重复递增 */
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_load_participant_permissions(g_acc, "CN=Bob", xml, 8));
    TEST_ASSERT_EQUAL(2, (int)g_acc->active_subjects);
}

/* ── 访问决策 ──────────────────────────────────────────────────────────── */

static void test_check_permission_default_allow(void)
{
    dds_access_request_t req;
    memset(&req, 0, sizeof(req));
    req.subject_name = "CN=Stranger";
    req.topic_name = "AnyTopic";
    req.requested_action = DDS_PERM_WRITE;

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_DECISION_ALLOW, (int)dds_access_check_permission(g_acc, &req));
    TEST_ASSERT_EQUAL(1, (int)g_acc->access_checks);
    TEST_ASSERT_EQUAL(1, (int)g_acc->access_granted);
    TEST_ASSERT_EQUAL(0, (int)g_acc->access_denied);

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_DECISION_DENY, (int)dds_access_check_permission(NULL, &req));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_DECISION_DENY, (int)dds_access_check_permission(g_acc, NULL));
}

static void test_check_permission_default_deny(void)
{
    /* 白盒: 开启 strict 默认拒绝 */
    g_acc->default_policy.default_deny = true;

    dds_access_request_t req;
    memset(&req, 0, sizeof(req));
    req.subject_name = "CN=Stranger";
    req.topic_name = "AnyTopic";
    req.requested_action = DDS_PERM_WRITE;

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_DECISION_DENY, (int)dds_access_check_permission(g_acc, &req));
    TEST_ASSERT_EQUAL(1, (int)g_acc->access_denied);

    /* 已加载权限的 subject 不受 default_deny 影响 */
    uint8_t xml[8] = {0};
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_load_participant_permissions(g_acc, "CN=Known", xml, 8));
    dds_access_request_t req2;
    memset(&req2, 0, sizeof(req2));
    req2.subject_name = "CN=Known";
    req2.topic_name = "AnyTopic";
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_DECISION_ALLOW, (int)dds_access_check_permission(g_acc, &req2));
}

static int g_denied_cb_count;
static void on_denied_cb(const char *subject, const char *topic, dds_permission_action_t action)
{
    (void)subject; (void)topic; (void)action;
    g_denied_cb_count++;
}

static void test_check_permission_denied_callback(void)
{
    g_acc->on_access_denied = on_denied_cb;
    g_denied_cb_count = 0;
    g_acc->default_policy.default_deny = true;

    dds_access_request_t req;
    memset(&req, 0, sizeof(req));
    req.subject_name = "CN=X";
    req.topic_name = "T";
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_DECISION_DENY, (int)dds_access_check_permission(g_acc, &req));
    TEST_ASSERT_EQUAL(1, g_denied_cb_count);
    g_acc->on_access_denied = NULL;
}

static void test_check_join_permission(void)
{
    /* NOTE 实际行为: NULL subject 返回 ACCESS_DENIED 而非 INVALID_PARAM */
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_ACCESS_DENIED,
                      (int)dds_access_check_join_permission(g_acc, NULL, 0, DDS_SECURITY_ASIL_QM));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_ACCESS_DENIED,
                      (int)dds_access_check_join_permission(NULL, "CN=A", 0, DDS_SECURITY_ASIL_QM));
    /* 未加载权限 + 默认放行 */
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_check_join_permission(g_acc, "CN=Anyone", 0, DDS_SECURITY_ASIL_B));
}

static void test_check_publish_subscribe_read_write(void)
{
    dds_access_request_t req;
    memset(&req, 0, sizeof(req));
    req.subject_name = "CN=P";
    req.topic_name = "Topic";
    req.requested_action = DDS_PERM_PUBLISH;

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_check_publish(NULL, &req));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_check_publish(g_acc, NULL));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_check_subscribe(g_acc, NULL));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_check_write(NULL, &req));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_check_read(g_acc, NULL));

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK, (int)dds_access_check_publish(g_acc, &req));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK, (int)dds_access_check_subscribe(g_acc, &req));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK, (int)dds_access_check_read(g_acc, &req));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK, (int)dds_access_check_write(g_acc, &req));

    /* 默认拒绝时 publish/subscribe 全部走 DENY */
    g_acc->default_policy.default_deny = true;
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_ACCESS_DENIED, (int)dds_access_check_publish(g_acc, &req));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_ACCESS_DENIED, (int)dds_access_check_subscribe(g_acc, &req));
}

/* ── 主题通配符匹配 ────────────────────────────────────────────────────── */

static void test_match_topic_pattern(void)
{
    /* 精确 */
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("Sensor.Temp", "Sensor.Temp"));
    TEST_ASSERT_FALSE(dds_access_match_topic_pattern("Sensor.Temp", "Sensor.Press"));
    /* 末尾 * */
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("Sensor.*", "Sensor.Temp"));
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("Sensor.*", "Sensor."));
    TEST_ASSERT_FALSE(dds_access_match_topic_pattern("Sensor.*", "Actuator.Temp"));
    /* 开头 * */
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("*Temp", "SensorTemp"));
    TEST_ASSERT_FALSE(dds_access_match_topic_pattern("*Temp", "SensorTempX"));
    /* 中间 * */
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("A*C", "ABC"));
    TEST_ASSERT_FALSE(dds_access_match_topic_pattern("A*C", "ABCD"));
    /* 单字符 ? */
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("Sensor.?", "Sensor.A"));
    TEST_ASSERT_FALSE(dds_access_match_topic_pattern("Sensor.?", "Sensor.AB"));
    /* 纯 * */
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("*", "Anything"));
    TEST_ASSERT_TRUE(dds_access_match_topic_pattern("*", ""));
    /* NULL 守卫 */
    TEST_ASSERT_FALSE(dds_access_match_topic_pattern(NULL, "x"));
    TEST_ASSERT_FALSE(dds_access_match_topic_pattern("x", NULL));
    TEST_ASSERT_FALSE(dds_access_match_topic_pattern(NULL, NULL));
}

/* ── 辅助函数 / 统计 ───────────────────────────────────────────────────── */

static void test_get_subject_name(void)
{
    dds_security_cert_t cert;
    char name[64];
    memset(&cert, 0, sizeof(cert));
    strncpy(cert.subject_name, "CN=TestAlice,O=DDS", sizeof(cert.subject_name) - 1);

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK, (int)dds_access_get_subject_name(&cert, name, sizeof(name)));
    TEST_ASSERT_EQUAL_STRING("CN=TestAlice,O=DDS", name);

    /* 截断 */
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK, (int)dds_access_get_subject_name(&cert, name, 6));
    TEST_ASSERT_EQUAL_STRING("CN=Te", name);

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM, (int)dds_access_get_subject_name(NULL, name, 64));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM, (int)dds_access_get_subject_name(&cert, NULL, 64));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM, (int)dds_access_get_subject_name(&cert, name, 0));
}

static void test_clear_permission_cache(void)
{
    dds_access_clear_permission_cache(NULL); /* 安全 */
    dds_access_clear_permission_cache(g_acc);
    TEST_ASSERT_EQUAL('\0', g_acc->permission_cache.last_subject[0]);
    TEST_ASSERT_NULL(g_acc->permission_cache.cached_permission);
    TEST_ASSERT_EQUAL(0, (int)g_acc->permission_cache.cache_time);
}

static void test_get_stats(void)
{
    dds_access_request_t req;
    memset(&req, 0, sizeof(req));
    req.subject_name = "CN=S";
    req.topic_name = "T";
    dds_access_check_permission(g_acc, &req);

    uint64_t checks = 0, denied = 0, granted = 0;
    dds_access_get_stats(g_acc, &checks, &denied, &granted);
    TEST_ASSERT_EQUAL(1, (int)checks);
    TEST_ASSERT_EQUAL(1, (int)granted);
    TEST_ASSERT_EQUAL(0, (int)denied);

    dds_access_get_stats(NULL, &checks, &denied, &granted);
    dds_access_get_stats(g_acc, NULL, NULL, NULL);
}

static void test_check_permissions_expiry(void)
{
    TEST_ASSERT_EQUAL(0, (int)dds_access_check_permissions_expiry(NULL, 100U));

    uint8_t xml[8] = {0};
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_load_participant_permissions(g_acc, "CN=Exp", xml, 8));

    /* 加载时 valid_until 默认 0 → current_time > 0 即过期 */
    TEST_ASSERT_EQUAL(1, (int)dds_access_check_permissions_expiry(g_acc, 1U));

    /* 白盒: 设置远期有效期 → 不再过期 */
    g_acc->permissions_db[0].subjects[0].valid_until = 0xFFFFFFFFFFFFFFFFULL;
    TEST_ASSERT_EQUAL(0, (int)dds_access_check_permissions_expiry(g_acc, 1U));
}

/* ── 治理决策 ──────────────────────────────────────────────────────────── */

static void test_get_domain_governance_defaults(void)
{
    static dds_governance_config_t gov;
    memset(&gov, 0, sizeof(gov));

    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_get_domain_governance(NULL, 0, &gov));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_ERROR_INVALID_PARAM,
                      (int)dds_access_get_domain_governance(g_acc, 0, NULL));

    /* governance_file_path 未配置 → 默认 */
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_get_domain_governance(g_acc, 0, &gov));
    TEST_ASSERT_EQUAL(1, (int)gov.version_major);
    TEST_ASSERT_EQUAL(0, (int)gov.domain_count);
}

static void test_get_domain_governance_from_file(void)
{
    FILE *fp = fopen(GOV_XML_PATH, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("<dds>\n"
          "  <domain>\n"
          "    <id>3</id>\n"
          "    <allow_unauthenticated_participants>true</allow_unauthenticated_participants>\n"
          "  </domain>\n"
          "</dds>\n", fp);
    fclose(fp);

    strncpy(g_acc->governance_file_path, GOV_XML_PATH, sizeof(g_acc->governance_file_path) - 1);

    dds_governance_config_t *gov = &s_gov_cfg;
    memset(gov, 0, sizeof(*gov));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_get_domain_governance(g_acc, 3, gov));
    TEST_ASSERT_EQUAL(1, (int)gov->domain_count);
    TEST_ASSERT_EQUAL(3, (int)gov->domains[0].domain_id);

    /* 未配置的 domain → 默认值 (domain_count 0) */
    memset(gov, 0, sizeof(*gov));
    TEST_ASSERT_EQUAL((int)DDS_ACCESS_OK,
                      (int)dds_access_get_domain_governance(g_acc, 9, gov));
    TEST_ASSERT_EQUAL(0, (int)gov->domain_count);
}

static void test_protection_switches_default_off(void)
{
    /* 无治理配置 → 保护默认关闭 */
    TEST_ASSERT_FALSE(dds_access_discovery_protection_enabled(g_acc, 0, "Topic"));
    TEST_ASSERT_FALSE(dds_access_liveliness_protection_enabled(g_acc, 0, "Topic"));
    TEST_ASSERT_EQUAL(0, (int)dds_access_rtps_protection_kind(g_acc, 0));

    TEST_ASSERT_FALSE(dds_access_discovery_protection_enabled(NULL, 0, "Topic"));
    TEST_ASSERT_FALSE(dds_access_liveliness_protection_enabled(NULL, 0, "Topic"));
    TEST_ASSERT_EQUAL(0, (int)dds_access_rtps_protection_kind(NULL, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_null_config_returns_null);
    RUN_TEST(test_init_defaults);
    RUN_TEST(test_init_copies_paths);
    RUN_TEST(test_deinit_null_safe);
    RUN_TEST(test_parse_permissions_xml_guards);
    RUN_TEST(test_parse_permissions_xml_single_subject);
    RUN_TEST(test_parse_permissions_xml_multi_subject);
    RUN_TEST(test_parse_governance_xml);
    RUN_TEST(test_validate_permissions_signature);
    RUN_TEST(test_load_unload_participant_permissions);
    RUN_TEST(test_reload_same_subject_leaks_slot);
    RUN_TEST(test_check_permission_default_allow);
    RUN_TEST(test_check_permission_default_deny);
    RUN_TEST(test_check_permission_denied_callback);
    RUN_TEST(test_check_join_permission);
    RUN_TEST(test_check_publish_subscribe_read_write);
    RUN_TEST(test_match_topic_pattern);
    RUN_TEST(test_get_subject_name);
    RUN_TEST(test_clear_permission_cache);
    RUN_TEST(test_get_stats);
    RUN_TEST(test_check_permissions_expiry);
    RUN_TEST(test_get_domain_governance_defaults);
    RUN_TEST(test_get_domain_governance_from_file);
    RUN_TEST(test_protection_switches_default_off);
    return UNITY_END();
}
