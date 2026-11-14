# P1 Phase 8 — performance benchmark suite (tests/performance).
#
# Three host executables, all compiled with -O2 (production-like codegen):
#   os_benchmark        — task context switch, IRQ disable/enable latency,
#                         Com MainFunction scheduling jitter (1 ms tick)
#   comstack_benchmark  — CanIf_RxIndication dispatch at 1/32/64 scanned
#                         entries, Com signal pack/unpack (8/16/32-bit LE+BE),
#                         PduR_Transmit routing, full Com->PduR->CanIf->Can
#                         transmit chain
#   memstack_benchmark  — NvM CRC lookup-table equivalence gate + throughput
#                         (8/64/256/1024/4096 B), NvM write/read job cycles
#
# Test-side pre-compile configuration:
#   - CanIf.c is compiled against the local shadow CanIf_Cfg.h
#     (CANIF_NUM_RX_PDUS=64U) via -include, to benchmark Rx dispatch over a
#     64-entry lookup bucket. The shadow header is force-included into every
#     TU of comstack_benchmark.
#   - memstack_benchmark #includes NvM.c directly (no service_nvm link) so
#     the STATIC CRC helpers stay reachable for the micro-benchmarks.
#
# All dependency stubs (Det, PduR down/up-link, Can driver, CanTrcv, CanSM,
# Dcm, RAM-backed MemIf) are defined inside the benchmark sources themselves.

cmake_minimum_required(VERSION 3.16)

set(PERF_CANIF_SHADOW_CFG ${CMAKE_CURRENT_SOURCE_DIR}/CanIf_Cfg.h)

# ------------------------------------------------------------------
# os_benchmark
# ------------------------------------------------------------------
set(PERF_OS_SOURCES
    test_os_benchmark.c
    ${CMAKE_SOURCE_DIR}/src/bsw/services/com/src/Com.c
    ${CMAKE_SOURCE_DIR}/src/bsw/mcal/mcu/src/Mcal.c
)
add_executable(os_benchmark ${PERF_OS_SOURCES})

target_include_directories(os_benchmark
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/com/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/pdur/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(os_benchmark PRIVATE
    -Wall
    -Wextra
    -O2
    # swapcontext/makecontext are marked deprecated on Apple hosts
    -Wno-deprecated-declarations
)

set_target_properties(os_benchmark PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(os_benchmark PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME os_benchmark COMMAND os_benchmark)
message(STATUS "  Benchmark: os_benchmark (task switch / IRQ latency / MainFunction jitter)")

# ------------------------------------------------------------------
# comstack_benchmark
# ------------------------------------------------------------------
set(PERF_COMSTACK_SOURCES
    test_comstack_benchmark.c
    ${CMAKE_SOURCE_DIR}/src/bsw/ecual/canif/src/CanIf.c
    ${CMAKE_SOURCE_DIR}/src/bsw/services/com/src/Com.c
)
add_executable(comstack_benchmark ${PERF_COMSTACK_SOURCES})
# Raw-flag string (NOT a list): keeps the "-include <file>" two-word pair
# intact for every TU of the target. List-based -include items get
# deduplicated/space-mangled by CMake.
set_source_files_properties(${PERF_COMSTACK_SOURCES} PROPERTIES
    COMPILE_FLAGS "-include ${PERF_CANIF_SHADOW_CFG}"
)

target_include_directories(comstack_benchmark
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/canif/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/cantrcv/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/can/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/dio/include
        ${CMAKE_SOURCE_DIR}/src/bsw/mcal/spi/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/ecum/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/pdur/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/com/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(comstack_benchmark PRIVATE
    -Wall
    -Wextra
    -O2
    # CanTrcv.h header noise (same as canif_test)
    -Wno-typedef-redefinition
    -Wno-macro-redefined
)

set_target_properties(comstack_benchmark PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_pdur)
    target_link_libraries(comstack_benchmark PRIVATE service_pdur)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(comstack_benchmark PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME comstack_benchmark COMMAND comstack_benchmark)
message(STATUS "  Benchmark: comstack_benchmark (CanIf dispatch / Com pack-unpack / PduR routing)")

# ------------------------------------------------------------------
# memstack_benchmark
# ------------------------------------------------------------------
add_executable(memstack_benchmark
    test_memstack_benchmark.c
)

target_include_directories(memstack_benchmark
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/nvm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/nvm/src
        ${CMAKE_SOURCE_DIR}/src/bsw/services/memif/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/fee/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/ea/include
)

target_compile_options(memstack_benchmark PRIVATE
    -Wall
    -Wextra
    -O2
    # NvM.c intentionally shadows the NVM_E_* macros from NvM.h (same
    # documented deviation the nvm_test tolerates); silence the noise.
    -Wno-macro-redefined
)

set_target_properties(memstack_benchmark PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(memstack_benchmark PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME memstack_benchmark COMMAND memstack_benchmark)
message(STATUS "  Benchmark: memstack_benchmark (NvM CRC tables / NvM job cycles)")

# ------------------------------------------------------------------
# P1-4: CI-required performance test binaries
# (build/bin/test_latency, test_throughput, test_memory)
# ------------------------------------------------------------------
add_executable(test_latency test_latency.c)
set_target_properties(test_latency PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
target_compile_options(test_latency PRIVATE -Wall -Wextra -O2)
add_test(NAME test_latency COMMAND test_latency)
message(STATUS "  Perf test: test_latency (BSW stack call latency)")

add_executable(test_throughput test_throughput.c)
set_target_properties(test_throughput PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
target_compile_options(test_throughput PRIVATE -Wall -Wextra -O2)
add_test(NAME test_throughput COMMAND test_throughput)
message(STATUS "  Perf test: test_throughput (BSW stack message throughput)")

add_executable(test_memory test_memory.c)
set_target_properties(test_memory PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
target_compile_options(test_memory PRIVATE -Wall -Wextra -O2)
add_test(NAME test_memory COMMAND test_memory)
message(STATUS "  Perf test: test_memory (NvM block memory profiling)")
