# ASW (Application Software) Components CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# ASW Components
set(ASW_COMPONENTS
    communication_manager
    diagnostic_manager
    engine_control
    io_control
    mode_manager
    storage_manager
    vehicle_dynamics
    watchdog_manager
)

foreach(component ${ASW_COMPONENTS})
    file(GLOB ${component}_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/${component}/src/*.c
    )
    
    if(${component}_SOURCES)
        add_library(${component} STATIC ${${component}_SOURCES})
        
        target_include_directories(${component}
            PUBLIC 
                ${CMAKE_CURRENT_SOURCE_DIR}/${component}/include
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/rte/include
                ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/services/com/include
                ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/services/nvm/include
                ${CMAKE_CURRENT_SOURCE_DIR}/../bsw/services/dcm/include
        )
        
        set_target_properties(${component} PROPERTIES
            C_STANDARD 99
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/asw
        )
        
        message(STATUS "  ASW component: ${component}")
    endif()
endforeach()

# ASW Meta-Target
add_library(AswComponents INTERFACE)
target_link_libraries(AswComponents INTERFACE
    communication_manager
    diagnostic_manager
    engine_control
    io_control
    mode_manager
    storage_manager
    vehicle_dynamics
    watchdog_manager
)

message(STATUS "ASW configured (8 components)")
