# examples/CMakeLists.txt
# Example applications

# Only build examples if explicitly enabled
if(NOT BUILD_EXAMPLES)
    return()
endif()

message(STATUS "Building example applications")

# Temperature sensor example (basic)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/temperature_sensor)
    add_subdirectory(temperature_sensor)
endif()

# ADAS perception example (ASIL-D)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/adas_perception AND ENABLE_SAFETY_RAM)
    add_subdirectory(adas_perception)
endif()

# Powertrain control example (ASIL-D)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/powertrain_control AND ENABLE_SAFETY_RAM)
    add_subdirectory(powertrain_control)
endif()

# Body electronics example (ASIL-B)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/body_electronics)
    add_subdirectory(body_electronics)
endif()

# Infotainment example (QM)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/infotainment)
    add_subdirectory(infotainment)
endif()

# Diagnostics example
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics)
    add_subdirectory(diagnostics)
endif()
