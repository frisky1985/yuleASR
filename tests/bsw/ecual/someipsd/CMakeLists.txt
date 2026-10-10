# SomeIpSd unit tests — substantiated against the canonical SOME/IP Service
# Discovery implementation (src/bsw/services/someip). The ecual/someipsd copy
# was converged (shimmed) in Phase 2; this test now links service_someip.
#
# Det_ReportError is defined inline in the test source (the SUT's DET guards
# compile out because SomeIpSd.c does not #include SomeIpSd_Cfg.h, so the
# mock is never called — kept to catch any future wiring change).

cmake_minimum_required(VERSION 3.16)

add_executable(someipsd_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_someipsd.c
)

target_include_directories(someipsd_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/someip/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
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

if(TARGET service_someip)
    target_link_libraries(someipsd_test PRIVATE service_someip)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(someipsd_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME someipsd_test COMMAND someipsd_test)

message(STATUS "  Unit test: someipsd_test (SOME/IP Service Discovery)")
