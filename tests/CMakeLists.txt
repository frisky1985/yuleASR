# ETH-DDS 集成项目测试CMakeLists.txt

cmake_minimum_required(VERSION 3.14)
project(eth_dds_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Unity框架
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# 测试定义
add_compile_definitions(UNIT_TEST)

# 头文件路径
include_directories(
    ${CMAKE_SOURCE_DIR}/../include/telemetry
    ${CMAKE_SOURCE_DIR}/../include/dds
    ${CMAKE_SOURCE_DIR}/../include/eth
    ${CMAKE_SOURCE_DIR}/../include/diag
    ${CMAKE_SOURCE_DIR}/mocks
)

# 测试可执行文件
add_executable(eth_dds_tests
    test_main.c
    test_telemetry.c
    test_dds.c
    test_eth.c
    test_diag.c
    ../src/telemetry/telemetry.c
    ../src/telemetry/telemetry_diag.c
)

# 链接Unity
target_link_libraries(eth_dds_tests unity)

# 测试配置
enable_testing()
add_test(NAME eth_dds_tests COMMAND eth_dds_tests)

# 详细测试输出
add_custom_target(test_verbose
    COMMAND ${CMAKE_CTEST_COMMAND} -V
    DEPENDS eth_dds_tests
)

# 覆盖率报告 (如果安装了gcov)
find_program(GCOV gcov)
if(GCOV)
    target_compile_options(eth_dds_tests PRIVATE --coverage)
    target_link_options(eth_dds_tests PRIVATE --coverage)
    
    add_custom_target(coverage
        COMMAND ${CMAKE_CTEST_COMMAND}
        COMMAND gcov -r ${CMAKE_SOURCE_DIR}/../src/telemetry/*.c
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
