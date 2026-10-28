# =============================================================================
# tests/unit/ddsudp/CMakeLists.txt
# microdds UDP 传输层 (src/transport/udp.c) 单元测试
#
# udp.c 在 macOS/Linux 上走 MICRODDS_PLATFORM_POSIX 路径, 测试直接编译真实
# 产品源码, 通过 127.0.0.1 回环 socket 实测收发往返 (非桩)。
#
# NOTE: udp.c 未包含定义 NULL_PTR 的任何产品头, 单独编译会失败; 测试构建以
#       compile definition 注入该宏, 不改动产品代码。
# =============================================================================
set(DDSUDP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDSUDP_FRAMEWORK ${CMAKE_SOURCE_DIR}/tests/unit/framework)
set(DDSUDP_MDDS ${CMAKE_SOURCE_DIR}/src/middleware/microdds)

add_executable(test_microdds_udp
    ${DDSUDP_DIR}/test_microdds_udp.c
    ${DDSUDP_MDDS}/src/transport/udp.c
    ${DDSUDP_FRAMEWORK}/unity.c
)

target_compile_options(test_microdds_udp PRIVATE
    -Wall -Wextra -g -O0
    -Wno-unused-parameter -Wno-unused-function
)

# udp.c 直接使用 NULL_PTR 但其 include 链中无人定义 → 测试侧注入
# (函数式宏需走 compile options, target_compile_definitions 会拆掉括号)
target_compile_options(test_microdds_udp PRIVATE "-DNULL_PTR=((void*)0)")
# udp.c 在定义前调用 MicroDDS_UDP_AddPeer (无前置声明, C99 编译错误) →
# -include 注入前置声明垫片 (不改产品代码, 同 dds1 目录先例)
target_compile_options(test_microdds_udp PRIVATE
    -include ${DDSUDP_DIR}/ddsudp_shim.h)

target_include_directories(test_microdds_udp PRIVATE
    ${DDSUDP_FRAMEWORK}
    ${DDSUDP_MDDS}/include
)

set_target_properties(test_microdds_udp PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME microdds_udp COMMAND test_microdds_udp)
message(STATUS "  Unit test: test_microdds_udp (microdds UDP transport, loopback sockets)")
