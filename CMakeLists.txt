# YuleTech AutoSAR BSW Platform - 项目顶层 CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# 项目信息
project(YuleTech_AutoSAR_BSW
    VERSION 1.0.0
    DESCRIPTION "YuleTech AutoSAR BSW Platform for NXP i.MX8M Mini"
    LANGUAGES C CXX
)

# C 标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 选项
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_SANITIZER "Enable address sanitizer" OFF)

# 编译器选项
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror=return-type
        -Werror=uninitialized
        -Werror=implicit-function-declaration
        -Werror=incompatible-pointer-types
    )
endif()

# 覆盖率配置
if(ENABLE_COVERAGE AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

# 内存检查器
if(ENABLE_SANITIZER AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# 添加子目录
add_subdirectory(tools/build)

# 测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 打印配置信息
message(STATUS "YuleTech AutoSAR BSW Platform v${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Code coverage: ${ENABLE_COVERAGE}")
message(STATUS "  Address sanitizer: ${ENABLE_SANITIZER}")
