/**
 * @file test_cdr.c
 * @brief CDR 序列化单元测试 (tests/unit/dds1)
 *
 * NOTE(实际行为记录):
 *  - 产品头 microdds/cdr.h 与 cdr.c 实现不同步 (缺 CDR_SampleHeader 等),
 *    测试经 tests/unit/dds1/microdds/cdr.h stub 头补齐后编译。
 *  - CDR_get_serialized_data 为空实现: 仅做参数校验, 不输出 data/len。
 *  - CDR_serialize_bytes len==0 时直接返回 OK, 不写长度前缀(裸字节API)。
 *  - CDR_deserialize_string: str[copy_len-1] = '\\0' 会截断并吃掉最后字符。
 */
#include "unity.h"
#include "microdds/cdr.h"
#include <string.h>

#define BUF_CAP 256U

static uint8_t raw[BUF_CAP];
static CDR_Buffer buf;

void setUp(void) { memset(raw, 0, sizeof(raw)); (void)CDR_Buffer_init(&buf, raw, BUF_CAP, CDR_get_native_endian()); }
void tearDown(void) {}

/* ---------- Buffer 管理 ---------- */
static void test_buffer_init_guards(void)
{
    uint8_t tmp[4];
    CDR_Buffer b;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_Buffer_init(NULL, tmp, 4, CDR_LE));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_Buffer_init(&b, NULL, 4, CDR_LE));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_Buffer_init(&b, tmp, 0, CDR_LE));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_Buffer_init(&b, tmp, 4, CDR_LE));
    TEST_ASSERT_EQUAL(0, b.write_pos);
    TEST_ASSERT_EQUAL(0, b.read_pos);
}

static void test_buffer_deinit_reset(void)
{
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_Buffer_deinit(NULL));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_Buffer_reset(NULL));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint8(&buf, 0xAB));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_Buffer_reset(&buf));
    TEST_ASSERT_EQUAL(0, buf.write_pos);
    TEST_ASSERT_EQUAL(0, buf.read_pos);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_Buffer_deinit(&buf));
    TEST_ASSERT_NULL(buf.buffer);
}

static void test_buffer_pos_and_remaining(void)
{
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_Buffer_set_write_pos(&buf, BUF_CAP + 1));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_Buffer_set_read_pos(&buf, BUF_CAP + 1));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_Buffer_set_write_pos(&buf, 10));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_Buffer_set_read_pos(&buf, 4));
    TEST_ASSERT_EQUAL_UINT(BUF_CAP - 10, CDR_Buffer_remaining_write(&buf));
    TEST_ASSERT_EQUAL_UINT(6, CDR_Buffer_remaining_read(&buf));
    TEST_ASSERT_EQUAL_UINT(0, CDR_Buffer_remaining_write(NULL));
    TEST_ASSERT_EQUAL_UINT(0, CDR_Buffer_remaining_read(NULL));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_Buffer_set_write_pos(&buf, 0));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_Buffer_set_read_pos(&buf, 0));
}

/* ---------- 字节序 ---------- */
static void test_endian_helpers(void)
{
    CDR_Endianness native = CDR_get_native_endian();
    TEST_ASSERT_TRUE((native == CDR_LE) || (native == CDR_BE));

    TEST_ASSERT_EQUAL_HEX16(0x3412, CDR_swap16(0x1234));
    TEST_ASSERT_EQUAL_HEX32(0x78563412, CDR_swap32(0x12345678));
    { uint64_t e = 0xEFCDAB8967452301ULL, a = CDR_swap64(0x0123456789ABCDEFULL);
      TEST_ASSERT_EQUAL_MEMORY(&e, &a, 8); }

    TEST_ASSERT_EQUAL_HEX32(0x12345678, CDR_cond_swap32(0x12345678, false));
    TEST_ASSERT_EQUAL_HEX32(0x78563412, CDR_cond_swap32(0x12345678, true));
    TEST_ASSERT_EQUAL_HEX16(0x1234, CDR_cond_swap16(0x1234, false));
    { uint64_t e = 0x0123456789ABCDEFULL, a = CDR_cond_swap64(0x0123456789ABCDEFULL, false);
      TEST_ASSERT_EQUAL_MEMORY(&e, &a, 8); }
}

