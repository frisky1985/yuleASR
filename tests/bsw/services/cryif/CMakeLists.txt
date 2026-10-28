# CryIf unit tests — substantiated against the production CryIf implementation.
#
# The production CryIf.c is compiled directly (unmodified) into the test target
# (same pattern as csm_test). CryIf is configuration-table driven and does not
# call any external crypto driver (MapToCryptoDriver is a commented placeholder),
# so the only stub required is Det_ReportError (counting stub, lives in
# test_cryif.c). The test supplies its own CryIf_ConfigType instance.

cmake_minimum_required(VERSION 3.16)

set(CRYIF_PROD_SRC ${CMAKE_SOURCE_DIR}/src/bsw/services/cryif/src/CryIf.c)

set(CRYIF_TEST_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_cryif.c
)

add_executable(cryif_test
    ${CRYIF_PROD_SRC}
    ${CRYIF_TEST_SRC}
)

target_include_directories(cryif_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/cryif/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(cryif_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(cryif_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(cryif_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME cryif_test COMMAND cryif_test)

message(STATUS "  Unit test: cryif_test (Crypto Interface)")
