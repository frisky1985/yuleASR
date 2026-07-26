#===================================================================================================
# Project              : YuleTech AutoSAR BSW
# Module               : Mock HAL CMake Build Target
#
# (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
# All Rights Reserved.
#
# @file CMakeLists.txt
# @brief CMake build configuration for mock HAL library
#
# Usage:
#   In MCAL unit test CMakeLists.txt:
#     add_subdirectory(${CMAKE_SOURCE_DIR}/tests/mock ${CMAKE_BINARY_DIR}/mock_hal)
#     target_link_libraries(my_test PRIVATE MockHAL)
#     target_compile_definitions(my_test PRIVATE
#       REG_READ32=mock_hal_read32
#       REG_WRITE32=mock_hal_write32
#       REG_READ16=mock_hal_read16
#       REG_WRITE16=mock_hal_write16
#       REG_READ8=mock_hal_read8
#       REG_WRITE8=mock_hal_write8
#       REG_RMW32=mock_hal_rmw32
#     )
#===================================================================================================

cmake_minimum_required(VERSION 3.10)

# Mock HAL library — memory-backed register emulation
project(MockHAL C)

# Source files
set(MOCK_HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_hal.c
)

# Header files
set(MOCK_HAL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_hal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_hal_config.h
)

# Create static library
add_library(MockHAL STATIC
    ${MOCK_HAL_SOURCES}
    ${MOCK_HAL_HEADERS}
)

# Include the mock directory itself
target_include_directories(MockHAL
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compilation options for host (macOS/Linux) testing
target_compile_options(MockHAL PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -g
    -O0
)

# Coverage support (optional)
option(ENABLE_COVERAGE "Enable coverage instrumentation" OFF)
if(ENABLE_COVERAGE)
    target_compile_options(MockHAL PRIVATE
        -fprofile-arcs
        -ftest-coverage
    )
    target_link_libraries(MockHAL PRIVATE gcov)
endif()

# Properties
set_target_properties(MockHAL PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

message(STATUS "MockHAL library configured — ${MOCK_HAL_SOURCES}")
