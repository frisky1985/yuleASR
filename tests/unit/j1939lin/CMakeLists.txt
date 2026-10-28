# J1939Tp / J1939Nm / LinM (services/linm) unit tests — coverage uplift
#
# Compiles the REAL production sources against host stubs. Because all three
# modules keep their runtime state and state-machine helpers as `static`, each
# test file #includes the product .c directly (same white-box pattern as
# tests/unit/services/test_PduR_branch.c / test_CanTSyn_branch.c) — so each
# target compiles only the test translation unit.
#
# Note: the module in src/bsw/services/linm/ is LinM (LIN Master Management,
# header LinM.h). The test file is named test_linnm.c per task convention.

cmake_minimum_required(VERSION 3.16)

set(J1939LIN_STUB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stubs)

function(j1939lin_add_test TARGET_NAME TEST_SRC MODULE_INCLUDE_DIR)
    add_executable(${TARGET_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SRC}
    )

    target_include_directories(${TARGET_NAME}
        PRIVATE
            ${TEST_FRAMEWORK_INCLUDE}
            ${J1939LIN_STUB_DIR}                                       # CanIf.h / PduR.h minimal stubs
            ${MODULE_INCLUDE_DIR}
            ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
            ${CMAKE_SOURCE_DIR}/src/middleware/rte/include             # ComStack_Types.h (BUFREQ_*) BEFORE include/autosar
            ${CMAKE_SOURCE_DIR}/src/common/include
            ${CMAKE_SOURCE_DIR}/include/autosar
            ${CMAKE_SOURCE_DIR}                                        # for #include "src/bsw/..." (white-box)
    )

    target_compile_options(${TARGET_NAME} PRIVATE
        -Wall -Wextra -g -O0 -DUNIT_TEST
        -Wno-unused-parameter -Wno-unused-function
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
        C_STANDARD 99
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(${TARGET_NAME} PRIVATE ${TEST_FRAMEWORK_LIB})
    endif()

    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()

# ── J1939Tp — SAE J1939-21 transport protocol (BAM + CMDT) ─────────────────
j1939lin_add_test(J1939Tp_UnitTest test_j1939tp.c
    ${CMAKE_SOURCE_DIR}/src/bsw/services/j1939tp/include
)
message(STATUS "  Unit test: J1939Tp_UnitTest (J1939 TP state machine)")

# ── J1939Nm — SAE J1939-81 network management (address claiming) ───────────
j1939lin_add_test(J1939Nm_UnitTest test_j1939nm.c
    ${CMAKE_SOURCE_DIR}/src/bsw/services/j1939nm/include
)
message(STATUS "  Unit test: J1939Nm_UnitTest (J1939 NM address claiming)")

# ── LinM — LIN master schedule management (src/bsw/services/linm) ──────────
j1939lin_add_test(LinM_UnitTest test_linnm.c
    ${CMAKE_SOURCE_DIR}/src/bsw/services/linm/include
)
message(STATUS "  Unit test: LinM_UnitTest (LIN master schedule management)")
