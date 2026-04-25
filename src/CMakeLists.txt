# src/CMakeLists.txt
# Source directory build configuration

# Common utilities library
add_subdirectory(common)

# Ethernet driver
if(ENABLE_ETHERNET)
    add_subdirectory(ethernet)
endif()

# DDS middleware
if(ENABLE_DDS)
    add_subdirectory(dds)
endif()

# Transport layer
add_subdirectory(transport)

# TSN stack
if(ENABLE_TSN)
    add_subdirectory(tsn)
endif()

# AUTOSAR integration
if(ENABLE_AUTOSAR_CLASSIC OR ENABLE_AUTOSAR_ADAPTIVE)
    add_subdirectory(autosar)
endif()

# Safety modules
if(ENABLE_SAFETY_RAM OR ENABLE_SAFETY_NVM OR ENABLE_SAFETY_SAFERAM)
    add_subdirectory(safety)
endif()

# Platform abstraction
if(ENABLE_PLATFORM_FREERTOS OR ENABLE_PLATFORM_BAREMETAL)
    add_subdirectory(platform)
endif()

# BSW (Basic Software)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw)
    add_subdirectory(bsw)
endif()
