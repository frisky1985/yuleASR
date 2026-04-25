# ETH-DDS Tests CMake Configuration
cmake_minimum_required(VERSION 3.14)

#==============================================================================
# 测试配置
#==============================================================================

# 添加Unity测试框架
add_subdirectory(unity)

# 测试可执行文件目录
set(TEST_BIN_DIR ${CMAKE_BINARY_DIR}/tests)

# 内存泄漏检测选项
option(ENABLE_MEMORY_LEAK_DETECTION "Enable memory leak detection in tests" OFF)

if(ENABLE_MEMORY_LEAK_DETECTION)
    add_definitions(-DENABLE_MEMORY_LEAK_DETECTION)
endif()

#==============================================================================
# 测试框架和模拟库
#==============================================================================

# 模拟库
set(MOCK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_freertos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_eth_driver.c
)

add_library(eth_dds_mocks STATIC ${MOCK_SOURCES})
target_include_directories(eth_dds_mocks PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

# 测试框架
set(FRAMEWORK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/test_runner.c
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/memory_leak_detector.c
)

add_library(test_framework STATIC ${FRAMEWORK_SOURCES})
target_include_directories(test_framework PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
)
target_link_libraries(test_framework PUBLIC unity)

#==============================================================================
# 单元测试 - L1
#==============================================================================

set(UNIT_TEST_SOURCES
    # DDS测试
    unit/dds/test_dds_domain.c
    unit/dds/test_dds_topic.c
    unit/dds/test_dds_pubsub.c
    unit/dds/test_rtps_discovery.c
    unit/dds/test_safety_ecc.c
    
    # OS测试
    unit/os/test_safety_nvm.c
    
    # 传输层测试
    unit/transport/test_eth_transport.c
    
    # 平台层测试
    unit/platform/test_freertos_port.c
)

# 创建单元测试目标
function(add_unit_test TEST_NAME TEST_SOURCE)
    get_filename_component(TEST_FILENAME ${TEST_SOURCE} NAME_WE)
    set(TEST_TARGET "unit_${TEST_FILENAME}")
    
    add_executable(${TEST_TARGET} ${TEST_SOURCE})
    target_link_libraries(${TEST_TARGET} 
        eth_dds 
        eth_dds_mocks
        unity
        test_framework
    )
    target_include_directories(${TEST_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests/unity
        ${CMAKE_SOURCE_DIR}/tests/mocks
        ${CMAKE_SOURCE_DIR}/tests/framework
    )
    set_target_properties(${TEST_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_BIN_DIR}/unit
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_TARGET})
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "unit"
        TIMEOUT 60
    )
endfunction()

# 添加各个单元测试
add_unit_test(test_dds_domain unit/dds/test_dds_domain.c)
add_unit_test(test_dds_topic unit/dds/test_dds_topic.c)
add_unit_test(test_dds_pubsub unit/dds/test_dds_pubsub.c)
add_unit_test(test_rtps_discovery unit/dds/test_rtps_discovery.c)
add_unit_test(test_safety_ecc unit/dds/test_safety_ecc.c)
add_unit_test(test_safety_nvm unit/os/test_safety_nvm.c)
add_unit_test(test_eth_transport unit/transport/test_eth_transport.c)
add_unit_test(test_freertos_port unit/platform/test_freertos_port.c)

#==============================================================================
# 集成测试 - L2
#==============================================================================

set(INTEGRATION_TEST_SOURCES
    integration/test_discovery_integration.c
    integration/test_autosar_dds_integration.c
    integration/test_security_integration.c
    integration/test_tsn_dds_integration.c
)

function(add_integration_test TEST_NAME TEST_SOURCE)
    get_filename_component(TEST_FILENAME ${TEST_SOURCE} NAME_WE)
    set(TEST_TARGET "intg_${TEST_FILENAME}")
    
    add_executable(${TEST_TARGET} ${TEST_SOURCE})
    target_link_libraries(${TEST_TARGET} 
        eth_dds
        eth_dds_mocks
        unity
        test_framework
    )
    target_include_directories(${TEST_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests/unity
        ${CMAKE_SOURCE_DIR}/tests/mocks
    )
    set_target_properties(${TEST_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_BIN_DIR}/integration
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_TARGET})
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "integration"
        TIMEOUT 120
    )
endfunction()

# DDS发现测试
add_integration_test(test_discovery_integration integration/test_discovery_integration.c)

# TSN-DDS集成测试
if(ENABLE_TSN AND ENABLE_DDS)
    add_integration_test(test_tsn_dds_integration integration/test_tsn_dds_integration.c)
endif()

# AUTOSAR-DDS集成测试
if((ENABLE_AUTOSAR_CLASSIC OR ENABLE_AUTOSAR_ADAPTIVE) AND ENABLE_DDS)
    add_integration_test(test_autosar_dds_integration integration/test_autosar_dds_integration.c)
endif()

# 安全通信测试
if(ENABLE_DDS_SECURITY)
    add_integration_test(test_security_integration integration/test_security_integration.c)
endif()

#==============================================================================
# 系统测试 - L3
#==============================================================================

set(SYSTEM_TEST_SOURCES
    system/test_end_to_end.c
    system/test_multi_node.c
    system/test_fault_recovery.c
    system/test_stability.c
)

function(add_system_test TEST_NAME TEST_SOURCE)
    get_filename_component(TEST_FILENAME ${TEST_SOURCE} NAME_WE)
    set(TEST_TARGET "sys_${TEST_FILENAME}")
    
    add_executable(${TEST_TARGET} ${TEST_SOURCE})
    target_link_libraries(${TEST_TARGET} 
        eth_dds
        eth_dds_mocks
        unity
    )
    target_include_directories(${TEST_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests/unity
    )
    set_target_properties(${TEST_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_BIN_DIR}/system
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_TARGET})
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "system"
        TIMEOUT 300
    )