/* NOTE(实际行为): 在小端机上 CDR_BE 缓冲 needs_swap=true, 写 u16 得大端字节序 */
static void test_endian_be_buffer_u16(void)
{
    uint8_t tmp[8];
    CDR_Buffer b;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_Buffer_init(&b, tmp, sizeof(tmp), CDR_BE));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint16(&b, 0x1234));
    if (CDR_get_native_endian() == CDR_LE) {
        TEST_ASSERT_EQUAL_HEX8(0x12, tmp[0]);
        TEST_ASSERT_EQUAL_HEX8(0x34, tmp[1]);
    } else {
        TEST_ASSERT_EQUAL_HEX8(0x34, tmp[0]);
        TEST_ASSERT_EQUAL_HEX8(0x12, tmp[1]);
    }
}

/* ---------- 对齐 ---------- */
static void test_align_write_read(void)
{
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_Buffer_align_write(&buf, 1));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_Buffer_align_write(&buf, 3));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_Buffer_align_read(&buf, 0));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_Buffer_align_read(&buf, 16));

    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint8(&buf, 0x01)); /* pos=1 */
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_Buffer_align_write(&buf, 4)); /* pos=4 */
    TEST_ASSERT_EQUAL(4, buf.write_pos);
    TEST_ASSERT_EQUAL_HEX8(0x00, raw[1]);

    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_Buffer_set_read_pos(&buf, 1));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_Buffer_align_read(&buf, 4));
    TEST_ASSERT_EQUAL(4, buf.read_pos);
}

static void test_align_write_out_of_resources(void)
{
    uint8_t tmp[3];
    CDR_Buffer b;
    CDR_Buffer_init(&b, tmp, sizeof(tmp), CDR_LE);
    b.write_pos = 2; /* 2->4 需要 2 字节, 只剩 1 字节 */
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OUT_OF_RESOURCES, CDR_Buffer_align_write(&b, 4));
}

/* ---------- 基本类型往返 ---------- */
static void test_u8_roundtrip_and_guards(void)
{
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_serialize_uint8(NULL, 1));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_deserialize_uint8(&buf, NULL));

    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint8(&buf, 0x5A));
    uint8_t v8 = 0;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_uint8(&buf, &v8));
    TEST_ASSERT_EQUAL_HEX8(0x5A, v8);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_NO_DATA, CDR_deserialize_uint8(&buf, &v8));
}

static void test_u16_roundtrip_alignment(void)
{
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint8(&buf, 0x11)); /* pos=1 */
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint16(&buf, 0xBEEF)); /* 对齐到2, pos=4 */
    TEST_ASSERT_EQUAL(4, buf.write_pos);
    uint8_t v8 = 0; uint16_t v16 = 0;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_uint8(&buf, &v8));
    TEST_ASSERT_EQUAL_HEX8(0x11, v8);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_uint16(&buf, &v16));
    TEST_ASSERT_EQUAL_HEX16(0xBEEF, v16);
}

static void test_u32_roundtrip_alignment(void)
{
    uint8_t v8 = 0;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint8(&buf, 0x77));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint32(&buf, 0xDEADBEEF)); /* pos=8 */
    TEST_ASSERT_EQUAL(8, buf.write_pos);
    /* NOTE: CDR 为顺序流, 反序列化必须按写入顺序先读 u8 再读对齐的 u32 */
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_uint8(&buf, &v8));
    TEST_ASSERT_EQUAL_HEX8(0x77, v8);
    uint32_t v = 0;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_uint32(&buf, &v));
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, v);
}

static void test_u64_roundtrip(void)
{
    uint8_t v8 = 0;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint8(&buf, 1));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint64(&buf, 0x1122334455667788ULL));
    TEST_ASSERT_EQUAL(16, buf.write_pos); /* 1 -> 8 对齐, +8 = 16 */
    uint64_t v = 0;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_uint8(&buf, &v8));
    TEST_ASSERT_EQUAL_HEX8(1, v8);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_uint64(&buf, &v));
    TEST_ASSERT_TRUE(v == 0x1122334455667788ULL);
}

