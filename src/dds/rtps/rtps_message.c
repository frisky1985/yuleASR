/**
 * @file rtps_message.c
 * @brief RTPS报文编解码实现
 * @version 1.0
 * @date 2026-04-24
 */

#include "rtps_message.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * 字节序辅助函数
 * ============================================================================ */

static inline uint16_t read_u16(const uint8_t *p, bool little_endian) {
    if (little_endian) {
        return ((uint16_t)p[1] << 8) | p[0];
    }
    return ((uint16_t)p[0] << 8) | p[1];
}

static inline void write_u16(uint8_t *p, uint16_t v, bool little_endian) {
    if (little_endian) {
        p[0] = v & 0xFF;
        p[1] = (v >> 8) & 0xFF;
    } else {
        p[0] = (v >> 8) & 0xFF;
        p[1] = v & 0xFF;
    }
}

static inline uint32_t read_u32(const uint8_t *p, bool little_endian) {
    if (little_endian) {
        return ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) | 
               ((uint32_t)p[1] << 8) | p[0];
    }
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | 
           ((uint32_t)p[2] << 8) | p[3];
}

static inline void write_u32(uint8_t *p, uint32_t v, bool little_endian) {
    if (little_endian) {
        p[0] = v & 0xFF;
        p[1] = (v >> 8) & 0xFF;
        p[2] = (v >> 16) & 0xFF;
        p[3] = (v >> 24) & 0xFF;
    } else {
        p[0] = (v >> 24) & 0xFF;
        p[1] = (v >> 16) & 0xFF;
        p[2] = (v >> 8) & 0xFF;
        p[3] = v & 0xFF;
    }
}

static inline int32_t read_i32(const uint8_t *p, bool little_endian) {
    return (int32_t)read_u32(p, little_endian);
}

static inline void write_i32(uint8_t *p, int32_t v, bool little_endian) {
    write_u32(p, (uint32_t)v, little_endian);
}

static inline int64_t read_i64(const uint8_t *p, bool little_endian) {
    if (little_endian) {
        return ((int64_t)p[7] << 56) | ((int64_t)p[6] << 48) |
               ((int64_t)p[5] << 40) | ((int64_t)p[4] << 32) |
               ((int64_t)p[3] << 24) | ((int64_t)p[2] << 16) |
               ((int64_t)p[1] << 8) | p[0];
    }
    return ((int64_t)p[0] << 56) | ((int64_t)p[1] << 48) |
           ((int64_t)p[2] << 40) | ((int64_t)p[3] << 32) |
           ((int64_t)p[4] << 24) | ((int64_t)p[5] << 16) |
           ((int64_t)p[6] << 8) | p[7];
}

static inline void write_i64(uint8_t *p, int64_t v, bool little_endian) {
    if (little_endian) {
        p[0] = v & 0xFF;
        p[1] = (v >> 8) & 0xFF;
        p[2] = (v >> 16) & 0xFF;
        p[3] = (v >> 24) & 0xFF;
        p[4] = (v >> 32) & 0xFF;
        p[5] = (v >> 40) & 0xFF;
        p[6] = (v >> 48) & 0xFF;
        p[7] = (v >> 56) & 0xFF;
    } else {
        p[0] = (v >> 56) & 0xFF;
        p[1] = (v >> 48) & 0xFF;
        p[2] = (v >> 40) & 0xFF;
        p[3] = (v >> 32) & 0xFF;
        p[4] = (v >> 24) & 0xFF;
        p[5] = (v >> 16) & 0xFF;
        p[6] = (v >> 8) & 0xFF;
        p[7] = v & 0xFF;
    }
}

/* ============================================================================
 * 报文头操作实现
 * ============================================================================ */

