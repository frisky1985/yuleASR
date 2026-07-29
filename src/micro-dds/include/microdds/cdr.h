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
    CDR_ENDIAN_BIG = 0,
    CDR_ENDIAN_LITTLE = 1
} CDR_Endianness;

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
void CDR_Buffer_reset(CDR_Buffer* buffer);
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

/* ============================================================================
 * DDS Sample封装函数
 * ============================================================================ */

DDS_ReturnCode_t CDR_serialize_sample_header(CDR_Buffer* buffer, uint32_t sequence_number, DDS_Time_t timestamp);
DDS_ReturnCode_t CDR_deserialize_sample_header(CDR_Buffer* buffer, uint32_t* sequence_number, DDS_Time_t* timestamp);

#ifdef __cplusplus
}
#endif

#endif /* MICRODDS_CDR_H */
