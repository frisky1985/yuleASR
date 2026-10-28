# Network / COM protocol unit tests (Xcp, E2E, ISO-TP, DoIP, CanNm, LinNm, J1939)
#
# Compiles the real production sources against host stubs. Tests are added by
# the coverage-uplift work; see test_*.c here.

cmake_minimum_required(VERSION 3.16)

# ── XCP (services layer) — full protocol command coverage ─────────────────
add_executable(Xcp_Services_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_xcp_services.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/xcp/src/Xcp.c
)

target_include_directories(Xcp_Services_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/xcp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(Xcp_Services_UnitTest PRIVATE
    -Wall -Wextra -g -O0 -DUNIT_TEST
    -Wno-unused-parameter
)

set_target_properties(Xcp_Services_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(Xcp_Services_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME Xcp_Services_UnitTest COMMAND Xcp_Services_UnitTest)
message(STATUS "  Unit test: Xcp_Services_UnitTest (XCP protocol command set)")

# ── FrTp — FlexRay transport protocol state machine ────────────────────────
# Note: FrTp_Lcfg.c is excluded on purpose — it duplicates the global
# FrTp_Runtime definition from FrTp.c; the test provides its own
# FrTp_ConnectionConfigs table instead.
add_executable(FrTp_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_frtp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/frtp/src/FrTp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/frtp/src/FrTp_PrivUtil.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/frtp/src/FrTp_Tx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/frtp/src/FrTp_Rx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/frtp/src/FrTp_TxSm.c
)

target_include_directories(FrTp_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/frtp_stubs
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/frtp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/middleware/rte/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(FrTp_UnitTest PRIVATE
    -Wall -Wextra -g -O0 -DUNIT_TEST
    -Wno-unused-parameter -Wno-unused-function
)

set_target_properties(FrTp_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(FrTp_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME FrTp_UnitTest COMMAND FrTp_UnitTest)
message(STATUS "  Unit test: FrTp_UnitTest (FlexRay TP state machine)")

# ── CanNm — CAN network management state machine ───────────────────────────
add_executable(CanNm_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_cannm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/canNm/src/CanNm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/canNm/src/CanNm_Lcfg.c
)

target_include_directories(CanNm_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/cannm_stubs
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/canNm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/nm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/middleware/rte/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(CanNm_UnitTest PRIVATE
    -Wall -Wextra -g -O0 -DUNIT_TEST
    -Wno-unused-parameter -Wno-unused-function
)

set_target_properties(CanNm_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(CanNm_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME CanNm_UnitTest COMMAND CanNm_UnitTest)
message(STATUS "  Unit test: CanNm_UnitTest (CAN NM state machine)")

# ── DoIP — Diagnostic over IP protocol dispatch ────────────────────────────
add_executable(DoIP_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_doip.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/doIP/src/DoIP.c
)

target_include_directories(DoIP_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/doip_stubs
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/doIP/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/pdur/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/middleware/rte/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(DoIP_UnitTest PRIVATE
    -Wall -Wextra -g -O0 -DUNIT_TEST
    -Wno-unused-parameter -Wno-unused-function
)

set_target_properties(DoIP_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(DoIP_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME DoIP_UnitTest COMMAND DoIP_UnitTest)
message(STATUS "  Unit test: DoIP_UnitTest (DoIP protocol dispatch)")
