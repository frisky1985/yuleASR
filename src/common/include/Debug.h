/*******************************************************************************
 * @file Debug.h
 * @brief 调试输出抽象层 (MISRA C:2012 20.9 合规)
 *
 * 所有生产代码的 printf/snprintf 调用必须通过本抽象层。
 * 仅 Debug.c 内部允许包含 <stdio.h>（附 MISRA 20.9 deviation 说明）。
 *
 * 编译开关：
 *   -DDEBUG_ENABLE=1  : Debug_Printf 输出到 stdout（调试构建）
 *   -DDEBUG_ENABLE=0  : Debug_Printf 为空操作（量产构建，零开销）
 *   Debug_Snprintf 始终可用（用于缓冲区格式化）。
 ******************************************************************************/

#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 格式化输出到缓冲区（替代 snprintf）。
 *        行为与 C99 snprintf 一致：最多写入 size-1 字节并以 '\0' 结尾。
 * @param buf  目标缓冲区
 * @param size 缓冲区大小
 * @param fmt  格式串
 * @return 若成功返回应写入的字符数（不含 '\0'）；出错返回负值。
 */
int Debug_Snprintf(char* buf, size_t size, const char* fmt, ...);

/**
 * @brief 变参版本（替代 vsnprintf）。
 */
int Debug_Vsnprintf(char* buf, size_t size, const char* fmt, va_list args);

/**
 * @brief 格式化输出到流（替代 fprintf）。stream 为底层文件流指针（FILE*）。
 *        用于日志文件写入等合法场景。
 */
int Debug_Fprintf(void* stream, const char* fmt, ...);

#if defined(DEBUG_ENABLE) && (DEBUG_ENABLE == 1)
/**
 * @brief 格式化输出到 stdout（替代 printf）。仅调试构建可用。
 */
void Debug_Printf(const char* fmt, ...);
#else
/* 量产构建：Debug_Printf 编译为空操作，零运行时开销 */
#define Debug_Printf(...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_H */
