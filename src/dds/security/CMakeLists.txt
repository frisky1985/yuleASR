# CMakeLists.txt for DDS-Security module
# OMG DDS-Security Specification v1.1 Implementation

cmake_minimum_required(VERSION 3.10)

# DDS-Security library
add_library(dds_security STATIC
    dds_security_types.h
    dds_auth.h
    dds_auth.c
    dds_access.h
    dds_access.c
    dds_crypto.h
    dds_crypto.c
    dds_security_manager.h
    dds_security_manager.c
)

target_include_directories(dds_security
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common/types
)

target_link_libraries(dds_security
    PUBLIC
        dds_core
)

# Set compiler options for security-critical code
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dds_security PRIVATE
        -Wall
        -Wextra
        -Werror=return-type
        -Werror=uninitialized
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
        -O2
    )
    
    # Additional security hardening options
    target_link_options(dds_security PRIVATE
        -Wl,-z,relro,-z,now
    )
endif()

# Define security features
if(DDS_SECURITY_ENABLE_ASIL_D)
    target_compile_definitions(dds_security PRIVATE DDS_SECURITY_ASIL_D=1)
endif()

if(DDS_SECURITY_ENABLE_FIPS_MODE)
    target_compile_definitions(dds_security PRIVATE DDS_SECURITY_FIPS_MODE=1)
endif()

# Installation
install(TARGETS dds_security
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES
    dds_security_types.h
    dds_auth.h
    dds_access.h
    dds_crypto.h
    dds_security_manager.h
    DESTINATION include/dds/security
)

# Tests for security module
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
