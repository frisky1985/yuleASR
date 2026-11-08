# src/common/log/tests/CMakeLists.txt
# dds_log 模块宿主机单元测试 (P3 覆盖率提升)

cmake_minimum_required(VERSION 3.16)

find_package(Threads REQUIRED)

add_executable(test_dds_log
    test_dds_log.c
)

target_include_directories(test_dds_log PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_dds_log PRIVATE
    eth_dds_common
    Threads::Threads
)

add_test(NAME dds_log COMMAND test_dds_log)
