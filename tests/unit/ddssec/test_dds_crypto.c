/**
 * @file test_dds_crypto.c
 * @brief DDS-Security 加密传输模块 (dds_crypto.c) 单元测试
 *
 * 链接真实产品源码, 断言真实行为:
 * - AES-GCM (简化实现) 加解密往返 / GMAC 校验
 * - 会话管理与密钥派生 / 轮换 / 过期检查
 * - RTPS 数据封装加密往返 / 非法参数守卫
 */
#include "unity.h"
#include "dds_crypto.h"
#include <string.h>

static dds_crypto_context_t *g_ctx;
static dds_security_config_t g_cfg;

static rtps_guid_t make_guid(uint8_t seed)
{
    rtps_guid_t g;
    memset(&g, 0, sizeof(g));
    for (int i = 0; i < 12; i++) { g.prefix[i] = (uint8_t)(seed + i); }
    g.entity_id[0] = seed;
    return g;
}

void setUp(void)
{
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_ctx = dds_crypto_init(&g_cfg);
    TEST_ASSERT_NOT_NULL(g_ctx);
}

void tearDown(void)
{
    if (g_ctx) {
        dds_crypto_deinit(g_ctx);
        g_ctx = NULL;
    }
}

/* ── init / deinit ─────────────────────────────────────────────────────── */

static void test_init_null_config_returns_null(void)
{
    TEST_ASSERT_NULL(dds_crypto_init(NULL));
}

static void test_init_defaults(void)
{
    TEST_ASSERT_EQUAL(DDS_CRYPTO_MAX_SESSIONS, (int)g_ctx->max_sessions);
    /* config->key_update_interval_ms == 0 → 默认 10 分钟 */
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_DEFAULT_KEY_UPDATE_MS,
                      (int)g_ctx->key_update_interval_ms);
    TEST_ASSERT_EQUAL(DDS_CRYPTO_ALG_AES_256_GCM, (int)g_ctx->algorithm);
    TEST_ASSERT_NOT_NULL(g_ctx->sessions);
}

static void test_init_custom_interval(void)
{
    dds_security_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.key_update_interval_ms = 5000U;
    dds_crypto_context_t *ctx = dds_crypto_init(&cfg);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(5000, (int)ctx->key_update_interval_ms);
    dds_crypto_deinit(ctx);
}

static void test_deinit_null_safe(void)
{
    dds_crypto_deinit(NULL); /* 不崩溃即通过 */
}

static void test_deinit_clears_sessions(void)
{
    dds_crypto_deinit(g_ctx);
    /* 白盒: 静态表无需 free, 但指针应置空 */
    TEST_ASSERT_NULL(g_ctx->sessions);
    TEST_ASSERT_NULL(g_ctx->key_manager.key_sessions);
    g_ctx = NULL; /* tearDown 不再重复 deinit */
}

/* ── 会话管理 ──────────────────────────────────────────────────────────── */

static void test_create_session_param_guards(void)
{
    rtps_guid_t l = make_guid(1), r = make_guid(2);
    uint8_t secret[32];
    memset(secret, 0xAB, sizeof(secret));

    TEST_ASSERT_EQUAL(0, (int)dds_crypto_create_session(NULL, &l, &r, secret, 32));
    TEST_ASSERT_EQUAL(0, (int)dds_crypto_create_session(g_ctx, NULL, &r, secret, 32));
    TEST_ASSERT_EQUAL(0, (int)dds_crypto_create_session(g_ctx, &l, NULL, secret, 32));
    TEST_ASSERT_EQUAL(0, (int)dds_crypto_create_session(g_ctx, &l, &r, NULL, 32));
}

