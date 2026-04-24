#==============================================================================
# src/CMakeLists.txt - Source directory build configuration
#==============================================================================

# FreeRTOS platform must be built before OS module
cmake_minimum_required(VERSION 3.14)

#==============================================================================
# Platform Selection
#==============================================================================
if(ENABLE_PLATFORM_FREERTOS)
    # FreeRTOS must be built before OS module
    add_subdirectory(platform/freertos)
endif()

#==============================================================================
# BSW (Basic Software) Modules
#==============================================================================
# OS Module - depends on platform
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/os)
    add_subdirectory(bsw/os)
endif()

# NVM Module
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/nvm)
    add_subdirectory(nvm)
endif()

# Safety RAM Module
if(ENABLE_SAFETY_RAM AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/safety/saferam)
    add_subdirectory(safety/saferam)
endif()

#==============================================================================
# Platform Abstraction Layer
#==============================================================================
# Linux platform support
if(ENABLE_PLATFORM_LINUX AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux)
    add_subdirectory(platform/linux)
endif()

# Bare-metal platform support
if(ENABLE_PLATFORM_BAREMETAL AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/platform/baremetal)
    add_subdirectory(platform/baremetal)
endif()
