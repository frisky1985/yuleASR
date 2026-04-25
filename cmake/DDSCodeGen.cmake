# DDS Code Generation CMake Module
# Provides functions for generating code from DDS configurations
#
# Usage:
#   include(DDSCodeGen)
#   dds_generate_code(CONFIG_FILE config.yaml OUTPUT_DIR generated/)
#

# Find Python for code generation
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Path to code generation scripts
set(DDS_CODEGEN_DIR "${CMAKE_CURRENT_LIST_DIR}/../tools/codegen")
set(DDS_CONFIG_SCHEMA "${CMAKE_CURRENT_LIST_DIR}/../dds-config-tool/config_schema.yaml")
set(DDS_TEMPLATES_DIR "${CMAKE_CURRENT_LIST_DIR}/../tools/templates")

#==============================================================================
# Function: dds_generate_code
# Generates DDS code from configuration file
#==============================================================================
function(dds_generate_code)
    set(options FORCE VERBOSE)
    set(oneValueArgs CONFIG_FILE OUTPUT_DIR DDS_IMPL TARGET_OS SAFETY_LEVEL)
    set(multiValueArgs GENERATE_TYPES)
    cmake_parse_arguments(DDS_GEN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT DDS_GEN_CONFIG_FILE)
        message(FATAL_ERROR "dds_generate_code: CONFIG_FILE is required")
    endif()

    if(NOT EXISTS ${DDS_GEN_CONFIG_FILE})
        message(FATAL_ERROR "Config file not found: ${DDS_GEN_CONFIG_FILE}")
    endif()

    # Set defaults
    if(NOT DDS_GEN_OUTPUT_DIR)
        set(DDS_GEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    endif()

    if(NOT DDS_GEN_DDS_IMPL)
        set(DDS_GEN_DDS_IMPL "fastdds")
    endif()

    if(NOT DDS_GEN_TARGET_OS)
        set(DDS_GEN_TARGET_OS "freertos")
    endif()

    if(NOT DDS_GEN_SAFETY_LEVEL)
        set(DDS_GEN_SAFETY_LEVEL "QM")
    endif()

    # Create output directory
    file(MAKE_DIRECTORY ${DDS_GEN_OUTPUT_DIR})

    # Get absolute paths
    get_filename_component(CONFIG_ABS ${DDS_GEN_CONFIG_FILE} ABSOLUTE)
    get_filename_component(OUTPUT_ABS ${DDS_GEN_OUTPUT_DIR} ABSOLUTE)

    # Generate timestamp file for dependency checking
    set(TIMESTAMP_FILE "${OUTPUT_ABS}/.dds_codegen_timestamp")

    # Command to generate code
    add_custom_command(
        OUTPUT 
            "${OUTPUT_ABS}/dds_types.hpp"
            "${OUTPUT_ABS}/dds_types.cpp"
            "${OUTPUT_ABS}/dds_config.h"
            "${TIMESTAMP_FILE}"
        COMMAND ${Python3_EXECUTABLE} ${DDS_CODEGEN_DIR}/dds_config_cli.py
            generate
            --input ${CONFIG_ABS}
            --output ${OUTPUT_ABS}
            --dds-impl ${DDS_GEN_DDS_IMPL}
            --os ${DDS_GEN_TARGET_OS}
            --safety ${DDS_GEN_SAFETY_LEVEL}
            --type all
        COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE}
        DEPENDS 
            ${CONFIG_ABS}
            ${DDS_CODEGEN_DIR}/dds_config_cli.py
            ${DDS_CODEGEN_DIR}/dds_type_generator.py
            ${DDS_CODEGEN_DIR}/dds_idl_parser.py
            ${DDS_CODEGEN_DIR}/config_validator.py
        COMMENT "Generating DDS code from ${DDS_GEN_CONFIG_FILE}"
        VERBATIM
    )

    # Create custom target
    get_filename_component(CONFIG_NAME ${DDS_GEN_CONFIG_FILE} NAME_WE)
    set(TARGET_NAME "dds_codegen_${CONFIG_NAME}")

    add_custom_target(${TARGET_NAME} ALL
        DEPENDS 
            "${OUTPUT_ABS}/dds_types.hpp"
            "${OUTPUT_ABS}/dds_types.cpp"
            "${OUTPUT_ABS}/dds_config.h"
    )

    # Set output variables in parent scope
    set(DDS_GENERATED_INCLUDE_DIR ${OUTPUT_ABS} PARENT_SCOPE)
    set(DDS_GENERATED_SOURCES 
        "${OUTPUT_ABS}/dds_types.cpp"
        PARENT_SCOPE
    )

    if(DDS_GEN_VERBOSE)
        message(STATUS "DDS Code Generation:")
        message(STATUS "  Config: ${DDS_GEN_CONFIG_FILE}")
        message(STATUS "  Output: ${DDS_GEN_OUTPUT_DIR}")
        message(STATUS "  DDS Impl: ${DDS_GEN_DDS_IMPL}")
        message(STATUS "  Target OS: ${DDS_GEN_TARGET_OS}")
        message(STATUS "  Safety Level: ${DDS_GEN_SAFETY_LEVEL}")
    endif()