static void test_create_and_find_session(void)
{
    rtps_guid_t l = make_guid(1), r = make_guid(2);
    uint8_t secret[32];
    memset(secret, 0x5A, sizeof(secret));

    uint64_t id = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);
    TEST_ASSERT_TRUE(id != 0U);
    TEST_ASSERT_EQUAL(1, (int)g_ctx->active_sessions);

    dds_crypto_session_t *s = dds_crypto_find_session(g_ctx, &l, &r);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL((int)id, (int)s->session_id);
    TEST_ASSERT_TRUE(s->established);

    /* GUID 反向不匹配 */
    rtps_guid_t x = make_guid(9);
    TEST_ASSERT_NULL(dds_crypto_find_session(g_ctx, &l, &x));
    TEST_ASSERT_NULL(dds_crypto_find_session(g_ctx, &x, &r));
    TEST_ASSERT_NULL(dds_crypto_find_session(g_ctx, NULL, &r));
}

static void test_close_session(void)
{
    rtps_guid_t l = make_guid(3), r = make_guid(4);
    uint8_t secret[32];
    memset(secret, 0x11, sizeof(secret));

    uint64_t id = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);
    TEST_ASSERT_TRUE(id != 0U);

    dds_crypto_close_session(NULL, id);
    dds_crypto_close_session(g_ctx, 0U); /* 非法 id, 安全返回 */

    dds_crypto_close_session(g_ctx, id);
    TEST_ASSERT_EQUAL(0, (int)g_ctx->active_sessions);
    TEST_ASSERT_NULL(dds_crypto_find_session(g_ctx, &l, &r));

    /* 关闭后槽位可复用 */
    uint64_t id2 = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);
    TEST_ASSERT_TRUE(id2 != 0U);
    TEST_ASSERT_EQUAL(1, (int)g_ctx->active_sessions);
}

static void test_session_pool_exhaustion(void)
{
    rtps_guid_t l = make_guid(5), r = make_guid(6);
    uint8_t secret[32];
    memset(secret, 0x22, sizeof(secret));

    uint64_t ids[DDS_CRYPTO_MAX_SESSIONS];
    for (uint32_t i = 0; i < DDS_CRYPTO_MAX_SESSIONS; i++) {
        ids[i] = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);
        TEST_ASSERT_TRUE(ids[i] != 0U);
    }
    /* 表满 → 第 65 个失败 */
    TEST_ASSERT_EQUAL(0, (int)dds_crypto_create_session(g_ctx, &l, &r, secret, 32));

    dds_crypto_close_session(g_ctx, ids[0]);
    TEST_ASSERT_TRUE(dds_crypto_create_session(g_ctx, &l, &r, secret, 32) != 0U);
}

static void test_get_session_keys(void)
{
    rtps_guid_t l = make_guid(7), r = make_guid(8);
    uint8_t secret[32];
    memset(secret, 0x33, sizeof(secret));

    dds_crypto_key_material_msg_t km;
    memset(&km, 0, sizeof(km));

    /* 参数守卫 */
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_get_session_keys(NULL, 1, &km));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_get_session_keys(g_ctx, 0, &km));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_get_session_keys(g_ctx, 1, NULL));

    /* 未创建的会话 */
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_SESSION_INVALID,
                      (int)dds_crypto_get_session_keys(g_ctx, 999, &km));

    uint64_t id = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK, (int)dds_crypto_get_session_keys(g_ctx, id, &km));
    TEST_ASSERT_EQUAL(0, (int)km.key_seq_number);

    uint8_t zeros[32];
    memset(zeros, 0, sizeof(zeros));
    TEST_ASSERT_TRUE(memcmp(km.sender_key, zeros, 32) != 0);
}

/* ── 密钥管理 ──────────────────────────────────────────────────────────── */

static void test_generate_key(void)
{
    uint8_t key[32];
    memset(key, 0, sizeof(key));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_generate_key(NULL, key, 32));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_generate_key(g_ctx, NULL, 32));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_generate_key(g_ctx, key, 0));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK, (int)dds_crypto_generate_key(g_ctx, key, 32));
    uint8_t zeros[32];
    memset(zeros, 0, sizeof(zeros));
    TEST_ASSERT_TRUE(memcmp(key, zeros, 32) != 0);
}

