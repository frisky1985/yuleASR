# Unit Tests CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# Test executable for each module
set(UNIT_TEST_MODULES
    PduR
    NvM
    Com
    Dem
    Dcm
    WdgM
)

# Add subdirectory tests (RTE, etc.)
add_subdirectory(rte)

# EthSM 双网络回归测试 (P0-A: trcvIdx/ctrlIdx 赋值恢复验证)
add_subdirectory(ethsm)

# ── P1 Phase 8: Dcm UDS 测试矩阵 + Dem DTC 测试矩阵 ──
add_subdirectory(dcm)
add_subdirectory(dem)

# EthSwt 单元测试 (B2: VLAN/流控/统计/镜像补全)
add_subdirectory(ecual)

# TcpIp 单元测试 (B1: API 补全/多连接/状态机/VLAN/统计)
add_subdirectory(services/tcpip)

# Services 层单元测试 (2026-08-15: DLT context 配置体系合并)
add_subdirectory(services)

# Cdd_Fvm 单元测试 (B3-1: Flash 虚拟化多 bank — 搬移/保护/故障切换)
add_subdirectory(cdd)

# Libraries 独立算法库测试 (B3-2: crc + aes)
add_subdirectory(libs)

foreach(module ${UNIT_TEST_MODULES})
    # Check if test file exists
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${module}_test.c)
        set(TEST_NAME ${module}_UnitTest)
        add_executable(${TEST_NAME} 
            ${CMAKE_CURRENT_SOURCE_DIR}/${module}_test.c
        )
        
        target_include_directories(${TEST_NAME}
            PRIVATE
                ${TEST_FRAMEWORK_INCLUDE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/${module}/include
        )
        
        # Link with module library (use object files for testing)
        if(TARGET ${module})
            target_link_libraries(${TEST_NAME} PRIVATE ${module})
        endif()
        
        if(TEST_FRAMEWORK_LIB)
            target_link_libraries(${TEST_NAME} PRIVATE ${TEST_FRAMEWORK_LIB})
        endif()
        
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
        
        set_target_properties(${TEST_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
        )
        
        message(STATUS "  Unit test: ${TEST_NAME}")
    endif()
endforeach()

# Test utilities
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_utils.c)
    add_library(TestUtils STATIC ${CMAKE_CURRENT_SOURCE_DIR}/test_utils.c)
    target_include_directories(TestUtils
        PUBLIC 
            ${TEST_FRAMEWORK_INCLUDE}
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# ── NvM_Redundant 单元测试 (2026-08-14: 冗余存储宏开启后真实验证) ──
# 链接真实 NvM_Redundant.c + Crc.c（查表），mock NvM_ReadBlock/WriteBlock/NvM_Config
add_executable(NvM_Redundant_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_nvm_redundant_verify.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/nvm/src/NvM_Redundant.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/crc/src/Crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/crc/src/Crc_Lcfg.c
)
target_include_directories(NvM_Redundant_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/nvm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/crc/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/memif/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/autosar
        ${CMAKE_CURRENT_SOURCE_DIR}/../../config/input/services
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/mcal/lin/include
)
# 头文件漂移规避：全局 include_directories 把 include/autosar（旧版 Crc.h，无 Crc_Cfg.h）
# 排在 src/bsw/services/crc/include（新版，含 Crc_Cfg.h MODE 宏）之前，导致 Crc.c
# 编译时 CRC_16_MODE 未定义 → CRC 恒返回 0。BEFORE 强制本 target 优先新版头。
target_include_directories(NvM_Redundant_UnitTest
    BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/crc/include
)
target_compile_options(NvM_Redundant_UnitTest PRIVATE
    -Wall -Wextra -Werror -g -O0
)
# Crc.c 在 TABLE_MODE/DEV_ERROR_DETECT 配置下部分变量/参数未使用（既有配置相关），单独豁免
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/crc/src/Crc.c
    PROPERTIES COMPILE_OPTIONS "-Wno-unused-variable;-Wno-unused-parameter"
)
set_target_properties(NvM_Redundant_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
add_test(NAME NvM_Redundant_UnitTest COMMAND NvM_Redundant_UnitTest)
message(STATUS "  Unit test: NvM_Redundant_UnitTest (redundant storage, 8 cases)")

# ── Os_Callback_Alarm 单元测试 (alarm dispatch branches 5,6: NvM/Dem) ──
# 提供 stub MainFunctions, 仅编译 Os_Cfg.c (不链接完整 Os 库, 避免拉入 FreeRTOS/Rte 依赖)
add_executable(OsCfg_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_os_cfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/os/src/Os_Cfg.c
)
target_include_directories(OsCfg_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/os/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/bswm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/com/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/dcm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/nvm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/dem/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/ecual/canif/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/mcal/can/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/autosar
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/middleware/rte/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/freertos/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/freertos/portable/posix
)
target_link_libraries(OsCfg_UnitTest PRIVATE Unity)
target_compile_options(OsCfg_UnitTest PRIVATE -Wno-unused-parameter)
set_target_properties(OsCfg_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
add_test(NAME OsCfg_UnitTest COMMAND OsCfg_UnitTest)
message(STATUS "  Unit test: OsCfg_UnitTest (Os_Callback_Alarm dispatch)")

# ── OS PCP (Priority Ceiling Protocol) 单元测试 ──
# 独立测试 Os_Internal_GetResource / Os_Internal_ReleaseResource 的 PCP 逻辑
# stub 所有 FreeRTOS API，不链接 FreeRTOS 内核
add_executable(OsPcp_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_os_pcp.c
)
target_include_directories(OsPcp_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
)
target_link_libraries(OsPcp_UnitTest PRIVATE Unity)
target_compile_options(OsPcp_UnitTest PRIVATE -Wall -Wextra -Werror -g -O0)
set_target_properties(OsPcp_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
add_test(NAME OsPcp_UnitTest COMMAND OsPcp_UnitTest)
message(STATUS "  Unit test: OsPcp_UnitTest (PCP elevation/restore, 15 cases)")

# ── OS Task Management, Event Management, Alarm Management unit tests ──
# Each compiles actual Os.c with mock FreeRTOS headers (tests/unit/os_mocks/)
# and stubs all FreeRTOS API functions. No real FreeRTOS linked.

set(OS_TEST_COMMON_INCLUDES
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/autosar
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/middleware/rte/include
)

# Helper function to add an OS unit test that compiles Os.c
function(add_os_unit_test TEST_NAME TEST_SRC)
    add_executable(${TEST_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/os/src/Os.c
    )
    target_include_directories(${TEST_NAME}
        BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/os_mocks
    )
    target_include_directories(${TEST_NAME}
        PRIVATE
        ${OS_TEST_COMMON_INCLUDES}
    )
    target_link_libraries(${TEST_NAME} PRIVATE Unity)
    target_compile_options(${TEST_NAME} PRIVATE -Wno-unused-parameter -Wno-unused-function)
    set_target_properties(${TEST_NAME} PROPERTIES C_STANDARD 99 RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    message(STATUS "  Unit test: ${TEST_NAME}")
endfunction()

add_os_unit_test(OsTask_UnitTest test_os_task.c)
add_os_unit_test(OsEvent_UnitTest test_os_event.c)
add_os_unit_test(OsAlarm_UnitTest test_os_alarm.c)

# ── Coverage uplift: domains that previously had no registered tests ──
# Fee (ECUAL) — Flash EEPROM emulation
add_subdirectory(fee)
# Safety — saferam / RAM ECC primitives
add_subdirectory(safety)
# DDS middleware — transport / RTPS / security / logging
add_subdirectory(dds)
# Network/COM protocols — Xcp / E2E / ISO-TP / DoIP / CAN-LIN NM / J1939
add_subdirectory(netcom)
# Adaptive AUTOSAR — ara::com / state management / execution
add_subdirectory(adaptive)

# J1939Tp / J1939Nm / LinM (LIN) unit tests — coverage uplift
add_subdirectory(j1939lin)

# ── Crypto (MCAL) 单元测试 (2026-10-26: 注册 mcal/test_crypto.c, 首次纳入构建) ──
# 链接真实 Crypto.c + 真实 blake2b/blake2s (third_party/crypto/blake2/src)。
# HSM / MbedTLS 后端符号由 tests/unit/crypto_stubs/crypto_backend_stubs.c 提供:
#   Hsm_Init 返回 E_NOT_OK -> Crypto_HsmAvailable=FALSE -> 覆盖软件回退路径;
#   MbedTLS 桩返回 E_OK 并回写确定性伪数据; Det_ReportError 提供计数实现。
add_executable(Crypto_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/mcal/test_crypto.c
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto_stubs/crypto_backend_stubs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/mcal/crypto/src/Crypto.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/crypto/blake2/src/blake2b.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/crypto/blake2/src/blake2s.c
)
# 头文件铁律顺序: 模块目录 / rte/include 必须先于 include/autosar (BEFORE 强制),
# 规避全局 include_directories 中旧版头漂移 (同 NvM_Redundant_UnitTest 的做法)
target_include_directories(Crypto_UnitTest
    BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/mcal/crypto/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/middleware/rte/include
)
target_include_directories(Crypto_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/mcal/crypto/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/middleware/rte/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/autosar
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/crypto/blake2/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)
# framework/unity.h 的 UnityRunTest/UnityBegin 实现在 unity.c (bundled Unity 库)
target_link_libraries(Crypto_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
target_compile_options(Crypto_UnitTest PRIVATE
    -Wall -Wextra -g -O0 -Wno-unused-parameter -Wno-unused-function
)
set_target_properties(Crypto_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
add_test(NAME Crypto_UnitTest COMMAND Crypto_UnitTest)
message(STATUS "  Unit test: Crypto_UnitTest (MCAL Crypto driver, 29 cases)")
