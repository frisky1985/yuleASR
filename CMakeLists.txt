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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsw/os/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsw/general/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsw/services/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsw/services/pdur/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsw/services/nvm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsw/services/com/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsw/services/dcm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsw/services/dem/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsw/mcal/mcu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsw/mcal/port/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rte/include
)

# AUTOSAR stub headers for native builds
if(NOT CMAKE_CROSSCOMPILING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/autosar)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/autosar)
endif()

# Compiler/MemMap/Std_Types stubs are also required for cross builds
# (arm-none-eabi has no newlib, so the AUTOSAR stub headers fill the gap)
if(CMAKE_CROSSCOMPILING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/autosar)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/autosar)
endif()

# Freestanding libc headers for bare-metal cross builds without newlib
# (real C99 standard declarations; implementation is provided at link time
#  by newlib/picolibc in production builds)
if(CMAKE_CROSSCOMPILING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freestanding-include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/freestanding-include)
endif()

# mbedTLS: use the yuleASR configuration (platform entropy/timing policy)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mbedtls/configs/yuleasr_config.h)
    add_compile_definitions(MBEDTLS_CONFIG_FILE=<yuleasr_config.h>)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/mbedtls/configs)
endif()

# Global coverage compile/link flags (unconditional — all C files produce coverage data)
# Code coverage compile/link flags: --coverage is equivalent to -fprofile-arcs -ftest-coverage
# for GCC/Clang.  Only set when ENABLE_COVERAGE is ON to avoid unconditional instrumentation.
#
# NOTE (Apple Clang): Static library objects compiled with --coverage produce .gcno note
# files but do NOT generate .gcda at runtime when linked into an executable on Apple Clang.
# Use batch10_coverage.sh (or the coverage-batch CI target below) which compiles all
# sources directly into a single test executable for proper .gcda generation.
if(NOT CMAKE_CROSSCOMPILING AND ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -O0 -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -O0 -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage -g")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage -g")
endif()

# Add subdirectories
# ── Real mbedTLS (Mbed-TLS 3.6.2 LTS, vendored) ──────────────────────────
set(ENABLE_PROGRAMS OFF CACHE BOOL "Build Mbed TLS programs." FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "Build Mbed TLS tests." FORCE)
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "Compiler warnings treated as errors" FORCE)
set(DISABLE_PACKAGE_CONFIG_AND_INSTALL ON CACHE BOOL "Disable mbedTLS install" FORCE)
add_subdirectory(third_party/mbedtls)

add_subdirectory(src/bsw/services)
add_subdirectory(src/bsw/mcal)
add_subdirectory(src/bsw/ecual)
add_subdirectory(src/bsw/boot)
add_subdirectory(src/rte)
add_subdirectory(src/bsw/os)
add_subdirectory(src/platform)

# DDS middleware (ETH-DDS integration)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/dds/CMakeLists.txt)
    add_subdirectory(src/dds)
endif()

# ASW components (optional for embedded target)
if(NOT CMAKE_CROSSCOMPILING OR BUILD_EXAMPLES)
    add_subdirectory(src/asw)
endif()

# Safety components
if(YULE_ENABLE_SAFETY AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/safety/CMakeLists.txt)
    add_subdirectory(src/safety)
endif()

# Code coverage configuration
if(ENABLE_COVERAGE)
    message(STATUS "Code coverage: ENABLED")
    
    # Find gcovr for report generation
    find_program(GCOVR_PATH gcovr)
    if(GCOVR_PATH)
        message(STATUS "gcovr found: ${GCOVR_PATH}")
        
        # Coverage report targets (only if BUILD_TESTING is also ON)
        if(BUILD_TESTING)
            add_custom_target(coverage-run
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running tests for coverage data collection..."
            )
            
            add_custom_target(coverage-html
                COMMAND ${CMAKE_COMMAND} -E echo "Generating HTML coverage report..."
                COMMAND ${GCOVR_PATH} --root ${CMAKE_SOURCE_DIR}
                        --filter "src/.*"
                        --exclude "tests/.*"
                        --exclude "third_party/.*"
                        --exclude-unreachable-branches
                        --html --html-details
                        --output ${CMAKE_BINARY_DIR}/coverage/index.html
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating HTML coverage report (gcovr)"
            )
            
            add_custom_target(coverage-json
                COMMAND ${CMAKE_COMMAND} -E echo "Generating JSON coverage report..."
                COMMAND ${GCOVR_PATH} --root ${CMAKE_SOURCE_DIR}
                        --filter "src/.*"
                        --exclude "tests/.*"
                        --exclude "third_party/.*"
                        --json
                        --output ${CMAKE_BINARY_DIR}/coverage/coverage.json
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating JSON coverage report (gcovr)"
            )
            
            add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E echo "=== Generating all coverage reports ==="
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                COMMAND ${GCOVR_PATH} --root ${CMAKE_SOURCE_DIR}
                        --filter "src/.*"
                        --exclude "tests/.*"
                        --exclude "third_party/.*"
                        --exclude-unreachable-branches
                        --html --html-details
                        --output ${CMAKE_BINARY_DIR}/coverage/index.html
                COMMAND ${GCOVR_PATH} --root ${CMAKE_SOURCE_DIR}
                        --filter "src/.*"
                        --exclude "tests/.*"
                        --exclude "third_party/.*"
                        --json
                        --output ${CMAKE_BINARY_DIR}/coverage/coverage.json
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running tests and generating coverage reports (gcovr)"
            )
            
            # Note: coverage-run is run as part of the 'coverage' target chain
        endif()
    else()
        message(WARNING "gcovr not found; coverage reports will not be generated. Install with: pip install gcovr")
        if(BUILD_TESTING)
            # Fallback: just ensure .gcda files are produced, manual gcov can be used
            add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E echo "gcovr not available. Run 'gcov' manually on .gcda files in ${CMAKE_BINARY_DIR}"
                COMMENT "gcovr not found — install with 'pip install gcovr' for automated reports"
            )
        endif()
    endif()

    # Coverage target using batch10_coverage.sh (bypasses Apple Clang static-lib .gcda issue)
    # This compiles test and production sources directly into a single executable,
    # ensuring all .gcda coverage data is generated.  Run from the project root:
    #   cmake --build . --target coverage-batch
    add_custom_target(coverage-batch
        COMMAND ${CMAKE_COMMAND} -E echo "=== Batch coverage build (batch10_coverage.sh) ==="
        COMMAND ${CMAKE_COMMAND} -E echo "This compiles all sources directly (bypasses static lib issue on Apple Clang)"
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests and generating reports..."
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_SOURCE_DIR}/batch10_coverage.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running batch10_coverage.sh for full coverage data (Apple Clang workaround)"
    )

    # Meta target: runs batch coverage, then gates on threshold
    add_custom_target(coverage-ci
        COMMAND ${CMAKE_COMMAND} -E echo "=== Coverage CI Pipeline ==="
        COMMAND ${CMAKE_COMMAND} -E echo "Step 1: Run batch coverage (produces .gcda for all production code)"
        COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_SOURCE_DIR}/batch10_coverage.sh
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "Step 2: Check coverage gate threshold"
        COMMAND ${CMAKE_COMMAND} -E env python3 ${CMAKE_SOURCE_DIR}/tools/check_coverage_gate.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Full CI coverage pipeline: batch build + coverage gate"
    )
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
