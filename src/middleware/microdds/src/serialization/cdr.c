/** @file cdr.c
 * @brief CDR (Common Data Representation) 序列化/反序列化实现
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 遵循MISRA C:2012规范
 */
/* @req SHALL_MICRODDS */


/* ============================================================================
 * 包含文件
 * ============================================================================ */
#include "microdds/cdr.h"
#include <string.h>
#include <limits.h>

/* ============================================================================
 * 内部常量
 * ============================================================================ */
/** @brief RTPS协议标识 */
static const char RTPS_PROTOCOL_ID[4] = {'R', 'T', 'P', 'S'};

/** @brief 予乐科技厂商ID */
static const uint8_t VENDOR_ID[2] = {0x01U, 0xFFU};

/* ============================================================================
 * 内部变量
 * ============================================================================ */
/** @brief 序列号计数器（静态变量，线程不安全） */
static uint32_t sequence_counter = 1U;

/* ============================================================================
 * 字节序检测与转换
 * ============================================================================ */
CDR_Endianness CDR_get_native_endian(void) {
    uint16_t test_val = 0x0102U;
    uint8_t test_bytes[2U];

    (void)memcpy(test_bytes, &test_val, sizeof(test_bytes));

    return (test_bytes[0] == 0x01U) ? CDR_ENDIAN_BIG : CDR_ENDIAN_LITTLE;
}

uint16_t CDR_swap16(uint16_t value) {
    return (uint16_t)(((value & 0x00FFU) << 8) |
                      ((value & 0xFF00U) >> 8));
}

uint32_t CDR_swap32(uint32_t value) {
    return ((value & 0x000000FFUL) << 24) |
           ((value & 0x0000FF00UL) << 8) |
           ((value & 0x00FF0000UL) >> 8) |
           ((value & 0xFF000000UL) >> 24);
}

uint64_t CDR_swap64(uint64_t value) {
    return ((value & 0x00000000000000FFULL) << 56) |
           ((value & 0x000000000000FF00ULL) << 40) |
           ((value & 0x0000000000FF0000ULL) << 24) |
           ((value & 0x00000000FF000000ULL) << 8) |
           ((value & 0x000000FF00000000ULL) >> 8) |
           ((value & 0x0000FF0000000000ULL) >> 24) |
           ((value & 0x00FF000000000000ULL) >> 40) |
           ((value & 0xFF00000000000000ULL) >> 56);
}

uint16_t CDR_cond_swap16(uint16_t value, bool needs_swap) {
    return needs_swap ? CDR_swap16(value) : value;
}

uint32_t CDR_cond_swap32(uint32_t value, bool needs_swap) {
    return needs_swap ? CDR_swap32(value) : value;
}

uint64_t CDR_cond_swap64(uint64_t value, bool needs_swap) {
    return needs_swap ? CDR_swap64(value) : value;
}

/* ============================================================================
 * 缓冲区管理实现
 * ============================================================================ */
