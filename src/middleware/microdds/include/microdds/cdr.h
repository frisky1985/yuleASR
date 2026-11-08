/** @file cdr.h
 * @brief CDR (Common Data Representation) 序列化/反序列化头文件
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 遵循OMG CDR规范
 */

#ifndef MICRODDS_CDR_H
#define MICRODDS_CDR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "microdds/types.h"

/* ============================================================================
 * 字节序定义
 * ============================================================================ */

typedef enum {
    CDR_ENDIAN_LITTLE = 0,
    CDR_ENDIAN_BIG = 1,
    CDR_LE = CDR_ENDIAN_LITTLE,
    CDR_BE = CDR_ENDIAN_BIG
} CDR_Endianness;

#define CDR_MAX_STRING_LENGTH 256U

/* ============================================================================
 * CDR缓冲区结构
 * ============================================================================ */

typedef struct {
    uint8_t* buffer;
    uint32_t size;
    uint32_t write_pos;
    uint32_t read_pos;
    CDR_Endianness endian;
    bool needs_swap;
    bool owns_buffer;
} CDR_Buffer;

/* ============================================================================
 * DDS 样本头 / RTPS 头结构
 * ============================================================================ */

typedef struct {
    uint16_t encapsulation;
    uint16_t options;
    uint32_t sequence_number;
    uint32_t timestamp_sec;
    uint32_t timestamp_nsec;
} CDR_SampleHeader;

typedef struct {
    char protocol[4];
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t vendor_id[2];
    uint8_t guid_prefix[12];
} CDR_RTPSHeader;

/* ============================================================================
 * 字节序转换函数
 * ============================================================================ */

CDR_Endianness CDR_get_native_endian(void);
uint16_t CDR_swap16(uint16_t value);
uint32_t CDR_swap32(uint32_t value);
uint64_t CDR_swap64(uint64_t value);
uint16_t CDR_cond_swap16(uint16_t value, bool needs_swap);
uint32_t CDR_cond_swap32(uint32_t value, bool needs_swap);
uint64_t CDR_cond_swap64(uint64_t value, bool needs_swap);

/* ============================================================================
 * 缓冲区管理函数
 * ============================================================================ */

DDS_ReturnCode_t CDR_Buffer_init(CDR_Buffer* buffer, uint8_t* data, uint32_t size, CDR_Endianness endian);
DDS_ReturnCode_t CDR_Buffer_deinit(CDR_Buffer* buffer);
DDS_ReturnCode_t CDR_Buffer_reset(CDR_Buffer* buffer);
DDS_ReturnCode_t CDR_Buffer_set_write_pos(CDR_Buffer* buffer, uint32_t offset);
DDS_ReturnCode_t CDR_Buffer_set_read_pos(CDR_Buffer* buffer, uint32_t offset);
uint32_t CDR_Buffer_remaining_write(const CDR_Buffer* buffer);
uint32_t CDR_Buffer_remaining_read(const CDR_Buffer* buffer);
DDS_ReturnCode_t CDR_Buffer_align_write(CDR_Buffer* buffer, uint8_t alignment);
DDS_ReturnCode_t CDR_Buffer_align_read(CDR_Buffer* buffer, uint8_t alignment);
uint32_t CDR_Buffer_get_serialized_data_length(const CDR_Buffer* buffer);
bool CDR_Buffer_has_overflow(const CDR_Buffer* buffer);

/* ============================================================================
 * 基本类型序列化函数
 * ============================================================================ */

DDS_ReturnCode_t CDR_serialize_uint8(CDR_Buffer* buffer, uint8_t value);
DDS_ReturnCode_t CDR_serialize_uint16(CDR_Buffer* buffer, uint16_t value);
DDS_ReturnCode_t CDR_serialize_uint32(CDR_Buffer* buffer, uint32_t value);
DDS_ReturnCode_t CDR_serialize_uint64(CDR_Buffer* buffer, uint64_t value);
DDS_ReturnCode_t CDR_serialize_int8(CDR_Buffer* buffer, int8_t value);
DDS_ReturnCode_t CDR_serialize_int16(CDR_Buffer* buffer, int16_t value);
DDS_ReturnCode_t CDR_serialize_int32(CDR_Buffer* buffer, int32_t value);
DDS_ReturnCode_t CDR_serialize_int64(CDR_Buffer* buffer, int64_t value);
DDS_ReturnCode_t CDR_serialize_float(CDR_Buffer* buffer, float value);
DDS_ReturnCode_t CDR_serialize_double(CDR_Buffer* buffer, double value);
DDS_ReturnCode_t CDR_serialize_bool(CDR_Buffer* buffer, bool value);
DDS_ReturnCode_t CDR_serialize_char(CDR_Buffer* buffer, char value);
DDS_ReturnCode_t CDR_serialize_string(CDR_Buffer* buffer, const char* value);
DDS_ReturnCode_t CDR_serialize_octet_array(CDR_Buffer* buffer, const uint8_t* data, uint32_t length);

