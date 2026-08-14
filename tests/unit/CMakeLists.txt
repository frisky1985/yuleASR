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

# EthSwt 单元测试 (B2: VLAN/流控/统计/镜像补全)
add_subdirectory(ecual)

# TcpIp 单元测试 (B1: API 补全/多连接/状态机/VLAN/统计)
add_subdirectory(services/tcpip)

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
