# Crypto MCAL host unit tests
# ---------------------------------------------------------------------------
# - Crypto_MbedTLS_UnitTest : software backend (links mbedcrypto)
# - Crypto_Hsm_UnitTest     : HSM stub backend (white-box, no mbedTLS symbols)
#
# Note: `mbedcrypto` is created by third_party/mbedtls before src/ is added,
# and host builds append tests/mock/mbedtls_host_entropy.c to it (see the
# root CMakeLists.txt), so mbedtls_ctr_drbg_seed() works on Linux/macOS.

set(CRYPTO_MCAL_ROOT    ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(CRYPTO_MCAL_INCLUDE ${CRYPTO_MCAL_ROOT}/include)
set(CRYPTO_UNITY_DIR    ${CMAKE_SOURCE_DIR}/tests/unit/framework)

set(CRYPTO_COMMON_INCLUDES
    ${CRYPTO_MCAL_INCLUDE}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CRYPTO_UNITY_DIR}
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/third_party/crypto/blake2/include
    ${CMAKE_SOURCE_DIR}/third_party/mbedtls/include
    ${CMAKE_SOURCE_DIR}/third_party/mbedtls/configs
)

set(CRYPTO_TEST_WARNINGS
    -Wall -Wextra -g -O0
    -Wno-unused-parameter
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-sign-conversion
    -Wno-conversion
    -Wno-missing-prototypes
)

# ---------------------------------------------------------------------------
# mbedTLS software backend
# ---------------------------------------------------------------------------
add_executable(Crypto_MbedTLS_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_crypto_mbedtls.c
    ${CRYPTO_MCAL_ROOT}/src/Crypto_MbedTLS.c
    ${CRYPTO_MCAL_ROOT}/src/Crypto_MbedTLS_Mem.c
    ${CRYPTO_UNITY_DIR}/unity.c
)

target_include_directories(Crypto_MbedTLS_UnitTest BEFORE PRIVATE ${CRYPTO_MCAL_INCLUDE})
target_include_directories(Crypto_MbedTLS_UnitTest PRIVATE ${CRYPTO_COMMON_INCLUDES})
target_compile_definitions(Crypto_MbedTLS_UnitTest PRIVATE MBEDTLS_ALLOW_PRIVATE_ACCESS)
target_compile_options(Crypto_MbedTLS_UnitTest PRIVATE ${CRYPTO_TEST_WARNINGS})
target_link_libraries(Crypto_MbedTLS_UnitTest PRIVATE mbedcrypto)
set_target_properties(Crypto_MbedTLS_UnitTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME Crypto_MbedTLS_UnitTest COMMAND Crypto_MbedTLS_UnitTest)

# ---------------------------------------------------------------------------
# HSM backend (stub implementation)
# ---------------------------------------------------------------------------
add_executable(Crypto_Hsm_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_crypto_hsm.c
    ${CRYPTO_UNITY_DIR}/unity.c
)

target_include_directories(Crypto_Hsm_UnitTest BEFORE PRIVATE ${CRYPTO_MCAL_INCLUDE})
target_include_directories(Crypto_Hsm_UnitTest PRIVATE ${CRYPTO_COMMON_INCLUDES})
target_compile_options(Crypto_Hsm_UnitTest PRIVATE ${CRYPTO_TEST_WARNINGS})
set_target_properties(Crypto_Hsm_UnitTest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME Crypto_Hsm_UnitTest COMMAND Crypto_Hsm_UnitTest)
