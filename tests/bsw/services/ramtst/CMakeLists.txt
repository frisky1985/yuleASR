# RamTst unit tests — substantiated against the production RamTst implementation
# Links the real service_ramtst library with a Det mock defined inside the
# test file. The March C test runs against a real RAM buffer in the test.

cmake_minimum_required(VERSION 3.16)

set(RAMTST_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_ramtst.c)

add_executable(ramtst_test
    ${RAMTST_TEST_SRC}
)

target_include_directories(ramtst_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/ramtst/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_compile_options(ramtst_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(ramtst_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_ramtst)
    target_link_libraries(ramtst_test PRIVATE service_ramtst)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ramtst_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ramtst_test COMMAND ramtst_test)

message(STATUS "  Unit test: ramtst_test (RAM Test March C)")