static void test_derive_session_keys(void)
{
    uint8_t secret[32];
    memset(secret, 0x77, sizeof(secret));
    dds_crypto_key_material_msg_t km;
    memset(&km, 0, sizeof(km));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_derive_session_keys(NULL, secret, 32, &km));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_derive_session_keys(g_ctx, NULL, 32, &km));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_derive_session_keys(g_ctx, secret, 32, NULL));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_derive_session_keys(g_ctx, secret, 32, &km));
    /* 三段密钥材料均非全零 */
    uint8_t zeros[32];
    memset(zeros, 0, sizeof(zeros));
    TEST_ASSERT_TRUE(memcmp(km.sender_key, zeros, 32) != 0);
    TEST_ASSERT_TRUE(memcmp(km.receiver_key, zeros, 32) != 0);
    TEST_ASSERT_TRUE(memcmp(km.master_sender_key, zeros, 32) != 0);

    /* secret_len=0 也被接受 (派生自全零 PRK) — NOTE 实际行为 */
    dds_crypto_key_material_msg_t km0;
    memset(&km0, 0, sizeof(km0));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_derive_session_keys(g_ctx, secret, 0, &km0));
}

static void test_update_session_key(void)
{
    rtps_guid_t l = make_guid(10), r = make_guid(11);
    uint8_t secret[32];
    memset(secret, 0x44, sizeof(secret));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_update_session_key(NULL, 1));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_update_session_key(g_ctx, 0));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_SESSION_INVALID,
                      (int)dds_crypto_update_session_key(g_ctx, 4242));

    uint64_t id = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);
    uint32_t rot_before = (uint32_t)g_ctx->key_rotations;
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK, (int)dds_crypto_update_session_key(g_ctx, id));
    TEST_ASSERT_EQUAL((int)(rot_before + 1), (int)g_ctx->key_rotations);

    dds_crypto_session_t *s = dds_crypto_find_session(g_ctx, &l, &r);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL(1, (int)s->active_key_index);
    TEST_ASSERT_EQUAL(1, (int)s->key_seq_number);
}

static int g_rotation_cb_count;
static void on_rotation_cb(uint64_t session_id)
{
    (void)session_id;
    g_rotation_cb_count++;
}

static void test_update_session_key_callback(void)
{
    g_ctx->on_key_rotation = on_rotation_cb;
    g_rotation_cb_count = 0;

    rtps_guid_t l = make_guid(12), r = make_guid(13);
    uint8_t secret[32];
    memset(secret, 0x55, sizeof(secret));
    uint64_t id = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK, (int)dds_crypto_update_session_key(g_ctx, id));
    TEST_ASSERT_EQUAL(1, g_rotation_cb_count);
    g_ctx->on_key_rotation = NULL;
}

static void test_check_key_expiry(void)
{
    TEST_ASSERT_EQUAL(0, (int)dds_crypto_check_key_expiry(NULL, 1000));

    rtps_guid_t l = make_guid(14), r = make_guid(15);
    uint8_t secret[32];
    memset(secret, 0x66, sizeof(secret));
    uint64_t id = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);
    TEST_ASSERT_TRUE(id != 0U);

    /* NOTE 实际行为: create_session 未设置 km->creation_time (恒 0),
     * 故 current_time 超过 MAX_KEY_LIFETIME 即触发轮换 */
    TEST_ASSERT_EQUAL(0, (int)dds_crypto_check_key_expiry(g_ctx, 1000));
    TEST_ASSERT_EQUAL(1, (int)dds_crypto_check_key_expiry(
        g_ctx, (uint64_t)DDS_CRYPTO_MAX_KEY_LIFETIME_MS + 1U));
}

