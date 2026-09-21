# LinTrcv unit tests — substantiated against the production LinTrcv implementation
# (src/bsw/ecual/lintrcv). Links the real ecual_lintrcv library; the external
# dependencies (Det_ReportError, Dio_Read/WriteChannel, EcuM_SetWakeupEvent)
# are provided by the test source itself as host-safe mocks, so no mocks from
# tests/mocks are linked (the test defines its own Det_ReportError inline).

cmake_minimum_required(VERSION 3.16)

add_executable(lintrcv_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_lintrcv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_lintrcv_runner.c
)

target_include_directories(lintrcv_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/lintrcv/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/dio/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/ecual
        ${CMAKE_SOURCE_DIR}/config/input/mcal
)

target_compile_options(lintrcv_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(lintrcv_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET ecual_lintrcv)
    target_link_libraries(lintrcv_test PRIVATE ecual_lintrcv)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(lintrcv_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME lintrcv_test COMMAND lintrcv_test)

message(STATUS "  Unit test: lintrcv_test (LIN Transceiver Driver)")
