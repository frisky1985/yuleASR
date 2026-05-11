# CMakeLists.txt for Hash Algorithms Library

cmake_minimum_required(VERSION 3.14)

# Library target
add_library(HashAlgos STATIC
    src/sha1.c
    src/sha224.c
    src/sha256.c
    src/sha384.c
    src/sha512.c
    src/hash_autosar.c
)

target_include_directories(HashAlgos
    PUBLIC
        include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include/autosar/classic/crypto
)

# Compiler options
target_compile_options(HashAlgos PRIVATE
    -Wall
    -Wextra
    -O2
)

# Enable specific SHA algorithms
option(HASH_ENABLE_SHA1 "Enable SHA-1 support" ON)
option(HASH_ENABLE_SHA224 "Enable SHA-224 support" ON)
option(HASH_ENABLE_SHA256 "Enable SHA-256 support" ON)
option(HASH_ENABLE_SHA384 "Enable SHA-384 support" ON)
option(HASH_ENABLE_SHA512 "Enable SHA-512 support" ON)

if(HASH_ENABLE_SHA1)
    target_compile_definitions(HashAlgos PRIVATE HASH_SHA1_ENABLED)
endif()

if(HASH_ENABLE_SHA224)
    target_compile_definitions(HashAlgos PRIVATE HASH_SHA224_ENABLED)
endif()

if(HASH_ENABLE_SHA256)
    target_compile_definitions(HashAlgos PRIVATE HASH_SHA256_ENABLED)
endif()

if(HASH_ENABLE_SHA384)
    target_compile_definitions(HashAlgos PRIVATE HASH_SHA384_ENABLED)
endif()

if(HASH_ENABLE_SHA512)
    target_compile_definitions(HashAlgos PRIVATE HASH_SHA512_ENABLED)
endif()

# Tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
