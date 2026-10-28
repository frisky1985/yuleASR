/**
 * @file test_dds_security_manager.c
 * @brief DDS-Security 安全管理器 (dds_security_manager.c) 单元测试
 *
 * 链接真实产品源码 (4 个 security 模块整体编译), 断言真实行为:
 * - 初始化/反初始化与子模块协调
 * - 参与者注册/认证/数据保护往返
 * - 访问控制联动 / 事件与审计日志 / 重放防护
 *
 * NOTE: 头文件声明但未实现的 API (process_auth_message, verify_data_integrity,
 *       update_permissions, get_event_history, get_audit_logs, clear_audit_logs,
 *       reset_replay_protection, load/update/get_policy, refresh_certificates,
 *       reset_statistics) 不在测试范围, 引用将导致链接失败 (产品缺陷已记录)。
 */
#include "unity.h"
#include "dds_security_manager.h"
#include <string.h>
#include <stdio.h>

#define AUDIT_FILE_PATH "/tmp/ddssec_audit.log"

static dds_security_context_t *g_mgr;
static dds_security_config_t g_cfg;

static rtps_guid_t make_guid(uint8_t seed)
{
    rtps_guid_t g;
    memset(&g, 0, sizeof(g));
    for (int i = 0; i < 12; i++) { g.prefix[i] = (uint8_t)(seed * 8 + i); }
    g.entity_id[0] = (uint8_t)(seed + 0x40);
    return g;
}

void setUp(void)
{
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.enable_audit_log = true;
    g_cfg.max_audit_entries = 16;
    g_mgr = dds_security_manager_init(&g_cfg);
    TEST_ASSERT_NOT_NULL(g_mgr);
}

void tearDown(void)
{
    if (g_mgr) {
        dds_security_manager_deinit(g_mgr);
        g_mgr = NULL;
    }
    remove(AUDIT_FILE_PATH);
}

/* ── init / deinit ─────────────────────────────────────────────────────── */

static void test_init_null_config_returns_null(void)
{
    TEST_ASSERT_NULL(dds_security_manager_init(NULL));
}

static void test_init_state_and_submodules(void)
{
    TEST_ASSERT_EQUAL((int)DDS_SECMGR_STATE_READY, (int)g_mgr->state);
    TEST_ASSERT_TRUE(dds_security_manager_is_initialized(g_mgr));
    TEST_ASSERT_EQUAL((int)DDS_SECMGR_STATE_READY,
                      (int)dds_security_manager_get_state(g_mgr));
    TEST_ASSERT_NOT_NULL(g_mgr->auth_ctx);
    TEST_ASSERT_NOT_NULL(g_mgr->access_ctx);
    TEST_ASSERT_NOT_NULL(g_mgr->crypto_ctx);
    TEST_ASSERT_EQUAL((int)DDS_SECMGR_MAX_PARTICIPANTS, (int)g_mgr->max_participants);
    /* init 时写入 SYSTEM_STARTUP 审计 */
    TEST_ASSERT_EQUAL(1, (int)g_mgr->audit_mgr.entry_count);
}

static void test_get_state_null_safe(void)
{
    TEST_ASSERT_EQUAL((int)DDS_SECMGR_STATE_UNINITIALIZED,
                      (int)dds_security_manager_get_state(NULL));
    TEST_ASSERT_FALSE(dds_security_manager_is_initialized(NULL));
}

static void test_deinit(void)
{
    TEST_ASSERT_EQUAL((int)DDS_SECMGR_STATE_READY,
                      (int)dds_security_manager_get_state(g_mgr));
    dds_security_manager_deinit(g_mgr);
    TEST_ASSERT_EQUAL((int)DDS_SECMGR_STATE_UNINITIALIZED,
                      (int)dds_security_manager_get_state(g_mgr));
    TEST_ASSERT_NULL(g_mgr->auth_ctx);
    TEST_ASSERT_NULL(g_mgr->access_ctx);
    TEST_ASSERT_NULL(g_mgr->crypto_ctx);
    TEST_ASSERT_NULL(g_mgr->participants);
    TEST_ASSERT_FALSE(dds_security_manager_is_initialized(g_mgr));
    g_mgr = NULL; /* tearDown 不重复 deinit */
    dds_security_manager_deinit(NULL); /* 安全 */
}

