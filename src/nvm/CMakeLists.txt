# NVM (Non-Volatile Memory) Module CMakeLists.txt
# ASIL-D Compliant AUTOSAR NvM Implementation

cmake_minimum_required(VERSION 3.10)

# NVM Module Library
set(NVM_SOURCES
    nvm.c
    nvm_core.c
    nvm_block_manager.c
    nvm_redundancy.c
    nvm_job_queue.c
    nvm_storage_if.c
)

set(NVM_HEADERS
    nvm.h
    nvm_types.h
    nvm_core.h
    nvm_block_manager.h
    nvm_redundancy.h
    nvm_job_queue.h
    nvm_storage_if.h
)

# Create static library
add_library(nvm STATIC ${NVM_SOURCES})

# Include directories
target_include_directories(nvm
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler flags for safety-critical code
target_compile_options(nvm
    PRIVATE
        -Wall
        -Wextra
        -Werror
        -O2
        -fstrict-aliasing
        -fdata-sections
        -ffunction-sections
)

# Link options
target_link_options(nvm
    PRIVATE
        -Wl,--gc-sections
)

# Example executable
add_executable(nvm_example
    example_nvm_usage.c
)

target_link_libraries(nvm_example
    PRIVATE
        nvm
)

# Tests
if(BUILD_TESTING)
    enable_testing()
    
    # Add test executables here
    add_executable(nvm_unit_tests
        tests/test_nvm_core.c
        tests/test_nvm_block_manager.c
        tests/test_nvm_redundancy.c
        tests/test_nvm_job_queue.c
    )
    
    target_link_libraries(nvm_unit_tests
        PRIVATE
            nvm
            unity
    )
    
    add_test(NAME nvm_core_test COMMAND nvm_unit_tests)
endif()

# Install
install(TARGETS nvm
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${NVM_HEADERS}
    DESTINATION include/nvm
)

# Documentation
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(nvm_docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating NVM documentation"
            VERBATIM
        )
    endif()
endif()
