# cmake/modules/compiler.cmake
# Compiler configuration and flags

#==============================================================================
# C/C++ Standard Settings
#==============================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)  # Allow GNU extensions for embedded

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#==============================================================================
# Build Type Configuration
#==============================================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS 
    "Debug" 
    "Release" 
    "RelWithDebInfo" 
    "MinSizeRel"
)

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

#==============================================================================
# Position Independent Code (for shared libraries)
#==============================================================================
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#==============================================================================
# Compiler Warnings (Common)
#==============================================================================
set(COMMON_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wcast-align
    -Wunused
    -Wconversion
    -Wsign-conversion
    -Wdouble-promotion
    -Wformat=2
    -Wmissing-declarations
    -Wstrict-prototypes
    -Wold-style-definition
)

#==============================================================================
# GNU/Clang Compiler Configuration
#==============================================================================
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    
    # Add common warnings
    add_compile_options(${COMMON_WARNINGS})
    
    # Build type specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Using Debug configuration")
        add_compile_options(
            -g3                  # Maximum debug info
            -O0                  # No optimization
            -DDEBUG
            -fno-omit-frame-pointer
        )
        
        # Sanitizers (optional)
        if(ENABLE_SANITIZERS)
            message(STATUS "Enabling AddressSanitizer and UBSan")
            add_compile_options(
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize-address-use-after-scope
            )
            add_link_options(
                -fsanitize=address
                -fsanitize=undefined
            )
        endif()
        
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Using Release configuration")
        add_compile_options(
            -O3                  # Maximum optimization
            -DNDEBUG
            -fomit-frame-pointer
            -ffunction-sections
            -fdata-sections
        )
        add_link_options(
            -Wl,--gc-sections    # Remove unused sections
        )
        
        # Link Time Optimization
        if(ENABLE_LTO)
            message(STATUS "Enabling Link Time Optimization")
            add_compile_options(-flto)
            add_link_options(-flto)
        endif()
        
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        message(STATUS "Using RelWithDebInfo configuration")
        add_compile_options(
            -O2
            -g
            -DNDEBUG
        )
        
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        message(STATUS "Using MinSizeRel configuration")
        add_compile_options(
            -Os                  # Optimize for size
            -DNDEBUG
            -ffunction-sections
            -fdata-sections
        )
        add_link_options(
            -Wl,--gc-sections
        )
    endif()
    
    # Coverage (Debug only)
    if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Enabling code coverage")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()
    
    # Profiling (optional)
    if(ENABLE_PROFILING)
        message(STATUS "Enabling profiling")
        add_compile_options(-pg)
        add_link_options(-pg)
    endif()
    
    # Embedded-specific flags for bare-metal
    if(ENABLE_PLATFORM_BAREMETAL)
        message(STATUS "Adding bare-metal embedded flags")
        add_compile_options(
            -ffreestanding       # Freestanding environment
            -nostdlib            # No standard library
        )
        add_link_options(
            -nostdlib
            -nodefaultlibs
        )
    endif()

#==============================================================================
# MSVC Compiler Configuration
#==============================================================================
elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    message(STATUS "Using MSVC compiler")
    
    add_compile_options(
        /W4                  # High warning level
        /wd4996              # Disable deprecation warnings
        /permissive-         # Strict conformance
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(
            /Od              # No optimization
            /Zi              # Debug info
            /DEBUG
            /RTC1            # Run-time checks
        )
    else()
        add_compile_options(
            /O2              # Optimize for speed
            /DNDEBUG
        )
    endif()
    
    # Enable parallel compilation
    add_compile_options(/MP)

#==============================================================================
# ARM Compiler Configuration
#==============================================================================
elseif(CMAKE_C_COMPILER_ID MATCHES "ARM")
    message(STATUS "Using ARM Compiler")
    
    add_compile_options(
        --c99
        --gnu
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3 -Otime)
    endif()

#==============================================================================
# Unknown Compiler
#==============================================================================
else()
    message(WARNING "Unknown compiler: ${CMAKE_C_COMPILER_ID}")
endif()

#==============================================================================
# Linker Configuration
#==============================================================================
# Common linker options
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined")

#==============================================================================
# Static Analysis (optional)
#==============================================================================
if(ENABLE_STATIC_ANALYSIS)
    message(STATUS "Enabling static analysis")
    
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        add_compile_options(
            --analyze
            -Xanalyzer -analyzer-output=text
        )
    endif()
    
    # cppcheck target will be created in testing.cmake
endif()

#==============================================================================
# Compiler Feature Detection
#==============================================================================
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Check for specific flags
check_c_compiler_flag(-Werror=incompatible-pointer-types HAS_WERROR_INCOMPATIBLE_POINTER)
if(HAS_WERROR_INCOMPATIBLE_POINTER)
    add_compile_options(-Werror=incompatible-pointer-types)
endif()

#==============================================================================
# Print Compiler Info
#==============================================================================
message(STATUS "")
message(STATUS "Compiler Configuration:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
