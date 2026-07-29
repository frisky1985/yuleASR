# YuleTech AutoSAR BSW Platform
# Root CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# Project definition
project(YuleTech_AutoSAR
    VERSION 1.0.0
    DESCRIPTION "YuleTech AutoSAR BSW Platform - Open Source BSW Implementation"
    LANGUAGES C CXX ASM
)

# CMake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Include helper modules
include(ModuleHelpers)
include(PlatformConfig)

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTING "Build tests" OFF)
option(BUILD_DOCUMENTATION "Build documentation" OFF)
option(BUILD_EXAMPLES "Build example applications" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_LTO "Enable Link Time Optimization" OFF)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

# Position independent code (for static libraries that might be linked to shared)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Print configuration summary
message(STATUS "")
message(STATUS "========================================")
message(STATUS "YuleTech AutoSAR BSW Platform")
message(STATUS "========================================")
message(STATUS "Version:        ${PROJECT_VERSION}")
message(STATUS "Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "Toolchain:      ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "Target:         ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Cross-compile:  ${CMAKE_CROSSCOMPILING}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")
message(STATUS "Options:")
message(STATUS "  Build tests:       ${BUILD_TESTING}")
message(STATUS "  Build docs:        ${BUILD_DOCUMENTATION}")
message(STATUS "  Build examples:    ${BUILD_EXAMPLES}")
message(STATUS "  MCAL drivers:      ${YULE_ENABLE_MCAL}")
message(STATUS "  Safety features:   ${YULE_ENABLE_SAFETY}")
message(STATUS "  Code coverage:     ${ENABLE_COVERAGE}")
message(STATUS "========================================")
message(STATUS "")

# Global include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsw/classic/com
)

# Add subdirectories
add_subdirectory(src/bsw/services)
add_subdirectory(src/bsw/mcal)
add_subdirectory(src/bsw/ecual)
add_subdirectory(src/bsw/boot)
add_subdirectory(src/rte)
add_subdirectory(src/bsw/os)
add_subdirectory(src/platform)

# ASW components (optional for embedded target)
if(NOT CMAKE_CROSSCOMPILING OR BUILD_EXAMPLES)
    add_subdirectory(src/asw)
endif()

# Safety components
if(YULE_ENABLE_SAFETY AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/safety/CMakeLists.txt)
    add_subdirectory(src/safety)
endif()

# Testing
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
if(BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(doc ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    else()
        message(WARNING "Doxygen not found, documentation will not be built")
    endif()
endif()

# Installation
include(GNUInstallDirs)

# Install headers
install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/yuletech
    FILES_MATCHING PATTERN "*.h"
    PATTERN "*.c" EXCLUDE
    PATTERN "*.md" EXCLUDE
)

# Install cmake config
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/YuleTechAutoSARConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# CPack configuration
set(CPACK_PACKAGE_NAME "YuleTech-AutoSAR")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "YuleTech AutoSAR BSW Platform")
set(CPACK_PACKAGE_VENDOR "Shanghai Yule Electronics Technology Co., Ltd.")
include(CPack)

# Print summary of targets
message(STATUS "")
message(STATUS "Configured targets:")
get_property(targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
foreach(target ${targets})
    message(STATUS "  - ${target}")
endforeach()
message(STATUS "")