static void test_generate_iv(void)
{
    rtps_guid_t l = make_guid(16), r = make_guid(17);
    uint8_t secret[32];
    memset(secret, 0x88, sizeof(secret));
    uint64_t id = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);
    dds_crypto_session_t *s = dds_crypto_find_session(g_ctx, &l, &r);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL((int)id, (int)s->session_id);

    uint8_t iv1[DDS_CRYPTO_GCM_IV_SIZE], iv2[DDS_CRYPTO_GCM_IV_SIZE];
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_generate_iv(NULL, s, iv1));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_generate_iv(g_ctx, NULL, iv1));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_generate_iv(g_ctx, s, NULL));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK, (int)dds_crypto_generate_iv(g_ctx, s, iv1));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK, (int)dds_crypto_generate_iv(g_ctx, s, iv2));
    /* NOTE: IV = salt||seq, 连续生成的 IV 因 seq 递增而不同 */
    TEST_ASSERT_TRUE(memcmp(iv1, iv2, DDS_CRYPTO_GCM_IV_SIZE) != 0);
    TEST_ASSERT_EQUAL_MEMORY(iv1, iv2, 8); /* salt 部分一致 */
}

/* ── AES-GCM 加解密 ────────────────────────────────────────────────────── */

static void test_aes_gcm_roundtrip_256(void)
{
    uint8_t key[32], iv[12], pt[64], ct[64], rt[64], tag[16];
    memset(key, 0xA5, sizeof(key));
    memset(iv, 0x5A, sizeof(iv));
    for (uint32_t i = 0; i < sizeof(pt); i++) { pt[i] = (uint8_t)i; }

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, key, 32, iv, pt, 64,
                                                      NULL, 0, ct, tag));
    TEST_ASSERT_TRUE(memcmp(pt, ct, 64) != 0);

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_decrypt_aes_gcm(g_ctx, key, 32, iv, ct, 64,
                                                      NULL, 0, tag, rt));
    TEST_ASSERT_EQUAL_MEMORY(pt, rt, 64);
}

static void test_aes_gcm_roundtrip_128_and_partial_block(void)
{
    uint8_t key[16], iv[12], pt[21], ct[21], rt[21], tag[16];
    memset(key, 0x0F, sizeof(key));
    memset(iv, 0xF0, sizeof(iv));
    for (uint32_t i = 0; i < sizeof(pt); i++) { pt[i] = (uint8_t)(0x80 + i); }

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, key, 16, iv, pt, 21,
                                                      NULL, 0, ct, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_decrypt_aes_gcm(g_ctx, key, 16, iv, ct, 21,
                                                      NULL, 0, tag, rt));
    TEST_ASSERT_EQUAL_MEMORY(pt, rt, 21);
}

static void test_aes_gcm_with_aad(void)
{
    uint8_t key[32], iv[12], pt[32], ct[32], rt[32], tag[16], aad[8];
    memset(key, 0x01, sizeof(key));
    memset(iv, 0x02, sizeof(iv));
    memset(aad, 0x03, sizeof(aad));
    memset(pt, 0xEE, sizeof(pt));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, key, 32, iv, pt, 32,
                                                      aad, 8, ct, tag));
    /* NOTE 实际行为: 简化 GHASH 只把 AAD 长度(位数)混入 tag, AAD 内容不参与
     * → 同长度不同内容的 AAD 产生相同 tag (产品缺陷) */
    uint8_t tag2[16], ct2[32];
    uint8_t aad2[8];
    memset(aad2, 0x04, sizeof(aad2));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, key, 32, iv, pt, 32,
                                                      aad2, 8, ct2, tag2));
    TEST_ASSERT_EQUAL_MEMORY(tag, tag2, 16);
    /* CTR 密文与 AAD 无关 */
    TEST_ASSERT_EQUAL_MEMORY(ct, ct2, 32);

    /* AAD 长度不同 → 长度块不同 → tag 不同 */
    uint8_t tag3[16];
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, key, 32, iv, pt, 32,
                                                      aad, 7, ct2, tag3));
    TEST_ASSERT_TRUE(memcmp(tag, tag3, 16) != 0);

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_decrypt_aes_gcm(g_ctx, key, 32, iv, ct, 32,
                                                      aad, 8, tag, rt));
    TEST_ASSERT_EQUAL_MEMORY(pt, rt, 32);
}

