# E2E Protection + ISO-TP core unit tests (self-contained pure-algorithm modules)
#
# Both modules compile without external stubs:
#  - src/autosar/e2e/e2e_protection.c   (e2e_protection.h + string.h + stdio.h)
#  - src/diagnostics/isotp/isotp_core.c (isotp_core.h + string.h)
#
# NOTE: autosar_types.h / autosar_errors.h live in src/common (verified by grep),
# not include/autosar.

cmake_minimum_required(VERSION 3.16)

# ── E2E Protection (AUTOSAR R22-11, Profiles 01/02/04/05/06/07/11/22) ──────
add_executable(E2E_Protection_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_e2e_protection.c
    ${CMAKE_SOURCE_DIR}/src/autosar/e2e/e2e_protection.c
)

target_include_directories(E2E_Protection_UnitTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/autosar/e2e
    ${CMAKE_SOURCE_DIR}/src/common
    ${TEST_FRAMEWORK_INCLUDE}
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(E2E_Protection_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

target_compile_options(E2E_Protection_UnitTest PRIVATE
    -Wall -Wextra -g -O0 -Wno-unused-parameter
)

set_target_properties(E2E_Protection_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME E2E_Protection_UnitTest COMMAND E2E_Protection_UnitTest)
message(STATUS "  Unit test: E2E_Protection_UnitTest (E2E profiles P01-P22)")

# ── ISO-TP core (ISO 15765-2 SF/FF/CF/FC RX/TX state machines) ─────────────
add_executable(IsoTp_Core_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_isotp_core.c
    ${CMAKE_SOURCE_DIR}/src/diagnostics/isotp/isotp_core.c
)

target_include_directories(IsoTp_Core_UnitTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/diagnostics/isotp
    ${TEST_FRAMEWORK_INCLUDE}
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(IsoTp_Core_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

target_compile_options(IsoTp_Core_UnitTest PRIVATE
    -Wall -Wextra -g -O0 -Wno-unused-parameter
)

set_target_properties(IsoTp_Core_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME IsoTp_Core_UnitTest COMMAND IsoTp_Core_UnitTest)
message(STATUS "  Unit test: IsoTp_Core_UnitTest (ISO-TP core state machine)")
