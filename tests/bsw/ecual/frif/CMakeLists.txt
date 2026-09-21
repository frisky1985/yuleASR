# FrIf unit tests — substantiated against the production FlexRay Interface.
#
# Links the real ecual_frif library. FrIf.c has exactly one external symbol
# (Det_ReportError), provided by the inline mock in test_frif.c, so no
# tests/mocks object is linked (that would duplicate the symbol).
#
# Hardware note (macOS arm64 host): the FlexRay controller is not present;
# all assertions are on RAM-resident module state exposed via the FrIf API,
# never on MMIO addresses.

cmake_minimum_required(VERSION 3.16)

set(FRIF_TEST_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_frif.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_frif_runner.c
)

add_executable(frif_test ${FRIF_TEST_SRC})

target_include_directories(frif_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/frif/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/ecual
)

target_compile_options(frif_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(frif_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET ecual_frif)
    target_link_libraries(frif_test PRIVATE ecual_frif)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(frif_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME frif_test COMMAND frif_test)

message(STATUS "  Unit test: frif_test (FlexRay Interface)")
