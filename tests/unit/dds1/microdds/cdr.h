/**
 * @file cdr.h  (tests/unit/dds1/microdds/cdr.h)
 * @brief 测试侧 CDR 头 - 通过 -I tests/unit/dds1 劫持 cdr.c 的
 *        #include "microdds/cdr.h"。
 *
 * NOTE: 产品头 src/middleware/microdds/include/microdds/cdr.h 与
 * src/middleware/microdds/src/serialization/cdr.c 实现不一致：
 *  - CDR_SampleHeader / CDR_RTPSHeader / CDR_MAX_STRING_LENGTH /
 *    CDR_serialize_sample_header 的新签名等在产品头中缺失,
 *    cdr.c 无法用产品头编译。
 *  - 本 stub 头按 cdr.c 实际实现补齐缺失类型/常量/签名。
 *  (产品缺陷, 已记录, 不改产品代码)
 */
#ifndef DDS1_TEST_CDR_STUB_H
#define DDS1_TEST_CDR_STUB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "microdds/types.h"

#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

#define CDR_MAX_STRING_LENGTH 256U

typedef enum {
    CDR_ENDIAN_LITTLE = 0,
    CDR_ENDIAN_BIG = 1,
    CDR_LE = CDR_ENDIAN_LITTLE,
    CDR_BE = CDR_ENDIAN_BIG,
} CDR_Endianness;

typedef struct {
    uint8_t *buffer;
    uint32_t size;
    uint32_t write_pos;
    uint32_t read_pos;
    CDR_Endianness endian;
    bool needs_swap;
    bool owns_buffer;
} CDR_Buffer;

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

/* 与实现一致的 sample header API (产品头中的旧签名已废弃) */
void CDR_SampleHeader_init(CDR_SampleHeader *header, uint32_t seq_num, uint16_t encapsulation);
void CDR_SampleHeader_set_timestamp(CDR_SampleHeader *header, uint32_t sec, uint32_t nsec);
DDS_ReturnCode_t CDR_serialize_sample_header(CDR_Buffer *buffer, const CDR_SampleHeader *header);
DDS_ReturnCode_t CDR_deserialize_sample_header(CDR_Buffer *buffer, CDR_SampleHeader *header);
void CDR_RTPSHeader_init(CDR_RTPSHeader *header, uint8_t version_major, uint8_t version_minor);
DDS_ReturnCode_t CDR_serialize_rtps_header(CDR_Buffer *buffer, const CDR_RTPSHeader *header);
DDS_ReturnCode_t CDR_deserialize_rtps_header(CDR_Buffer *buffer, CDR_RTPSHeader *header);
DDS_ReturnCode_t CDR_begin_sample(CDR_Buffer *buffer, uint16_t encapsulation);
DDS_ReturnCode_t CDR_end_sample(CDR_Buffer *buffer);
DDS_ReturnCode_t CDR_get_serialized_data(const CDR_Buffer *buffer, uint8_t **data, uint32_t *len);
uint32_t CDR_generate_sequence_number(void);
void CDR_reset_sequence_number(void);