endfunction()

#==============================================================================
# Function: dds_generate_rte
# Generates RTE code from ARXML file
#==============================================================================
function(dds_generate_rte)
    set(options FORCE)
    set(oneValueArgs ARXML_FILE OUTPUT_DIR TARGET_OS SAFETY_LEVEL)
    cmake_parse_arguments(RTE_GEN "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT RTE_GEN_ARXML_FILE)
        message(FATAL_ERROR "dds_generate_rte: ARXML_FILE is required")
    endif()

    if(NOT EXISTS ${RTE_GEN_ARXML_FILE})
        message(FATAL_ERROR "ARXML file not found: ${RTE_GEN_ARXML_FILE}")
    endif()

    if(NOT RTE_GEN_OUTPUT_DIR)
        set(RTE_GEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_rte")
    endif()

    if(NOT RTE_GEN_TARGET_OS)
        set(RTE_GEN_TARGET_OS "freertos")
    endif()

    if(NOT RTE_GEN_SAFETY_LEVEL)
        set(RTE_GEN_SAFETY_LEVEL "QM")
    endif()

    file(MAKE_DIRECTORY ${RTE_GEN_OUTPUT_DIR})

    get_filename_component(ARXML_ABS ${RTE_GEN_ARXML_FILE} ABSOLUTE)
    get_filename_component(OUTPUT_ABS ${RTE_GEN_OUTPUT_DIR} ABSOLUTE)

    add_custom_command(
        OUTPUT 
            "${OUTPUT_ABS}/Rte_Type.h"
            "${OUTPUT_ABS}/Rte_Main.c"
        COMMAND ${Python3_EXECUTABLE} ${DDS_CODEGEN_DIR}/dds_config_cli.py
            parse-arxml
            ${ARXML_ABS}
            --generate-rte
            --output ${OUTPUT_ABS}/arxml_parsed.json
        COMMAND ${Python3_EXECUTABLE} ${DDS_CODEGEN_DIR}/dds_config_cli.py
            generate
            --input ${ARXML_ABS}
            --output ${OUTPUT_ABS}
            --type rte
            --os ${RTE_GEN_TARGET_OS}
            --safety ${RTE_GEN_SAFETY_LEVEL}
        DEPENDS 
            ${ARXML_ABS}
            ${DDS_CODEGEN_DIR}/dds_config_cli.py
            ${DDS_CODEGEN_DIR}/rte_generator.py
            ${DDS_CODEGEN_DIR}/autosar_arxml_parser.py
        COMMENT "Generating RTE code from ${RTE_GEN_ARXML_FILE}"
        VERBATIM
    )

    get_filename_component(ARXML_NAME ${RTE_GEN_ARXML_FILE} NAME_WE)
    set(TARGET_NAME "rte_codegen_${ARXML_NAME}")

    add_custom_target(${TARGET_NAME} ALL
        DEPENDS 
            "${OUTPUT_ABS}/Rte_Type.h"
            "${OUTPUT_ABS}/Rte_Main.c"
    )

    set(RTE_GENERATED_INCLUDE_DIR ${OUTPUT_ABS} PARENT_SCOPE)
    file(GLOB RTE_SOURCES "${OUTPUT_ABS}/*.c")
    set(RTE_GENERATED_SOURCES ${RTE_SOURCES} PARENT_SCOPE)
endfunction()

#==============================================================================
# Function: dds_validate_config
# Validates DDS configuration file
#==============================================================================
function(dds_validate_config)
    set(options STRICT)
    set(oneValueArgs CONFIG_FILE REPORT_FILE)
    cmake_parse_arguments(VAL "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT VAL_CONFIG_FILE)
        message(FATAL_ERROR "dds_validate_config: CONFIG_FILE is required")
    endif()

    if(NOT EXISTS ${VAL_CONFIG_FILE})
        message(FATAL_ERROR "Config file not found: ${VAL_CONFIG_FILE}")
    endif()

    set(VALIDATE_ARGS validate ${VAL_CONFIG_FILE})

    if(VAL_STRICT)
        list(APPEND VALIDATE_ARGS --strict)
    endif()

    if(VAL_REPORT_FILE)
        list(APPEND VALIDATE_ARGS --output ${VAL_REPORT_FILE})
    endif()

    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${DDS_CODEGEN_DIR}/dds_config_cli.py ${VALIDATE_ARGS}
        RESULT_VARIABLE VALIDATE_RESULT
        OUTPUT_VARIABLE VALIDATE_OUTPUT
        ERROR_VARIABLE VALIDATE_ERROR
    )

    if(VALIDATE_RESULT EQUAL 0)
        message(STATUS "Configuration validation PASSED: ${VAL_CONFIG_FILE}")
    else()
        message(WARNING "Configuration validation FAILED: ${VAL_CONFIG_FILE}")
        message(WARNING "Output: ${VALIDATE_OUTPUT}")
    endif()

    # Return result to parent
    set(DDS_CONFIG_VALID ${VALIDATE_RESULT} PARENT_SCOPE)
endfunction()

#==============================================================================
# Function: dds_add_generated_library
# Creates a library from generated DDS code
#==============================================================================
function(dds_add_generated_library TARGET_NAME)
    set(options)
    set(oneValueArgs CONFIG_FILE OUTPUT_DIR)
    cmake_parse_arguments(LIB "" "${oneValueArgs}" "" ${ARGN})

    if(NOT LIB_CONFIG_FILE)
        message(FATAL_ERROR "dds_add_generated_library: CONFIG_FILE is required")
    endif()

    # Generate code first
    dds_generate_code(
        CONFIG_FILE ${LIB_CONFIG_FILE}
        OUTPUT_DIR ${LIB_OUTPUT_DIR}
    )

    # Get config name for dependency
    get_filename_component(CONFIG_NAME ${LIB_CONFIG_FILE} NAME_WE)
    set(CODEGEN_TARGET "dds_codegen_${CONFIG_NAME}")

    # Create library
    add_library(${TARGET_NAME} STATIC
        ${DDS_GENERATED_SOURCES}
    )

    add_dependencies(${TARGET_NAME} ${CODEGEN_TARGET})

    target_include_directories(${TARGET_NAME} PUBLIC
        ${DDS_GENERATED_INCLUDE_DIR}
    )

    # Link with DDS implementation
    if(TARGET fastrtps)
        target_link_libraries(${TARGET_NAME} PUBLIC fastrtps)
    endif()
endfunction()

#==============================================================================
# Function: dds_configure_file_generation
# Configures template file generation
#==============================================================================
function(dds_configure_file_generation)
    set(options)
    set(oneValueArgs INPUT OUTPUT CONFIG_FILE)
    cmake_parse_arguments(CFG "" "${oneValueArgs}" "" ${ARGN})

    if(NOT CFG_INPUT OR NOT CFG_OUTPUT)
        message(FATAL_ERROR "dds_configure_file_generation: INPUT and OUTPUT required")
    endif()

    # Parse config to get values
    if(CFG_CONFIG_FILE AND EXISTS ${CFG_CONFIG_FILE})
        file(READ ${CFG_CONFIG_FILE} CONFIG_CONTENT)
        
        # Extract version info (simplified)
        string(REGEX MATCH "version: \"([^\"]+)\"" VERSION_MATCH "${CONFIG_CONTENT}")
        if(VERSION_MATCH)
            set(CONFIG_VERSION ${CMAKE_MATCH_1})
        else()
            set(CONFIG_VERSION "1.0.0")
        endif()
    endif()

    # Configure file with CMake
    set(GENERATED_TIMESTAMP "${CMAKE_CURRENT_DATE}")
    
    configure_file(
        ${CFG_INPUT}
        ${CFG_OUTPUT}
        @ONLY
    )
endfunction()

#==============================================================================
# Function: dds_add_config_watcher
# Adds file watcher to regenerate code when config changes
#==============================================================================
function(dds_add_config_watcher CONFIG_FILE)
    if(NOT EXISTS ${CONFIG_FILE})
        return()
    endif()

    get_filename_component(CONFIG_ABS ${CONFIG_FILE} ABSOLUTE)
    get_filename_component(CONFIG_NAME ${CONFIG_FILE} NAME)

    # Create a custom command that will be triggered when config changes
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CONFIG_ABS})

    message(STATUS "Added config watcher for: ${CONFIG_NAME}")
endfunction()

#==============================================================================
# Convenience macro for full DDS setup
#==============================================================================
macro(dds_setup_from_config CONFIG_FILE)
    # Validate first
    dds_validate_config(CONFIG_FILE ${CONFIG_FILE})

    # Generate code
    dds_generate_code(
        CONFIG_FILE ${CONFIG_FILE}
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dds_generated
    )

    # Add watcher for automatic regeneration
    dds_add_config_watcher(${CONFIG_FILE})

    # Include generated headers
    include_directories(${DDS_GENERATED_INCLUDE_DIR})
endmacro()

#==============================================================================
# Print module info
#==============================================================================
message(STATUS "DDS Code Generation module loaded")
message(STATUS "  Codegen Dir: ${DDS_CODEGEN_DIR}")
message(STATUS "  Templates Dir: ${DDS_TEMPLATES_DIR}")