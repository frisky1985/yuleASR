#==============================================================================
# OS Module CMake Configuration
# AUTOSAR OS (Operating System) Module with FreeRTOS Support
#==============================================================================

cmake_minimum_required(VERSION 3.14)

#==============================================================================
# OS Module Sources
#==============================================================================
set(OS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Os.c
)

set(OS_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Os.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Os_Cfg.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Os_Types.h
)

#==============================================================================
# Create OS Library
#==============================================================================
add_library(os_module STATIC
    ${OS_SOURCES}
)

#==============================================================================
# Include Directories
#==============================================================================
target_include_directories(os_module PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ETH_DDS_SRC_DIR}/common/types
    ${ETH_DDS_SRC_DIR}/common
)

#==============================================================================
# FreeRTOS Support
#==============================================================================
if(ENABLE_PLATFORM_FREERTOS)
    message(STATUS "OS Module: Enabling FreeRTOS support")
    
    # Add FreeRTOS compile definition
    target_compile_definitions(os_module PUBLIC
        FREERTOS_USED
    )
    
    # Add FreeRTOS include directories
    target_include_directories(os_module PUBLIC
        ${ETH_DDS_SRC_DIR}/platform/freertos/kernel/include
        ${ETH_DDS_SRC_DIR}/platform/freertos/config
    )
    
    # Link FreeRTOS kernel library
    if(TARGET freertos_kernel)
        target_link_libraries(os_module PUBLIC freertos_kernel)
    else()
        message(WARNING "FreeRTOS kernel target not found. Make sure platform/freertos is built first.")
    endif()
    
    # Add portable layer include
    target_include_directories(os_module PUBLIC
        ${ETH_DDS_SRC_DIR}/platform/freertos/port
    )
endif()

#==============================================================================
# Linux Platform Support
#==============================================================================
if(ENABLE_PLATFORM_LINUX)
    message(STATUS "OS Module: Enabling Linux platform support")
    target_compile_definitions(os_module PUBLIC PLATFORM_LINUX)
    target_link_libraries(os_module PUBLIC pthread rt)
endif()

#==============================================================================
# Bare-metal Platform Support
#==============================================================================
if(ENABLE_PLATFORM_BAREMETAL)
    message(STATUS "OS Module: Enabling bare-metal platform support")
    target_compile_definitions(os_module PUBLIC PLATFORM_BAREMETAL)
endif()

#==============================================================================
# Compiler Options
#==============================================================================
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(os_module PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<CONFIG:Debug>:-g3 -O0>
        $<$<CONFIG:Release>:-O2>
    )
endif()

#==============================================================================
# Install
#==============================================================================
install(TARGETS os_module
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${OS_HEADERS}
    DESTINATION include/os
)

message(STATUS "OS Module configured successfully")
