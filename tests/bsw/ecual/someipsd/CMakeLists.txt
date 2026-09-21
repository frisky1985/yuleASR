# SomeIpSd unit tests — substantiated against the production SOME/IP Service
# Discovery implementation (src/bsw/ecual/someipsd). Links the real
# ecual_someipsd library; Det_ReportError is defined inline in the test
# source (the SUT never calls it — kept to assert that actual behavior), so
# no mocks from tests/mocks are linked.
#
# Note: another worker mounts tests/bsw/ecual/someipsd2/ as a separate
# someipsd2_test executable against the same SUT; the two executables are
# independent and do not conflict.

cmake_minimum_required(VERSION 3.16)

add_executable(someipsd_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_someipsd.c
)

target_include_directories(someipsd_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/someipsd/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/ecual
)

target_compile_options(someipsd_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(someipsd_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET ecual_someipsd)
    target_link_libraries(someipsd_test PRIVATE ecual_someipsd)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(someipsd_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME someipsd_test COMMAND someipsd_test)

message(STATUS "  Unit test: someipsd_test (SOME/IP Service Discovery)")
