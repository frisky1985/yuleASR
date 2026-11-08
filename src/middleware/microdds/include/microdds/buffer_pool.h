/** @file buffer_pool.h
 * @brief 静态缓冲区池接口
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */
#ifndef MICRODDS_BUFFER_POOL_H
#define MICRODDS_BUFFER_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

bool MicroDDS_BufferPool_Init(void);
void MicroDDS_BufferPool_Shutdown(void);
void* MicroDDS_BufferPool_Alloc(void);
void MicroDDS_BufferPool_Free(void* buffer);
uint16_t MicroDDS_BufferPool_GetBufferSize(void);
uint32_t MicroDDS_BufferPool_GetAvailableCount(void);

#ifdef __cplusplus
}
#endif

#endif /* MICRODDS_BUFFER_POOL_H */
