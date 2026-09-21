# Crc unit tests — substantiated against the production CRC implementation
# Links the real service_crc library with a DET mock.

cmake_minimum_required(VERSION 3.16)

set(CRC_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_crc.c)
set(CRC_MOCK_SRC
    ${CMAKE_SOURCE_DIR}/tests/mocks/mock_det.c
)

add_executable(crc_test
    ${CRC_TEST_SRC}
    ${CRC_MOCK_SRC}
)

target_include_directories(crc_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/crc/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_compile_options(crc_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(crc_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_crc)
    target_link_libraries(crc_test PRIVATE service_crc)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(crc_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME crc_test COMMAND crc_test)

message(STATUS "  Unit test: crc_test (CRC algorithms, version info)")
