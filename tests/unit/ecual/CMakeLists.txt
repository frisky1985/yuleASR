# EthSwt Unit Tests CMakeLists.txt
#
# B2 deep-dive (2026-08-09): compiles the real production source
# EthSwt.c with a Det_ReportError mock, covering init lifecycle, port
# config/query, VLAN member table + ingress/egress filtering + PVID +
# VID-PCP mapping, flow control (pause frames, high/low watermarks),
# per-port statistics (GetStatistics/ResetStatistics) and port mirroring.
#
# Use: cmake -DBUILD_TESTING=ON .. && make EthSwt_UnitTest && ctest -R EthSwt

cmake_minimum_required(VERSION 3.16)

set(ETHSWT_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_ethswt.c)
set(ETHSWT_PRODUCT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/ethswt/src/EthSwt.c)

add_executable(EthSwt_UnitTest
    ${ETHSWT_TEST_SRC}
    ${ETHSWT_PRODUCT_SRC}
)

target_include_directories(EthSwt_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/..                       # test_framework.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/ethswt/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(EthSwt_UnitTest PRIVATE
    -Wall
    -Wextra
    -Werror
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(EthSwt_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(EthSwt_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME EthSwt_UnitTest COMMAND EthSwt_UnitTest)

message(STATUS "  Unit test: EthSwt_UnitTest (B2 VLAN/流控/统计/镜像)")
