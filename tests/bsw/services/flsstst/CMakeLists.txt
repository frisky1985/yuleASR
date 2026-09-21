# FlStSt unit tests — substantiated against the production Flash Self-test implementation
# Links the real service_flstst library with a Det mock defined inside the
# test file (no external dependency stubs required).

cmake_minimum_required(VERSION 3.16)

set(FLSSTST_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_flsstst.c)

add_executable(flsstst_test
    ${FLSSTST_TEST_SRC}
)

target_include_directories(flsstst_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/flstst/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_compile_options(flsstst_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(flsstst_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_flstst)
    target_link_libraries(flsstst_test PRIVATE service_flstst)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(flsstst_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME flsstst_test COMMAND flsstst_test)

message(STATUS "  Unit test: flsstst_test (Flash Self-test March C)")