/* ── 参与者注册 ────────────────────────────────────────────────────────── */

static void test_register_participant_guards(void)
{
    rtps_guid_t g = make_guid(1);
    rtps_guid_t zero;
    memset(&zero, 0, sizeof(zero));

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_register_participant(NULL, &g, "CN=A"));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_register_participant(g_mgr, NULL, "CN=A"));
    /* 全零 GUID 是空槽哨兵 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_register_participant(g_mgr, &zero, "CN=A"));

    dds_sec_participant_t *p = dds_security_find_participant(g_mgr, &g);
    TEST_ASSERT_NULL(p);
    TEST_ASSERT_NULL(dds_security_find_participant(NULL, &g));
    TEST_ASSERT_NULL(dds_security_find_participant(g_mgr, NULL));
}

static void test_register_find_unregister(void)
{
    rtps_guid_t g = make_guid(2);

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_participant(g_mgr, &g, "CN=TestAlice"));
    TEST_ASSERT_EQUAL(1, (int)g_mgr->participant_count);

    dds_sec_participant_t *p = dds_security_find_participant(g_mgr, &g);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_MEMORY(&g, &p->guid, sizeof(rtps_guid_t));
    TEST_ASSERT_EQUAL_STRING("CN=TestAlice", p->subject_name);
    TEST_ASSERT_EQUAL((int)DDS_SEC_PARTICIPANT_UNAUTHENTICATED, (int)p->state);
    TEST_ASSERT_TRUE(p->created_time > 0U);

    /* 重复注册 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_ALREADY_INITIALIZED,
                      (int)dds_security_register_participant(g_mgr, &g, "CN=Other"));

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_unregister_participant(NULL, &g));
    rtps_guid_t other = make_guid(99);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_unregister_participant(g_mgr, &other));

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_unregister_participant(g_mgr, &g));
    TEST_ASSERT_EQUAL(0, (int)g_mgr->participant_count);
    TEST_ASSERT_NULL(dds_security_find_participant(g_mgr, &g));
    /* 二次注销 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_unregister_participant(g_mgr, &g));
}

/* ── 认证流程 ──────────────────────────────────────────────────────────── */

static void test_begin_authentication(void)
{
    rtps_guid_t local = make_guid(3), remote = make_guid(4);
    rtps_guid_t unreg = make_guid(5);

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_begin_authentication(NULL, &local, &remote));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_begin_authentication(g_mgr, NULL, &remote));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_begin_authentication(g_mgr, &local, NULL));
    /* 本地未注册 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_begin_authentication(g_mgr, &unreg, &remote));

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_participant(g_mgr, &local, "CN=Local"));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_begin_authentication(g_mgr, &local, &remote));

    dds_sec_participant_t *p = dds_security_find_participant(g_mgr, &local);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL((int)DDS_SEC_PARTICIPANT_AUTHENTICATING, (int)p->state);
    TEST_ASSERT_NOT_NULL(p->handshake);
    TEST_ASSERT_EQUAL((int)DDS_HANDSHAKE_STATE_BEGIN, (int)p->handshake->state);
    TEST_ASSERT_EQUAL(1, (int)g_mgr->total_auth_attempts);
}

static void test_complete_authentication_failure(void)
{
    rtps_guid_t g = make_guid(6);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_complete_authentication(NULL, &g));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_complete_authentication(g_mgr, NULL));
    rtps_guid_t unreg = make_guid(7);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_complete_authentication(g_mgr, &unreg));

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_participant(g_mgr, &g, "CN=P"));
    /* identity_verified == false → 失败 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_AUTHENTICATION_FAILED,
                      (int)dds_security_complete_authentication(g_mgr, &g));
    TEST_ASSERT_EQUAL(1, (int)g_mgr->total_auth_failures);
    dds_sec_participant_t *p = dds_security_find_participant(g_mgr, &g);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL(1, (int)p->auth_failures);
    TEST_ASSERT_EQUAL((int)DDS_SEC_PARTICIPANT_UNAUTHENTICATED, (int)p->state);
}

static void test_complete_authentication_success(void)
{
    rtps_guid_t g = make_guid(8);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_participant(g_mgr, &g, "CN=OK"));
    dds_sec_participant_t *p = dds_security_find_participant(g_mgr, &g);
    TEST_ASSERT_NOT_NULL(p);

    /* 白盒: 标记身份已验证 (正常流程由握手完成) */
    p->identity_verified = true;
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_complete_authentication(g_mgr, &g));
    TEST_ASSERT_EQUAL(1, (int)g_mgr->total_auth_success);
    TEST_ASSERT_EQUAL((int)DDS_SEC_PARTICIPANT_SECURE_ESTABLISHED, (int)p->state);
    TEST_ASSERT_TRUE(dds_security_is_participant_authenticated(g_mgr, &g));
    /* NOTE: get_shared_secret 因 DH 缺陷得到全零密钥 (len 32), 会话仍创建成功 */
    TEST_ASSERT_TRUE(p->crypto_session_id != 0U);
    TEST_ASSERT_TRUE(p->auth_completed_time > 0U);

    TEST_ASSERT_FALSE(dds_security_is_participant_authenticated(NULL, &g));
    TEST_ASSERT_FALSE(dds_security_is_participant_authenticated(g_mgr, NULL));
    rtps_guid_t unreg = make_guid(9);
    TEST_ASSERT_FALSE(dds_security_is_participant_authenticated(g_mgr, &unreg));
}

