# CMakeLists.txt for DDS-Security unit tests
# 集成: 由 src/dds/CMakeLists.txt 在 BUILD_TESTING 时挂载
# Unity 框架位于 tests/unit/framework (tests/unity 为兼容别名)

cmake_minimum_required(VERSION 3.16)

# Unity 路径 (兼容旧 tests/unity 引用)
if(EXISTS ${PROJECT_SOURCE_DIR}/tests/unity)
    set(DDS_TEST_UNITY_DIR ${PROJECT_SOURCE_DIR}/tests/unity)
elseif(EXISTS ${PROJECT_SOURCE_DIR}/tests/unit/framework)
    set(DDS_TEST_UNITY_DIR ${PROJECT_SOURCE_DIR}/tests/unit/framework)
else()
    set(DDS_TEST_UNITY_DIR ${PROJECT_SOURCE_DIR}/tests/unity)
endif()

# Security tests
add_executable(test_dds_security
    test_dds_auth.c
    test_dds_access.c
    test_dds_crypto.c
    test_dds_security_manager.c
    test_runner.c
    ${DDS_TEST_UNITY_DIR}/unity.c
)

target_include_directories(test_dds_security
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../core
        ${CMAKE_CURRENT_SOURCE_DIR}/../../rtps
        ${CMAKE_CURRENT_SOURCE_DIR}/../../pubsub
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../common/types
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../common/log
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../common/utils
        ${DDS_TEST_UNITY_DIR}
        ${PROJECT_SOURCE_DIR}/include/autosar
)

target_link_libraries(test_dds_security
    PRIVATE
        eth_dds_middleware
        mbedtls
)

# Add test
add_test(NAME dds_security COMMAND test_dds_security)
