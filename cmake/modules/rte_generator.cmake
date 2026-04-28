#==============================================================================
# RTE Generator Module
#==============================================================================
# This module provides support for the RTE Code Generator tool
#
# Usage:
#   include(rte_generator)
#   rte_generate_code(CONFIG <config_file> OUTPUT <output_dir>)
#
#==============================================================================

#==============================================================================
# Find RTE Generator
#==============================================================================
find_program(RTE_GENERATOR_SCRIPT
    NAMES rte_generator.py
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/rte_generator
        ${CMAKE_CURRENT_SOURCE_DIR}/../tools/rte_generator
    DOC "RTE Generator Python script"
)

if(RTE_GENERATOR_SCRIPT)
    message(STATUS "Found RTE Generator: ${RTE_GENERATOR_SCRIPT}")
else()
    message(WARNING "RTE Generator not found. RTE code generation will be disabled.")
endif()

#==============================================================================
# Find Python
#==============================================================================
find_package(Python3 COMPONENTS Interpreter QUIET)

if(NOT Python3_FOUND)
    message(WARNING "Python3 not found. RTE code generation will be disabled.")
endif()

#==============================================================================
# RTE Generate Code Function
#==============================================================================
function(rte_generate_code)
    if(NOT RTE_GENERATOR_SCRIPT OR NOT Python3_FOUND)
        message(WARNING "RTE Generator dependencies not met. Skipping code generation.")
        return()
    endif()

    # Parse arguments
    set(options "")
    set(oneValueArgs CONFIG OUTPUT)
    set(multiValueArgs "")
    cmake_parse_arguments(RTE_GEN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate arguments
    if(NOT RTE_GEN_CONFIG)
        message(FATAL_ERROR "rte_generate_code: CONFIG argument required")
    endif()

    if(NOT RTE_GEN_OUTPUT)
        set(RTE_GEN_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/rte)
    endif()

    # Resolve paths
    get_filename_component(CONFIG_ABS ${RTE_GEN_CONFIG} ABSOLUTE)
    get_filename_component(OUTPUT_ABS ${RTE_GEN_OUTPUT} ABSOLUTE)

    # Check config file exists
    if(NOT EXISTS ${CONFIG_ABS})
        message(FATAL_ERROR "RTE config file not found: ${CONFIG_ABS}")
    endif()

    # Create output directory
    file(MAKE_DIRECTORY ${OUTPUT_ABS})

    # Generate command comment
    message(STATUS "RTE Generator:")
    message(STATUS "  Config: ${CONFIG_ABS}")
    message(STATUS "  Output: ${OUTPUT_ABS}")

    # Add custom command for code generation
    add_custom_command(
        OUTPUT ${OUTPUT_ABS}/Rte_Generated.stamp
        COMMAND ${Python3_EXECUTABLE} ${RTE_GENERATOR_SCRIPT}
            --config ${CONFIG_ABS}
            --output ${OUTPUT_ABS}
        COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT_ABS}/Rte_Generated.stamp
        DEPENDS ${CONFIG_ABS} ${RTE_GENERATOR_SCRIPT}
        COMMENT "Generating RTE code from ${RTE_GEN_CONFIG}"
        VERBATIM
    )

    # Add custom target
    add_custom_target(rte_generate
        DEPENDS ${OUTPUT_ABS}/Rte_Generated.stamp
        COMMENT "RTE Code Generation Target"
    )

    # Include generated files in build
    file(GLOB_RECURSE RTE_GENERATED_SOURCES ${OUTPUT_ABS}/*.c)
    file(GLOB_RECURSE RTE_GENERATED_HEADERS ${OUTPUT_ABS}/*.h)

    # Set variables in parent scope
    set(RTE_GENERATED_DIR ${OUTPUT_ABS} PARENT_SCOPE)
    set(RTE_GENERATED_SOURCES ${RTE_GENERATED_SOURCES} PARENT_SCOPE)
    set(RTE_GENERATED_HEADERS ${RTE_GENERATED_HEADERS} PARENT_SCOPE)

    message(STATUS "  Sources: ${RTE_GENERATED_SOURCES}")
    message(STATUS "  Headers: ${RTE_GENERATED_HEADERS}")
endfunction()

#==============================================================================
# RTE Add Generated Code to Target
#==============================================================================
function(rte_target_add_generated target)
    if(NOT TARGET rte_generate)
        message(WARNING "rte_generate target not found. Run rte_generate_code first.")
        return()
    endif()

    # Make target depend on RTE generation
    add_dependencies(${target} rte_generate)

    # Add include directory if RTE_GENERATED_DIR is set
    if(RTE_GENERATED_DIR)
        target_include_directories(${target} PRIVATE ${RTE_GENERATED_DIR})
    endif()

    # Add generated sources to target
    if(RTE_GENERATED_SOURCES)
        target_sources(${target} PRIVATE ${RTE_GENERATED_SOURCES})
    endif()

    message(STATUS "RTE generated code added to target: ${target}")
endfunction()

#==============================================================================
# RTE Generation Option
#==============================================================================
option(ENABLE_RTE_GENERATION "Enable RTE code generation during build" OFF)

if(ENABLE_RTE_GENERATION)
    message(STATUS "RTE Generation: Enabled")
else()
    message(STATUS "RTE Generation: Disabled (use -DENABLE_RTE_GENERATION=ON to enable)")
endif()