/* ── 数据保护 ──────────────────────────────────────────────────────────── */

static void register_authenticated(uint8_t seed, dds_sec_participant_t **out)
{
    rtps_guid_t g = make_guid(seed);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_participant(g_mgr, &g, NULL));
    dds_sec_participant_t *p = dds_security_find_participant(g_mgr, &g);
    TEST_ASSERT_NOT_NULL(p);
    p->identity_verified = true;
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_complete_authentication(g_mgr, &g));
    if (out) { *out = p; }
}

static void test_protect_unprotect_roundtrip(void)
{
    dds_sec_participant_t *p = NULL;
    register_authenticated(10, &p);

    uint8_t pt[32], protected_buf[128], rt[64];
    for (int i = 0; i < 32; i++) { pt[i] = (uint8_t)(0xA0 + i); }
    uint32_t protected_len = sizeof(protected_buf);

    /* 未认证 / 未注册守卫 */
    rtps_guid_t unreg = make_guid(11);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_protect_data(NULL, &p->guid, "T", pt, 32,
                                                     protected_buf, &protected_len));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_protect_data(g_mgr, NULL, "T", pt, 32,
                                                     protected_buf, &protected_len));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_protect_data(g_mgr, &unreg, "T", pt, 32,
                                                     protected_buf, &protected_len));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_protect_data(g_mgr, &p->guid, "T", NULL, 32,
                                                     protected_buf, &protected_len));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_protect_data(g_mgr, &p->guid, "T", pt, 32,
                                                     protected_buf, NULL));

    rtps_guid_t g2 = make_guid(12);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_participant(g_mgr, &g2, NULL));
    /* 未完成认证 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_AUTHENTICATION_FAILED,
                      (int)dds_security_protect_data(g_mgr, &g2, "T", pt, 32,
                                                     protected_buf, &protected_len));

    /* 加密 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_protect_data(g_mgr, &p->guid, "Sensor.Temp",
                                                     pt, 32, protected_buf, &protected_len));
    TEST_ASSERT_EQUAL((int)dds_crypto_get_encrypted_size(32), (int)protected_len);
    TEST_ASSERT_EQUAL(1, (int)p->messages_encrypted);

    /* 解密往返 */
    uint32_t pt_len = sizeof(rt);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_unprotect_data(NULL, &p->guid, &p->guid,
                                                       protected_buf, protected_len, rt, &pt_len));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_unprotect_data(g_mgr, NULL, &p->guid,
                                                       protected_buf, protected_len, rt, &pt_len));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_unprotect_data(g_mgr, &p->guid, &p->guid,
                                                       protected_buf, protected_len, rt, &pt_len));
    TEST_ASSERT_EQUAL(32, (int)pt_len);
    TEST_ASSERT_EQUAL_MEMORY(pt, rt, 32);
    TEST_ASSERT_EQUAL(1, (int)p->messages_decrypted);

    /* NOTE 实际行为: unprotect 只调 detect_replay(只读), 不经
     * update_seq_window 记录序列号 → 重放窗口始终为空 (产品缺陷:
     * 解密成功后未记录 seq, 同一 seq 可无限重放) */
    TEST_ASSERT_EQUAL(0, (int)p->replay_window.valid_entries);
    TEST_ASSERT_EQUAL(0, (int)p->replay_window.window[0]);
    TEST_ASSERT_EQUAL(0, (int)p->last_seq_number);
}