/* ============================================================================
 * 基本类型反序列化函数
 * ============================================================================ */

DDS_ReturnCode_t CDR_deserialize_uint8(CDR_Buffer* buffer, uint8_t* value);
DDS_ReturnCode_t CDR_deserialize_uint16(CDR_Buffer* buffer, uint16_t* value);
DDS_ReturnCode_t CDR_deserialize_uint32(CDR_Buffer* buffer, uint32_t* value);
DDS_ReturnCode_t CDR_deserialize_uint64(CDR_Buffer* buffer, uint64_t* value);
DDS_ReturnCode_t CDR_deserialize_int8(CDR_Buffer* buffer, int8_t* value);
DDS_ReturnCode_t CDR_deserialize_int16(CDR_Buffer* buffer, int16_t* value);
DDS_ReturnCode_t CDR_deserialize_int32(CDR_Buffer* buffer, int32_t* value);
DDS_ReturnCode_t CDR_deserialize_int64(CDR_Buffer* buffer, int64_t* value);
DDS_ReturnCode_t CDR_deserialize_float(CDR_Buffer* buffer, float* value);
DDS_ReturnCode_t CDR_deserialize_double(CDR_Buffer* buffer, double* value);
DDS_ReturnCode_t CDR_deserialize_bool(CDR_Buffer* buffer, bool* value);
DDS_ReturnCode_t CDR_deserialize_char(CDR_Buffer* buffer, char* value);
DDS_ReturnCode_t CDR_deserialize_string(CDR_Buffer* buffer, char* value, uint32_t max_length);
DDS_ReturnCode_t CDR_deserialize_octet_array(CDR_Buffer* buffer, uint8_t* data, uint32_t length);
DDS_ReturnCode_t CDR_serialize_fixed_string(CDR_Buffer* buffer, const char* str, uint32_t fixed_len);
DDS_ReturnCode_t CDR_deserialize_fixed_string(CDR_Buffer* buffer, char* str, uint32_t fixed_len);
DDS_ReturnCode_t CDR_serialize_bytes(CDR_Buffer* buffer, const uint8_t* data, uint32_t len);
DDS_ReturnCode_t CDR_deserialize_bytes(CDR_Buffer* buffer, uint8_t* data, uint32_t len);

/* ============================================================================
 * DDS Sample封装函数
 * ============================================================================ */

void CDR_SampleHeader_init(CDR_SampleHeader* header, uint32_t seq_num, uint16_t encapsulation);
void CDR_SampleHeader_set_timestamp(CDR_SampleHeader* header, uint32_t sec, uint32_t nsec);
DDS_ReturnCode_t CDR_serialize_sample_header(CDR_Buffer* buffer, const CDR_SampleHeader* header);
DDS_ReturnCode_t CDR_deserialize_sample_header(CDR_Buffer* buffer, CDR_SampleHeader* header);

/* ============================================================================
 * RTPS 头封装函数
 * ============================================================================ */

void CDR_RTPSHeader_init(CDR_RTPSHeader* header, uint8_t version_major, uint8_t version_minor);
DDS_ReturnCode_t CDR_serialize_rtps_header(CDR_Buffer* buffer, const CDR_RTPSHeader* header);
DDS_ReturnCode_t CDR_deserialize_rtps_header(CDR_Buffer* buffer, CDR_RTPSHeader* header);

/* ============================================================================
 * 序列号与样本边界
 * ============================================================================ */

uint32_t CDR_generate_sequence_number(void);
void CDR_reset_sequence_number(void);
DDS_ReturnCode_t CDR_begin_sample(CDR_Buffer* buffer, uint16_t encapsulation);
DDS_ReturnCode_t CDR_end_sample(CDR_Buffer* buffer);
DDS_ReturnCode_t CDR_get_serialized_data(const CDR_Buffer* buffer, uint8_t** data, uint32_t* len);

#ifdef __cplusplus
}
#endif

#endif /* MICRODDS_CDR_H */
