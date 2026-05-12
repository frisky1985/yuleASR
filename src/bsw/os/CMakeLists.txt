# OS (Operating System) CMakeLists.txt
# Based on FreeRTOS

cmake_minimum_required(VERSION 3.16)

file(GLOB OS_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

add_library(Os STATIC ${OS_SOURCES})

target_include_directories(Os
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# FreeRTOS configuration
if(CMAKE_CROSSCOMPILING)
    target_compile_definitions(Os PRIVATE
        FREERTOS_ARM_CM7
    )
endif()

set_target_properties(Os PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/os
)

message(STATUS "OS configured (FreeRTOS)")
