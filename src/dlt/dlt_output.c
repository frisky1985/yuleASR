/**
 * @file dlt_output.c
 * @brief DLT输出通道实现
 */

#include "dlt/dlt_output.h"
#include <string.h>

/*===========================================================================*/
/* 内部状态                                                            */
/*===========================================================================*/

static struct {
    Dlt_OutputChannelConfigType channels[DLT_MAX_OUTPUT_CHANNELS];
    Dlt_OutputStatisticsType stats;
    uint8_t channel_count;
    bool initialized;
    uint8_t active_channels;
} g_output_ctx = {0};

/*===========================================================================*/
/* 辅助函数                                                            */
/*===========================================================================*/

static int8_t find_channel_index(Dlt_OutputChannelType type) {
    for (uint8_t i = 0; i < g_output_ctx.channel_count; i++) {
        if (g_output_ctx.channels[i].type == type) {
            return (int8_t)i;
        }
    }
    return -1;
}

static void update_active_count(void) {
    g_output_ctx.active_channels = 0;
    for (uint8_t i = 0; i < g_output_ctx.channel_count; i++) {
        if (g_output_ctx.channels[i].enabled) {
            g_output_ctx.active_channels++;
        }
    }
}

/*===========================================================================*/
/* 通道实现函数 - UDP                                                   */
/*===========================================================================*/

