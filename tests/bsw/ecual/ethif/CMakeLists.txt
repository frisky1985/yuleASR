# EthIf unit tests — substantiated against the production EthIf implementation.
# EthIf.c has NO ECUAL library target, so EthIf.c is compiled directly into
# the test executable.  A minimal Det mock and test-local config struct are
# provided by the test file.  No real Ethernet driver is linked — EthIf does
# not delegate to Eth.c in this baseline (it manages only its own controller
# mode FSM + RX filter table).

cmake_minimum_required(VERSION 3.16)

add_executable(ethif_test
    test_ethif.c
    ${CMAKE_SOURCE_DIR}/src/bsw/ecual/ethif/src/EthIf.c
)

target_include_directories(ethif_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/ethif/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(ethif_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(ethif_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ethif_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ethif_test COMMAND ethif_test)

message(STATUS "  Unit test: ethif_test (Ethernet Interface)")
