/** @file ddscore_shim.h
 * @brief ddscore 测试垫片: 补齐 microdds 自带测试缺失的驱动宏
 *
 * microdds 自带单元测试 (src/middleware/microdds/tests/unity/) 使用的
 * UNITY_BEGIN / UNITY_END / RUN_TEST / TEST_PASS 宏在其自带 unity.h 中
 * 没有定义 (该头只有 UnityBegin/UnityEnd/UnityRunTest 函数声明), 单独
 * 编译必然失败。本垫片通过 -include 前置注入补齐这 4 个宏, 不改动
 * microdds 的测试文件与框架文件。
 *
 * 另外 src/utils/buffer_pool.c 使用 NULL_PTR 但其 include 链
 * (stdint/stdbool/string) 无人定义该宏 — 与 udp.c 同类编译缺陷, 一并
 * 在此注入 (同 tests/unit/ddsudp 先例)。
 */
#ifndef DDSCORE_SHIM_H
#define DDSCORE_SHIM_H

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

#ifndef UNITY_BEGIN
#define UNITY_BEGIN()       UnityBegin()
#endif

#ifndef UNITY_END
#define UNITY_END()         UnityEnd()
#endif

#ifndef RUN_TEST
#define RUN_TEST(func)      UnityRunTest(func, #func, 0)
#endif

/* test_buffer_pool.c 使用 TEST_PASS() 标记隐式通过; 与
 * tests/unit/test_framework.h 的语义一致, 作为空操作即可。 */
#ifndef TEST_PASS
#define TEST_PASS()         do { } while (0)
#endif

/* test_writer.c (及 examples/hello_world/main.c) 使用 DDS_HANDLE_NIL, 但
 * 产品 types.h 只定义了 MICRODDS_HANDLE_NIL (0U) — 命名不一致的产品缺陷,
 * 此处映射到产品定义 (值语义完全一致)。 */
#ifndef DDS_HANDLE_NIL
#define DDS_HANDLE_NIL      MICRODDS_HANDLE_NIL
#endif

#endif /* DDSCORE_SHIM_H */
