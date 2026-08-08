# TcpIp Unit Tests CMakeLists.txt
#
# B1 deep-dive (2026-08-09): compiles the real production source
# TcpIp.c with a Det_ReportError mock, covering init lifecycle,
# socket lifecycle, connection state machine, multi-connection,
# zero-copy RX/TX buffers, VLAN and statistics.
#
# Use: cmake -DBUILD_TESTING=ON .. && make TcpIp_UnitTest && ctest -R TcpIp

cmake_minimum_required(VERSION 3.16)

set(TCPIP_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_tcpip.c)
set(TCPIP_PRODUCT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/bsw/services/tcpip/src/TcpIp.c)

add_executable(TcpIp_UnitTest
    ${TCPIP_TEST_SRC}
    ${TCPIP_PRODUCT_SRC}
)

target_include_directories(TcpIp_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..                      # test_framework.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/bsw/services/tcpip/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/autosar
)

target_compile_options(TcpIp_UnitTest PRIVATE
    -Wall
    -Wextra
    -Werror
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(TcpIp_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(TcpIp_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME TcpIp_UnitTest COMMAND TcpIp_UnitTest)

message(STATUS "  Unit test: TcpIp_UnitTest (B1 API/多连接/VLAN/统计)")

# ── lwIP bridge compile check (B1) ─────────────────────────────────────
# Compiles TcpIp.c with TCPIP_ENABLE_LWIP=STD_ON against the vendored
# lwIP 2.2.2 headers (unix port lwipopts/cc.h).  Verifies the guarded
# lwIP path stays API-compatible without linking a full lwIP port.
add_library(tcpip_lwip_compile_check OBJECT
    ${TCPIP_PRODUCT_SRC}
)

target_compile_definitions(tcpip_lwip_compile_check PRIVATE
    TCPIP_ENABLE_LWIP=STD_ON
)

target_include_directories(tcpip_lwip_compile_check PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/bsw/services/tcpip/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/bsw/services/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/bsw/os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/autosar
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party/lwip/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party/lwip/contrib/ports/unix/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party/lwip/contrib/ports/unix/port/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party/lwip/src/include/posix
)

target_compile_options(tcpip_lwip_compile_check PRIVATE
    -Wall
    -Wextra
    -Werror
    -std=c99
)

set_target_properties(tcpip_lwip_compile_check PROPERTIES
    C_STANDARD 99
    EXCLUDE_FROM_ALL TRUE
)

message(STATUS "  Compile check: tcpip_lwip_compile_check (lwIP 2.2.2 bridge)")
