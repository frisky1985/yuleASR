# Platform RAM self-test unit tests (March C- / checkerboard)
#
# Compiles the real platform component against a RAM buffer, verifying that
# the algorithms run over the supplied region and reject invalid geometry.

cmake_minimum_required(VERSION 3.16)

set(PLATFORM_RAMTEST_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/platform/s32k312/src/Platform_RamTest.c
)

add_executable(PlatformRamTest_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_platform_ramtest.c
    ${PLATFORM_RAMTEST_SRC}
)

target_include_directories(PlatformRamTest_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/platform/s32k312/include
)

target_compile_options(PlatformRamTest_UnitTest PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(PlatformRamTest_UnitTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

target_link_libraries(PlatformRamTest_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})

add_test(NAME PlatformRamTest_UnitTest COMMAND PlatformRamTest_UnitTest)
