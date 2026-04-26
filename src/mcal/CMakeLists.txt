# MCAL CMake Configuration
# Storage (Fls + Fee), Ethernet (Eth), and Microcontroller (Mcu)

cmake_minimum_required(VERSION 3.10)

#=============================================================================#
# MCAL Storage Library (Fls + Fee)
#=============================================================================#
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

set_target_properties(mcal_storage PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

#=============================================================================#
# MCAL Ethernet Library (Eth)
#=============================================================================#
add_library(mcal_eth STATIC
    eth/Eth.c
    eth/Eth_Irq.c
)

target_include_directories(mcal_eth
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/mcal
)

target_compile_options(mcal_eth PRIVATE
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

set_target_properties(mcal_eth PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

#=============================================================================#
# MCAL MCU Library (Mcu)
#=============================================================================#
add_library(mcal_mcu STATIC
    mcu/Mcu.c
    mcu/Mcu_RamInit.c
)

target_include_directories(mcal_mcu
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/mcal
)

target_compile_options(mcal_mcu PRIVATE
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

set_target_properties(mcal_mcu PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

#=============================================================================#
# Combined MCAL Interface Library
#=============================================================================#
add_library(mcal INTERFACE)
target_link_libraries(mcal INTERFACE
    mcal_storage
    mcal_eth
    mcal_mcu
)

#=============================================================================#
# Unit Tests
#=============================================================================#
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

    # Eth Tests
    add_executable(test_eth
        ../../tests/unit/mcal/test_eth.c
    )
    target_link_libraries(test_eth
        mcal_eth
    )
    target_include_directories(test_eth PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/mcal
    )
    add_test(NAME eth_unit_tests COMMAND test_eth)

    # Mcu Tests
    add_executable(test_mcu
        ../../tests/unit/mcal/test_mcu.c
    )
    target_link_libraries(test_mcu
        mcal_mcu
    )
    target_include_directories(test_mcu PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/mcal
    )
    add_test(NAME mcu_unit_tests COMMAND test_mcu)
endif()

#=============================================================================#
# Installation
#=============================================================================#
install(TARGETS mcal_storage mcal_eth mcal_mcu
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# Storage headers
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

# Ethernet headers
install(FILES
    ../../include/mcal/eth/Eth.h
    ../../include/mcal/eth/Eth_Types.h
    ../../include/mcal/eth/Eth_Cfg.h
    DESTINATION include/mcal/eth
)

# MCU headers
install(FILES
    ../../include/mcal/mcu/Mcu.h
    ../../include/mcal/mcu/Mcu_Types.h
    ../../include/mcal/mcu/Mcu_Cfg.h
    DESTINATION include/mcal/mcu
)