static void test_aes_gcm_tamper_detection(void)
{
    uint8_t key[32], iv[12], pt[32], ct[32], rt[32], tag[16];
    memset(key, 0x0B, sizeof(key));
    memset(iv, 0x0C, sizeof(iv));
    memset(pt, 0x0D, sizeof(pt));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, key, 32, iv, pt, 32,
                                                      NULL, 0, ct, tag));

    /* 篡改密文 */
    ct[7] ^= 0x40U;
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_TAG_MISMATCH,
                      (int)dds_crypto_decrypt_aes_gcm(g_ctx, key, 32, iv, ct, 32,
                                                      NULL, 0, tag, rt));
    ct[7] ^= 0x40U;

    /* 篡改 tag */
    tag[0] ^= 0x80U;
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_TAG_MISMATCH,
                      (int)dds_crypto_decrypt_aes_gcm(g_ctx, key, 32, iv, ct, 32,
                                                      NULL, 0, tag, rt));
}

static void test_aes_gcm_param_guards(void)
{
    uint8_t key[32], badkey[15], iv[12], pt[16], ct[16], tag[16];
    memset(key, 1, sizeof(key));
    memset(badkey, 1, sizeof(badkey));
    memset(iv, 2, sizeof(iv));
    memset(pt, 3, sizeof(pt));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_encrypt_aes_gcm(NULL, key, 32, iv, pt, 16,
                                                      NULL, 0, ct, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, NULL, 32, iv, pt, 16,
                                                      NULL, 0, ct, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, key, 32, NULL, pt, 16,
                                                      NULL, 0, ct, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, key, 32, iv, NULL, 16,
                                                      NULL, 0, ct, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, key, 32, iv, pt, 16,
                                                      NULL, 0, NULL, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, key, 32, iv, pt, 16,
                                                      NULL, 0, ct, NULL));
    /* NOTE: 仅接受 16/24/32 字节密钥 */
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_KEY_INVALID,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, badkey, 15, iv, pt, 16,
                                                      NULL, 0, ct, tag));
    /* 解密 NULL 守卫 */
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_decrypt_aes_gcm(NULL, key, 32, iv, pt, 16,
                                                      NULL, 0, tag, ct));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_decrypt_aes_gcm(g_ctx, key, 32, iv, NULL, 16,
                                                      NULL, 0, tag, ct));
}

/* ── GMAC ──────────────────────────────────────────────────────────────── */

static void test_gmac_verify_roundtrip(void)
{
    uint8_t key[32], iv[12], msg[48], work[48], tag[16], aad[6];
    memset(key, 0x21, sizeof(key));
    memset(iv, 0x22, sizeof(iv));
    memset(aad, 0x23, sizeof(aad));
    for (uint32_t i = 0; i < sizeof(msg); i++) { msg[i] = (uint8_t)(0x30 + i); }

    /* NOTE 实际行为: compute_gmac 内部走 CTR 原地 XOR, 会改写传入的 msg
     * 缓冲; 校验时必须以原始消息副本输入 verify。 */
    memcpy(work, msg, sizeof(msg));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_compute_gmac(g_ctx, key, 32, iv, work, 48,
                                                   aad, 6, tag));

    memcpy(work, msg, sizeof(msg));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_verify_gmac(g_ctx, key, 32, iv, work, 48,
                                                  aad, 6, tag));

    /* 篡改消息 */
    memcpy(work, msg, sizeof(msg));
    work[3] ^= 0x10U;
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_TAG_MISMATCH,
                      (int)dds_crypto_verify_gmac(g_ctx, key, 32, iv, work, 48,
                                                  aad, 6, tag));

    /* 篡改 AAD 内容 → tag 不变 → 校验仍通过 (NOTE 产品缺陷: AAD 内容不参与
     * tag 计算, 仅 aad_len 参与; 改变 AAD 长度才会被检测) */
    uint8_t aad2[6];
    memcpy(aad2, aad, 6);
    aad2[0] ^= 0x01U;
    memcpy(work, msg, sizeof(msg));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_verify_gmac(g_ctx, key, 32, iv, work, 48,
                                                  aad2, 6, tag));
}