static void test_unprotect_tamper_detected(void)
{
    dds_sec_participant_t *p = NULL;
    register_authenticated(13, &p);

    uint8_t pt[16], protected_buf[128], rt[64];
    memset(pt, 0xB0, sizeof(pt));
    uint32_t protected_len = sizeof(protected_buf);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_protect_data(g_mgr, &p->guid, "T", pt, 16,
                                                     protected_buf, &protected_len));

    /* 篡改密文 (header 25 字节后) */
    protected_buf[26] ^= 0x20U;
    uint32_t pt_len = sizeof(rt);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_DECRYPTION_FAILED,
                      (int)dds_security_unprotect_data(g_mgr, &p->guid, &p->guid,
                                                       protected_buf, protected_len, rt, &pt_len));
    TEST_ASSERT_EQUAL(1, (int)p->decrypt_failures);

    /* 密文缓冲过小 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_protect_data(g_mgr, &p->guid, "T", pt, 16,
                                                     protected_buf, &protected_len));
    pt_len = 5;
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_DECRYPTION_FAILED,
                      (int)dds_security_unprotect_data(g_mgr, &p->guid, &p->guid,
                                                       protected_buf, protected_len, rt, &pt_len));
}

static void test_protect_data_buffer_too_small(void)
{
    dds_sec_participant_t *p = NULL;
    register_authenticated(14, &p);

    uint8_t pt[32], small[20];
    memset(pt, 1, sizeof(pt));
    uint32_t protected_len = sizeof(small);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_ENCRYPTION_FAILED,
                      (int)dds_security_protect_data(g_mgr, &p->guid, "T", pt, 32,
                                                     small, &protected_len));
}

/* ── 访问控制联动 ──────────────────────────────────────────────────────── */

static int g_violation_cb_count;
static void on_violation_cb(const char *topic, dds_permission_action_t action)
{
    (void)topic; (void)action;
    g_violation_cb_count++;
}

static void test_check_access_skipped_when_disabled(void)
{
    /* policy_flags 无 ACCESS_CONTROL → 恒 OK */
    rtps_guid_t unreg = make_guid(15);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_check_access(g_mgr, &unreg, 0, "T", DDS_PERM_WRITE));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_check_access(NULL, &unreg, 0, "T", DDS_PERM_WRITE));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_check_access(g_mgr, NULL, 0, "T", DDS_PERM_WRITE));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_check_access(g_mgr, &unreg, 0, NULL, DDS_PERM_WRITE));
}