DDS_ReturnCode_t CDR_Buffer_init(CDR_Buffer *buffer, uint8_t *data, uint32_t size, CDR_Endianness endian) {
    if ((buffer == NULL_PTR) || (data == NULL_PTR) || (size == 0U)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    buffer->buffer = data;
    buffer->size = size;
    buffer->write_pos = 0U;
    buffer->read_pos = 0U;
    buffer->endian = endian;
    buffer->needs_swap = (endian != CDR_get_native_endian());
    buffer->owns_buffer = false;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_Buffer_deinit(CDR_Buffer *buffer) {
    if (buffer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    buffer->buffer = NULL_PTR;
    buffer->size = 0U;
    buffer->write_pos = 0U;
    buffer->read_pos = 0U;
    buffer->owns_buffer = false;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_Buffer_reset(CDR_Buffer *buffer) {
    if (buffer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    buffer->write_pos = 0U;
    buffer->read_pos = 0U;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_Buffer_set_write_pos(CDR_Buffer *buffer, uint32_t offset) {
    if (buffer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    if (offset > buffer->size) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    buffer->write_pos = offset;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_Buffer_set_read_pos(CDR_Buffer *buffer, uint32_t offset) {
    if (buffer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    if (offset > buffer->size) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    buffer->read_pos = offset;

    return DDS_RETCODE_OK;
}

uint32_t CDR_Buffer_remaining_write(const CDR_Buffer *buffer) {
    if (buffer == NULL_PTR) {
        return 0U;
    }

    return (buffer->size > buffer->write_pos) ? (buffer->size - buffer->write_pos) : 0U;
}

uint32_t CDR_Buffer_remaining_read(const CDR_Buffer *buffer) {
    if (buffer == NULL_PTR) {
        return 0U;
    }

    return (buffer->write_pos > buffer->read_pos) ? (buffer->write_pos - buffer->read_pos) : 0U;
}

DDS_ReturnCode_t CDR_Buffer_align_write(CDR_Buffer *buffer, uint8_t alignment) {
    if (buffer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    if ((alignment != 2U) && (alignment != 4U) && (alignment != 8U)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    uint32_t mask = (uint32_t)(alignment - 1U);
    uint32_t padding = ((buffer->write_pos + mask) & ~mask) - buffer->write_pos;

    if ((buffer->write_pos + padding) > buffer->size) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }

    for (uint32_t i = 0U; i < padding; i++) {
        buffer->buffer[buffer->write_pos + i] = 0U;
    }
    buffer->write_pos += padding;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_Buffer_align_read(CDR_Buffer *buffer, uint8_t alignment) {
    if (buffer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    if ((alignment != 2U) && (alignment != 4U) && (alignment != 8U)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    uint32_t mask = (uint32_t)(alignment - 1U);
    uint32_t padding = ((buffer->read_pos + mask) & ~mask) - buffer->read_pos;

    if ((buffer->read_pos + padding) > buffer->write_pos) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }

    buffer->read_pos += padding;

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * 8位整数序列化
 * ============================================================================ */
DDS_ReturnCode_t CDR_serialize_int8(CDR_Buffer *buffer, int8_t value) {
    return CDR_serialize_uint8(buffer, (uint8_t)value);
}

DDS_ReturnCode_t CDR_deserialize_int8(CDR_Buffer *buffer, int8_t *value) {
    uint8_t u8_val;
    DDS_ReturnCode_t ret = CDR_deserialize_uint8(buffer, &u8_val);
    if (DDS_RETCODE_IS_OK(ret)) {
        *value = (int8_t)u8_val;
    }
    return ret;
}

DDS_ReturnCode_t CDR_serialize_uint8(CDR_Buffer *buffer, uint8_t value) {
    if (buffer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    if (buffer->write_pos >= buffer->size) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }

    buffer->buffer[buffer->write_pos] = value;
    buffer->write_pos++;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_deserialize_uint8(CDR_Buffer *buffer, uint8_t *value) {
    if ((buffer == NULL_PTR) || (value == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    if (buffer->read_pos >= buffer->write_pos) {
        return DDS_RETCODE_NO_DATA;
    }

    *value = buffer->buffer[buffer->read_pos];
    buffer->read_pos++;

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * 16位整数序列化
 * ============================================================================ */
DDS_ReturnCode_t CDR_serialize_int16(CDR_Buffer *buffer, int16_t value) {
    return CDR_serialize_uint16(buffer, (uint16_t)value);
}

DDS_ReturnCode_t CDR_deserialize_int16(CDR_Buffer *buffer, int16_t *value) {
    uint16_t u16_val;
    DDS_ReturnCode_t ret = CDR_deserialize_uint16(buffer, &u16_val);
    if (DDS_RETCODE_IS_OK(ret)) {
        *value = (int16_t)u16_val;
    }
    return ret;
}

DDS_ReturnCode_t CDR_serialize_uint16(CDR_Buffer *buffer, uint16_t value) {
    DDS_ReturnCode_t ret;

    if (buffer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    ret = CDR_Buffer_align_write(buffer, 2U);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    if ((buffer->write_pos + 2U) > buffer->size) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }

    uint16_t val = CDR_cond_swap16(value, buffer->needs_swap);
    buffer->buffer[buffer->write_pos] = (uint8_t)(val & 0xFFU);
    buffer->buffer[buffer->write_pos + 1U] = (uint8_t)((val >> 8) & 0xFFU);
    buffer->write_pos += 2U;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_deserialize_uint16(CDR_Buffer *buffer, uint16_t *value) {
    DDS_ReturnCode_t ret;

    if ((buffer == NULL_PTR) || (value == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    ret = CDR_Buffer_align_read(buffer, 2U);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    if ((buffer->read_pos + 2U) > buffer->write_pos) {
        return DDS_RETCODE_NO_DATA;
    }

    uint16_t val = (uint16_t)buffer->buffer[buffer->read_pos] |
                   ((uint16_t)buffer->buffer[buffer->read_pos + 1U] << 8);
    *value = CDR_cond_swap16(val, buffer->needs_swap);
    buffer->read_pos += 2U;

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * 32位整数序列化
 * ============================================================================ */
DDS_ReturnCode_t CDR_serialize_int32(CDR_Buffer *buffer, int32_t value) {
    return CDR_serialize_uint32(buffer, (uint32_t)value);
}

DDS_ReturnCode_t CDR_deserialize_int32(CDR_Buffer *buffer, int32_t *value) {
    uint32_t u32_val;
    DDS_ReturnCode_t ret = CDR_deserialize_uint32(buffer, &u32_val);
    if (DDS_RETCODE_IS_OK(ret)) {
        *value = (int32_t)u32_val;
    }
    return ret;
}

DDS_ReturnCode_t CDR_serialize_uint32(CDR_Buffer *buffer, uint32_t value) {
    DDS_ReturnCode_t ret;

    if (buffer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    ret = CDR_Buffer_align_write(buffer, 4U);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    if ((buffer->write_pos + 4U) > buffer->size) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }

    uint32_t val = CDR_cond_swap32(value, buffer->needs_swap);
    buffer->buffer[buffer->write_pos] = (uint8_t)(val & 0xFFU);
    buffer->buffer[buffer->write_pos + 1U] = (uint8_t)((val >> 8) & 0xFFU);
    buffer->buffer[buffer->write_pos + 2U] = (uint8_t)((val >> 16) & 0xFFU);
    buffer->buffer[buffer->write_pos + 3U] = (uint8_t)((val >> 24) & 0xFFU);
    buffer->write_pos += 4U;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_deserialize_uint32(CDR_Buffer *buffer, uint32_t *value) {
    DDS_ReturnCode_t ret;

    if ((buffer == NULL_PTR) || (value == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    ret = CDR_Buffer_align_read(buffer, 4U);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    if ((buffer->read_pos + 4U) > buffer->write_pos) {
        return DDS_RETCODE_NO_DATA;
    }

    uint32_t val = (uint32_t)buffer->buffer[buffer->read_pos] |
                   ((uint32_t)buffer->buffer[buffer->read_pos + 1U] << 8) |
                   ((uint32_t)buffer->buffer[buffer->read_pos + 2U] << 16) |
                   ((uint32_t)buffer->buffer[buffer->read_pos + 3U] << 24);
    *value = CDR_cond_swap32(val, buffer->needs_swap);
    buffer->read_pos += 4U;

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * 64位整数序列化
 * ============================================================================ */
DDS_ReturnCode_t CDR_serialize_int64(CDR_Buffer *buffer, int64_t value) {
    return CDR_serialize_uint64(buffer, (uint64_t)value);
}

DDS_ReturnCode_t CDR_deserialize_int64(CDR_Buffer *buffer, int64_t *value) {
    uint64_t u64_val;
    DDS_ReturnCode_t ret = CDR_deserialize_uint64(buffer, &u64_val);
    if (DDS_RETCODE_IS_OK(ret)) {
        *value = (int64_t)u64_val;
    }
    return ret;
}

DDS_ReturnCode_t CDR_serialize_uint64(CDR_Buffer *buffer, uint64_t value) {
    DDS_ReturnCode_t ret;

    if (buffer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    ret = CDR_Buffer_align_write(buffer, 8U);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    if ((buffer->write_pos + 8U) > buffer->size) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }

    uint64_t val = CDR_cond_swap64(value, buffer->needs_swap);
    for (uint32_t i = 0U; i < 8U; i++) {
        buffer->buffer[buffer->write_pos + i] = (uint8_t)((val >> (i * 8U)) & 0xFFULL);
    }
    buffer->write_pos += 8U;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_deserialize_uint64(CDR_Buffer *buffer, uint64_t *value) {
    DDS_ReturnCode_t ret;

    if ((buffer == NULL_PTR) || (value == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    ret = CDR_Buffer_align_read(buffer, 8U);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    if ((buffer->read_pos + 8U) > buffer->write_pos) {
        return DDS_RETCODE_NO_DATA;
    }

    uint64_t val = 0ULL;
    for (uint32_t i = 0U; i < 8U; i++) {
        val |= ((uint64_t)buffer->buffer[buffer->read_pos + i] << (i * 8U));
    }
    *value = CDR_cond_swap64(val, buffer->needs_swap);
    buffer->read_pos += 8U;

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * 浮点数序列化
 * ============================================================================ */
DDS_ReturnCode_t CDR_serialize_float(CDR_Buffer *buffer, float value) {
    uint32_t u;

    (void)memcpy(&u, &value, sizeof(u));
    return CDR_serialize_uint32(buffer, u);
}

DDS_ReturnCode_t CDR_deserialize_float(CDR_Buffer *buffer, float *value) {
    uint32_t u;

    DDS_ReturnCode_t ret = CDR_deserialize_uint32(buffer, &u);
    if (DDS_RETCODE_IS_OK(ret)) {
        (void)memcpy(value, &u, sizeof(u));
    }
    return ret;
}

DDS_ReturnCode_t CDR_serialize_double(CDR_Buffer *buffer, double value) {
    uint64_t u;

    (void)memcpy(&u, &value, sizeof(u));
    return CDR_serialize_uint64(buffer, u);
}

DDS_ReturnCode_t CDR_deserialize_double(CDR_Buffer *buffer, double *value) {
    uint64_t u;

    DDS_ReturnCode_t ret = CDR_deserialize_uint64(buffer, &u);
    if (DDS_RETCODE_IS_OK(ret)) {
        (void)memcpy(value, &u, sizeof(u));
    }
    return ret;
}

/* ============================================================================
 * 布尔和字符序列化
 * ============================================================================ */
DDS_ReturnCode_t CDR_serialize_bool(CDR_Buffer *buffer, bool value) {
    uint8_t byte_val = value ? 1U : 0U;
    return CDR_serialize_uint8(buffer, byte_val);
}

DDS_ReturnCode_t CDR_deserialize_bool(CDR_Buffer *buffer, bool *value) {
    uint8_t byte_val;
    DDS_ReturnCode_t ret = CDR_deserialize_uint8(buffer, &byte_val);
    if (DDS_RETCODE_IS_OK(ret)) {
        *value = (byte_val != 0U);
    }
    return ret;
}

DDS_ReturnCode_t CDR_serialize_char(CDR_Buffer *buffer, char value) {
    return CDR_serialize_uint8(buffer, (uint8_t)value);
}

DDS_ReturnCode_t CDR_deserialize_char(CDR_Buffer *buffer, char *value) {
    uint8_t byte_val;
    DDS_ReturnCode_t ret = CDR_deserialize_uint8(buffer, &byte_val);
    if (DDS_RETCODE_IS_OK(ret)) {
        *value = (char)byte_val;
    }
    return ret;
}

/* ============================================================================
 * 字符串序列化
 * ============================================================================ */
DDS_ReturnCode_t CDR_serialize_string(CDR_Buffer *buffer, const char *str) {
    if ((buffer == NULL_PTR) || (str == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    uint32_t len = (uint32_t)strlen(str);
    if (len > CDR_MAX_STRING_LENGTH) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* CDR字符串包含null终止符，所以长度+1 */
    uint32_t total_len = len + 1U;

    DDS_ReturnCode_t ret = CDR_serialize_uint32(buffer, total_len);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    if ((buffer->write_pos + total_len) > buffer->size) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }

    for (uint32_t i = 0U; i < total_len; i++) {
        buffer->buffer[buffer->write_pos + i] = (uint8_t)str[i];
    }
    buffer->write_pos += total_len;

    /* 4字节对齐 */
    ret = CDR_Buffer_align_write(buffer, 4U);

    return ret;
}

DDS_ReturnCode_t CDR_deserialize_string(CDR_Buffer *buffer, char *str, uint32_t max_len) {
    if ((buffer == NULL_PTR) || (str == NULL_PTR) || (max_len == 0U)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    uint32_t total_len;
    DDS_ReturnCode_t ret = CDR_deserialize_uint32(buffer, &total_len);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    if (total_len > CDR_MAX_STRING_LENGTH) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    if ((buffer->read_pos + total_len) > buffer->write_pos) {
        return DDS_RETCODE_NO_DATA;
    }

    uint32_t copy_len = total_len;
    if (copy_len > max_len) {
        copy_len = max_len;
    }

    for (uint32_t i = 0U; i < copy_len; i++) {
        str[i] = (char)buffer->buffer[buffer->read_pos + i];
    }

    /* 确保null终止 */
    if (copy_len > 0U) {
        str[copy_len - 1U] = '\0';
    }

    buffer->read_pos += total_len;

    /* 4字节对齐 */
    (void)CDR_Buffer_align_read(buffer, 4U);

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_serialize_fixed_string(CDR_Buffer *buffer, const char *str, uint32_t fixed_len) {
    if ((buffer == NULL_PTR) || (str == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    if ((buffer->write_pos + fixed_len) > buffer->size) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }

    uint32_t str_len = (uint32_t)strlen(str);
    if (str_len > fixed_len) {
        str_len = fixed_len;
    }

    for (uint32_t i = 0U; i < str_len; i++) {
        buffer->buffer[buffer->write_pos + i] = (uint8_t)str[i];
    }

    /* 填充剩余空间 */
    for (uint32_t i = str_len; i < fixed_len; i++) {
        buffer->buffer[buffer->write_pos + i] = 0U;
    }

    buffer->write_pos += fixed_len;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_deserialize_fixed_string(CDR_Buffer *buffer, char *str, uint32_t fixed_len) {
    if ((buffer == NULL_PTR) || (str == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    if ((buffer->read_pos + fixed_len) > buffer->write_pos) {
        return DDS_RETCODE_NO_DATA;
    }

    for (uint32_t i = 0U; i < fixed_len; i++) {
        str[i] = (char)buffer->buffer[buffer->read_pos + i];
    }
    str[fixed_len - 1U] = '\0';

    buffer->read_pos += fixed_len;

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * 原始字节序列化
 * ============================================================================ */
DDS_ReturnCode_t CDR_serialize_bytes(CDR_Buffer *buffer, const uint8_t *data, uint32_t len) {
    if ((buffer == NULL_PTR) || (data == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    if (len == 0U) {
        return DDS_RETCODE_OK;
    }

    if ((buffer->write_pos + len) > buffer->size) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }

    for (uint32_t i = 0U; i < len; i++) {
        buffer->buffer[buffer->write_pos + i] = data[i];
    }
    buffer->write_pos += len;

    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_deserialize_bytes(CDR_Buffer *buffer, uint8_t *data, uint32_t len) {
    if ((buffer == NULL_PTR) || (data == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    if (len == 0U) {
        return DDS_RETCODE_OK;
    }

    if ((buffer->read_pos + len) > buffer->write_pos) {
        return DDS_RETCODE_NO_DATA;
    }

    for (uint32_t i = 0U; i < len; i++) {
        data[i] = buffer->buffer[buffer->read_pos + i];
    }
    buffer->read_pos += len;

    return DDS_RETCODE_OK;
}

/* ============================================================================
 * 样本头部实现
 * ============================================================================ */
void CDR_SampleHeader_init(CDR_SampleHeader *header, uint32_t seq_num, uint16_t encapsulation) {
    if (header != NULL_PTR) {
        header->sequence_number = seq_num;
        header->timestamp_sec = 0U;
        header->timestamp_nsec = 0U;
        header->encapsulation = encapsulation;
        header->options = 0U;
    }
}

void CDR_SampleHeader_set_timestamp(CDR_SampleHeader *header, uint32_t sec, uint32_t nsec) {
    if (header != NULL_PTR) {
        header->timestamp_sec = sec;
        header->timestamp_nsec = nsec;
    }
}

DDS_ReturnCode_t CDR_serialize_sample_header(CDR_Buffer *buffer, const CDR_SampleHeader *header) {
    DDS_ReturnCode_t ret;

    if ((buffer == NULL_PTR) || (header == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    ret = CDR_serialize_uint16(buffer, header->encapsulation);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_serialize_uint16(buffer, header->options);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_serialize_uint32(buffer, header->sequence_number);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_serialize_uint32(buffer, header->timestamp_sec);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_serialize_uint32(buffer, header->timestamp_nsec);

    return ret;
}

DDS_ReturnCode_t CDR_deserialize_sample_header(CDR_Buffer *buffer, CDR_SampleHeader *header) {
    DDS_ReturnCode_t ret;

    if ((buffer == NULL_PTR) || (header == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    ret = CDR_deserialize_uint16(buffer, &header->encapsulation);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_deserialize_uint16(buffer, &header->options);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_deserialize_uint32(buffer, &header->sequence_number);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_deserialize_uint32(buffer, &header->timestamp_sec);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_deserialize_uint32(buffer, &header->timestamp_nsec);

    return ret;
}

/* ============================================================================
 * RTPS头部实现
 * ============================================================================ */
void CDR_RTPSHeader_init(CDR_RTPSHeader *header, uint8_t version_major, uint8_t version_minor) {
    if (header != NULL_PTR) {
        for (uint32_t i = 0U; i < 4U; i++) {
            header->protocol[i] = RTPS_PROTOCOL_ID[i];
        }
        header->version_major = version_major;
        header->version_minor = version_minor;
        header->vendor_id[0] = VENDOR_ID[0];
        header->vendor_id[1] = VENDOR_ID[1];
        for (uint32_t i = 0U; i < 12U; i++) {
            header->guid_prefix[i] = 0U;
        }
    }
}

DDS_ReturnCode_t CDR_serialize_rtps_header(CDR_Buffer *buffer, const CDR_RTPSHeader *header) {
    DDS_ReturnCode_t ret;

    if ((buffer == NULL_PTR) || (header == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* 协议标识 - 原始字节，不进行字节序转换 */
    for (uint32_t i = 0U; i < 4U; i++) {
        ret = CDR_serialize_uint8(buffer, (uint8_t)header->protocol[i]);
        if (!DDS_RETCODE_IS_OK(ret)) {
            return ret;
        }
    }

    ret = CDR_serialize_uint8(buffer, header->version_major);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_serialize_uint8(buffer, header->version_minor);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_serialize_bytes(buffer, header->vendor_id, 2U);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_serialize_bytes(buffer, header->guid_prefix, 12U);

    return ret;
}

DDS_ReturnCode_t CDR_deserialize_rtps_header(CDR_Buffer *buffer, CDR_RTPSHeader *header) {
    DDS_ReturnCode_t ret;

    if ((buffer == NULL_PTR) || (header == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* 协议标识 */
    for (uint32_t i = 0U; i < 4U; i++) {
        uint8_t byte_val;
        ret = CDR_deserialize_uint8(buffer, &byte_val);
        if (!DDS_RETCODE_IS_OK(ret)) {
            return ret;
        }
        header->protocol[i] = (char)byte_val;
    }

    uint8_t byte_val;
    ret = CDR_deserialize_uint8(buffer, &byte_val);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }
    header->version_major = byte_val;

    ret = CDR_deserialize_uint8(buffer, &byte_val);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }
    header->version_minor = byte_val;

    ret = CDR_deserialize_bytes(buffer, header->vendor_id, 2U);
    if (!DDS_RETCODE_IS_OK(ret)) {
        return ret;
    }

    ret = CDR_deserialize_bytes(buffer, header->guid_prefix, 12U);

    return ret;
}

/* ============================================================================
 * 序列号管理
 * ============================================================================ */
uint32_t CDR_generate_sequence_number(void) {
    uint32_t seq = sequence_counter;
    sequence_counter++;
    if (sequence_counter == 0U) {
        sequence_counter = 1U;
    }
    return seq;
}

void CDR_reset_sequence_number(void) {
    sequence_counter = 1U;
}

/* ============================================================================
 * 高级封装API
 * ============================================================================ */
DDS_ReturnCode_t CDR_begin_sample(CDR_Buffer *buffer, uint16_t encapsulation) {
    if (buffer == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    CDR_SampleHeader header;
    CDR_SampleHeader_init(&header, CDR_generate_sequence_number(), encapsulation);

    /* 设置当前时间戳（简化实现） */
    CDR_SampleHeader_set_timestamp(&header, 0U, 0U);

    return CDR_serialize_sample_header(buffer, &header);
}

DDS_ReturnCode_t CDR_end_sample(CDR_Buffer *buffer) {
    /* CDR样本不需要特殊的结束标记 */
    (void)buffer;
    return DDS_RETCODE_OK;
}

DDS_ReturnCode_t CDR_get_serialized_data(const CDR_Buffer *buffer, uint8_t **data, uint32_t *len) {
    if ((buffer == NULL_PTR) || (data == NULL_PTR) || (len == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    *data = buffer->buffer;
    *len = buffer->write_pos;

    return DDS_RETCODE_OK;
}
