# Telemetry模块测试CMakeLists.txt

cmake_minimum_required(VERSION 3.14)
project(telemetry_tests C)

# 设置C标准
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

# 头文件路径
include_directories(
    ${CMAKE_SOURCE_DIR}/../include/telemetry
)

# 测试可执行文件
add_executable(telemetry_tests
    telemetry_test.c
    ../src/telemetry/telemetry.c
)

# 链接Unity
target_link_libraries(telemetry_tests unity)

# 测试配置
enable_testing()
add_test(NAME telemetry_tests COMMAND telemetry_tests)

# 详细测试输出
add_custom_target(test_verbose
    COMMAND ${CMAKE_CTEST_COMMAND} -V
    DEPENDS telemetry_tests
)