eth_status_t rtps_header_build(rtps_header_t *header,
                                const rtps_guid_prefix_t guid_prefix,
                                uint16_t vendor_id)
{
    if (header == NULL || guid_prefix == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    header->magic[0] = 'R';
    header->magic[1] = 'T';
    header->magic[2] = 'P';
    header->magic[3] = 'S';
    header->version_major = RTPS_VERSION_MAJOR;
    header->version_minor = RTPS_VERSION_MINOR;
    header->vendor_id[0] = (vendor_id >> 8) & 0xFF;
    header->vendor_id[1] = vendor_id & 0xFF;
    memcpy(header->guid_prefix, guid_prefix, RTPS_GUID_PREFIX_SIZE);
    
    return ETH_OK;
}

eth_status_t rtps_header_parse(const uint8_t *data,
                                uint32_t len,
                                rtps_header_t *header,
                                uint32_t *bytes_consumed)
{
    if (data == NULL || header == NULL || bytes_consumed == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (len < RTPS_HEADER_SIZE) {
        return ETH_ERROR;
    }
    
    memcpy(header->magic, data, 4);
    header->version_major = data[4];
    header->version_minor = data[5];
    header->vendor_id[0] = data[6];
    header->vendor_id[1] = data[7];
    memcpy(header->guid_prefix, &data[8], RTPS_GUID_PREFIX_SIZE);
    
    *bytes_consumed = RTPS_HEADER_SIZE;
    return ETH_OK;
}

bool rtps_header_check_magic(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len < 4) {
        return false;
    }
    return (data[0] == 'R' && data[1] == 'T' && 
            data[2] == 'P' && data[3] == 'S');
}

/* ============================================================================
 * 报文构建器实现
 * ============================================================================ */

eth_status_t rtps_message_builder_init(rtps_message_builder_t *builder,
                                         uint8_t *buffer,
                                         uint32_t max_size,
                                         const rtps_guid_prefix_t guid_prefix,
                                         bool little_endian)
{
    if (builder == NULL || buffer == NULL || guid_prefix == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (max_size < RTPS_HEADER_SIZE) {
        return ETH_ERROR;
    }
    
    builder->buffer = buffer;
    builder->max_size = max_size;
    builder->current_pos = 0;
    builder->little_endian = little_endian;
    memcpy(builder->src_guid_prefix, guid_prefix, RTPS_GUID_PREFIX_SIZE);
    
    /* 构建报文头 */
    buffer[0] = 'R';
    buffer[1] = 'T';
    buffer[2] = 'P';
    buffer[3] = 'S';
    buffer[4] = RTPS_VERSION_MAJOR;
    buffer[5] = RTPS_VERSION_MINOR;
    buffer[6] = 0x01;  /* Vendor ID high */
    buffer[7] = 0x00;  /* Vendor ID low */
    memcpy(&buffer[8], guid_prefix, RTPS_GUID_PREFIX_SIZE);
    
    builder->current_pos = RTPS_HEADER_SIZE;
    
    return ETH_OK;
}

eth_status_t rtps_message_add_data(rtps_message_builder_t *builder,
                                    const rtps_entity_id_t reader_id,
                                    const rtps_entity_id_t writer_id,
                                    const rtps_sequence_number_t *seq_number,
                                    const uint8_t *data,
                                    uint32_t data_len,
                                    const uint8_t *inline_qos,
                                    uint32_t inline_qos_len)
{
    if (builder == NULL || reader_id == NULL || writer_id == NULL || 
        seq_number == NULL || data == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 计算所需空间 */
    uint32_t submsg_size = 4;  /* extra_flags + octets_to_inline_qos */
    submsg_size += 4;  /* reader_id */
    submsg_size += 4;  /* writer_id */
    submsg_size += 8;  /* writer_sn */
    
    uint32_t octets_to_inline_qos = submsg_size - 4;
    
    if (inline_qos != NULL && inline_qos_len > 0) {
        submsg_size += inline_qos_len;
        submsg_size = (submsg_size + 3) & ~3;  /* 4字节对齐 */
    }
    
    submsg_size += 4;  /* encapsulated data header */
    submsg_size += data_len;
    submsg_size = (submsg_size + 3) & ~3;
    
    if (builder->current_pos + RTPS_SUBMESSAGE_HEADER_SIZE + submsg_size > builder->max_size) {
        return ETH_ERROR;
    }
    
    uint32_t pos = builder->current_pos;
    
    /* Submessage header */
    builder->buffer[pos++] = RTPS_SUBMESSAGE_ID_DATA;
    uint8_t flags = RTPS_SUBMESSAGE_FLAG_DATA_PRESENT;
    if (builder->little_endian) flags |= RTPS_SUBMESSAGE_FLAG_ENDIANNESS;
    if (inline_qos != NULL && inline_qos_len > 0) flags |= RTPS_SUBMESSAGE_FLAG_INLINE_QOS;
    builder->buffer[pos++] = flags;
    write_u16(&builder->buffer[pos], (uint16_t)(submsg_size - 4), builder->little_endian);
    pos += 2;
    
    /* extra_flags */
    write_u16(&builder->buffer[pos], 0, builder->little_endian);
    pos += 2;
    
    /* octets_to_inline_qos */
    write_u16(&builder->buffer[pos], (uint16_t)octets_to_inline_qos, builder->little_endian);
    pos += 2;
    
    /* reader_id */
    memcpy(&builder->buffer[pos], reader_id, 4);
    pos += 4;
    
    /* writer_id */
    memcpy(&builder->buffer[pos], writer_id, 4);
    pos += 4;
    
    /* writer_sn */
    write_i32(&builder->buffer[pos], seq_number->high, builder->little_endian);
    pos += 4;
    write_u32(&builder->buffer[pos], seq_number->low, builder->little_endian);
    pos += 4;
    
    /* inline QoS (if present) */
    if (inline_qos != NULL && inline_qos_len > 0) {
        memcpy(&builder->buffer[pos], inline_qos, inline_qos_len);
        pos += inline_qos_len;
        while (pos % 4 != 0) builder->buffer[pos++] = 0;
    }
    
    /* encapsulated data header (CDR_BE) */
    write_u16(&builder->buffer[pos], RTPS_CDR_BE, builder->little_endian);
    pos += 2;
    builder->buffer[pos++] = 0;
    builder->buffer[pos++] = 0;
    
    /* serialized data */
    memcpy(&builder->buffer[pos], data, data_len);
    pos += data_len;
    while (pos % 4 != 0) builder->buffer[pos++] = 0;
    
    builder->current_pos = pos;
    return ETH_OK;
}

eth_status_t rtps_message_add_heartbeat(rtps_message_builder_t *builder,
                                         const rtps_entity_id_t reader_id,
                                         const rtps_entity_id_t writer_id,
                                         const rtps_sequence_number_t *first_sn,
                                         const rtps_sequence_number_t *last_sn,
                                         uint32_t count,
                                         bool final)
{
    if (builder == NULL || reader_id == NULL || writer_id == NULL || 
        first_sn == NULL || last_sn == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    const uint32_t submsg_size = 28;  /* 4 + 4 + 4 + 8 + 8 + 4 */
    
    if (builder->current_pos + RTPS_SUBMESSAGE_HEADER_SIZE + submsg_size > builder->max_size) {
        return ETH_ERROR;
    }
    
    uint32_t pos = builder->current_pos;
    
    /* Submessage header */
    builder->buffer[pos++] = RTPS_SUBMESSAGE_ID_HEARTBEAT;
    uint8_t flags = 0;
    if (builder->little_endian) flags |= RTPS_SUBMESSAGE_FLAG_ENDIANNESS;
    if (final) flags |= RTPS_SUBMESSAGE_FLAG_FINAL;
    builder->buffer[pos++] = flags;
    write_u16(&builder->buffer[pos], submsg_size - 4, builder->little_endian);
    pos += 2;
    
    /* reader_id */
    memcpy(&builder->buffer[pos], reader_id, 4);
    pos += 4;
    
    /* writer_id */
    memcpy(&builder->buffer[pos], writer_id, 4);
    pos += 4;
    
    /* first_sn */
    write_i32(&builder->buffer[pos], first_sn->high, builder->little_endian);
    pos += 4;
    write_u32(&builder->buffer[pos], first_sn->low, builder->little_endian);
    pos += 4;
    
    /* last_sn */
    write_i32(&builder->buffer[pos], last_sn->high, builder->little_endian);
    pos += 4;
    write_u32(&builder->buffer[pos], last_sn->low, builder->little_endian);
    pos += 4;
    
    /* count */
    write_u32(&builder->buffer[pos], count, builder->little_endian);
    pos += 4;
    
    builder->current_pos = pos;
    return ETH_OK;
}

eth_status_t rtps_message_add_acknack(rtps_message_builder_t *builder,
                                       const rtps_entity_id_t reader_id,
                                       const rtps_entity_id_t writer_id,
                                       const rtps_sequence_number_t *bitmap_base,
                                       const uint32_t *bitmap,
                                       uint32_t num_bits,
                                       uint32_t count)
{
    if (builder == NULL || reader_id == NULL || writer_id == NULL || 
        bitmap_base == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    uint32_t num_words = (num_bits + 31) / 32;
    if (num_words > 8) num_words = 8;  /* 限制最大位数 */
    
    uint32_t submsg_size = 4;  /* reader_id */
    submsg_size += 4;  /* writer_id */
    submsg_size += 8;  /* bitmap_base */
    submsg_size += 4;  /* num_bits */
    submsg_size += num_words * 4;  /* bitmap */
    submsg_size += 4;  /* count */
    
    if (builder->current_pos + RTPS_SUBMESSAGE_HEADER_SIZE + submsg_size > builder->max_size) {
        return ETH_ERROR;
    }
    
    uint32_t pos = builder->current_pos;
    
    /* Submessage header */
    builder->buffer[pos++] = RTPS_SUBMESSAGE_ID_ACKNACK;
    uint8_t flags = 0;
    if (builder->little_endian) flags |= RTPS_SUBMESSAGE_FLAG_ENDIANNESS;
    builder->buffer[pos++] = flags;
    write_u16(&builder->buffer[pos], submsg_size - 4, builder->little_endian);
    pos += 2;
    
    /* reader_id */
    memcpy(&builder->buffer[pos], reader_id, 4);
    pos += 4;
    
    /* writer_id */
    memcpy(&builder->buffer[pos], writer_id, 4);
    pos += 4;
    
    /* bitmap_base */
    write_i32(&builder->buffer[pos], bitmap_base->high, builder->little_endian);
    pos += 4;
    write_u32(&builder->buffer[pos], bitmap_base->low, builder->little_endian);
    pos += 4;
    
    /* num_bits */
    write_u32(&builder->buffer[pos], num_bits, builder->little_endian);
    pos += 4;
    
    /* bitmap */
    for (uint32_t i = 0; i < num_words; i++) {
        write_u32(&builder->buffer[pos], bitmap ? bitmap[i] : 0, builder->little_endian);
        pos += 4;
    }
    
    /* count */
    write_u32(&builder->buffer[pos], count, builder->little_endian);
    pos += 4;
    
    builder->current_pos = pos;
    return ETH_OK;
}

eth_status_t rtps_message_add_gap(rtps_message_builder_t *builder,
                                   const rtps_entity_id_t reader_id,
                                   const rtps_entity_id_t writer_id,
                                   const rtps_sequence_number_t *gap_start,
                                   const uint32_t *gap_bitmap,
                                   uint32_t num_bits)
{
    if (builder == NULL || reader_id == NULL || writer_id == NULL || gap_start == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    uint32_t num_words = (num_bits + 31) / 32;
    if (num_words > 8) num_words = 8;
    
    uint32_t submsg_size = 4 + 4 + 8 + 4 + num_words * 4;
    
    if (builder->current_pos + RTPS_SUBMESSAGE_HEADER_SIZE + submsg_size > builder->max_size) {
        return ETH_ERROR;
    }
    
    uint32_t pos = builder->current_pos;
    
    builder->buffer[pos++] = RTPS_SUBMESSAGE_ID_GAP;
    uint8_t flags = builder->little_endian ? RTPS_SUBMESSAGE_FLAG_ENDIANNESS : 0;
    builder->buffer[pos++] = flags;
    write_u16(&builder->buffer[pos], submsg_size - 4, builder->little_endian);
    pos += 2;
    
    memcpy(&builder->buffer[pos], reader_id, 4);
    pos += 4;
    memcpy(&builder->buffer[pos], writer_id, 4);
    pos += 4;
    
    write_i32(&builder->buffer[pos], gap_start->high, builder->little_endian);
    pos += 4;
    write_u32(&builder->buffer[pos], gap_start->low, builder->little_endian);
    pos += 4;
    
    write_u32(&builder->buffer[pos], num_bits, builder->little_endian);
    pos += 4;
    
    for (uint32_t i = 0; i < num_words; i++) {
        write_u32(&builder->buffer[pos], gap_bitmap ? gap_bitmap[i] : 0xFFFFFFFF, builder->little_endian);
        pos += 4;
    }
    
    builder->current_pos = pos;
    return ETH_OK;
}

eth_status_t rtps_message_add_info_dst(rtps_message_builder_t *builder,
                                        const rtps_guid_prefix_t guid_prefix)
{
    if (builder == NULL || guid_prefix == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (builder->current_pos + RTPS_SUBMESSAGE_HEADER_SIZE + 12 > builder->max_size) {
        return ETH_ERROR;
    }
    
    uint32_t pos = builder->current_pos;
    
    builder->buffer[pos++] = RTPS_SUBMESSAGE_ID_INFO_DST;
    uint8_t flags = builder->little_endian ? RTPS_SUBMESSAGE_FLAG_ENDIANNESS : 0;
    builder->buffer[pos++] = flags;
    write_u16(&builder->buffer[pos], 12, builder->little_endian);
    pos += 2;
    
    memcpy(&builder->buffer[pos], guid_prefix, RTPS_GUID_PREFIX_SIZE);
    pos += RTPS_GUID_PREFIX_SIZE;
    
    builder->current_pos = pos;
    return ETH_OK;
}

eth_status_t rtps_message_add_info_ts(rtps_message_builder_t *builder,
                                       int64_t timestamp)
{
    if (builder == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    bool has_timestamp = (timestamp >= 0);
    uint32_t submsg_size = has_timestamp ? 8 : 0;
    
    if (builder->current_pos + RTPS_SUBMESSAGE_HEADER_SIZE + submsg_size > builder->max_size) {
        return ETH_ERROR;
    }
    
    uint32_t pos = builder->current_pos;
    
    builder->buffer[pos++] = RTPS_SUBMESSAGE_ID_INFO_TS;
    uint8_t flags = builder->little_endian ? RTPS_SUBMESSAGE_FLAG_ENDIANNESS : 0;
    if (!has_timestamp) flags |= 0x02;  /* Invalidate flag */
    builder->buffer[pos++] = flags;
    write_u16(&builder->buffer[pos], (uint16_t)submsg_size, builder->little_endian);
    pos += 2;
    
    if (has_timestamp) {
        write_i64(&builder->buffer[pos], timestamp, builder->little_endian);
        pos += 8;
    }
    
    builder->current_pos = pos;
    return ETH_OK;
}

eth_status_t rtps_message_builder_finalize(rtps_message_builder_t *builder,
                                            uint32_t *final_length)
{
    if (builder == NULL || final_length == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    *final_length = builder->current_pos;
    return ETH_OK;
}

/* ============================================================================
 * 报文解析器实现
 * ============================================================================ */

eth_status_t rtps_message_parser_init(rtps_message_parser_t *parser,
                                        const uint8_t *data,
                                        uint32_t len)
{
    if (parser == NULL || data == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (len < RTPS_HEADER_SIZE) {
        return ETH_ERROR;
    }
    
    parser->buffer = data;
    parser->buffer_size = len;
    parser->current_pos = RTPS_HEADER_SIZE;
    
    /* 解析报文头获取字节序 */
    parser->little_endian = false;  /* 默认大端 */
    memcpy(parser->header.magic, data, 4);
    parser->header.version_major = data[4];
    parser->header.version_minor = data[5];
    memcpy(parser->header.vendor_id, &data[6], 2);
    memcpy(parser->header.guid_prefix, &data[8], 12);
    
    return ETH_OK;
}

eth_status_t rtps_message_parser_next(rtps_message_parser_t *parser,
                                        rtps_parsed_submessage_t *submsg)
{
    if (parser == NULL || submsg == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (parser->current_pos + RTPS_SUBMESSAGE_HEADER_SIZE > parser->buffer_size) {
        return ETH_TIMEOUT;  /* 没有更多Submessage */
    }
    
    uint32_t pos = parser->current_pos;
    
    submsg->id = (rtps_submessage_id_t)parser->buffer[pos++];
    submsg->flags = parser->buffer[pos++];
    parser->little_endian = (submsg->flags & RTPS_SUBMESSAGE_FLAG_ENDIANNESS) != 0;
    uint16_t length = read_u16(&parser->buffer[pos], parser->little_endian);
    pos += 2;
    
    /* 处理特殊长度 */
    uint32_t submsg_length = length;
    if (submsg_length == 0 && parser->current_pos == RTPS_HEADER_SIZE) {
        /* 最后一个Submessage可能长度为0 */
        submsg_length = parser->buffer_size - pos;
    }
    
    if (pos + submsg_length > parser->buffer_size) {
        return ETH_ERROR;
    }
    
    submsg->data = &parser->buffer[pos];
    submsg->length = (uint16_t)submsg_length;
    
    parser->current_pos = pos + submsg_length;
    
    /* 4字节对齐 */
    parser->current_pos = (parser->current_pos + 3) & ~3;
    
    return ETH_OK;
}

eth_status_t rtps_parse_data_submessage(const uint8_t *data,
                                         uint16_t len,
                                         uint8_t flags,
                                         bool little_endian,
                                         rtps_entity_id_t out_reader_id,
                                         rtps_entity_id_t out_writer_id,
                                         rtps_sequence_number_t *out_seq_number,
                                         const uint8_t **out_payload,
                                         uint32_t *out_payload_len)
{
    if (data == NULL || out_reader_id == NULL || out_writer_id == NULL || 
        out_seq_number == NULL || out_payload == NULL || out_payload_len == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (len < 20) {
        return ETH_ERROR;
    }
    
    uint16_t pos = 0;
    
    /* 跳过 extra_flags 和 octets_to_inline_qos */
    pos += 4;
    
    /* reader_id */
    memcpy(out_reader_id, &data[pos], 4);
    pos += 4;
    
    /* writer_id */
    memcpy(out_writer_id, &data[pos], 4);
    pos += 4;
    
    /* writer_sn */
    out_seq_number->high = read_i32(&data[pos], little_endian);
    pos += 4;
    out_seq_number->low = read_u32(&data[pos], little_endian);
    pos += 4;
    
    /* 处理 inline QoS */
    if (flags & RTPS_SUBMESSAGE_FLAG_INLINE_QOS) {
        /* 简化处理：跳过到parameter list sentinel */
        while (pos + 4 <= len) {
            uint16_t param_id = read_u16(&data[pos], little_endian);
            if (param_id == RTPS_PARAMETER_ID_SENTINEL) {
                pos += 4;
                break;
            }
            uint16_t param_len = read_u16(&data[pos + 2], little_endian);
            pos += 4 + param_len;
            pos = (pos + 3) & ~3;
        }
    }
    
    /* 处理封装数据 */
    if (pos + 4 <= len) {
        uint16_t encapsulation = read_u16(&data[pos], little_endian);
        (void)encapsulation;
        pos += 4;  /* 跳过 encapsulation header */
        
        *out_payload = &data[pos];
        *out_payload_len = len - pos;
    } else {
        *out_payload = NULL;
        *out_payload_len = 0;
    }
    
    return ETH_OK;
}

eth_status_t rtps_parse_heartbeat_submessage(const uint8_t *data,
                                              uint16_t len,
                                              uint8_t flags,
                                              bool little_endian,
                                              rtps_entity_id_t out_reader_id,
                                              rtps_entity_id_t out_writer_id,
                                              rtps_sequence_number_t *out_first_sn,
                                              rtps_sequence_number_t *out_last_sn,
                                              uint32_t *out_count)
{
    if (data == NULL || out_reader_id == NULL || out_writer_id == NULL || 
        out_first_sn == NULL || out_last_sn == NULL || out_count == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (len < 24) {
        return ETH_ERROR;
    }
    
    (void)flags;
    
    memcpy(out_reader_id, &data[0], 4);
    memcpy(out_writer_id, &data[4], 4);
    
    out_first_sn->high = read_i32(&data[8], little_endian);
    out_first_sn->low = read_u32(&data[12], little_endian);
    
    out_last_sn->high = read_i32(&data[16], little_endian);
    out_last_sn->low = read_u32(&data[20], little_endian);
    
    *out_count = read_u32(&data[24], little_endian);
    
    return ETH_OK;
}

eth_status_t rtps_parse_acknack_submessage(const uint8_t *data,
                                            uint16_t len,
                                            uint8_t flags,
                                            bool little_endian,
                                            rtps_entity_id_t out_reader_id,
                                            rtps_entity_id_t out_writer_id,
                                            rtps_sequence_number_t *out_bitmap_base,
                                            uint32_t *out_bitmap,
                                            uint32_t *out_num_bits,
                                            uint32_t *out_count)
{
    if (data == NULL || out_reader_id == NULL || out_writer_id == NULL || 
        out_bitmap_base == NULL || out_num_bits == NULL || out_count == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (len < 16) {
        return ETH_ERROR;
    }
    
    (void)flags;
    
    memcpy(out_reader_id, &data[0], 4);
    memcpy(out_writer_id, &data[4], 4);
    
    out_bitmap_base->high = read_i32(&data[8], little_endian);
    out_bitmap_base->low = read_u32(&data[12], little_endian);
    
    *out_num_bits = read_u32(&data[16], little_endian);
    
    uint32_t num_words = (*out_num_bits + 31) / 32;
    if (16 + 4 + num_words * 4 > len) {
        num_words = (len - 20) / 4;
    }
    
    if (out_bitmap != NULL) {
        for (uint32_t i = 0; i < num_words && i < 8; i++) {
            out_bitmap[i] = read_u32(&data[20 + i * 4], little_endian);
        }
    }
    
    *out_count = read_u32(&data[20 + num_words * 4], little_endian);
    
    return ETH_OK;
}

/* ============================================================================
 * 序列化工具实现
 * ============================================================================ */

int64_t rtps_seqnum_to_int64(const rtps_sequence_number_t *seq)
{
    if (seq == NULL) {
        return 0;
    }
    return ((int64_t)seq->high << 32) | seq->low;
}

void rtps_int64_to_seqnum(int64_t value, rtps_sequence_number_t *seq)
{
    if (seq == NULL) {
        return;
    }
    seq->high = (int32_t)(value >> 32);
    seq->low = (uint32_t)(value & 0xFFFFFFFF);
}

uint32_t rtps_serialize_guid(const rtps_guid_t *guid, 
                              uint8_t *buffer, 
                              bool little_endian)
{
    if (guid == NULL || buffer == NULL) {
        return 0;
    }
    
    memcpy(buffer, guid->prefix, RTPS_GUID_PREFIX_SIZE);
    memcpy(&buffer[RTPS_GUID_PREFIX_SIZE], guid->entity_id, RTPS_ENTITY_ID_SIZE);
    
    (void)little_endian;
    return RTPS_GUID_SIZE;
}

eth_status_t rtps_deserialize_guid(const uint8_t *buffer,
                                    uint32_t len,
                                    bool little_endian,
                                    rtps_guid_t *guid)
{
    if (buffer == NULL || guid == NULL || len < RTPS_GUID_SIZE) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(guid->prefix, buffer, RTPS_GUID_PREFIX_SIZE);
    memcpy(guid->entity_id, &buffer[RTPS_GUID_PREFIX_SIZE], RTPS_ENTITY_ID_SIZE);
    
    (void)little_endian;
    return ETH_OK;
}

uint32_t rtps_serialize_entity_id(const rtps_entity_id_t entity_id, uint8_t *buffer)
{
    if (entity_id == NULL || buffer == NULL) {
        return 0;
    }
    memcpy(buffer, entity_id, RTPS_ENTITY_ID_SIZE);
    return RTPS_ENTITY_ID_SIZE;
}

eth_status_t rtps_deserialize_entity_id(const uint8_t *buffer, 
                                         rtps_entity_id_t entity_id)
{
    if (buffer == NULL || entity_id == NULL) {
        return ETH_INVALID_PARAM;
    }
    memcpy(entity_id, buffer, RTPS_ENTITY_ID_SIZE);
    return ETH_OK;
}

uint32_t rtps_calculate_checksum(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0) {
        return 0;
    }
    
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < len; i++) {
        checksum += data[i];
    }
    return checksum;
}

/* ============================================================================
 * 车载优化实现
 * ============================================================================ */

eth_status_t rtps_message_add_data_autosar(rtps_message_builder_t *builder,
                                            const rtps_entity_id_t reader_id,
                                            const rtps_entity_id_t writer_id,
                                            const rtps_sequence_number_t *seq_number,
                                            const uint8_t *data,
                                            uint32_t data_len,
                                            uint32_t activation_time_us)
{
    if (builder == NULL || reader_id == NULL || writer_id == NULL || 
        seq_number == NULL || data == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 计算所需空间 - 包含AUTOSAR扩展 */
    uint32_t submsg_size = 4 + 4 + 4 + 8;
    submsg_size += 4;  /* AUTOSAR extension header */
    submsg_size += 4;  /* activation_time_us */
    submsg_size += 4;  /* encapsulated data header */
    submsg_size += data_len;
    submsg_size = (submsg_size + 3) & ~3;
    
    if (builder->current_pos + RTPS_SUBMESSAGE_HEADER_SIZE + submsg_size > builder->max_size) {
        return ETH_ERROR;
    }
    
    uint32_t pos = builder->current_pos;
    
    /* Submessage header */
    builder->buffer[pos++] = RTPS_SUBMESSAGE_ID_DATA;
    uint8_t flags = RTPS_SUBMESSAGE_FLAG_DATA_PRESENT | RTPS_SUBMESSAGE_FLAG_INLINE_QOS;
    if (builder->little_endian) flags |= RTPS_SUBMESSAGE_FLAG_ENDIANNESS;
    builder->buffer[pos++] = flags;
    write_u16(&builder->buffer[pos], (uint16_t)(submsg_size - 4), builder->little_endian);
    pos += 2;
    
    /* extra_flags */
    write_u16(&builder->buffer[pos], 0x0001, builder->little_endian);  /* AUTOSAR flag */
    pos += 2;
    
    /* octets_to_inline_qos */
    write_u16(&builder->buffer[pos], 16, builder->little_endian);
    pos += 2;
    
    /* reader_id */
    memcpy(&builder->buffer[pos], reader_id, 4);
    pos += 4;
    
    /* writer_id */
    memcpy(&builder->buffer[pos], writer_id, 4);
    pos += 4;
    
    /* writer_sn */
    write_i32(&builder->buffer[pos], seq_number->high, builder->little_endian);
    pos += 4;
    write_u32(&builder->buffer[pos], seq_number->low, builder->little_endian);
    pos += 4;
    
    /* AUTOSAR inline QoS - activation time */
    write_u16(&builder->buffer[pos], 0x8001, builder->little_endian);  /* Vendor-specific parameter ID */
    pos += 2;
    write_u16(&builder->buffer[pos], 4, builder->little_endian);
    pos += 2;
    write_u32(&builder->buffer[pos], activation_time_us, builder->little_endian);
    pos += 4;
    
    /* Sentinel */
    write_u16(&builder->buffer[pos], RTPS_PARAMETER_ID_SENTINEL, builder->little_endian);
    pos += 2;
    write_u16(&builder->buffer[pos], 0, builder->little_endian);
    pos += 2;
    
    /* encapsulated data */
    write_u16(&builder->buffer[pos], RTPS_CDR_BE, builder->little_endian);
    pos += 2;
    builder->buffer[pos++] = 0;
    builder->buffer[pos++] = 0;
    
    memcpy(&builder->buffer[pos], data, data_len);
    pos += data_len;
    while (pos % 4 != 0) builder->buffer[pos++] = 0;
    
    builder->current_pos = pos;
    return ETH_OK;
}

eth_status_t rtps_parse_autosar_extension(const uint8_t *data,
                                           uint32_t len,
                                           uint32_t *out_activation_time_us)
{
    if (data == NULL || out_activation_time_us == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    uint32_t pos = 0;
    while (pos + 4 <= len) {
        uint16_t param_id = (data[pos] << 8) | data[pos + 1];
        uint16_t param_len = (data[pos + 2] << 8) | data[pos + 3];
        
        if (param_id == RTPS_PARAMETER_ID_SENTINEL) {
            break;
        }
        
        if (param_id == 0x8001 && param_len == 4 && pos + 8 <= len) {
            *out_activation_time_us = ((uint32_t)data[pos + 4] << 24) |
                                       ((uint32_t)data[pos + 5] << 16) |
                                       ((uint32_t)data[pos + 6] << 8) |
                                       data[pos + 7];
            return ETH_OK;
        }
        
        pos += 4 + param_len;
        pos = (pos + 3) & ~3;
    }
    
    return ETH_ERROR;
}

bool rtps_message_validate(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len < RTPS_HEADER_SIZE) {
        return false;
    }
    
    /* 验证魔法字 */
    if (!rtps_header_check_magic(data, len)) {
        return false;
    }
    
    /* 验证版本 */
    if (data[4] != 2) {
        return false;
    }
    
    /* 检查长度合理性 */
    if (len > RTPS_MAX_MESSAGE_SIZE) {
        return false;
    }
    
    return true;
}

uint32_t rtps_message_estimate_size(uint32_t data_len, bool has_inline_qos)
{
    uint32_t size = RTPS_HEADER_SIZE;
    size += RTPS_SUBMESSAGE_HEADER_SIZE;
    size += 20;  /* DATA submessage fixed fields */
    if (has_inline_qos) {
        size += 8;  /* 预估QoS字段 */
    }
    size += 4;  /* encapsulation header */
    size += data_len;
    size = (size + 3) & ~3;
    return size;
}
