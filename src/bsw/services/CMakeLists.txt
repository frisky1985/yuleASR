# Services Layer CMakeLists.txt
# Collects all service-layer submodules using file(GLOB)
# Libraries are prefixed with "service_" to avoid name conflicts
# across BSW layers (services, mcal, ecual may share module names)

cmake_minimum_required(VERSION 3.16)

# Get all submodule directories
file(GLOB SERVICES_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)

# Pre-collect all service module include directories so every module
# can include headers from any other service module (PduR.h, Det.h, etc.)
set(SERVICES_INCLUDE_DIRS)
foreach(subdir ${SERVICES_SUBDIRS})
    if(IS_DIRECTORY ${subdir}/include)
        list(APPEND SERVICES_INCLUDE_DIRS ${subdir}/include)
    endif()
endforeach()

foreach(subdir ${SERVICES_SUBDIRS})
    get_filename_component(module_name ${subdir} NAME)
    
    # Collect source files
    file(GLOB_RECURSE module_sources ${subdir}/src/*.c)
    
    if(module_sources)
        # Create library with service_ prefix to avoid name conflicts
        set(target_name "service_${module_name}")
        add_library(${target_name} STATIC ${module_sources})
        
        # Add include directories:
        # - PUBLIC: Own module headers (for targets that link to this library)
        # - PRIVATE: ALL service include dirs (so any module can include any service header)
        target_include_directories(${target_name}
            PUBLIC ${subdir}/include
            PRIVATE ${SERVICES_INCLUDE_DIRS}
        )
        
        set_target_properties(${target_name} PROPERTIES
            C_STANDARD 99
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
        )
        
        message(STATUS "  Service module: ${module_name} (target: ${target_name})")
    endif()
endforeach()

message(STATUS "Services layer configured")