static void test_gmac_param_guards(void)
{
    uint8_t key[32], iv[12], msg[8], tag[16];
    memset(key, 1, sizeof(key));
    memset(iv, 2, sizeof(iv));
    memset(msg, 3, sizeof(msg));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_compute_gmac(NULL, key, 32, iv, msg, 8, NULL, 0, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_compute_gmac(g_ctx, NULL, 32, iv, msg, 8, NULL, 0, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_compute_gmac(g_ctx, key, 32, NULL, msg, 8, NULL, 0, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_compute_gmac(g_ctx, key, 32, iv, NULL, 8, NULL, 0, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_compute_gmac(g_ctx, key, 32, iv, msg, 8, NULL, 0, NULL));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_verify_gmac(NULL, key, 32, iv, msg, 8, NULL, 0, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_verify_gmac(g_ctx, key, 32, iv, NULL, 8, NULL, 0, tag));
}

/* ── RTPS 数据封装加密 ─────────────────────────────────────────────────── */

static void test_rtps_data_roundtrip(void)
{
    rtps_guid_t l = make_guid(20), r = make_guid(21);
    uint8_t secret[32];
    memset(secret, 0x99, sizeof(secret));
    uint64_t id = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);
    TEST_ASSERT_TRUE(id != 0U);

    uint8_t pt[32], out[128], rt[64];
    memset(pt, 0xC3, sizeof(pt));
    for (uint32_t i = 0; i < sizeof(pt); i++) { pt[i] = (uint8_t)(0xC0 + i); }

    uint32_t out_len = 0;
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_encrypt_rtps_data(g_ctx, id, pt, 32, out,
                                                        sizeof(out), &out_len));
    TEST_ASSERT_EQUAL((int)dds_crypto_get_encrypted_size(32), (int)out_len);

    uint32_t pt_len = 0;
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_decrypt_rtps_data(g_ctx, id, out, out_len,
                                                        rt, sizeof(rt), &pt_len));
    TEST_ASSERT_EQUAL(32, (int)pt_len);
    TEST_ASSERT_EQUAL_MEMORY(pt, rt, 32);
}

static void test_rtps_data_param_guards(void)
{
    uint8_t pt[16], out[128];
    uint32_t out_len = 0;

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_encrypt_rtps_data(NULL, 1, pt, 16, out,
                                                        sizeof(out), &out_len));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_encrypt_rtps_data(g_ctx, 0, pt, 16, out,
                                                        sizeof(out), &out_len));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_encrypt_rtps_data(g_ctx, 1, NULL, 16, out,
                                                        sizeof(out), &out_len));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_encrypt_rtps_data(g_ctx, 1, pt, 16, out,
                                                        sizeof(out), NULL));

    /* 缓冲过小 */
    rtps_guid_t l = make_guid(22), r = make_guid(23);
    uint8_t secret[32];
    memset(secret, 0xAA, sizeof(secret));
    uint64_t id = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_BUFFER_TOO_SMALL,
                      (int)dds_crypto_encrypt_rtps_data(g_ctx, id, pt, 16, out, 10, &out_len));

    /* 会话无效 */
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_SESSION_INVALID,
                      (int)dds_crypto_encrypt_rtps_data(g_ctx, 777, pt, 16, out,
                                                        sizeof(out), &out_len));

    /* 解密守卫 */
    uint32_t pt_len = 0;
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_decrypt_rtps_data(NULL, id, out, 64, pt, 16, &pt_len));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_decrypt_rtps_data(g_ctx, id, NULL, 64, pt, 16, &pt_len));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_decrypt_rtps_data(g_ctx, id, out, 64, NULL, 16, &pt_len));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_decrypt_rtps_data(g_ctx, id, out, 64, pt, 16, NULL));
    /* NOTE: header(25)+tag(16)=41, 短于该长度的输入视为非法 */
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_decrypt_rtps_data(g_ctx, id, out, 30, pt, 16, &pt_len));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_SESSION_INVALID,
                      (int)dds_crypto_decrypt_rtps_data(g_ctx, 777, out, 64, pt, 16, &pt_len));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_BUFFER_TOO_SMALL,
                      (int)dds_crypto_decrypt_rtps_data(g_ctx, id, out, 64, pt, 5, &pt_len));
}

