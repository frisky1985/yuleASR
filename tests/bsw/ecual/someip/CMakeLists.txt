# SomeIp unit tests — substantiated against the production SOME/IP protocol
# implementation (src/bsw/services/someip). Links the real service_someip
# library; Det_ReportError is defined inline in the test source, so no mocks
# from tests/mocks are linked (would be a duplicate symbol).

cmake_minimum_required(VERSION 3.16)

add_executable(someip_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_someip.c
)

target_include_directories(someip_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/someip/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
)

target_compile_options(someip_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(someip_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_someip)
    target_link_libraries(someip_test PRIVATE service_someip)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(someip_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME someip_test COMMAND someip_test)

message(STATUS "  Unit test: someip_test (SOME/IP Protocol)")
