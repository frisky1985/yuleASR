# Services Layer Unit Tests CMakeLists.txt
#
# 2026-08-15: DLT 单测接入 (ecual context 配置体系合并)
# - 编译真实生产源码 Dlt.c + Dlt_Lcfg.c + 测试文件
# - Det_ReportError 由测试文件内 mock
# - 覆盖: 初始化生命周期 / 应用注册 / 日志与跟踪消息 / 过滤器 /
#          队列操作 / 链接期 context 配置表

cmake_minimum_required(VERSION 3.16)

set(DLT_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_Dlt.c)
set(DLT_PRODUCT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/dlt/src/Dlt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/dlt/src/Dlt_Lcfg.c
)

add_executable(Dlt_UnitTest
    ${DLT_TEST_SRC}
    ${DLT_PRODUCT_SRC}
)

target_include_directories(Dlt_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..                      # test_framework.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/dlt/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(Dlt_UnitTest PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(Dlt_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(Dlt_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME Dlt_UnitTest COMMAND Dlt_UnitTest)

message(STATUS "  Unit test: Dlt_UnitTest (ecual context 配置合并)")

# ── DLT Context 管理 API 单测 (ecual 合并 包3) ────────────────────────────
set(DLTCTX_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_DltContext.c)

add_executable(DltContext_UnitTest
    ${DLTCTX_TEST_SRC}
    ${DLT_PRODUCT_SRC}
)

target_include_directories(DltContext_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..                      # test_framework.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/dlt/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(DltContext_UnitTest PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(DltContext_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(DltContext_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME DltContext_UnitTest COMMAND DltContext_UnitTest)

message(STATUS "  Unit test: DltContext_UnitTest (context API 17 cases)")

# ── NvM 单测 ─────────────────────────────────────────────────────────────
set(NVM_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_NvM_BranchCoverage.c)
set(NVM_PRODUCT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/nvm/src/NvM.c
)

add_executable(NvM_UnitTest
    ${NVM_TEST_SRC}
    ${NVM_PRODUCT_SRC}
)

target_include_directories(NvM_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..                      # test_framework.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/nvm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/memif/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/fee/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/ea/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(NvM_UnitTest PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(NvM_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(NvM_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME NvM_UnitTest COMMAND NvM_UnitTest)

message(STATUS "  Unit test: NvM_UnitTest (branch coverage)")

# ── ComM 分支覆盖单测 ─────────────────────────────────────────────────────
set(COMM_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_ComM_branch.c)
set(COMM_PRODUCT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/comM/src/ComM.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/comM/src/ComM_Lcfg.c
)

add_executable(ComM_BranchTest
    ${COMM_TEST_SRC}
    ${COMM_PRODUCT_SRC}
)

target_include_directories(ComM_BranchTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/comM/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(ComM_BranchTest PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(ComM_BranchTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ComM_BranchTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ComM_BranchTest COMMAND ComM_BranchTest)

message(STATUS "  Unit test: ComM_BranchTest (80+ branch coverage cases)")

# ── Com (signal-level) 分支覆盖单测 ─────────────────────────────────────────
set(COM_BRANCH_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_Com_branch.c)
set(COM_PRODUCT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/com/src/Com.c
)

add_executable(Com_BranchTest
    ${COM_BRANCH_TEST_SRC}
    ${COM_PRODUCT_SRC}
)

target_include_directories(Com_BranchTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/com/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(Com_BranchTest PRIVATE
    -Wall
    -Wextra
    -Wno-unknown-pragmas
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(Com_BranchTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(Com_BranchTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME Com_BranchTest COMMAND Com_BranchTest)

message(STATUS "  Unit test: Com_BranchTest (signal-level branch coverage)")

# ── WdgM 分支覆盖单测 ─────────────────────────────────────────────────────
set(WDGM_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_WdgM_branch.c)
set(WDGM_PRODUCT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/wdgm/src/WdgM.c
)

add_executable(WdgM_BranchTest
    ${WDGM_TEST_SRC}
    ${WDGM_PRODUCT_SRC}
)

target_include_directories(WdgM_BranchTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/wdgm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/dem/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/ramsafety/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(WdgM_BranchTest PRIVATE
    -Wall
    -Wextra
    -Wno-unknown-pragmas
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(WdgM_BranchTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(WdgM_BranchTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME WdgM_BranchTest COMMAND WdgM_BranchTest)

message(STATUS "  Unit test: WdgM_BranchTest (branch coverage)")

# ── KeyM 分支覆盖单测 ─────────────────────────────────────────────────────
set(KEYM_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_KeyM_branch.c)
set(KEYM_PRODUCT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/keym/src/KeyM.c
)

add_executable(KeyM_BranchTest
    ${KEYM_TEST_SRC}
    ${KEYM_PRODUCT_SRC}
)

target_include_directories(KeyM_BranchTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/keym/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(KeyM_BranchTest PRIVATE
    -Wall
    -Wextra
    -Wno-unknown-pragmas
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(KeyM_BranchTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(KeyM_BranchTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME KeyM_BranchTest COMMAND KeyM_BranchTest)

message(STATUS "  Unit test: KeyM_BranchTest (branch coverage)")

# ── PduR 分支覆盖单测 ─────────────────────────────────────────────────────
# PduR.c 和 PduR_Lcfg.c 通过 #include 直接嵌入测试文件，以访问 STATIC 函数
set(PDUR_BRANCH_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_PduR_branch.c)

add_executable(PduR_BranchTest
    ${PDUR_BRANCH_TEST_SRC}
)

target_include_directories(PduR_BranchTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/pdur/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/pdur/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

target_compile_options(PduR_BranchTest PRIVATE
    -Wall
    -Wextra
    -Wno-unknown-pragmas
    -Wno-unused-function
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(PduR_BranchTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(PduR_BranchTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME PduR_BranchTest COMMAND PduR_BranchTest)

message(STATUS "  Unit test: PduR_BranchTest (90 test cases, full branch coverage)")

# ── E2E_P01 分支覆盖单测 ─────────────────────────────────────────────────
set(E2E_P01_BRANCH_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_E2E_P01_branch.c)

add_executable(E2E_P01_BranchTest
    ${E2E_P01_BRANCH_TEST_SRC}
)

target_include_directories(E2E_P01_BranchTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}                                       # for #include "src/bsw/..."
        ${CMAKE_SOURCE_DIR}/src/bsw/services/e2e/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/e2e/src              # for direct .c include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(E2E_P01_BranchTest PRIVATE
    -Wall
    -Wextra
    -Wno-unused-function
    -Wno-unused-variable
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(E2E_P01_BranchTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(E2E_P01_BranchTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME E2E_P01_BranchTest COMMAND E2E_P01_BranchTest)

message(STATUS "  Unit test: E2E_P01_BranchTest (branch coverage supplement)")

# ── Det 分支覆盖单测 ─────────────────────────────────────────────────────
set(DET_BRANCH_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_Det_branch.c)

add_executable(Det_BranchTest
    ${DET_BRANCH_TEST_SRC}
)

target_include_directories(Det_BranchTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}                                       # for #include "src/bsw/..."
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/src              # for direct .c include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(Det_BranchTest PRIVATE
    -Wall
    -Wextra
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unknown-pragmas
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(Det_BranchTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(Det_BranchTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME Det_BranchTest COMMAND Det_BranchTest)

message(STATUS "  Unit test: Det_BranchTest (branch coverage supplement)")

# ── CanTSyn 分支覆盖单测 ─────────────────────────────────────────────────
set(CANTSYN_BRANCH_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_CanTSyn_branch.c)

add_executable(CanTSyn_BranchTest
    ${CANTSYN_BRANCH_TEST_SRC}
)

target_include_directories(CanTSyn_BranchTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}                                       # for #include "src/bsw/..."
        ${CMAKE_SOURCE_DIR}/src/bsw/services/cantsyn/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/cantsyn/src          # for direct .c include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/stbm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/canif/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/can/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/eth/include
        ${CMAKE_SOURCE_DIR}/src/bsw/os/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_compile_options(CanTSyn_BranchTest PRIVATE
    -Wall
    -Wextra
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unknown-pragmas
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(CanTSyn_BranchTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(CanTSyn_BranchTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME CanTSyn_BranchTest COMMAND CanTSyn_BranchTest)

message(STATUS "  Unit test: CanTSyn_BranchTest (branch coverage supplement)")

# ── StbM 分支覆盖单测 ───────────────────────────────────────────────────
set(STBM_BRANCH_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_StbM_branch.c)

add_executable(StbM_BranchTest
    ${STBM_BRANCH_TEST_SRC}
)

target_include_directories(StbM_BranchTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}                                       # for #include "src/bsw/..."
        ${CMAKE_SOURCE_DIR}/src/bsw/services/stbm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/stbm/src             # for direct .c include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/eth/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(StbM_BranchTest PRIVATE
    -Wall
    -Wextra
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unknown-pragmas
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(StbM_BranchTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(StbM_BranchTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME StbM_BranchTest COMMAND StbM_BranchTest)

message(STATUS "  Unit test: StbM_BranchTest (branch coverage supplement)")
