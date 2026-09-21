# Copyright (c) 2024 Ingeek
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

# stbm_test executable
# Tests the StbM (Synchronized Time Base Manager) service
#
# SUT:    src/bsw/services/stbm (target service_stbm)
# Runner: handwritten main() in test_stbm.c
#
# Note: test_stbm.c defines its own Det_ReportError recording mock,
# so tests/mocks/mock_det.c is NOT linked.

add_executable(stbm_test
    test_stbm.c
)

target_compile_options(stbm_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(stbm_test PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

target_include_directories(stbm_test PRIVATE
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_SOURCE_DIR}/src/bsw/services/stbm/include
    ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
    ${CMAKE_SOURCE_DIR}/src/bsw/os/include
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/src/bsw/mcal/eth/include
    ${CMAKE_SOURCE_DIR}/config/input/services
)

if(TARGET service_stbm)
    target_link_libraries(stbm_test PRIVATE service_stbm)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(stbm_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME stbm_test COMMAND stbm_test)
