# Crypto Stack CMakeLists.txt
# AUTOSAR Crypto Stack (SecOC/CSM/CryIf/KeyM)

cmake_minimum_required(VERSION 3.10)

# SecOC Library
add_library(secoc STATIC
    secoc/secoc_core.c
    secoc/secoc_freshness.c
    secoc/secoc_dds_integration.c
)

target_include_directories(secoc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/secoc
    ${CMAKE_SOURCE_DIR}/src/dds/security
    ${CMAKE_SOURCE_DIR}/src/dds/rtps
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(secoc PUBLIC
    dds_security
)

# CSM Library (stub for future implementation)
add_library(csm STATIC
    csm/csm_core.c
)

target_include_directories(csm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/csm
    ${CMAKE_SOURCE_DIR}/src/common
)

# CryIf Library (stub for future implementation)
add_library(cryif STATIC
    cryif/cryif_core.c
)

target_include_directories(cryif PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cryif
    ${CMAKE_SOURCE_DIR}/src/common
)

# KeyM Library (stub for future implementation)
add_library(keym STATIC
    keym/keym_core.c
)

target_include_directories(keym PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/keym
    ${CMAKE_SOURCE_DIR}/src/common
)

# Combined Crypto Stack
add_library(crypto_stack INTERFACE)
target_link_libraries(crypto_stack INTERFACE
    secoc
    csm
    cryif
    keym
)

# Tests
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
