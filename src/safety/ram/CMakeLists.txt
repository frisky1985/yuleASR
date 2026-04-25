# src/safety/ram/CMakeLists.txt
# RAM ECC (SECDED) safety module

set(RAM_ECC_SOURCES
    ecc_encoder.c
    ecc_checker.c
    ram_monitor.c
    ecc_allocator.c
    ram_safety.c
)

set(RAM_ECC_HEADERS
    ecc_encoder.h
    ecc_checker.h
    ram_monitor.h
    ecc_allocator.h
    ram_safety.h
)

# Create RAM ECC library
add_library(eth_dds_safety_ram STATIC
    ${RAM_ECC_SOURCES}
)

target_include_directories(eth_dds_safety_ram PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

target_link_libraries(eth_dds_safety_ram PUBLIC
    eth_dds_common
)

# ASIL-D requires strict compilation flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(eth_dds_safety_ram PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wstrict-overflow
        -fno-strict-aliasing
    )
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    
    # Collect test sources
    file(GLOB RAM_ECC_TEST_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c
    )
    
    if(RAM_ECC_TEST_SOURCES)
        add_executable(test_ram_ecc
            ${RAM_ECC_TEST_SOURCES}
        )
        
        target_link_libraries(test_ram_ecc
            eth_dds_safety_ram
            unity
        )
        
        add_test(NAME ram_ecc_unit COMMAND test_ram_ecc)
    endif()
endif()

# Install headers
install(FILES ${RAM_ECC_HEADERS}
    DESTINATION include/eth_dds/safety
    COMPONENT headers
)
