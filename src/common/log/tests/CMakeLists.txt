# src/common/log/tests/CMakeLists.txt
# dds_log 模块宿主机单元测试 (P3 覆盖率提升)
#
# test_dds_log_expose.c 通过 #define static + #include 暴露 dds_log.c 内部符号,
# 使测试代码能访问 g_log_state 等 static 状态变量用于覆盖深路径 (audit_mask 等)。
# 不链接 eth_dds_common — 避免 dds_log.c 双重编译导致符号冲突。

cmake_minimum_required(VERSION 3.16)

find_package(Threads REQUIRED)

add_executable(test_dds_log
    test_dds_log.c
    dds_log_expose.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../utils/eth_utils.c
)

target_include_directories(test_dds_log PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..           # dds_log.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../types     # eth_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils     # eth_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include    # Debug.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../..         # autosar_types.h 等
)

target_link_libraries(test_dds_log PRIVATE
    Threads::Threads
    m
)

add_test(NAME dds_log COMMAND test_dds_log)
