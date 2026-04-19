/**
 * @file Compiler.h
 * @brief 编译器抽象层
 * @version 1.0.0
 * @date 2026-04-13
 * @author YuleTech
 *
 * @copyright Copyright (c) 2026 YuleTech
 *
 * @details 提供编译器特定的宏定义，用于支持不同编译器
 */

#ifndef COMPILER_H
#define COMPILER_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/

/*==================================================================================================
*                                       GCC Compiler
==================================================================================================*/
#if defined(__GNUC__)
    /** @brief 函数内联 */
    #define INLINE              inline

    /** @brief 局部内联函数 */
    #define LOCAL_INLINE        static inline

    /** @brief 强制内联 */
    #define FORCE_INLINE        __attribute__((always_inline)) inline

    /** @brief 从不内联 */
    #define NO_INLINE           __attribute__((noinline))

    /** @brief 中断服务程序 */
    #define INTERRUPT           __attribute__((interrupt))

    /** @brief 弱符号 */
    #define WEAK                __attribute__((weak))

    /** @brief 已使用标记 */
    #define USED                __attribute__((used))

    /** @brief 未使用标记 */
    #define UNUSED              __attribute__((unused))

    /** @brief 对齐 */
    #define ALIGN(n)            __attribute__((aligned(n)))

    /** @brief 打包 */
    #define PACKED              __attribute__((packed))

    /** @brief 段定义 */
    #define SECTION(name)       __attribute__((section(name)))

    /** @brief 中断向量 */
    #define ISR(vector)         void vector(void) __attribute__((interrupt))

/*==================================================================================================
*                                       IAR Compiler
==================================================================================================*/
#elif defined(__ICCARM__)
    #define INLINE              inline
    #define LOCAL_INLINE        static inline
    #define FORCE_INLINE        __attribute__((always_inline)) inline
    #define NO_INLINE           __attribute__((noinline))
    #define INTERRUPT           __interrupt
    #define WEAK                __weak
    #define USED                __root
    #define UNUSED
    #define ALIGN(n)            __attribute__((aligned(n)))
    #define PACKED              __packed
    #define SECTION(name)       __attribute__((section(name)))
    #define ISR(vector)         __interrupt void vector(void)

/*==================================================================================================
*                                       Tasking Compiler
==================================================================================================*/
#elif defined(__TASKING__)
    #define INLINE              inline
    #define LOCAL_INLINE        static inline
    #define FORCE_INLINE        inline
    #define NO_INLINE
    #define INTERRUPT           __interrupt
    #define WEAK                __weak
    #define USED
    #define UNUSED
    #define ALIGN(n)            __align(n)
    #define PACKED              __packed__
    #define SECTION(name)       __section(name)
    #define ISR(vector)         __interrupt void vector(void)

/*==================================================================================================
*                                       Green Hills Compiler
==================================================================================================*/
#elif defined(__ghs__)
    #define INLINE              __inline
    #define LOCAL_INLINE        static __inline
    #define FORCE_INLINE        __attribute__((always_inline)) __inline
    #define NO_INLINE           __attribute__((noinline))
    #define INTERRUPT           __interrupt
    #define WEAK                __attribute__((weak))
    #define USED                __attribute__((used))
    #define UNUSED              __attribute__((unused))
    #define ALIGN(n)            __attribute__((aligned(n)))
    #define PACKED              __attribute__((packed))
    #define SECTION(name)       __attribute__((section(name)))
    #define ISR(vector)         __interrupt void vector(void)

/*==================================================================================================
*                                       Unknown Compiler
==================================================================================================*/
#else
    #error "Unsupported compiler! Please define compiler-specific macros."
#endif

/*==================================================================================================
*                                      MEMORY QUALIFIERS
==================================================================================================*/
/** @brief RAM 数据 */
#define VAR(var)                    var

/** @brief 常量数据 */
#define CONST(const)                const

/** @brief 静态变量 */
#define STATIC                      static

/** @brief 外部声明 */
#define EXTERN                      extern

/*==================================================================================================
*                                      FUNCTION-LIKE MACROS
==================================================================================================*/
/** @brief 获取数组元素个数 */
#define ARRAY_SIZE(arr)             (sizeof(arr) / sizeof((arr)[0]))

/** @brief 获取结构体成员偏移 */
#define OFFSET_OF(type, member)     ((uint32)(&((type *)0)->member))

/** @brief 获取结构体成员大小 */
#define SIZE_OF(type, member)       (sizeof(((type *)0)->member))

/** @brief 最小值 */
#define MIN(a, b)                   (((a) < (b)) ? (a) : (b))

/** @brief 最大值 */
#define MAX(a, b)                   (((a) > (b)) ? (a) : (b))

/** @brief 绝对值 */
#define ABS(x)                      (((x) < 0) ? -(x) : (x))

/** @brief 限制范围 */
#define CLAMP(x, min, max)          (MIN(MAX((x), (min)), (max)))

/*==================================================================================================
*                                      DEBUG SUPPORT
==================================================================================================*/
#ifdef DEBUG
    /** @brief 断言 */
    #define ASSERT(expr)            \
        do {                        \
            if (!(expr)) {          \
                while(1);           \
            }                       \
        } while(0)
#else
    #define ASSERT(expr)            ((void)0)
#endif

/*==================================================================================================
*                                      VERSION INFORMATION
==================================================================================================*/
#define COMPILER_VENDOR_ID                      0x0055U
#define COMPILER_MODULE_ID                      0x00FEU
#define COMPILER_SW_MAJOR_VERSION               1U
#define COMPILER_SW_MINOR_VERSION               0U
#define COMPILER_SW_PATCH_VERSION               0U

#endif /* COMPILER_H */
