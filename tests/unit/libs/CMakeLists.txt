# Libraries Unit Tests CMakeLists.txt
#
# B3-2 (2026-08-09): independent algorithm library tests (XMEN
# Libraries/ 对齐).  Links the real libs_crc / libs_aes targets:
#   - LibCrc_UnitTest: catalogue check values + streaming equivalence
#   - LibAes_UnitTest: NIST FIPS-197 (ECB) + SP 800-38A (CBC) vectors
#
# Use: cmake -DBUILD_TESTING=ON .. && make LibCrc_UnitTest LibAes_UnitTest

cmake_minimum_required(VERSION 3.16)

# ── CRC ─────────────────────────────────────────────────────────────────
add_executable(LibCrc_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_lib_crc.c
)
target_include_directories(LibCrc_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/..                       # test_framework.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/libs/crc/include
)
target_compile_options(LibCrc_UnitTest PRIVATE
    -Wall -Wextra -Werror -g -O0
)
set_target_properties(LibCrc_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
if(TARGET libs_crc)
    target_link_libraries(LibCrc_UnitTest PRIVATE libs_crc)
endif()
if(TEST_FRAMEWORK_LIB)
    target_link_libraries(LibCrc_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()
add_test(NAME LibCrc_UnitTest COMMAND LibCrc_UnitTest)
message(STATUS "  Unit test: LibCrc_UnitTest (B3-2 CRC algorithm library)")

# ── AES ─────────────────────────────────────────────────────────────────
add_executable(LibAes_UnitTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_lib_aes.c
)
target_include_directories(LibAes_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/..                       # test_framework.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/libs/aes/include
)
target_compile_options(LibAes_UnitTest PRIVATE
    -Wall -Wextra -Werror -g -O0
)
set_target_properties(LibAes_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
if(TARGET libs_aes)
    target_link_libraries(LibAes_UnitTest PRIVATE libs_aes)
endif()
if(TEST_FRAMEWORK_LIB)
    target_link_libraries(LibAes_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()
add_test(NAME LibAes_UnitTest COMMAND LibAes_UnitTest)
message(STATUS "  Unit test: LibAes_UnitTest (B3-2 AES algorithm library)")
