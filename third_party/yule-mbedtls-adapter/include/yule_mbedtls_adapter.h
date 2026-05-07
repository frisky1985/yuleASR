/**
 * @file yule_mbedtls_adapter.h
 * @brief YuleTech AUTOSAR mbedTLS Platform Adapter
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 提供mbedTLS与AUTOSAR BSW的集成:
 * - 内存分配适配 (MemMap)
 * - 随机数生成适配
 * - 时间戳适配
 * - 调试输出适配
 */

#ifndef YULE_MBEDTLS_ADAPTER_H
#define YULE_MBEDTLS_ADAPTER_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 配置宏
 *===========================================================================*/

/* mbedTLS内存池大小 */
#define YULE_MBEDTLS_HEAP_SIZE      (32768U)  /* 32KB 静态堆 */

/* 随机数缓冲区大小 */
#define YULE_MBEDTLS_ENTROPY_LEN    (48U)

/*============================================================================
 * 初始化和反初始化
 *===========================================================================*/

/**
 * @brief 初始化mbedTLS适配层
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType YuleMbedtls_Init(void);

/**
 * @brief 反初始化mbedTLS适配层
 */
extern void YuleMbedtls_DeInit(void);

/**
 * @brief 获取mbedTLS堆内存使用情况
 * @param totalSize 总大小 (输出)
 * @param usedSize 已使用大小 (输出)
 * @param maxUsedSize 最大使用大小 (输出)
 */
extern void YuleMbedtls_GetHeapStats(uint32* totalSize, 
                                      uint32* usedSize, 
                                      uint32* maxUsedSize);

/*============================================================================
 * 随机数生成器
 *===========================================================================*/

/**
 * @brief 获取硬件随机数
 * @param output 输出缓冲区
 * @param len 需要的长度
 * @return E_OK: 成功, E_NOT_OK: 失败
 * 
 * 优先使用Trng硬件，如果不可用则使用软件随机数
 */
extern Std_ReturnType YuleMbedtls_GetHardwareRandom(uint8* output, uint32 len);

/*============================================================================
 * 时间戳服务
 *===========================================================================*/

/**
 * @brief 获取当前Unix时间戳
 * @return Unix时间戳 (秒)
 * 
 * 通过StbM或Gpt模块获取精确时间
 */
extern uint32 YuleMbedtls_GetUnixTime(void);

/*============================================================================
 * 调试和日志
 *===========================================================================*/

/**
 * @brief 调试输出回调
 * @param level 日志级别 (1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG)
 * @param file 文件名
 * @param line 行号
 * @param msg 消息
 */
extern void YuleMbedtls_DebugCallback(
    int level,
    const char* file,
    int line,
    const char* msg
);

/*============================================================================
 * 错误码转换
 *===========================================================================*/

/**
 * @brief 将mbedTLS错误码转换为MQTT错误码
 * @param mbedtlsError mbedTLS错误码
 * @return MQTT错误码
 */
extern sint32 YuleMbedtls_ConvertError(int mbedtlsError);

#ifdef __cplusplus
}
#endif

#endif /* YULE_MBEDTLS_ADAPTER_H */
