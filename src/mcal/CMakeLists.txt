# MCAL Storage Stack CMake Configuration
# Fls + Fee (Flash EEPROM Emulation)

cmake_minimum_required(VERSION 3.10)

# MCAL Storage Library
add_library(mcal_storage STATIC
    fls/fls.c
    fls/fls_hw_emulator.c
    fls/fls_hw_aurix.c
    fls/fls_hw_s32g3.c
    fee/fee.c
    fee/fee_gc.c
    fee/fee_wl.c
    mcal_storage.c
)

target_include_directories(mcal_storage
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/mcal
)

# Compiler flags for MISRA C compliance
target_compile_options(mcal_storage PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wcast-align
    -Wunused
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -O2
)

# Set C standard
set_target_properties(mcal_storage PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

# Unit tests
if(BUILD_TESTING)
    # Fls Tests
    add_executable(test_fls
        ../../tests/unit/mcal/test_fls.c
    )
    target_link_libraries(test_fls
        mcal_storage
    )
    target_include_directories(test_fls PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/mcal
    )
    add_test(NAME fls_unit_tests COMMAND test_fls)

    # Fee Tests
    add_executable(test_fee
        ../../tests/unit/mcal/test_fee.c
    )
    target_link_libraries(test_fee
        mcal_storage
    )
    target_include_directories(test_fee PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/mcal
    )
    add_test(NAME fee_unit_tests COMMAND test_fee)
endif()

# Installation
install(TARGETS mcal_storage
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES
    ../../include/mcal/mcal_storage.h
    ../../include/mcal/fls/fls.h
    ../../include/mcal/fls/fls_types.h
    ../../include/mcal/fee/fee.h
    ../../include/mcal/fee/fee_types.h
    ../../include/mcal/fee/fee_gc.h
    ../../include/mcal/fee/fee_wl.h
    DESTINATION include/mcal
)
