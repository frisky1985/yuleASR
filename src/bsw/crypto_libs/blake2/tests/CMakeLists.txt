#**********************************************************************************************************************
# @file       CMakeLists.txt
# @brief      BLAKE2 Unit Tests Build Configuration
#**********************************************************************************************************************

cmake_minimum_required(VERSION 3.16)

# Test executable
add_executable(test_blake2
    test_blake2.c
)

# Include directories
target_include_directories(test_blake2
    PRIVATE
        ../include
)

# Link with BLAKE2 library
target_link_libraries(test_blake2
    PRIVATE
        blake2
)

# Compile options
target_compile_options(test_blake2
    PRIVATE
        -Wall
        -Wextra
        $<$<CONFIG:Debug>:-g -O0>
)

# Add test
add_test(NAME blake2_unit_tests COMMAND test_blake2)

# Set test properties
set_tests_properties(blake2_unit_tests
    PROPERTIES
        TIMEOUT 60
        LABELS "crypto;hash;blake2"
)