/* 其余 API 与产品头声明一致 */
CDR_Endianness CDR_get_native_endian(void);
uint16_t CDR_swap16(uint16_t value);
uint32_t CDR_swap32(uint32_t value);
uint64_t CDR_swap64(uint64_t value);
uint16_t CDR_cond_swap16(uint16_t value, bool needs_swap);
uint32_t CDR_cond_swap32(uint32_t value, bool needs_swap);
uint64_t CDR_cond_swap64(uint64_t value, bool needs_swap);
DDS_ReturnCode_t CDR_Buffer_init(CDR_Buffer *buffer, uint8_t *data, uint32_t size, CDR_Endianness endian);
DDS_ReturnCode_t CDR_Buffer_deinit(CDR_Buffer *buffer);
DDS_ReturnCode_t CDR_Buffer_reset(CDR_Buffer *buffer);
DDS_ReturnCode_t CDR_Buffer_set_write_pos(CDR_Buffer *buffer, uint32_t offset);
DDS_ReturnCode_t CDR_Buffer_set_read_pos(CDR_Buffer *buffer, uint32_t offset);
uint32_t CDR_Buffer_remaining_write(const CDR_Buffer *buffer);
uint32_t CDR_Buffer_remaining_read(const CDR_Buffer *buffer);
DDS_ReturnCode_t CDR_Buffer_align_write(CDR_Buffer *buffer, uint8_t alignment);
DDS_ReturnCode_t CDR_Buffer_align_read(CDR_Buffer *buffer, uint8_t alignment);
DDS_ReturnCode_t CDR_serialize_int8(CDR_Buffer *buffer, int8_t value);
DDS_ReturnCode_t CDR_deserialize_int8(CDR_Buffer *buffer, int8_t *value);
DDS_ReturnCode_t CDR_serialize_uint8(CDR_Buffer *buffer, uint8_t value);
DDS_ReturnCode_t CDR_deserialize_uint8(CDR_Buffer *buffer, uint8_t *value);
DDS_ReturnCode_t CDR_serialize_int16(CDR_Buffer *buffer, int16_t value);
DDS_ReturnCode_t CDR_deserialize_int16(CDR_Buffer *buffer, int16_t *value);
DDS_ReturnCode_t CDR_serialize_uint16(CDR_Buffer *buffer, uint16_t value);
DDS_ReturnCode_t CDR_deserialize_uint16(CDR_Buffer *buffer, uint16_t *value);
DDS_ReturnCode_t CDR_serialize_int32(CDR_Buffer *buffer, int32_t value);
DDS_ReturnCode_t CDR_deserialize_int32(CDR_Buffer *buffer, int32_t *value);
DDS_ReturnCode_t CDR_serialize_uint32(CDR_Buffer *buffer, uint32_t value);
DDS_ReturnCode_t CDR_deserialize_uint32(CDR_Buffer *buffer, uint32_t *value);
DDS_ReturnCode_t CDR_serialize_int64(CDR_Buffer *buffer, int64_t value);
DDS_ReturnCode_t CDR_deserialize_int64(CDR_Buffer *buffer, int64_t *value);
DDS_ReturnCode_t CDR_serialize_uint64(CDR_Buffer *buffer, uint64_t value);
DDS_ReturnCode_t CDR_deserialize_uint64(CDR_Buffer *buffer, uint64_t *value);
DDS_ReturnCode_t CDR_serialize_float(CDR_Buffer *buffer, float value);
DDS_ReturnCode_t CDR_deserialize_float(CDR_Buffer *buffer, float *value);
DDS_ReturnCode_t CDR_serialize_double(CDR_Buffer *buffer, double value);
DDS_ReturnCode_t CDR_deserialize_double(CDR_Buffer *buffer, double *value);
DDS_ReturnCode_t CDR_serialize_bool(CDR_Buffer *buffer, bool value);
DDS_ReturnCode_t CDR_deserialize_bool(CDR_Buffer *buffer, bool *value);
DDS_ReturnCode_t CDR_serialize_char(CDR_Buffer *buffer, char value);
DDS_ReturnCode_t CDR_deserialize_char(CDR_Buffer *buffer, char *value);
DDS_ReturnCode_t CDR_serialize_string(CDR_Buffer *buffer, const char *str);
DDS_ReturnCode_t CDR_deserialize_string(CDR_Buffer *buffer, char *str, uint32_t max_len);
DDS_ReturnCode_t CDR_serialize_fixed_string(CDR_Buffer *buffer, const char *str, uint32_t fixed_len);
DDS_ReturnCode_t CDR_deserialize_fixed_string(CDR_Buffer *buffer, char *str, uint32_t fixed_len);
DDS_ReturnCode_t CDR_serialize_bytes(CDR_Buffer *buffer, const uint8_t *data, uint32_t len);
DDS_ReturnCode_t CDR_deserialize_bytes(CDR_Buffer *buffer, uint8_t *data, uint32_t len);

#endif /* DDS1_TEST_CDR_STUB_H */