static void test_check_access_enforced(void)
{
    dds_security_manager_deinit(g_mgr);
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.policy_flags = (uint32_t)DDS_SEC_POLICY_ACCESS_CONTROL;
    g_cfg.on_access_violation = on_violation_cb;
    g_cfg.max_audit_entries = 16;
    g_mgr = dds_security_manager_init(&g_cfg);
    TEST_ASSERT_NOT_NULL(g_mgr);

    g_violation_cb_count = 0;
    rtps_guid_t g = make_guid(16);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_participant(g_mgr, &g, "CN=AccessP"));

    /* access 默认 default_deny=false → 允许 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_check_access(g_mgr, &g, 0, "Sensor", DDS_PERM_PUBLISH));
    TEST_ASSERT_EQUAL(0, g_violation_cb_count);
    TEST_ASSERT_EQUAL(0, (int)g_mgr->total_access_violations);

    /* 白盒: 打开 strict → 拒绝并回调 + 统计 */
    g_mgr->access_ctx->default_policy.default_deny = true;
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_ACCESS_DENIED,
                      (int)dds_security_check_access(g_mgr, &g, 0, "Sensor", DDS_PERM_PUBLISH));
    TEST_ASSERT_EQUAL(1, g_violation_cb_count);
    TEST_ASSERT_EQUAL(1, (int)g_mgr->total_access_violations);

    /* READ 走 check_subscribe 分支 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_ACCESS_DENIED,
                      (int)dds_security_check_access(g_mgr, &g, 0, "Sensor", DDS_PERM_READ));
    TEST_ASSERT_EQUAL(2, g_violation_cb_count);
}

/* ── 事件管理 ──────────────────────────────────────────────────────────── */

static dds_security_event_type_t g_last_event;
static int g_event_cb_count;
static void on_event_cb(dds_security_event_type_t event, dds_security_severity_t severity,
                        const rtps_guid_t *guid, const char *desc, void *user)
{
    (void)severity; (void)guid; (void)desc; (void)user;
    g_last_event = event;
    g_event_cb_count++;
}

static void test_register_event_callback(void)
{
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_register_event_callback(NULL, on_event_cb, NULL));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_register_event_callback(g_mgr, NULL, NULL));

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_event_callback(g_mgr, on_event_cb, NULL));
    TEST_ASSERT_EQUAL(on_event_cb, g_mgr->event_mgr.callback);

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_unregister_event_callback(g_mgr, on_event_cb));
    TEST_ASSERT_NULL(g_mgr->event_mgr.callback);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_unregister_event_callback(NULL, on_event_cb));
}

static void test_trigger_event(void)
{
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_trigger_event(NULL, DDS_SEC_EVT_CONFIG_CHANGED,
                                                      DDS_SEC_SEVERITY_INFO, NULL, "x"));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_trigger_event(g_mgr, DDS_SEC_EVT_CONFIG_CHANGED,
                                                      DDS_SEC_SEVERITY_INFO, NULL, NULL));

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_event_callback(g_mgr, on_event_cb, NULL));
    g_event_cb_count = 0;
    uint32_t count_before = g_mgr->event_mgr.event_count;
    uint32_t audit_before = g_mgr->audit_mgr.entry_count;

    /* INFO 事件: 进历史 + 回调, 不进审计 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_trigger_event(g_mgr, DDS_SEC_EVT_CONFIG_CHANGED,
                                                      DDS_SEC_SEVERITY_INFO, NULL,
                                                      "event %d", 42));
    TEST_ASSERT_EQUAL(count_before + 1, g_mgr->event_mgr.event_count);
    TEST_ASSERT_EQUAL(1, g_event_cb_count);
    TEST_ASSERT_EQUAL((int)DDS_SEC_EVT_CONFIG_CHANGED, (int)g_last_event);
    TEST_ASSERT_EQUAL(audit_before, g_mgr->audit_mgr.entry_count);

    /* ERROR 事件: 同时进审计 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_trigger_event(g_mgr, DDS_SEC_EVT_ENCRYPTION_ERROR,
                                                      DDS_SEC_SEVERITY_ERROR, NULL, "oops"));
    TEST_ASSERT_EQUAL(2, g_event_cb_count);
    TEST_ASSERT_EQUAL(audit_before + 1, g_mgr->audit_mgr.entry_count);
}

/* ── 审计日志 ──────────────────────────────────────────────────────────── */

