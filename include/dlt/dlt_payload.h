/**
 * @file dlt_payload.h
 * @brief DLT Payload Type Info定义
 * 
 * 完整实现AutoSAR DLT规范的Payload Type Info（MSIN字段）位定义
 */

#ifndef DLT_PAYLOAD_H
#define DLT_PAYLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* 基础数据类型                                                        */
/*===========================================================================*/

typedef enum {
    DLT_TYPE_LOG = 0x00,
    DLT_TYPE_APP_TRACE = 0x01,
    DLT_TYPE_NW_TRACE = 0x02,
    DLT_TYPE_CONTROL = 0x03,
    DLT_TYPE_RESERVED1 = 0x04,
    DLT_TYPE_RESERVED2 = 0x05,
    DLT_TYPE_RESERVED3 = 0x06,
    DLT_TYPE_RESERVED4 = 0x07
} Dlt_MessageTypeType;

/*===========================================================================*/
/* Log Message特定信息类型                                          */
/*===========================================================================*/

typedef enum {
    DLT_LOG_OFF = 0x00,
    DLT_LOG_FATAL = 0x01,
    DLT_LOG_ERROR = 0x02,
    DLT_LOG_WARN = 0x03,
    DLT_LOG_INFO = 0x04,
    DLT_LOG_DEBUG = 0x05,
    DLT_LOG_VERBOSE = 0x06,
    DLT_LOG_RESERVED = 0x07
} Dlt_LogLevelType;

/*===========================================================================*/
/* Trace Message特定信息类型                                        */
/*===========================================================================*/

typedef enum {
    DLT_TRACE_VARIABLE = 0x01,
    DLT_TRACE_FUNCTION_IN = 0x02,
    DLT_TRACE_FUNCTION_OUT = 0x03,
    DLT_TRACE_STATE = 0x04,
    DLT_TRACE_VFB = 0x05,
    DLT_TRACE_RESERVED1 = 0x06,
    DLT_TRACE_RESERVED2 = 0x07
} Dlt_TraceInfoType;

/*===========================================================================*/
/* Network Trace特定信息类型                                        */
/*===========================================================================*/

typedef enum {
    DLT_NW_TRACE_IPC = 0x01,
    DLT_NW_TRACE_CAN = 0x02,
    DLT_NW_TRACE_FLEXRAY = 0x03,
    DLT_NW_TRACE_MOST = 0x04,
    DLT_NW_TRACE_RESERVED1 = 0x05,
    DLT_NW_TRACE_RESERVED2 = 0x06,
    DLT_NW_TRACE_RESERVED3 = 0x07,
    DLT_NW_TRACE_VFB = 0x08,
    DLT_NW_TRACE_ETHERNET = 0x09,
    DLT_NW_TRACE_RESERVED4 = 0x0A,
    DLT_NW_TRACE_RESERVED5 = 0x0B,
    DLT_NW_TRACE_RESERVED6 = 0x0C,
    DLT_NW_TRACE_RESERVED7 = 0x0D,
    DLT_NW_TRACE_RESERVED8 = 0x0E,
    DLT_NW_TRACE_RESERVED9 = 0x0F
} Dlt_NwTraceInfoType;

/*===========================================================================*/
/* Control Message特定信息类型                                      */
/*===========================================================================*/

typedef enum {
    DLT_CONTROL_REQUEST = 0x01,
    DLT_CONTROL_RESPONSE = 0x02,
    DLT_CONTROL_TIME = 0x03
} Dlt_ControlInfoType;

/*===========================================================================*/
/* MSIN字段构建                                                       */
/*===========================================================================*/

/**
 * @brief 构建Log Message的MSIN字段
 * 
 * @param log_level 日志级别
 * @return uint8_t MSIN字段值
 */
static inline uint8_t Dlt_BuildMsinLog(Dlt_LogLevelType log_level) {
    return (DLT_TYPE_LOG << 1) | (log_level << 4);
}

/**
 * @brief 构建App Trace Message的MSIN字段
 * 
 * @param trace_info Trace信息类型
 * @return uint8_t MSIN字段值
 */
static inline uint8_t Dlt_BuildMsinAppTrace(Dlt_TraceInfoType trace_info) {
    return (DLT_TYPE_APP_TRACE << 1) | (trace_info << 4);
}

/**
 * @brief 构建Network Trace Message的MSIN字段
 * 
 * @param nw_trace_info 网络Trace信息类型
 * @return uint8_t MSIN字段值
 */
static inline uint8_t Dlt_BuildMsinNwTrace(Dlt_NwTraceInfoType nw_trace_info) {
    return (DLT_TYPE_NW_TRACE << 1) | (nw_trace_info << 4);
}

