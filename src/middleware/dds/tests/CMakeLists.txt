# src/middleware/dds/tests/CMakeLists.txt
# DDS 核心层单元测试 — pubsub 全链路 + rtps_wire roundtrip
# 集成: 由 src/middleware/dds/CMakeLists.txt 在 BUILD_TESTING 时挂载

cmake_minimum_required(VERSION 3.16)

# Unity 路径
if(EXISTS ${PROJECT_SOURCE_DIR}/tests/unit/framework)
    set(DDS_UNITY_DIR ${PROJECT_SOURCE_DIR}/tests/unit/framework)
else()
    set(DDS_UNITY_DIR ${PROJECT_SOURCE_DIR}/tests/unity)
endif()

add_executable(test_dds_core
    test_dds_core.c
    ${DDS_UNITY_DIR}/unity.c
)

target_include_directories(test_dds_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DDS_UNITY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../rtps
    ${CMAKE_CURRENT_SOURCE_DIR}/../pubsub
    ${CMAKE_CURRENT_SOURCE_DIR}/../security
    ${CMAKE_CURRENT_SOURCE_DIR}/../runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/types
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/utils
    ${PROJECT_SOURCE_DIR}/include/autosar
)
target_link_libraries(test_dds_core PRIVATE eth_dds_middleware)

add_test(NAME dds_core COMMAND test_dds_core)

# 聚合目标
add_custom_target(run_dds_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_dds_core test_dds_security
)