static void test_log_audit(void)
{
    rtps_guid_t g = make_guid(17);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_log_audit(NULL, DDS_SEC_EVT_CONFIG_CHANGED,
                                                  DDS_SEC_SEVERITY_INFO, NULL, "m"));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_log_audit(g_mgr, DDS_SEC_EVT_CONFIG_CHANGED,
                                                  DDS_SEC_SEVERITY_INFO, NULL, NULL));

    uint32_t before = g_mgr->audit_mgr.entry_count;
    uint64_t logged_before = g_mgr->audit_mgr.total_logged;
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_log_audit(g_mgr, DDS_SEC_EVT_KEY_ROTATION,
                                                  DDS_SEC_SEVERITY_INFO, &g, "rotated"));
    TEST_ASSERT_EQUAL(before + 1, g_mgr->audit_mgr.entry_count);
    TEST_ASSERT_EQUAL(logged_before + 1, g_mgr->audit_mgr.total_logged);

    /* 关闭审计 → 不记录 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_configure_audit(g_mgr, false, NULL, DDS_SEC_SEVERITY_INFO));
    TEST_ASSERT_FALSE(g_mgr->config.enable_audit_log);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_log_audit(g_mgr, DDS_SEC_EVT_KEY_ROTATION,
                                                  DDS_SEC_SEVERITY_INFO, &g, "no-op"));
    TEST_ASSERT_EQUAL(before + 1, g_mgr->audit_mgr.entry_count);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_configure_audit(NULL, false, NULL, DDS_SEC_SEVERITY_INFO));
}

static void test_audit_file_logging(void)
{
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_configure_audit(g_mgr, true, AUDIT_FILE_PATH,
                                                        DDS_SEC_SEVERITY_INFO));
    TEST_ASSERT_TRUE(g_mgr->audit_mgr.file_logging_enabled);
    TEST_ASSERT_EQUAL_STRING(AUDIT_FILE_PATH, g_mgr->audit_mgr.log_file_path);

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_log_audit(g_mgr, DDS_SEC_EVT_CONFIG_CHANGED,
                                                  DDS_SEC_SEVERITY_WARNING, NULL, "file line"));
    FILE *fp = fopen(AUDIT_FILE_PATH, "r");
    TEST_ASSERT_NOT_NULL(fp);
    char line[256];
    TEST_ASSERT_NOT_NULL(fgets(line, sizeof(line), fp));
    TEST_ASSERT_TRUE(strstr(line, "file line") != NULL);
    fclose(fp);
}

static void test_audit_ring_wrap(void)
{
    /* max_audit_entries=16, 已有 1 条 startup; 再灌 20 条 → 环绕丢弃 */
    for (int i = 0; i < 20; i++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "entry-%d", i);
        TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                          (int)dds_security_log_audit(g_mgr, DDS_SEC_EVT_CONFIG_CHANGED,
                                                      DDS_SEC_SEVERITY_INFO, NULL, msg));
    }
    TEST_ASSERT_EQUAL(16, (int)g_mgr->audit_mgr.entry_count);
    TEST_ASSERT_EQUAL(21, (int)g_mgr->audit_mgr.total_logged);
    TEST_ASSERT_EQUAL(5, (int)g_mgr->audit_mgr.total_dropped);
    /* 环绕后 write_index 回绕 */
    TEST_ASSERT_EQUAL(5, (int)g_mgr->audit_mgr.write_index);
}

/* ── 重放防护 ──────────────────────────────────────────────────────────── */

static void test_detect_replay_disabled(void)
{
    /* policy 无 REPLAY_PROTECTION → 恒 OK */
    rtps_guid_t g = make_guid(18);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_detect_replay(g_mgr, &g, 7));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_detect_replay(NULL, &g, 7));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM,
                      (int)dds_security_detect_replay(g_mgr, NULL, 7));
    dds_security_update_seq_window(NULL, &g, 7);
    dds_security_update_seq_window(g_mgr, NULL, 7);
}