static void test_encrypted_size_helpers(void)
{
    TEST_ASSERT_EQUAL((int)(sizeof(dds_crypto_transformation_header_t) + 100U +
                            DDS_CRYPTO_GCM_TAG_SIZE),
                      (int)dds_crypto_get_encrypted_size(100));
    TEST_ASSERT_EQUAL(0, (int)dds_crypto_get_decrypted_size(10));
    TEST_ASSERT_EQUAL(0, (int)dds_crypto_get_decrypted_size(41));
    TEST_ASSERT_EQUAL(32, (int)dds_crypto_get_decrypted_size(41 + 32));
}

/* ── 消息 MAC / 变换头 ─────────────────────────────────────────────────── */

static void test_compute_message_mac(void)
{
    rtps_guid_t l = make_guid(24), r = make_guid(25);
    uint8_t secret[32];
    memset(secret, 0xBB, sizeof(secret));
    uint64_t id = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);

    uint8_t msg[24], hdr[8], mac[16];
    memset(msg, 0x4D, sizeof(msg));
    memset(hdr, 0x48, sizeof(hdr));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_compute_message_mac(NULL, id, msg, 24, hdr, 8, mac));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_compute_message_mac(g_ctx, 0, msg, 24, hdr, 8, mac));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_compute_message_mac(g_ctx, id, NULL, 24, hdr, 8, mac));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_compute_message_mac(g_ctx, id, msg, 24, hdr, 8, NULL));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_SESSION_INVALID,
                      (int)dds_crypto_compute_message_mac(g_ctx, 888, msg, 24, hdr, 8, mac));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_compute_message_mac(g_ctx, id, msg, 24, hdr, 8, mac));
    uint8_t zeros[16];
    memset(zeros, 0, sizeof(zeros));
    TEST_ASSERT_TRUE(memcmp(mac, zeros, 16) != 0);
}

static void test_create_transform_header(void)
{
    rtps_guid_t l = make_guid(26), r = make_guid(27);
    uint8_t secret[32];
    memset(secret, 0xCC, sizeof(secret));
    uint64_t id = dds_crypto_create_session(g_ctx, &l, &r, secret, 32);
    dds_crypto_session_t *s = dds_crypto_find_session(g_ctx, &l, &r);
    TEST_ASSERT_NOT_NULL(s);

    dds_crypto_transformation_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_create_transform_header(NULL, s, 2, &hdr));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_create_transform_header(g_ctx, NULL, 2, &hdr));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_create_transform_header(g_ctx, s, 2, NULL));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_create_transform_header(g_ctx, s,
                                                              DDS_CRYPTO_TRANSFORM_AES256_GCM,
                                                              &hdr));
    TEST_ASSERT_EQUAL(DDS_CRYPTO_TRANSFORM_AES256_GCM, (int)hdr.transformation_kind);
    /* session_id 大端编码 */
    TEST_ASSERT_EQUAL((uint8_t)(id >> 56), hdr.session_id[0]);
    TEST_ASSERT_EQUAL((uint8_t)(id & 0xFFU), hdr.session_id[7]);
}

