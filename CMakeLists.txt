cmake_minimum_required(VERSION 3.14)
project(eth_dds_integration VERSION 2.0.0 LANGUAGES C CXX)

#==============================================================================
# Module Path
#==============================================================================
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

#==============================================================================
# Include Build Modules
#==============================================================================
message(STATUS "Configuring ETH-DDS Integration Framework")
message(STATUS "Project version: ${PROJECT_VERSION}")
message(STATUS "")

# Build options and platform selection
include(options)

# Compiler configuration
include(compiler)

# External dependencies
include(dependencies)

#==============================================================================
# Project Directories
#==============================================================================
set(ETH_DDS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ETH_DDS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(ETH_DDS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(ETH_DDS_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(ETH_DDS_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(ETH_DDS_TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools)

#==============================================================================
# Source Code Collection
#==============================================================================
# Common utilities
file(GLOB_RECURSE COMMON_SOURCES
    ${ETH_DDS_SRC_DIR}/common/*.c
)
file(GLOB_RECURSE COMMON_HEADERS
    ${ETH_DDS_SRC_DIR}/common/*.h
)

# Ethernet module
if(ENABLE_ETHERNET)
    file(GLOB_RECURSE ETHERNET_SOURCES
        ${ETH_DDS_SRC_DIR}/ethernet/*.c
    )
    file(GLOB_RECURSE ETHERNET_HEADERS
        ${ETH_DDS_SRC_DIR}/ethernet/*.h
    )
endif()

# DDS module
if(ENABLE_DDS)
    file(GLOB_RECURSE DDS_SOURCES
        ${ETH_DDS_SRC_DIR}/dds/core/*.c
        ${ETH_DDS_SRC_DIR}/dds/rtps/*.c
        ${ETH_DDS_SRC_DIR}/dds/pubsub/*.c
    )
    file(GLOB_RECURSE DDS_HEADERS
        ${ETH_DDS_SRC_DIR}/dds/core/*.h
        ${ETH_DDS_SRC_DIR}/dds/rtps/*.h
        ${ETH_DDS_SRC_DIR}/dds/pubsub/*.h
    )
    
    if(ENABLE_DDS_SECURITY)
        file(GLOB_RECURSE DDS_SECURITY_SOURCES
            ${ETH_DDS_SRC_DIR}/dds/security/*.c
        )
        file(GLOB_RECURSE DDS_SECURITY_HEADERS
            ${ETH_DDS_SRC_DIR}/dds/security/*.h
        )
        list(APPEND DDS_SOURCES ${DDS_SECURITY_SOURCES})
        list(APPEND DDS_HEADERS ${DDS_SECURITY_HEADERS})
    endif()
    
    if(ENABLE_DDS_RUNTIME)
        file(GLOB_RECURSE DDS_RUNTIME_SOURCES
            ${ETH_DDS_SRC_DIR}/dds/runtime/*.c
        )
        file(GLOB_RECURSE DDS_RUNTIME_HEADERS
            ${ETH_DDS_SRC_DIR}/dds/runtime/*.h
        )
        list(APPEND DDS_SOURCES ${DDS_RUNTIME_SOURCES})
        list(APPEND DDS_HEADERS ${DDS_RUNTIME_HEADERS})
    endif()
endif()

# Transport module
file(GLOB_RECURSE TRANSPORT_SOURCES
    ${ETH_DDS_SRC_DIR}/transport/*.c
)
file(GLOB_RECURSE TRANSPORT_HEADERS
    ${ETH_DDS_SRC_DIR}/transport/*.h
)

# TSN module
if(ENABLE_TSN)
    file(GLOB_RECURSE TSN_SOURCES
        ${ETH_DDS_SRC_DIR}/tsn/*.c
    )
    file(GLOB_RECURSE TSN_HEADERS
        ${ETH_DDS_SRC_DIR}/tsn/*.h
    )
endif()

# AUTOSAR module
if(ENABLE_AUTOSAR_CLASSIC)
    file(GLOB_RECURSE AUTOSAR_CLASSIC_SOURCES
        ${ETH_DDS_SRC_DIR}/autosar/classic/*.c
        ${ETH_DDS_SRC_DIR}/autosar/e2e/*.c
        ${ETH_DDS_SRC_DIR}/autosar/arxml/*.c
    )
    file(GLOB_RECURSE AUTOSAR_CLASSIC_HEADERS
        ${ETH_DDS_SRC_DIR}/autosar/classic/*.h
        ${ETH_DDS_SRC_DIR}/autosar/e2e/*.h
        ${ETH_DDS_SRC_DIR}/autosar/arxml/*.h
    )
endif()

if(ENABLE_AUTOSAR_ADAPTIVE)
    file(GLOB_RECURSE AUTOSAR_ADAPTIVE_SOURCES
        ${ETH_DDS_SRC_DIR}/autosar/adaptive/*.c
    )
    file(GLOB_RECURSE AUTOSAR_ADAPTIVE_HEADERS
        ${ETH_DDS_SRC_DIR}/autosar/adaptive/*.h
    )
endif()

# Safety modules
if(ENABLE_SAFETY_RAM)
    file(GLOB_RECURSE SAFETY_RAM_SOURCES
        ${ETH_DDS_SRC_DIR}/safety/ram/*.c
    )
    file(GLOB_RECURSE SAFETY_RAM_HEADERS
        ${ETH_DDS_SRC_DIR}/safety/ram/*.h
    )
endif()

if(ENABLE_SAFETY_NVM)
    file(GLOB_RECURSE SAFETY_NVM_SOURCES
        ${ETH_DDS_SRC_DIR}/safety/nvm/*.c
    )
    file(GLOB_RECURSE SAFETY_NVM_HEADERS
        ${ETH_DDS_SRC_DIR}/safety/nvm/*.h
    )
endif()

if(ENABLE_SAFETY_SAFERAM)
    file(GLOB_RECURSE SAFETY_SAFERAM_SOURCES
        ${ETH_DDS_SRC_DIR}/safety/saferam/*.c
    )
    file(GLOB_RECURSE SAFETY_SAFERAM_HEADERS
        ${ETH_DDS_SRC_DIR}/safety/saferam/*.h
    )
endif()

# Platform-specific sources
if(ENABLE_PLATFORM_BAREMETAL)
    file(GLOB_RECURSE PLATFORM_COMMON_SOURCES
        ${ETH_DDS_SRC_DIR}/platform/common/*.c
    )
    
    if(PLATFORM_TARGET STREQUAL "s32g3")
        file(GLOB_RECURSE PLATFORM_SOURCES
            ${ETH_DDS_SRC_DIR}/platform/s32g3/*.c
        )
    elseif(PLATFORM_TARGET STREQUAL "aurix_tc3xx")
        file(GLOB_RECURSE PLATFORM_SOURCES
            ${ETH_DDS_SRC_DIR}/platform/aurix/*.c
        )
    endif()
endif()

#==============================================================================
# Create Main Library
#==============================================================================
add_library(eth_dds
    ${COMMON_SOURCES}
    $<$<BOOL:${ENABLE_ETHERNET}>:${ETHERNET_SOURCES}>
    $<$<BOOL:${ENABLE_DDS}>:${DDS_SOURCES}>
    ${TRANSPORT_SOURCES}
    $<$<BOOL:${ENABLE_TSN}>:${TSN_SOURCES}>
    $<$<BOOL:${ENABLE_AUTOSAR_CLASSIC}>:${AUTOSAR_CLASSIC_SOURCES}>
    $<$<BOOL:${ENABLE_AUTOSAR_ADAPTIVE}>:${AUTOSAR_ADAPTIVE_SOURCES}>
    $<$<BOOL:${ENABLE_SAFETY_RAM}>:${SAFETY_RAM_SOURCES}>
    $<$<BOOL:${ENABLE_SAFETY_NVM}>:${SAFETY_NVM_SOURCES}>
    $<$<BOOL:${ENABLE_SAFETY_SAFERAM}>:${SAFETY_SAFERAM_SOURCES}>
    $<$<BOOL:${ENABLE_PLATFORM_BAREMETAL}>:${PLATFORM_COMMON_SOURCES}>
    $<$<BOOL:${ENABLE_PLATFORM_BAREMETAL}>:${PLATFORM_SOURCES}>
)

set_target_properties(eth_dds PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "eth_dds"
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

#==============================================================================
# Include Directories
#==============================================================================
target_include_directories(eth_dds PUBLIC
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/common/types>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/common/utils>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/common/log>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/ethernet>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/ethernet/driver>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/dds/core>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/dds/rtps>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/dds/pubsub>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/dds/security>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/dds/runtime>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/transport>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn/gptp>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn/tas>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn/cbs>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn/fp>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/tsn/srp>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/autosar/classic>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/autosar/adaptive>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/autosar/e2e>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/autosar/arxml>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/safety/ram>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/safety/saferam>
    $<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/safety/nvm>
    $<$<BOOL:${ENABLE_PLATFORM_FREERTOS}>:$<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/platform/freertos/kernel/include>>
    $<$<BOOL:${ENABLE_PLATFORM_FREERTOS}>:$<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/platform/freertos/config>>
    $<$<BOOL:${ENABLE_PLATFORM_FREERTOS}>:$<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/platform/freertos/port>>
    $<$<BOOL:${ENABLE_PLATFORM_BAREMETAL}>:$<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/platform/s32g3>>
    $<$<BOOL:${ENABLE_PLATFORM_BAREMETAL}>:$<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/platform/aurix>>
    $<$<BOOL:${ENABLE_PLATFORM_BAREMETAL}>:$<BUILD_INTERFACE:${ETH_DDS_SRC_DIR}/platform/common>>
    $<INSTALL_INTERFACE:include/eth_dds>
)

#==============================================================================
# Compile Definitions
#==============================================================================
target_compile_definitions(eth_dds PUBLIC
    ETH_DDS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ETH_DDS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    ETH_DDS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    $<$<BOOL:${ENABLE_ETHERNET}>:ENABLE_ETHERNET>
    $<$<BOOL:${ENABLE_DDS}>:ENABLE_DDS>
    $<$<BOOL:${ENABLE_TSN}>:ENABLE_TSN>
    $<$<BOOL:${ENABLE_AUTOSAR_CLASSIC}>:ENABLE_AUTOSAR_CLASSIC>
    $<$<BOOL:${ENABLE_AUTOSAR_ADAPTIVE}>:ENABLE_AUTOSAR_ADAPTIVE>
    $<$<BOOL:${ENABLE_DDS_SECURITY}>:ENABLE_DDS_SECURITY>
    $<$<BOOL:${ENABLE_DDS_ADVANCED}>:ENABLE_DDS_ADVANCED>
    $<$<BOOL:${ENABLE_DDS_RUNTIME}>:ENABLE_DDS_RUNTIME>
    $<$<BOOL:${ENABLE_SAFETY_RAM}>:ENABLE_SAFETY_RAM>
    $<$<BOOL:${ENABLE_SAFETY_NVM}>:ENABLE_SAFETY_NVM>
    $<$<BOOL:${ENABLE_SAFETY_SAFERAM}>:ENABLE_SAFETY_SAFERAM>
    $<$<STREQUAL:${PLATFORM_TARGET},"s32g3">:BOARD_S32G3>
    $<$<STREQUAL:${PLATFORM_TARGET},"aurix_tc3xx">:BOARD_AURIX_TC3XX>
    $<$<STREQUAL:${PLATFORM_TARGET},"posix">:BOARD_POSIX>
)

#==============================================================================
# Platform-Specific Configuration
#==============================================================================
if(ENABLE_PLATFORM_LINUX)
    message(STATUS "Configuring for Linux platform")
    target_compile_definitions(eth_dds PUBLIC PLATFORM_LINUX)
    target_link_libraries(eth_dds PUBLIC 
        Threads::Threads
        ${MATH_LIBRARY}
    )
endif()

if(ENABLE_PLATFORM_FREERTOS)
    message(STATUS "Configuring for FreeRTOS platform")
    target_compile_definitions(eth_dds PUBLIC 
        PLATFORM_FREERTOS 
        FREERTOS
        FREERTOS_USED
    )
    
    # Add FreeRTOS subdirectory
    add_subdirectory(${ETH_DDS_SRC_DIR}/platform/freertos)
    target_link_libraries(eth_dds PUBLIC freertos_kernel)
endif()

if(ENABLE_PLATFORM_BAREMETAL)
    message(STATUS "Configuring for bare-metal platform")
    target_compile_definitions(eth_dds PUBLIC 
        PLATFORM_BAREMETAL 
        BAREMETAL
    )
    set_target_properties(eth_dds PROPERTIES LINKER_LANGUAGE C)
endif()

#==============================================================================
# Testing
#==============================================================================
if(BUILD_TESTS)
    include(testing)
endif()

#==============================================================================
# Examples
#==============================================================================
if(BUILD_EXAMPLES)
    add_subdirectory(${ETH_DDS_EXAMPLES_DIR})
endif()

#==============================================================================
# Packaging
#==============================================================================
if(BUILD_PACKAGE)
    include(packaging)
endif()

#==============================================================================
# Summary
#==============================================================================
message(STATUS "")
message(STATUS "Configuration Complete")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build documentation: ${BUILD_DOCUMENTATION}")
message(STATUS "  Build package: ${BUILD_PACKAGE}")
message(STATUS "")
message(STATUS "To build: cmake --build ${CMAKE_BINARY_DIR}")
message(STATUS "To test: ctest --test-dir ${CMAKE_BINARY_DIR}")
message(STATUS "To install: cmake --install ${CMAKE_BINARY_DIR}")
message(STATUS "")