static void test_detect_replay_window(void)
{
    dds_security_manager_deinit(g_mgr);
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.policy_flags = (uint32_t)DDS_SEC_POLICY_REPLAY_PROTECTION;
    g_cfg.max_audit_entries = 16;
    g_mgr = dds_security_manager_init(&g_cfg);

    rtps_guid_t g = make_guid(19);
    /* 未注册参与者放行 (发现阶段) */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK, (int)dds_security_detect_replay(g_mgr, &g, 1));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_participant(g_mgr, &g, NULL));

    /* seq=0 跳过检测 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK, (int)dds_security_detect_replay(g_mgr, &g, 0));

    /* 记录 seq=1 → 精确重放被检测 */
    dds_security_update_seq_window(g_mgr, &g, 1);
    TEST_ASSERT_EQUAL(1, (int)dds_security_find_participant(g_mgr, &g)->last_seq_number);
    uint32_t evt_before = g_mgr->event_mgr.event_count;
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_REPLAY_DETECTED,
                      (int)dds_security_detect_replay(g_mgr, &g, 1));
    /* NOTE 产品缺陷: REPLAY_DETECTED 只触发事件, total_replay_detected
     * 统计字段从不累计 (恒 0) */
    TEST_ASSERT_EQUAL(evt_before + 1, g_mgr->event_mgr.event_count);

    /* 记录 seq=5; 更小的 seq=1 在窗口内但未记录 → 放行 (简化实现) */
    dds_security_update_seq_window(g_mgr, &g, 5);
    TEST_ASSERT_EQUAL(5, (int)dds_security_find_participant(g_mgr, &g)->last_seq_number);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK, (int)dds_security_detect_replay(g_mgr, &g, 1));
    /* 已记录的 5 重放 → 检测 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_REPLAY_DETECTED,
                      (int)dds_security_detect_replay(g_mgr, &g, 5));
}

/* ── 维护操作 ──────────────────────────────────────────────────────────── */

static void test_maintain(void)
{
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM, (int)dds_security_maintain(NULL, 0));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK, (int)dds_security_maintain(g_mgr, 1000U));

    dds_security_manager_deinit(g_mgr);
    g_mgr = NULL;
}

static void test_maintain_after_deinit(void)
{
    dds_security_context_t *ctx = g_mgr;
    dds_security_manager_deinit(g_mgr);
    g_mgr = NULL;
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_NOT_INITIALIZED,
                      (int)dds_security_maintain(ctx, 1000U));
    /* NOTE 产品缺陷: deinit 后 ctx->participants = NULL, 但
     * rotate_keys(ctx, NULL) / check_certificate_expiry() 会直接遍历
     * participants[0..63] → NULL 解引用段错误, 无法在测试中调用
     * (崩溃行为已记录, 不做断言)。 */
}

static void test_rotate_keys(void)
{
    dds_sec_participant_t *p = NULL;
    register_authenticated(20, &p);

    uint32_t rot_before = g_mgr->crypto_ctx->key_rotations;
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_INVALID_PARAM, (int)dds_security_rotate_keys(NULL, NULL));

    rtps_guid_t unreg = make_guid(21);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK, (int)dds_security_rotate_keys(g_mgr, &unreg));
    TEST_ASSERT_EQUAL((int)rot_before, (int)g_mgr->crypto_ctx->key_rotations);

    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK, (int)dds_security_rotate_keys(g_mgr, &p->guid));
    TEST_ASSERT_EQUAL((int)(rot_before + 1), (int)g_mgr->crypto_ctx->key_rotations);

    /* NULL = 全部参与者 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK, (int)dds_security_rotate_keys(g_mgr, NULL));
    TEST_ASSERT_EQUAL((int)(rot_before + 2), (int)g_mgr->crypto_ctx->key_rotations);
}

static void test_check_certificate_expiry(void)
{
    rtps_guid_t unreg = make_guid(22);
    /* UNAUTHENTICATED 状态的参与者不检查 */
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_participant(g_mgr, &unreg, NULL));
    TEST_ASSERT_EQUAL(0U, dds_security_check_certificate_expiry(g_mgr, 1U));

    /* 已认证参与者: 白盒使 certs[0] 有效期归零 → 过期 */
    dds_sec_participant_t *p = NULL;
    register_authenticated(23, &p);
    p->identity.cert_chain.certs[0].valid_from = 0;
    p->identity.cert_chain.certs[0].valid_until = 0;
    TEST_ASSERT_EQUAL(1U, dds_security_check_certificate_expiry(g_mgr, 1U));
    TEST_ASSERT_EQUAL(0U, dds_security_check_certificate_expiry(NULL, 1U));
}

