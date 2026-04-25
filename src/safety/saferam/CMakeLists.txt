# src/safety/saferam/CMakeLists.txt
# SafeRAM memory protection module

set(SAFERAM_SOURCES
    saferam_manager.c
    ram_partition.c
    stack_protection.c
    heap_protection.c
    safe_data.c
    fault_injection.c
)

set(SAFERAM_HEADERS
    saferam.h
    saferam_manager.h
    ram_partition.h
    stack_protection.h
    heap_protection.h
    safe_data.h
    fault_injection.h
)

# Create SafeRAM library
add_library(eth_dds_safety_saferam STATIC
    ${SAFERAM_SOURCES}
)

target_include_directories(eth_dds_safety_saferam PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

target_link_libraries(eth_dds_safety_saferam PUBLIC
    eth_dds_common
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(eth_dds_safety_saferam PRIVATE
        -Wall
        -Wextra
    )
endif()

# Install headers
install(FILES ${SAFERAM_HEADERS}
    DESTINATION include/eth_dds/safety
    COMPONENT headers
)
