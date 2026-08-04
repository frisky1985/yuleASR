# Crypto Stack CMake Configuration
# Includes CSM, CryIf, KeyM, and SecOC modules

cmake_minimum_required(VERSION 3.10)

# Crypto Stack library
add_library(crypto_stack STATIC
    # CSM (Crypto Services Manager)
    csm/csm_core.c
    csm/csm_jobs.c
    
    # CryIf (Crypto Interface)
    cryif/cryif_core.c
    
    # KeyM (Key Manager)
    keym/keym_core.c
    
    # SecOC (existing)
    secoc/secoc_core.c
    secoc/secoc_freshness.c
    secoc/secoc_dds_integration.c
)

# Include directories
target_include_directories(crypto_stack PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/csm
    ${CMAKE_CURRENT_SOURCE_DIR}/cryif
    ${CMAKE_CURRENT_SOURCE_DIR}/keym
    ${CMAKE_CURRENT_SOURCE_DIR}/secoc
    ${CMAKE_CURRENT_SOURCE_DIR}/../dds/security
    ${CMAKE_CURRENT_SOURCE_DIR}/../dds/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/types
)

# Link with DDS middleware (eth_dds_middleware provides dds_crypto/dds_security types)
target_link_libraries(crypto_stack PUBLIC
    eth_dds_middleware
)

# Compiler flags for security
target_compile_options(crypto_stack PRIVATE
    -Wall
    -Wextra
    -Werror=implicit-function-declaration
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
)

# Tests (顶层 BUILD_TESTING 门控, 由 tests/CMakeLists.txt 定义)
if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
