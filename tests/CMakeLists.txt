# ETH-DDS Tests CMake Configuration

cmake_minimum_required(VERSION 3.14)

#==============================================================================
# 测试配置
#==============================================================================

# 添加Unity测试框架
add_subdirectory(unity)

# 测试可执行文件目录
set(TEST_BIN_DIR ${CMAKE_BINARY_DIR}/tests)

#==============================================================================
# 单元测试
#==============================================================================

# 创建单元测试执行文件
function(add_unit_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} 
        eth_dds 
        unity
    )
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests/unity
    )
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_BIN_DIR}/unit
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Common模块测试
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_common.c)
    add_unit_test(test_common unit/test_common.c)
endif()

# Ethernet模块测试
if(ENABLE_ETHERNET AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_ethernet.c)
    add_unit_test(test_ethernet unit/test_ethernet.c)
endif()

# DDS模块测试
if(ENABLE_DDS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_dds.c)
    add_unit_test(test_dds unit/test_dds.c)
endif()

# DDS安全模块测试
if(ENABLE_DDS_SECURITY AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_dds_security.c)
    add_unit_test(test_dds_security unit/test_dds_security.c)
endif()

# TSN模块测试
if(ENABLE_TSN AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_tsn.c)
    add_unit_test(test_tsn unit/test_tsn.c)
endif()

# Transport模块测试
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_transport.c)
    add_unit_test(test_transport unit/test_transport.c)
endif()

# AUTOSAR模块测试
if((ENABLE_AUTOSAR_CLASSIC OR ENABLE_AUTOSAR_ADAPTIVE) AND 
   EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_autosar.c)
    add_unit_test(test_autosar unit/test_autosar.c)
endif()

#==============================================================================
# 整合测试
#==============================================================================

function(add_integration_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} 
        eth_dds
        unity
    )
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests/unity
    )
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_BIN_DIR}/integration
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# DDS发现测试
add_integration_test(test_discovery_integration 
    integration/test_discovery_integration.c)

# TSN-DDS集成测试
if(ENABLE_TSN AND ENABLE_DDS)
    add_integration_test(test_tsn_dds_integration 
        integration/test_tsn_dds_integration.c)
endif()

# AUTOSAR-DDS集成测试
if((ENABLE_AUTOSAR_CLASSIC OR ENABLE_AUTOSAR_ADAPTIVE) AND ENABLE_DDS)
    add_integration_test(test_autosar_dds_integration 
        integration/test_autosar_dds_integration.c)
endif()

# 安全通信测试
if(ENABLE_DDS_SECURITY)
    add_integration_test(test_security_integration 
        integration/test_security_integration.c)
endif()

#==============================================================================
# 系统测试
#==============================================================================

function(add_system_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} 
        eth_dds
        unity
    )
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests/unity
    )
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_BIN_DIR}/system
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "system"
    )
endfunction()

# 端到端系统测试
add_system_test(test_end_to_end system/test_end_to_end.c)

# 多节点系统测试
add_system_test(test_multi_node system/test_multi_node.c)

# 故障恢复测试
add_system_test(test_fault_recovery system/test_fault_recovery.c)

# 长时间稳定性测试
add_system_test(test_stability system/test_stability.c)

#==============================================================================
# 性能测试
#==============================================================================

function(add_performance_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} 
        eth_dds
    )
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TEST_BIN_DIR}/performance
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "performance"
    )
endfunction()

# 吞吐量测试
add_performance_test(test_throughput performance/test_throughput.c)

# 延迟测试
add_performance_test(test_latency performance/test_latency.c)

# TSN确定性测试
if(ENABLE_TSN)
    add_performance_test(test_tsn_determinism 
        performance/test_tsn_determinism.c)
endif()

# 资源占用测试
add_performance_test(test_resource_usage performance/test_resource_usage.c)

#==============================================================================
# 测试配置
#==============================================================================

# 设置测试环境
set(ENV{ETH_DDS_TEST_MODE} "1")

# 添加测试数据目录
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_data)

# 测试标签
add_custom_target(test-unit
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^test_" -L "unit" --output-on-failure
    DEPENDS eth_dds
)

add_custom_target(test-integration
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^test_" -L "integration" --output-on-failure
    DEPENDS eth_dds
)

add_custom_target(test-system
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^test_" -L "system" --output-on-failure
    DEPENDS eth_dds
)

add_custom_target(test-performance
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^test_" -L "performance" --output-on-failure
    DEPENDS eth_dds
)

add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS eth_dds
)

message(STATUS "Tests configured:")
message(STATUS "  - Unit tests")
message(STATUS "  - Integration tests")
message(STATUS "  - System tests")
message(STATUS "  - Performance tests")