Dlt_ReturnType Dlt_Output_UdpInit(const Dlt_UdpConfigType *config) {
    /* 平台相关：创建socket并配置 */
    (void)config;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Output_UdpSend(const uint8_t *data, uint16_t length) {
    /* 平台相关：sendto系统调用 */
    (void)data;
    (void)length;
    return DLT_RETURN_OK;
}

void Dlt_Output_UdpDeInit(void) {
    /* 平台相关：closesocket */
}

/*===========================================================================*/
/* 通道实现函数 - TCP                                                   */
/*===========================================================================*/

Dlt_ReturnType Dlt_Output_TcpInit(const Dlt_TcpConfigType *config) {
    (void)config;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Output_TcpSend(const uint8_t *data, uint16_t length) {
    (void)data;
    (void)length;
    return DLT_RETURN_OK;
}

bool Dlt_Output_TcpIsConnected(void) {
    return false;
}

void Dlt_Output_TcpDeInit(void) {
}

/*===========================================================================*/
/* 通道实现函数 - Serial                                                */
/*===========================================================================*/

Dlt_ReturnType Dlt_Output_SerialInit(const Dlt_SerialConfigType *config) {
    (void)config;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Output_SerialSend(const uint8_t *data, uint16_t length) {
    (void)data;
    (void)length;
    return DLT_RETURN_OK;
}

void Dlt_Output_SerialDeInit(void) {
}

/*===========================================================================*/
/* 通道实现函数 - File                                                  */
/*===========================================================================*/

Dlt_ReturnType Dlt_Output_FileInit(const Dlt_FileConfigType *config) {
    (void)config;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Output_FileSend(const uint8_t *data, uint16_t length) {
    (void)data;
    (void)length;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Output_FileRotate(void) {
    return DLT_RETURN_OK;
}

void Dlt_Output_FileDeInit(void) {
}

/*===========================================================================*/
/* API实现                                                            */
/*===========================================================================*/

Dlt_ReturnType Dlt_Output_Init(const Dlt_OutputManagerConfigType *config) {
    if (g_output_ctx.initialized) {
        return DLT_RETURN_ERROR;
    }
    
    memset(&g_output_ctx, 0, sizeof(g_output_ctx));
    
    if (config != NULL) {
        g_output_ctx.channel_count = config->channel_count;
        if (g_output_ctx.channel_count > DLT_MAX_OUTPUT_CHANNELS) {
            g_output_ctx.channel_count = DLT_MAX_OUTPUT_CHANNELS;
        }
        
        for (uint8_t i = 0; i < g_output_ctx.channel_count; i++) {
            memcpy(&g_output_ctx.channels[i], &config->channels[i], 
                   sizeof(Dlt_OutputChannelConfigType));
            
            /* 初始化各通道 */
            if (g_output_ctx.channels[i].enabled) {
                switch (g_output_ctx.channels[i].type) {
                    case DLT_OUTPUT_UDP:
                        Dlt_Output_UdpInit(&g_output_ctx.channels[i].config.udp);
                        break;
                    case DLT_OUTPUT_TCP:
                        Dlt_Output_TcpInit(&g_output_ctx.channels[i].config.tcp);
                        break;
                    case DLT_OUTPUT_SERIAL:
                        Dlt_Output_SerialInit(&g_output_ctx.channels[i].config.serial);
                        break;
                    case DLT_OUTPUT_FILE:
                        Dlt_Output_FileInit(&g_output_ctx.channels[i].config.file);
                        break;
                    default:
                        break;
                }
            }
        }
        
        update_active_count();
    }
    
    g_output_ctx.initialized = true;
    return DLT_RETURN_OK;
}

void Dlt_Output_DeInit(void) {
    if (!g_output_ctx.initialized) {
        return;
    }
    
    for (uint8_t i = 0; i < g_output_ctx.channel_count; i++) {
        if (g_output_ctx.channels[i].enabled) {
            switch (g_output_ctx.channels[i].type) {
                case DLT_OUTPUT_UDP:
                    Dlt_Output_UdpDeInit();
                    break;
                case DLT_OUTPUT_TCP:
                    Dlt_Output_TcpDeInit();
                    break;
                case DLT_OUTPUT_SERIAL:
                    Dlt_Output_SerialDeInit();
                    break;
                case DLT_OUTPUT_FILE:
                    Dlt_Output_FileDeInit();
                    break;
                default:
                    break;
            }
        }
    }
    
    memset(&g_output_ctx, 0, sizeof(g_output_ctx));
}

Dlt_ReturnType Dlt_Output_Send(const uint8_t *data, uint16_t length) {
    if (!g_output_ctx.initialized || data == NULL || length == 0) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    if (g_output_ctx.active_channels == 0) {
        return DLT_RETURN_ERROR;
    }
    
    Dlt_ReturnType result = DLT_RETURN_ERROR;
    
    for (uint8_t i = 0; i < g_output_ctx.channel_count; i++) {
        if (!g_output_ctx.channels[i].enabled) {
            continue;
        }
        
        Dlt_ReturnType channel_result = DLT_RETURN_ERROR;
        
        switch (g_output_ctx.channels[i].type) {
            case DLT_OUTPUT_UDP:
                channel_result = Dlt_Output_UdpSend(data, length);
                break;
            case DLT_OUTPUT_TCP:
                if (Dlt_Output_TcpIsConnected()) {
                    channel_result = Dlt_Output_TcpSend(data, length);
                }
                break;
            case DLT_OUTPUT_SERIAL:
                channel_result = Dlt_Output_SerialSend(data, length);
                break;
            case DLT_OUTPUT_FILE:
                channel_result = Dlt_Output_FileSend(data, length);
                break;
            case DLT_OUTPUT_CALLBACK:
                if (g_output_ctx.channels[i].config.callback.callback != NULL) {
                    g_output_ctx.channels[i].config.callback.callback(
                        data, length, 
                        g_output_ctx.channels[i].config.callback.user_data);
                    channel_result = DLT_RETURN_OK;
                }
                break;
            default:
                break;
        }
        
        if (channel_result == DLT_RETURN_OK) {
            g_output_ctx.stats.bytes_sent[g_output_ctx.channels[i].type] += length;
            result = DLT_RETURN_OK;
        } else {
            g_output_ctx.stats.errors[g_output_ctx.channels[i].type]++;
        }
    }
    
    return result;
}

Dlt_ReturnType Dlt_Output_Flush(void) {
    if (!g_output_ctx.initialized) {
        return DLT_RETURN_ERROR;
    }
    
    /* 刷新文件通道 */
    for (uint8_t i = 0; i < g_output_ctx.channel_count; i++) {
        if (g_output_ctx.channels[i].enabled && 
            g_output_ctx.channels[i].type == DLT_OUTPUT_FILE) {
            /* 平台相关：fflush */
        }
    }
    
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Output_AddChannel(const Dlt_OutputChannelConfigType *channel) {
    if (!g_output_ctx.initialized || channel == NULL) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    if (g_output_ctx.channel_count >= DLT_MAX_OUTPUT_CHANNELS) {
        return DLT_RETURN_ERROR;
    }
    
    if (find_channel_index(channel->type) >= 0) {
        return DLT_RETURN_ERROR;
    }
    
    uint8_t idx = g_output_ctx.channel_count++;
    memcpy(&g_output_ctx.channels[idx], channel, sizeof(Dlt_OutputChannelConfigType));
    
    update_active_count();
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Output_RemoveChannel(Dlt_OutputChannelType type) {
    if (!g_output_ctx.initialized) {
        return DLT_RETURN_ERROR;
    }
    
    int8_t idx = find_channel_index(type);
    if (idx < 0) {
        return DLT_RETURN_ERROR;
    }
    
    /* 移除通道 */
    for (uint8_t i = (uint8_t)idx; i < g_output_ctx.channel_count - 1; i++) {
        g_output_ctx.channels[i] = g_output_ctx.channels[i + 1];
    }
    
    g_output_ctx.channel_count--;
    update_active_count();
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Output_EnableChannel(Dlt_OutputChannelType type, bool enable) {
    if (!g_output_ctx.initialized) {
        return DLT_RETURN_ERROR;
    }
    
    int8_t idx = find_channel_index(type);
    if (idx < 0) {
        return DLT_RETURN_ERROR;
    }
    
    g_output_ctx.channels[idx].enabled = enable;
    update_active_count();
    return DLT_RETURN_OK;
}

bool Dlt_Output_IsChannelEnabled(Dlt_OutputChannelType type) {
    if (!g_output_ctx.initialized) {
        return false;
    }
    
    int8_t idx = find_channel_index(type);
    if (idx < 0) {
        return false;
    }
    
    return g_output_ctx.channels[idx].enabled;
}

const Dlt_OutputStatisticsType* Dlt_Output_GetStatistics(void) {
    return &g_output_ctx.stats;
}

void Dlt_Output_ResetStatistics(void) {
    memset(&g_output_ctx.stats, 0, sizeof(Dlt_OutputStatisticsType));
}
