# cmake/modules/options.cmake
# Build options and feature toggles for ETH-DDS Integration

#==============================================================================
# Project Version
#==============================================================================
set(PROJECT_VERSION_MAJOR 2)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

#==============================================================================
# Build Options
#==============================================================================
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build unit and integration tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" OFF)
option(BUILD_DOCUMENTATION "Build documentation with Doxygen" OFF)
option(BUILD_PACKAGE "Enable package generation" OFF)

#==============================================================================
# Feature Module Options
#==============================================================================
option(ENABLE_ETHERNET "Enable Ethernet driver support" ON)
option(ENABLE_DDS "Enable DDS middleware support" ON)
option(ENABLE_TSN "Enable Time-Sensitive Networking support" ON)
option(ENABLE_AUTOSAR_CLASSIC "Enable AUTOSAR Classic RTE integration" ON)
option(ENABLE_AUTOSAR_ADAPTIVE "Enable AUTOSAR Adaptive ara::com integration" ON)
option(ENABLE_DDS_SECURITY "Enable DDS Security extensions" ON)
option(ENABLE_DDS_ADVANCED "Enable advanced DDS features (ownership, persistence)" ON)
option(ENABLE_DDS_RUNTIME "Enable DDS Runtime monitoring" ON)

#==============================================================================
# Safety Module Options
#==============================================================================
option(ENABLE_SAFETY_RAM "Enable RAM Safety/ECC protection module" ON)
option(ENABLE_SAFETY_NVM "Enable Non-Volatile Memory safety module" ON)
option(ENABLE_SAFETY_SAFERAM "Enable SafeRAM memory protection" ON)

#==============================================================================
# Platform Options
#==============================================================================
option(ENABLE_PLATFORM_LINUX "Enable Linux platform support" ON)
option(ENABLE_PLATFORM_FREERTOS "Enable FreeRTOS platform support" OFF)
option(ENABLE_PLATFORM_BAREMETAL "Enable Bare-metal platform support" OFF)

#==============================================================================
# Hardware Platform Target Selection
#==============================================================================
set(PLATFORM_TARGET "posix" CACHE STRING "Target hardware platform")
set_property(CACHE PLATFORM_TARGET PROPERTY STRINGS 
    "s32g3" 
    "aurix_tc3xx" 
    "posix"
)

message(STATUS "Platform Target: ${PLATFORM_TARGET}")

#==============================================================================
# Platform-specific Settings
#==============================================================================
if(PLATFORM_TARGET STREQUAL "s32g3")
    set(BOARD_S32G3 ON)
    set(ENABLE_PLATFORM_BAREMETAL ON)
    set(ENABLE_PLATFORM_LINUX OFF)
    add_compile_definitions(BOARD_S32G3 PLATFORM_BAREMETAL)
    message(STATUS "  Board: NXP S32G3")
    message(STATUS "  OS: Bare-metal")
    
elseif(PLATFORM_TARGET STREQUAL "aurix_tc3xx")
    set(BOARD_AURIX_TC3XX ON)
    set(ENABLE_PLATFORM_BAREMETAL ON)
    set(ENABLE_PLATFORM_LINUX OFF)
    add_compile_definitions(BOARD_AURIX_TC3XX PLATFORM_BAREMETAL)
    message(STATUS "  Board: Infineon AURIX TC3xx")
    message(STATUS "  OS: Bare-metal")
    
elseif(PLATFORM_TARGET STREQUAL "posix")
    set(BOARD_POSIX ON)
    set(ENABLE_PLATFORM_LINUX ON)
    set(ENABLE_PLATFORM_BAREMETAL OFF)
    add_compile_definitions(BOARD_POSIX PLATFORM_LINUX)
    message(STATUS "  Board: POSIX/x86_64")
    message(STATUS "  OS: Linux")
    
else()
    message(FATAL_ERROR "Unknown PLATFORM_TARGET: ${PLATFORM_TARGET}. "
                        "Valid options: s32g3, aurix_tc3xx, posix")
endif()

#==============================================================================
# Advanced Options
#==============================================================================
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
option(ENABLE_PROFILING "Enable performance profiling" OFF)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)
option(ENABLE_STATIC_ANALYSIS "Enable static analysis tools" OFF)
option(ENABLE_LTO "Enable Link Time Optimization" OFF)

#==============================================================================
# Feature Dependencies Check
#==============================================================================
# AUTOSAR requires DDS
if(ENABLE_AUTOSAR_CLASSIC OR ENABLE_AUTOSAR_ADAPTIVE)
    if(NOT ENABLE_DDS)
        message(WARNING "AUTOSAR integration requires DDS. Enabling DDS.")
        set(ENABLE_DDS ON)
    endif()
endif()

# TSN requires Ethernet
if(ENABLE_TSN)
    if(NOT ENABLE_ETHERNET)
        message(WARNING "TSN requires Ethernet. Enabling Ethernet.")
        set(ENABLE_ETHERNET ON)
    endif()
endif()

# DDS Security requires DDS
if(ENABLE_DDS_SECURITY)
    if(NOT ENABLE_DDS)
        message(WARNING "DDS Security requires DDS. Enabling DDS.")
        set(ENABLE_DDS ON)
    endif()
endif()

#==============================================================================
# Print Configuration Summary
#==============================================================================
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "")
message(STATUS "  Features:")
message(STATUS "    Ethernet: ${ENABLE_ETHERNET}")
message(STATUS "    DDS: ${ENABLE_DDS}")
message(STATUS "    TSN: ${ENABLE_TSN}")
message(STATUS "    AUTOSAR Classic: ${ENABLE_AUTOSAR_CLASSIC}")
message(STATUS "    AUTOSAR Adaptive: ${ENABLE_AUTOSAR_ADAPTIVE}")
message(STATUS "    DDS Security: ${ENABLE_DDS_SECURITY}")
message(STATUS "    DDS Advanced: ${ENABLE_DDS_ADVANCED}")
message(STATUS "    DDS Runtime: ${ENABLE_DDS_RUNTIME}")
message(STATUS "")
message(STATUS "  Safety Modules:")
message(STATUS "    RAM ECC: ${ENABLE_SAFETY_RAM}")
message(STATUS "    NVM: ${ENABLE_SAFETY_NVM}")
message(STATUS "    SafeRAM: ${ENABLE_SAFETY_SAFERAM}")
message(STATUS "")