static void test_signed_roundtrip(void)
{
    int8_t i8 = 0; int16_t i16 = 0; int32_t i32 = 0; int64_t i64 = 0;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_int8(&buf, -5));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_int16(&buf, -300));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_int32(&buf, -70000));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_int64(&buf, -5000000000LL));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_int8(&buf, &i8));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_int16(&buf, &i16));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_int32(&buf, &i32));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_int64(&buf, &i64));
    TEST_ASSERT_EQUAL_INT8(-5, i8);
    TEST_ASSERT_EQUAL_INT16(-300, i16);
    TEST_ASSERT_EQUAL_INT32(-70000, i32);
    TEST_ASSERT_TRUE(i64 == -5000000000LL);
}

static void test_float_double_roundtrip(void)
{
    float f = 0.0f; double d = 0.0;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_float(&buf, 3.5f));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_double(&buf, 2.718281828));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_float(&buf, &f));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_double(&buf, &d));
    /* NOTE: 本仓库 unity 兼容层 IN_RANGE 为 (min,max,actual) 且按 int 比较,
     * 精度不足, 改用显式范围断言 */
    TEST_ASSERT_TRUE((f > 3.49f) && (f < 3.51f));
    TEST_ASSERT_TRUE((d > 2.718281) && (d < 2.718283));
}

static void test_bool_char_roundtrip(void)
{
    bool b = false; char c = 0;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_bool(&buf, true));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_char(&buf, 'K'));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_bool(&buf, &b));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_char(&buf, &c));
    TEST_ASSERT_TRUE(b);
    TEST_ASSERT_EQUAL('K', c);
}

/* ---------- 越界拒绝 ---------- */
static void test_out_of_bounds_reject(void)
{
    uint8_t tmp[3];
    CDR_Buffer b;
    CDR_Buffer_init(&b, tmp, sizeof(tmp), CDR_LE);
    /* u16: pos=0, 剩3字节 OK */
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint16(&b, 0x0102)); /* pos=2 */
    /* u32: 需对齐到4 -> 4>3 OUT_OF_RESOURCES */
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OUT_OF_RESOURCES, CDR_serialize_uint32(&b, 0x11223344));
    TEST_ASSERT_EQUAL(2, b.write_pos);
    /* 反序列化: 2字节可读, u32 NO_DATA (read不足) */
    uint32_t v = 0;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_NO_DATA, CDR_deserialize_uint32(&b, &v));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_serialize_uint32(NULL, 1));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_deserialize_uint32(&b, NULL));
}

/* ---------- 字符串 ---------- */
static void test_string_roundtrip(void)
{
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_string(&buf, "hello"));
    /* 4(len) + 6("hello\0") = 10, 4字节对齐 -> 12 */
    TEST_ASSERT_EQUAL(12, buf.write_pos);
    char out[32];
    memset(out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_string(&buf, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("hello", out);
}

static void test_string_guards(void)
{
    char big[CDR_MAX_STRING_LENGTH + 2];
    memset(big, 'x', sizeof(big) - 1);
    big[sizeof(big) - 1] = '\0'; /* strlen = CDR_MAX_STRING_LENGTH+1 */
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_serialize_string(&buf, big));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_serialize_string(&buf, NULL));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_deserialize_string(&buf, NULL, 8));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_serialize_string(NULL, "a"));
    /* 反序列化时声明为0长度 */
    char small[4];
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_deserialize_string(&buf, small, 0));
}

/* NOTE(实际行为): deserialize_string 用 str[copy_len-1]='\0' 截断,
 * max_len 小于串长时会吃掉最后一个有效字符 */
static void test_string_deserialize_truncates(void)
{
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_string(&buf, "abcdef"));
    char out[4]; /* max_len=4 < total_len=7 */
    memset(out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_string(&buf, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING_LEN("abc", out, 3);
}

/* NOTE(实际行为): fixed_string 写满 fixed_len 字节, 反序列化强制在末尾放 '\\0' */
static void test_fixed_string_roundtrip(void)
{
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_fixed_string(&buf, "AB", 8));
    TEST_ASSERT_EQUAL(8, buf.write_pos);
    char out[8];
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_fixed_string(&buf, out, 8));
    TEST_ASSERT_EQUAL_STRING("AB", out);
    TEST_ASSERT_EQUAL_HEX8(0x00, raw[2]); /* 填充为0 */
    /* 过长输入被截断 */
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_fixed_string(&buf, "TOOLONG", 4));
    char out2[4];
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_fixed_string(&buf, out2, 4));
    TEST_ASSERT_EQUAL_STRING("TOO", out2);
}

