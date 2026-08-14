cmake_minimum_required(VERSION 3.14)

# 项目信息
project(microdds
    VERSION 0.1.0
    DESCRIPTION "轻量级DDS实现 - 专为资源受限MCU设计"
    LANGUAGES C
)

# C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -O2
        -g
    )
endif()

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 源文件
set(MICRODDS_SOURCES
    src/core/domain.c
    src/core/topic.c
    src/core/publisher.c
    src/core/subscriber.c
    src/core/writer.c
    src/core/reader.c
    src/qos/qos_policy.c
    src/transport/udp.c
    src/utils/buffer_pool.c
    src/serialization/cdr.c
)

# 静态库
add_library(microdds STATIC ${MICRODDS_SOURCES})

# 安装目标
install(TARGETS microdds
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
)

# 测试配置
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例配置
option(BUILD_EXAMPLES "构建示例程序" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples/hello_world)
endif()
