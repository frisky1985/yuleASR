# Integration Test CMakeLists.txt
# 集成测试构建配置

cmake_minimum_required(VERSION 3.14)
project(autosar_integration_tests C)

# 添加Unity测试框架
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../unity unity)

# 测试源文件
set(TEST_SOURCES
    integration_test_runner.c
    test_communication_stack.c
    test_diagnostic_stack.c
    test_safety_stack.c
    test_memory_stack.c
)

# 创建测试可执行文件
add_executable(integration_tests ${TEST_SOURCES})

# 添加头文件路径
target_include_directories(integration_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/Com/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/PduR/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/CanIf/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/Can/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/Dcm/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/DoIP/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/CanTSyn/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/E2E/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/Crc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/Os/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/NvM/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/Fee/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/Fls/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BSW/Ea/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../unity
)

# 链接Unity框架
target_link_libraries(integration_tests unity)

# 测试目标
add_test(NAME integration_tests COMMAND integration_tests)
