# UartSx host-side unit tests
#
# Compiles UartSx.c (UNIT_TEST 宏 + UartSx_Cfg.h) against host stubs for Uart / OS.

cmake_minimum_required(VERSION 3.16)

find_package(Threads REQUIRED)

add_executable(test_uartsx
    test_uartsx.c
    uartsx_test_support.c
    ${CMAKE_SOURCE_DIR}/src/bsw/ecual/uartsx/src/UartSx.c
)

target_compile_definitions(test_uartsx PRIVATE UNIT_TEST)
target_compile_options(test_uartsx PRIVATE -Wall -Wextra -O0 -g)

target_include_directories(test_uartsx PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/uartsx/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/mcal/uart/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/types
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/platform/s32k312/include
)

target_link_libraries(test_uartsx PRIVATE Threads::Threads)

add_test(NAME test_uartsx COMMAND test_uartsx)
