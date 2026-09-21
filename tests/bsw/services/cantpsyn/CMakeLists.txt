# CanTSyn unit tests — substantiated against the production CanTSyn implementation
# Links the real service_cantsyn library with StbM/CanIf stubs and a Det mock
# defined inside the test file.

cmake_minimum_required(VERSION 3.16)

set(CANTSYN_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_cantpsyn.c)

add_executable(cantpsyn_test
    ${CANTSYN_TEST_SRC}
)

target_include_directories(cantpsyn_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/cantsyn/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/canif/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/eth/include
        ${CMAKE_SOURCE_DIR}/src/bsw/os/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_compile_options(cantpsyn_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(cantpsyn_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_cantsyn)
    target_link_libraries(cantpsyn_test PRIVATE service_cantsyn)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(cantpsyn_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME cantpsyn_test COMMAND cantpsyn_test)

message(STATUS "  Unit test: cantpsyn_test (CanTSyn time sync over CAN)")
