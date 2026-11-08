# CanTrcv unit tests — white-box (CanTrcv.c is included by the test TU so the
# static helpers and runtime state can be exercised). DIO/EcuM/DET are stubbed
# in the test file, therefore no ECUAL library target is linked here.

cmake_minimum_required(VERSION 3.16)

add_executable(cantrcv_test
    test_cantrcv.c
)

target_include_directories(cantrcv_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/cantrcv/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/dio/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/spi/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/ecum/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(cantrcv_test PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-return-type
    -g
    -O0
)

set_target_properties(cantrcv_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(cantrcv_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

if(ENABLE_COVERAGE)
    target_compile_options(cantrcv_test PRIVATE --coverage)
    if(NOT APPLE)
        target_link_libraries(cantrcv_test PRIVATE gcov)
    endif()
endif()

add_test(NAME cantrcv_test COMMAND cantrcv_test)

message(STATUS "  Unit test: cantrcv_test (CAN Transceiver Driver)")