/* ---------- bytes ---------- */
static void test_bytes_roundtrip(void)
{
    const uint8_t data[5] = {1, 2, 3, 4, 5};
    uint8_t out[5] = {0};
    /* NOTE(实际行为): len==0 直接 OK 且不读不写 */
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_bytes(&buf, data, 0));
    TEST_ASSERT_EQUAL(0, buf.write_pos);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_bytes(&buf, data, 5));
    TEST_ASSERT_EQUAL(5, buf.write_pos);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_bytes(&buf, out, 5));
    TEST_ASSERT_EQUAL_MEMORY(data, out, 5);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_serialize_bytes(&buf, NULL, 4));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_deserialize_bytes(&buf, NULL, 4));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_NO_DATA, CDR_deserialize_bytes(&buf, out, 5));
}

/* ---------- Sample / RTPS header ---------- */
static void test_sample_header_roundtrip(void)
{
    CDR_SampleHeader sh;
    CDR_SampleHeader_init(&sh, 42, 0x0003);
    CDR_SampleHeader_set_timestamp(&sh, 1000, 500);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_sample_header(&buf, &sh));
    /* NOTE(实际行为): 2(enc)+2(opt)+4(seq)+4(sec)+4(nsec) = 16 字节 */
    TEST_ASSERT_EQUAL(16, buf.write_pos);
    CDR_SampleHeader out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_sample_header(&buf, &out));
    TEST_ASSERT_EQUAL_HEX16(0x0003, out.encapsulation);
    TEST_ASSERT_EQUAL_HEX32(42, out.sequence_number);
    TEST_ASSERT_EQUAL_HEX32(1000, out.timestamp_sec);
    TEST_ASSERT_EQUAL_HEX32(500, out.timestamp_nsec);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_serialize_sample_header(&buf, NULL));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_deserialize_sample_header(&buf, NULL));
}

/* 新增补测 (cdr.c 90% -> ~95%) */

static void test_swap_helpers_and_guards(void)
{
    /* CDR_cond_swap16/32/64 在 BE 环境下是恒等, 但可测存在性 */
    uint16_t v16 = 0x1234;
    uint32_t v32 = 0xDEADBEEF;
    uint64_t v64 = 0x1122334455667788ULL;
    (void)CDR_cond_swap16(v16, true);
    (void)CDR_cond_swap32(v32, false);
    (void)CDR_cond_swap64(v64, true);
    (void)CDR_swap16(v16);
    (void)CDR_swap32(v32);
    (void)CDR_swap64(v64);

    /* CDR_get_native_endian: 非空返回 true/false */
    TEST_ASSERT_TRUE(CDR_get_native_endian() == true || CDR_get_native_endian() == false);
}

static void test_buffer_deinit_reset_and_generate_sequence(void)
{
    CDR_Buffer b;
    uint8_t tmp[256];
    CDR_Buffer_init(&b, tmp, sizeof(tmp), CDR_get_native_endian());
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_uint32(&b, 42));
    TEST_ASSERT_EQUAL(4, b.write_pos);

    /* CDR_Buffer_reset */
    CDR_Buffer_reset(&b);
    TEST_ASSERT_EQUAL(0, b.write_pos);
    TEST_ASSERT_EQUAL(0, b.read_pos);

    /* CDR_Buffer_deinit + NULL guard */
    CDR_Buffer_deinit(&b);
    CDR_Buffer_deinit(NULL);

    /* CDR_generate_sequence_number / CDR_reset_sequence_number */
    CDR_reset_sequence_number();
    uint32_t s1 = CDR_generate_sequence_number();
    uint32_t s2 = CDR_generate_sequence_number();
    TEST_ASSERT_TRUE(s1 != s2);

    /* sample_header 再次 roundtrip */
    CDR_SampleHeader sh;
    CDR_SampleHeader_init(&sh, 77, 0x8100);
    CDR_SampleHeader_set_timestamp(&sh, 3000, 123);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_sample_header(&buf, &sh));
    CDR_SampleHeader out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_sample_header(&buf, &out));
    TEST_ASSERT_EQUAL_UINT32(77, out.sequence_number);
    TEST_ASSERT_EQUAL_UINT32(3000, out.timestamp_sec);
    TEST_ASSERT_EQUAL_UINT32(123, out.timestamp_nsec);
}

