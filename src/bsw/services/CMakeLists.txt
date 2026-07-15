# Services Layer CMakeLists.txt
# Collects all service-layer submodules using file(GLOB)
# Libraries are prefixed with "service_" to avoid name conflicts
# across BSW layers (services, mcal, ecual may share module names)

cmake_minimum_required(VERSION 3.16)

# Global includes for Std_Types.h, etc.
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../rte/include
)

# Get all submodule directories
file(GLOB SERVICES_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)

foreach(subdir ${SERVICES_SUBDIRS})
    get_filename_component(module_name ${subdir} NAME)
    
    # Collect source files
    file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    
    if(module_sources)
        # Create library with service_ prefix to avoid name conflicts
        set(target_name "service_${module_name}")
        add_library(${target_name} STATIC ${module_sources})
        
        # Add include directory
        if(IS_DIRECTORY ${subdir}/include)
            target_include_directories(${target_name}
                PUBLIC ${subdir}/include
            )
        endif()
        
        set_target_properties(${target_name} PROPERTIES
            C_STANDARD 99
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
        )
        
        message(STATUS "  Service module: ${module_name} (target: ${target_name})")
    endif()
endforeach()

message(STATUS "Services layer configured")
