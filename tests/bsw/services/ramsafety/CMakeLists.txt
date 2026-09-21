# RamSafety unit tests — substantiated against the production RamSafety implementation
# Links the real service_ramsafety library with Platform_RamSafety/Mcal stubs
# and a Det mock defined inside the test file.

cmake_minimum_required(VERSION 3.16)

set(RAMSAFETY_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_ramsafety.c)

add_executable(ramsafety_test
    ${RAMSAFETY_TEST_SRC}
)

target_include_directories(ramsafety_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/ramsafety/include
        ${CMAKE_SOURCE_DIR}/src/platform/s32k312/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_compile_options(ramsafety_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(ramsafety_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_ramsafety)
    target_link_libraries(ramsafety_test PRIVATE service_ramsafety)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ramsafety_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ramsafety_test COMMAND ramsafety_test)

message(STATUS "  Unit test: ramsafety_test (RAM safety March C-/ECC)")