/* ── 重放防护 (简化实现) / 统计 ────────────────────────────────────────── */

static void test_replay_check_simplified(void)
{
    /* NOTE 实际行为: 当前实现是简化版, 除参数守卫外恒返回 OK */
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_check_replay(NULL, 1, 1));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_ERROR_INVALID_PARAM,
                      (int)dds_crypto_check_replay(g_ctx, 0, 1));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK, (int)dds_crypto_check_replay(g_ctx, 1, 1));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK, (int)dds_crypto_check_replay(g_ctx, 1, 1));

    dds_crypto_update_replay_window(NULL, 1, 1);
    dds_crypto_update_replay_window(g_ctx, 0, 1);
    dds_crypto_update_replay_window(g_ctx, 1, 1);
}

static void test_get_stats(void)
{
    uint64_t enc = 0, dec = 0, fail = 0;
    dds_crypto_get_stats(NULL, &enc, &dec, &fail);

    uint8_t key[32], iv[12], pt[16], ct[16], rt[16], tag[16];
    memset(key, 0x0E, sizeof(key));
    memset(iv, 0x0F, sizeof(iv));
    memset(pt, 0x1E, sizeof(pt));

    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_encrypt_aes_gcm(g_ctx, key, 32, iv, pt, 16,
                                                      NULL, 0, ct, tag));
    TEST_ASSERT_EQUAL((int)DDS_CRYPTO_OK,
                      (int)dds_crypto_decrypt_aes_gcm(g_ctx, key, 32, iv, ct, 16,
                                                      NULL, 0, tag, rt));

    dds_crypto_get_stats(g_ctx, &enc, &dec, &fail);
    /* NOTE 实际行为: decrypt 内部复用 encrypt 重算 CTR, total_encrypted
     * 被重复累计 (16 + 16 = 32), total_decrypted = 16 */
    TEST_ASSERT_EQUAL(32, (int)enc);
    TEST_ASSERT_EQUAL(16, (int)dec);

    /* NULL 输出指针安全 */
    dds_crypto_get_stats(g_ctx, NULL, NULL, NULL);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_null_config_returns_null);
    RUN_TEST(test_init_defaults);
    RUN_TEST(test_init_custom_interval);
    RUN_TEST(test_deinit_null_safe);
    RUN_TEST(test_deinit_clears_sessions);
    RUN_TEST(test_create_session_param_guards);
    RUN_TEST(test_create_and_find_session);
    RUN_TEST(test_close_session);
    RUN_TEST(test_session_pool_exhaustion);
    RUN_TEST(test_get_session_keys);
    RUN_TEST(test_generate_key);
    RUN_TEST(test_derive_session_keys);
    RUN_TEST(test_update_session_key);
    RUN_TEST(test_update_session_key_callback);
    RUN_TEST(test_check_key_expiry);
    RUN_TEST(test_generate_iv);
    RUN_TEST(test_aes_gcm_roundtrip_256);
    RUN_TEST(test_aes_gcm_roundtrip_128_and_partial_block);
    RUN_TEST(test_aes_gcm_with_aad);
    RUN_TEST(test_aes_gcm_tamper_detection);
    RUN_TEST(test_aes_gcm_param_guards);
    RUN_TEST(test_gmac_verify_roundtrip);
    RUN_TEST(test_gmac_param_guards);
    RUN_TEST(test_rtps_data_roundtrip);
    RUN_TEST(test_rtps_data_param_guards);
    RUN_TEST(test_encrypted_size_helpers);
    RUN_TEST(test_compute_message_mac);
    RUN_TEST(test_create_transform_header);
    RUN_TEST(test_replay_check_simplified);
    RUN_TEST(test_get_stats);
    return UNITY_END();
}
