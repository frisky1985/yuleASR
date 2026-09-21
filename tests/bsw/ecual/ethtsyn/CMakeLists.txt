# EthTSyn unit tests — substantiated against the production EthTSyn service
# (src/bsw/services/ethtsyn). Links the real service_ethtsyn library; the
# external dependency Det_ReportError is provided by the test source itself
# as a host-safe mock.

cmake_minimum_required(VERSION 3.16)

add_executable(ethtsyn_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ethtsyn.c
)

target_include_directories(ethtsyn_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/ethtsyn/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/eth/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(ethtsyn_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(ethtsyn_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_ethtsyn)
    target_link_libraries(ethtsyn_test PRIVATE service_ethtsyn)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ethtsyn_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ethtsyn_test COMMAND ethtsyn_test)

message(STATUS "  Unit test: ethtsyn_test (Ethernet Time Synchronization)")
