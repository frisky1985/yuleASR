# Integration Test CMakeLists.txt
# 集成测试构建配置

cmake_minimum_required(VERSION 3.14)
project(autosar_integration_tests C)

# 添加Unity测试框架 (if available)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../unity/CMakeLists.txt)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../unity unity)
endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/com/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/pdur/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/ecual/canif/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/mcal/can/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/dcm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/doip/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/cantsyn/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/e2e/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/crc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/nvm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/fee/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/fls/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/ea/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../unit/framework
)

# 链接Unity框架 (if available)
if(TARGET unity)
    target_link_libraries(integration_tests unity)
endif()

# 测试目标
add_test(NAME integration_tests COMMAND integration_tests)
