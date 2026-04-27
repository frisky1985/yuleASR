/**
 * @file dlt_payload.c
 * @brief DLT Payload Builder实现
 */

#include "dlt/dlt_payload.h"
#include <string.h>

/*===========================================================================*/
/* Payload Builder API实现                                              */
/*===========================================================================*/

void Dlt_PayloadBuilder_Init(Dlt_PayloadBuilderType *builder, 
                              uint8_t *buffer, 
                              uint16_t size) {
    if (builder == NULL || buffer == NULL || size == 0) {
        return;
    }
    
    builder->buffer = buffer;
    builder->size = size;
    builder->position = 0;
    builder->remaining = size;
}

Dlt_ReturnType Dlt_PayloadBuilder_AddHeader(Dlt_PayloadBuilderType *builder,
                                             Dlt_MessageTypeType msg_type,
                                             uint8_t specific_info) {
    if (builder == NULL) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    if (builder->remaining < 1) {
        return DLT_RETURN_ERROR;
    }
    
    /* 构建MSIN */
    builder->buffer[builder->position] = ((msg_type & 0x07) << 1) | 
                                          ((specific_info & 0x0F) << 4);
    builder->position++;
    builder->remaining--;
    
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_PayloadBuilder_AddVariable(Dlt_PayloadBuilderType *builder,
                                               Dlt_PayloadDataType data_type,
                                               const void *value,
                                               uint16_t length) {
    if (builder == NULL || value == NULL) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    uint16_t required = 1; /* Type Info */
    uint16_t data_length = 0;
    
    switch (data_type) {
        case DLT_TYPE_BOOL:
        case DLT_TYPE_SINT8:
        case DLT_TYPE_UINT8:
            data_length = 1;
            break;
        case DLT_TYPE_SINT16:
        case DLT_TYPE_UINT16:
            data_length = 2;
            break;
        case DLT_TYPE_SINT32:
        case DLT_TYPE_UINT32:
        case DLT_TYPE_FLOA32:
            data_length = 4;
            break;
        case DLT_TYPE_SINT64:
        case DLT_TYPE_UINT64:
        case DLT_TYPE_FLOA64:
            data_length = 8;
            break;
        case DLT_TYPE_ARAY:
        case DLT_TYPE_RAWD:
            data_length = 2 + length; /* Length field + data */
            break;
        case DLT_TYPE_STRG:
            data_length = 2 + length + 1; /* Length + string + null terminator */
            break;
        default:
            return DLT_RETURN_ERROR;
    }
    
    required += data_length;
    
    if (builder->remaining < required) {
        return DLT_RETURN_ERROR;
    }
    
    /* 写入Type Info */
    builder->buffer[builder->position++] = data_type;
    builder->remaining--;
    
    /* 写入数据 */
    switch (data_type) {
        case DLT_TYPE_BOOL:
            builder->buffer[builder->position++] = *(const uint8_t *)value ? 1 : 0;
            break;
        case DLT_TYPE_SINT8:
        case DLT_TYPE_UINT8:
            builder->buffer[builder->position++] = *(const uint8_t *)value;
            break;
        case DLT_TYPE_SINT16:
        case DLT_TYPE_UINT16: {
            uint16_t val = *(const uint16_t *)value;
            builder->buffer[builder->position++] = val & 0xFF;
            builder->buffer[builder->position++] = (val >> 8) & 0xFF;
            break;
        }
        case DLT_TYPE_SINT32:
        case DLT_TYPE_UINT32: {
            uint32_t val = *(const uint32_t *)value;
            builder->buffer[builder->position++] = val & 0xFF;
            builder->buffer[builder->position++] = (val >> 8) & 0xFF;
            builder->buffer[builder->position++] = (val >> 16) & 0xFF;
            builder->buffer[builder->position++] = (val >> 24) & 0xFF;
            break;
        }
        case DLT_TYPE_SINT64:
        case DLT_TYPE_UINT64: {
            uint64_t val = *(const uint64_t *)value;
            for (int i = 0; i < 8; i++) {
                builder->buffer[builder->position++] = (val >> (i * 8)) & 0xFF;
            }
            break;
        }
        case DLT_TYPE_FLOA32: {
            union { float f; uint32_t u; } conv;
            conv.f = *(const float *)value;
            builder->buffer[builder->position++] = conv.u & 0xFF;
            builder->buffer[builder->position++] = (conv.u >> 8) & 0xFF;
            builder->buffer[builder->position++] = (conv.u >> 16) & 0xFF;
            builder->buffer[builder->position++] = (conv.u >> 24) & 0xFF;
            break;
        }
        case DLT_TYPE_FLOA64: {
            union { double d; uint64_t u; } conv;
            conv.d = *(const double *)value;
            for (int i = 0; i < 8; i++) {
                builder->buffer[builder->position++] = (conv.u >> (i * 8)) & 0xFF;
            }
            break;
        }
        case DLT_TYPE_ARAY:
        case DLT_TYPE_RAWD:
            builder->buffer[builder->position++] = length & 0xFF;
            builder->buffer[builder->position++] = (length >> 8) & 0xFF;
            memcpy(&builder->buffer[builder->position], value, length);
            builder->position += length;
            break;
        case DLT_TYPE_STRG:
            builder->buffer[builder->position++] = (length + 1) & 0xFF;
            builder->buffer[builder->position++] = ((length + 1) >> 8) & 0xFF;
            memcpy(&builder->buffer[builder->position], value, length);
            builder->position += length;
            builder->buffer[builder->position++] = 0; /* Null terminator */
            break;
        default:
            break;
    }
    
    builder->remaining -= data_length;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_PayloadBuilder_AddString(Dlt_PayloadBuilderType *builder,
                                             Dlt_StringCodingType coding,
                                             const char *str) {
    if (builder == NULL || str == NULL) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    uint16_t str_len = strlen(str);
    uint16_t required = 1 + 2 + str_len + 1; /* Type Info + Length + String + Null */
    
    if (builder->remaining < required) {
        return DLT_RETURN_ERROR;
    }
    
    /* Type Info with coding */
    builder->buffer[builder->position++] = DLT_TYPE_STRG | (coding << 6);
    builder->remaining--;
    
    /* String length (including null) */
    uint16_t total_len = str_len + 1;
    builder->buffer[builder->position++] = total_len & 0xFF;
    builder->buffer[builder->position++] = (total_len >> 8) & 0xFF;
    
    /* String data */
    memcpy(&builder->buffer[builder->position], str, str_len);
    builder->position += str_len;
    builder->buffer[builder->position++] = 0;
    builder->remaining -= (2 + str_len + 1);
    
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_PayloadBuilder_AddRawData(Dlt_PayloadBuilderType *builder,
                                              const uint8_t *data,
                                              uint16_t length) {
    if (builder == NULL || data == NULL) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    uint16_t required = 1 + 2 + length; /* Type Info + Length + Data */
    
    if (builder->remaining < required) {
        return DLT_RETURN_ERROR;
    }
    
    /* Type Info */
    builder->buffer[builder->position++] = DLT_TYPE_RAWD;
    builder->remaining--;
    
    /* Data length */
    builder->buffer[builder->position++] = length & 0xFF;
    builder->buffer[builder->position++] = (length >> 8) & 0xFF;
    
    /* Raw data */
    memcpy(&builder->buffer[builder->position], data, length);
    builder->position += length;
    builder->remaining -= (2 + length);
    
    return DLT_RETURN_OK;
}
