# Copyright (c) 2024 Ingeek
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

# swc_test executable
# Tests the Swc (SW Component) service
#
# SUT:    src/bsw/services/swc (target service_swc)
# Runner: handwritten main() in test_swc.c
#
# Note: test_swc.c defines its own Det_ReportError recording mock,
# so tests/mocks/mock_det.c is NOT linked.

add_executable(swc_test
    test_swc.c
)

target_compile_options(swc_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(swc_test PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

target_include_directories(swc_test PRIVATE
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_SOURCE_DIR}/src/bsw/services/swc/include
    ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
    ${CMAKE_SOURCE_DIR}/src/bsw/os/include
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/config/input/services
)

if(TARGET service_swc)
    target_link_libraries(swc_test PRIVATE service_swc)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(swc_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME swc_test COMMAND swc_test)
