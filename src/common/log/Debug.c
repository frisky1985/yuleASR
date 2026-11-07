/*******************************************************************************
 * @file Debug.c
 * @brief 调试输出抽象层实现
 *
 * MISRA C:2012 Rule 20.9 deviation:
 *   本文件是整个工程中唯一允许包含 <stdio.h> 的生产代码文件。
 *   所有其他模块必须通过 Debug.h 的 Debug_Printf/Debug_Snprintf 间接使用，
 *   以满足 "stdio.h 不应在生产代码中使用" 的要求。
 *   此处使用 snprintf/printf 仅用于调试输出与缓冲区格式化，不涉及文件 I/O
 *   或动态内存，且输入均为内部可控数据，无格式化字符串注入风险。
 ******************************************************************************/

#include "Debug.h"

/* cppcheck-suppress misra-c2012-20.9 */
#include <stdio.h>

/* cppcheck-suppress misra-c2012-20.9 */
int Debug_Snprintf(char* buf, size_t size, const char* fmt, ...)
{
    va_list args;
    int ret;
    va_start(args, fmt);
    ret = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return ret;
}

/* cppcheck-suppress misra-c2012-20.9 */
int Debug_Vsnprintf(char* buf, size_t size, const char* fmt, va_list args)
{
    return vsnprintf(buf, size, fmt, args);
}

/* cppcheck-suppress misra-c2012-20.9 */
int Debug_Fprintf(void* stream, const char* fmt, ...)
{
    va_list args;
    int ret;
    va_start(args, fmt);
    ret = vfprintf((FILE*)stream, fmt, args);
    va_end(args);
    return ret;
}

#if defined(DEBUG_ENABLE) && (DEBUG_ENABLE == 1)
/* cppcheck-suppress misra-c2012-20.9 */
void Debug_Printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    (void)vprintf(fmt, args);
    va_end(args);
}
#endif
