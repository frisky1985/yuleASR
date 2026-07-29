# MQTT TLS模块单元测试 CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

# 测试项目名称
project(mqtt_tls_tests C)

# Unity测试框架路径
set(UNITY_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../third_party/unity)

# 源文件目录
set(MQTT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)
set(MQTT_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(BSW_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(MCAL_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../mcal)

# 测试源文件
set(TEST_SOURCES
    test_mqtt_tls.c
    ${UNITY_ROOT}/src/unity.c
)

# 被测源文件
set(SOURCES_UNDER_TEST
    ${MQTT_SRC_DIR}/Mqtt_Tls.c
    ${MQTT_SRC_DIR}/Mqtt_CertMgr.c
)

# 创建测试可执行文件
add_executable(mqtt_tls_tests
    ${TEST_SOURCES}
    ${SOURCES_UNDER_TEST}
)

# 头文件搜索路径
target_include_directories(mqtt_tls_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${UNITY_ROOT}/src
    ${MQTT_INC_DIR}
    ${BSW_INC_DIR}
    ${MCAL_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)

# 测试特定的头文件搜索路径(覆盖原始头文件)
target_include_directories(mqtt_tls_tests BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)

# 编译选项
target_compile_options(mqtt_tls_tests PRIVATE
    -Wall
    -Wextra
    -Werror
    -g
    -O0
    -fprofile-arcs
    -ftest-coverage
    -DMQTT_SUPPORT_TLS=STD_ON
    -DUNIT_TEST
)

# 链接选项
target_link_options(mqtt_tls_tests PRIVATE
    -fprofile-arcs
    -ftest-coverage
)

# 添加库
target_link_libraries(mqtt_tls_tests PRIVATE
    gcov
)

# 启用测试
enable_testing()

# 添加测试
add_test(NAME mqtt_tls_tests COMMAND mqtt_tls_tests)

# 覆盖率报告目标
add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E echo "Running coverage analysis..."
    COMMAND ./mqtt_tls_tests
    COMMAND gcov -b -c ${SOURCES_UNDER_TEST}
    COMMAND lcov --capture --directory . --output-file coverage.info
    COMMAND lcov --remove coverage.info '/usr/*' '*/unity/*' '*/test/*' --output-file coverage_filtered.info
    COMMAND genhtml coverage_filtered.info --output-directory coverage_report
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS mqtt_tls_tests
)
