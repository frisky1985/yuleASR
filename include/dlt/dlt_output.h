/**
 * @file dlt_output.h
 * @brief DLT输出通道管理
 * 
 * 实现AutoSAR DLT规范的多通道输出功能
 * 支持: UDP, TCP, Serial, File
 */

#ifndef DLT_OUTPUT_H
#define DLT_OUTPUT_H

#include "dlt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* 输出通道类型                                                            */
/*===========================================================================*/
typedef enum {
    DLT_OUTPUT_NONE = 0,
    DLT_OUTPUT_UDP,
    DLT_OUTPUT_TCP,
    DLT_OUTPUT_SERIAL,
    DLT_OUTPUT_FILE,
    DLT_OUTPUT_CALLBACK,
    DLT_OUTPUT_COUNT
} Dlt_OutputChannelType;

/*===========================================================================*/
/* 输出回调函数类型                                                        */
/*===========================================================================*/
typedef void (*Dlt_OutputCallback_t)(const uint8_t *data, uint16_t length, void *user_data);

/*===========================================================================*/
/* UDP配置                                                              */
/*===========================================================================*/
typedef struct {
    char remote_address[64];
    uint16_t remote_port;
    char local_address[64];
    uint16_t local_port;
    bool use_multicast;
    uint8_t multicast_ttl;
    int socket_fd;  /* 平台相关，实际为int类型 */
} Dlt_UdpConfigType;

/*===========================================================================*/
/* TCP配置                                                              */
/*===========================================================================*/
typedef struct {
    char server_address[64];
    uint16_t server_port;
    bool auto_reconnect;
    uint16_t reconnect_interval_ms;
    int socket_fd;
    bool connected;
} Dlt_TcpConfigType;

/*===========================================================================*/
/* 串口配置                                                            */
/*===========================================================================*/
typedef struct {
    char port_name[32];
    uint32_t baud_rate;
    uint8_t data_bits;
    uint8_t stop_bits;
    uint8_t parity;
    void *handle;
} Dlt_SerialConfigType;

/*===========================================================================*/
/* 文件配置                                                            */
/*===========================================================================*/
typedef struct {
    char file_path[256];
    uint32_t max_file_size;
    uint8_t max_file_count;
    bool append_mode;
    void *file_handle;
    uint32_t current_size;
    uint8_t current_index;
} Dlt_FileConfigType;

/*===========================================================================*/
/* 回调配置                                                            */
/*===========================================================================*/
typedef struct {
    Dlt_OutputCallback_t callback;
    void *user_data;
} Dlt_CallbackConfigType;

/*===========================================================================*/
/* 输出通道配置联合体                                                      */
/*===========================================================================*/
typedef union {
    Dlt_UdpConfigType udp;
    Dlt_TcpConfigType tcp;
    Dlt_SerialConfigType serial;
    Dlt_FileConfigType file;
    Dlt_CallbackConfigType callback;
} Dlt_OutputChannelConfigUnion;

/*===========================================================================*/
/* 输出通道配置                                                         */
/*===========================================================================*/
typedef struct {
    Dlt_OutputChannelType type;
    bool enabled;
    uint8_t priority;
    Dlt_OutputChannelConfigUnion config;
} Dlt_OutputChannelConfigType;

/*===========================================================================*/
/* 输出管理器配置                                                        */
/*===========================================================================*/
#define DLT_MAX_OUTPUT_CHANNELS 4

typedef struct {
    Dlt_OutputChannelConfigType channels[DLT_MAX_OUTPUT_CHANNELS];
    uint8_t channel_count;
    bool async_mode;
    uint16_t flush_interval_ms;
} Dlt_OutputManagerConfigType;

/*===========================================================================*/
/* API函数声明                                                          */
/*===========================================================================*/

/**
 * @brief 初始化输出管理器
 */
Dlt_ReturnType Dlt_Output_Init(const Dlt_OutputManagerConfigType *config);

/**
 * @brief 反初始化输出管理器
 */
void Dlt_Output_DeInit(void);

/**
 * @brief 发送数据到所有使能的输出通道
 */
Dlt_ReturnType Dlt_Output_Send(const uint8_t *data, uint16_t length);

/**
 * @brief 刷新所有输出通道
 */
Dlt_ReturnType Dlt_Output_Flush(void);

/**
 * @brief 添加输出通道
 */
Dlt_ReturnType Dlt_Output_AddChannel(const Dlt_OutputChannelConfigType *channel);

/**
 * @brief 移除输出通道
 */
Dlt_ReturnType Dlt_Output_RemoveChannel(Dlt_OutputChannelType type);

/**
 * @brief 使能/禁用输出通道
 */
Dlt_ReturnType Dlt_Output_EnableChannel(Dlt_OutputChannelType type, bool enable);

/**
 * @brief 获取输出通道状态
 */
bool Dlt_Output_IsChannelEnabled(Dlt_OutputChannelType type);

/**
 * @brief 获取输出统计信息
 */
typedef struct {
    uint32_t bytes_sent[DLT_OUTPUT_COUNT];
    uint32_t bytes_dropped[DLT_OUTPUT_COUNT];
    uint32_t errors[DLT_OUTPUT_COUNT];
    uint32_t reconnects;
} Dlt_OutputStatisticsType;

const Dlt_OutputStatisticsType* Dlt_Output_GetStatistics(void);
void Dlt_Output_ResetStatistics(void);

/*===========================================================================*/
/* 各通道特定API                                                         */
/*===========================================================================*/

/* UDP通道 */
Dlt_ReturnType Dlt_Output_UdpInit(const Dlt_UdpConfigType *config);
Dlt_ReturnType Dlt_Output_UdpSend(const uint8_t *data, uint16_t length);
void Dlt_Output_UdpDeInit(void);

/* TCP通道 */
Dlt_ReturnType Dlt_Output_TcpInit(const Dlt_TcpConfigType *config);
Dlt_ReturnType Dlt_Output_TcpSend(const uint8_t *data, uint16_t length);
bool Dlt_Output_TcpIsConnected(void);
void Dlt_Output_TcpDeInit(void);

/* 串口通道 */
Dlt_ReturnType Dlt_Output_SerialInit(const Dlt_SerialConfigType *config);
Dlt_ReturnType Dlt_Output_SerialSend(const uint8_t *data, uint16_t length);
void Dlt_Output_SerialDeInit(void);

/* 文件通道 */
Dlt_ReturnType Dlt_Output_FileInit(const Dlt_FileConfigType *config);
Dlt_ReturnType Dlt_Output_FileSend(const uint8_t *data, uint16_t length);
Dlt_ReturnType Dlt_Output_FileRotate(void);
void Dlt_Output_FileDeInit(void);

#ifdef __cplusplus
}
#endif

#endif /* DLT_OUTPUT_H */
