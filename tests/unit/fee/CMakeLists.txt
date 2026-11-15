# Fee (Flash EEPROM Emulation) unit tests
#
# 两个编路径:
#   test_fee_*.c (api_ext/init/read/write) → 直接编 Fee.c + fee_test_support.c
#   test_fee_mainloop.c                    → 编 fee_expose.c（已用 #define static include Fee.c）
#                                              + fee_test_support.c（不重复编 Fee.c）

cmake_minimum_required(VERSION 3.16)

set(FEE_TEST_FRAMEWORK_INCLUDE
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/fee/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/memif/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/mcal/fls/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/services/det/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/platform/s32k312/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/autosar
)

set(FEE_TEST_COMMON_OPTS -Wall -Wextra -g -O0 -DUNIT_TEST)

# ---- Group 1: direct Fee.c compile (api_ext/init/read/write) ----
set(FEE_DIRECT_SOURCES
    test_fee_init.c
    test_fee_read.c
    test_fee_write.c
    test_fee_api_ext.c
)

foreach(test_src ${FEE_DIRECT_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(target_name "Fee_${test_name}_UnitTest")

    add_executable(${target_name}
        ${CMAKE_CURRENT_SOURCE_DIR}/${test_src}
        ${CMAKE_CURRENT_SOURCE_DIR}/fee_test_support.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/fee/src/Fee.c
    )

    target_include_directories(${target_name} PRIVATE ${FEE_TEST_FRAMEWORK_INCLUDE})
    target_compile_options(${target_name} PRIVATE ${FEE_TEST_COMMON_OPTS})
    set_target_properties(${target_name} PROPERTIES
        C_STANDARD 99
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(${target_name} PRIVATE ${TEST_FRAMEWORK_LIB})
    endif()

    add_test(NAME ${target_name} COMMAND ${target_name})
    message(STATUS "  Unit test: ${target_name} (Fee direct)")
endforeach()

# ---- Group 2: mainloop driver (fee_expose includes Fee.c with static exposed) ----
set(target_name "Fee_mainloop_UnitTest")

add_executable(${target_name}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_fee_mainloop.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fee_test_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fee_expose.c     # 内部 #include Fee.c
)

target_include_directories(${target_name} PRIVATE ${FEE_TEST_FRAMEWORK_INCLUDE})
target_compile_options(${target_name} PRIVATE ${FEE_TEST_COMMON_OPTS})
set_target_properties(${target_name} PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(${target_name} PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

# ---- Group 3: GC driver (fee_expose already includes Fee.c) ----
set(target_name "Fee_gc_UnitTest")

add_executable(${target_name}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_fee_gc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fee_test_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fee_expose.c     # 内部 #include Fee.c
)

target_include_directories(${target_name} PRIVATE ${FEE_TEST_FRAMEWORK_INCLUDE})
target_compile_options(${target_name} PRIVATE ${FEE_TEST_COMMON_OPTS})
set_target_properties(${target_name} PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(${target_name} PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ${target_name} COMMAND ${target_name})
message(STATUS "  Unit test: ${target_name} (Fee GC driver)")
