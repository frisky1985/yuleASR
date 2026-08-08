# FVM (Flash Virtual Memory) Unit Tests CMakeLists.txt
#
# B3-1 (2026-08-09): compiles the real production sources
# Cdd_Fvm_1.0.0.c + Cdd_Fvm_Hw.c (RAM backend) with a Det_ReportError
# mock, covering init lifecycle, bank registration/selection, read/write,
# erase, write protection, bank-to-bank copy and corrupt-active failover.
#
# Use: cmake -DBUILD_TESTING=ON .. && make CddFvm_UnitTest && ctest -R Fvm

cmake_minimum_required(VERSION 3.16)

set(FVM_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/fvm/test_fvm.c)
set(FVM_PRODUCT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/cdd/src/Cdd_Fvm_1.0.0.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/cdd/src/Cdd_Fvm_Hw.c
)

add_executable(CddFvm_UnitTest
    ${FVM_TEST_SRC}
    ${FVM_PRODUCT_SRC}
)

target_include_directories(CddFvm_UnitTest
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/..                       # test_framework.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/cdd/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/os/include
)

target_compile_options(CddFvm_UnitTest PRIVATE
    -Wall
    -Wextra
    -Werror
    -g
    -O0
    -DUNIT_TEST
    -DCDD_FVM_BACKEND=CDD_FVM_BACKEND_RAM
    # Cdd_MemMap.h uses ARMCC-style #pragma section (cross toolchain);
    # native Apple Clang does not know the pragma -> suppress the warning
    -Wno-unknown-pragmas
)

set_target_properties(CddFvm_UnitTest PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(CddFvm_UnitTest PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME CddFvm_UnitTest COMMAND CddFvm_UnitTest)

message(STATUS "  Unit test: CddFvm_UnitTest (B3-1 Flash Virtual Memory)")