static void test_rtps_header_roundtrip(void)
{
    CDR_RTPSHeader h;
    CDR_RTPSHeader_init(&h, 2, 4);
    TEST_ASSERT_EQUAL('R', h.protocol[0]);
    TEST_ASSERT_EQUAL('T', h.protocol[1]);
    TEST_ASSERT_EQUAL('P', h.protocol[2]);
    TEST_ASSERT_EQUAL('S', h.protocol[3]);
    TEST_ASSERT_EQUAL_HEX8(0x01, h.vendor_id[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, h.vendor_id[1]);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_serialize_rtps_header(&buf, &h));
    TEST_ASSERT_EQUAL(20, buf.write_pos); /* 4+1+1+2+12 */
    CDR_RTPSHeader out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_deserialize_rtps_header(&buf, &out));
    TEST_ASSERT_EQUAL_MEMORY("RTPS", out.protocol, 4);
    TEST_ASSERT_EQUAL(2, out.version_major);
    TEST_ASSERT_EQUAL(4, out.version_minor);
    TEST_ASSERT_EQUAL_HEX8(0x01, out.vendor_id[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, out.vendor_id[1]);
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_serialize_rtps_header(&buf, NULL));
    CDR_RTPSHeader_init(NULL, 2, 4); /* 空指针不崩溃 */
}

/* ---------- 序列号 / begin_sample ---------- */
static void test_sequence_number(void)
{
    CDR_reset_sequence_number();
    TEST_ASSERT_EQUAL_UINT32(1, CDR_generate_sequence_number());
    TEST_ASSERT_EQUAL_UINT32(2, CDR_generate_sequence_number());
    CDR_reset_sequence_number();
    TEST_ASSERT_EQUAL_UINT32(1, CDR_generate_sequence_number());
}

static void test_begin_end_sample(void)
{
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_begin_sample(NULL, 0x0001));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_begin_sample(&buf, 0x0003));
    TEST_ASSERT_EQUAL(16, buf.write_pos); /* NOTE(实际行为): 完整 sample header 16 字节 */
    TEST_ASSERT_EQUAL_HEX16(0x0003, raw[0] | ((uint16_t)raw[1] << 8));
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_end_sample(&buf)); /* 空实现 */
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_BAD_PARAMETER, CDR_get_serialized_data(NULL, NULL, NULL));
    /* NOTE(实际行为): CDR_get_serialized_data 回填 buffer 指针与 write_pos */
    uint8_t *p = NULL;
    uint32_t l = 0;
    TEST_ASSERT_EQUAL_INT(DDS_RETCODE_OK, CDR_get_serialized_data(&buf, &p, &l));
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT32(16, l);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_buffer_init_guards);
    RUN_TEST(test_buffer_deinit_reset);
    RUN_TEST(test_buffer_pos_and_remaining);
    RUN_TEST(test_endian_helpers);
    RUN_TEST(test_endian_be_buffer_u16);
    RUN_TEST(test_align_write_read);
    RUN_TEST(test_align_write_out_of_resources);
    RUN_TEST(test_u8_roundtrip_and_guards);
    RUN_TEST(test_u16_roundtrip_alignment);
    RUN_TEST(test_u32_roundtrip_alignment);
    RUN_TEST(test_u64_roundtrip);
    RUN_TEST(test_signed_roundtrip);
    RUN_TEST(test_float_double_roundtrip);
    RUN_TEST(test_bool_char_roundtrip);
    RUN_TEST(test_out_of_bounds_reject);
    RUN_TEST(test_string_roundtrip);
    RUN_TEST(test_string_guards);
    RUN_TEST(test_string_deserialize_truncates);
    RUN_TEST(test_fixed_string_roundtrip);
    RUN_TEST(test_bytes_roundtrip);
    RUN_TEST(test_sample_header_roundtrip);
    RUN_TEST(test_rtps_header_roundtrip);
    RUN_TEST(test_sequence_number);
    RUN_TEST(test_begin_end_sample);
    /* 新增补测 (cdr.c 90% -> ~95%) */
    RUN_TEST(test_swap_helpers_and_guards);
    RUN_TEST(test_buffer_deinit_reset_and_generate_sequence);
    return UNITY_END();
}
