# EthSwt unit tests — substantiated against the production EthSwt driver
# (src/bsw/ecual/ethswt). Links the real ecual_ethswt library; the external
# dependency (Det_ReportError) is provided by the test source itself as a
# host-safe mock.

cmake_minimum_required(VERSION 3.16)

add_executable(ethswt_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ethswt.c
)

target_include_directories(ethswt_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/ethswt/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/ecual
)

target_compile_options(ethswt_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(ethswt_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET ecual_ethswt)
    target_link_libraries(ethswt_test PRIVATE ecual_ethswt)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ethswt_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ethswt_test COMMAND ethswt_test)

message(STATUS "  Unit test: ethswt_test (Ethernet Switch Driver)")