/**
 * @brief 构建Control Message的MSIN字段
 * 
 * @param control_info 控制信息类型
 * @return uint8_t MSIN字段值
 */
static inline uint8_t Dlt_BuildMsinControl(Dlt_ControlInfoType control_info) {
    return (DLT_TYPE_CONTROL << 1) | (control_info << 4);
}

/*===========================================================================*/
/* MSIN字段解析                                                       */
/*===========================================================================*/

/**
 * @brief 从MSIN字段提取消息类型
 */
static inline Dlt_MessageTypeType Dlt_GetMessageTypeFromMsin(uint8_t msin) {
    return (Dlt_MessageTypeType)((msin >> 1) & 0x07);
}

/**
 * @brief 从MSIN字段提取特定信息
 */
static inline uint8_t Dlt_GetSpecificInfoFromMsin(uint8_t msin) {
    return (msin >> 4) & 0x0F;
}

/**
 * @brief 检查MSIN是否为Log Message
 */
static inline bool Dlt_IsLogMessage(uint8_t msin) {
    return Dlt_GetMessageTypeFromMsin(msin) == DLT_TYPE_LOG;
}

/**
 * @brief 检查MSIN是否为Control Message
 */
static inline bool Dlt_IsControlMessage(uint8_t msin) {
    return Dlt_GetMessageTypeFromMsin(msin) == DLT_TYPE_CONTROL;
}

/**
 * @brief 从Log Message的MSIN提取日志级别
 */
static inline Dlt_LogLevelType Dlt_GetLogLevelFromMsin(uint8_t msin) {
    if (!Dlt_IsLogMessage(msin)) {
        return DLT_LOG_OFF;
    }
    return (Dlt_LogLevelType)((msin >> 4) & 0x07);
}

/*===========================================================================*/
/* Payload数据类型定义                                               */
/*===========================================================================*/

typedef enum {
    DLT_TYPE_BOOL = 0x10,
    DLT_TYPE_SINT8 = 0x11,
    DLT_TYPE_UINT8 = 0x12,
    DLT_TYPE_SINT16 = 0x13,
    DLT_TYPE_UINT16 = 0x14,
    DLT_TYPE_SINT32 = 0x15,
    DLT_TYPE_UINT32 = 0x16,
    DLT_TYPE_SINT64 = 0x17,
    DLT_TYPE_UINT64 = 0x18,
    DLT_TYPE_FLOA32 = 0x19,
    DLT_TYPE_FLOA64 = 0x1A,
    DLT_TYPE_ARAY = 0x1B,
    DLT_TYPE_STRG = 0x1C,
    DLT_TYPE_RAWD = 0x1D,
    DLT_TYPE_TRAI = 0x1E,
    DLT_TYPE_STRU = 0x1F,
    DLT_TYPE_SCOD = 0x20
} Dlt_PayloadDataType;

/*===========================================================================*/
/* Payload编码方式                                                       */
/*===========================================================================*/

typedef enum {
    DLT_SCOD_ASCII = 0x00,
    DLT_SCOD_UTF8 = 0x01,
    DLT_SCOD_RESERVED1 = 0x02,
    DLT_SCOD_RESERVED2 = 0x03
} Dlt_StringCodingType;

/*===========================================================================
 * 完整的Payload类型编码表
 * 
 * 4位类型ID | 4位标志 = 8位类型信息
 * 
 * 类型ID:
 *  0x1: 布尔、整数、浮点
 *  0x2: 数组
 *  0x3: 字符串
 *  0x4: 原始数据
 *  0x5: Trace信息
 *  0x6: 结构体
 *  0x7: 编码方式
 *===========================================================================*/

#define DLT_TYPE_INFO_BOOL   0x0010
#define DLT_TYPE_INFO_SINT8  0x0110
#define DLT_TYPE_INFO_UINT8  0x0210
#define DLT_TYPE_INFO_SINT16 0x0310
#define DLT_TYPE_INFO_UINT16 0x0410
#define DLT_TYPE_INFO_SINT32 0x0510
#define DLT_TYPE_INFO_UINT32 0x0610
#define DLT_TYPE_INFO_SINT64 0x0710
#define DLT_TYPE_INFO_UINT64 0x0810
#define DLT_TYPE_INFO_FLOAT32 0x0910
#define DLT_TYPE_INFO_FLOAT64 0x0A10

#define DLT_TYPE_INFO_ARRAY  0x0B20
#define DLT_TYPE_INFO_STRING 0x0C30
#define DLT_TYPE_INFO_RAW    0x0D40
#define DLT_TYPE_INFO_TRACE  0x0E50
#define DLT_TYPE_INFO_STRUCT 0x0F60
#define DLT_TYPE_INFO_CODING 0x1070

