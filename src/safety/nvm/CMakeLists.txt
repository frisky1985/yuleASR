# src/safety/nvm/CMakeLists.txt
# Non-Volatile Memory safety module

set(NVM_SOURCES
    nvm_core.c
    nvm_block_manager.c
    nvm_job_queue.c
    nvm_storage_if.c
    nvm_redundancy.c
)

set(NVM_HEADERS
    nvm.h
    nvm_types.h
    nvm_core.h
    nvm_block_manager.h
    nvm_job_queue.h
    nvm_storage_if.h
    nvm_redundancy.h
)

# Create NVM library
add_library(eth_dds_safety_nvm STATIC
    ${NVM_SOURCES}
)

target_include_directories(eth_dds_safety_nvm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

target_link_libraries(eth_dds_safety_nvm PUBLIC
    eth_dds_common
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(eth_dds_safety_nvm PRIVATE
        -Wall
        -Wextra
    )
endif()

# Install headers
install(FILES ${NVM_HEADERS}
    DESTINATION include/eth_dds/safety
    COMPONENT headers
)
