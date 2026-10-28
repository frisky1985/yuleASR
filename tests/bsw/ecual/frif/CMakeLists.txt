# FrIf unit tests — substantiated against the production FlexRay Interface.
#
# FrIf.c is compiled *directly* into the test executable via frif_test_hacks.c
# (which #includes FrIf.c). This lets the hacks TU host accessor functions
# that reach FrIf's `static` file-scope state — needed to drive hard-to-reach
# branches (controller in NORMAL_ACTIVE for Transmit's success path, etc.).
#
# The real Det_ReportError comes from the inline mock in test_frif.c —
# ecual_frif is NOT linked (that would duplicate every FrIf symbol).

cmake_minimum_required(VERSION 3.16)

set(FRIF_TEST_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_frif.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_frif_runner.c
    ${CMAKE_CURRENT_SOURCE_DIR}/frif_test_hacks.c
)

add_executable(frif_test ${FRIF_TEST_SRC})

target_include_directories(frif_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/frif/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(frif_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
    # FrIf.c pulls in PduR.h / Det.h; we provide minimal stubs inline
    -DTEST_Frif
)

set_target_properties(frif_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

# NOTE: ecual_frif is intentionally NOT linked — FrIf.c is compiled
# through frif_test_hacks.c into the test binary. Linking the library
# would produce multiple-definition errors for every FrIf API symbol.

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(frif_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME frif_test COMMAND frif_test)

message(STATUS "  Unit test: frif_test (FlexRay Interface)")
