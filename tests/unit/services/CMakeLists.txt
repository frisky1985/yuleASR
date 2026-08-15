# Services Layer Unit Tests CMakeLists.txt
#
# 2026-08-15: DLT 单测接入 (ecual context 配置体系合并)
# - 编译真实生产源码 Dlt.c + Dlt_Lcfg.c + 测试文件
# - Det_ReportError 由测试文件内 mock
# - 覆盖: 初始化生命周期 / 应用注册 / 日志与跟踪消息 / 过滤器 /
#          队列操作 / 链接期 context 配置表

cmake_minimum_required(VERSION 3.16)

set(DLT_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_Dlt.c)
set(DLT_PRODUCT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/dlt/src/Dlt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/dlt/src/Dlt_Lcfg.c
)

add_executable(Dlt_UnitTest
    ${DLT_TEST_SRC}
    ${DLT_PRODUCT_SRC}
)

target_include_directories(Dlt_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..                      # test_framework.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/dlt/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(Dlt_UnitTest PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(Dlt_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(Dlt_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME Dlt_UnitTest COMMAND Dlt_UnitTest)

message(STATUS "  Unit test: Dlt_UnitTest (ecual context 配置合并)")
