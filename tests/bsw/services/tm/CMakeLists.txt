# Copyright (c) 2024 Ingeek
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

# tm_test executable
# Tests the Tm (Time Manager) service
#
# SUT:    src/bsw/services/tm (target service_tm)
# Runner: handwritten main() in test_tm.c
#
# Note: test_tm.c defines its own Det_ReportError recording mock,
# so tests/mocks/mock_det.c is NOT linked.

add_executable(tm_test
    test_tm.c
)

target_compile_options(tm_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(tm_test PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

target_include_directories(tm_test PRIVATE
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_SOURCE_DIR}/src/bsw/services/tm/include
    ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
    ${CMAKE_SOURCE_DIR}/src/bsw/os/include
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/config/input/services
)

if(TARGET service_tm)
    target_link_libraries(tm_test PRIVATE service_tm)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(tm_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME tm_test COMMAND tm_test)
