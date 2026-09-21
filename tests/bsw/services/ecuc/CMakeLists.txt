# EcuC unit tests — substantiated against the production EcuC implementation.
# The test file defines its own Det_ReportError mock, so tests/mocks/mock_det.c
# must NOT be linked (duplicate symbol).

cmake_minimum_required(VERSION 3.16)

add_executable(ecuc_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ecuc.c
)

target_include_directories(ecuc_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/ecuC/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/bsw/os/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/config/input/services
)

target_compile_options(ecuc_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(ecuc_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_ecuC)
    target_link_libraries(ecuc_test PRIVATE service_ecuC)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ecuc_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ecuc_test COMMAND ecuc_test)

message(STATUS "  Unit test: ecuc_test (ECU Configuration service)")
