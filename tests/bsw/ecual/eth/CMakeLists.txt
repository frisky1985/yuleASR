# Eth unit tests — substantiated against the production Eth driver
# (src/bsw/mcal/eth/src/Eth.c). Eth.c uses a software-emulated HW layer
# (static Eth_Hw* helpers, no MMIO), so it compiles and runs directly on the
# host. The external dependency Det_ReportError is provided by the test
# source itself as a host-safe mock.

cmake_minimum_required(VERSION 3.16)

add_executable(eth_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_eth.c
    ${CMAKE_SOURCE_DIR}/src/bsw/mcal/eth/src/Eth.c
)

target_include_directories(eth_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/eth/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/mcal
)

target_compile_options(eth_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(eth_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(eth_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME eth_test COMMAND eth_test)

message(STATUS "  Unit test: eth_test (Ethernet Driver)")
