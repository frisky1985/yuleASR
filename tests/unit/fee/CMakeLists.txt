# Fee (Flash EEPROM Emulation) unit tests
#
# Compiles the real production source src/bsw/ecual/fee/src/Fee.c against
# Fls/MemIf/Det stubs. Test cases live in test_fee_init.c / test_fee_read.c /
# test_fee_write.c — each file has its own main().

cmake_minimum_required(VERSION 3.16)

set(FEE_TEST_SOURCES
    test_fee_init.c
    test_fee_read.c
    test_fee_write.c
)

foreach(test_src ${FEE_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(target_name "Fee_${test_name}_UnitTest")

    add_executable(${target_name}
        ${CMAKE_CURRENT_SOURCE_DIR}/${test_src}
        ${CMAKE_CURRENT_SOURCE_DIR}/fee_test_support.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/bsw/ecual/fee/src/Fee.c
    )

    target_include_directories(${target_name}
        PRIVATE
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

    target_compile_options(${target_name} PRIVATE -Wall -Wextra -g -O0 -DUNIT_TEST)
    set_target_properties(${target_name} PROPERTIES
        C_STANDARD 99
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(${target_name} PRIVATE ${TEST_FRAMEWORK_LIB})
    endif()

    add_test(NAME ${target_name} COMMAND ${target_name})
    message(STATUS "  Unit test: ${target_name} (Fee)")
endforeach()
