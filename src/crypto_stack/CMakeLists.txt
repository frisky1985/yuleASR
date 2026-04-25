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
)

# Link with DDS security if available
target_link_libraries(crypto_stack PUBLIC
    dds_security
)

# Compiler flags for security
target_compile_options(crypto_stack PRIVATE
    -Wall
    -Wextra
    -Werror=implicit-function-declaration
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    
    # CSM Tests
    add_executable(test_csm
        tests/test_csm.c
    )
    target_link_libraries(test_csm PRIVATE crypto_stack)
    add_test(NAME CSM_Tests COMMAND test_csm)
    
    # CryIf Tests
    add_executable(test_cryif
        tests/test_cryif.c
    )
    target_link_libraries(test_cryif PRIVATE crypto_stack)
    add_test(NAME CryIf_Tests COMMAND test_cryif)
    
    # KeyM Tests
    add_executable(test_keym
        tests/test_keym.c
    )
    target_link_libraries(test_keym PRIVATE crypto_stack)
    add_test(NAME KeyM_Tests COMMAND test_keym)
endif()
