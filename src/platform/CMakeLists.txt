# Platform Layer CMakeLists.txt
# Platform support for S32K312

cmake_minimum_required(VERSION 3.16)

# Global includes for Std_Types.h, etc.
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/services/det/include
    ${CMAKE_SOURCE_DIR}/src/bsw/services/ramsafety/include
    ${CMAKE_SOURCE_DIR}/src/bsw/services/nvm/include
    ${CMAKE_SOURCE_DIR}/src/bsw/mcal/mcu/include
    ${CMAKE_SOURCE_DIR}/config/input/services
    ${CMAKE_SOURCE_DIR}/include/autosar
)

option(PLATFORM_S32K312 "Enable S32K312 platform support" ON)

if(NOT PLATFORM_S32K312)
    message(STATUS "Platform support disabled")
    return()
endif()

# S32K312 platform sources
file(GLOB S32K312_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/s32k312/src/*.c
)

# S32K312 startup assembly (if exists)
file(GLOB S32K312_ASM
    ${CMAKE_CURRENT_SOURCE_DIR}/s32k312/src/*.S
    ${CMAKE_CURRENT_SOURCE_DIR}/s32k312/src/*.s
)

# Create platform library
add_library(Platform STATIC ${S32K312_SOURCES} ${S32K312_ASM})

target_include_directories(Platform
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/s32k312/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/mcal/mcu/include
)

target_compile_definitions(Platform PRIVATE
    S32K312
    S32K3XX
)

# Assembly flags for startup code
if(S32K312_ASM)
    set_source_files_properties(${S32K312_ASM} PROPERTIES
        LANGUAGE ASM
    )
endif()

set_target_properties(Platform PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/platform
)

# Generate linker script if template exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/s32k312/linker/s32k312.ld.in)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/s32k312/linker/s32k312.ld.in
        ${CMAKE_BINARY_DIR}/s32k312.ld
        @ONLY
    )
    message(STATUS "Generated linker script: s32k312.ld")
endif()

message(STATUS "Platform configured: S32K312")