/*===========================================================================*/
/* Variable Info定义（可选字段）                                      */
/*===========================================================================*/

#define DLT_MSIN_VERS 0x01  /* Version信息存在 */
#define DLT_MSIN_MTIN 0x02  /* Message Type信息存在 */

/*===========================================================================*/
/* 完整的控制消息服务ID列表                                         */
/*===========================================================================*/

typedef enum {
    /* 标准控制服务 */
    DLT_SERVICE_ID_SET_LOG_LEVEL = 0x01,
    DLT_SERVICE_ID_SET_TRACE_STATUS = 0x02,
    DLT_SERVICE_ID_GET_LOG_INFO = 0x03,
    DLT_SERVICE_ID_GET_DEFAULT_LOG_LEVEL = 0x04,
    DLT_SERVICE_ID_STORE_CONFIG = 0x05,
    DLT_SERVICE_ID_RESET_TO_FACTORY_DEFAULT = 0x06,
    DLT_SERVICE_ID_SET_COM_INTERFACE_STATUS = 0x07,
    DLT_SERVICE_ID_SET_COM_INTERFACE_MAX_BANDWIDTH = 0x08,
    DLT_SERVICE_ID_SET_VERBOSE_MODE = 0x09,
    DLT_SERVICE_ID_SET_MESSAGE_FILTERING = 0x0A,
    DLT_SERVICE_ID_SET_TIMING_PACKETS = 0x0B,
    DLT_SERVICE_ID_GET_LOCAL_TIME = 0x0C,
    DLT_SERVICE_ID_USE_ECU_ID = 0x0D,
    DLT_SERVICE_ID_USE_SESSION_ID = 0x0E,
    DLT_SERVICE_ID_USE_TIMESTAMP = 0x0F,
    DLT_SERVICE_ID_USE_EXTENDED_HEADER = 0x10,
    DLT_SERVICE_ID_SET_DEFAULT_LOG_LEVEL = 0x11,
    DLT_SERVICE_ID_SET_DEFAULT_TRACE_STATUS = 0x12,
    DLT_SERVICE_ID_GET_SOFTWARE_VERSION = 0x13,
    DLT_SERVICE_ID_MESSAGE_BUFFER_OVERFLOW = 0x14,
    
    /* 自定义服务 */
    DLT_SERVICE_ID_USER = 0x1000
} Dlt_ServiceIdType;

/*===========================================================================*/
/* 控制消息响应码                                                     */
/*===========================================================================*/

typedef enum {
    DLT_RESPONSE_OK = 0x00,
    DLT_RESPONSE_ERROR = 0x01,
    DLT_RESPONSE_NOT_SUPPORTED = 0x02,
    DLT_RESPONSE_PARAMETER_ERROR = 0x03
} Dlt_ControlResponseType;

/*===========================================================================*/
/* 高级API: 完整Payload构建                                            */
/*===========================================================================*/

typedef struct {
    uint8_t *buffer;
    uint16_t size;
    uint16_t position;
    uint16_t remaining;
} Dlt_PayloadBuilderType;

/**
 * @brief 初始化Payload构建器
 */
void Dlt_PayloadBuilder_Init(Dlt_PayloadBuilderType *builder, 
                              uint8_t *buffer, 
                              uint16_t size);

/**
 * @brief 添加标准头到Payload
 */
Dlt_ReturnType Dlt_PayloadBuilder_AddHeader(Dlt_PayloadBuilderType *builder,
                                             Dlt_MessageTypeType msg_type,
                                             uint8_t specific_info);

/**
 * @brief 添加变量到Payload
 */
Dlt_ReturnType Dlt_PayloadBuilder_AddVariable(Dlt_PayloadBuilderType *builder,
                                               Dlt_PayloadDataType data_type,
                                               const void *value,
                                               uint16_t length);

/**
 * @brief 添加字符串到Payload
 */
Dlt_ReturnType Dlt_PayloadBuilder_AddString(Dlt_PayloadBuilderType *builder,
                                             Dlt_StringCodingType coding,
                                             const char *str);

/**
 * @brief 添加原始数据到Payload
 */
Dlt_ReturnType Dlt_PayloadBuilder_AddRawData(Dlt_PayloadBuilderType *builder,
                                              const uint8_t *data,
                                              uint16_t length);

/**
 * @brief 获取当前Payload长度
 */
static inline uint16_t Dlt_PayloadBuilder_GetLength(const Dlt_PayloadBuilderType *builder) {
    return builder->position;
}

#ifdef __cplusplus
}
#endif

#endif /* DLT_PAYLOAD_H */