endfunction()

# 系统测试
add_system_test(test_end_to_end system/test_end_to_end.c)
add_system_test(test_multi_node system/test_multi_node.c)
add_system_test(test_fault_recovery system/test_fault_recovery.c)
add_system_test(test_stability system/test_stability.c)

#==============================================================================
# 性能测试 - L3 Performance
#==============================================================================

set(PERFORMANCE_TEST_SOURCES
    performance/latency_test.c
    performance/throughput_test.c
    performance/jitter_test.c
    performance/test_throughput.c
    performance/test_latency.c
    performance/test_resource_usage.c
    performance/test_tsn_determinism.c
)

function(add_performance_test TEST_NAME TEST_SOURCE)
    get_filename_component(TEST_FILENAME ${TEST_SOURCE} NAME_WE)
    set(TEST_TARGET "perf_${TEST_FILENAME}")
    
    add_executable(${TEST_TARGET} ${TEST_SOURCE})
    target_link_libraries(${TEST_TARGET} 
        eth_dds
        eth_dds_mocks
    )
    target_include_directories(${TEST_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests/mocks
    )
    target_compile_options(${TEST_TARGET} PRIVATE
        -O3  # 性能测试使用最高优化
    )
    set_target_properties(${TEST_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_BIN_DIR}/performance
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_TARGET})
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "performance"
        TIMEOUT 600
    )
endfunction()

# 延迟测试
add_performance_test(test_latency performance/latency_test.c)

# 吞吐量测试
add_performance_test(test_throughput performance/throughput_test.c)

# 抖动测试
add_performance_test(test_jitter performance/jitter_test.c)

# 原有性能测试
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_throughput.c)
    add_performance_test(test_throughput_legacy performance/test_throughput.c)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_latency.c)
    add_performance_test(test_latency_legacy performance/test_latency.c)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_resource_usage.c)
    add_performance_test(test_resource_usage performance/test_resource_usage.c)
endif()
if(ENABLE_TSN AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_tsn_determinism.c)
    add_performance_test(test_tsn_determinism performance/test_tsn_determinism.c)
endif()

#==============================================================================
# HIL测试 - L4 (平台特定，通常需要特殊硬件)
#==============================================================================

# HIL测试通常需要连接到实际硬件
# 这里只定义构建目标，实际执行需要特定硬件

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hil/s32g3/test_s32g3_dds.c)
    add_executable(hil_s32g3 hil/s32g3/test_s32g3_dds.c)
    target_link_libraries(hil_s32g3 eth_dds)
    set_target_properties(hil_s32g3 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_BIN_DIR}/hil
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hil/aurix/test_aurix_dds.c)
    add_executable(hil_aurix hil/aurix/test_aurix_dds.c)
    target_link_libraries(hil_aurix eth_dds)
    set_target_properties(hil_aurix PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_BIN_DIR}/hil
    )
endif()

#==============================================================================
# 测试数据目录
#==============================================================================

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_data)

#==============================================================================
# 测试目标
#==============================================================================

# 运行所有测试
add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -j$(nproc)
    DEPENDS eth_dds eth_dds_mocks test_framework
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 运行单元测试
add_custom_target(test-unit
    COMMAND ${CMAKE_CTEST_COMMAND} -L "unit" --output-on-failure -j$(nproc)
    DEPENDS eth_dds eth_dds_mocks test_framework
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 运行集成测试
add_custom_target(test-integration
    COMMAND ${CMAKE_CTEST_COMMAND} -L "integration" --output-on-failure
    DEPENDS eth_dds eth_dds_mocks test_framework
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 运行系统测试
add_custom_target(test-system
    COMMAND ${CMAKE_CTEST_COMMAND} -L "system" --output-on-failure
    DEPENDS eth_dds eth_dds_mocks
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 运行性能测试
add_custom_target(test-performance
    COMMAND ${CMAKE_CTEST_COMMAND} -L "performance" --output-on-failure
    DEPENDS eth_dds eth_dds_mocks
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 生成覆盖率报告
add_custom_target(coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR} 
            --output-file ${CMAKE_BINARY_DIR}/coverage.info
            --exclude "*/tests/*" 
            --exclude "*/build/CMakeFiles/*"
            --exclude "/usr/*"
    COMMAND genhtml ${CMAKE_BINARY_DIR}/coverage.info 
            --output-directory ${CMAKE_BINARY_DIR}/coverage-report
    DEPENDS eth_dds
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 使用gcovr生成覆盖率报告
add_custom_target(coverage-gcovr
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND gcovr --root ${CMAKE_SOURCE_DIR} 
            --html --html-details
            -o ${CMAKE_BINARY_DIR}/coverage-gcovr.html
            --exclude ${CMAKE_SOURCE_DIR}/tests/
            --exclude ${CMAKE_BINARY_DIR}/CMakeFiles/
    DEPENDS eth_dds
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

#==============================================================================
# 测试配置汇总
#==============================================================================

message(STATUS "Tests configured:")
message(STATUS "  - Unit tests: DDS, OS, Transport, Platform")
message(STATUS "  - Integration tests: Discovery, AUTOSAR, Security, TSN")
message(STATUS "  - System tests: E2E, Multi-node, Fault recovery, Stability")
message(STATUS "  - Performance tests: Latency, Throughput, Jitter")
message(STATUS "  - Custom targets: test, test-unit, test-integration, test-system, test-performance, coverage")