/* ── 统计与字符串 ──────────────────────────────────────────────────────── */

static void test_get_statistics(void)
{
    uint64_t succ = 0, fail = 0, viol = 0, replay = 0;
    dds_security_get_statistics(NULL, &succ, &fail, &viol, &replay);
    dds_security_get_statistics(g_mgr, NULL, NULL, NULL, NULL);

    dds_security_get_statistics(g_mgr, &succ, &fail, &viol, &replay);
    TEST_ASSERT_EQUAL(0, (int)succ);
    TEST_ASSERT_EQUAL(0, (int)fail);

    rtps_guid_t g = make_guid(24);
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_OK,
                      (int)dds_security_register_participant(g_mgr, &g, "CN=S"));
    TEST_ASSERT_EQUAL((int)DDS_SECURITY_ERROR_AUTHENTICATION_FAILED,
                      (int)dds_security_complete_authentication(g_mgr, &g));

    dds_security_get_statistics(g_mgr, &succ, &fail, &viol, &replay);
    TEST_ASSERT_EQUAL(0, (int)succ);
    TEST_ASSERT_EQUAL(1, (int)fail);
}

static void test_state_and_error_strings(void)
{
    TEST_ASSERT_EQUAL_STRING("UNAUTHENTICATED",
                             dds_security_participant_state_string(DDS_SEC_PARTICIPANT_UNAUTHENTICATED));
    TEST_ASSERT_EQUAL_STRING("AUTHENTICATING",
                             dds_security_participant_state_string(DDS_SEC_PARTICIPANT_AUTHENTICATING));
    TEST_ASSERT_EQUAL_STRING("SECURE_ESTABLISHED",
                             dds_security_participant_state_string(DDS_SEC_PARTICIPANT_SECURE_ESTABLISHED));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", dds_security_participant_state_string((dds_sec_participant_state_t)99));

    TEST_ASSERT_EQUAL_STRING("OK", dds_security_error_string(DDS_SECURITY_OK));
    TEST_ASSERT_EQUAL_STRING("Invalid parameter",
                             dds_security_error_string(DDS_SECURITY_ERROR_INVALID_PARAM));
    TEST_ASSERT_EQUAL_STRING("Authentication failed",
                             dds_security_error_string(DDS_SECURITY_ERROR_AUTHENTICATION_FAILED));
    TEST_ASSERT_EQUAL_STRING("Replay attack detected",
                             dds_security_error_string(DDS_SECURITY_ERROR_REPLAY_DETECTED));
    TEST_ASSERT_EQUAL_STRING("Unknown error", dds_security_error_string((dds_security_status_t)999));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_null_config_returns_null);
    RUN_TEST(test_init_state_and_submodules);
    RUN_TEST(test_get_state_null_safe);
    RUN_TEST(test_deinit);
    RUN_TEST(test_register_participant_guards);
    RUN_TEST(test_register_find_unregister);
    RUN_TEST(test_begin_authentication);
    RUN_TEST(test_complete_authentication_failure);
    RUN_TEST(test_complete_authentication_success);
    RUN_TEST(test_protect_unprotect_roundtrip);
    RUN_TEST(test_unprotect_tamper_detected);
    RUN_TEST(test_protect_data_buffer_too_small);
    RUN_TEST(test_check_access_skipped_when_disabled);
    RUN_TEST(test_check_access_enforced);
    RUN_TEST(test_register_event_callback);
    RUN_TEST(test_trigger_event);
    RUN_TEST(test_log_audit);
    RUN_TEST(test_audit_file_logging);
    RUN_TEST(test_audit_ring_wrap);
    RUN_TEST(test_detect_replay_disabled);
    RUN_TEST(test_detect_replay_window);
    RUN_TEST(test_maintain);
    RUN_TEST(test_maintain_after_deinit);
    RUN_TEST(test_rotate_keys);
    RUN_TEST(test_check_certificate_expiry);
    RUN_TEST(test_get_statistics);
    RUN_TEST(test_state_and_error_strings);
    return UNITY_END();
}
